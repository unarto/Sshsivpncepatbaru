#include "../ssh/channel.h"
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include "../transport/transport.h"
#include "../transport/tcp_transport.h"
#include "../transport/tls_transport.h"
#include "../proxy/http_request.h"
#include "split_engine.h"
#include "payload_builder.h"
#include "../session/session.h"
#include "../common/error.h"
#include <sys/select.h>

// Global state for payload manager
static ChannelContext g_pm_ctx;
static TlsContext g_tls_ctx;
static int g_local_fd = -1;
static EventContext *g_ev_ctx_local = NULL;
static EventContext *g_ev_ctx_remote = NULL;

static uint8_t g_buf_l2r[8192];
static size_t g_len_l2r = 0;
static size_t g_off_l2r = 0;

static uint8_t g_buf_r2l[8192];
static size_t g_len_r2l = 0;
static size_t g_off_r2l = 0;

static bool g_local_eof = false;
static bool g_remote_eof = false;

static SplitEngineContext g_se_ctx;
static bool g_se_active = false;

static void update_events(void) {
    if (g_ev_ctx_local) {
        uint32_t ev = 0;
        if (g_len_l2r == 0 && !g_local_eof) ev |= EVENT_READ;
        if (g_len_r2l > 0) ev |= EVENT_WRITE;
        ev_mod(g_ev_ctx_local, ev);
    }
    if (g_ev_ctx_remote) {
        if (g_se_active) {
            uint32_t ev = se_get_events(&g_se_ctx);
            ev_mod(g_ev_ctx_remote, ev);
        } else {
            uint32_t ev = 0;
            if (g_len_r2l == 0 && !g_remote_eof) ev |= EVENT_READ;
            if (g_len_l2r > 0) ev |= EVENT_WRITE;
            ev_mod(g_ev_ctx_remote, ev);
        }
    }
}

static void remote_cb(EventContext *ctx, uint32_t events) {
    if (events & EVENT_ERROR) {
        pm_stop();
        return;
    }
    
    if (g_se_active) {
        int r = se_process(&g_se_ctx);
        if (r == 1) { // Done
            g_se_active = false;
            update_events();
        } else if (r < 0) { // Error
            pm_stop();
        } else {
            update_events();
        }
        return;
    }
    
    if (events & EVENT_READ) {
        int ret = transport_read(&g_pm_ctx, g_buf_r2l, sizeof(g_buf_r2l));
        if (ret > 0) {
            g_len_r2l = ret;
            g_off_r2l = 0;
        } else if (ret == 0 || (ret != ENGINE_AGAIN && ret != ENGINE_INTR)) {
            g_remote_eof = true;
        }
    }
    
    if (events & EVENT_WRITE && g_len_l2r > 0) {
        int ret = transport_write(&g_pm_ctx, g_buf_l2r + g_off_l2r, g_len_l2r);
        if (ret > 0) {
            g_off_l2r += ret;
            g_len_l2r -= ret;
        } else if (ret != ENGINE_AGAIN && ret != ENGINE_INTR) {
            g_remote_eof = true;
        }
    }
    
    update_events();
}

static void local_cb(EventContext *ctx, uint32_t events) {
    if (events & EVENT_ERROR) {
        pm_stop();
        return;
    }
    
    if (events & EVENT_READ) {
        int ret = read(g_local_fd, g_buf_l2r, sizeof(g_buf_l2r));
        if (ret > 0) {
            g_len_l2r = ret;
            g_off_l2r = 0;
        } else if (ret == 0 || (ret < 0 && errno != EAGAIN)) {
            g_local_eof = true;
        }
    }
    
    if (events & EVENT_WRITE && g_len_r2l > 0) {
        int ret = write(g_local_fd, g_buf_r2l + g_off_r2l, g_len_r2l);
        if (ret > 0) {
            g_off_r2l += ret;
            g_len_r2l -= ret;
        } else if (ret < 0 && errno != EAGAIN) {
            g_local_eof = true;
        }
    }
    
    update_events();
}

static int wait_for_socket(int fd, int timeout_ms) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    int r = select(fd + 1, NULL, &fds, NULL, &tv);
    if (r <= 0) return -1;
    int err = 0;
    socklen_t len = sizeof(err);
    getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);
    return err == 0 ? 0 : -1;
}

int pm_start(const TunnelConfig *cfg) {
    if (!cfg->proxy_host[0]) {
        return -1;
    }
    
    memset(&g_pm_ctx, 0, sizeof(ChannelContext));
    int sock = tcp_transport_connect_nonblocking(cfg->proxy_host, cfg->proxy_port);
    if (sock < 0) {
        LOGE("Payload: Failed to create socket");
        return -1;
    }
    
    if (wait_for_socket(sock, 5000) != 0) {
        LOGE("Payload: Failed to connect to proxy");
        close(sock);
        return -1;
    }
    
    g_pm_ctx.socket_fd = sock;
    
    if (cfg->tls_version > 0) {
        memset(&g_tls_ctx, 0, sizeof(TlsContext));
        g_pm_ctx.ssl_ctx = &g_tls_ctx;
        g_pm_ctx.protocol = PROTO_SSL;
        strncpy(g_pm_ctx.sni, cfg->sni, sizeof(g_pm_ctx.sni) - 1);
        
        // Blocking TLS handshake for simplicity here
        int ret;
        do {
            ret = tls_transport_handshake_step(&g_pm_ctx);
            if (ret == ENGINE_AGAIN || ret == ENGINE_INTR) {
                usleep(10000);
            }
        } while (ret == ENGINE_AGAIN || ret == ENGINE_INTR);
        
        if (ret != ENGINE_OK) {
            LOGE("Payload: TLS handshake failed");
            tcp_transport_close(sock);
            return -1;
        }
    }
    
    if (cfg->num_payload_chunks > 0) {
        se_init(&g_se_ctx, &g_pm_ctx, cfg);
        g_se_active = true;
    } else {
        g_se_active = false;
    }
    
    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0) {
        LOGE("socketpair failed");
        if (g_pm_ctx.ssl_ctx) tls_transport_close(&g_pm_ctx);
        tcp_transport_close(sock);
        return -1;
    }
    
    g_local_fd = fds[0];
    int flags = fcntl(g_local_fd, F_GETFL, 0);
    fcntl(g_local_fd, F_SETFL, flags | O_NONBLOCK);
    
    g_ev_ctx_local = ev_add(g_local_fd, EVENT_READ, local_cb, NULL);
    g_ev_ctx_remote = ev_add(g_pm_ctx.socket_fd, EVENT_READ, remote_cb, NULL);
    
    g_len_l2r = g_len_r2l = 0;
    g_local_eof = g_remote_eof = false;
    
    return fds[1]; // libssh2 side
}

void pm_stop(void) {
    if (g_ev_ctx_local) { ev_del(g_ev_ctx_local); g_ev_ctx_local = NULL; }
    if (g_ev_ctx_remote) { ev_del(g_ev_ctx_remote); g_ev_ctx_remote = NULL; }
    if (g_local_fd != -1) { close(g_local_fd); g_local_fd = -1; }
    if (g_pm_ctx.ssl_ctx) {
        tls_transport_close(&g_pm_ctx);
        g_pm_ctx.ssl_ctx = NULL;
    }
    if (g_pm_ctx.socket_fd >= 0) { 
        tcp_transport_close(g_pm_ctx.socket_fd); 
        g_pm_ctx.socket_fd = -1; 
    }
    g_se_active = false;
}

void pm_process(void) {
    if (g_se_active) {
        int r = se_process(&g_se_ctx);
        if (r == 1) { // Done
            g_se_active = false;
            update_events();
        } else if (r < 0) { // Error
            pm_stop();
        } else {
            update_events();
        }
    }
}
