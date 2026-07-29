#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include <stdbool.h>
#include "../ssh/channel.h"

#ifdef __cplusplus
extern "C" {
#endif

void config_manager_init(void);
bool config_manager_read_config(TunnelConfig *cfg);

#ifdef __cplusplus
}
#endif

#endif // CONFIG_LOADER_H
