#include <stdlib.h>
#include "log.h"

#define TRACE_ENTRY if (dunes7_trace) log_trace(RED "ENTRY:" CRESET " %s", __func__);

#define TRACE_LOG(fmt, ...) if (dunes7_trace) log_trace(fmt, __VA_ARGS__)

#define TRACE_EXIT if (dunes7_trace) log_trace(RED "EXIT:" CRESET " %s", __func__);

#define TRACE_ENTRY_MSG(fmt, ...) \
    if (dunes7_trace) log_trace(RED "ENTRY:" CRESET " %s, " fmt, __func__, __VA_ARGS__);

#define DUMP_PREFIX(pfx) \
    if (pfx) {                                          \
        log_debug("\tpfx start: '%.15s'", (pfx)->start);     \
        log_debug("\tpfx len: '%d'", (pfx)->len);     \
    } else {                                            \
        log_debug("prefix: NULL");                      \
    }
#define TRACE_LOG_DEBUG(fmt, msg) log_debug(fmt, msg)

#define TRACE_S7_DUMP(lvl, msg, x) if (dunes7_debug > lvl) (({char*s=s7_object_to_c_string(s7, x);log_debug("%s: '%s'", msg, s);fflush(NULL);free(s);}))
#define TRACE_S7_DUMP60(msg, x) (({char*s=s7_object_to_c_string(s7, x);log_debug("%s: '%.60s' (first 60 chars)", msg, s);fflush(NULL);free(s);}))

#define LOG_DEBUG(lvl, fmt, ...) if (dunes7_debug>lvl) log_debug(fmt, __VA_ARGS__)
#define LOG_ERROR(lvl, fmt, ...) if (dunes7_debug>lvl) log_error(fmt, __VA_ARGS__)
#define LOG_INFO(lvl, fmt, ...)  if (dunes7_debug>lvl) log_info(fmt, __VA_ARGS__)
#define LOG_TRACE(lvl, fmt, ...) if (dunes7_debug>lvl) log_trace(fmt, __VA_ARGS__)
#define LOG_WARN(lvl, fmt, ...)  if (dunes7_debug>lvl) log_warn(fmt, __VA_ARGS__)
