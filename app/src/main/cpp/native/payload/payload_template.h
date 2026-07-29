#ifndef NATIVE_PAYLOAD_TEMPLATE_H
#define NATIVE_PAYLOAD_TEMPLATE_H

typedef enum {
    PAYLOAD_TEMPLATE_DIRECT,
    PAYLOAD_TEMPLATE_PROXY,
    PAYLOAD_TEMPLATE_WEBSOCKET
} PayloadTemplateType;

// Generates a default payload template string based on connection type.
// Returns a static string, do not free.
const char* payload_template_get(PayloadTemplateType type);

#endif // NATIVE_PAYLOAD_TEMPLATE_H
