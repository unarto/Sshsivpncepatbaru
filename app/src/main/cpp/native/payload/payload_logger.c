#include "payload_logger.h"
#include "payload_delay.h"
#include "../common/logger.h"
#include <stddef.h>

static uint64_t g_start_time = 0;

void payload_logger_start_timer(void) {
    g_start_time = payload_delay_now_ms();
}

void payload_logger_log_completion(const char* operation_name) {
    if (g_start_time == 0) return;
    
    uint64_t now = payload_delay_now_ms();
    uint64_t diff = now - g_start_time;
    
    LOGI("PayloadLogger: [%s] completed in %llu ms", operation_name ? operation_name : "Operation", (unsigned long long)diff);
    
    g_start_time = 0; // Reset
}

void payload_logger_log_tx(const char* data, size_t length) {
    if (!data || length == 0) return;
    // Log preview of first 64 bytes for debugging
    char preview[65];
    size_t preview_len = length > 64 ? 64 : length;
    for(size_t i=0; i<preview_len; i++) {
        preview[i] = data[i] >= 32 && data[i] <= 126 ? data[i] : '.';
    }
    preview[preview_len] = '\0';
    
    LOGI("PayloadLogger TX: [%zu bytes] %s...", length, preview);
}

void payload_logger_log_rx(const char* data, size_t length) {
    if (!data || length == 0) return;
    char preview[65];
    size_t preview_len = length > 64 ? 64 : length;
    for(size_t i=0; i<preview_len; i++) {
        preview[i] = data[i] >= 32 && data[i] <= 126 ? data[i] : '.';
    }
    preview[preview_len] = '\0';
    
    LOGI("PayloadLogger RX: [%zu bytes] %s...", length, preview);
}
