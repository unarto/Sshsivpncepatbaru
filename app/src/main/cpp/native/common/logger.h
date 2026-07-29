#ifndef NATIVE_LOGGER_H
#define NATIVE_LOGGER_H
#include <android/log.h>

void native_log(int level, const char* tag, const char* fmt, ...);

#endif
