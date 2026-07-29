#include "http_version.h"
#include <string.h>

HttpVersion http_version_from_string(const char* version_str) {
    if (!version_str) return HTTP_VERSION_UNKNOWN;
    
    if (strcasecmp(version_str, "HTTP/1.0") == 0) return HTTP_VERSION_1_0;
    if (strcasecmp(version_str, "HTTP/1.1") == 0) return HTTP_VERSION_1_1;
    
    return HTTP_VERSION_UNKNOWN;
}

const char* http_version_to_string(HttpVersion version) {
    switch (version) {
        case HTTP_VERSION_1_0: return "HTTP/1.0";
        case HTTP_VERSION_1_1: return "HTTP/1.1";
        default: return "HTTP/1.1"; // Default fallback
    }
}

bool http_version_is_valid(const char* version_str) {
    return http_version_from_string(version_str) != HTTP_VERSION_UNKNOWN;
}
