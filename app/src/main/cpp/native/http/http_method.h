#ifndef NATIVE_HTTP_METHOD_H
#define NATIVE_HTTP_METHOD_H

#include <stdbool.h>

// Supported HTTP Methods
typedef enum {
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
    HTTP_METHOD_CONNECT,
    HTTP_METHOD_HEAD,
    HTTP_METHOD_PUT,
    HTTP_METHOD_DELETE,
    HTTP_METHOD_PATCH,
    HTTP_METHOD_OPTIONS,
    HTTP_METHOD_TRACE,
    HTTP_METHOD_UNKNOWN
} HttpMethod;

// Convert string to HttpMethod enum
HttpMethod http_method_from_string(const char* method_str);

// Convert HttpMethod enum to string
const char* http_method_to_string(HttpMethod method);

// Validate if a method string is a supported HTTP method
bool http_method_is_valid(const char* method_str);

#endif // NATIVE_HTTP_METHOD_H
