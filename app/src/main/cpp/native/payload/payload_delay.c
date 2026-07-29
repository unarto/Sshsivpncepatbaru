#include "payload_delay.h"
#include <time.h>
#include <stddef.h>

uint64_t payload_delay_now_ms(void) {
    struct timespec ts;
    // Use CLOCK_MONOTONIC to prevent time-jumping issues (like system clock update)
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (uint64_t)(ts.tv_sec * 1000) + (uint64_t)(ts.tv_nsec / 1000000);
    }
    return 0;
}

uint64_t payload_delay_set_target(int delay_ms) {
    if (delay_ms <= 0) return 0;
    return payload_delay_now_ms() + delay_ms;
}

bool payload_delay_is_elapsed(uint64_t target_ms) {
    if (target_ms == 0) return true;
    return payload_delay_now_ms() >= target_ms;
}

int payload_delay_time_left(uint64_t target_ms) {
    if (target_ms == 0) return 0;
    uint64_t now = payload_delay_now_ms();
    if (now >= target_ms) return 0;
    return (int)(target_ms - now);
}
