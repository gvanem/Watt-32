
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include "suck.h"
#include "config.h"
#include "both.h"
#include "utils.h"
#include "dedupe.h"
#include "phrases.h"
#include "killfile.h"
#include "timer.h"
#include "active.h"
#include "batch.h"

#ifdef CHECK_HISTORY
#include "chkhist.h"
#endif

static int  get_articles    (PMaster);
static int  get_one_article (PMaster, int);
static int  do_connect      (PMaster, int);
static int  allocnode       (PMaster, char *, int, char *);
static int  restart_yn      (PMaster);
static int  scan_args       (PMaster, int, char *[]);
static void do_cleanup      (void);
static int  do_authenticate (PMaster);
static void load_phrases    (PMaster);
static void free_phrases    (void);
static int  parse_args      (PMaster, int, char *[]);
static int  get_group_number(PMaster, char *);

#ifdef MYSIGNAL
  void sighandler (int);
  void pause_signal (int, PMaster);

  enum {
    PAUSE_SETUP, PAUSE_DO
  };
  enum {
    CONNECT_FIRST, CONNECT_AGAIN
  };

  static int GotSignal = FALSE;
#endif

char **both_phrases   = default_both_phrases;
char **suck_phrases   = default_suck_phrases;
char **timer_phrases  = default_timer_phrases;
char **chkh_phrases   = default_chkh_phrases;
char **dedupe_phrases = default_dedupe_phrases;
char **killf_reasons  = default_killf_reasons;
char **killf_phrases  = default_killf_phrases;
char **killp_phrases  = default_killp_phrases;
char **sucku_phrases  = default_sucku_phrases;
char **active_phrases = default_active_phrases;
char **batch_phrases  = default_batch_phrases;

enum {
  STATUS_STDOUT, STATUS_STDERR
};

enum {
  RESTART_YES, RESTART_NO, RESTART_ERROR
};

enum {
  ARG_NO_MATCH, ARG_ALWAYS_BATCH, ARG_BATCH_INN, ARG_BATCH_RNEWS,
  ARG_BATCH_LMOVE, ARG_BATCH_INNFEED, ARG_BATCH_POST, ARG_CLEANUP,
  ARG_DIR_TEMP, ARG_DIR_DATA, ARG_DIR_MSGS, ARG_DEF_ERRLOG, ARG_HOST,
  ARG_NO_POSTFIX, ARG_LANGUAGE, ARG_MULTIFILE, ARG_POSTFIX, ARG_QUIET,
  ARG_RNEWSSIZE, ARG_DEF_STATLOG, ARG_WAIT_SIG, ARG_ACTIVE, ARG_RECONNECT,
  ARG_DEBUG, ARG_ERRLOG, ARG_HISTORY, ARG_KILLFILE, ARG_KLOG_NONE,
  ARG_KLOG_SHORT, ARG_KLOG_LONG, ARG_MODEREADER, ARG_PORTNR, ARG_PASSWD,
  ARG_RESCAN, ARG_STATLOG, ARG_USERID, ARG_VERSION, ARG_WAIT, ARG_LOCALHOST,
  ARG_TIMEOUT, ARG_NRMODE, ARG_AUTOAUTH, ARG_NODEDUPE, ARG_NO_CHK_MSGID,
  ARG_READACTIVE, ARG_PREBATCH, ARG_SKIP_ON_RESTART, ARG_KLOG_NAME
};

typedef struct Arglist {
        const char *sarg;
        const char *larg;
        int   nr_params;
        int   flag;
        int   errmsg;    /* this is an index into suck_phrases */
      } Args, *Pargs;

const Args arglist[] = {
  { "a",  "always_batch",      0, ARG_ALWAYS_BATCH, -1},
  { "bi", "batch_inn",         1, ARG_BATCH_INN, 40},
  { "br", "batch_rnews",       1, ARG_BATCH_RNEWS, 40},
  { "bl", "batch_lmove",       1, ARG_BATCH_LMOVE, 40},
  { "bf", "batch_innfeed",     1, ARG_BATCH_INNFEED, 40},
  { "bp", "batch_post",        0, ARG_BATCH_POST, -1},
  { "c",  "cleanup",           0, ARG_CLEANUP, -1},
  { "dt", "dir_temp",          1, ARG_DIR_TEMP, 37},
  { "dd", "dir_data",          1, ARG_DIR_DATA, 37},
  { "dm", "dir_msgs",          1, ARG_DIR_MSGS, 37},
  { "e",  "def_error_log",     0, ARG_DEF_ERRLOG, -1},
  { "h",  "host",              1, ARG_HOST, 51},
  { "hl", "localhost",         1, ARG_LOCALHOST, 50},
  { "k",  "kill_no_postfix",   0, ARG_NO_POSTFIX, -1},
  { "l",  "language_file",     1, ARG_LANGUAGE, 47},
  { "m",  "multifile",         0, ARG_MULTIFILE, -1},
  { "n",  "number_mode",       0, ARG_NRMODE, -1},
  { "p",  "postfix",           1, ARG_POSTFIX, 36},
  { "q",  "quiet",             0, ARG_QUIET, -1},
  { "r",  "rnews_size",        1, ARG_RNEWSSIZE, 35},
  { "s",  "def_status_log",    0, ARG_DEF_STATLOG, -1},
  { "u",  "auto_authorization",0, ARG_AUTOAUTH, -1},
  { "w",  "wait_signal",       2, ARG_WAIT_SIG, 46},
  { "x",  "no_chk_msgid",      0, ARG_NO_CHK_MSGID, -1},
  { "z",  "no_dedupe",         0, ARG_NODEDUPE, -1},
  { "A",  "active",            0, ARG_ACTIVE, -1},
  { "AL", "read_active",       1, ARG_READACTIVE, 56},
  { "B",  "pre-batch",         0, ARG_PREBATCH, -1},
  { "C",  "reconnect",         1, ARG_RECONNECT, 49},
  { "D",  "debug",             0, ARG_DEBUG, -1},
  { "E",  "error_log",         1, ARG_ERRLOG, 41},
  { "H",  "no_history",        0, ARG_HISTORY, -1},
  { "K",  "killfile",          0, ARG_KILLFILE, -1},
  { "L",  "kill_log_none",     0, ARG_KLOG_NONE, -1},
  { "LF", "kill_log_name",     1, ARG_KLOG_NAME, 61},
  { "LS", "kill_log_short",    0, ARG_KLOG_SHORT, -1},
  { "LL", "kill_log_long",     0, ARG_KLOG_LONG, -1},
  { "M",  "mode_reader",       0, ARG_MODEREADER, -1},
  { "N",  "portnr",            1, ARG_PORTNR, 45},
  { "O",  "skip_on_restart",   0, ARG_SKIP_ON_RESTART, -1},
  { "P",  "password",          1, ARG_PASSWD, 44},
  { "R",  "no_rescan",         0, ARG_RESCAN, -1},
  { "S",  "status_log",        1, ARG_STATLOG, 42},
#ifdef TIMEOUT
  { "T",  "timeout",           1, ARG_TIMEOUT, 52},
#endif
  { "U",  "userid",            1, ARG_USERID, 43},
  { "V",  "version",           0, ARG_VERSION, -1},
  { "W",  "wait",              2, ARG_WAIT, 46},
};

#define MAX_ARG_PARAMS 2	/* max nr of params with any arg */
#define NR_ARGS        (sizeof(arglist)/sizeof(arglist[0]))

#define TRUE_STR(x) (((x) == TRUE) ? "TRUE" : "FALSE")
#define NULL_STR(x) (((x) == NULL) ? "NULL" : (x))

int main (int argc, char **argv)
{
  struct  stat sbuf;
  Master  master;
  PList   temp;
  PGroups ptemp;
  char   *inbuf, **args, **fargs = NULL;
  int     nr, resp, loop, fargc, retval = RETVAL_OK;

#ifdef LOCKFILE
  const char *lockfile = NULL;
#endif

  /* initialize master structure
   */
  master.head             = master.curr = NULL;
  master.nritems          = master.itemon = master.nrgot = 0;
  master.MultiFile        = FALSE;
  master.msgs             = stdout;
  master.sockfd           = -1;
  master.status_file      = FALSE;     /* are status messages going to a file */
  master.status_file_name = NULL;
  master.do_killfile      = TRUE;
  master.do_chkhistory    = TRUE;
  master.do_modereader    = FALSE;
  master.always_batch     = FALSE;
  master.rnews_size       = 0L;
  master.batch            = BATCH_FALSE;
  master.batchfile        = NULL;
  master.cleanup          = FALSE;
  master.portnr           = DEFAULT_NNRP_PORT;
  master.host             = getenv ("NNTPSERVER");  /* the default */
  master.pause_time       = -1;
  master.pause_nrmsgs     = -1;
  master.sig_pause_time   = -1;
  master.sig_pause_nrmsgs = -1;
  master.killfile_log     = KILL_LOG_LONG;  /* do we log killed messages */
  master.phrases          = NULL;
  master.errlog           = NULL;
  master.debug            = FALSE;
  master.rescan           = TRUE;      /* do we do rescan on a restart */
  master.quiet            = FALSE;     /* do we display BPS and msg count */
  master.killp            = NULL;      /* pointer to killfile structure */
  master.kill_ignore_postfix = FALSE;
  master.reconnect_nr     = 0;         /* how many x msgs do we disconnect and reconnect 0 = never */
  master.innfeed          = NULL;
  master.do_active        = FALSE;     /* do we scan the local active list  */
  master.localhost        = NULL;
  master.groups           = NULL;
  master.nrmode           = FALSE;     /* use nrs or msgids to get article */
  master.auto_auth        = FALSE;     /* do we do auto authorization */
  master.passwd           = NULL;
  master.userid           = NULL;
  master.no_dedupe        = FALSE;
  master.chk_msgid        = TRUE;      /* do we check MsgID for trailing > */
  master.activefile       = NULL;
  master.prebatch         = FALSE;     /* do we try to batch any left over articles b4 we start? */
  master.grpnr            = -1;        /* what group number are we currently on */
  master.skip_on_restart  = FALSE;
  master.kill_log_name    = N_KILLLOG;

  /* have to do this next so if set on cmd line, overrides this */

#ifdef N_PHRASES		/* in case someone nukes def */
  if (stat (N_PHRASES, &sbuf) == 0 && S_ISREG (sbuf.st_mode))
  {
    /* we have a regular phrases file make it the default */
    master.phrases = N_PHRASES;
  }
#endif

  /* allow no args, only the hostname, or hostname as first arg
   * also have to do the file argument checking
   */
  switch (argc)
  {
    case 1:
	 break;
    case 2:
	 /* the fargs == NULL test so only process first file name */
	 if (argv[1][0] == FILE_CHAR)
	 {
           fargs = build_args (&argv[1][1], &fargc);
           if (fargs)
             retval = scan_args (&master, fargc, fargs);
         }
	 else if (argv[1][0] == '-')
	 {
	   /* in case of suck -V */
	   retval = scan_args (&master, 1, &(argv[1]));
	 }
	 else
           master.host = argv[1];
         break;
    default:
         for (loop = 1; loop < argc && !fargs; loop++)
	 {
	   if (argv[loop][0] == FILE_CHAR)
	   {
             fargs = build_args (&argv[loop][1], &fargc);
             if (fargs)
               retval = scan_args (&master, fargc, fargs);
           }
	 }
	 /* this is here so anything at command line overrides file */
	 if (argv[1][0] != '-' && argv[1][0] != FILE_CHAR)
	 {
	   master.host = argv[1];
	   argc -= 2;
	   args = &(argv[2]);
	 }
	 else
	 {
	   args = &(argv[1]);
	   argc--;
	 }
	 retval = scan_args (&master, argc, args);
	 break;
  }

  /* print out status stuff */
  if (master.debug == TRUE)
  {
    do_debug ("Suck version %s\n",                 SUCK_VERSION);
    do_debug ("master.MultiFile = %d\n",           master.MultiFile);
    do_debug ("master.status_file = %d\n",         master.status_file);
    do_debug ("master.status_file_name = %s\n",    NULL_STR (master.status_file_name));
    do_debug ("master.do_killfile = %s\n",         TRUE_STR (master.do_killfile));
    do_debug ("master.do_chkhistory = %s\n",       TRUE_STR (master.do_chkhistory));
    do_debug ("master.do_modereader = %s\n",       TRUE_STR (master.do_modereader));
    do_debug ("master.always_batch = %s\n",        TRUE_STR (master.always_batch));
    do_debug ("master.rnews_size = %ld\n",         master.rnews_size);
    do_debug ("master.batch = %d\n",               master.batch);
    do_debug ("master.batchfile = %s\n",           NULL_STR (master.batchfile));
    do_debug ("master.cleanup = %s\n",             TRUE_STR (master.cleanup));
    do_debug ("master.host = %s\n",                NULL_STR (master.host));
    do_debug ("master.portnr = %u\n",              master.portnr);
    do_debug ("master.pause_time = %d\n",          master.pause_time);
    do_debug ("master.pause_nrmsgs = %d\n",        master.pause_nrmsgs);
    do_debug ("master.sig_pause_time = %d\n",      master.sig_pause_time);
    do_debug ("master.sig_pause_nrmsgs = %d\n",    master.sig_pause_nrmsgs);
    do_debug ("master.killfile_log = %d\n",        master.killfile_log);
    do_debug ("master.phrases = %s\n",             NULL_STR (master.phrases));
    do_debug ("master.errlog = %s\n",              NULL_STR (master.errlog));
    do_debug ("master.rescan = %s\n",              TRUE_STR (master.rescan == TRUE));
    do_debug ("master.quiet = %s\n",               TRUE_STR (master.quiet));
    do_debug ("master.kill_ignore_postfix = %s\n", TRUE_STR (master.kill_ignore_postfix));
    do_debug ("master.reconnect_nr=%d\n",          master.reconnect_nr);
    do_debug ("master.do_active = %s\n",           TRUE_STR (master.do_active));
    do_debug ("master.localhost = %s\n",           NULL_STR (master.localhost));
    do_debug ("master.nrmode = %s\n",              TRUE_STR (master.nrmode));
    do_debug ("master.auto_auth = %s\n",           TRUE_STR (master.auto_auth));
    do_debug ("master.no_dedupe = %s\n",           TRUE_STR (master.no_dedupe));
    do_debug ("master.chk_msgid = %s\n",           TRUE_STR (master.chk_msgid));
    do_debug ("master.activefile = %s\n",          NULL_STR (master.activefile));
    do_debug ("master.prebatch = %s\n",            TRUE_STR (master.prebatch));
    do_debug ("master.skip_on_restart = %s\n",     TRUE_STR (master.skip_on_restart));
    do_debug ("master.kill_log_name = %s\n",       NULL_STR (master.kill_log_name));

#ifdef TIMEOUT
    do_debug ("TimeOut = %d\n", TimeOut);
#endif
    do_debug ("master.debug = TRUE\n");
  }

  /* now do any final args checks needed
   * check to see if we have enough info to scan the localhost active file
   */
  if ((master.do_active == TRUE || master.batch == BATCH_LIHAVE) &&
      !master.localhost)
  {
    retval = RETVAL_ERROR;
    error_log (ERRLOG_REPORT, suck_phrases[6], NULL);
  }

  /* okay now the main stuff */
  if (retval == RETVAL_OK)
  {
    if (master.status_file == FALSE)
    {
      /* if not in multifile mode, all status msgs MUST go to stderr to
       * not mess up articles. This has to go before lockfile code, so
       * lockfile prints msg to correct place.
       */
      master.msgs = (master.MultiFile == FALSE ? stderr : stdout);
    }
#ifdef LOCKFILE
    /* this has to be here since we use full_path() to get path for lock
     * file and it isn't set up until we process the args above.
     */
    if (do_lockfile (&master) != RETVAL_OK)
      return (-1);
#endif

#ifdef MYSIGNAL
    signal (MYSIGNAL, sighandler);        /* set up signal handlers */
    signal (PAUSESIGNAL, sighandler);
    signal_block (MYSIGNAL_SETUP);        /* set up sgetline() to block signal */
    pause_signal (PAUSE_SETUP, &master);  /* set up routine for pause swap if signal */
#endif
    load_phrases (&master);	/* this has to be here so rest prints okay */

    /* set up status log, if none specified or unable to open status log
     * then  use stdout or stderr
     */
    if (master.status_file_name)
    {
      /* okay attempt to open it up */
      master.msgs = fopen (master.status_file_name, "a");
      if (!master.msgs)
      {
	MyPerror (suck_phrases[0]);
	master.msgs = stdout;	/* reset to default */
      }
      else
        master.status_file = TRUE;
    }
    setvbuf (master.msgs, NULL, _IOLBF, 0);     /* set to line buffering */

    sock_init();                                /* in WatTCP library */

    /* do we batch up any lingering articles ? */
    if (master.prebatch == TRUE)
    {
      switch (master.batch)
      {
        case BATCH_FALSE:
	     error_log (ERRLOG_REPORT, suck_phrases[58], NULL);
	     break;
        case BATCH_INNXMIT:
	     do_innbatch (&master);
	     break;
        case BATCH_RNEWS:
	     do_rnewsbatch (&master);
	     break;
        case BATCH_LMOVE:
	     do_lmovebatch (&master);
	     break;
        case BATCH_LIHAVE:
	     do_localpost (&master);
	     break;
      }
    }

    /* now parse the killfile */
#ifdef KILLFILE
    if (master.do_killfile == TRUE)
      master.killp = parse_killfile (master.killfile_log, master.debug,
                                     master.kill_ignore_postfix);
#endif

    print_phrases (master.msgs, suck_phrases[1], master.host, NULL);
    if (do_connect (&master, CONNECT_FIRST) != RETVAL_OK)
      retval = RETVAL_ERROR;
    else
    {
      /* first, check for restart articles,
       * then scan for any new articles
       */
      if ((loop = restart_yn (&master)) == RESTART_ERROR)
        retval = RETVAL_ERROR;

      else if (loop == RESTART_NO || master.rescan == TRUE)
      {
        /* we don't do scan if we had restart and option = FALSE
         * do we scan the local active file?
         */
        if (master.do_active == TRUE || master.activefile)
	{
	  if (get_message_index_active (&master) < RETVAL_OK)
            retval = RETVAL_ERROR;
        }
	else if (get_message_index (&master) < RETVAL_OK)
          retval = RETVAL_ERROR;
      }
      if (retval == RETVAL_OK)
      {
	if (master.nritems == 0)
	{
	  print_phrases (master.msgs, suck_phrases[3], NULL);
	  retval = RETVAL_NOARTICLES;
	}
	else
          retval = get_articles (&master);
      }
      /* send quit, and get reply */
      sputline (master.sockfd, "quit\r\n");
      if (master.debug == TRUE)
        do_debug ("Sending command: quit\n");
      do
      {
	resp = sgetline (master.sockfd, &inbuf);
	if (resp > 0)
	{
	  if (master.debug == TRUE)
            do_debug ("Quitting GOT: %s", inbuf);
          number (inbuf, &nr);
        }
      }
      while (nr != 205 && resp > 0);
    }
    if (master.sockfd >= 0)
    {
      close (master.sockfd);
      print_phrases (master.msgs, suck_phrases[4], master.host, NULL);
    }
    if (master.debug == TRUE)
      do_debug ("retval=%d (RETVAL_OK=%d), m.nrgot=%d, m.batch=%d\n",
                retval, RETVAL_OK, master.nrgot, master.batch);

    if ((retval == RETVAL_OK || master.always_batch == TRUE) && master.nrgot > 0)
    {
      switch (master.batch)
      {
        case BATCH_INNXMIT:
	     do_innbatch (&master);
	     break;
        case BATCH_RNEWS:
	     do_rnewsbatch (&master);
	     break;
        case BATCH_LMOVE:
	     do_lmovebatch (&master);
	     break;
        case BATCH_LIHAVE:
	     do_localpost (&master);
	     break;
        default:
	     break;
      }
    }
    if ((retval == RETVAL_NOARTICLES || retval == RETVAL_OK) &&
        master.cleanup == TRUE)
    {
      print_phrases (master.msgs, suck_phrases[7], NULL);
      do_cleanup();
    }
    /* close out status log */
    if (master.msgs && master.msgs != stdout && master.msgs != stderr)
    {
      fclose (master.msgs);
    }
    if (master.head)
    {
      /* clean up memory */
      master.curr = master.head;
      while (master.curr)
      {
	temp = (master.curr)->next;
	free_one_node (master.curr);
	master.curr = temp;
      }
    }
    /* free up group list */
    while (master.groups)
    {
      ptemp = (master.groups)->next;
      if ((master.groups)->group)
        free ((master.groups)->group);

      free (master.groups);
      master.groups = ptemp;
    }

#ifdef KILLFILE
    free_killfile (master.killp);
#endif

    if (fargs)
      free_args (fargc, fargs);

#ifdef LOCKFILE
    lockfile = full_path (FP_GET, FP_TMPDIR, N_LOCKFILE);
    if (lockfile)
      unlink (lockfile);
#endif
  }
  free_phrases();     /* do this last so everything is displayed correctly */
  return (retval);
}


int do_connect (PMaster master, int which_time)
{
  char  *inbuf;
  int    nr, resp, retval = RETVAL_OK;
  struct hostent *hi;
  FILE  *fp;

  if (which_time != CONNECT_FIRST)
  {
    /* close down previous connection */
    sputline (master->sockfd, "quit\r\n");
    do
    {
      resp = sgetline (master->sockfd, &inbuf);
      if (resp > 0)
      {
	if (master->debug == TRUE)
          do_debug ("Reconnect GOT: %s", inbuf);
        number (inbuf, &nr);
      }
    }
    while (nr != 205 && resp > 0);
    close (master->sockfd);

    /* now reset everything */
    if (master->curr)
       (master->curr)->sentcmd = FALSE;
    master->grpnr = -1;
  }

  if (master->debug == TRUE)
    do_debug ("Connecting to %s on port %d\n", master->host, master->portnr);

  fp = (which_time == CONNECT_FIRST) ? master->msgs : NULL;
  master->sockfd = connect_to_nntphost (master->host, &hi, fp, master->portnr);
  if (master->sockfd < 0)
    retval = RETVAL_ERROR;

  else if (sgetline (master->sockfd, &inbuf) < 0)
  {
    /* Get the announcement line */
    retval = RETVAL_ERROR;
  }
  else
  {
    if (master->debug == TRUE)
      do_debug ("Got: %s", inbuf);

    if (which_time == CONNECT_FIRST)
      fprintf (master->msgs, "%s", inbuf);

    /* check to see if we have to do authorization now */
    number (inbuf, &resp);
    if (resp == 480)
      retval = do_authenticate (master);

    if (retval == RETVAL_OK && master->do_modereader == TRUE)
    {
      retval = send_command (master, "mode reader\r\n", &inbuf, 0);
      if (retval == RETVAL_OK)
      {
	/* Again the announcement */
	if (which_time == CONNECT_FIRST)
          fprintf (master->msgs, "%s", inbuf);
      }
    }
    if (master->auto_auth == TRUE)
    {
      if (!master->passwd || !master->userid)
      {
	error_log (ERRLOG_REPORT, suck_phrases[55], NULL);
	retval = RETVAL_ERROR;
      }
      else
      {
        /* auto authorize */
	retval = do_authenticate (master);
      }
    }
  }
  return (retval);
}


int get_message_index (PMaster master)
{
  long  lastread;
  int   nrread, retval, maxread;
  char  buf[MAXLINLEN], group[512];
  FILE *ifp, *tmpfp;

  retval = RETVAL_OK;
  ifp    = tmpfp = NULL;

  TimerFunc (TIMER_START, 0, NULL);

  ifp = fopen (full_path(FP_GET,FP_DATADIR,N_OLDRC), "r");
  if (!ifp)
  {
    MyPerror (full_path (FP_GET, FP_DATADIR, N_OLDRC));
    retval = RETVAL_ERROR;
  }
  else if ((tmpfp = fopen (full_path(FP_GET,FP_TMPDIR,N_NEWRC),"w")) == NULL)
  {
    MyPerror (full_path (FP_GET, FP_TMPDIR, N_NEWRC));
    retval = RETVAL_ERROR;
  }

  while (retval == RETVAL_OK && fgets (buf, MAXLINLEN - 1, ifp))
  {
    if (buf[0] == SUCKNEWSRC_COMMENT_CHAR)
    {
      /* skip this group */
      fputs (buf, tmpfp);
      print_phrases (master->msgs, suck_phrases[8], buf, NULL);
    }
    else
    {
      maxread = -1;		/* just in case */
      nrread = sscanf (buf, "%s %ld %d\n", group, &lastread, &maxread);
      if (nrread < 2 || nrread > 3)
      {
	error_log (ERRLOG_REPORT, suck_phrases[9], buf, NULL);
	fputs (buf, tmpfp);	/* rewrite the line */
      }
      else if (maxread == 0)
      {
	/* just rewrite the line */
	fputs (buf, tmpfp);
      }
      else
        retval = do_one_group (master, buf, group, tmpfp, lastread, maxread);
    }
  }

  TimerFunc (TIMER_TIMEONLY, 0, master->msgs);

  if (retval == RETVAL_OK)
  {
    print_phrases (master->msgs, suck_phrases[16], str_int(master->nritems), NULL);
    retval = do_supplemental (master);
  }
  else if (ifp)
  {
    /* this is in case we had to abort the above while loop (due to loss of
     * pipe to server) and we hadn't finished writing out the suck.newrc,
     * this finishes it up.
     */
    do
      fputs (buf, tmpfp);
    while (fgets (buf, MAXLINLEN - 1, ifp));
  }
  if (tmpfp)
    fclose (tmpfp);

  if (ifp)
    fclose (ifp);

  return (retval);
}


int do_one_group (PMaster master, char *buf, char *group, FILE *newrc,
                  long lastread, int maxread)
{
  char *sp, *inbuf, cmd[MAXLINLEN];
  long  count, low, high;
  int   response, retval, i;

  retval = RETVAL_OK;

  sprintf (cmd, "group %s\r\n", group);
  if (send_command (master, cmd, &inbuf, 0) != RETVAL_OK)
    retval = RETVAL_ERROR;

  else
  {
    sp = number (inbuf, &response);
    if (response != 211)
    {
      fputs (buf, newrc);	/* rewrite line AS IS in newrc */
      /* handle group not found */
      switch (response)
      {
        case 411:
	     error_log (ERRLOG_REPORT, suck_phrases[11], group, NULL);
	     break;
        case 500:
	     error_log (ERRLOG_REPORT, suck_phrases[48], NULL);
	     break;
        default:
             error_log (ERRLOG_REPORT, suck_phrases[12], group, str_int(response), NULL);
	     retval = RETVAL_ERROR;	/* bomb out on wacko errors */
	     break;
      }
    }
    else
    {
      sp = get_long (sp, &count);
      sp = get_long (sp, &low);
      sp = get_long (sp, &high);
      fprintf (newrc, "%s %ld", group, high);

      if (maxread > 0)
        fprintf (newrc, " %d", maxread);

      fputs ("\n", newrc);

      /* Add a sanity check in case remote host changes its numbering scheme.
       * The > 0 is needed, since if a nnrp site has no article it will reset.
       * The count to 0.  So not an error.
       */
      if (lastread > high && high > 0)
        print_phrases (master->msgs, suck_phrases[13], group, str_int(high), NULL);

      if (lastread < 0)
      {
        /* if a neg number, get the last X nr of messages, handy for starting.
         * a new group off ,say -100 will get the last 100 messages.
         */
	lastread += high;	/* this works since lastread is neg */
	if (lastread < 0)
            lastread = 0;       /* just to be on the safeside */
      }

      /* This has to be >= 0 since if there are no article on server high = 0.
       * So if we write out 0, we must be able to handle zero as valid lastread.
       * The count > 0 so if no messages available we don't even try.
       * Or if low > high no messages either
       */
      if (low <= high && count > 0 && lastread < high && lastread >= 0)
      {
	/* add group name to list of groups */

	if (lastread < low)
            lastread = low - 1;

	/* now set the max nr of messages to read */
	if (maxread > 0 && high - maxread > lastread)
	{
	  lastread = high - maxread;
	  print_phrases (master->msgs, suck_phrases[14], group, str_int (maxread), NULL);
	}

	sprintf (cmd, "xhdr Message-ID %ld-\r\n", lastread + 1);
        print_phrases (master->msgs, suck_phrases[15], group,
                       str_long(high - lastread), str_long(lastread + 1),
                       str_long(high), NULL);
	i = send_command (master, cmd, &inbuf, 221);
	if (i != RETVAL_OK)
          retval = RETVAL_ERROR;
        else
	{
	  do
	  {
	    if (sgetline (master->sockfd, &inbuf) < 0)
              retval = RETVAL_ERROR;

	    else if (*inbuf != '.')
              retval = allocnode (master, inbuf, MANDATORY_OPTIONAL, group);
          }
	  while (retval == RETVAL_OK && *inbuf != '.' && *(inbuf + 1) != '\n');
	}			/* end if response */
      }				/* end if lastread */
    }				/* end response */
  }				/* end else */

  if (retval == RETVAL_ERROR)
    error_log (ERRLOG_REPORT, suck_phrases[59], NULL);
  return (retval);
}


int get_articles (PMaster master)
{
  int    retval, logcount, i;
  FILE  *logfp;
  double bps;

#ifdef KILLFILE
  int (*get_message) (PMaster,int);  /* which function will we use
                                      * get_one_article or get_one_article_kill)
                                      */
  /* do we use killfiles? */
  get_message = (master->killp == NULL) ? get_one_article : get_one_article_kill;
#endif

  retval = RETVAL_OK;
  logfp  = NULL;

  /* Figure out how many digits wide the articleCount is for display purposes
   * This used to be log10()+1, but that meant I needed the math library
    */
  for (logcount = 1, i = master->nritems; i > 9; logcount++)
    i /= 10;

  if (master->MultiFile == TRUE && !checkdir(full_path(FP_GET,FP_MSGDIR,NULL)))
    retval = RETVAL_ERROR;
  else
  {
    logfp = fopen (full_path (FP_GET, FP_TMPDIR, N_RESTART), "w");
    if (!logfp)
    {
      MyPerror (full_path (FP_GET, FP_TMPDIR, N_RESTART));
      retval = RETVAL_ERROR;
    }
    else
    {
      /* do this here so we write out the restart for all articles
       * already downloaded
       */
      master->curr = master->head;
      for (i = 0; i < master->itemon; i++)
      {
	fprintf (logfp, "1");
	master->curr = (master->curr)->next;
      }
      fflush (logfp);

      if (master->batch == BATCH_INNFEED)
      {
	/* open up batch file for appending to */
        master->innfeed = fopen (master->batchfile, "a");
        if (!master->innfeed)
           MyPerror (master->batchfile);
      }
      else if (master->batch == BATCH_LIHAVE)
      {
        master->innfeed = fopen (full_path(FP_GET,FP_TMPDIR,master->batchfile),"a");
        if (!master->innfeed)
	{
	  MyPerror (full_path (FP_GET, FP_TMPDIR, master->batchfile));
	  retval = RETVAL_ERROR;
	}
      }

      TimerFunc (TIMER_START, 0, NULL);

#ifdef MYSIGNAL
      while (retval == RETVAL_OK && master->curr && GotSignal == FALSE)
#else
      while (retval == RETVAL_OK && master->curr)
#endif
      {
        if (master->debug == TRUE)
          do_debug ("Article nr = %s mandatory = %c\n", (master->curr)->msgnr, (master->curr)->mandatory);

	/* write out restart */
	fputc ('1', logfp);
	fflush (logfp);
	master->itemon++;

        /* to be polite to the server, lets allow for a pause every once
         * in a while
         */
	if (master->pause_time > 0 && master->pause_nrmsgs > 0)
	{
	  if (master->itemon % master->pause_nrmsgs == 0)
            sleep (master->pause_time);
        }
	if (master->status_file == FALSE && master->quiet == FALSE)
	{
          /* If we are going to a file, we don't want all of these articles
           * printed. Or if quiet flag is set
           */
	  bps = TimerFunc (TIMER_GET_BPS, 0, master->msgs);

          /* These don't go thru print_phrases so I can keep spacing right.
           * And it only prints numbers and the BPS
           */
          fprintf (master->msgs, "%5d %9.1f %s\r",
                   master->nritems - master->itemon, bps, suck_phrases[5]);
          fflush (master->msgs);
	}
        /* get one article */
#ifdef KILLFILE
	retval = (*get_message) (master, logcount);
#else
	retval = get_one_article (master, logcount);
#endif
	master->curr = (master->curr)->next;	/* get next article */

        /* to be NOT polite to the server reconnect every X msgs to combat
         * the INND LIKE_PULLERS=DONT.  This is last in loop, so if problem
         * can abort gracefully
         */
	if (master->reconnect_nr > 0 && master->itemon % master->reconnect_nr == 0)
	{
	  retval = do_connect (master, CONNECT_AGAIN);
          if (master->curr)
             (master->curr)->sentcmd = FALSE; /* if we sent command force resend */
        }
      }
      fclose (logfp);
      if (retval == RETVAL_OK && master->nritems == master->itemon)
        unlink (full_path (FP_GET, FP_TMPDIR, N_RESTART));

      if (retval == RETVAL_OK)
        TimerFunc (TIMER_TOTALS, 0, master->msgs);

      if (master->innfeed)
        fclose (master->innfeed);
    }
  }
  return (retval);
}

/*
 * add items from supplemental list to link list then write it out
 */
int do_supplemental (PMaster master)
{
  FILE   *fp;
  int     groupnr, retval, oldkept;
  PList   curr;
  char    linein[MAXLINLEN + 1];
  PGroups grps;

  retval  = RETVAL_OK;
  oldkept = master->nritems;

  fp = fopen (full_path (FP_GET, FP_DATADIR, N_SUPPLEMENTAL), "r");
  if (fp)
  {
    print_phrases (master->msgs, suck_phrases[17], NULL);
    while (retval == RETVAL_OK && fgets (linein, MAXLINLEN, fp))
    {
      if (linein[0] != '<')
           error_log (ERRLOG_REPORT, suck_phrases[18], linein, NULL);
      else retval = allocnode (master, linein, MANDATORY_YES, NULL);
    }
    print_phrases (master->msgs, suck_phrases[19],
                   str_int(master->nritems - oldkept),
                   str_int(master->nritems), NULL);
    fclose (fp);
  }

  if (master->head && master->nritems > 0)
  {
    /* if we don't have any messages, we don't need to do this */
    if (master->no_dedupe == FALSE)
      dedupe_list (master);

#ifdef CHECK_HISTORY
    if (master->do_chkhistory == TRUE)
      chkhistory (master);
#endif
    print_phrases (master->msgs, suck_phrases[20],
                   str_int(master->nritems), NULL);

    /* create suck.restart */
    if (master->head && master->nritems > 0)
    {
      /* now write out whatever list we have */
      fp = fopen (full_path (FP_GET, FP_TMPDIR, N_SORTED), "w");
      if (!fp)
      {
	retval = RETVAL_ERROR;
	MyPerror (full_path (FP_GET, FP_TMPDIR, N_SORTED));
      }
      else
      {
	curr = master->head;
	groupnr = 0;
        while (curr && retval == RETVAL_OK)
	{
	  if (curr->groupnr != groupnr)
	  {
	    /* print out group name */
	    groupnr = curr->groupnr;
	    if (groupnr > 0)
	    {
	      grps = master->groups;
	      while (grps->nr != groupnr)
                grps = grps->next;

	      fprintf (fp, "%s %s\n", RESTART_GROUP, grps->group);
	    }
	  }

	  if (fprintf (fp, "%ld %s %c\n", curr->nr, curr->msgnr, curr->mandatory) == 0)
	  {
	    retval = RETVAL_ERROR;
	    MyPerror (full_path (FP_GET, FP_TMPDIR, N_SORTED));
	  }
	  curr = curr->next;
	}
	fclose (fp);
      }
    }
  }
  return (retval);
}


int get_group_number (PMaster master, char *grpname)
{
  PGroups grps, gptr;
  int     groupnr = 0;

  /* first, find out if it doesn't exist already */
  gptr = master->groups;
  while (gptr && groupnr == 0)
  {
    if (!strcmp (gptr->group, grpname))
         groupnr = gptr->nr; /* bingo */
    else gptr = gptr->next;
  }

  if (groupnr == 0)
  {
    /* add group to group list and get group nr */
    grps = malloc (sizeof(Groups));
    if (!grps)
    {
      /* out of memory */
      error_log (ERRLOG_REPORT, suck_phrases[22], NULL);
    }
    else
    {
      grps->next  = NULL;
      grps->group = strdup (grpname);
      if (!grps->group)
      {
	/* out of memory */
	error_log (ERRLOG_REPORT, suck_phrases[22], NULL);
	free (grps);
      }
      else
      {
        /* now add to list and count groupnr */
        if (!master->groups)
	{
	  grps->nr = 1;
	  master->groups = grps;
	}
	else
	{
	  gptr = master->groups;
          while (gptr->next)
            gptr = gptr->next;

	  gptr->next = grps;
          grps->nr++;
	}
	groupnr = grps->nr;

	if (master->debug == TRUE)
          do_debug ("Adding to group list: %d %s\n", grps->nr, grps->group);
      }
    }
  }
  return (groupnr);
}


int allocnode (PMaster master, char *linein, int mandatory, char *group)
{
  /* if allocate memory here, must free in free_one_node */

  PList  ptr = NULL;
  char  *end_ptr, *st_ptr;
  static PList curr = NULL;	/* keep track of current end of list */
  int    groupnr    = 0, retval = RETVAL_OK;
  long   msgnr      = 0;
  static int warned = FALSE;

  /* get the article nr */
  if (!group)
  {
    /* we're called by do_supplemental */
    st_ptr = linein;
  }
  else
  {
    st_ptr = get_long (linein, &msgnr);
    if (msgnr == 0 && warned == FALSE)
    {
      warned = TRUE;
      error_log (ERRLOG_REPORT, suck_phrases[53], NULL);
    }
    if (msgnr > 0 && group)
      groupnr = get_group_number (master, group);
  }

  if (retval == RETVAL_OK)
  {
    /* find the message id */
    while (*st_ptr != '<' && *st_ptr)
      st_ptr++;

    end_ptr = st_ptr;
    while (*end_ptr != '>' && *end_ptr)
      end_ptr++;

    if (*st_ptr != '<' || (master->chk_msgid == TRUE && *end_ptr != '>'))
      error_log (ERRLOG_REPORT, suck_phrases[21], linein, NULL);

    else
    {
      *(end_ptr+1) = '\0';    /* ensure null termination */

      ptr = malloc (sizeof(List));
      if (!ptr)
      {
	error_log (ERRLOG_REPORT, suck_phrases[22], NULL);
	retval = RETVAL_ERROR;
      }
      else if ((ptr->msgnr = strdup(st_ptr)) == NULL)
      {
	error_log (ERRLOG_REPORT, suck_phrases[22], NULL);
	free (ptr);
	retval = RETVAL_ERROR;
      }
      else
      {
        ptr->next      = NULL;
        ptr->mandatory = mandatory;
        ptr->sentcmd   = FALSE;   /* have we sent command to remote */
        ptr->nr        = msgnr;
        ptr->groupnr   = groupnr;
	if (master->debug == TRUE)
          do_debug ("MSGID %s NR %d GRP %d MANDATORY %c added\n",
                    ptr->msgnr, ptr->nr, ptr->groupnr, ptr->mandatory);

	/* now put on list */
        if (!curr)
	{
	  /* first node */
	  master->head = curr = ptr;
	}
	else
	{
	  curr->next = ptr;
	  curr = ptr;
	}
	master->nritems++;
      }
    }
  }
  return (retval);
}


void free_one_node (PList node)
{
  free (node->msgnr);
  free (node);
}

const char *build_command (PMaster master, const char *cmdstart, PList article)
{
  static  char cmd[MAXLINLEN + 1];
  static  int  warned = FALSE;
  char   *resp;
  char    grpcmd[MAXLINLEN];
  PGroups grps;

  /* build command to get article/head/body
   * if nrmode is on send group cmd if needed
   */
  if (master->nrmode == TRUE)
  {
    if (article->nr == 0)
       master->nrmode = FALSE;

    else if (master->grpnr != article->groupnr)
    {
      grps = master->groups;
      while (grps && grps->nr != article->groupnr)
        grps = grps->next;

      if (grps)
      {
	/* okay send group command */
	sprintf (grpcmd, "GROUP %s\r\n", grps->group);
	if (send_command (master, grpcmd, &resp, 211) != RETVAL_OK)
        {
          master->nrmode = FALSE;       /* can't chg groups turn it off */
        }
        else
	{
         /* so don't redo group command on next one */
	  master->grpnr = grps->nr;
	}
      }
    }
    if (master->nrmode == TRUE)
    {
      /* everything hunky dory, we've switched groups, and got a good nr */
      sprintf (cmd, "%s %ld\r\n", cmdstart, article->nr);
    }
    else if (warned == FALSE)
    {
      /* tell of the switch to nonnr mode */
      warned = TRUE;
      error_log (ERRLOG_REPORT, suck_phrases[54], NULL);
    }
  }
  /* the if is in case we changed it above */
  if (master->nrmode == FALSE)
    sprintf (cmd, "%s %s\r\n", cmdstart, article->msgnr);

  return (cmd);
}


int get_one_article (PMaster master, int logcount)
{
  int   nr, len, retval = RETVAL_OK;
  char  buf[MAXLINLEN + 1];
  const char *cmd = "", *tname = NULL;
  char  fname[PATH_MAX + 1];
  char *resp;
  PList plist;
  FILE *fptr = stdout;          /* the default */

  fname[0] = '\0';		/* just in case */

  /* first send command to get article if not already sent
   */
  if ((master->curr)->sentcmd == FALSE)
  {
    cmd = build_command (master, "article", master->curr);
    if (master->debug == TRUE)
      do_debug ("Sending command: \"%s\"", cmd);

    sputline (master->sockfd, cmd);
    (master->curr)->sentcmd = TRUE;
  }
  plist = (master->curr)->next;
  if (plist)
  {
    if (master->nrmode == FALSE || plist->groupnr == (master->curr)->groupnr)
    {
      /* can't use pipelining if nrmode on, cause of GROUP command I'll
       * have to send
       */
      if (plist->sentcmd == FALSE)
      {
	/* send next command */
	cmd = build_command (master, "article", plist);
	if (master->debug == TRUE)
          do_debug ("Sending command: \"%s\"", cmd);

	sputline (master->sockfd, cmd);
	plist->sentcmd = TRUE;
      }
    }
  }

  /* now while the remote is finding the article lets set up */

  if (master->MultiFile == TRUE)
  {
    /* open file
     * file name will be ####-#### ex 001-166 (nron,total)
     */
    sprintf (buf, "%0*d-%d", logcount, master->itemon, master->nritems);

    /* the strcpy to avoid wiping out fname in second call to full_path
     */
    strcpy (fname, full_path (FP_GET, FP_MSGDIR, buf));
    strcat (buf, N_TMP_EXTENSION);	/* add tmp extension */
    tname = full_path (FP_GET, FP_TMPDIR, buf);		/* temp file name */

    if (master->debug == TRUE)
      do_debug ("File name = \"%s\" temp = \"%s\"", fname, tname);

    fptr = fopen (tname, "w");
    if (!fptr)
    {
      MyPerror (tname);
      retval = RETVAL_ERROR;
    }
  }

  /* okay hopefully by this time the remote end is ready */
  len = sgetline (master->sockfd, &resp);
  if (len < 0)
     retval = RETVAL_ERROR;
  else
  {
    if (master->debug == TRUE)
      do_debug ("got answer: %s", resp);

    TimerFunc (TIMER_ADDBYTES, len, NULL);

    number (resp, &nr);
    if (nr == 480)
    {
      /* got to authorize, negate send-ahead for next msg
       */
      plist->sentcmd = FALSE;	/* cause its gonna error out in do_auth() */
      if (do_authenticate(master) == RETVAL_OK)
      {
	/* resend command for current article */
	cmd = build_command (master, "article", master->curr);
	if (master->debug == TRUE)
          do_debug ("Sending command: \"%s\"", cmd);

	sputline (master->sockfd, cmd);
	len = sgetline (master->sockfd, &resp);
	if (master->debug == TRUE)
          do_debug ("got answer: %s", resp);

	TimerFunc (TIMER_ADDBYTES, len, NULL);
	number (resp, &nr);
      }
      else
        retval = RETVAL_ERROR;
    }
    if (nr == 220)
    {
      /* get the article */
      if ((retval = get_a_chunk (master->sockfd, fptr)) == RETVAL_OK)
      {
	master->nrgot++;
	if (fptr == stdout)
	{
	  /* gracefully end the message in stdout version */
	  fputs (".\n", stdout);
	}
      }
    }
    else
    {
      /* don't return RETVAL_ERROR so we can get next article */
      error_log (ERRLOG_REPORT, suck_phrases[29], cmd, resp, NULL);
    }
  }
  if (fptr && fptr != stdout)
    fclose (fptr);

  if (master->MultiFile == TRUE)
  {
    if (retval == RETVAL_ERROR || nr != 220)
      unlink (tname);

    /* now rename it to the permanent file name */
    else
    {
      move_file (tname, fname);
      if ((master->batch == BATCH_INNFEED || master->batch == BATCH_LIHAVE) &&
          master->innfeed)
      {
	/* write path name and msgid to file */
	fprintf (master->innfeed, "%s %s\n", fname, (master->curr)->msgnr);
	fflush (master->innfeed);	/* so it gets written sooner */
      }
    }
  }
  return (retval);
}


int get_a_chunk (int sockfd, FILE * fptr)
{
  int    done, partial, len, retval;
  char  *inbuf;
  size_t nr;

  retval  = RETVAL_OK;
  done    = FALSE;
  partial = FALSE;

 /* partial = was the previous line a complete line or not
  * this is needed to avoid a scenario where the line is MAXLINLEN+1
  * long and the last character is a ., which would make us think
  * that we are at the end of the article when we actually aren't
  */

  while (done == FALSE && (len = sgetline(sockfd,&inbuf)) >= 0)
  {
    TimerFunc (TIMER_ADDBYTES, len, NULL);

    if (inbuf[0] == '.' && partial == FALSE)
    {
      if (len == 2 && inbuf[1] == '\n')
        done = TRUE;

      else if (fptr != stdout)
      {
       /* handle double dots IAW RFC977 2.4.1
        * if we are going to stdout, we have to leave double dots alone
        * since it might mess up the .\n for EOM
        */
	inbuf++;		/* move past first dot */
	len--;
      }
    }
    if (done == FALSE)
    {
      if (retval == RETVAL_OK)
      {
	nr = fwrite (inbuf, sizeof (inbuf[0]), len, fptr);
	fflush (fptr);
	if (nr != len)
	{
	  retval = RETVAL_ERROR;
	  error_log (ERRLOG_REPORT, suck_phrases[57], NULL);
	}
      }
      partial = (len == MAXLINLEN && inbuf[len-1] != '\n') ? TRUE : FALSE;
    }
  }
  if (len < 0)
    retval = RETVAL_ERROR;

  return (retval);
}

/*
 * The restart file consists of 1 byte for each article prior to it
 * being written.  So if its 2 bytes long, we've actually only got
 * the first article.  If its zero bytes long, we aborted before we
 * even got a chance to write the first byte
 */
int restart_yn (PMaster master)
{
  struct stat buf;
  int    i, len, x, grpnr, retval = RESTART_NO;
  FILE  *fp;
  char   msgid[MAXLINLEN], group[MAXLINLEN], optional;
  const  char *fso;

  strcpy (msgid, full_path (FP_GET, FP_TMPDIR, N_RESTART));
  if (!stat(msgid,&buf))
  {
    /* If suck.restart exists something went wrong */
    retval = RESTART_YES;
    master->itemon = (buf.st_size == 0 ? 0 : buf.st_size-1);

    /* the above is done since we write the byte before hand
     * now build the link list in master
     *
     * do we skip the article we timed out on when we aborted?
     */
    if (master->skip_on_restart == TRUE)
    {
      master->itemon++;
      print_phrases (master->msgs, suck_phrases[60], NULL);
    }

    fso = full_path (FP_GET, FP_TMPDIR, N_SORTED);
    i   = RETVAL_OK;
    fp  = fopen (fso, "r");
    if (fp)
    {
      len = strlen (RESTART_GROUP);
      grpnr = 0;
      while (fgets(msgid,MAXLINLEN,fp))
      {
	x = strlen (msgid);
	/* strip off nl */
        if (msgid[x-1] == '\n')
            msgid[--x] = '\0';

        if (!strncmp(msgid, RESTART_GROUP, len))
	{
	  /* got a group */
	  strcpy (group, &msgid[len + 1]);
	}
	else
	{
	  optional = msgid[x];
          msgid[x-1] = '\0';
	  /* restart recovery file is in format that allocnode needs */
	  i = allocnode (master, msgid, optional, group);
	}
      }
      fclose (fp);
      print_phrases (master->msgs, suck_phrases[2],
                     str_int(master->itemon), str_int(master->nritems), NULL);
    }
    else
    {
      error_log (ERRLOG_REPORT, suck_phrases[23], msgid, fso, NULL);
      unlink (msgid);		/* so we have a clean start */
      retval = RESTART_NO;
    }
  }
  return (retval);
}


#ifdef MYSIGNAL
void sighandler (int what)
{
  switch (what)
  {
    case PAUSESIGNAL:
	 pause_signal (PAUSE_DO, NULL);
	 /* if we don't do this, the next time called, we'll abort */
	 signal (PAUSESIGNAL, sighandler);
         signal_block (MYSIGNAL_SETUP);   /* just to be on the safe side */
	 break;
    case MYSIGNAL:
    default:
	 error_log (ERRLOG_REPORT, suck_phrases[24], NULL);
	 GotSignal = TRUE;
  }
}


void pause_signal (int action, PMaster master)
{                                                
  int    x, y;
  static PMaster psave = NULL;

  switch (action)
  {
    case PAUSE_SETUP:
	 psave = master;
	 break;
    case PAUSE_DO:
         if (!psave)
           error_log (ERRLOG_REPORT, suck_phrases[25], NULL);
         else
	 {
	   /* swap pause_time and pause_nrmsgs with the sig versions */
	   x = psave->pause_time;
	   y = psave->pause_nrmsgs;
           psave->pause_time       = psave->sig_pause_time;
           psave->pause_nrmsgs     = psave->sig_pause_nrmsgs;
           psave->sig_pause_time   = x;
	   psave->sig_pause_nrmsgs = y;
	   print_phrases (psave->msgs, suck_phrases[26], NULL);
	 }
  }
}
#endif


int send_command (PMaster master, const char *cmd, char **ret_response,
                  int good_response)
{
  int   len, retval = RETVAL_OK, nr;
  char *resp;

  if (master->debug == TRUE)
    do_debug ("sending command: %s", cmd);

  sputline (master->sockfd, cmd);
  len = sgetline (master->sockfd, &resp);
  if (len < 0)
    retval = RETVAL_ERROR;

  else
  {
    if (master->debug == TRUE)
      do_debug ("got answer: %s", resp);

    TimerFunc (TIMER_ADDBYTES, len, NULL);

    number (resp, &nr);
    if (nr == 480)
    {
      /* we must do authorization */
      retval = do_authenticate (master);
      if (retval == RETVAL_OK)
      {
	/* resend command */
	sputline (master->sockfd, cmd);
	if (master->debug == TRUE)
          do_debug ("sending command: %s", cmd);

	len = sgetline (master->sockfd, &resp);
	if (len < 0)
          retval = RETVAL_ERROR;

	else
	{
	  number (resp, &nr);
	  if (master->debug == TRUE)
            do_debug ("got answer: %s", resp);

	  TimerFunc (TIMER_ADDBYTES, len, NULL);
        }
      }
    }
    if (good_response && nr != good_response)
    {
      error_log (ERRLOG_REPORT, suck_phrases[29], cmd, resp, NULL);
      retval = RETVAL_UNEXPECTEDANS;
    }
  }
  if (ret_response)
     *ret_response = resp;

  return (retval);
}

/*
 * 1. move N_OLDRC to N_OLD_OLDRC    N_OLDRC might not exist
 * 2. move N_NEWRC to N_OLDRC
 * 3. rm N_SORTED
 * 4. rm N_SUPPLEMENTAL
 */
void do_cleanup (void)
{
  const  char *oldptr;
  char   ptr[PATH_MAX + 1];
  struct stat buf;
  int    exist;

  strcpy (ptr, full_path (FP_GET, FP_DATADIR, N_OLDRC));

  /* must strcpy since full path overwrites itself everytime */
  oldptr = full_path (FP_GET, FP_DATADIR, N_OLD_OLDRC);

  /* does the sucknewsrc file exist ? */
  exist = TRUE;
  if (stat (ptr, &buf) && errno == ENOENT)
    exist = FALSE;

  if (exist == TRUE && move_file (ptr, oldptr))
  {
    MyPerror (suck_phrases[30]);
  }
  else if (move_file (full_path (FP_GET, FP_TMPDIR, N_NEWRC), ptr))
  {
    MyPerror (suck_phrases[31]);
  }
  else if (unlink (full_path (FP_GET, FP_TMPDIR, N_SORTED)) && errno != ENOENT)
  {
    /* ENOENT is not an error since this file may not always exist
     * and it won't if there weren't any articles
     */
    MyPerror (suck_phrases[32]);
  }
  else if (unlink (full_path (FP_GET, FP_DATADIR, N_SUPPLEMENTAL)) && errno != ENOENT)
  {
    /* ENOENT is not an error since this file may not always exist
     */
    MyPerror (suck_phrases[33]);
  }
}


int scan_args (PMaster master, int argc, char **argv)
{
  int loop, i, whicharg, arg, retval = RETVAL_OK;

  for (loop = 0; loop < argc && retval == RETVAL_OK; loop++)
  {
    arg = ARG_NO_MATCH;
    whicharg = -1;

    if (master->debug == TRUE)
      do_debug ("Checking arg #%d-%d: '%s'\n", loop, argc, argv[loop]);

    if (argv[loop][0] == FILE_CHAR)
    {
      /* totally skip these they are processed elsewhere */
      continue;
    }

    if (argv[loop][0] == '-' && argv[loop][1] == '-')
    {
      /* check long args */
      for (i = 0; i < NR_ARGS && arg == ARG_NO_MATCH; i++)
      {
        if (arglist[i].larg)
	{
          if (!strcmp (&argv[loop][2], arglist[i].larg))
	  {
	    arg = arglist[i].flag;
	    whicharg = i;
	  }
	}
      }
    }
    else if (argv[loop][0] == '-')
    {
      /* check short args */
      for (i = 0; i < NR_ARGS; i++)
      {
        if (arglist[i].sarg)
	{
          if (!strcmp (&argv[loop][1], arglist[i].sarg))
	  {
	    arg = arglist[i].flag;
	    whicharg = i;
	  }
	}
      }
    }
    if (arg == ARG_NO_MATCH)
    {
      /* we ignore the file_char arg, it is processed elsewhere */
      retval = RETVAL_ERROR;
      error_log (ERRLOG_REPORT, suck_phrases[34], argv[loop], NULL);
    }
    else
    {
      /* need to check for valid nr of args and feed em down */
      if ((loop + arglist[whicharg].nr_params) < argc)
      {
	retval = parse_args (master, arg, &(argv[loop + 1]));
	/* skip em so they don't get processed as args */
	loop += arglist[whicharg].nr_params;
      }
      else
      {
	i = (arglist[whicharg].errmsg < 0) ? 34 : arglist[whicharg].errmsg;
	error_log (ERRLOG_REPORT, suck_phrases[i], NULL);
	retval = RETVAL_ERROR;
      }
    }
  }
  return (retval);
}


int parse_args (PMaster master, int arg, char **argv)
{
  int retval = RETVAL_OK;

  switch (arg)
  {
    case ARG_ALWAYS_BATCH:
         /* if we have downloaded at least one article, then batch up
          * even on errors
          */
	 master->always_batch = TRUE;
	 break;
    /* batch file implies MultiFile mode */
    case ARG_BATCH_INN:
         master->batch     = BATCH_INNXMIT;
	 master->MultiFile = TRUE;
	 master->batchfile = argv[0];
	 break;
    case ARG_BATCH_RNEWS:
         master->batch     = BATCH_RNEWS;
	 master->MultiFile = TRUE;
	 master->batchfile = argv[0];
	 break;
    case ARG_BATCH_LMOVE:
         master->batch     = BATCH_LMOVE;
	 master->MultiFile = TRUE;
	 master->batchfile = argv[0];
	 break;
    case ARG_BATCH_INNFEED:
         master->batch     = BATCH_INNFEED;
	 master->MultiFile = TRUE;
	 master->batchfile = argv[0];
	 break;
    case ARG_BATCH_POST:
         master->batch     = BATCH_LIHAVE;
	 master->MultiFile = TRUE;
	 master->batchfile = N_POSTFILE;
	 break;
    case ARG_CLEANUP:        /* cleanup option */
	 master->cleanup = TRUE;
	 break;
    case ARG_DIR_TEMP:
	 full_path (FP_SET, FP_TMPDIR, argv[0]);
	 break;
    case ARG_DIR_DATA:
	 full_path (FP_SET, FP_DATADIR, argv[0]);
	 break;
    case ARG_DIR_MSGS:
	 full_path (FP_SET, FP_MSGDIR, argv[0]);
	 break;
    case ARG_DEF_ERRLOG:     /* use default error log path */
	 error_log (ERRLOG_SET_FILE, ERROR_LOG, NULL);
	 master->errlog = ERROR_LOG;
	 break;
    case ARG_HOST:           /* host name */
	 master->host = argv[0];
	 break;
    case ARG_NO_POSTFIX:     /* kill files don't use the postfix */
	 master->kill_ignore_postfix = TRUE;
	 break;
    case ARG_LANGUAGE:       /* load language support */
	 master->phrases = argv[0];
	 break;
    case ARG_MULTIFILE:
	 master->MultiFile = TRUE;
	 break;
    case ARG_POSTFIX:
	 full_path (FP_SET_POSTFIX, FP_NONE, argv[0]);
	 break;
    case ARG_QUIET:          /* don't display BPS and message count */
	 master->quiet = TRUE;
	 break;
    case ARG_RNEWSSIZE:
	 master->rnews_size = atol (argv[0]);
	 break;
    case ARG_DEF_STATLOG:    /* use default status log name */
	 master->status_file_name = STATUS_LOG;
	 break;
    case ARG_WAIT_SIG:       /* wait (sleep) x seconds between every y articles IF we get a signal */
         master->sig_pause_time   = atoi (argv[0]);
	 master->sig_pause_nrmsgs = atoi (argv[1]);
	 break;
    case ARG_ACTIVE:
	 master->do_active = TRUE;
	 break;
    case ARG_RECONNECT:      /* how often do we drop and reconnect to battle INNS's LIKE_PULLER=DONT */
	 master->reconnect_nr = atoi (argv[0]);
	 break;
    case ARG_DEBUG:          /* debug */
	 master->debug = TRUE;
	 break;
    case ARG_ERRLOG:         /* error log path */
	 error_log (ERRLOG_SET_FILE, argv[0], NULL);
	 master->errlog = argv[0];
	 break;
    case ARG_HISTORY:
	 master->do_chkhistory = FALSE;
	 break;
    case ARG_KILLFILE:
	 master->do_killfile = FALSE;
	 break;
    case ARG_KLOG_NONE:
	 master->killfile_log = KILL_LOG_NONE;
	 break;
    case ARG_KLOG_SHORT:
	 master->killfile_log = KILL_LOG_SHORT;
	 break;
    case ARG_KLOG_LONG:
	 master->killfile_log = KILL_LOG_LONG;
	 break;
    case ARG_MODEREADER:     /* mode reader */
	 master->do_modereader = TRUE;
	 break;
    case ARG_PORTNR:         /* override default portnr */
	 master->portnr = atoi (argv[0]);
	 break;
    case ARG_PASSWD:         /* passwd */
	 master->passwd = argv[0];
	 break;
    case ARG_RESCAN:         /* don't rescan on restart */
	 master->rescan = FALSE;
	 break;
    case ARG_STATLOG:        /* status log path */
	 master->status_file_name = argv[0];
	 break;
    case ARG_USERID:         /* userid */
	 master->userid = argv[0];
	 break;
    case ARG_VERSION:        /* show version number */
	 error_log (ERRLOG_REPORT, "Suck version %v1%\n", SUCK_VERSION);
	 retval = RETVAL_VERNR;	/* so we don't do anything else */
	 break;
    case ARG_WAIT:           /* wait (sleep) x seconds between every y articles */
         master->pause_time   = atoi (argv[0]);
	 master->pause_nrmsgs = atoi (argv[1]);
	 break;
    case ARG_LOCALHOST:
	 master->localhost = argv[0];
	 break;
    case ARG_NRMODE:
	 master->nrmode = TRUE;
	 break;
    case ARG_AUTOAUTH:
	 master->auto_auth = TRUE;
	 break;
    case ARG_NODEDUPE:
	 master->no_dedupe = TRUE;
	 break;
    case ARG_READACTIVE:
	 master->activefile = argv[0];
	 break;
    case ARG_PREBATCH:
	 master->prebatch = TRUE;
	 break;
    case ARG_SKIP_ON_RESTART:
	 master->skip_on_restart = TRUE;
	 break;
    case ARG_KLOG_NAME:
	 master->kill_log_name = argv[0];
	 break;
#ifdef TIMEOUT
    case ARG_TIMEOUT:
	 TimeOut = atoi (argv[0]);
	 break;
#endif
  }
  return (retval);
}

/*
 * authenticate when receive a 480
 */
int do_authenticate (PMaster master)
{
  int   len, nr, retval = RETVAL_OK;
  char *resp, buf[MAXLINLEN];

  /* we must do authorization */
  sprintf (buf, "AUTHINFO USER %s\r\n", master->userid);
  if (master->debug == TRUE)
    do_debug ("sending command: %s", buf);

  sputline (master->sockfd, buf);
  len = sgetline (master->sockfd, &resp);
  if (len < 0)
    retval = RETVAL_ERROR;

  else
  {
    if (master->debug == TRUE)
      do_debug ("got answer: %s", resp);

    TimerFunc (TIMER_ADDBYTES, len, NULL);

    number (resp, &nr);
    if (nr == 480)
    {
      /* This is because of the pipelining code.
       * We get the second need auth.
       * Just ignore it and get the next line which should be the 381 we need
       */
      len = sgetline (master->sockfd, &resp);
      TimerFunc (TIMER_ADDBYTES, len, NULL);

      number (resp, &nr);
    }

    if (nr != 381)
    {
      error_log (ERRLOG_REPORT, suck_phrases[27], resp, NULL);
      retval = RETVAL_NOAUTH;
    }
    else
    {
      sprintf (buf, "AUTHINFO PASS %s\r\n", master->passwd);
      sputline (master->sockfd, buf);
      if (master->debug == TRUE)
        do_debug ("sending command: %s", buf);

      len = sgetline (master->sockfd, &resp);
      if (len < 0)
        retval = RETVAL_ERROR;

      else
      {
	if (master->debug == TRUE)
          do_debug ("got answer: %s", resp);

	TimerFunc (TIMER_ADDBYTES, len, NULL);

	number (resp, &nr);
	switch (nr)
        {
          case 281:          /* bingo */
	       retval = RETVAL_OK;
	       break;
          case 502:          /* permission denied */
	       retval = RETVAL_NOAUTH;
	       error_log (ERRLOG_REPORT, suck_phrases[28], NULL);
	       break;
          default:           /* wacko error */
	       error_log (ERRLOG_REPORT, suck_phrases[27], resp, NULL);
	       retval = RETVAL_NOAUTH;
	       break;
        }
      }
    }
  }
  return (retval);
}

/*
 * The strings in this routine is the only one not in the arrays, since
 * we are in the middle of reading the arrays, and they may or may not
 * be valid.
 */
void load_phrases (PMaster master)
{
  int   error = TRUE;
  FILE *fpi;
  char  buf[MAXLINLEN];

  if (master->phrases)
  {
    fpi = fopen (master->phrases, "r");
    if (!fpi)
       MyPerror (master->phrases);
    else
    {
      fgets (buf, MAXLINLEN, fpi);
      if (strncmp (buf, SUCK_VERSION, strlen (SUCK_VERSION)))
        error_log (ERRLOG_REPORT, "Invalid Phrase File, wrong version\n", NULL);

      else if ((both_phrases = read_array (fpi, NR_BOTH_PHRASES, TRUE)) != NULL)
      {
	read_array (fpi, NR_RPOST_PHRASES, FALSE);	/* skip these */
	read_array (fpi, NR_TEST_PHRASES, FALSE);
        if ((suck_phrases   = read_array (fpi, NR_SUCK_PHRASES,  TRUE)) != NULL &&
            (timer_phrases  = read_array (fpi, NR_TIMER_PHRASES, TRUE)) &&
            (chkh_phrases   = read_array (fpi, NR_CHKH_PHRASES,  TRUE)) &&
            (dedupe_phrases = read_array (fpi, NR_DEDUPE_PHRASES,TRUE)) &&
            (killf_reasons  = read_array (fpi, NR_KILLF_REASONS, TRUE)) &&
            (killf_phrases  = read_array (fpi, NR_KILLF_PHRASES, TRUE)) &&
            (sucku_phrases  = read_array (fpi, NR_SUCKU_PHRASES, TRUE)) &&
	    (read_array (fpi, NR_LMOVE_PHRASES, FALSE) == NULL) &&	/* skip this one */
            (active_phrases = read_array (fpi, NR_ACTIVE_PHRASES,TRUE)) &&
            (batch_phrases  = read_array (fpi, NR_BATCH_PHRASES, TRUE)))
          error = FALSE;
      }
    }
    fclose (fpi);
    if (error == TRUE)
    {
      /* reset back to default */
      error_log (ERRLOG_REPORT, "Using default Language phrases\n", NULL);
      both_phrases   = default_both_phrases;
      suck_phrases   = default_suck_phrases;
      timer_phrases  = default_timer_phrases;
      chkh_phrases   = default_chkh_phrases;
      dedupe_phrases = default_dedupe_phrases;
      killf_reasons  = default_killf_reasons;
      killf_phrases  = default_killf_phrases;
      killp_phrases  = default_killp_phrases;
      sucku_phrases  = default_sucku_phrases;
      active_phrases = default_active_phrases;
      batch_phrases  = default_batch_phrases;
    }
  }
}


void free_phrases (void)
{
  /* free up the memory alloced in load_phrases()
   */
  if (both_phrases != default_both_phrases)
    free_array (NR_BOTH_PHRASES, both_phrases);

  if (suck_phrases != default_suck_phrases)
    free_array (NR_SUCK_PHRASES, suck_phrases);

  if (timer_phrases != default_timer_phrases)
    free_array (NR_TIMER_PHRASES, timer_phrases);

  if (chkh_phrases != default_chkh_phrases)
    free_array (NR_CHKH_PHRASES, chkh_phrases);

  if (dedupe_phrases != default_dedupe_phrases)
    free_array (NR_DEDUPE_PHRASES, dedupe_phrases);

  if (killf_reasons != default_killf_reasons)
    free_array (NR_KILLF_REASONS, killf_reasons);

  if (killf_phrases != default_killf_phrases)
    free_array (NR_KILLF_PHRASES, killf_phrases);

  if (killp_phrases != default_killp_phrases)
    free_array (NR_KILLP_PHRASES, killp_phrases);

  if (sucku_phrases != default_sucku_phrases)
    free_array (NR_SUCKU_PHRASES, sucku_phrases);

  if (active_phrases != default_active_phrases)
    free_array (NR_ACTIVE_PHRASES, active_phrases);

  if (batch_phrases != default_batch_phrases)
    free_array (NR_BATCH_PHRASES, batch_phrases);
}
