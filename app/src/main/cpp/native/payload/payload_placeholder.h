#ifndef NATIVE_PAYLOAD_PLACEHOLDER_H
#define NATIVE_PAYLOAD_PLACEHOLDER_H

#include "payload_parser.h"

// Context for placeholder replacement
typedef struct {
    const char* host;
    int port;
    const char* path;
    const char* query;
    const char* protocol;
    const char* method;
    const char* user_agent;
} PayloadPlaceholderContext;

// Replaces a specific placeholder token with its actual value.
// Returns a dynamically allocated string (caller must free), or NULL if no replacement.
char* payload_placeholder_replace(const PayloadToken* token, const PayloadPlaceholderContext* ctx);

#endif // NATIVE_PAYLOAD_PLACEHOLDER_H
