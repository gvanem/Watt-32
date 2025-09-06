
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>

#ifndef WEXITSTATUS
#define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >>8)
#endif

#ifndef WIFEXITED
#define WIFEXITED(stat_val) ((stat_val) & 255) == 0)
#endif

#include "suck.h"
#include "config.h"
#include "both.h"
#include "utils.h"
#include "killfile.h"
#include "phrases.h"

/*
 * this file contains all subroutines pertaining to forking a killfile
 * program, calling the program for each message and checking the results.
 * It also must handle gracefully the child dieing, etc.
 */

static int find_in_path (char *);

/*
 * take the char string args, parse it to get the program to check to see
 * if it exists, then try to execl, and set up pipe to it.  If
 * successful, return TRUE, else return FALSE.
 */
int killprg_forkit (PKillStruct mkill, char *args, int debug)
{
  volatile int i;
  int   retval = FALSE;
  int   newstdin [2];
  int   newstdout[2];
  char *prg[KILLPRG_MAXARGS + 1];
  char *myargs = NULL;
  char *ptr, *nptr;

  /* initialize the master struct */
  mkill->child.Stdin = mkill->child.Stdout = -1;
  mkill->child.Pid   = -1;

  /* do this so can muck around with string with messing up original */
  myargs = strdup (args);
  if (!myargs)
    error_log (ERRLOG_REPORT, killp_phrases[0], NULL);
  else
  {
    i = strlen (myargs);
    /* strip off nl */
    if (myargs[i-1] == '\n')
        myargs[i-1] = '\0';

    /* build array of pointers to string, by finding spaces and
     * replacing them with \0
     */
    i = 0;
    ptr = myargs;
    do
    {
      prg[i++] = ptr;
      nptr = strchr (ptr, ' ');
      if (nptr)
      {
	*nptr = '\0';
	nptr++;
      }
      ptr = nptr;
    }
    while (ptr && i < KILLPRG_MAXARGS);
    prg[i] = NULL;		/* must do this for execv */
  }

  if (debug == TRUE)
    do_debug ("Prg = %s, %d args\n", prg[0], i);

  if (prg[0] && find_in_path(prg[0]) == TRUE)
  {
    /* okay it exists, fork, execl, etc
     * first set up our pipes
     */
    if (pipe (newstdin) == 0)
    {
      if (pipe (newstdout) == 0)
        retval = TRUE;
      else
      {
	close (newstdin[0]);
	close (newstdin[1]);
      }
    }
    if (retval == FALSE)
      MyPerror (killp_phrases[1]);

    else
    {
      mkill->child.Pid = fork();
      if (mkill->child.Pid == 0)
      {
	/* in child */
	close (newstdin[1]);	/* will only be reading from this */
	close (newstdout[0]);	/* will only be writing to this */
	close (0);
	close (1);		/* just to be on safe side */
	dup2 (newstdin[0], 0);	/* copy new read to stdin */
	dup2 (newstdout[1], 1);	/* copy new write to stdout */
	execvp (prg[0], prg);
      }
      else if (mkill->child.Pid == -1)
      {
	/* whoops */
	MyPerror (killp_phrases[3]);
	retval = FALSE;
	/* close down all the pipes */
	close (newstdin[0]);
	close (newstdin[1]);
	close (newstdout[0]);
	close (newstdout[1]);
      }
      else
      {
	/* parent */
	close (newstdin[0]);	/* will only be writing to new stdin */
	close (newstdout[1]);	/* will only be reading from new stdout */
	/* so subroutine can read/write to child */
	mkill->child.Stdin = newstdin[1];
	mkill->child.Stdout = newstdout[0];
	mkill->killfunc = chk_msg_kill_fork;	/* point to our subroutine */
      }
    }

  }
  if (myargs)
    free (myargs);
  return (retval);
}

/*
 * parse the path and search thru it for the program to see if it exists
 */
int find_in_path (char *prg)
{
  int    retval = FALSE, len;
  char   fullpath[PATH_MAX + 1], *ptr, *mypath, *nptr;
  struct stat buf;

  /* if prg has a slash/back-slash in it, its an absolute/relative path, no
   * search done
   */
  if (!strchr(prg,'\\') && !strchr(prg,'/'))
  {
    ptr = getenv ("PATH");
    if (ptr)
    {
      len = strlen (ptr) + 1;
      /* now have to copy the environment, since I can't touch the ptr */
      mypath = strdup (ptr);
      if (!mypath)
        error_log (ERRLOG_REPORT, "%s %s", killp_phrases[2], NULL);
      else
      {
	ptr = mypath;		/* start us off */
	do
	{
	  nptr = strchr (ptr, PATH_SEPARATOR);
          if (nptr)
	  {
	    *nptr = '\0';	/* null terminate the current one */
	    nptr++;		/* move past it */
	  }
	  /* build the fullpath name */
	  strcpy (fullpath, ptr);
          if (ptr[strlen (ptr)-1] != '\\')
            strcat (fullpath, "\\");

          strcat (fullpath, prg);
	  /* okay now have path, check to see if it exists */
          if (!stat(fullpath,&buf))
	  {
	    if (S_ISREG (buf.st_mode))
               retval = TRUE;
          }
	  /* go onto next one */
	  ptr = nptr;
	}
        while (ptr && retval == FALSE);
	free (mypath);
      }
    }
  }
  /* last ditch try, in case of a relative path or current directory */
  if (retval == FALSE &&
      !stat(full_path(FP_GET_NOPOSTFIX,FP_NONE,prg), &buf) &&
      S_ISREG (buf.st_mode))
     retval = TRUE;

  if (retval == FALSE)
    error_log (ERRLOG_REPORT, killp_phrases[3], prg, NULL);
  return (retval);
}

/*
 * write the header to ChildStdin, read from ChildStdout
 * read 0 if get whole message, 1 if skip
 * return TRUE if kill article, FALSE if keep
 */
int chk_msg_kill_fork (PMaster master, PKillStruct pkill,
                       char *header, int headerlen)
{
  int   retval = FALSE;
  char  keepyn, keep[4], len[KILLPRG_LENGTHLEN + 1];
  pid_t waitstatus = 0;

  /* first make sure our child is alive WNOHANG so if its alive,
   * immed return
   */
  keepyn = -1;			/* our error status */

//waitstatus = waitpid (pkill->child.Pid, &status, WNOHANG);
  if (!waitstatus)
  {				/* child alive */
    if (master->debug == TRUE)
      do_debug ("Writing to child\n");

    /* first write the length down */
    sprintf (len, "%-*d\n", KILLPRG_LENGTHLEN - 1, headerlen);
    if (write (pkill->child.Stdin, len, KILLPRG_LENGTHLEN) <= 0)
      error_log (ERRLOG_REPORT, killp_phrases[4], NULL);

    /* then write the actual header */
    else if (write (pkill->child.Stdin, header, headerlen) <= 0)
      error_log (ERRLOG_REPORT, killp_phrases[4], NULL);

    /* read the result back */
    else
    {
      if (master->debug == TRUE)
        do_debug ("Reading from child\n");

      if (read (pkill->child.Stdout, &keep, 2) <= 0)
        error_log (ERRLOG_REPORT, killp_phrases[5], NULL);
      else
      {
	if (master->debug == TRUE)
	{
	  keep[2] = '\0';	/* just in case */
	  do_debug ("killprg: read '%s'\n", keep);
	}
	keepyn = keep[0] - '0';	/* convert ascii to 0/1 */
      }
    }
  }
  else if (waitstatus == -1)
       MyPerror (killp_phrases[6]);
  else error_log (ERRLOG_REPORT, killp_phrases[7]);  /* child died */

  switch (keepyn)
  {
    case 0:
	 retval = FALSE;
	 break;
    case 1:
	 retval = TRUE;
	 if (pkill->logyn != KILL_LOG_NONE)
	 {
	   /* log it */
           if (!pkill->logfp)
	   {
             pkill->logfp = fopen (full_path(FP_GET, FP_TMPDIR, master->kill_log_name), "a");
             if (!pkill->logfp)
               MyPerror (killp_phrases[12]);
           }
           if (pkill->logfp)
	   {
	     /* first print our one line reason */
	     print_phrases (pkill->logfp, killp_phrases[13], (master->curr)->msgnr, NULL);
	     if (pkill->logyn == KILL_LOG_LONG)
	     {
	       /* print the header as well */
	       print_phrases (pkill->logfp, "%v1%", header, NULL);
	     }
	   }
	 }
	 if (master->debug == TRUE)
            do_debug ("Kill program killed: %s", header);
         break;
    default:         /* error in child don't use anymore */
	 retval = FALSE;
	 pkill->killfunc = chk_msg_kill;	/* back to normal method */
  }
  return (retval);
}

/*
 * tell it to close down by writing a length of zero
 * then wait for the pid to close down
 */
void killprg_closeit (PKillStruct master)
{
#ifndef WATT32
  char len[KILLPRG_LENGTHLEN + 1];

  sprintf (len, "%-*d\n", KILLPRG_LENGTHLEN - 1, 0);
  write (master->child.Stdin, len, KILLPRG_LENGTHLEN);
  waitpid (master->child.Pid, NULL, 0);
#endif
}
