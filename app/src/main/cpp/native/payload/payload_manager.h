#ifndef NATIVE_PAYLOAD_MANAGER_H
#define NATIVE_PAYLOAD_MANAGER_H

#include "../ssh/channel.h"

// Initialize the payload manager with the given config.
// Validates, parses, generates headers, and splits the payload.
// Populates cfg->payload_chunks and cfg->num_payload_chunks directly.
// Returns 0 on success, -1 on error.
int payload_manager_build(TunnelConfig* cfg);

#endif // NATIVE_PAYLOAD_MANAGER_H
