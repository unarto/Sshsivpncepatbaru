#ifndef NATIVE_RECONNECT_H
#define NATIVE_RECONNECT_H

#include <stdbool.h>

// Initialize reconnect module configuration
void reconnect_init(int max_retries, int base_delay_sec, int max_delay_sec);

// Notify native engine about network interface state change (e.g. WiFi <-> Mobile)
void reconnect_notify_network_changed(bool is_connected);

// Reset exponential backoff state (called on successful auth/connect)
void reconnect_reset_backoff(void);

// Get current retry count
int reconnect_get_retry_count(void);

// Get calculated backoff delay in seconds for current retry attempt
int reconnect_get_next_delay(void);

#endif
