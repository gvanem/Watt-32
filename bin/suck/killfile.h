#ifndef _SUCK_KILLFILE_H
#define _SUCK_KILLFILE_H

#ifdef HAVE_REGEX_H
#include <sys/types.h>
#include <regex.h>
#endif

#define SIZEOF_SKIPARRAY 256

typedef struct {
#ifdef HAVE_REGEX_H
	regex_t *ptrs;
#endif
        char    *header;
        char    *string;
        void    *next;
        BYTE     skiparray[SIZEOF_SKIPARRAY];
        int      case_sensitive;
      } my_regex, *pmy_regex;

/* this is a structure so can add other kill options later
 */
typedef struct {
        int   hilines;    /* nr of lines max article length */
        int   lowlines;   /* nr of lines min article length */
        int   maxgrps;    /* max nr of grps (to prevent spams) */
        DWORD bodybig;    /* max size of article body  */
        DWORD bodysmall;  /* minimum size of article body */
        char  quote;      /* character to use as quote (for case compare) */
        char  non_regex;  /* character to use as non_regex (don't do regex compare ) */
#ifdef HAVE_REGEX_H	
        int   use_extended;     /* do we use extended regex */
#endif
	pmy_regex header;       /* scan the entire header */
	pmy_regex body;		/* scan the body */
        pmy_regex list;         /* linked list of headers and what to match */
      } OneKill, *POneKill;

typedef struct {
	OneKill match;
        int     delkeep;
        char   *group;    /* dynamically allocated */
      } Group, *PGroup;

typedef struct {
	int Stdin;
	int Stdout;
	pid_t Pid;
      } Child;
 
typedef struct killstruct {
        FILE   *logfp;
        int     logyn;
        int     grp_override;
        int     tie_delete;
        int     totgrps;
        int     ignore_postfix;
        PGroup  grps;    /* dynamicly allocated array */
        int   (*killfunc)(PMaster, struct killstruct*, char*, int); /* function to call to check header */
        char   *pbody;
        DWORD   bodylen;
        int     use_extended_regex;
        Child   child;            /* these two are last since can't initialize */
	OneKill master;
      } KillStruct, *PKillStruct;

/* function prototypes for killfile.c
 */
int         get_one_article_kill (PMaster, int);
PKillStruct parse_killfile (int, int, int);
void        free_killfile (PKillStruct);
int         chk_msg_kill (PMaster, PKillStruct, char *, int);

/* function prototypes for killprg.c
 */
int  killprg_forkit    (PKillStruct, char *, int);
int  chk_msg_kill_fork (PMaster, PKillStruct, char *, int);
void killprg_closeit   (PKillStruct);

enum { KILL_LOG_NONE,     /* what level of logging do we use */
       KILL_LOG_SHORT,
       KILL_LOG_LONG
     }; 

#endif /* _SUCK_KILLFILE_H */
