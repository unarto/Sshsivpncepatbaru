#ifndef ENGINE_ERROR_H
#define ENGINE_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ENGINE_OK = 0,
    ENGINE_TIMEOUT = -1,
    ENGINE_IO_ERROR = -2,
    ENGINE_PROTOCOL_ERROR = -3,
    ENGINE_AUTH_FAILED = -4,
    ENGINE_TLS_ERROR = -5,
    ENGINE_PROXY_ERROR = -6,
    ENGINE_SSH_ERROR = -7,
    ENGINE_AGAIN = -8,
    ENGINE_WOULDBLOCK = -9,
    ENGINE_INTR = -10,
    ENGINE_INVALID_ARG = -11
} EngineError;

#ifdef __cplusplus
}
#endif

#endif // ENGINE_ERROR_H
