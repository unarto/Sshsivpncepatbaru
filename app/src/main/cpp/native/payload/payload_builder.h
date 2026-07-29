#ifndef PAYLOAD_MGR_H
#define PAYLOAD_MGR_H

#include "../ssh/channel.h"

// Returns the FD to be given to libssh2.
// If it fails, returns -1.
int pm_start(const TunnelConfig *cfg);
void pm_stop(void);
void pm_process(void);

#endif
