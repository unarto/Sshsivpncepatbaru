#ifndef NATIVE_PAYLOAD_LOGGER_H
#define NATIVE_PAYLOAD_LOGGER_H

#include <stdint.h>

// Start timing a payload operation
void payload_logger_start_timer(void);

// Log the completion of a payload operation, recording the latency
void payload_logger_log_completion(const char* operation_name);

// Log raw payload data sent
void payload_logger_log_tx(const char* data, size_t length);

// Log raw payload data received
void payload_logger_log_rx(const char* data, size_t length);

#endif // NATIVE_PAYLOAD_LOGGER_H
