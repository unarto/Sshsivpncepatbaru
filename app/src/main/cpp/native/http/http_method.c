#include "http_method.h"
#include <string.h>

HttpMethod http_method_from_string(const char* method_str) {
    if (!method_str) return HTTP_METHOD_UNKNOWN;
    
    if (strcasecmp(method_str, "GET") == 0) return HTTP_METHOD_GET;
    if (strcasecmp(method_str, "POST") == 0) return HTTP_METHOD_POST;
    if (strcasecmp(method_str, "CONNECT") == 0) return HTTP_METHOD_CONNECT;
    if (strcasecmp(method_str, "HEAD") == 0) return HTTP_METHOD_HEAD;
    if (strcasecmp(method_str, "PUT") == 0) return HTTP_METHOD_PUT;
    if (strcasecmp(method_str, "DELETE") == 0) return HTTP_METHOD_DELETE;
    if (strcasecmp(method_str, "PATCH") == 0) return HTTP_METHOD_PATCH;
    if (strcasecmp(method_str, "OPTIONS") == 0) return HTTP_METHOD_OPTIONS;
    if (strcasecmp(method_str, "TRACE") == 0) return HTTP_METHOD_TRACE;
    
    return HTTP_METHOD_UNKNOWN;
}

const char* http_method_to_string(HttpMethod method) {
    switch (method) {
        case HTTP_METHOD_GET: return "GET";
        case HTTP_METHOD_POST: return "POST";
        case HTTP_METHOD_CONNECT: return "CONNECT";
        case HTTP_METHOD_HEAD: return "HEAD";
        case HTTP_METHOD_PUT: return "PUT";
        case HTTP_METHOD_DELETE: return "DELETE";
        case HTTP_METHOD_PATCH: return "PATCH";
        case HTTP_METHOD_OPTIONS: return "OPTIONS";
        case HTTP_METHOD_TRACE: return "TRACE";
        default: return "UNKNOWN";
    }
}

bool http_method_is_valid(const char* method_str) {
    return http_method_from_string(method_str) != HTTP_METHOD_UNKNOWN;
}
