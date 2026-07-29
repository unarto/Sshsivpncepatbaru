#include "tcp_transport.h"
#include "../common/error.h"
#include "../common/utils.h"
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>

int tcp_transport_connect_nonblocking(const char *host, int port) {
    struct addrinfo hints, *result, *rp;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);
    
    if (getaddrinfo(host, port_str, &hints, &result) != 0) {
        return ENGINE_IO_ERROR;
    }
    
    int sock = -1;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock < 0) continue;
        
        set_nonblock(sock);
        
        int rc = connect(sock, rp->ai_addr, rp->ai_addrlen);
        if (rc == 0) {
            break; // connected immediately
        }
        if (errno == EINPROGRESS) {
            break; // connection in progress
        }
        
        close(sock);
        sock = -1;
    }
    
    freeaddrinfo(result);
    
    if (sock < 0) {
        return ENGINE_IO_ERROR;
    }
    
    return sock;
}

ssize_t tcp_transport_peek(int fd, void *buf, size_t len) {
    if (fd < 0 || !buf) return ENGINE_INVALID_ARG;
    ssize_t ret = recv(fd, buf, len, MSG_PEEK);
    if (ret < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return ENGINE_AGAIN;
        if (errno == EINTR) return ENGINE_INTR;
        return ENGINE_IO_ERROR;
    }
    return ret;
}

ssize_t tcp_transport_read(int fd, void *buf, size_t len) {
    if (fd < 0 || !buf) return ENGINE_INVALID_ARG;
    ssize_t ret = read(fd, buf, len);
    if (ret < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return ENGINE_AGAIN;
        if (errno == EINTR) return ENGINE_INTR;
        return ENGINE_IO_ERROR;
    }
    return ret;
}

ssize_t tcp_transport_write(int fd, const void *buf, size_t len) {
    if (fd < 0 || !buf) return ENGINE_INVALID_ARG;
    ssize_t ret = write(fd, buf, len);
    if (ret < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return ENGINE_AGAIN;
        if (errno == EINTR) return ENGINE_INTR;
        return ENGINE_IO_ERROR;
    }
    return ret;
}

void tcp_transport_close(int fd) {
    if (fd >= 0) {
        close(fd);
    }
}
