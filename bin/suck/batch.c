#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "suck.h"
#include "both.h"
#include "batch.h"
#include "utils.h"
#include "phrases.h"
#include "active.h"
#include "timer.h"

static int post_one_msg (PMaster, int, char *);

/*
 * Build batch file that contains article listing
 * needed by innxmit
 * this is done by searching thru MSGDIR to find files
 * which match our naming convention
 */
int do_innbatch (PMaster master)
{
  int    i, retval = RETVAL_OK;
  FILE  *fptr;
  const  char *tmp;
  DIR   *dptr;
  struct dirent *entry;

  print_phrases (master->msgs, batch_phrases[3], NULL);

  fptr = fopen (master->batchfile, "w");
  if (!fptr)
  {
    MyPerror (master->batchfile);
    retval = RETVAL_ERROR;
  }
  else if ((dptr = opendir (full_path (FP_GET, FP_MSGDIR, ""))) == NULL)
  {
    MyPerror (full_path (FP_GET, FP_MSGDIR, ""));
    retval = RETVAL_ERROR;
    fclose (fptr);
  }
  else
  {
    tmp = full_path (FP_GET_POSTFIX, 0, "");	/* this will be string we search for */
    /* look for entries which have our postfix */
    while ((entry = readdir (dptr)) != NULL && retval == RETVAL_OK)
    {
      /* ignore hidden files */
      if (entry->d_name[0] != '.' && strstr (entry->d_name, tmp))
      {
	i = fprintf (fptr, "%s\n", full_path (FP_GET_NOPOSTFIX, FP_MSGDIR, entry->d_name));
	if (i <= 0)
	{
	  retval = RETVAL_ERROR;
	  MyPerror (master->batchfile);
	}
      }
    }
    fclose (fptr);
    closedir (dptr);
  }
  return (retval);
}

/*
 * build rnews formated file of articles
 * this is done by searching thru MSGDIR to find files
 * which match our naming convention
 */
int do_rnewsbatch (PMaster master)
{  
  /* if max_file_size > 0, then create multiple files up to max file size
   */

  int    i, x, batchnr = 0, retval = RETVAL_OK;
  FILE  *fptr = NULL, *fpin;
  DIR   *dptr;
  const  char *tptr, *tmp;
  char   buf[MAXLINLEN];
  struct dirent *entry;
  struct stat sbuf, tbuf;
  long   cursize = 0L;

  print_phrases (master->msgs, batch_phrases[4], NULL);

  dptr = opendir (full_path(FP_GET, FP_MSGDIR, ""));
  if (!dptr)
  {
    MyPerror (full_path (FP_GET, FP_MSGDIR, ""));
    retval = RETVAL_ERROR;
  }
  else
  {
    tmp = full_path (FP_GET_POSTFIX, 0, "");   /* this will be string we search for */

    /* look for entries which have our postfix */
    while (retval == RETVAL_OK && (entry = readdir(dptr)) != NULL)
    {
      /* ignore hidden files */
      if (entry->d_name[0] != '.' && strstr (entry->d_name, tmp))
      {
	tptr = full_path (FP_GET_NOPOSTFIX, FP_MSGDIR, entry->d_name);
        if (stat(tptr, &sbuf) || (fpin = fopen (tptr, "r")) == NULL)
	{
	  MyPerror (tptr);
	  retval = RETVAL_ERROR;
	}
	else
	{
	  if (cursize == 0)
	  {
            if (fptr)
	    {
	      /* close old file */
	      fclose (fptr);
	      batchnr++;
	    }
	    /* have to open file */
	    if (batchnr == 0)
                 strcpy (buf, master->batchfile);
            else sprintf (buf, "%s%d", master->batchfile, batchnr);

	    if (master->debug == TRUE)
              do_debug ("BATCH FILE: %s\n", buf);

	    if (stat (buf, &tbuf) == 0)
	    {
	      /* whoops file already exists */
	      MyPerror (buf);
	      retval = RETVAL_ERROR;
	    }
            else if ((fptr = fopen(buf,"w")) == NULL)
	    {
	      MyPerror (buf);
	      retval = RETVAL_ERROR;
	    }
	  }
	  if (retval == RETVAL_OK)
	  {
	    /* first put #! rnews size */
	    fprintf (fptr, "#! rnews %ld\n", (long) sbuf.st_size);

	    /* use fread/fwrite in case lines are longer than MAXLINLEN */
	    while ((i = fread (buf, 1, MAXLINLEN, fpin)) > 0 && retval == RETVAL_OK)
	    {
	      x = fwrite (buf, 1, i, fptr);
	      if (x != i)
	      {
		/* error writing file */
		retval = RETVAL_ERROR;
		MyPerror (buf);
	      }
	    }
	    if (retval == RETVAL_OK)
	    {
	      unlink (tptr);	/* we are done with it, nuke it */
	      cursize += sbuf.st_size;
	      /* keep track of current file size, we can ignore the #! rnews */
	      /* size, since it adds so little to the overall size */

	      if (master->rnews_size > 0L && cursize > master->rnews_size)
	      {
		/* reached file size length */
		cursize = 0L;	/* this will force a close and open on next article */
	      }
	    }
	  }
	  fclose (fpin);
	}
      }
    }
    if (fptr)
      fclose (fptr);

    closedir (dptr);
  }
  return (retval);
}

void do_lmovebatch (PMaster master)
{
  char const *args[LMOVE_MAX_ARGS];
  int  i, x;

  print_phrases (master->msgs, batch_phrases[0], NULL);

  /* first build command */
  args[0] = "lmove";
  args[1] = "-c";
  args[2] = master->batchfile;
  args[3] = "-d";
  args[4] = full_path (FP_GET, FP_MSGDIR, "");
  i = 5;
  if (master->errlog)
  {
    args[i++] = "-E";
    args[i++] = master->errlog;
  }
  if (master->phrases)
  {
    args[i++] = "-l";
    args[i++] = master->phrases;
  }

  if (master->debug == TRUE)
  {
    args[i++] = "-D";
  }
  args[i] = NULL;

  if (master->debug == TRUE)
  {
    do_debug ("Calling lmove with args:");
    for (x = 0; x < i; x++)
      do_debug (" %s", args[x]);
    do_debug ("\n");
  }

  if (execvp (args[0], (char * const*) args) < 0)
     MyPerror (batch_phrases[2]);
}

/*
 * post articles to local server using IHAVE
 */
int do_localpost (PMaster master)
{
  int   sockfd, count = 0, retval = RETVAL_OK;
  FILE *fp_list;
  char  msg[MAXLINLEN + 1];
  const char *fname;

  TimerFunc (TIMER_START, 0, NULL);

  print_phrases (master->msgs, batch_phrases[5], master->localhost, NULL);

  if (!master->batchfile)
  {
    error_log (ERRLOG_REPORT, batch_phrases[6], NULL);
    retval = RETVAL_ERROR;
  }
  else
  {
    fname   = full_path (FP_GET, FP_TMPDIR, master->batchfile);
    fp_list = fopen (fname, "r");
    if (!fp_list)
    {
      MyPerror (fname);
      retval = RETVAL_ERROR;
    }
    else
    {
      sockfd = connect_local (master);
      if (sockfd < 0)
         retval = RETVAL_ERROR;
      else
      {
        while (retval == RETVAL_OK && fgets(msg,MAXLINLEN,fp_list))
	{
	  retval = post_one_msg (master, sockfd, msg);
	  if (retval == RETVAL_OK)
             count++;
        }
	close (sockfd);
      }
      fclose (fp_list);
    }
    if (retval == RETVAL_OK)
    {
      if (master->debug == TRUE)
        do_debug ("deleting %s\n", fname);
      unlink (fname);
    }
  }

  print_phrases (master->msgs, batch_phrases[10], str_int(count), NULL);
  TimerFunc (TIMER_TIMEONLY, 0, master->msgs);

  return (retval);
}


int post_one_msg (PMaster master, int sockfd, char *msg)
{  
  int   len, nr, longline, do_unlink = FALSE, retval = RETVAL_OK;
  char *msgid, *resp, linein[MAXLINLEN + 4];
  FILE *fpi;

  /* msg contains the path and msgid
   */
  msgid = strstr (msg, " <");	/* find the start of the msgid */
  if (!msgid)
    error_log (ERRLOG_REPORT, batch_phrases[7], msg, NULL);
  else
  {
    *msgid = '\0';		/* end the path name */
    msgid++;			/* so we point to the < */

    len = strlen (msgid);
    /* strip a nl */
    if (msgid[len-1] == '\n')
        msgid[len-1] = '\0';

    fpi = fopen (msg, "r");
    if (!fpi)
      MyPerror (msg);
    else
    {
      sprintf (linein, "IHAVE %s\r\n", msgid);
      if (master->debug == TRUE)
        do_debug ("sending command %s", linein);

      sputline (sockfd, linein);
      if (sgetline (sockfd, &resp) < 0)
        retval = RETVAL_ERROR;
      else
      {
	if (master->debug == TRUE)
          do_debug ("got answer: %s", resp);
        number (resp, &nr);
	if (nr == 435)
	{
	  error_log (ERRLOG_REPORT, batch_phrases[11], msgid, resp, NULL);
	  do_unlink = TRUE;
	}
	else if (nr != 335)
          error_log (ERRLOG_REPORT, batch_phrases[8], msgid, resp, NULL);
        else
	{
	  /* send the article */
	  longline = FALSE;
          while (fgets (linein, MAXLINLEN, fpi))
	  {
	    len = strlen (linein);
	    if (longline == FALSE && linein[0] == '.')
	    {
	      /* double the . at beginning of line */
	      memmove (linein + 1, linein, ++len);
	      linein[0] = '.';
	    }

	    longline = (len - 1 == '\n') ? TRUE : FALSE;
	    if (longline == FALSE)
	    {
	      /* replace nl with cr nl */
	      strncpy (&linein[len - 1], "\r\n", 3);
	    }
	    sputline (sockfd, linein);
	  }
	  if (longline == TRUE)
	  {
	    /* end the last line */
	    sputline (sockfd, "\r\n");
	  }
	  /* end the article */
	  sputline (sockfd, ".\r\n");
	  if (sgetline (sockfd, &resp) < 0)
            retval = RETVAL_ERROR;
          else
	  {
	    if (master->debug == TRUE)
              do_debug ("Got response: %s", resp);

	    number (resp, &nr);
	    if (nr == 437)
	    {
	      error_log (ERRLOG_REPORT, batch_phrases[12], msgid, resp, NULL);
	      do_unlink = TRUE;
	    }
	    else if (nr == 235)
	    {
	      /* successfully posted, nuke it */
	      do_unlink = TRUE;
	    }
	    else
              error_log (ERRLOG_REPORT, batch_phrases[9], msgid, resp, NULL);
          }
	}
      }
      fclose (fpi);
      if (do_unlink == TRUE)
        unlink (msg);
    }
  }
  return (retval);
}
