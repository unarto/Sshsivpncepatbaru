#ifndef NATIVE_PAYLOAD_UA_H
#define NATIVE_PAYLOAD_UA_H

typedef enum {
    PAYLOAD_UA_CUSTOM = 0,
    PAYLOAD_UA_CHROME,
    PAYLOAD_UA_FIREFOX,
    PAYLOAD_UA_SAFARI,
    PAYLOAD_UA_EDGE,
    PAYLOAD_UA_ANDROID
} PayloadUserAgentType;

// Returns a predefined User-Agent string based on type.
// If type is CUSTOM, returns the provided custom_ua (or default if NULL).
const char* payload_ua_get(PayloadUserAgentType type, const char* custom_ua);

// Converts a string representation of UA to its Enum type
PayloadUserAgentType payload_ua_from_string(const char* ua_str);

#endif // NATIVE_PAYLOAD_UA_H
