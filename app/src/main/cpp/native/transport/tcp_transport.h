#ifndef TCP_TRANSPORT_H
#define TCP_TRANSPORT_H

#include <stddef.h>
#include <sys/types.h>
#include "../session/session.h"

#ifdef __cplusplus
extern "C" {
#endif

int tcp_transport_connect_nonblocking(const char *host, int port);
ssize_t tcp_transport_peek(int fd, void *buf, size_t len);
ssize_t tcp_transport_read(int fd, void *buf, size_t len);
ssize_t tcp_transport_write(int fd, const void *buf, size_t len);
void tcp_transport_close(int fd);

#ifdef __cplusplus
}
#endif

#endif // TCP_TRANSPORT_H
