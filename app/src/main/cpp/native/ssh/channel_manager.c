#include "channel.h"
#include "../session/session.h"
#include "../common/buffer.h"
#include "../common/utils.h"
#include "../keepalive/keepalive.h"
#include "../transport/websocket.h"
#include "../proxy/proxy_connect.h"
#include "../transport/tcp_transport.h"
#include "../common/error.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 16384

typedef enum {
    SOCKS_GREETING,
    SOCKS_REQUEST,
    HTTP_PROXY_REQUEST,
    SOCKS_OPEN_CHANNEL,
    SOCKS_BRIDGING,
    SOCKS_CLOSED
} SocksState;

typedef struct ChannelNode {
    ChannelContext ctx;

    
    EventContext *ev_ctx;
    SocksState state;
    
    
    
    
    
    
    
    
    uint8_t rx_buf[BUF_SIZE];
    size_t rx_len;
    size_t rx_offset;
    
    uint8_t tx_buf[BUF_SIZE];
    size_t tx_len;
    size_t tx_offset;
    
    bool local_eof;
    bool remote_eof;
    bool ssh_eof_sent;
    
    struct ChannelNode *next;
} ChannelNode;

static int g_socks_server_fd = -1;
static EventContext *g_socks_ev_ctx = NULL;
static int g_http_proxy_fd = -1;
static EventContext *g_http_ev_ctx = NULL;

static ChannelNode *g_channels = NULL;

static void close_channel(ChannelNode *node) {
    if (node->state == SOCKS_CLOSED) return;
    node->state = SOCKS_CLOSED;
    
    if (node->ev_ctx) {
        ev_del(node->ev_ctx);
        node->ev_ctx = NULL;
    }
    if (node->ctx.socket_fd != -1) {
        close(node->ctx.socket_fd);
        node->ctx.socket_fd = -1;
    }
}

static void free_closed_channels(void) {
    ChannelNode **curr = &g_channels;
    while (*curr) {
        if ((*curr)->state == SOCKS_CLOSED) {
            if ((*curr)->ssh_channel) {
                libssh2_channel_free((*curr)->ssh_channel);
                (*curr)->ssh_channel = NULL;
            }
            if ((*curr)->rx_buffer) {
                buffer_destroy((*curr)->rx_buffer);
                (*curr)->rx_buffer = NULL;
            }
            if ((*curr)->tx_buffer) {
                buffer_destroy((*curr)->tx_buffer);
                (*curr)->tx_buffer = NULL;
            }
            ChannelNode *temp = *curr;
            *curr = (*curr)->next;
            free(temp);
        } else {
            curr = &(*curr)->next;
        }
    }
}

static void process_socks_greeting(ChannelNode *node) {
    size_t tx_left = node->ctx.tx_buffer ? buffer_get_readable_size(node->ctx.tx_buffer) : node->tx_len;
    if (tx_left > 0) return; // Wait for previous write
    
    uint8_t buf[256];
    ssize_t n = tcp_transport_read(node->ctx.socket_fd, buf, sizeof(buf));
    if (n <= 0) {
        if (n != ENGINE_AGAIN && n != ENGINE_INTR) close_channel(node);
        return;
    }
    
    if (buf[0] != 5) {
        close_channel(node);
        return;
    }
    
    // Reply NO AUTH REQUIRED
    node->rx_buf[0] = 5;
    node->rx_buf[1] = 0;
    tcp_transport_write(node->ctx.socket_fd, node->rx_buf, 2);
    
    node->state = SOCKS_REQUEST;
}

static void process_socks_request(ChannelNode *node) {
    uint8_t buf[512];
    ssize_t n = tcp_transport_peek(node->ctx.socket_fd, buf, sizeof(buf));
    if (n <= 0) {
        if (n != ENGINE_AGAIN && n != ENGINE_INTR) close_channel(node);
        return;
    }
    
    if (n < 4) return; // Need more data
    
    int atyp = buf[3];
    int req_len = 4;
    
    if (atyp == 1) req_len += 4 + 2; // IPv4 + Port
    else if (atyp == 3) req_len += 1 + buf[4] + 2; // Domain len + Domain + Port
    else if (atyp == 4) req_len += 16 + 2; // IPv6 + Port
    else { close_channel(node); return; }
    
    if (n < req_len) return; // Wait for full request
    
    // Consume request
    tcp_transport_read(node->ctx.socket_fd, buf, req_len);
    
    if (buf[1] != 1) { // Only CONNECT
        close_channel(node);
        return;
    }
    
    if (atyp == 1) {
        snprintf(node->ctx.hostname, sizeof(node->ctx.hostname), "%d.%d.%d.%d", buf[4], buf[5], buf[6], buf[7]);
        node->ctx.port = (buf[8] << 8) | buf[9];
    } else if (atyp == 3) {
        int dlen = buf[4];
        memcpy(node->ctx.hostname, buf + 5, dlen);
        node->ctx.hostname[dlen] = '\0';
        node->ctx.port = (buf[5+dlen] << 8) | buf[6+dlen];
    }
    
    node->state = SOCKS_OPEN_CHANNEL;
    sm_request_write();
}

static void process_channel_bridge(ChannelNode *node) {
    if (!node->ctx.ssh_channel || node->state != SOCKS_BRIDGING) return;
    
    // Read from local socket into tx_buffer
    if (!node->local_eof) {
        if (node->ctx.tx_buffer && buffer_get_readable_size(node->ctx.tx_buffer) < BUF_SIZE) {
            uint8_t temp[4096];
            ssize_t n = tcp_transport_read(node->ctx.socket_fd, temp, sizeof(temp));
            if (n > 0) {
                buffer_append(node->ctx.tx_buffer, temp, (size_t)n);
                sm_request_write();
            } else if (n == 0 || (n < 0 && n != ENGINE_AGAIN && n != ENGINE_INTR)) {
                node->local_eof = true;
                sm_request_write();
            }
        } else if (!node->ctx.tx_buffer && node->tx_len == 0) {
            ssize_t n = tcp_transport_read(node->ctx.socket_fd, node->tx_buf, BUF_SIZE);
            if (n > 0) {
                node->tx_len = n;
                node->tx_offset = 0;
                sm_request_write();
            } else if (n == 0 || (n < 0 && n != ENGINE_AGAIN && n != ENGINE_INTR)) {
                node->local_eof = true;
                sm_request_write();
            }
        }
    }
    
    // Write to local socket from rx_buffer
    if (node->ctx.rx_buffer && buffer_get_readable_size(node->ctx.rx_buffer) > 0) {
        size_t avail = buffer_get_readable_size(node->ctx.rx_buffer);
        uint8_t *ptr = buffer_get_read_ptr(node->ctx.rx_buffer);
        ssize_t n = tcp_transport_write(node->ctx.socket_fd, ptr, avail);
        if (n > 0) {
            buffer_consume(node->ctx.rx_buffer, (size_t)n);
        } else if (n < 0 && n != ENGINE_AGAIN && n != ENGINE_INTR) {
            node->local_eof = true;
        }
    } else if (node->rx_len > 0) {
        ssize_t n = tcp_transport_write(node->ctx.socket_fd, node->rx_buf + node->rx_offset, node->rx_len);
        if (n > 0) {
            node->rx_offset += n;
            node->rx_len -= n;
        } else if (n < 0 && n != ENGINE_AGAIN && n != ENGINE_INTR) {
            node->local_eof = true;
        }
    }
    
    size_t tx_left = node->ctx.tx_buffer ? buffer_get_readable_size(node->ctx.tx_buffer) : node->tx_len;
    size_t rx_left = node->ctx.rx_buffer ? buffer_get_readable_size(node->ctx.rx_buffer) : node->rx_len;
    
    if (node->local_eof && node->remote_eof && tx_left == 0 && rx_left == 0) {
        close_channel(node);
    }
}

static void process_http_proxy_request(ChannelNode *node) {
    uint8_t buf[2048];
    ssize_t n = tcp_transport_peek(node->ctx.socket_fd, buf, sizeof(buf) - 1);
    if (n <= 0) {
        if (n != ENGINE_AGAIN && n != ENGINE_INTR) close_channel(node);
        return;
    }
    buf[n] = '\0';

    char *end_headers = strstr((char*)buf, "\r\n\r\n");
    if (!end_headers) {
        if (n >= sizeof(buf) - 1) close_channel(node); // Too large
        return; // Wait for more
    }
    int req_len = (end_headers - (char*)buf) + 4;
    
    // Consume request
    tcp_transport_read(node->ctx.socket_fd, buf, req_len);
    buf[req_len] = '\0';
    
    // Parse
    char method[16], url[512], protocol[16];
    if (sscanf((char*)buf, "%15s %511s %15s", method, url, protocol) != 3) {
        close_channel(node);
        return;
    }
    
    if (strcmp(method, "CONNECT") == 0) {
        // url is host:port
        char *colon = strrchr(url, ':');
        if (colon) {
            *colon = '\0';
            strncpy(node->ctx.hostname, url, sizeof(node->ctx.hostname) - 1);
            node->ctx.port = atoi(colon + 1);
        } else {
            strncpy(node->ctx.hostname, url, sizeof(node->ctx.hostname) - 1);
            node->ctx.port = 443;
        }
        
        // Reply 200 OK
        const char *reply = "HTTP/1.1 200 Connection Established\r\n\r\n";
        tcp_transport_write(node->ctx.socket_fd, reply, strlen(reply));
        
        node->state = SOCKS_OPEN_CHANNEL;
        sm_request_write();
    } else {
        // Rewrite to origin form (GET /path HTTP/1.1)
        char *host = strstr(url, "://");
        char *path = "/";
        if (host) {
            host += 3;
            char *slash = strchr(host, '/');
            if (slash) {
                path = slash;
                *slash = '\0'; // Split host and path
            } else {
                path = "/";
            }
            char *colon = strchr(host, ':');
            if (colon) {
                *colon = '\0';
                node->ctx.port = atoi(colon + 1);
            } else {
                node->ctx.port = 80;
            }
            strncpy(node->ctx.hostname, host, sizeof(node->ctx.hostname) - 1);
        } else {
            close_channel(node);
            return;
        }
        
        // Find end of first line
        char *first_line_end = strstr((char*)buf, "\r\n");
        char *headers = first_line_end ? first_line_end + 2 : "";
        
        // Write rewritten request into tx_buffer so it's sent to SSH
        char req_buf[2048];
        int len = snprintf(req_buf, sizeof(req_buf), "%s %s %s\r\n%s", method, path, protocol, headers);
        if (node->ctx.tx_buffer) {
            buffer_append(node->ctx.tx_buffer, (const uint8_t*)req_buf, (size_t)len);
        } else {
            memcpy(node->tx_buf, req_buf, len);
            node->tx_len = len;
            node->tx_offset = 0;
        }
        
        node->state = SOCKS_OPEN_CHANNEL;
        sm_request_write();
    }
}

static void client_event_cb(EventContext *ctx, uint32_t events) {
    ChannelNode *node = (ChannelNode*)ctx->data;
    if (events & EVENT_ERROR) {
        close_channel(node);
        return;
    }
    
    if (node->state == SOCKS_GREETING) process_socks_greeting(node);
    else if (node->state == SOCKS_REQUEST) process_socks_request(node);
    else if (node->state == HTTP_PROXY_REQUEST) process_http_proxy_request(node);
    else if (node->state == SOCKS_BRIDGING) process_channel_bridge(node);
}

static void server_event_cb(EventContext *ctx, uint32_t events) {
    if (events & EVENT_READ) {
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        int client_fd = accept(ctx->fd, (struct sockaddr*)&addr, &len);
        if (client_fd >= 0) {
            set_nonblock(client_fd);
            
            ChannelNode *node = (ChannelNode*)calloc(1, sizeof(ChannelNode));
            node->ctx.socket_fd = client_fd;
            node->state = SOCKS_GREETING;
            node->ctx.rx_buffer = buffer_create(BUF_SIZE);
            node->ctx.tx_buffer = buffer_create(BUF_SIZE);
            node->ev_ctx = ev_add(client_fd, EVENT_READ | EVENT_WRITE, client_event_cb, node);
            
            if (!node->ev_ctx) {
                close(client_fd);
                if (node->ctx.rx_buffer) buffer_destroy(node->ctx.rx_buffer);
                if (node->ctx.tx_buffer) buffer_destroy(node->ctx.tx_buffer);
                free(node);
            } else {
                node->next = g_channels;
                g_channels = node;
            }
        }
    }
}

static void server_http_event_cb(EventContext *ctx, uint32_t events) {
    if (events & EVENT_READ) {
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        int client_fd = accept(ctx->fd, (struct sockaddr*)&addr, &len);
        if (client_fd >= 0) {
            set_nonblock(client_fd);
            
            ChannelNode *node = (ChannelNode*)calloc(1, sizeof(ChannelNode));
            node->ctx.socket_fd = client_fd;
            node->state = HTTP_PROXY_REQUEST;
            node->ctx.rx_buffer = buffer_create(BUF_SIZE);
            node->ctx.tx_buffer = buffer_create(BUF_SIZE);
            node->ev_ctx = ev_add(client_fd, EVENT_READ | EVENT_WRITE, client_event_cb, node);
            
            if (!node->ev_ctx) {
                close(client_fd);
                if (node->ctx.rx_buffer) buffer_destroy(node->ctx.rx_buffer);
                if (node->ctx.tx_buffer) buffer_destroy(node->ctx.tx_buffer);
                free(node);
            } else {
                node->next = g_channels;
                g_channels = node;
            }
        }
    }
}

void cm_init(void) {
    g_channels = NULL;
    keepalive_start();
}

void cm_start_socks_server(int port) {
    g_socks_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_socks_server_fd < 0) return;
    
    int opt = 1;
    setsockopt(g_socks_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    set_nonblock(g_socks_server_fd);
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    
    if (bind(g_socks_server_fd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        if (listen(g_socks_server_fd, 100) == 0) {
            g_socks_ev_ctx = ev_add(g_socks_server_fd, EVENT_READ, server_event_cb, NULL);
            LOGI("SOCKS5 server started on port %d", port);
            return;
        }
    }
    close(g_socks_server_fd);
    g_socks_server_fd = -1;
}

void cm_stop_socks_server(void) {
    if (g_socks_ev_ctx) {
        ev_del(g_socks_ev_ctx);
        g_socks_ev_ctx = NULL;
    }
    if (g_socks_server_fd != -1) {
        close(g_socks_server_fd);
        g_socks_server_fd = -1;
    }
}

void cm_start_http_proxy(int port) {
    if (g_http_proxy_fd != -1) return;
    g_http_proxy_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_http_proxy_fd < 0) return;
    
    int opt = 1;
    setsockopt(g_http_proxy_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    set_nonblock(g_http_proxy_fd);
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (bind(g_http_proxy_fd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        if (listen(g_http_proxy_fd, 100) == 0) {
            g_http_ev_ctx = ev_add(g_http_proxy_fd, EVENT_READ, server_http_event_cb, NULL);
            LOGI("HTTP Proxy server started on port %d", port);
            return;
        }
    }
    close(g_http_proxy_fd);
    g_http_proxy_fd = -1;
}

void cm_stop_http_proxy(void) {
    if (g_http_ev_ctx) {
        ev_del(g_http_ev_ctx);
        g_http_ev_ctx = NULL;
    }
    if (g_http_proxy_fd != -1) {
        close(g_http_proxy_fd);
        g_http_proxy_fd = -1;
    }
}

void cm_process_channels(void) {
    if (!sm_is_ready()) return;
    LIBSSH2_SESSION *session = sm_get_session();
    
    // Periodic KeepAlive check on SSH session
    keepalive_periodic(-1, session, NULL, false, NULL, KEEPALIVE_TYPE_SSH_IGNORE);
    
    ChannelNode *node = g_channels;
    while (node) {
        if (node->state == SOCKS_OPEN_CHANNEL) {
            node->ctx.ssh_channel = libssh2_channel_direct_tcpip_ex(session, node->ctx.hostname, node->ctx.port, "127.0.0.1", 22);
            if (node->ctx.ssh_channel) {
                uint8_t rep[] = {0x05, 0x00, 0x00, 0x01, 0,0,0,0, 0,0};
                tcp_transport_write(node->ctx.socket_fd, rep, sizeof(rep));
                node->state = SOCKS_BRIDGING;
            } else {
                int err = libssh2_session_last_error(session, NULL, NULL, 0);
                if (err != LIBSSH2_ERROR_EAGAIN) {
                    uint8_t rep[] = {0x05, 0x01, 0x00, 0x01, 0,0,0,0, 0,0};
                    tcp_transport_write(node->ctx.socket_fd, rep, sizeof(rep));
                    close_channel(node);
                }
            }
        }
        
        if (node->state == SOCKS_BRIDGING) {
            // Write to SSH from tx_buffer
            if (node->ctx.tx_buffer && buffer_get_readable_size(node->ctx.tx_buffer) > 0) {
                size_t avail = buffer_get_readable_size(node->ctx.tx_buffer);
                uint8_t *ptr = buffer_get_read_ptr(node->ctx.tx_buffer);
                ssize_t nw = libssh2_channel_write(node->ctx.ssh_channel, (const char*)ptr, avail);
                if (nw > 0) {
                    buffer_consume(node->ctx.tx_buffer, (size_t)nw);
                } else if (nw != LIBSSH2_ERROR_EAGAIN) {
                    close_channel(node);
                }
            } else if (node->tx_len > 0) {
                ssize_t nw = libssh2_channel_write(node->ctx.ssh_channel, (const char*)node->tx_buf + node->tx_offset, node->tx_len);
                if (nw > 0) {
                    node->tx_offset += nw;
                    node->tx_len -= nw;
                } else if (nw != LIBSSH2_ERROR_EAGAIN) {
                    close_channel(node);
                }
            }
            
            // Read from SSH into rx_buffer
            if (!node->remote_eof) {
                if (node->ctx.rx_buffer && buffer_get_readable_size(node->ctx.rx_buffer) < BUF_SIZE) {
                    uint8_t temp[4096];
                    ssize_t nr = libssh2_channel_read(node->ctx.ssh_channel, (char*)temp, sizeof(temp));
                    if (nr > 0) {
                        buffer_append(node->ctx.rx_buffer, temp, (size_t)nr);
                        if (node->ev_ctx) ev_mod(node->ev_ctx, EVENT_READ | EVENT_WRITE);
                    } else if (nr == 0) {
                        node->remote_eof = true;
                    } else if (nr != LIBSSH2_ERROR_EAGAIN) {
                        close_channel(node);
                    }
                } else if (!node->ctx.rx_buffer && node->rx_len == 0) {
                    ssize_t nr = libssh2_channel_read(node->ctx.ssh_channel, (char*)node->rx_buf, BUF_SIZE);
                    if (nr > 0) {
                        node->rx_len = nr;
                        node->rx_offset = 0;
                        if (node->ev_ctx) ev_mod(node->ev_ctx, EVENT_READ | EVENT_WRITE);
                    } else if (nr == 0) {
                        node->remote_eof = true;
                    } else if (nr != LIBSSH2_ERROR_EAGAIN) {
                        close_channel(node);
                    }
                }
            }
            
            // Send EOF
            size_t tx_left = node->ctx.tx_buffer ? buffer_get_readable_size(node->ctx.tx_buffer) : node->tx_len;
            if (node->local_eof && tx_left == 0 && !node->ssh_eof_sent) {
                int rc = libssh2_channel_send_eof(node->ctx.ssh_channel);
                if (rc == 0) {
                    node->ssh_eof_sent = true;
                } else if (rc != LIBSSH2_ERROR_EAGAIN) {
                    close_channel(node);
                }
            }
        }
        
        node = node->next;
    }
    
    free_closed_channels();
}

void cm_cleanup(void) {
    cm_stop_socks_server();
    cm_stop_http_proxy();
    ChannelNode *node = g_channels;
    while (node) {
        close_channel(node);
        node = node->next;
    }
    free_closed_channels();
}
