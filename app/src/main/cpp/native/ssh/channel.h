#ifndef NATIVE_CHANNEL_H
#define NATIVE_CHANNEL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>
#include <libssh2.h>
#include <android/log.h>
#include <sys/epoll.h>

#define LOG_TAG "NativeChannelEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Config
typedef enum {
    SPLIT_NORMAL = 0,
    SPLIT_INSTANT = 1,
    SPLIT_DELAY = 2
} SplitType;

typedef struct {
    char content[2048];
    SplitType split_type;
} PayloadChunk;

typedef struct {
    char host[256];
    int port;
    char user[128];
    char pass[128];
    char fingerprint[256];
    char known_hosts_path[512];
    int socks_port;
    int http_proxy_port;
    bool hotshare_enabled;
    char proxy_host[256];
    int proxy_port;
        PayloadChunk payload_chunks[16];
    int num_payload_chunks;
    
    // --- Added for Custom Payload Engine ---

    char raw_payload[2048]; // Raw custom payload string

    int user_agent_type;    // Uses PayloadUserAgentType

    char custom_user_agent[256];

    int payload_template_type; // Uses PayloadTemplateType

    // ---------------------------------------
    char sni[256];
    int tls_version;
    int auto_ping;
    bool modo_dropbear;
} TunnelConfig;

// Event loop definitions
typedef enum {
    EVENT_READ = EPOLLIN,
    EVENT_WRITE = EPOLLOUT,
    EVENT_ERROR = EPOLLERR | EPOLLHUP | EPOLLRDHUP
} EventType;

typedef struct EventContext {
    int fd;
    void (*callback)(struct EventContext *ctx, uint32_t events);
    void *data;
} EventContext;

int ev_init(void);
EventContext* ev_add(int fd, uint32_t events, void (*cb)(EventContext*, uint32_t), void *data);
int ev_mod(EventContext *ctx, uint32_t events);
int ev_del(EventContext *ctx);
void ev_loop(void);
void ev_stop(void);
void ev_cleanup(void);

// Session Manager
bool sm_start(const TunnelConfig *cfg);
void sm_stop(void);
bool sm_is_ready(void);
LIBSSH2_SESSION* sm_get_session(void);
void sm_request_write(void);
void sm_periodic(void);

// Channel Manager
void cm_init(void);
void cm_start_socks_server(int port);
void cm_stop_socks_server(void);
void cm_start_http_proxy(int port);
void cm_stop_http_proxy(void);
void cm_process_channels(void);
void cm_cleanup(void);

#endif // NATIVE_CHANNEL_H
