#ifndef _SUCK_SUCKUTILS_H
#define _SUCK_SUCKUTILS_H
 
int   checkdir        (const char *);
const char *full_path (int, int, const char *);
int   cmp_msgid       (const char *, const char *);
int   move_file       (const char *, const char *);

#ifdef LOCKFILE
int   do_lockfile (PMaster);
#endif

enum { FP_SET,        /* get or set call for full_path() */
       FP_GET,
       FP_SET_POSTFIX,
       FP_GET_NOPOSTFIX,
       FP_GET_POSTFIX
     }; 

enum { FP_TMPDIR,     /* which dir in full_path() */
       FP_DATADIR,
       FP_MSGDIR,
       FP_NONE
     }; 

#define FP_NR_DIRS 3  /* this must match nr of enums */

#endif /* _SUCK_SUCKUTILS_H */
