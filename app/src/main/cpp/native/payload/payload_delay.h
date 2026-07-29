#ifndef NATIVE_PAYLOAD_DELAY_H
#define NATIVE_PAYLOAD_DELAY_H

#include <stdint.h>
#include <stdbool.h>

// Get current monotonic time in milliseconds
uint64_t payload_delay_now_ms(void);

// Setup a target timestamp based on current time + delay_ms
uint64_t payload_delay_set_target(int delay_ms);

// Check if the delay target has been reached
bool payload_delay_is_elapsed(uint64_t target_ms);

// Determine how many milliseconds are left (returns 0 if elapsed)
int payload_delay_time_left(uint64_t target_ms);

#endif // NATIVE_PAYLOAD_DELAY_H
