#include "mmkv_bridge.h"
#include <MMKV/MMKV.h>
#include <string>
#include <cstring>

extern "C" {

int mmkv_init(void) {
    // MMKV is initialized in Android Java/Kotlin layer.
    return 0;
}

int mmkv_get_string(const char *key, char *out_buf, size_t buf_size, const char *default_val) {
    if (!key || !out_buf || buf_size == 0) return -1;

    MMKV *mmkv = MMKV::mmkvWithID("sivpn_settings", MMKV::MULTI_PROCESS_MODE);
    if (!mmkv) {
        if (default_val) {
            strncpy(out_buf, default_val, buf_size - 1);
            out_buf[buf_size - 1] = '\0';
        } else {
            out_buf[0] = '\0';
        }
        return -1;
    }

    std::string str_val;
    if (mmkv->getString(key, str_val)) {
        strncpy(out_buf, str_val.c_str(), buf_size - 1);
        out_buf[buf_size - 1] = '\0';
        return 0;
    }

    if (default_val) {
        strncpy(out_buf, default_val, buf_size - 1);
        out_buf[buf_size - 1] = '\0';
    } else {
        out_buf[0] = '\0';
    }
    return -1;
}

int mmkv_put_string(const char *key, const char *value) {
    if (!key || !value) return -1;
    MMKV *mmkv = MMKV::mmkvWithID("sivpn_settings", MMKV::MULTI_PROCESS_MODE);
    if (!mmkv) return -1;
    return mmkv->set(value, key) ? 0 : -1;
}

int mmkv_get_int(const char *key, int default_val) {
    if (!key) return default_val;
    MMKV *mmkv = MMKV::mmkvWithID("sivpn_settings", MMKV::MULTI_PROCESS_MODE);
    if (!mmkv) return default_val;
    return mmkv->getInt32(key, default_val);
}

int mmkv_put_int(const char *key, int value) {
    if (!key) return -1;
    MMKV *mmkv = MMKV::mmkvWithID("sivpn_settings", MMKV::MULTI_PROCESS_MODE);
    if (!mmkv) return -1;
    return mmkv->set((int32_t)value, key) ? 0 : -1;
}

bool mmkv_get_bool(const char *key, bool default_val) {
    if (!key) return default_val;
    MMKV *mmkv = MMKV::mmkvWithID("sivpn_settings", MMKV::MULTI_PROCESS_MODE);
    if (!mmkv) return default_val;
    return mmkv->getBool(key, default_val);
}

int mmkv_put_bool(const char *key, bool value) {
    if (!key) return -1;
    MMKV *mmkv = MMKV::mmkvWithID("sivpn_settings", MMKV::MULTI_PROCESS_MODE);
    if (!mmkv) return -1;
    return mmkv->set(value, key) ? 0 : -1;
}

int mmkv_remove(const char *key) {
    if (!key) return -1;
    MMKV *mmkv = MMKV::mmkvWithID("sivpn_settings", MMKV::MULTI_PROCESS_MODE);
    if (!mmkv) return -1;
    mmkv->removeValueForKey(key);
    return 0;
}

bool mmkv_contains(const char *key) {
    if (!key) return false;
    MMKV *mmkv = MMKV::mmkvWithID("sivpn_settings", MMKV::MULTI_PROCESS_MODE);
    if (!mmkv) return false;
    return mmkv->containsKey(key);
}

} // extern "C"
