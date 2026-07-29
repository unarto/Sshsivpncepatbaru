#ifndef NATIVE_UTILS_H
#define NATIVE_UTILS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

char* utils_base64_encode_alloc(const unsigned char *src, size_t len);
void utils_generate_ws_key(char *out_key, size_t out_len);
char* utils_generate_ws_key_alloc(void);

#ifdef __cplusplus
}
#endif
#endif // NATIVE_UTILS_H
