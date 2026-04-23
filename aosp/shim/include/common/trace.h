// SF-internal trace wrapper; forward everything to ATRACE no-ops.
#pragma once
#include <cutils/trace.h>

#define SFTRACE_CALL() ((void)0)
#define SFTRACE_NAME(name) ((void)0)
#define SFTRACE_INT(name, value) ((void)0)
#define SFTRACE_INT64(name, value) ((void)0)
#define SFTRACE_FORMAT(fmt, ...) ((void)0)
#define SFTRACE_FORMAT_INSTANT(fmt, ...) ((void)0)
#define SFTRACE_INSTANT(name) ((void)0)
#define SFTRACE_ASYNC_BEGIN(name, cookie) ((void)0)
#define SFTRACE_ASYNC_END(name, cookie) ((void)0)
#define SFTRACE_ASYNC_FOR_TRACK_BEGIN(track, name, cookie) ((void)0)
#define SFTRACE_ASYNC_FOR_TRACK_END(track, cookie) ((void)0)
#define SFTRACE_BEGIN(name) ((void)0)
#define SFTRACE_END() ((void)0)
