#include "payload_validator.h"
#include <string.h>

bool payload_validator_check(const char* raw_payload) {
    if (!raw_payload) return false;
    
    int bracket_level = 0;
    const char* p = raw_payload;
    
    while (*p != '\0') {
        if (*p == '[') {
            bracket_level++;
            if (bracket_level > 1) {
                // Nested brackets are not allowed in this syntax
                return false;
            }
        } else if (*p == ']') {
            bracket_level--;
            if (bracket_level < 0) {
                // Closing bracket without opening bracket
                return false;
            }
        }
        p++;
    }
    
    // Valid if all opened brackets are closed
    return bracket_level == 0;
}
