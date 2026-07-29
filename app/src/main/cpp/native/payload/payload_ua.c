#include "payload_ua.h"
#include <string.h>

const char* payload_ua_get(PayloadUserAgentType type, const char* custom_ua) {
    switch (type) {
        case PAYLOAD_UA_CHROME:
            return "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36";
        case PAYLOAD_UA_FIREFOX:
            return "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:109.0) Gecko/20100101 Firefox/121.0";
        case PAYLOAD_UA_SAFARI:
            return "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.2 Safari/605.1.15";
        case PAYLOAD_UA_EDGE:
            return "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36 Edg/120.0.0.0";
        case PAYLOAD_UA_ANDROID:
            return "Mozilla/5.0 (Linux; Android 14; Pixel 8 Pro) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.6099.193 Mobile Safari/537.36";
        case PAYLOAD_UA_CUSTOM:
        default:
            return custom_ua ? custom_ua : "SiVPN/1.0 (Android)";
    }
}

PayloadUserAgentType payload_ua_from_string(const char* ua_str) {
    if (!ua_str) return PAYLOAD_UA_CUSTOM;
    
    if (strcasecmp(ua_str, "chrome") == 0) return PAYLOAD_UA_CHROME;
    if (strcasecmp(ua_str, "firefox") == 0) return PAYLOAD_UA_FIREFOX;
    if (strcasecmp(ua_str, "safari") == 0) return PAYLOAD_UA_SAFARI;
    if (strcasecmp(ua_str, "edge") == 0) return PAYLOAD_UA_EDGE;
    if (strcasecmp(ua_str, "android") == 0) return PAYLOAD_UA_ANDROID;
    
    return PAYLOAD_UA_CUSTOM;
}
