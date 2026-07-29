#include "utils.h"
#include "../ssh/channel.h"
#include "../payload/payload_builder.h"
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "mbedtls/base64.h"
#include <stdlib.h>

int set_nonblock(int fd) {
    if (fd < 0) return -1;
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

}

static int epoll_fd = -1;
static volatile bool loop_running = false;
static int wakeup_pipe[2] = {-1, -1};
static EventContext wakeup_ctx;

static void wakeup_cb(EventContext *ctx, uint32_t events) {
    if (events & EVENT_READ) {
        char buf[8];
        read(ctx->fd, buf, sizeof(buf));
    }
}

int utils_base64_encode(const unsigned char *src, size_t len, char *out, size_t out_len) {
    if (!src || !out || out_len == 0) return -1;
    size_t olen = 0;
    if (mbedtls_base64_encode((unsigned char *)out, out_len, &olen, src, len) != 0) {
        return -1;
    }
    return 0;
}

char* utils_base64_encode_alloc(const unsigned char *src, size_t len) {
    if (!src || len == 0) return NULL;
    size_t out_len = 0;
    mbedtls_base64_encode(NULL, 0, &out_len, src, len);
    if (out_len == 0) return NULL;
    char* b64_str = (char*)malloc(out_len);
    if (!b64_str) return NULL;
    size_t final_len = 0;
    if (mbedtls_base64_encode((unsigned char*)b64_str, out_len, &final_len, src, len) != 0) {
        free(b64_str);
        return NULL;
    }
    return b64_str;
}

void utils_generate_ws_key(char *out_key, size_t out_len) {
    unsigned char random_bytes[16];
    for (int i = 0; i < 16; i++) {
        random_bytes[i] = (unsigned char)(rand() % 256);
    }
    utils_base64_encode(random_bytes, 16, out_key, out_len);
}

char* utils_generate_ws_key_alloc(void) {
    unsigned char random_bytes[16];
    for (int i = 0; i < 16; i++) {
        random_bytes[i] = (unsigned char)(rand() % 256);
    }
    return utils_base64_encode_alloc(random_bytes, 16);
}
int ev_init(void) {
    epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd < 0) return -1;

    if (pipe(wakeup_pipe) == 0) {
        ev_add(wakeup_pipe[0], EVENT_READ, wakeup_cb, NULL);
    }
    return 0;
}

EventContext* ev_add(int fd, uint32_t events, void (*cb)(EventContext*, uint32_t), void *data) {
    if (epoll_fd < 0 || fd < 0) return NULL;
    EventContext *ctx = (EventContext*)malloc(sizeof(EventContext));
    if (!ctx) return NULL;
    ctx->fd = fd;
    ctx->callback = cb;
    ctx->data = data;

    struct epoll_event ev = {0};
    ev.events = events;
    ev.data.ptr = ctx;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        free(ctx);
        return NULL;
    }
    return ctx;
}

int ev_mod(EventContext *ctx, uint32_t events) {
    if (epoll_fd < 0 || !ctx) return -1;
    struct epoll_event ev = {0};
    ev.events = events;
    ev.data.ptr = ctx;
    return epoll_ctl(epoll_fd, EPOLL_CTL_MOD, ctx->fd, &ev);
}

int ev_del(EventContext *ctx) {
    if (epoll_fd < 0 || !ctx) return -1;
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, ctx->fd, NULL);
    free(ctx);
    return 0;
}

void ev_stop(void) {
    loop_running = false;
    if (wakeup_pipe[1] != -1) {
        write(wakeup_pipe[1], "x", 1);
    }
}

void ev_loop(void) {
    loop_running = true;
    struct epoll_event events[64];
    while (loop_running) {
        int n = epoll_wait(epoll_fd, events, 64, 500); // 500ms timeout for housekeeping
        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }

        for (int i = 0; i < n; i++) {
            EventContext *ctx = (EventContext*)events[i].data.ptr;
            if (ctx && ctx->callback) {
                ctx->callback(ctx, events[i].events);
            }
        }
        
        // Also let channel manager process libssh2 channels
        cm_process_channels();
        sm_periodic();
        pm_process();
    }
}

void ev_cleanup(void) {
    if (wakeup_pipe[0] != -1) close(wakeup_pipe[0]);
    if (wakeup_pipe[1] != -1) close(wakeup_pipe[1]);
    if (epoll_fd != -1) close(epoll_fd);
    epoll_fd = -1;
    wakeup_pipe[0] = -1;
    wakeup_pipe[1] = -1;
}
