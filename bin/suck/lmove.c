
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>

#include "suck.h"
#include "config.h"
#include "both.h"
#include "phrases.h"

typedef struct {
        char    *group;
        unsigned lownr;
        unsigned highnr;
        char     active;
      } Group, *PGroup;

/* Master Structure */
typedef struct {
        FILE  *msgs;
        int    nrgroups;
        int    debug;
        PGroup groups;
        char   active_file[PATH_MAX + 1];
        const  char *config_file;
        char   basedir[PATH_MAX + 1];
        const  char *msgdir;
        const  char *phrases;
      } _Master, *_PMaster;

#define Master   _Master
#define PMaster  _PMaster

enum {
  LOCKFILE_LOCK,
  LOCKFILE_UNLOCK
};

#define NEWSGROUP         "Newsgroups: "
#define NEWSGROUP_LEN     12
#define GROUP_SEP         ','
#define CONFIG_BASE       "BASE="
#define CONFIG_ACTIVE     "ACTIVE="
#define CONFIG_BASE_LEN   5
#define CONFIG_ACTIVE_LEN 7

/* for phrases stuff
 */
char **both_phrases  = default_both_phrases;
char **lmove_phrases = default_lmove_phrases;

static void   scan_args (PMaster, int, char *[]);
static void   load_phrases (PMaster);
static void   free_phrases (void);
static int    load_active (PMaster);
static int    load_config (PMaster);
static void   free_active (PMaster);
static int    check_dirs (PMaster);
static char  *make_dirname (char *, char *);
static int    move_msgs (PMaster);
static char  *find_groups (PMaster, char *);
static PGroup match_group (PMaster, char *);
static int    rewrite_active (PMaster);
static int    do_lockfile (PMaster, int);

int main (int argc, char **argv)
{
  int    retval = RETVAL_OK;
  Master master;

  /* set up defaults */
  master.msgs        = stdout;
  master.nrgroups    = 0;
  master.debug       = 0;
  master.groups      = NULL;
  master.config_file = N_LMOVE_CONFIG;
  master.phrases     = NULL;
  master.basedir[0]  = '\0';
  master.msgdir      = NULL;
  strcpy (master.active_file, N_LMOVE_ACTIVE);

  scan_args (&master, argc, argv);
  load_phrases (&master);	/* this has to be here so rest prints okay */

  /* have to load this first so get basedir */
  retval = load_config (&master);

  if (master.debug == TRUE)
  {
    do_debug ("master.active_file = %s\n", master.active_file == NULL ? "NULL" : master.active_file);
    do_debug ("master.config_file = %s\n", master.config_file == NULL ? "NULL" : master.config_file);
    do_debug ("master.basedir = %s\n",     master.basedir);
    do_debug ("master.msgdir = %s\n",      master.msgdir  == NULL ? "NULL" : master.msgdir);
    do_debug ("master.phrases = %s\n",     master.phrases == NULL ? "NULL" : master.phrases);
  }
  if (master.basedir[0] == '\0' || master.msgdir[0] == '\0')
    retval = RETVAL_ERROR;

  if (retval == RETVAL_OK)
  {
#ifdef LOCKFILE
    if (do_lockfile (&master, LOCKFILE_LOCK) != RETVAL_OK)
      retval = RETVAL_ERROR;
    else
    {
#endif
      if ((retval = load_active (&master)) == RETVAL_OK)
      {
	/* check to make sure all our directories are there */
	if ((retval = check_dirs (&master)) == RETVAL_OK)
	{
	  retval = move_msgs (&master);
	  if (retval == RETVAL_OK)
            retval = rewrite_active (&master);
        }
      }
      free_active (&master);
#ifdef LOCKFILE
      do_lockfile (&master, LOCKFILE_UNLOCK);
    }
#endif
  }
  free_phrases();
  return (retval);
}

/*
 * move old active to active.old and write active
 */
int rewrite_active (PMaster master)
{
  FILE *fpo;
  char  fname[PATH_MAX + 1];
  int   i, retval = RETVAL_OK;

  sprintf (fname, "%s.old", master->active_file);
  if (rename (master->active_file, fname) != 0)
  {
    MyPerror (fname);
    retval = RETVAL_ERROR;
  }
  else if ((fpo = fopen (master->active_file, "w")) == NULL)
  {
    MyPerror (master->active_file);
    retval = RETVAL_ERROR;
  }
  else
  {
    for (i = 0; i < master->nrgroups && retval == RETVAL_OK; i++)
    {
      if (fprintf (fpo, "%s %u %u %c\n",
                   master->groups[i].group,
                   master->groups[i].highnr, master->groups[i].lownr,
                   master->groups[i].active) <= 0)
      {
	error_log (ERRLOG_REPORT, lmove_phrases[13], master->active_file, NULL);
	retval = RETVAL_ERROR;
      }
    }
    fclose (fpo);
    if (chmod (master->active_file, LMOVE_ACTIVE_MODE) != 0)
       MyPerror (master->active_file);
  }
  return (retval);
}

/*
 * go thru msg dir, and move the messages, based on "Newsgroups: subj line"
 * into newsgroup directory
 */
int move_msgs (PMaster master)
{
  struct dirent *entry;
  struct stat statbuf;
  DIR   *dptr;
  char  *ptr;
  PGroup group;
  char   o_fname[PATH_MAX + 1], n_fname[PATH_MAX + 1];
  int    retval = RETVAL_OK;

  dptr = opendir (master->msgdir);
  if (!dptr)
  {
    MyPerror (master->msgdir);
    retval = RETVAL_ERROR;
  }
  else
  {
    entry = readdir (dptr);
    while (entry)
    {
      sprintf (o_fname, "%s\\%s", master->msgdir, entry->d_name);
      if (stat (o_fname, &statbuf) == 0 && S_ISREG (statbuf.st_mode))
      {
        ptr = find_groups (master, o_fname);
        if (!ptr)
          error_log (ERRLOG_REPORT, lmove_phrases[11], entry->d_name, NULL);

	else if ((group = match_group (master, ptr)) == NULL)
          error_log (ERRLOG_REPORT, lmove_phrases[12], entry->d_name, NULL);

	else
	{
          /* try the move.
           * make destination filename
           */
          sprintf (n_fname, "%s\\%d",
                   make_dirname(master->basedir, group->group), group->highnr);
          if (rename(o_fname, n_fname))
	  {
	    error_log (ERRLOG_REPORT, lmove_phrases[13], o_fname, n_fname);
	    MyPerror ("");
	  }
	  else
            group->highnr++;
        }
      }
    }
    closedir (dptr);
  }
  return (retval);
}

PGroup match_group (PMaster master, char *newsgroups)
{
  PGroup retval = NULL;
  char  *ptr, *eptr, save;
  int    i;

  ptr = eptr = &newsgroups[NEWSGROUP_LEN];	/* skip header */
  do
  {
    /* first find end of this group or NULL */
    while (*eptr != GROUP_SEP && *eptr != '\0')
      eptr++;

    save = *eptr;
    *eptr = '\0';		/* so can return ptr as group name */

    for (i = 0; !retval && i < master->nrgroups; i++)
    {
      if (!strcmp (master->groups[i].group, ptr))
      {
	/* bingo */
	if (master->debug == TRUE)
          do_debug ("Matched group %s\n", ptr);

        retval = &master->groups[i];
      }
    }
    ptr = ++eptr;		/* set up for next group */
  }
  while (save != '\0' && !retval);
  return (retval);
}

char *find_groups (PMaster master, char *fname)
{
  FILE  *fpi;
  char  *retval;
  static char linein[LMOVE_MAXLINLEN + 1];
  int    i;

  retval = NULL;
  fpi    = fopen (fname, "r");
  if (!fpi)
     MyPerror (fname);
  else
  {
    while (!retval && fgets(linein,LMOVE_MAXLINLEN,fpi))
    {
      if (!strncmp (linein, NEWSGROUP, NEWSGROUP_LEN))
      {
	retval = linein;
	/* now strip off NL off of EOL */
	i = strlen (linein) - 1;
	if (linein[i] == '\n')
            linein[i] = '\0';
      }
    }
    fclose (fpi);
  }
  return (retval);
}


int load_config (PMaster master)
{
  int   i, retval = RETVAL_OK;
  char  linein[MAXLINLEN + 1];
  FILE *fpi = fopen (master->config_file, "r");

  if (!fpi)
  {
    MyPerror (master->config_file);
    retval = RETVAL_ERROR;
  }
  else
  {
    while (fgets(linein,MAXLINLEN,fpi))
    {
      /* strip the nl */
      i = strlen (linein);
      if (linein[i-1] == '\n')
          linein[i-1] = '\0';

      if (!strncmp (linein, CONFIG_BASE, CONFIG_BASE_LEN))
        strcpy (master->basedir, &linein[CONFIG_BASE_LEN]);

      else if (!strncmp (linein, CONFIG_ACTIVE, CONFIG_ACTIVE_LEN))
        strcpy (master->active_file, &linein[CONFIG_ACTIVE_LEN]);

      else
        error_log (ERRLOG_REPORT, lmove_phrases[20], NULL);
    }
    fclose (fpi);
  }
  return (retval);
}

int load_active (PMaster master)
{
  int      i, retval = RETVAL_OK;
  unsigned lownr, highnr;
  FILE    *fpi;
  char     linein[MAXLINLEN + 1], group[MAXLINLEN + 1], active_char;

  /* each line in active is in the form
   * "group name high_msg_nr low_msg_nr  active_char"
   */
  fpi = fopen (master->active_file, "r");
  if (!fpi)
  {
    MyPerror (master->active_file);
    retval = RETVAL_ERROR;
  }
  else
  {
    /* pass one, count the number of valid entries
     * and get the BASE dir and MSG dir entries
     */
    while ((i = fscanf(fpi,"%s %u %u %c",linein,&highnr,&lownr,&active_char)) != EOF)
    {
      if (i == 4)
           master->nrgroups++;
      else error_log (ERRLOG_REPORT, lmove_phrases[9], linein, NULL);
    }
    if (master->debug == TRUE)
      do_debug ("Nr of groups in active = %d\n", master->nrgroups);

    fseek (fpi, 0L, SEEK_SET);	/* rewind for pass two */

    /* pass two, put them into the master->groups array */
    master->groups = calloc (master->nrgroups, sizeof(Group));
    if (!master->groups)
    {
      error_log (ERRLOG_REPORT, lmove_phrases[8], NULL);
      retval = RETVAL_ERROR;
    }
    else
    {
      i = 0;
      while (retval == RETVAL_OK && fgets(linein, MAXLINLEN, fpi))
      {
        if (sscanf (linein,"%s %u %u %c",group,&highnr,&lownr,&active_char) == 4)
	{
	  if (highnr == 0)
              highnr = 1;          /* so we start at 1 */
          master->groups[i].lownr  = lownr;
	  master->groups[i].highnr = highnr;
	  master->groups[i].active = active_char;

          master->groups[i].group = strdup (group);
          if (!master->groups[i].group)
	  {
	    error_log (ERRLOG_REPORT, lmove_phrases[8], NULL);
	    retval = RETVAL_ERROR;
	  }
	  else
	  {
	    if (master->debug == TRUE)
              do_debug ("Group %d = %s\n", i, group);
	    i++;
	  }
	}
      }
    }
    fclose (fpi);
  }
  return (retval);
}


void free_active (PMaster master)
{
  int i;

  for (i = 0; i < master->nrgroups; i++)
  {
    if (master->groups[i].group)
      free (master->groups[i].group);
  }
  free (master->groups);
  master->groups = NULL;
}

/*
 * check to make sure the base, msg, and all group dirs are available
 */
int check_dirs (PMaster master)
{
  struct stat statbuf;
  int    i, y, retval = RETVAL_OK;
  char  *ptr, path[PATH_MAX + 1];

  if (stat (master->basedir, &statbuf) != 0 || !S_ISDIR (statbuf.st_mode))
  {
    error_log (ERRLOG_REPORT, lmove_phrases[10], master->basedir);
    retval = RETVAL_ERROR;
  }
  else if (stat (master->msgdir, &statbuf) != 0 || !S_ISDIR (statbuf.st_mode))
  {
    error_log (ERRLOG_REPORT, lmove_phrases[10], master->msgdir);
    retval = RETVAL_ERROR;
  }
  else
  {
    for (i = 0; i < master->nrgroups && retval == RETVAL_OK; i++)
    {
      ptr = make_dirname (master->basedir, master->groups[i].group);
      if (master->debug == TRUE)
        do_debug ("Checking dir %s\n", ptr);

      if (stat (ptr, &statbuf) != 0 && errno == ENOENT)
      {
        /* it don't exist. try to make it
         * lets work our way down and try to make all dirs
         */
        sprintf (path, "%s\\", master->basedir);
        y   = strlen (path);      /* start at end of string */
	ptr = master->groups[i].group;
	while (retval == RETVAL_OK && *ptr != '\0')
	{
	  /* copy til next dit or end */
	  while (*ptr != '.' && *ptr != '\0')
	  {
	    path[y] = *ptr;
	    y++;
	    ptr++;
	  }
	  path[y] = '\0';	/* just to be on safe side */
	  if (master->debug == TRUE)
            do_debug ("Trying to make dir %s : mode %o\n", path, LMOVE_DEFAULT_DIR_MODE);

	  umask (0);		/* so mode defined is mode we actually get */
	  if (mkdir (path, LMOVE_DEFAULT_DIR_MODE) != 0 && errno != EEXIST)
	  {
	    MyPerror (path);
	    retval = RETVAL_ERROR;
	  }
	  if (*ptr != '\0')
	  {
	    /* move on to next */
	    path[y++] = '/';
	    ptr++;
	  }
	}
      }
      /* one final check */
      if (retval == RETVAL_OK)
      {
	/* have to redo stat, since we might have made dirs above */
	ptr = make_dirname (master->basedir, master->groups[i].group);
	if (stat (ptr, &statbuf) != 0 || !S_ISDIR (statbuf.st_mode))
	{
	  error_log (ERRLOG_REPORT, lmove_phrases[10], ptr, NULL);
	  retval = RETVAL_ERROR;
	}
      }
    }
  }
  return (retval);
}


void scan_args (PMaster master, int argc, char **argv)
{
  int loop;

  for (loop = 1; loop < argc; loop++)
  {
    if (argv[loop][0] != '-' || argv[loop][2] != '\0')
      error_log (ERRLOG_REPORT, lmove_phrases[0], argv[loop], NULL);
    else
    {
      switch (argv[loop][1])
      {
        case 'e':            /* use default error log path */
	     error_log (ERRLOG_SET_FILE, ERROR_LOG, NULL);
	     break;
        case 'E':            /* error log path */
	     if (loop + 1 == argc)
                  error_log (ERRLOG_REPORT, "%s\n", lmove_phrases[1], NULL);
             else error_log (ERRLOG_SET_FILE, argv[++loop]);
             break;
        case 'l':            /* language  phrase file */
	     if (loop + 1 == argc)
                  error_log (ERRLOG_REPORT, lmove_phrases[4], NULL);
             else master->phrases = argv[++loop];
             break;
        case 'D':            /* debug */
	     master->debug = TRUE;
	     break;
        case 'a':            /* active file */
	     if (loop + 1 == argc)
                  error_log (ERRLOG_REPORT, lmove_phrases[6], NULL);
             else strcpy (master->active_file, argv[++loop]);
             break;
        case 'c':            /* config file */
	     if (loop + 1 == argc)
                  error_log (ERRLOG_REPORT, lmove_phrases[19], NULL);
             else master->config_file = argv[++loop];
             break;
        case 'd':            /* article directory */
	     if (loop + 1 == argc)
                  error_log (ERRLOG_REPORT, lmove_phrases[7], NULL);
             else master->msgdir = argv[++loop];
             break;
        default:
	     error_log (ERRLOG_REPORT, lmove_phrases[0], argv[loop], NULL);
	     break;
      }
    }
  }
}


char *make_dirname (char *base, char *group)
{
  static char path[PATH_MAX + 1];
  int    i;
  char  *ptr;

  /* take base dir & a group name, change all "." in group to "\"
   * and return the full path
   */
  i = strlen (base);
  strcpy (path, base);

  ptr = &path[i];
  sprintf (path, "%s\\%s", base, group);
  while (*ptr != '\0')
  {
    if (*ptr == '.')
        *ptr = '\\';
    ptr++;
  }
  return (path);
}

#ifdef LOCKFILE
/*--------------------------------------------------------------*/
int do_lockfile (PMaster master, int option)
{
  static char lockfile[PATH_MAX + 1];
  int    retval;
  FILE  *f_lock;
  pid_t  pid;

  retval = RETVAL_OK;
  if (option == LOCKFILE_UNLOCK)
    unlink (lockfile);
  else
  {
    sprintf (lockfile, "%s/%s", master->basedir, N_LMOVE_LOCKFILE);
    f_lock = fopen (lockfile, "r");
    if (f_lock)
    {
      /* okay, let's try and see if this sucker is truly alive
       */
      fscanf (f_lock, "%ld", (long *) &pid);
      fclose (f_lock);
      if (pid <= 0)
      {
	error_log (ERRLOG_REPORT, lmove_phrases[14], lockfile, NULL);
	retval = RETVAL_ERROR;
      }
      /* this next technique borrowed from pppd, sys-linux.c (ppp2.2.0c)
       * if the pid doesn't exist (can't send any signal to it), then try
       * to remove the stale PID file so can recreate it.
       */
      else if (kill (pid, 0) == -1 && errno == ESRCH)
      {
	/* no pid found */
	if (unlink (lockfile) == 0)
	{
	  /* this isn't error so don't put in error log */
	  print_phrases (master->msgs, lmove_phrases[15], lockfile, NULL);
	}
	else
	{
	  error_log (ERRLOG_REPORT, lmove_phrases[16], lockfile, NULL);
	  retval = RETVAL_ERROR;
	}
      }
      else
      {
	error_log (ERRLOG_REPORT, lmove_phrases[17], lockfile, NULL);
	retval = RETVAL_ERROR;
      }
    }
    if (retval == RETVAL_OK)
    {
      f_lock = fopen (lockfile, "w");
      if (f_lock)
      {
        fprintf (f_lock, "%ld", (long) getpid());
	fclose (f_lock);
      }
      else
      {
	error_log (ERRLOG_REPORT, lmove_phrases[18], lockfile, NULL);
	retval = RETVAL_ERROR;
      }
    }
  }
  return (retval);
}
#endif

/*
 * THE strings in this routine is the only one not in the arrays, since
 * we are in the middle of reading the arrays, and they may or may not be
 * valid.
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
      if (strncmp (buf, SUCK_VERSION, strlen(SUCK_VERSION)))
        error_log (ERRLOG_REPORT, "Invalid Phrase File, wrong version\n", NULL);
      else if ((both_phrases = read_array (fpi, NR_BOTH_PHRASES, TRUE)) != NULL)
      {
	read_array (fpi, NR_RPOST_PHRASES, FALSE);	/* skip these */
        read_array (fpi, NR_TEST_PHRASES,  FALSE);
        read_array (fpi, NR_SUCK_PHRASES,  FALSE);
	read_array (fpi, NR_TIMER_PHRASES, FALSE);
        read_array (fpi, NR_CHKH_PHRASES,  FALSE);
        read_array (fpi, NR_DEDUPE_PHRASES,FALSE);
	read_array (fpi, NR_KILLF_REASONS, FALSE);
	read_array (fpi, NR_KILLF_PHRASES, FALSE);
	read_array (fpi, NR_KILLP_PHRASES, FALSE);
        lmove_phrases = read_array (fpi, NR_LMOVE_PHRASES, TRUE);
        if (lmove_phrases)
          error = FALSE;
      }
    }
    fclose (fpi);
    if (error == TRUE)
    {
      /* reset back to default */
      error_log (ERRLOG_REPORT, "Using default Language phrases\n", NULL);
      lmove_phrases = default_lmove_phrases;
      both_phrases = default_both_phrases;
    }
  }
}

void free_phrases (void)
{
  /* free up the memory alloced in load_phrases() */
  if (lmove_phrases != default_lmove_phrases)
    free_array (NR_LMOVE_PHRASES, lmove_phrases);

  if (both_phrases != default_both_phrases)
    free_array (NR_BOTH_PHRASES, both_phrases);
}
