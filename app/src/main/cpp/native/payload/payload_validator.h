#ifndef NATIVE_PAYLOAD_VALIDATOR_H
#define NATIVE_PAYLOAD_VALIDATOR_H

#include <stdbool.h>

// Validates the raw payload string for structural integrity
// Checks for unbalanced brackets (e.g. unclosed '[' or ']')
// Returns true if valid, false otherwise.
bool payload_validator_check(const char* raw_payload);

#endif // NATIVE_PAYLOAD_VALIDATOR_H
