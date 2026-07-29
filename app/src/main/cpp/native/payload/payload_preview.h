#ifndef NATIVE_PAYLOAD_PREVIEW_H
#define NATIVE_PAYLOAD_PREVIEW_H

#include "payload_placeholder.h"

// Generates a dry-run string of the payload replacing placeholders 
// with context for previewing on UI before actually connecting.
// Returns an allocated string (caller must free).
char* payload_preview_generate(const char* raw_payload, const PayloadPlaceholderContext* ctx);

#endif // NATIVE_PAYLOAD_PREVIEW_H
