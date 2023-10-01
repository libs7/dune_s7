/* #define TRACE_ENTRY */
/* #define TRACE_EXIT */
/* #define TRACE_ENTRY_STR(str) */
/* #define TRACE_S7_DUMP(msg, x) */
/* #define DUMP_PREFIX(pfx) */
/* #define TRACE_LOG_DEBUG(fmt, msg) */
/* #define TRACE_S7_DUMP(msg, x) */
/* #define TRACE_S7_DUMP60(msg, x) */


#include <stdlib.h>
#include "log.h"

#define TRACE_ENTRY log_trace(RED "ENTRY:" CRESET " %s", __func__);
#define TRACE_EXIT log_trace(RED "EXIT:" CRESET " %s", __func__);
#define TRACE_ENTRY_STR(fn, str) \
    if (mibl_trace) log_trace(RED "ENTRY:" CRESET " " #fn ": %s", str);

#define DUMP_PREFIX(pfx) \
    if (pfx) {                                          \
        log_debug("\tpfx start: '%.15s'", (pfx)->start);     \
        log_debug("\tpfx len: '%d'", (pfx)->len);     \
    } else {                                            \
        log_debug("prefix: NULL");                      \
    }
#define TRACE_LOG_DEBUG(fmt, msg) log_debug(fmt, msg)

#define TRACE_S7_DUMP(msg, x) (({char*s=s7_object_to_c_string(s7, x);log_debug("%s: '%s'", msg, s);fflush(NULL);free(s);}))
#define TRACE_S7_DUMP60(msg, x) (({char*s=s7_object_to_c_string(s7, x);log_debug("%s: '%.60s' (first 60 chars)", msg, s);fflush(NULL);free(s);}))

