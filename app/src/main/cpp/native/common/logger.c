#include "logger.h"
#include <android/log.h>
#include <stdarg.h>

void native_log(int level, const char* tag, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    __android_log_vprint(level, tag, fmt, args);
    va_end(args);
}
