#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <unistd.h>

#include "libs7.h"
#include "dunefile_reader.h"

bool multiline_string_mode = false;

#if defined(PROFILE_fastbuild)
#define DEBUG_LEVEL dune_s7_debug
extern int  DEBUG_LEVEL;
#define TRACE_FLAG dune_s7_trace
extern bool TRACE_FLAG;
#define S7_DEBUG_LEVEL libs7_debug
extern int     libs7_debug;
extern int     s7plugin_debug;
#endif

char *read_dunefile(const char *dunefile_name)
{
    TRACE_ENTRY;
    /* log_debug("df: %s", dunefile_name); */
    TRACE_LOG("dunefile: %s", dunefile_name);

    size_t file_size;
    char *inbuf = NULL;
    struct stat stbuf;
    int fd;
    FILE *instream = NULL;

    errno = 0;
    fd = open(dunefile_name, O_RDONLY);
    if (fd == -1) {
        /* Handle error */
        fprintf(stderr, "fd open error: %s\n", dunefile_name);
        LOG_TRACE(1, "cwd: %s", getcwd(NULL, 0));
        exit(EXIT_FAILURE);
    }

    /* log_debug("fopened %s", dunefile_name); */
    if ((fstat(fd, &stbuf) != 0) || (!S_ISREG(stbuf.st_mode))) {
        /* Handle error */
        fprintf(stderr, "fstat error: %s\n",
                dunefile_name);
        // exit?
        goto cleanup;
    }

    file_size = stbuf.st_size;
#if defined(PROFILE_fastbuild)
    LOG_DEBUG(1, "filesize: %d", file_size);
#endif

    /* allocate enough to handle expansion due to
       converting '.' to "./" */
    inbuf = (char*)calloc(file_size + 1024, sizeof(char));
    if (inbuf == NULL) {
        /* Handle error */
        LOG_ERROR(0, "malloc file_size fail", "");
        goto cleanup;
    }

    /* FIXME: what about e.g. unicode in string literals? */
    errno = 0;
    instream = fdopen(fd, "r");
    if (instream == NULL) {
        /* Handle error */
        LOG_ERROR(0, "fdopen failure: %s", dunefile_name);
        /* printf(RED "ERROR" CRESET "fdopen failure: %s\n", */
        /*        dunefile_name); */
               /* utstring_body(dunefile_name)); */
        perror(NULL);
        close(fd);
        goto cleanup;
    } else {
#if defined(PROFILE_fastbuild)
        LOG_DEBUG(1, "fdopened %s", dunefile_name);
        /* utstring_body(dunefile_name)); */
#endif
    }

    int c;
    int peeker;
    size_t i = 0;
    /* now read file one char at a time */
    /* baddot cases:  " .)", " . ", "(. " */
    errno = 0;
    multiline_string_mode = false;
    while ((c = fgetc(instream)) != EOF) {
        LOG_DEBUG(0, "inbuf: %s", (char*)inbuf);
        LOG_DEBUG(0, "C: '%c'", c);
        /* log_debug("char: %c", (char)c); */
        if (c == '.') {
            peeker = fgetc(instream);
            if (peeker == ')') { // most common: ".)"
                if (isspace(inbuf[i - 1])) {
                    LOG_DEBUG(0, "FOUND BADDOT1: %s", inbuf);
                    inbuf[i++] = '.';
                    inbuf[i++] = '/';
                    ungetc(peeker, instream); // rewind
                    continue;
                }
            }
            else if (peeker == ' ') {
                if (isspace(inbuf[i - 1])) { // " . "
                    LOG_DEBUG(0, "FOUND BADDOT2: %s", inbuf);
                    inbuf[i++] = '.';
                    inbuf[i++] = '/';
                    ungetc(peeker, instream); // rewind
                } else {
                    if (inbuf[i - 1] == '(') { // "(. "
                        LOG_DEBUG(0, "FOUND BADDOT3: %s",
                                  inbuf);
                        inbuf[i++] = '.';
                        inbuf[i++] = '/';
                        ungetc(peeker, instream); // rewind
                        continue;
                    } else {
                        /* "x.y " */
                        ungetc(peeker, instream); // rewind
                        inbuf[i++] = (char)c;
                        continue;
                    }
                }
            }
            else {
                // not  " .)" nor  " . " so dot is ok
                /* inbuf[i++] = 'X'; */
                /* inbuf[i++] = (char)c; */
                ungetc(peeker, instream); // rewind, c == '.'
                inbuf[i++] = (char)c;
                /* inbuf[i++] = 'Y'; */
            }
        } else if (c == '"') {
            /* multiline strings start with "\|
               following lines start with "\| or "\>
             */
            peeker = fgetc(instream);
            LOG_DEBUG(0, "peeker: '%c'", peeker);
            if (peeker == '\\') { // single '\', escaped in c
                int peeker2 = fgetc(instream);
                LOG_DEBUG(0, "peeker2: '%c'", peeker2);
                if (peeker2 == '|') {
                    LOG_DEBUG(0, "EOLSTR DELIM", "");
                    if (multiline_string_mode) {
                        // we're already in a block
                        continue;
                    } else {
                        LOG_DEBUG(0, "entering EOLSTR", "");
                        multiline_string_mode = true;
                        inbuf[i++] = '"';
                        // discard initial space following delim
                        peeker2 = fgetc(instream);
                        if (peeker2 != ' ') {
                            inbuf[i++] = peeker2;
                        }
                    }
                } else {
                    /* not staring a multiline string */
                    ungetc(peeker, instream);
                    inbuf[i++] = (char)c;
                }
            } else {
                /* not starting multiline string */
                ungetc(peeker, instream);
                inbuf[i++] = (char)c;
            }
        } else if (c == '\n') {
            LOG_DEBUG(0, "NEWLINE", "");
            if (multiline_string_mode) {
                LOG_DEBUG(0, "in EOLSTR mode", "");
                /* end-of-line in multiline mode */
                /* peek to find if next line starts
                   with "\| or "\>
                   NB: a space after a delim is ignored
                */
                // 1. consume whitespace
                int space_ct = 0;
                int eolpeeker = fgetc(instream);
                if (eolpeeker == '\n') {
                    LOG_DEBUG(0, "TERMINATING EOL", "");
                    multiline_string_mode = false;
                    inbuf[i++] = '"'; // eol terminator
                    continue;
                }
                while (isspace(eolpeeker)) {
                    LOG_DEBUG(0, "WS", "");
                    //FIXME: what about tabs?
                    space_ct++;
                    eolpeeker = fgetc(instream);
                }
                LOG_DEBUG(0, "NOT WS: '%c'", eolpeeker);
                // 2. is first non-ws char an eol delim?
                if (eolpeeker == '"') {
                    LOG_DEBUG(0, "MAYBE EOL", "");
                    eolpeeker = fgetc(instream);
                    if (eolpeeker == '\\') {
                        LOG_DEBUG(0, "MORE MAYBE EOL", "");
                        eolpeeker = fgetc(instream);
                        if (eolpeeker == '|') {
                            LOG_DEBUG(0, "EOL | DELIM", "");
                            // consume leading SP
                            eolpeeker = fgetc(instream);
                            if (eolpeeker != ' ') {
                                ungetc(eolpeeker, instream);
                            }
                            inbuf[i++] = '\\';
                            inbuf[i++] = 'n';
                        } else if (eolpeeker == '>') {
                            LOG_DEBUG(0, "EOL > DELIM", "");
                            inbuf[i++] = '\n';
                        } else {
                            LOG_DEBUG(0, "TERMINATING EOL", "");
                            inbuf[i++] = '"'; // eol terminator
                        }
                    } else {
                        LOG_DEBUG(0, "TERMINATING EOL", "");
                        inbuf[i++] = '"'; // eol terminator

                        // restore in reverse order
                        LOG_DEBUG(0, "RESTORING EOLP %c", eolpeeker);
                        ungetc(eolpeeker, instream);
                        ungetc('"', instream);
                        for (int j = 0; j < space_ct; j++) {
                            LOG_DEBUG(0, "RESTORING SP", "");
                            ungetc(' ', instream);
                        }
                        multiline_string_mode = false;
                    }
                } else {
                    LOG_DEBUG(0, "TERMINATING EOL", "");
                    // no: terminate preceding eol
                    inbuf[i++] = '"';
                    inbuf[i++] = eolpeeker;
                    multiline_string_mode = false;
                }
            } else {
                LOG_DEBUG(0, "newline NOT in EOLSTR", "");
                inbuf[i++] = (char)c;
            }
        } else {
            // c != '.'
            inbuf[i++] = (char)c;
        }
        errno = 0;
    } // end while

    /* If the stream is at end-of-file OR a read error occurs,
       fgetc returns EOF. */
    if (feof(instream)) {
        if (errno != 0) {
            fprintf(stderr, "fgetc error for %s: %s\n",
                      dunefile_name, strerror(errno));
        }
    } else {
        LOG_WARN(0, "bad feof? %s", dunefile_name);
        if (!ferror(instream)) {
            LOG_WARN(0, "ferror set: %s", dunefile_name);
        } else {
            LOG_WARN(0, "ferror not set: %s", dunefile_name);
        }
    }

    /* log_debug("INBUF:\n %s", (char*)inbuf); */

    return (char*)inbuf;

cleanup:
    //FIXME
    if (instream != NULL)
    {
        fclose(instream);
        close(fd);
    }
    if (inbuf != NULL) free(inbuf);
    /* if (outbuf != NULL) free(outbuf); */
    return NULL;
}

/* ################ OBSOLETE ################ */
const char *dunefile_to_string(s7_scheme *s7, const char *dunefile_name)
{
    TRACE_ENTRY;
#if defined(PROFILE_fastbuild)
    LOG_TRACE(1, "dunefile: %s", dunefile_name);
                  //utstring_body(dunefile_name));
    s7_pointer cip = s7_current_input_port(s7);
    TRACE_S7_DUMP(1, "cip: %s", cip);
#endif
    /* core/dune file size: 45572 */
    // 2K

    //FIXME: malloc
/* #define DUNE_BUFSZ 131072 */
/*     /\* static char inbuf[DUNE_BUFSZ]; *\/ */
/*     /\* memset(inbuf, '\0', DUNE_BUFSZ); *\/ */
/*     static char outbuf[DUNE_BUFSZ + 20]; */
/*     memset(outbuf, '\0', DUNE_BUFSZ); */

    size_t file_size;
    char *inbuf = NULL;
    struct stat stbuf;
    int fd;
    FILE *instream = NULL;

    errno = 0;
    fd = open(dunefile_name, O_RDONLY);
    if (fd == -1) {
        /* Handle error */
        fprintf(stderr, "fd open error: %s\n", dunefile_name);
        LOG_TRACE(1, "cwd: %s", getcwd(NULL, 0));
        s7_error(s7, s7_make_symbol(s7, "fd-open-error"),
                 s7_list(s7, 3,
                         s7_make_string(s7, "fd open error: ~A, ~A"),
                         s7_make_string(s7, dunefile_name),
                         s7_make_string(s7, strerror(errno))));
    }

    if ((fstat(fd, &stbuf) != 0) || (!S_ISREG(stbuf.st_mode))) {
        /* Handle error */
        fprintf(stderr, "fstat error on %s\n", dunefile_name);
        goto cleanup;
    }

    file_size = stbuf.st_size;
#if defined(PROFILE_fastbuild)
    LOG_DEBUG(1, "filesize: %d", file_size);
#endif

    inbuf = (char*)calloc(file_size, sizeof(char));
    if (inbuf == NULL) {
        /* Handle error */
        fprintf(stderr, "malloc file_size fail\n");
        goto cleanup;
    }

    /* FIXME: what about e.g. unicode in string literals? */
    errno = 0;
    instream = fdopen(fd, "r");
    if (instream == NULL) {
        /* Handle error */
        fprintf(stderr, "fdopen failure: %s", dunefile_name);
        /* printf(RED "ERROR" CRESET "fdopen failure: %s\n", */
        /*        dunefile_name); */
               /* utstring_body(dunefile_name)); */
        perror(NULL);
        close(fd);
        goto cleanup;
    } else {
#if defined(PROFILE_fastbuild)
        LOG_DEBUG(1, "fdopened %s", dunefile_name);
        /* utstring_body(dunefile_name)); */
#endif
    }

    // now read the entire file
    size_t read_ct = fread(inbuf, 1, file_size, instream);
#if defined(PROFILE_fastbuild)
    LOG_DEBUG(1, "read_ct: %d", read_ct);
    LOG_DEBUG(1, "readed txt: %s", (char*)inbuf);
#endif
    if (read_ct != file_size) {
        if (ferror(instream) != 0) {
            /* printf(RED "ERROR" CRESET "fread error 2 for %s\n", */
            /*        dunefile_name); */
            /* utstring_body(dunefile_name)); */
            fprintf(stderr, "fread error 2 for %s\n",
                      dunefile_name);
            /* utstring_body(dunefile_name)); */
            exit(EXIT_FAILURE); //FIXME: exit gracefully
        } else {
            if (feof(instream) == 0) {
                /* printf(RED "ERROR" CRESET "fread error 3 for %s\n", */
                /*        dunefile_name); */
                /* utstring_body(dunefile_name)); */
                fprintf(stderr, "fread error 3 for %s\n",
                          dunefile_name);
                /* utstring_body(dunefile_name)); */
                exit(EXIT_FAILURE); //FIXME: exit gracefully
            } else {
                //FIXME
                fprintf(stderr, "WTF????????????????\n");
                goto cleanup;
            }
        }
    } else {
        close(fd);
        fclose(instream);
    }

    inbuf[read_ct + 1] = '\0';
    // allocate twice (?) what we need
    uint64_t outFileSizeCounter = file_size * 3;
    errno = 0;
    fflush(NULL);
    static char outbuf[320000];
    /* char *outbuf = NULL; */
    /* outbuf = (char*)malloc(outFileSizeCounter); */
    /* outbuf = (char*)calloc(outFileSizeCounter, sizeof(char)); */
    /* fprintf(stderr, "XXXXXXXXXXXXXXXX"); */
    /* fflush(NULL); */
    /* if (outbuf == NULL) { */
    /*     LOG_ERROR(0, "calloc fail: %s", strerror(errno)); */
    /*     goto cleanup; */
    /* } else { */
    /*     LOG_INFO(0, "calloc success"); */
    /* } */
    memset((char*)outbuf, '\0', outFileSizeCounter);

    // FIXME: loop over the entire inbuf char by char, testing for
    // . or "\|
    char *inptr = (char*)inbuf;
    /* LOG_DEBUG(1, "INPTR str: %s", inptr); */
    char *outptr = (char*)outbuf;
    /* char *cursor = inptr; */

    bool eol_string = false;
    while (*inptr) {
        if (*inptr == '.') {
            if ((*(inptr+1) == ')')
                && isspace(*(inptr-1))){
                /* LOG_DEBUG(1, "FOUND DOT: %s", inptr); */
                *outptr++ = *inptr++;
                *outptr++ = '/';
                continue;
            }
        }
        if (*inptr == '"') {
            if (*(inptr+1) == '\\') {
                if (*(inptr+2) == '|') {
                    /* LOG_DEBUG(1, "FOUND EOL Q"); */
                    *outptr++ = *inptr++; // copy '"'
                    inptr += 2; // point to char after '"\|'
                    eol_string = true;
                    while (eol_string) {
                        if (*inptr == '\0') {
                            *outptr = *inptr;
                            eol_string = false;
                        }
                        if (*inptr == '\n') {
                            /* LOG_DEBUG(1, "hit eolstring newline"); */
                            // check to see if next line starts with "\|
                            char *tmp = inptr + 1;
                            while (isspace(*tmp)) {tmp++;}
                            /* LOG_DEBUG(1, "skipped to: %s", tmp); */
                            if (*(tmp) == '"') {
                                if (*(tmp+1) == '\\') {
                                    if (*(tmp+2) == '|') {
                                        // preserve \n
                                        *outptr++ = *inptr;
                                        /* *outptr++ = '\\'; */
                                        /* *outptr++ = 'n'; */
                                        inptr = tmp + 3;
                                        /* LOG_DEBUG(1, "resuming at %s", inptr); */
                                        continue;
                                    }
                                }
                            }
                            *outptr++ = '"';
                            inptr++; // omit \n
                            eol_string = false;
                        } else {
                            *outptr++ = *inptr++;
                        }
                    }
                }
            }
        }
        *outptr++ = *inptr++;
    }

/*     inptr = (char*)inbuf; */
/*     while (true) { */
/*         cursor = strstr(inptr, ".)"); */

/* /\* https://stackoverflow.com/questions/54592366/replacing-one-character-in-a-string-with-multiple-characters-in-c *\/ */

/*         if (cursor == NULL) { */
/* /\* #if defined(PROFILE_fastbuild) *\/ */
/* /\*             if (mibl_debug) LOG_DEBUG(1, "remainder: '%s'", inptr); *\/ */
/* /\* #endif *\/ */
/*             size_t ct = strlcpy(outptr, (const char*)inptr, file_size); // strlen(outptr)); */
/*             (void)ct;           /\* prevent -Wunused-variable *\/ */
/* /\* #if defined(PROFILE_fastbuild) *\/ */
/* /\*             if (mibl_debug) LOG_DEBUG(1, "concatenated: '%s'", outptr); *\/ */
/* /\* #endif *\/ */
/*             break; */
/*         } else { */
/* #if defined(PROFILE_fastbuild) */
/*             LOG_ERROR(0, "FOUND and fixing \".)\" at pos: %d", cursor - inbuf); */
/* #endif */
/*             size_t ct = strlcpy(outptr, (const char*)inptr, cursor - inptr); */
/* #if defined(PROFILE_fastbuild) */
/*             LOG_DEBUG(1, "copied %d chars", ct); */
/*             /\* LOG_DEBUG(1, "to buf: '%s'", outptr); *\/ */
/* #endif */
/*             /\* if (ct >= DUNE_BUFSZ) { *\/ */
/*             if (ct >= outFileSizeCounter) { */
/*                 printf("output string has been truncated!\n"); */
/*             } */
/*             outptr = outptr + (cursor - inptr) - 1; */
/*             outptr[cursor - inptr] = '\0'; */
/*             //FIXME: use memcpy */
/*             ct = strlcat(outptr, " ./", outFileSizeCounter); // DUNE_BUFSZ); */
/*             outptr += 3; */

/*             inptr = inptr + (cursor - inptr) + 1; */
/*             /\* printf(GRN "inptr:\n" CRESET " %s\n", inptr); *\/ */

/*             if (ct >= outFileSizeCounter) { // DUNE_BUFSZ) { */
/*                 LOG_ERROR(0, "write count exceeded output bufsz\n"); */
/*                 /\* printf(RED "ERROR" CRESET "write count exceeded output bufsz\n"); *\/ */
/*                 free(inbuf); */
/*                 exit(EXIT_FAILURE); */
/*                 // output string has been truncated */
/*             } */
/*         } */
/*     } */
    /* free(inbuf); */

    /* char *tmp = strndup((char*) outbuf, strlen((char*)outbuf)); */
    /* LOG_DEBUG(1, "x AAAAAAAAAAAAAAAA9"); */
    /* free(outbuf); */
    return (char*)outbuf;

cleanup:
    //FIXME
    if (instream != NULL)
    {
        fclose(instream);
        close(fd);
    }
    if (inbuf != NULL) free(inbuf);
    /* if (outbuf != NULL) free(outbuf); */
    return NULL;
}


char *xread_dunefile(const char *dunefile_name)
{
    TRACE_ENTRY;
    /* log_debug("df: %s", dunefile_name); */
    TRACE_LOG("dunefile: %s", dunefile_name);

    size_t file_size;
    char *inbuf = NULL;
    struct stat stbuf;
    int fd;
    FILE *instream = NULL;

    errno = 0;
    fd = open(dunefile_name, O_RDONLY);
    if (fd == -1) {
        /* Handle error */
        fprintf(stderr, "fd open error: %s\n", dunefile_name);
        LOG_TRACE(1, "cwd: %s", getcwd(NULL, 0));
        /* s7_error(s7, s7_make_symbol(s7, "fd-open-error"), */
        /*          s7_list(s7, 3, */
        /*                  s7_make_string(s7, "fd open error: ~A, ~A"), */
        /*                  s7_make_string(s7, dunefile_name), */
        /*                  s7_make_string(s7, strerror(errno)))); */
    }

    if ((fstat(fd, &stbuf) != 0) || (!S_ISREG(stbuf.st_mode))) {
        /* Handle error */
        fprintf(stderr, "fstat error on %s\n", dunefile_name);
        goto cleanup;
    }

    file_size = stbuf.st_size;
#if defined(PROFILE_fastbuild)
    LOG_DEBUG(1, "filesize: %d", file_size);
#endif

    inbuf = (char*)calloc(file_size, sizeof(char));
    if (inbuf == NULL) {
        /* Handle error */
        fprintf(stderr, "malloc file_size fail\n");
        goto cleanup;
    }

    /* FIXME: what about e.g. unicode in string literals? */
    errno = 0;
    instream = fdopen(fd, "r");
    if (instream == NULL) {
        /* Handle error */
        fprintf(stderr, "fdopen failure: %s\n", dunefile_name);
        /* printf(RED "ERROR" CRESET "fdopen failure: %s\n", */
        /*        dunefile_name); */
               /* utstring_body(dunefile_name)); */
        perror(NULL);
        close(fd);
        goto cleanup;
    } else {
#if defined(PROFILE_fastbuild)
        LOG_DEBUG(1, "fdopened %s", dunefile_name);
        /* utstring_body(dunefile_name)); */
#endif
    }

    // now read the entire file
    size_t read_ct = fread(inbuf, 1, file_size, instream);
#if defined(PROFILE_fastbuild)
    LOG_DEBUG(1, "read_ct: %d", read_ct);
    LOG_DEBUG(1, "readed txt: %s", (char*)inbuf);
#endif
    if (read_ct != file_size) {
        if (ferror(instream) != 0) {
            /* printf(RED "ERROR" CRESET "fread error 2 for %s\n", */
            /*        dunefile_name); */
            /* utstring_body(dunefile_name)); */
            fprintf(stderr, "fread error 2 for %s\n",
                    dunefile_name);
            /* utstring_body(dunefile_name)); */
            exit(EXIT_FAILURE); //FIXME: exit gracefully
        } else {
            if (feof(instream) == 0) {
                /* printf(RED "ERROR" CRESET "fread error 3 for %s\n", */
                /*        dunefile_name); */
                /* utstring_body(dunefile_name)); */
                fprintf(stderr, "fread error 3 for %s\n",
                        dunefile_name);
                /* utstring_body(dunefile_name)); */
                exit(EXIT_FAILURE); //FIXME: exit gracefully
            } else {
                //FIXME
                fprintf(stderr, "HUH????????????????\n");
                goto cleanup;
            }
        }
    } else {
        close(fd);
        fclose(instream);
    }

    inbuf[read_ct + 1] = '\0';
    // allocate twice (?) what we need
    uint64_t outFileSizeCounter = file_size * 3;
    errno = 0;
    fflush(NULL);
    static char outbuf[320000];
    /* char *outbuf = NULL; */
    /* outbuf = (char*)malloc(outFileSizeCounter); */
    /* outbuf = (char*)calloc(outFileSizeCounter, sizeof(char)); */
    /* fprintf(stderr, "XXXXXXXXXXXXXXXX"); */
    /* fflush(NULL); */
    /* if (outbuf == NULL) { */
    /*     LOG_ERROR(0, "calloc fail: %s", strerror(errno)); */
    /*     goto cleanup; */
    /* } else { */
    /*     LOG_INFO(0, "calloc success"); */
    /* } */
    memset((char*)outbuf, '\0', outFileSizeCounter);

    // FIXME: loop over the entire inbuf char by char, testing for
    // . or "\|
    char *inptr = (char*)inbuf;
    /* LOG_DEBUG(1, "INPTR str: %s", inptr); */
    char *outptr = (char*)outbuf;
    /* char *cursor = inptr; */

    bool eol_string = false;
    while (*inptr) {
        if (*inptr == '.') {
            if ((*(inptr+1) == ')')
                && isspace(*(inptr-1))){
                /* LOG_DEBUG(1, "FOUND DOT: %s", inptr); */
                *outptr++ = *inptr++;
                *outptr++ = '/';
                continue;
            }
        }
        if (*inptr == '"') {
            if (*(inptr+1) == '\\') {
                if (*(inptr+2) == '|') {
                    /* LOG_DEBUG(1, "FOUND EOL Q"); */
                    *outptr++ = *inptr++; // copy '"'
                    inptr += 2; // point to char after '"\|'
                    eol_string = true;
                    while (eol_string) {
                        if (*inptr == '\0') {
                            *outptr = *inptr;
                            eol_string = false;
                        }
                        if (*inptr == '\n') {
                            /* LOG_DEBUG(1, "hit eolstring newline"); */
                            // check to see if next line starts with "\|
                            char *tmp = inptr + 1;
                            while (isspace(*tmp)) {tmp++;}
                            /* LOG_DEBUG(1, "skipped to: %s", tmp); */
                            if (*(tmp) == '"') {
                                if (*(tmp+1) == '\\') {
                                    if (*(tmp+2) == '|') {
                                        // preserve \n
                                        *outptr++ = *inptr;
                                        /* *outptr++ = '\\'; */
                                        /* *outptr++ = 'n'; */
                                        inptr = tmp + 3;
                                        /* LOG_DEBUG(1, "resuming at %s", inptr); */
                                        continue;
                                    }
                                }
                            }
                            *outptr++ = '"';
                            inptr++; // omit \n
                            eol_string = false;
                        } else {
                            *outptr++ = *inptr++;
                        }
                    }
                }
            }
        }
        *outptr++ = *inptr++;
    }

    return (char*)outbuf;

cleanup:
    //FIXME
    if (instream != NULL)
    {
        fclose(instream);
        close(fd);
    }
    if (inbuf != NULL) free(inbuf);
    /* if (outbuf != NULL) free(outbuf); */
    return NULL;
}

