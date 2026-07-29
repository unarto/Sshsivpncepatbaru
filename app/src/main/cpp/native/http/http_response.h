#ifndef NATIVE_HTTP_RESPONSE_H
#define NATIVE_HTTP_RESPONSE_H

#include <stdbool.h>

// Parse the HTTP status code from a raw response string.
// Returns the integer status code (e.g., 200, 101), or -1 if invalid/not found.
int http_response_parse_status_code(const char* response);

// Check if the response is a successful connection (200 OK or 101 Switching Protocols)
bool http_response_is_success(int status_code);

// Extract the header value by key from a raw HTTP response.
// Returns an allocated string containing the value, or NULL if not found.
// Caller is responsible for freeing the memory.
char* http_response_extract_header(const char* response, const char* header_key);

#endif // NATIVE_HTTP_RESPONSE_H
