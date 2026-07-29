#ifndef MMKV_BRIDGE_H
#define MMKV_BRIDGE_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

int mmkv_init(void);

int mmkv_get_string(const char *key, char *out_buf, size_t buf_size, const char *default_val);
int mmkv_put_string(const char *key, const char *value);

int mmkv_get_int(const char *key, int default_val);
int mmkv_put_int(const char *key, int value);

bool mmkv_get_bool(const char *key, bool default_val);
int mmkv_put_bool(const char *key, bool value);

int mmkv_remove(const char *key);
bool mmkv_contains(const char *key);

#ifdef __cplusplus
}
#endif

#endif // MMKV_BRIDGE_H
