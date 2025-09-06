#ifndef _SUCK_CONFIG_H
#define _SUCK_CONFIG_H

/* Config Options
 */
#define DEFAULT_NNRP_PORT 119 	/* port to talk to on remote machine */
#define LOCAL_PORT        119   /* port to talk to on local host */

/* If you don't EVER plan to use the KILLFILE stuff, comment this out for
 * a slight speed increase. If you want to, or plan to, use the KILLFILE
 * stuff, uncomment this
 */
#define KILLFILE 1 

/* This is the character used in killfiles to tell if we should do a case
 * sensitive or a case insensitive compare.  It can be overridden in a
 * killfile.
 */
#define KILLFILE_QUOTE '"'

/* This is the character that is used as a comment flag in col 0 in
 * killfiles
 */
#define KILLFILE_COMMENT_CHAR '#'

/* This is the character used in killfiles to tell if we should always
 * do a non-regex compare on the string.  It can be overridden in a killfile.
 */
#define KILLFILE_NONREGEX '%'

/* this is the start buffer size for the killfile get body routine
 * this should be close to the max size of the message you expect to get
 */
#define KILL_BODY_BUF_SIZE 80960

/* this is the amount of extra memory we grab everytime we need to increase
 * memory typically for huge articles.
 * WARNING, THIS MUST BE BIGGER THAN MAXLINLEN
 */
#define KILL_CHUNK_BUF_INCREASE 10240

/* if using -A (scan active file), this is the number I use for lastread
 * when I get a new group, 0 = all messages, -100 means get last 100 messages
 */
#define ACTIVE_DEFAULT_LASTREAD -100

/* if you want to timeout if no read after X seconds define this
 * as number of seconds before timeout
 * else, comment it out.  If you comment it out, if link goes down
 * we'll just sit, twiddling our bits, until whenever.
 */
#define TIMEOUT 90

/* signal which will interrupt us DON'T USE SIGKILL OR SIGSTOP
 * if you don't want to be able to abort, and a miniscule speed
 * increase, comment this out
 * WARNING: if you plan to use the killprg part of the killfile routines
 * then leave this in.  If you don't, and the child program dies unexpectedly
 * suck will also die.
 */
#ifndef MYSIGNAL
#define MYSIGNAL    SIGTERM
#endif

#ifndef PAUSESIGNAL
#define PAUSESIGNAL SIGUSR1
#endif

/* If you want to have suck check in your history file for articles
 * that you already have, uncomment this.
 */
#define CHECK_HISTORY 1

/* The location of the History file
 */
#define HISTORY_FILE "history.1"

/* If your system doesn't like the lock file stuff in suck.c or lmove.c
 * comment this out
 */
//#define LOCKFILE 1

/* FULL PATH of error log used if -e option specifed to any of the programs
 * can be overridden at the command line with -E option
 */
#define ERROR_LOG "suck.errlog"

/* FULL PATH of status messages log if -s option specified to any of the
 * programs can be overridden at the command line with -S option
 */
#define STATUS_LOG "suck.staus"     /* /dev/null = silent mode */

/* FIRST Character used in args to signify read arguments from a file
 */
#define FILE_CHAR '@'

/* Comment character in arg file, everything after this on a line is ignored
 */
#define FILE_ARG_COMMENT '#'

/*  FILE NAMES
 * Data files read in
 */
#define N_OLDRC         "newsrc"         /* what newsgroups to download                   */
#define N_KILLFILE      "killfile"       /* Parameter file for which articles s NOT to download */
#define N_SUPPLEMENTAL  "othermsgs"      /* list of article nrs to add to our list to download */
#define N_LMOVE_ACTIVE  "active"         /* list of active newsgroups for lmove to read */
#define N_ACTIVE_IGNORE "active.ignore"  /* list of newsgroups ignored by read local active option */
#define N_LMOVE_CONFIG  "lmove.cfg"      /* config file for lmove */
#define N_PHRASES       "suck.phrases"   /* default location for phrase file */

/* TEMP FILES created
 */
#define N_NEWRC          "suck.newrc"
#define N_SORTED         "suck.sorted"
#define N_RESTART        "suck.restart"
#define N_KILLLOG        "suck.killlog"
#define N_LOCKFILE       "suck.lock"
#define N_TMP_EXTENSION  ".tmp"          /* added to file in process of being downloaded */
#define N_LMOVE_LOCKFILE "lmove.lock"
#define N_POSTFILE       "suck.post"

/* Old copy of N_OLDRC (saved just in case)
 */
#define N_OLD_OLDRC      "sucknewsrc.old"

/* Various DIRECTORY PATHS, these can be overriden from command line
 */
#define N_TMPDIR   "."           /* location of Temp Files */
#define N_DATADIR  "."           /* location of Data Files */
#define N_MSGDIR   ".\\Msgs"     /*location of articles produced by suck, if multifile option selected */

/* Argument substition strings for rpost
 */
#define RPOST_FILTER_IN   "$$i"
#define RPOST_FILTER_OUT  "$$o"

/* Max nr of args to the rpost filter
 */
#define RPOST_MAXARGS 128

/* extension added by rpost to the batch file when
 * a message upload fails and we have to write out
 * a failed file
 */
#define RPOST_FAIL_EXT    ".bad"

/* RNEWS program called by lpost
 */
#define RNEWS  "/usr/lib/news/rnews"

/* character used as a comment in sucknewsrc
 */
#define SUCKNEWSRC_COMMENT_CHAR '#'

/* character used as a separate in the PATH environment variable
 */
#define PATH_SEPARATOR  ';'

/* maximum number of args to killprg
 */
#define KILLPRG_MAXARGS 128

/* length of string sent to killprg (min length must be 8)
 */
#define KILLPRG_LENGTHLEN 8 

/* mode for directory when creating in lmove
 */
#define LMOVE_DEFAULT_DIR_MODE 0755	/* in octal */

/* longest line possible in a message for lmove
 */
#define LMOVE_MAXLINLEN 8192

/* mode for active file when rewriting it
 */
#define LMOVE_ACTIVE_MODE 0755		/* in octal */

/* max number of args for suck to pass to lmove
 */
#define LMOVE_MAX_ARGS 12

/*  Internal buffer size shouldn't need to change
 */
#define MAXLINLEN 4096

/* Internal buffer size for print_phrases
 */
#define PHRASES_BLOCK_SIZE 1024

/* Character that is used to separate variables (front and back) in phrases
 */
#define PHRASES_SEPARATOR  '%'
#define PHRASES_VAR_CHAR   'v'

/* phrase that defines a group name in suck.sorted
 */
#define RESTART_GROUP "GROUP"

#ifndef PATH_MAX        /* in case you don't have it */
#ifdef _POSIX_PATH_MAX
#define PATH_MAX _POSIX_PATH_MAX
#endif
#endif

#ifdef __HIGHC__ /* HighC has neither */
#define PATH_MAX _MAX_PATH
#endif

#ifndef PATH_MAX
#ifdef MAXPATHLEN
#define PATH_MAX MAXPATHLEN
#endif
#endif

#endif /* _SUCK_CONFIG_H */
