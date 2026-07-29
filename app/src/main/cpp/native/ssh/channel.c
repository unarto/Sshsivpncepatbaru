#include "channel.h"
#include <stdlib.h>
#include <string.h>

void channel_close(ChannelContext* ctx) {
    if (ctx && ctx->state != CHANNEL_STATE_CLOSED) {
        ctx->state = CHANNEL_STATE_CLOSED;
    }
}
