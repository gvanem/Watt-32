#ifndef _SUCK_BOTH_H
#define _SUCK_BOTH_H

#include <netdb.h>   /* for struct hostent */
#include "config.h"  /* for debug etc stuff */

int    sgetline (int fd, char **sbuf);
int    sputline (int fd, const char *outbuf);
int    connect_to_nntphost (const char *host, struct hostent **, FILE *, unsigned short int);
char  *number (char *sp, int *intPtr);
char  *get_long (char *, long *);

void   signal_block (int);
void   error_log (int mode, const char *fmt, ...);
void   MyPerror (const char *);
void   MyNetPerror (const char *);
void   free_args (int, char *[]);
char **build_args (const char *, int *);
char **read_array (FILE *, int, int);
void   free_array (int, char **);
char  *do_a_phrase (char []);
void   print_phrases (FILE *, const char *, ...);
char  *str_int (int);
char  *str_long (long);
void   do_debug (const char *, ...);
void   do_debug_binary (int, const char *);

struct hostent *get_hostent (const char *host);

#define N_DEBUG "debug.suck"

enum { MYSIGNAL_SETUP,
       MYSIGNAL_BLOCK,
       MYSIGNAL_UNBLOCK,
       MYSIGNAL_ADDPIPE
     };

enum { ERRLOG_SET_FILE,
       ERRLOG_SET_STDERR,
       ERRLOG_REPORT
     };

#define SOCKET_PROTOCOL 0	/* so testhost.c can get at it */

#ifndef FALSE
  #define FALSE 0
  #define TRUE  (!FALSE)
#endif

#ifdef TIMEOUT
extern int TimeOut;		/* used to pass TimeOut value to sgetline() */
#endif

#endif /* _SUCK_BOTH_H */
