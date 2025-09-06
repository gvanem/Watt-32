#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>

#include "suck.h"
#include "config.h"
#include "both.h"
#include "utils.h"
#include "phrases.h"

/*
 * Check if directory exists, if not, try to create it.
 * Return TRUE if made/exists and can write to it
 */
int checkdir (const char *dirname)
{   
  struct stat buf;
  int    retval = TRUE;

  if (stat(dirname, &buf) == 0)
  {
    /* it exists */
    if (!S_ISDIR (buf.st_mode))
    {
      error_log (ERRLOG_REPORT, sucku_phrases[0], dirname, NULL);
      retval = FALSE;
    }
    else if (access (dirname, W_OK))
    {
      error_log (ERRLOG_REPORT, sucku_phrases[1], dirname, NULL);
      retval = FALSE;
    }
  }
  else if (errno == ENOENT)
  {
    /* doesn't exist, make it */
    if (mkdir (dirname, S_IRUSR|S_IWUSR|S_IXUSR))
    {
      MyPerror (dirname);
      retval = FALSE;
    }
  }
  else
  {
    /* huh? */
    MyPerror (dirname);
    retval = FALSE;
  }
  return (retval);
}

/*
 * if which = FP_SET, store directory in array
 * = FP_GET, retrieve directory and add file name
 * = FP_GET_NOPOSTFIX, get without appending postfix
 * = FP_GET_POSTFIX, get ONLY the postfix
 * retval     FP_SET, if valid dir, the dir, else NULL
 * FP_GET, full path
 * FP_GET_POSTFIX, only the postfix
 */
const char *full_path (int which, int dir, const char *fname)
{
  static char dirs[FP_NR_DIRS][PATH_MAX + 1] = {
              N_TMPDIR, N_DATADIR, N_MSGDIR
            };
  static char path[PATH_MAX + 1];
  static char postfix[PATH_MAX + 1] = { "\0" };

  char *retptr = NULL;
  int   ok, i;

  switch (which)
  {
    case FP_GET:
    case FP_GET_NOPOSTFIX:
	 /* put the right directory as the first part of the path */
	 strcpy (path, dirs[dir]);

         /* if it contains a dit, then change to full path */
	 if (path[0] == '.')
	 {
	   path[0] = '\0';
           if (!getcwd (path, PATH_MAX))
	   {
	     MyPerror (path);
             /* restore current path and pray it will work later
              */
	     strcpy (path, dirs[dir]);
	   }
	   else
	   {
             /* tack on remainder of path
              * the 1 so skip .
              */
	     strcat (path, &dirs[dir][1]);
	   }
	 }
         if (fname && fname[0] != '\0')
	 {
	   i = strlen (path);
           /* if nothing to put on don't do any of this
            * put on trailing slant bar if needed
            */
           if (path[i-1] != '/' && path[i-1] != '\\')
	   {
	     path[i++] = '/';
	     path[i] = '\0';	/* null terminate it */
	   }
	   /* finally, put on filename */
	   strcpy (&path[i], fname);

	   if (which != FP_GET_NOPOSTFIX && postfix[0] != '\0')
             strcat (path, postfix);
         }
	 retptr = path;
	 break;
    case FP_SET_POSTFIX:
	 ok = TRUE;
	 strncpy (postfix, fname, sizeof (postfix));
	 break;
    case FP_GET_POSTFIX:
	 retptr = postfix;
	 break;
    case FP_SET:
	 ok = TRUE;
	 switch (dir)
         {
           case FP_TMPDIR:
		if (access (fname, W_OK) == -1)
		{
		  MyPerror (fname);
		  ok = FALSE;
		}
		break;
           case FP_DATADIR:
		if (access (fname, R_OK) == -1)
		{
		  MyPerror (fname);
		  ok = FALSE;
		}
		break;
           case FP_MSGDIR:   /* do_nothing, this is checked elsewhere */
		break;
         }
	 if (ok == FALSE)
           retptr = NULL;
         else
	 {
	   strncpy (dirs[dir], fname, sizeof (dirs[0]));
	   retptr = dirs[dir];
	 }
	 break;
  }
  return (retptr);
}

#ifdef LOCKFILE
int do_lockfile (PMaster master)
{
  int   retval = RETVAL_OK;
  pid_t pid;
  const char *lockfile = full_path (FP_GET, FP_TMPDIR, N_LOCKFILE);
  FILE       *f_lock   = fopen (lockfile, "r");

  if (f_lock)
  {
    /* okay, let's try and see if this sucker is truly alive */
    fscanf (f_lock, "%ld", (long *) &pid);
    fclose (f_lock);
    if (pid <= 0)
    {
      error_log (ERRLOG_REPORT, sucku_phrases[2], lockfile, NULL);
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
	print_phrases (master->msgs, sucku_phrases[3], lockfile, NULL);
      }
      else
      {
	error_log (ERRLOG_REPORT, sucku_phrases[4], lockfile, NULL);
	retval = RETVAL_ERROR;
      }
    }
    else
    {
      error_log (ERRLOG_REPORT, sucku_phrases[5], lockfile, NULL);
      retval = RETVAL_ERROR;
    }
  }
  if (retval == RETVAL_OK)
  {
    f_lock = fopen (lockfile, "w");
    if (f_lock)
    {
      fprintf (f_lock, "%ld", (long) getpid ());
      fclose (f_lock);
    }
    else
    {
      error_log (ERRLOG_REPORT, sucku_phrases[6], lockfile, NULL);
      retval = RETVAL_ERROR;
    }
  }
  return (retval);
}
#endif

/* cmp two article id numbers
 * which is <xxxxxxxxxxxxxxxxx@place.org
 * use case sensitive compares up to the @
 * then use case insensitive compares on the organization
 * this is to conform to rfc822
 */
int cmp_msgid (const char *nr1, const char *nr2)
{
  int retval = FALSE;

  if (nr1 && nr2)
  {
    while (*nr1 == *nr2 && *nr1 != '\0' && *nr1 != '@')
    {
      nr1++;
      nr2++;
    }
    if (*nr1 == '@')
    {
      while (tolower(*nr1) == tolower(*nr2) && *nr1)
      {
	nr1++;
	nr2++;
      }
    }
    if (*nr1 == *nr2 && *nr1 == '\0')
      retval = TRUE;
  }
  return (retval);
}

/*
 * move a file from one location to another
 */
int move_file (const char *src, const char *dest)
{
  char   block[MAXLINLEN];
  FILE  *fpi, *fpo;
  int    retval = RETVAL_OK;
  size_t nrread;

  /* first try the easy way */
  if (!src || !dest)
  {
    error_log (ERRLOG_REPORT, sucku_phrases[9], NULL);
    retval = RETVAL_ERROR;
  }
  else if (rename(src,dest))
  {
    if (errno != EXDEV)
    {
      MyPerror (src);
      retval = RETVAL_ERROR;
    }
    else
    {
      /* can't rename, try to copy file */
      fpi = fopen (src, "r");
      if (!fpi)
      {
	MyPerror (src);
	retval = RETVAL_ERROR;
      }
      else if ((fpo = fopen (dest, "w")) == NULL)
      {
	fclose (fpi);
	MyPerror (dest);
	retval = RETVAL_ERROR;
      }
      else
      {
	while (retval == RETVAL_OK && (nrread = fread (block, sizeof (char), MAXLINLEN, fpi)) > 0)
	{
	  if (fwrite (block, sizeof (char), nrread, fpo) != nrread)
	  {
	    error_log (ERRLOG_REPORT, sucku_phrases[7], dest, NULL);
	    retval = RETVAL_ERROR;
	  }
	}
        if (ferror(fpi))
          error_log (ERRLOG_REPORT, sucku_phrases[8], src, NULL);

	fclose (fpi);
	fclose (fpo);
      }
      /* now that copy is successful, nuke it */
      if (retval == RETVAL_OK)
        unlink (src);
    }
  }
  return (retval);
}
