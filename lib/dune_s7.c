/*
  protocol:
  (dune:read)
    g_dune_read
      _g_dune_read
        _dune_read_thunk

  read error:
  (dune:read)
    g_dune_read
      _g_dune_read
        _dune_read_thunk
        _dune_read_catcher
          fix_dunefile
            dunefile_to_string
          _dune_read_thunk
            returns to

 */

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined(PROFILE_fastbuild)
#include <execinfo.h>           /* backtrace */
#include <unistd.h>             /* write */
#endif

#include "liblogc.h"
#include "utstring.h"
/* #include "error_handler_dune.h" */

#if INTERFACE
#include "libs7.h"
#endif

#include "dune_s7_internal.h"

const char *dune_s7_version = DUNE_S7_VERSION;

#if defined(PROFILE_fastbuild)
#define TRACE_FLAG dune_s7_trace
bool    TRACE_FLAG = 0;
#define DEBUG_LEVEL dune_s7_debug
int     DEBUG_LEVEL = 0;
#define S7_DEBUG_LEVEL libs7_debug
extern int libs7_debug;
int     s7plugin_debug = 0;
#endif

/* bool  verbose; */

s7_pointer c_pointer_string, string_string, character_string, boolean_string, real_string, complex_string;
s7_pointer integer_string;
static s7_pointer int64_t__symbol, FILE__symbol;

s7_pointer result;

s7_pointer _inport_sym;         /* -dune-inport */
s7_pointer _infile_sym;         /* -dune-infile */
s7_pointer _dune_sexps;          /* -dune-sexps */

/* needed by read thunk and catcher */
/* const char *g_dunefile; */
/* s7_pointer g_dune_inport; */
/* s7_int gc_dune_inport = -1; */
/* s7_pointer g_stanzas; */
/* s7_int gc_stanzas = -1; */

s7_pointer e7; // tmp var for error printing
const char *e; // tmp var for error printing

const char *errmsg;

s7_pointer _dune_read_catcher_s7;
s7_int gc_dune_read_catcher_s7 = -1;

static s7_pointer _dune_read_input_port(s7_scheme*s7, s7_pointer port);
static s7_pointer _dune_read_string(s7_scheme*s7, s7_pointer s);

s7_pointer _fix_dunefile(s7_scheme *s7, const char *dunefile_name)
{
    TRACE_ENTRY;
    TRACE_LOG(0, "dunefile: %s", dunefile_name);

    const char *dunestring = dunefile_to_string(s7, dunefile_name);

    /* LOG_DEBUG(1, "read corrected string: %s", dunestring); */

    /* now s7_read using string port */

    /* first config err handling. clears out prev. error */
    /* close_error_config(); */
    /* error_config(); */
    /* init_error_handling(); */

    s7_pointer _inport = s7_open_input_string(s7, dunestring);
    if (!s7_is_input_port(s7, _inport)) { // g_dune_inport)) {
        LOG_ERROR(0, "BAD INPUT PORT", "");
        s7_error(s7, s7_make_symbol(s7, "bad input port"),
                     s7_nil(s7));
    }

    s7_pointer readlet
        = s7_inlet(s7,
                   s7_list(s7, 3,
                           s7_cons(s7, _inport_sym, _inport),
                           s7_cons(s7, _infile_sym,
                                   s7_make_string(s7, dunefile_name)),
                           s7_cons(s7, _dune_sexps, s7_nil(s7))));

    LOG_DEBUG(1, "reading corrected string from port: %s", dunestring);
    const char *dune = "(catch #t -dune-read-thunk -dune-read-catcher)";
    // same as (with-let (inlet ...) (catch...)) ???
    /* LOG_DEBUG(1, "evaluating c string %s", dune); */
    s7_pointer result
        = s7_eval_c_string_with_environment(s7, dune, readlet);

    /* result = _dune_read_input_port(s7, _inport); */

    /* TRACE_LOG_DEBUG(1, "reading corrected string:  %s", dunestring); */
    /* result = _dune_read_string(s7, s7_make_string(s7,dunestring)); */

    TRACE_S7_DUMP(1, "fixed stanzas: %s", result);
    s7_close_input_port(s7, _inport);
    /* free((void*)dunestring); */
    return result;

    /* **************************************************************** */
    //OBSOLETE

    if (s7_curlet(s7) == s7_nil(s7)) {
        log_warn("curlet is '()");
    }
    TRACE_S7_DUMP(1, "curlet: %s", s7_curlet(s7));
    /* } else { */
    /* LOG_DEBUG(1, "varletting curlet..."); */
    /* s7_varlet(s7, s7_curlet(s7), */
    /*            s7_make_symbol(s7, "-dune-inport"), */
    /*            _inport); */

    /* s7_varlet(s7, s7_curlet(s7), */
    /*               s7_make_symbol(s7, "-dune-infile"), */
    /*               s7_make_string(s7, dunefile_name)); */

    /* s7_varlet(s7, s7_curlet(s7), */
    /*               s7_make_symbol(s7, "-dune-dunes"), */
    /*               s7_nil(s7)); */

    LOG_DEBUG(1, "DUNEFILE_NAME: %s", dunefile_name);
    s7_pointer env = s7_inlet(s7,
                              s7_list(s7, 1,
                                      s7_cons(s7, _inport_sym, _inport),
                                      s7_cons(s7, _infile_sym,
                                              s7_make_string(s7, dunefile_name)),

                                      s7_cons(s7, _dune_sexps, s7_nil(s7))
                                      ));
    // WARNING: if the call to -g-dune-read raises an error that gets
    // handled by the catcher, then the continuation is whatever
    // called this routine (i.e. the c stack), NOT the assignment to
    // result below.
    s7_pointer stanzas = s7_eval_c_string_with_environment(s7, "(apply -g-dune-read (list -dune-inport))", env);

    //FIXME: this should use s7_eval_c_string... in case dunestring
    //has an error.

    // (with-let (inlet ...)
    //        (with-input-from-string dunestring dune:read))

    /* s7_pointer stanzas = s7_call_with_catch(s7, */
    /*                                 s7_t(s7),      /\* tag *\/ */
    /*                                 // _dune_read_thunk_s7 */
    /*                                 s7_name_to_value(s7, "-dune-read-thunk"), */
    /*                                 _dune_read_catcher_s7 */
    /*                                 /\* s7_name_to_value(s7, "-dune-read-thunk-catcher") *\/ */
    /*                                 ); */
    TRACE_S7_DUMP(1, "fixed stanzas", stanzas);

    /* s7_close_input_port(s7, sport); */
    // g_dune_inport will be closed by caller
    /* s7_gc_unprotect_at(s7, gc_baddot_loc); */
    /* close_error_config(); */

    /* leave error config as-is */
    /* free((void*)dunestring); */
    /* return s7_reverse(s7, stanzas); */
    return stanzas;
}

#if defined(PROFILE_fastbuild)
/* https://stackoverflow.com/questions/6934659/how-to-make-backtrace-backtrace-symbols-print-the-function-names */
static void full_write(int fd, const char *buf, size_t len)
{
        while (len > 0) {
                ssize_t ret = write(fd, buf, len);

                if ((ret == -1) && (errno != EINTR))
                        break;

                buf += (size_t) ret;
                len -= (size_t) ret;
        }
}

void print_c_backtrace(void) // s7_scheme *s7)
{
    static const char s7_start[] = "S7 STACKTRACE ------------\n";
    static const char c_start[] = "C BACKTRACE ------------\n";
    static const char end[] = "----------------------\n";

    full_write(STDERR_FILENO, s7_start, strlen(s7_start));

    /* s7_show_stack(s7); */
    /* s7_stacktrace(s7); */
    /* s7_flush_output_port(s7, s7_current_error_port(s7)); */
    /* fflush(NULL); */

    full_write(STDERR_FILENO, end, strlen(end));

    void *bt[1024];
    int bt_size;
    char **bt_syms;
    int i;

    bt_size = backtrace(bt, 1024);
    bt_syms = backtrace_symbols(bt, bt_size);
    full_write(STDERR_FILENO, c_start, strlen(c_start));
    for (i = 1; i < bt_size; i++) {
        size_t len = strlen(bt_syms[i]);
        full_write(STDERR_FILENO, bt_syms[i], len);
        full_write(STDERR_FILENO, "\n", 1);
    }
    full_write(STDERR_FILENO, end, strlen(end));
    free(bt_syms);
    /* fflush(NULL); */
}
#endif

/*
 * precondition: env contains -dune-inport, -dune-infile, -dune-dunes (accumulator)
 */
s7_pointer _dune_read_thunk(s7_scheme *s7, s7_pointer args)
/* s7_pointer _dune_read_port(s7_scheme *s7, s7_pointer inport) */
{
    TRACE_ENTRY;
    (void)args;

    s7_pointer _curlet = s7_curlet(s7);
#if defined(PROFILE_fastbuild)
    /* LOG_TRACE(1, "cwd: %s", getcwd(NULL, 0)); */
    /* char *tmp = s7_object_to_c_string(s7, _curlet); */
    /* LOG_DEBUG(1, "CURLET: %s", tmp); */
    /* free(tmp); */
#endif
    TRACE_S7_DUMP(1, "args", args);

    s7_pointer _inport = s7_let_ref(s7, s7_curlet(s7), _inport_sym);
    TRACE_S7_DUMP(1, "inport", _inport);
    if (!s7_is_input_port(s7, _inport)) {
        LOG_ERROR(0, "Bad input port", "");
        s7_error(s7, s7_make_symbol(s7, "bad-input-port"), s7_nil(s7));
    }

    // PROBLEM: if we hit a read-error after expanding an include,
    // then the corrected string will be passed as a string port, and
    // the original filename will be lost. But we need the dirname of
    // the original file because the include file is relative. So the
    // catcher puts -dune-infile into the curlet.
    // WARNING: we only use this to read files, so no support for string ports
    const char *dunefile = s7_port_filename(s7, _inport); // g_dune_inport);
    if (dunefile == NULL) {
        /* LOG_DEBUG(1, "no filename for inport"); */
        s7_pointer _curlet = s7_curlet(s7);
        if (_curlet != s7_nil(s7)) {
            s7_pointer dunefile7 = s7_let_ref(s7, s7_curlet(s7), _infile_sym);
            TRACE_S7_DUMP(1, "curlet -dune-infile", dunefile7);
            dunefile = s7_string(dunefile7);
        }
    /* } else { */
    /*     LOG_DEBUG(1, "inport filename: %s", dunefile); */
    }
    LOG_DEBUG(1, "inport file: %s", dunefile);

    s7_pointer _dunes = s7_nil(s7);
    if (_curlet != s7_nil(s7)) {
        s7_pointer x = s7_let_ref(s7, _curlet, _dune_sexps);
        if (x != s7_nil(s7))
            _dunes = s7_cons(s7, x, _dunes);
        TRACE_S7_DUMP(1, "00 _dunes", _dunes);
    }

    /* g_stanzas = s7_list(s7, 0); // s7_nil(s7)); */
    /* gc_stanzas = s7_gc_protect(s7, g_stanzas); */

    // so read thunk can access port:
    /* g_dunefile_port = inport; */
    /* gc_dune_inport = s7_gc_protect(s7, g_dunefile_port); */
    /* gc_dune_inport = s7_gc_protect(s7, g_dune_inport); */

#if defined(PROFILE_fastbuild)
    LOG_DEBUG(1, "reading stanzas", "");
    // from dunefile: %s", dunefile);
#endif

    //FIXME: error handling
    /* close_error_config(); */
    /* error_config(); */
    /* init_error_handling(); */

    /* s7_show_stack(s7); */
/* #if defined(PROFILE_fastbuild) */
/*     print_c_backtrace(); */
/* #endif */

    s7_gc_on(s7, false);

    /* repeat until all objects read */
    s7_pointer stanza;
    while(true) {
#if defined(PROFILE_fastbuild)
        LOG_TRACE(1, "reading next stanza", "");
#endif

        TRACE_S7_DUMP(1, "_dunes before read", _dunes);

        /* s7_show_stack(s7); */
        /* print_c_backtrace(); */
        stanza = s7_read(s7, _inport); // g_dune_inport);
        if (stanza == s7_eof_object(s7)) {
            /* LOG_DEBUG(1, "EOF"); */
            break;
        }

/* #if defined(PROFILE_fastbuild) */
        TRACE_S7_DUMP(1, "Readed stanza", stanza);
        TRACE_S7_DUMP(1, "_dunes after read", _dunes);

/* #endif */
        /* s7_show_stack(s7); */
        /* print_c_backtrace(); */
        /* errmsg = s7_get_output_string(s7, s7_current_error_port(s7)); */
        /* LOG_ERROR(0, "errmsg: %s", errmsg); */


        /* close_error_config(); */
        /* init_error_handling(); */
        /* error_config(); */
        /* s7_gc_unprotect_at(s7, gc_dune_inport); */

        if (stanza == s7_eof_object(s7)) {
#if defined(PROFILE_fastbuild)
            LOG_TRACE(1, "readed eof", "");
#endif
            break;
        }

        /* LOG_S7_DEBUG("DUNE", stanza); */
        /* if (mibl_debug_traversal) */
        /*     LOG_S7_DEBUG("stanza", stanza); */


        // handle (include ...) stanzas
        if (s7_is_pair(stanza)) {
            if (s7_is_equal(s7, s7_car(stanza),
                            s7_make_symbol(s7, "include"))) {
                LOG_DEBUG(1, "FOUND (include ...)", "");
                /* we can't insert a comment, e.g. ;;(include ...)
                   instead we would have to put the included file in an
                   alist and add a :comment entry. but we needn't bother,
                   we're not going for roundtrippability.
                */

                s7_pointer inc_file = s7_cadr(stanza);
                TRACE_S7_DUMP(1, "    including", inc_file);

                LOG_DEBUG(1, "dunefile: %s", dunefile);
                char *tmp = strdup(dunefile);
                const char *dir = dirname(tmp);
                LOG_DEBUG(1, "dir: %s", dir);

                UT_string *dunepath;
                utstring_new(dunepath);
                const char *tostr = s7_object_to_c_string(s7, inc_file);
                LOG_DEBUG(1, "INCFILE: %s", tostr);
                utstring_printf(dunepath,
                                "%s/%s",
                                //FIXME: dirname may mutate its arg
                                //dirname(path),
                                dir,
                                tostr);
                LOG_DEBUG(1, "dunepath: %s", utstring_body(dunepath));
                /* g_dunefile_port = dunefile_port; */
                /* /\* LOG_S7_DEBUG("nested", nested); *\/ */
                /* /\* LOG_S7_DEBUG("stanzas", stanzas); *\/ */

                if (s7_name_to_value(s7, "*dune:expand-includes*")
                    == s7_t(s7)) {

                    s7_pointer env = s7_inlet(s7,
                                              s7_list(s7, 1,
                                                      s7_cons(s7,
                                      s7_make_symbol(s7, "datafile"),
                        s7_make_string(s7, utstring_body(dunepath)))));

                    LOG_DEBUG(1, "expanding: %s", utstring_body(dunepath));
                    //FIXME: use
                    // (with-let (inlet ...) (with-input-from-file ...))
                    s7_pointer expanded
                        = s7_eval_c_string_with_environment(s7,
                           "(with-input-from-file datafile dune:read)",
                                                            env);
                    LOG_DEBUG(1, "expansion completed", "");

                /* s7_pointer expanded = s7_call_with_catch(s7, */
                /*                     s7_t(s7),      /\* tag *\/ */
                /*                     // _dune_read_thunk_s7 */
                /*                     s7_name_to_value(s7, "-dune-read-thunk"), */
                /*                     _dune_read_catcher_s7 */
                /*                     /\* s7_name_to_value(s7, "-dune-read-thunk-catcher") *\/ */
                /*                     ); */

                    /* _dunes = s7_append(s7, expanded, _dunes); */
                    TRACE_S7_DUMP(1, "A_dunes before", _dunes);
                    _dunes = s7_append(s7, s7_reverse(s7, expanded), _dunes);
                    TRACE_S7_DUMP(1, "A_dunes after", _dunes);
                    /* const char *x = s7_object_to_c_string(s7, _dunes); */
                    /* LOG_DEBUG(1, "_dunes w/inc: %s", x); */
                    /* free((void*)x); */

                    /* s7_pointer cmt = s7_cons(s7, */
                    /*                          s7_make_symbol(s7, "dune:cmt"), */
                    /*                          s7_list(s7, 1, */
                    /*                                  stanza)); */
                    /* g_stanzas = s7_cons(s7, cmt, g_stanzas); */
                } else {
                    TRACE_S7_DUMP(1, "X_dunes before", _dunes);
                    _dunes = s7_cons(s7, stanza, _dunes);
                    TRACE_S7_DUMP(1, "X_dunes after", _dunes);
                }
                /* free((void*)tmp); */
                utstring_free(dunepath);

            } else {
                TRACE_S7_DUMP(1, "_dunes BEFORE", _dunes);
                _dunes = s7_cons(s7, stanza, _dunes);
                TRACE_S7_DUMP(1, "_dunes AFTER", _dunes);

                /* g_stanzas = s7_cons(s7, stanza, g_stanzas); */
                /* TRACE_S7_DUMP(1, "g_stanzas", g_stanzas); */
                /* if (s7_is_null(s7,stanzas)) { */
                /*     stanzas = s7_cons(s7, stanza, stanzas); */
                /* } else{ */
                /*     stanzas = s7_append(s7,stanzas, s7_list(s7, 1, stanza)); */
                /* } */
            }
        } else {
            /* stanza not a pair - automatically means corrupt dunefile? */
            LOG_ERROR(0, "corrupt dune file? %s\n",
                      /* utstring_body(dunefile_name) */
                      "FIXME"
                      );
            s7_error(s7, s7_make_symbol(s7, "corrupt-dune-file"),
                     s7_nil(s7));
        }
    }
    /* s7_gc_unprotect_at(s7, gc_dune_inport); */
    /* fprintf(stderr, "s7_gc_unprotect_at gc_dune_inport: %ld\n", (long)gc_dune_inport); */
    /* s7_close_input_port(s7, inport); */
    // g_dune_inport must be closed by caller (e.g. with-input-from-file)

#if defined(PROFILE_fastbuild)
    LOG_DEBUG(1, "finished reading dunefile: %s", dunefile);
#endif

    s7_gc_on(s7, true);

    /* return _dunes; */
    return s7_reverse(s7, _dunes);
    /* return g_stanzas; */
    /* s7_close_input_port(s7, dunefile_port); */
    /* s7_gc_unprotect_at(s7, gc_loc); */
}

s7_pointer _dune_read_thunk_s7; /* initialized by init fn */

/* impl of _dune_read_thunk_s7 */
/* call by s7_call_with_catch as body arg*/
/* s7_pointer x_dune_read_thunk(s7_scheme *s7, s7_pointer args) { */
/*     (void)args; */
/*     TRACE_ENTRY; */
/* /\* #if defined(PROFILE_fastbuild) *\/ */
/* /\*     print_c_backtrace(); *\/ */
/* /\* #endif *\/ */
/*     /\* LOG_DEBUG(1, "reading dunefile: %s", g_dunefile); *\/ */

/*     return _dune_read_port(s7, g_dune_inport); */
/* } */

void _log_read_error(s7_scheme *s7)
{
    const char *s;
    s7_pointer ow_let = s7_call(s7, s7_name_to_value(s7, "owlet"), s7_nil(s7));
    s7_pointer edatum = s7_call(s7, ow_let,
                               s7_cons(s7,
                                       s7_make_symbol(s7, "error-type"),
                                       s7_nil(s7)));
    s = s7_symbol_name(edatum);
    log_warn("error-type: %s", s);

    edatum = s7_call(s7, ow_let,
                     s7_cons(s7,
                             s7_make_symbol(s7, "error-data"),
                             s7_nil(s7)));
    /* s = s7_object_to_c_string(s7, edatum); */
    /* log_warn("error-data: %s", s); */
    /* free((void*)s); */

    edatum = s7_call(s7, ow_let,
                     s7_cons(s7,
                             s7_make_symbol(s7, "error-code"),
                             s7_nil(s7)));
    /* s = s7_object_to_c_string(s7, edatum); */
    /* log_warn("error-code: %s", s); */
    /* free((void*)s); */

    edatum = s7_call(s7, ow_let,
                     s7_cons(s7,
                             s7_make_symbol(s7, "error-file"),
                             s7_nil(s7)));
    /* s = s7_object_to_c_string(s7, edatum); */
    /* log_warn("error-file: %s", s); */
    /* free((void*)s); */
}

// s7_pointer _dune_read_catcher_s7; /* initialized by init fn */

/* call by s7_call_with_catch as error_handler arg
   arg0: err symbol, e.g.'read-error
   arg1: msg, e.g. ("unexpected close paren: ...
   WARNING: we do not correct the error here, just return a sym so
   that caller can decide on corrective action.
   WARNING: printing s7_stacktrace clobbers globals, esp. g_dunefile!!!
 */
static s7_pointer _dune_read_catcher(s7_scheme *s7, s7_pointer args)
{
    (void)s7;
    (void)args;
    TRACE_ENTRY;
    TRACE_S7_DUMP(1, "args", args);

#if defined(PROFILE_fastbuild)
    s7_pointer owlet7 = s7_eval_c_string(s7,  "(owlet)");
    const char *owlet = s7_object_to_c_string(s7, owlet7);
    LOG_DEBUG(1, "owlet: %s", owlet);
    free((void*)owlet);
#endif

    //FIXME: what if error is in a string instead of a file?
    s7_pointer errfile7 = s7_eval_c_string(s7,  "((owlet) 'error-file)");
    const char *errfile = s7_string(errfile7);
    LOG_DEBUG(1, "errfile: %s", errfile);
    /* free((void*)errfile); */

    s7_pointer errtype7 = s7_eval_c_string(s7,  "((owlet) 'error-type)");
    const char *errtype = s7_object_to_c_string(s7, errtype7);
    (void)errtype;
    /* LOG_DEBUG(1, "errtype: %s", errtype); */
    if (errtype7 == s7_make_symbol(s7, "io-error")) {
        LOG_ERROR(0, "io-error: %s", errtype);
        s7_pointer errdata7 = s7_eval_c_string(s7,  "((owlet) 'error-data)");
        LOG_ERROR(0, "cwd: %s", getcwd(NULL, 0));
        s7_error(s7, s7_make_symbol(s7, "io-error"), errdata7);
                 /* s7_cons(s7, errdata7, s7_nil(s7))); */
    }
    /* free((void*)errtype); */

#if defined(PROFILE_fastbuild)
    // WARNING: (curlet) may be empty
    s7_pointer _curlet = s7_curlet(s7);
    TRACE_S7_DUMP(1, "catch curlet", _curlet);
    /* TRACE_S7_DUMP(1, "outlet", s7_outlet(s7, s7_curlet(s7))); */

    // debugging msgs only
    if (_curlet == s7_nil(s7)) {
        LOG_ERROR(0, "catch curlet is empty", "");
        /* s7_error(s7, s7_make_symbol(s7, "empty curlet"), s7_nil(s7)); */
    } else {
        s7_pointer _inport;
        (void)_inport;
        _inport = s7_let_ref(s7, s7_curlet(s7),
                             s7_make_symbol(s7, "-dune-inport"));
        TRACE_S7_DUMP(1, "catch curlet inport", _inport);

        s7_pointer _dunes;
        (void)_dunes;
        _dunes = s7_let_ref(s7, s7_curlet(s7),
                                       s7_make_symbol(s7, "-dune-dunes"));
        TRACE_S7_DUMP(1, "catch dunes", _dunes);
    }

    /* if (verbose) { */
        log_warn("Error reading dunefile: %s", errfile);
        e7 = s7_eval_c_string(s7,  "((owlet) 'error-data)");
        e = s7_object_to_c_string(s7, e7);
        log_warn("error-data: %s", e);
        free((void*)e);
        e7 = s7_eval_c_string(s7,  "((owlet) 'error-position)");
        e = s7_object_to_c_string(s7, e7);
        log_warn("error-position: %s", e);
        free((void*)e);
        e7 = s7_eval_c_string(s7,  "((owlet) 'error-line)");
        e = s7_object_to_c_string(s7, e7);
        log_warn("Error-line: %s", e);
        /* free((void*)e); */
    /* } */
#endif

/* #if defined(PROFILE_fastbuild) */
/*     print_c_backtrace(); */
/* #endif */

    /* if (verbose) { */
    /*     LOG_INFO(0, "fixing dunefile: %s", errfile); */
    /* } */

    /* s7_pointer err_sym = s7_car(args); */

    // we need the error msg, we can get it from either args
    // or the owlet (key 'error-data).
    /* s7_pointer err_msg = s7_cadr(args); */

/* #if defined(PROFILE_fastbuild) */
/*     TRACE_S7_DUMP(1, "s7_read_catcher err sym", err_sym); */
/*     TRACE_S7_DUMP(1, "s7_read_catcher err msg", err_msg); */
/* #endif */

    /* _log_read_error(s7); */

    s7_pointer fixed = _fix_dunefile(s7, errfile);

    TRACE_EXIT;
    return fixed;

    /* const char *s = s7_object_to_c_string(s7, err_msg); */
    /* log_warn("error-data: %s", s); */
    /* if (strstr(s, "(\"unexpected close paren:") != NULL) { */
    /*     free((void*)s); */
    /*     return s7_make_symbol(s7, "dune-baddot-error"); */
    /* } */
    /* else if (strstr(s, */
    /*                 "(\"end of input encountered while in a string") != NULL) { */
    /*     free((void*)s); */
    /*     return s7_make_symbol(s7, "dune-eol-string-error"); */
    /* } */

    /* return s7_make_symbol(s7, "dune-read-error"); */
}

/* ****************************************************************
   internal dune:read impl
   uses s7_call_with_catch
 */
static s7_pointer _g_dune_read(s7_scheme *s7, s7_pointer args)
{

    //FIXME: call read_dunefile(char *path)?

    TRACE_ENTRY;
    s7_pointer src;
    TRACE_S7_DUMP(1, "args", args);

    TRACE_S7_DUMP(1, "curlet", s7_curlet(s7));

    s7_gc_on(s7, false);

/* #if defined(PROFILE_fastbuild) */
/*     print_c_backtrace(); */
/* #endif */

    /* s7_gc_on(s7, false); */

    /* src = s7_car(args); */
    /* inc = s7_cadr(args); */
    /* g_expand_includes = s7_boolean(s7, inc); */
    /* g_expand_includes = true; */
    if (args == s7_nil(s7)) {
        LOG_DEBUG(1, "SOURCE: current-input-port", "");
        /* result = _dune_read_port(s7, s7_current_input_port(s7)); */

        /* g_dune_inport = s7_current_input_port(s7); */
        /* gc_dune_inport = s7_gc_protect(s7, g_dune_inport); */

        s7_pointer _curlet = s7_curlet(s7);
        /* TRACE_S7_DUMP(1, "g_dune_read curlet", _curlet); */

        s7_pointer _dune_inport_sym = s7_make_symbol(s7, "-dune-inport");

        s7_pointer _inport = s7_current_input_port(s7);
        if (s7_let_ref(s7, _curlet, _dune_inport_sym)) {
            LOG_DEBUG(1, "-dune-inport in curlet", "");
            s7_let_set(s7, _curlet, _dune_inport_sym, _inport);
        } else {
            LOG_DEBUG(1, "adding -dune-inport to curlet", "");
            s7_varlet(s7, _curlet, _dune_inport_sym, _inport);
        }
        // get dunefile before calling read thunk, since
        // read errors may close the port
        const char *dunefile = s7_port_filename(s7, _inport);
        s7_varlet(s7, s7_curlet(s7),
                  s7_make_symbol(s7, "-dune-infile"),
                  s7_make_string(s7, dunefile));

        s7_varlet(s7, s7_curlet(s7),
                  s7_make_symbol(s7, "-dune-dunes"),
                  s7_nil(s7));

/* #if defined(PROFILE_fastbuild) */
/*         const char *dunefile = s7_port_filename(s7, g_dune_inport); */
/*         LOG_DEBUG(1, "reading dunefile: %s", dunefile); */
/* #endif */

        result = s7_call_with_catch(s7,
                                    s7_t(s7),      /* tag */
                                    s7_name_to_value(s7, "-dune-read-thunk"),
                                    _dune_read_catcher_s7
                                    /* s7_name_to_value(s7, "-dune-read-thunk-catcher") */
                                    );

        if (s7_is_symbol(result)) {
            LOG_INFO(0, "read error; correcting...", "");
            if (result == s7_make_symbol(s7, "dune-baddot-error")) {
                LOG_DEBUG(1, "fixing baddot error for %s", dunefile);
                TRACE_S7_DUMP(1, "_inport", _inport);
                /* s7_gc_unprotect_at(s7, gc_stanzas); */
                /* s7_gc_unprotect_at(s7, gc_dune_inport); */
                /* s7_gc_unprotect_at(s7, gc_dune_read_catcher_s7); */

#if defined(PROFILE_fastbuild)
                /* s7_pointer _curlet = s7_curlet(s7); */
                /* TRACE_S7_DUMP(1, "fixing, curlet", _curlet); */
                /* s7_pointer _outlet = s7_outlet(s7, _curlet); */
                /* TRACE_S7_DUMP(1, "fixing, outlet", _outlet); */
#endif
                s7_pointer fixed = _fix_dunefile(s7, dunefile);
                s7_gc_on(s7, true);
                return fixed;
            }
            else if (result == s7_make_symbol(s7, "dune-eol-string-error")) {
                /* const char *dunefile = s7_port_filename(s7, g_dune_inport); */
                /* LOG_DEBUG(1, "fixing eol-string error for %s", dunefile); */
                /* TRACE_S7_DUMP(1, "stanzas readed so far", g_stanzas); */
                /* s7_gc_unprotect_at(s7, gc_stanzas); */
                s7_gc_unprotect_at(s7, gc_dune_read_catcher_s7);
                /* s7_gc_unprotect_at(s7, gc_dune_inport); */
                /* s7_close_input_port(s7, g_dune_inport); */

                /* s7_close_input_port(s7, _inport); */

                s7_pointer fixed = _fix_dunefile(s7, dunefile);
                s7_gc_on(s7, true);

                return fixed;
            }
        }
            /* s7_gc_unprotect_at(s7, gc_dune_inport); */
            /* s7_gc_unprotect_at(s7, gc_dune_read_catcher_s7); */
        TRACE_S7_DUMP(1, "_g_dune_read call/catch result", result);
        /* s7_flush_output_port(s7, s7_current_output_port(s7)); */
        s7_gc_on(s7, true);

        return s7_reverse(s7, result);

    } else {

        src = s7_car(args);
        if (s7_is_input_port(s7, src)) {
            LOG_DEBUG(1, "SOURCE: input port", "");
            /* return _dune_read_port(s7, src); */
            /* g_dune_inport = src; */

        s7_pointer _curlet = s7_curlet(s7);
        /* TRACE_S7_DUMP(1, "g_dune_read curlet", _curlet); */

        s7_pointer _dune_inport_sym = s7_make_symbol(s7, "-dune-inport");

        /* s7_pointer _inport = s7_current_input_port(s7); */
            s7_pointer _inport = src;
            /* s7_varlet(s7, s7_curlet(s7), */
            /*           s7_make_symbol(s7, "-dune-inport"), */
            /*           _inport); */
            /* s7_pointer x7 =  s7_let_ref(s7, _curlet, _dune_inport_sym); */
            /* TRACE_S7_DUMP(1, "x7", x7); */
            if (s7_let_ref(s7, _curlet, _dune_inport_sym)
                == s7_undefined(s7)) {
                LOG_DEBUG(1, "adding -dune-inport to curlet", "");
                s7_varlet(s7, _curlet, _dune_inport_sym, _inport);
            } else {
                LOG_DEBUG(1, "-dune-inport in curlet", "");
                s7_let_set(s7, _curlet, _dune_inport_sym, _inport);
            }

            const char *dunefile = s7_port_filename(s7, _inport);
            if (dunefile) {
                s7_varlet(s7, s7_curlet(s7),
                          s7_make_symbol(s7, "-dune-infile"),
                          s7_make_string(s7, dunefile));
            } else {
            }
            s7_varlet(s7, s7_curlet(s7),
                      s7_make_symbol(s7, "-dune-dunes"),
                      s7_nil(s7));

            result = s7_call_with_catch(s7,
                                        s7_t(s7),      /* tag */
                                        // _dune_read_thunk_s7
                                        s7_name_to_value(s7, "-dune-read-thunk"),
                                        _dune_read_catcher_s7
                                        /* s7_name_to_value(s7, "-dune-read-thunk-catcher") */
                                        );

            /* dunefile = s7_port_filename(s7, _inport); */
            if (result == s7_make_symbol(s7, "dune-baddot-error")) {
                LOG_DEBUG(1, "fixing baddot error for %s", dunefile);
                TRACE_S7_DUMP(1, "_inport", _inport);
                /* s7_gc_unprotect_at(s7, gc_stanzas); */
                /* s7_gc_unprotect_at(s7, gc_dune_inport); */
                s7_gc_unprotect_at(s7, gc_dune_read_catcher_s7);

#if defined(PROFILE_fastbuild)
                s7_pointer _curlet = s7_curlet(s7);
                TRACE_S7_DUMP(1, "fixing, curlet", _curlet);
                s7_pointer _outlet = s7_outlet(s7, _curlet);
                (void)_outlet;
                TRACE_S7_DUMP(1, "fixing, outlet", _outlet);
#endif
                s7_pointer fixed = _fix_dunefile(s7, dunefile);
                s7_gc_on(s7, true);
                return fixed;
            }
            else if (result == s7_make_symbol(s7, "dune-eol-string-error")) {
                /* const char *dunefile = s7_port_filename(s7, g_dune_inport); */
                /* LOG_DEBUG(1, "fixing eol-string error for %s", dunefile); */
                /* TRACE_S7_DUMP(1, "stanzas readed so far", g_stanzas); */
                /* s7_gc_unprotect_at(s7, gc_stanzas); */
                s7_gc_unprotect_at(s7, gc_dune_read_catcher_s7);
                /* s7_gc_unprotect_at(s7, gc_dune_inport); */
                /* s7_close_input_port(s7, g_dune_inport); */

                /* s7_close_input_port(s7, _inport); */

                s7_pointer fixed = _fix_dunefile(s7, dunefile);
                s7_gc_on(s7, true);
                return fixed;
            }

            /* s7_gc_unprotect_at(s7, gc_dune_inport); */
            /* s7_gc_unprotect_at(s7, gc_dune_read_catcher_s7); */
            TRACE_S7_DUMP(1, "_g_dune_read call/catch result", result);
            /* s7_flush_output_port(s7, s7_current_output_port(s7)); */
            s7_gc_on(s7, true);
            return s7_reverse(s7, result);

            /* LOG_DEBUG(1, "read_thunk result", result); */
            /* return result; */
        }
        else if (s7_is_string(src)) {
            LOG_DEBUG(1, "SOURCE: string", "");
            /* dune_str = (char*)s7_string(src); */
        }
        else {
            s7_gc_on(s7, true);
            return(s7_wrong_type_error(s7, s7_make_string_wrapper_with_length(s7, "dune:read", 10), 1, src, string_string));
        }
    }
    s7_gc_on(s7, true);

    return s7_f(s7);
}

/* **************************************************************** */
static s7_pointer _dune_read_current_input_port(s7_scheme*s7)
{
    TRACE_ENTRY;

    s7_pointer _inport = s7_current_input_port(s7);

    return _dune_read_input_port(s7, _inport);

    /* s7_pointer port_filename = s7_call(s7, */
    /*                                    s7_name_to_value(s7, "port-filename"), */
    /*                                    s7_list(s7, 1, _inport)); */
    /* TRACE_S7_DUMP(1, "cip fname", port_filename); */
    //FIXME: need we add fname to readlet?

    /* s7_pointer readlet */
    /*     = s7_inlet(s7, */
    /*                s7_list(s7, 2, */
    /*                        s7_cons(s7, _inport_sym, _inport), */
    /*                        s7_cons(s7, _dune_sexps,  s7_nil(s7)))); */

    /* const char *dune = "(catch #t -dune-read-thunk -dune-read-catcher)"; */
    /* // same as (with-let (inlet ...) (catch...)) ??? */
    /* /\* LOG_DEBUG(1, "evaluating c string %s", dune); *\/ */
    /* s7_pointer stanzas */
    /*     = s7_eval_c_string_with_environment(s7, dune, readlet); */

    /* TRACE_S7_DUMP(1, "cip readed stanazas", stanzas); */

    /* return stanzas; */
    /* return s7_make_string(s7,  "testing _dune_read_current_input_port"); */
}

static s7_pointer _dune_read_input_port(s7_scheme*s7, s7_pointer inport)
{
    TRACE_ENTRY;
    (void)inport;

    const char *dunefile = s7_port_filename(s7, inport);
    if (dunefile == NULL) {
        LOG_INFO(0, "string port (inport w/o filename)", "");
        s7_pointer _curlet = s7_curlet(s7);
        char *tmp = s7_object_to_c_string(s7, _curlet);
        (void)tmp;
        LOG_DEBUG(1, "CURLET: %s", tmp);
        s7_pointer dunefile7 = s7_let_ref(s7, s7_curlet(s7),
                               s7_make_symbol(s7, "-dune-infile"));
        /* TRACE_S7_DUMP(1, "dunefile7", dunefile7); */
        dunefile = s7_string(dunefile7);

        log_error("FIXME");
        exit(EXIT_FAILURE);
    /* } else { */
    /*     LOG_DEBUG(1, "inport filename: %s", dunefile); */
    } else {
        LOG_INFO(0, "inport w/filename: %s", dunefile);
        char *tmp = strndup(dunefile, strlen(dunefile));
        char *dunedir = dirname(tmp);
        char *dunetext = read_dunefile(dunefile);
        /* log_debug("dunetext (%d):\n%s", strlen(dunetext), dunetext); */

        s7_pointer dunetext_s7 = s7_make_string(s7, dunetext);

        s7_pointer env = s7_inlet(s7,
                                  s7_list(s7, 2,
                                          s7_cons(s7,
                                                  s7_make_symbol(s7, "dunesexps"),
                                                  dunetext_s7),
                                          s7_cons(s7,
                                                  s7_make_symbol(s7, "dunedir"),
                                                  s7_make_string(s7, dunedir))));
        LOG_DEBUG(1, "reading dunetext", "");
        s7_pointer stanzas
            = s7_eval_c_string_with_environment(s7,
                                                "(call-with-input-string dunesexps "
                                                " (lambda (p) "
                                                "  (let f ((x (read p))) "
                                                "   (if (eof-object? x) "
                                                "       '() "
                                                "       (if (eq? 'include (car x)) "
                                                "             (append (with-input-from-file "
                                                "                     (format #f \"~A/~A\" dunedir (symbol->string (cadr x))) "
                                                "                      dune:read) "
                                                "                    (f (read p))) "
                                                "             (cons x (f (read p))))))))",
                                                env);
        LOG_DEBUG(1, "reading completed", "");
        return stanzas;
    }
    /* log_debug("BYE"); */
    /* exit(EXIT_FAILURE); */

    /* /\* ****************************************************** *\/ */
    /* /\* OBSOLETE *\/ */
    /* s7_pointer readlet */
    /*     = s7_inlet(s7, */
    /*                s7_list(s7, 2, */
    /*                        s7_cons(s7, _inport_sym, inport), */
    /*                        s7_cons(s7, _dune_sexps,  s7_nil(s7)))); */

    /* const char *dune = "(catch #t -dune-read-thunk -dune-read-catcher)"; */
    /* // same as (with-let (inlet ...) (catch...)) ??? */
    /* /\* LOG_DEBUG(1, "evaluating c string %s", dune); *\/ */
    /* s7_pointer stanzas */
    /*     = s7_eval_c_string_with_environment(s7, dune, readlet); */
    /* /\* LOG_DEBUG(1, "XXXXXXXXXXXXXXXX"); *\/ */
    /* /\* TRACE_S7_DUMP(1, "ip readed stanazas", stanzas); *\/ */

    /* s7_close_input_port(s7, inport); */
    /* return stanzas; */
}

/* ********************************************************* */
static s7_pointer _dune_read_string(s7_scheme*s7, s7_pointer str)
{
    TRACE_ENTRY;
    TRACE_S7_DUMP(1, "string", str);

    s7_pointer _inport = s7_open_input_string(s7, s7_string(str));
    if (!s7_is_input_port(s7, _inport)) { // g_dune_inport)) {
        LOG_ERROR(0, "BAD INPUT PORT - _dune_read_string %s", str);
        s7_error(s7, s7_make_symbol(s7, "_dune_read_string bad input port"),
                     s7_nil(s7));
    }

    s7_pointer readlet
        = s7_inlet(s7,
                   s7_list(s7, 2,
                           s7_cons(s7, _inport_sym, _inport),
                           s7_cons(s7, _dune_sexps,  s7_nil(s7))));

    const char *dune = "(catch #t -dune-read-thunk -dune-read-catcher)";
    // same as (with-let (inlet ...) (catch...)) ???
    LOG_DEBUG(1, "evaluating c string %s", dune);
    s7_pointer stanzas
        = s7_eval_c_string_with_environment(s7, dune, readlet);

    TRACE_S7_DUMP(1, "readed stanazas", stanzas);

    return stanzas;
}

/*
  (dune:read) - read current-input-port
  (dune:read str) - read string str
  (dune:read p) - read port p
  calls internal _g_dune_read with local env
  global *dune:expand_includes*
 */
static s7_pointer g_dune_read(s7_scheme *s7, s7_pointer args)
{
    TRACE_ENTRY;
    /* s7_pointer p, arg; */
    TRACE_S7_DUMP(1, "args: %s", args);
#if defined(PROFILE_fastbuild)
    s7_pointer _curlet = s7_curlet(s7);
    char *tmp = s7_object_to_c_string(s7, _curlet);
    LOG_DEBUG(1, "CURLET: %s", tmp);
    free(tmp);
#endif

    s7_pointer result;

    /* #if defined(PROFILE_fastbuild) */
    /*     print_c_backtrace(); */
    /* #endif */

    if (args == s7_nil(s7)) {
        LOG_DEBUG(1, "SOURCE: current-input-port", "");
        result = _dune_read_current_input_port(s7);
        return result;
    } else {
        if (s7_list_length(s7, args) != 1) {
            s7_wrong_number_of_args_error(s7, "dune:read takes zero or 1 arg: ~S", args);
        }
        s7_pointer src = s7_car(args);
        if (s7_is_input_port(s7, src)) {
            LOG_DEBUG(1, "SOURCE: input port", "");
            result = _dune_read_input_port(s7, src);
            TRACE_S7_DUMP(1, "_dune_read_input_port res: %s", result);
            return result;
        }
        else if (s7_is_string(src)) {
            LOG_DEBUG(1, "SOURCE: string", "");
            result = _dune_read_string(s7, src);
            return result;
        }
        else {
            return(s7_wrong_type_error(s7, s7_make_string_wrapper_with_length(s7, "dune:read", 10), 1, src, string_string));
        }
    }


    // internal impl - must be exported to scheme (s7_define_function)
    // so it can be called with eval_c_string so that call_with_catch
    // will work.
    s7_define_function(s7, "-g-dune-read",
                       _g_dune_read,
                       0,
                       1, // string or port
                       false,
                       "internal dune:read");

    //TODO: to support recursion we need to use a local env rather
    //than global, to hold inport, stanzas, etc. so we alway use
    //eval_c_string_with_environment to call dune:read, and inlet
    // to create new local env.
    // but then what is the env for the catcher?

    /* s7_int gc_dune_read_s7 = s7_gc_protect(s7, _g_dune_read_s7); */
    /* s7_pointer result = _g_dune_read(s7, args); */
    /* s7_pointer result = s7_call(s7, _g_dune_read_s7, args); */
    s7_pointer env = s7_inlet(s7,
                              s7_list(s7, 1,
                                      s7_cons(s7,
                                              s7_make_symbol(s7, "xargs"),
                                              args)));
    // WARNING: if the call to -g-dune-read raises an error that gets
    // handled by the catcher, then the continuation is whatever
    // called this routine (i.e. the c stack), NOT the assignment to
    // result below.
    result = s7_eval_c_string_with_environment(s7, "(apply -g-dune-read xargs)", env);
    /* s7_gc_unprotect_at(s7, gc_dune_read_s7); */
    TRACE_S7_DUMP(1, "read result:", result);
    /* s7_gc_unprotect_at(s7, gc_stanzas); */
    return result;
}

s7_pointer pl_tx, pl_xx, pl_xxs,pl_sx, pl_sxi, pl_ix, pl_iis, pl_isix, pl_bxs;

//s7_pointer libdune_s7_init(s7_scheme *s7);
EXPORT s7_pointer libdune_s7_init(s7_scheme *s7)
{
    TRACE_ENTRY;
  s7_pointer cur_env;
  /* s7_pointer pl_tx, pl_xxs,pl_sx, pl_sxi, pl_ix, pl_iis, pl_isix, pl_bxs; */
  //  pl_xxsi, pl_ixs
  {
      s7_pointer t, x, b, s, i;

      t = s7_t(s7);
      x = s7_make_symbol(s7, "c-pointer?");
      b = s7_make_symbol(s7, "boolean?");
      s = s7_make_symbol(s7, "string?");
      i = s7_make_symbol(s7, "integer?");

      pl_tx = s7_make_signature(s7, 2, t, x);
      pl_xx = s7_make_signature(s7, 2, x, x);
      pl_xxs = s7_make_signature(s7, 3, x, x, s);
      /* pl_xxsi = s7_make_signature(s7, 4, x, x, s, i); */
      pl_sx = s7_make_signature(s7, 2, s, x);
      pl_sxi = s7_make_signature(s7, 3, s, x, i);
      pl_ix = s7_make_signature(s7, 2, i, x);
      pl_iis = s7_make_signature(s7, 3, i, i, s);
      pl_bxs = s7_make_signature(s7, 3, b, x, s);
      /* pl_ixs = s7_make_signature(s7, 3, i, x, s); */
      pl_isix = s7_make_signature(s7, 4, i, s, i, x);
  }

  string_string = s7_make_semipermanent_string(s7, "a string");
  c_pointer_string = s7_make_semipermanent_string(s7, "a c-pointer");
  character_string = s7_make_semipermanent_string(s7, "a character");
  boolean_string = s7_make_semipermanent_string(s7, "a boolean");
  real_string = s7_make_semipermanent_string(s7, "a real");
  complex_string = s7_make_semipermanent_string(s7, "a complex number");
  integer_string = s7_make_semipermanent_string(s7, "an integer");
  cur_env = s7_inlet(s7, s7_nil(s7));
  s7_pointer old_shadow = s7_set_shadow_rootlet(s7, cur_env);

  /* dune_table_init(s7, cur_env); */
  /* dune_array_init(s7, cur_env); */
  /* dune_datetime_init(s7, cur_env); */

  int64_t__symbol = s7_make_symbol(s7, "int64_t*");
  /* dune_datum_t__symbol = s7_make_symbol(s7, "dune_datum_t*"); */
  /* dune_array_t__symbol = s7_make_symbol(s7, "dune_array_t*"); */
  /* dune_table_t__symbol = s7_make_symbol(s7, "dune_table_t*"); */
  FILE__symbol = s7_make_symbol(s7, "FILE*");

  _inport_sym = s7_make_symbol(s7, "-dune-inport");
  _infile_sym = s7_make_symbol(s7, "-dune-infile");
  _dune_sexps  = s7_make_symbol(s7, "-dune-sexps");

  /* s7_define_constant(s7, "dune:version", s7_make_string(s7, "1.0-beta")); */

  /* s7_define(s7, cur_env, */
  /*           s7_make_symbol(s7, "dune:free"), */
  /*           s7_make_typed_function(s7, "dune:free", */
  /*                                  g_dune_free, */
  /*                                  1, 0, false, */
  /*                                  "(dune:free t) free table t", pl_tx)); */

  /* public api */
  s7_define_variable(s7, "*dune:expand-includes*", s7_t(s7));

  s7_define(s7, cur_env,
            s7_make_symbol(s7, "dune:read"),
            s7_make_typed_function(s7, "dune:read",
                                   g_dune_read,
                                   0, // 0 args: read from current inport
                                   // (for with-input-from-string or -file)
                                   1, // optional: string or port
                                   false,
                                   "(dune:read) read dunefile from current-input-port; (dune:read src) read dunefile from string or port",
                                   NULL)); //sig

  /* private */
  /* _dune_read_thunk_s7 = s7_make_function(s7, "-dune-read-thunk", */
  /*                                        _dune_read_thunk, */
  /*                                        0, 0, false, ""); */
  /* _dune_read_thunk_s7 = */
  s7_define_function(s7, "-dune-read-thunk",
                     _dune_read_thunk,
                     0, 0, false, "");

  /* _dune_read_catcher_s7 = */
  s7_define_function(s7, "-dune-read-catcher",
                   _dune_read_catcher,
                   2, // catcher must take 2
                   0, false,
                   "handle read error");

    /* gc_dune_read_catcher_s7 = s7_gc_protect(s7, _dune_read_catcher_s7); */

  /* _dune_read_catcher_s7 = s7_define_function(s7, "-dune-read-thunk-catcher", */
  /*                                                _dune_read_catcher, */
  /*                                                2, // catcher must take 2 */
  /*                                                0, false, ""); */

  s7_set_shadow_rootlet(s7, old_shadow);

  return(cur_env);
}
