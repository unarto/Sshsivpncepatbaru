#ifndef NATIVE_HTTP_VERSION_H
#define NATIVE_HTTP_VERSION_H

#include <stdbool.h>

// Supported HTTP Versions
typedef enum {
    HTTP_VERSION_1_0,
    HTTP_VERSION_1_1,
    HTTP_VERSION_UNKNOWN
} HttpVersion;

// Convert string to HttpVersion enum (e.g. "HTTP/1.1")
HttpVersion http_version_from_string(const char* version_str);

// Convert HttpVersion enum to string
const char* http_version_to_string(HttpVersion version);

// Validate if a version string is supported
bool http_version_is_valid(const char* version_str);

#endif // NATIVE_HTTP_VERSION_H
