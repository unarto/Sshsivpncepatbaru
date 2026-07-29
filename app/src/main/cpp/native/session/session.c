#include "../ssh/channel.h"
#include "../payload/payload_builder.h"
#include "../common/utils.h"
#include "session.h"
#include "reconnect.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <ctype.h>
#include <time.h>

static ChannelContext g_ctx = {0};
static EventContext *g_ev_ctx = NULL;
static TunnelConfig g_cfg;
static bool g_is_ready = false;
static bool g_payload_in_use = false;

static int g_max_retries = 10;
static int g_base_delay_sec = 2;
static int g_max_delay_sec = 60;
static time_t g_next_retry_time = 0;
static bool g_network_connected = true;

static void sm_update_events(void) {
    if (!g_ctx.ssh_session || !g_ev_ctx) return;
    int dir = libssh2_session_block_directions(g_ctx.ssh_session);
    uint32_t events = 0;
    if (dir & LIBSSH2_SESSION_BLOCK_INBOUND) events |= EVENT_READ;
    if (dir & LIBSSH2_SESSION_BLOCK_OUTBOUND) events |= EVENT_WRITE;
    ev_mod(g_ev_ctx, events);
}

static void sm_cleanup_internal(void) {
    if (g_ctx.ssh_session) {
        libssh2_session_disconnect(g_ctx.ssh_session, "Normal close");
        libssh2_session_free(g_ctx.ssh_session);
        g_ctx.ssh_session = NULL;
    }
    if (g_ev_ctx) {
        ev_del(g_ev_ctx);
        g_ev_ctx = NULL;
    }
    if (g_ctx.socket_fd != -1) {
        close(g_ctx.socket_fd);
        g_ctx.socket_fd = -1;
    }
    g_ctx.state = CONN_STATE_ERROR;
    g_is_ready = false;
    if (g_payload_in_use) {
        pm_stop();
        g_payload_in_use = false;
    }
}

static int verify_host_key_exact(LIBSSH2_SESSION *session, const char *expected_fingerprint) {
    if (!expected_fingerprint || expected_fingerprint[0] == '\0') return -1;
    char expected[100] = {0};
    int j = 0, is_sha256_b64 = 0;
    if (strncmp(expected_fingerprint, "SHA256:", 7) == 0 || strncmp(expected_fingerprint, "sha256:", 7) == 0) {
        strncpy(expected, expected_fingerprint + 7, 99);
        is_sha256_b64 = 1;
    } else {
        for (int i = 0; expected_fingerprint[i] != '\0' && j < 99; i++) {
            char c = expected_fingerprint[i];
            if (c != ':' && c != ' ') {
                if (c >= 'A' && c <= 'Z') c += 32;
                expected[j++] = c;
            }
        }
    }
    const char *hash_sha256 = libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_SHA256);
    char b64_sha256[100] = {0};
    if (hash_sha256) utils_base64_encode((const unsigned char *)hash_sha256, 32, b64_sha256, sizeof(b64_sha256));
    if (is_sha256_b64 && hash_sha256) {
        char clean_expected[100] = {0}, clean_actual[100] = {0};
        strncpy(clean_expected, expected, 99);
        strncpy(clean_actual, b64_sha256, 99);
        int len = strlen(clean_expected);
        while(len > 0 && clean_expected[len-1] == '=') clean_expected[--len] = '\0';
        len = strlen(clean_actual);
        while(len > 0 && clean_actual[len-1] == '=') clean_actual[--len] = '\0';
        if (strcmp(clean_actual, clean_expected) == 0) {
            LOGI("Host key verified (SHA256)");
            return 0;
        }
    }
    const char *hash_sha1 = libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_SHA1);
    char hex_sha1[41] = {0};
    if (hash_sha1) for (int i = 0; i < 20; i++) snprintf(hex_sha1 + (i*2), 3, "%02x", (unsigned char)hash_sha1[i]);
    const char *hash_md5 = libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_MD5);
    char hex_md5[33] = {0};
    if (hash_md5) for (int i = 0; i < 16; i++) snprintf(hex_md5 + (i*2), 3, "%02x", (unsigned char)hash_md5[i]);
    if (!is_sha256_b64) {
        if (hash_sha1 && strcmp(hex_sha1, expected) == 0) { LOGI("Host key verified (SHA1)"); return 0; }
        if (hash_md5 && strcmp(hex_md5, expected) == 0) { LOGI("Host key verified (MD5)"); return 0; }
    }
    LOGE("Host key verification failed!");
    return -1;
}

static int verify_known_hosts(LIBSSH2_SESSION *session, const char *host, int port, const char *known_hosts_path) {
    if (!known_hosts_path || known_hosts_path[0] == '\0') return -1;
    LIBSSH2_KNOWNHOSTS *nh = libssh2_knownhost_init(session);
    if (!nh) return -1;
    libssh2_knownhost_readfile(nh, known_hosts_path, LIBSSH2_KNOWNHOST_FILE_OPENSSH);
    size_t len;
    int type;
    const char *fingerprint = libssh2_session_hostkey(session, &len, &type);
    if (!fingerprint) { libssh2_knownhost_free(nh); return -1; }
    struct libssh2_knownhost *hostinfo;
    int check = libssh2_knownhost_checkp(nh, host, port, fingerprint, len,
                                         LIBSSH2_KNOWNHOST_TYPE_PLAIN | LIBSSH2_KNOWNHOST_KEYENC_RAW, &hostinfo);
    int result = -1;
    if (check == LIBSSH2_KNOWNHOST_CHECK_MATCH) {
        LOGI("Host key verified via known_hosts");
        result = 0;
    } else {
        LOGE("Host key mismatch in known_hosts");
    }
    libssh2_knownhost_free(nh);
    return result;
}

static void sm_do_handshake(void) {
    int rc = libssh2_session_handshake(g_ctx.ssh_session, g_ctx.socket_fd);
    if (rc == 0) {
        LOGI("SSH Handshake OK");
        
        int verify_result = -1;
        if (g_cfg.fingerprint[0] != '\0') {
            verify_result = verify_host_key_exact(g_ctx.ssh_session, g_cfg.fingerprint);
        } else {
            verify_result = verify_known_hosts(g_ctx.ssh_session, g_cfg.host, g_cfg.port, g_cfg.known_hosts_path);
        }
        
        if (verify_result == 0) {
            g_ctx.state = CONN_STATE_SSH_AUTH;
        } else {
            sm_cleanup_internal();
        }
    } else if (rc != LIBSSH2_ERROR_EAGAIN) {
        LOGE("SSH Handshake failed: %d", rc);
        sm_cleanup_internal();
    }
}

static void sm_do_auth(void) {
    int rc = libssh2_userauth_password(g_ctx.ssh_session, g_cfg.user, g_cfg.pass);
    if (rc == 0) {
        LOGI("SSH Auth OK");
        if (g_cfg.auto_ping) {
            libssh2_keepalive_config(g_ctx.ssh_session, 1, 3);
        }
        g_ctx.state = CONN_STATE_CONNECTED;
        g_is_ready = true;
        reconnect_reset_backoff();
    } else if (rc != LIBSSH2_ERROR_EAGAIN) {
        LOGE("SSH Auth failed: %d", rc);
        sm_cleanup_internal();
    }
}

static void sm_event_cb(EventContext *ctx, uint32_t events) {
    if (events & EVENT_ERROR) {
        LOGE("Session socket error");
        sm_cleanup_internal();
        return;
    }

    if (g_ctx.state == CONN_STATE_CONNECTING) {
        int err = 0;
        socklen_t len = sizeof(err);
        getsockopt(g_ctx.socket_fd, SOL_SOCKET, SO_ERROR, &err, &len);
        if (err != 0) {
            LOGE("TCP Connect failed: %s", strerror(err));
            sm_cleanup_internal();
            return;
        }
        LOGI("TCP Connected");
        
        g_ctx.ssh_session = libssh2_session_init();
        if (!g_ctx.ssh_session) {
            sm_cleanup_internal();
            return;
        }
        libssh2_session_set_blocking(g_ctx.ssh_session, 0);
        g_ctx.state = CONN_STATE_SSH_HANDSHAKE;
    }

    if (g_ctx.state == CONN_STATE_SSH_HANDSHAKE) sm_do_handshake();
    if (g_ctx.state == CONN_STATE_SSH_AUTH) sm_do_auth();

    if (g_ctx.state == CONN_STATE_CONNECTED) {
        cm_process_channels();
    }

    if (g_ctx.state != CONN_STATE_ERROR && g_ctx.state != CONN_STATE_DISCONNECTED) {
        sm_update_events();
    }
}

bool sm_start(const TunnelConfig *cfg) {
    memcpy(&g_cfg, cfg, sizeof(TunnelConfig));
    g_is_ready = false;
    g_ctx.state = CONN_STATE_DISCONNECTED;
    g_ctx.socket_fd = -1;
    g_ctx.ssh_session = NULL;
    g_ctx.reconnect_counter = 0;
    g_payload_in_use = false;

    if (cfg->proxy_host[0]) {
        g_ctx.socket_fd = pm_start(cfg);
        if (g_ctx.socket_fd < 0) {
            LOGE("Payload Manager failed to start");
            g_ctx.state = CONN_STATE_ERROR;
        return false;
        }
        g_payload_in_use = true;
        
        g_ev_ctx = ev_add(g_ctx.socket_fd, EVENT_WRITE | EVENT_READ, sm_event_cb, NULL);
        if (!g_ev_ctx) {
            pm_stop();
            g_ctx.socket_fd = -1;
            g_ctx.state = CONN_STATE_ERROR;
        return false;
        }
        g_ctx.ssh_session = libssh2_session_init();
        if (!g_ctx.ssh_session) {
            pm_stop();
            g_ctx.socket_fd = -1;
            g_ctx.state = CONN_STATE_ERROR;
        return false;
        }
        libssh2_session_set_blocking(g_ctx.ssh_session, 0);
        g_ctx.state = CONN_STATE_SSH_HANDSHAKE;
        return true;
    }

    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", cfg->port);

    if (getaddrinfo(cfg->host, port_str, &hints, &result) != 0) {
        LOGE("Failed to resolve host %s", cfg->host);
        g_ctx.state = CONN_STATE_ERROR;
        return false;
    }

    g_ctx.socket_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (g_ctx.socket_fd < 0) {
        freeaddrinfo(result);
        g_ctx.state = CONN_STATE_ERROR;
        return false;
    }

    set_nonblock(g_ctx.socket_fd);
    int opt = 1;
    setsockopt(g_ctx.socket_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
    setsockopt(g_ctx.socket_fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));

    int rc = connect(g_ctx.socket_fd, result->ai_addr, result->ai_addrlen);
    freeaddrinfo(result);

    if (rc < 0 && errno != EINPROGRESS) {
        close(g_ctx.socket_fd);
        g_ctx.socket_fd = -1;
        g_ctx.state = CONN_STATE_ERROR;
        return false;
    }

    g_ev_ctx = ev_add(g_ctx.socket_fd, EVENT_WRITE | EVENT_READ, sm_event_cb, NULL);
    if (!g_ev_ctx) {
        close(g_ctx.socket_fd);
        g_ctx.socket_fd = -1;
        g_ctx.state = CONN_STATE_ERROR;
        return false;
    }

    g_ctx.state = CONN_STATE_CONNECTING;
    return true;
}

void sm_stop(void) {
    sm_cleanup_internal();
}

bool sm_is_ready(void) {
    return g_is_ready;
}

LIBSSH2_SESSION* sm_get_session(void) {
    return g_ctx.ssh_session;
}

void sm_request_write(void) {
    if (g_ev_ctx) {
        ev_mod(g_ev_ctx, EVENT_READ | EVENT_WRITE);
    }
}


void reconnect_init(int max_retries, int base_delay_sec, int max_delay_sec) {
    if (max_retries > 0) g_max_retries = max_retries;
    if (base_delay_sec > 0) g_base_delay_sec = base_delay_sec;
    if (max_delay_sec > 0) g_max_delay_sec = max_delay_sec;
    g_ctx.reconnect_counter = 0;
    g_next_retry_time = 0;
    g_network_connected = true;
    LOGI("Reconnect initialized: max_retries=%d, base_delay=%ds, max_delay=%ds", 
         g_max_retries, g_base_delay_sec, g_max_delay_sec);
}

void reconnect_reset_backoff(void) {
    g_ctx.reconnect_counter = 0;
    g_next_retry_time = 0;
    LOGI("Reconnect backoff state reset");
}

int reconnect_get_retry_count(void) {
    return g_ctx.reconnect_counter;
}

int reconnect_get_next_delay(void) {
    int shift = g_ctx.reconnect_counter > 10 ? 10 : g_ctx.reconnect_counter;
    int delay = g_base_delay_sec * (1 << shift);
    if (delay > g_max_delay_sec) delay = g_max_delay_sec;
    return delay;
}

void reconnect_notify_network_changed(bool is_connected) {
    g_network_connected = is_connected;
    LOGI("Network interface status changed: connected=%d", is_connected);
    if (is_connected && g_ctx.state == CONN_STATE_ERROR) {
        LOGI("Network reconnected: resetting backoff and attempting instant reconnect...");
        reconnect_reset_backoff();
        sm_start(&g_cfg);
    }
}

void sm_periodic(void) {
    if (g_ctx.state == CONN_STATE_CONNECTED && g_ctx.ssh_session && g_cfg.auto_ping) {
        int seconds_to_next = 0;
        int rc = libssh2_keepalive_send(g_ctx.ssh_session, &seconds_to_next);
        if (rc < 0 && rc != LIBSSH2_ERROR_EAGAIN) {
            LOGE("KeepAlive failed, disconnecting...");
            sm_cleanup_internal();
        }
    }
    
    if (g_ctx.state == CONN_STATE_ERROR) {
        if (!g_network_connected) {
            // Waiting for network interface to come back online
            return;
        }
        
        if (g_max_retries > 0 && g_ctx.reconnect_counter >= g_max_retries) {
            // Reached maximum allowed reconnect retries
            return;
        }
        
        time_t now = time(NULL);
        if (g_next_retry_time == 0) {
            int delay = reconnect_get_next_delay();
            g_next_retry_time = now + delay;
            LOGI("Scheduled reconnect attempt #%d in %d seconds...", g_ctx.reconnect_counter + 1, delay);
        } else if (now >= g_next_retry_time) {
            g_ctx.reconnect_counter++;
            g_next_retry_time = 0;
            LOGI("Executing auto-reconnect attempt #%d...", g_ctx.reconnect_counter);
            sm_start(&g_cfg);
        }
    } else {
        g_next_retry_time = 0;
    }
}
