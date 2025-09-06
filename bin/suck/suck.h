#ifndef _SUCK_SUCK_H
#define _SUCK_SUCK_H

#include <stdio.h>
#include <tcp.h>
#include <sys/types.h>

#if defined (__DJGPP__) || defined(__HIGHC__) || defined(__WATCOMC__)
#include <unistd.h>
#endif

#ifdef __WATCOMC__
#include <direct.h>
#include <process.h>
#define pid_t int
#endif

/* link list structure one for each article
 */
typedef struct LinkList {
	struct LinkList *next;
        char  *msgnr;
        char   mandatory;
        int    sentcmd;
        int    groupnr;
        long   nr;
      } List, *PList;

/* link list for group names
 */
typedef struct GroupList {
        char  *group;
        int    nr;
	struct GroupList *next;
      } Groups, *PGroups;

/* Master Structure
 */
typedef struct {
        PList   head;
        PList   curr;
        PGroups groups;
        int     nritems;
        int     itemon;
        int     nrgot;
        int     sockfd;
        int     MultiFile;
        int     status_file;
        int     do_killfile;
        int     do_chkhistory;
        int     do_modereader;
        int     always_batch;
        int     cleanup;
        int     batch;
        int     pause_time;
        int     pause_nrmsgs;
        int     sig_pause_time;
        int     sig_pause_nrmsgs;
        int     killfile_log;
        int     debug;
        int     rescan;
        int     quiet;
        int     kill_ignore_postfix;
        int     reconnect_nr;
        int     do_active;
        int     nrmode;
        int     auto_auth;
        int     no_dedupe;
        int     chk_msgid;
        int     prebatch;
        int     skip_on_restart;
        WORD    portnr;
        long    rnews_size;
        int     grpnr;
        void   *killp;
        FILE   *msgs;
        FILE   *innfeed;

	const char *userid;
	const char *passwd;
	const char *host;
	const char *batchfile;
	const char *status_file_name;
	const char *phrases;
	const char *errlog;
	const char *localhost;
	const char *activefile;
	const char *kill_log_name;
      } Master, *PMaster;

int   get_a_chunk (int, FILE *);
void  free_one_node (PList);
int   send_command (PMaster, const char *, char **, int);
int   get_message_index (PMaster);
int   do_one_group (PMaster, char *, char *, FILE *, long, int);
int   do_supplemental (PMaster);
const char *build_command (PMaster, const char *, PList);

enum { RETVAL_ERROR = -1,
       RETVAL_OK = 0,
       RETVAL_NOARTICLES,
       RETVAL_UNEXPECTEDANS,
       RETVAL_VERNR,
       RETVAL_NOAUTH
     };

enum { BATCH_FALSE,         /* poss values for batch variable */
       BATCH_INNXMIT,
       BATCH_RNEWS,
       BATCH_LMOVE,
       BATCH_INNFEED,
       BATCH_LIHAVE
     };  

enum { MANDATORY_YES      = 'M' , /* do we have to download this article */
       MANDATORY_OPTIONAL = 'O'
     }; 

#endif /* _SUCK_SUCK_H */         
