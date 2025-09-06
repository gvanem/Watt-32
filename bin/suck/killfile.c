
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <limits.h>

#ifdef HAVE_REGEX_H
#include <sys/types.h>
#include <regex.h>
#endif

#include "suck.h"
#include "config.h"
#include "both.h"
#include "utils.h"
#include "killfile.h"
#include "phrases.h"
#include "timer.h"

#define GROUP_KEEP       "keep"
#define GROUP_DELETE     "delete"       /* word in param file parsed for on group line to signify keep or delete */
#define COMMA            ','            /* group separator in newsgroup line in article */
#define NEWSGROUP_HEADER "Newsgroups: " /* header for line to match group files to */

/* used for debugging print
 */
#define TRUE_STR(x)  (((x) == TRUE) ? "TRUE" : "FALSE")
#define NULL_STR(x)  (((x) == NULL) ? "NULL" : (x))

static int  find_string (int, pmy_regex, char *);
static int  get_chunk_mem (PMaster, unsigned long *len, int which, char **buf);
static int  check_newsgroups (char *, const char *);
static void free_node (OneKill);
static int  parse_a_file (const char *, const char *, POneKill, int, int, int);
static int  pass_two (PKillStruct, int);
static int  check_a_group (PMaster, POneKill, char *, char **);
static void print_debug (PKillStruct);
static void debug_one_kill (POneKill);
static void add_to_linkedlist (pmy_regex *, pmy_regex);

static pmy_regex regex_scan (char *, char, int, int, char);
static int regex_check      (char *, pmy_regex, int);
static int regex_block      (char *, pmy_regex, int);

#ifdef HAVE_REGEX_H
const char regex_chars[] = "*[]()^$\\?.";   /* characters which make it a regex */
#endif
       
/*-------------------------------------------------------------------------*/
/* the enum must match the phrases in suckengl.c  killf_reasons[] */
enum {
  REASON_NONE, REASON_TOOMANYLINES, REASON_NOTENUFLINES,
  REASON_NRGRPS, REASON_NOKEEP, REASON_TIE, REASON_HEADER,
  REASON_BODY, REASON_BODYBIG, REASON_BODYSMALL
};

enum {
  CHECK_EXACT, CHECK_CHECK
};

enum {
  CHUNK_HEADER, CHUNK_BODY
};

const struct {
      int   len;
      const char *name;
      int   headerlen;
      const char *header;
    } Params[] = {
      {
        8, "HILINES=", 7, "Lines: "
      } ,
      {
        9, "LOWLINES=", 7, "Lines: "
      } ,
      {
        7, "NRGRPS=", 12, "Newsgroups: "
      } ,
      {
        6, "GROUP=", 0, ""
      } ,
      {
        6, "QUOTE=", 0, ""
      } ,
      {
        8, "PROGRAM=", 0, ""
      } ,
      {
        7, "HEADER:", 0, ""
      } ,
      {
        5, "BODY:", 0, ""
      } ,
      {
        9, "BODYSIZE>", 0, ""
      } ,
      {
        9, "BODYSIZE<", 0, ""
      } ,
      {
        21, "GROUP_OVERRIDE_MASTER", 0, ""
      } ,  /* string in masterfile to signify that group files override the master */
      {
        17, "TIEBREAKER_DELETE", 0, ""
      } ,  /* what to do if match multi groups override default of keep */
      {
        18, "USE_EXTENDED_REGEX", 0, ""
      } ,  /* do we use extended regular expressions */
      {
        10, "NON_REGEX=", 0, ""
      }
    };

enum {
  PARAM_HILINE, PARAM_LOWLINE, PARAM_NRGRPS, PARAM_GROUP, PARAM_QUOTE,
  PARAM_PROGRAM, PARAM_HDRSCAN, PARAM_BODY, PARAM_BODYBIG, PARAM_BODYSMALL,
  PARAM_GRPOVERRIDE, PARAM_TIEDELETE, PARAM_E_REGEX, PARAM_NONREGEX
};

#define NR_PARAMS ((int) (sizeof(Params)/sizeof(Params[0])))

enum {
  DELKEEP_KEEP, DELKEEP_DELETE
};

/*--------------------------------------------------------------------------*/
PKillStruct parse_killfile (int logfile_yn, int debug, int ignore_postfix)
{
  FILE *fptr;
  char  buf[MAXLINLEN + 1];
  int   i, doprg = FALSE, retval = TRUE;

  static KillStruct Masterkill = { NULL, TRUE, FALSE, FALSE, 0,
                                   FALSE, NULL, chk_msg_kill, NULL, FALSE
                                 };

  /* Kill file is going to get three passes, 1st one to count how many
   * group files to process so can allocate memory for all the group stuff.
   * Also check for group override of masterkillfile and process PROGRAM
   * line if it exists.  If we have one we don't do anything else.
   * 2nd pass will be to actually process the group files
   * 3rd pass will be to process the master delete stuff
   */

  Masterkill.logyn = logfile_yn;
  /* which option to we use in call to full_path()?
   * Do we use the postfix or not?
   */
  Masterkill.ignore_postfix = (ignore_postfix == TRUE) ? FP_GET_NOPOSTFIX : FP_GET;

  /* FIRST PASS THRU MASTER KILLFILE - look for group delete/keeps and
   * count em  and check for PROGRAM call
   */
  fptr = fopen (full_path (Masterkill.ignore_postfix, FP_DATADIR, N_KILLFILE), "r");
  if (!fptr)
  {
    /* this is not really an error, so don't report it as such */
    retval = FALSE;
  }
  else
  {
    if (debug == TRUE)
      do_debug ("Pass 1 & 2 kill file: %s\n", full_path (Masterkill.ignore_postfix, FP_DATADIR, N_KILLFILE));

    while (fgets(buf, MAXLINLEN, fptr) && doprg == FALSE)
    {
      if (debug == TRUE)
      {
	/* so output looks better */
	i = strlen (buf);
	if (buf[i - 1] == '\n')
          buf[i - 1] = '\0';

        do_debug ("Read kill file line: %s\n", buf);
      }
      if (!strncmp (buf, Params[PARAM_GROUP].name, Params[PARAM_GROUP].len))
      {
	Masterkill.totgrps++;	/* how many group files must we process */
      }
      else if (!strncmp (buf, Params[PARAM_PROGRAM].name, Params[PARAM_PROGRAM].len))
      {
	doprg = killprg_forkit (&Masterkill, &buf[Params[PARAM_PROGRAM].len], debug);
      }
      else if (!strncmp (buf, Params[PARAM_GRPOVERRIDE].name, Params[PARAM_GRPOVERRIDE].len))
      {
	Masterkill.grp_override = TRUE;
      }
      else if (!strncmp (buf, Params[PARAM_TIEDELETE].name, Params[PARAM_TIEDELETE].len))
      {
	Masterkill.tie_delete = TRUE;
      }
      else if (!strncmp (buf, Params[PARAM_E_REGEX].name, Params[PARAM_E_REGEX].len))
      {
	Masterkill.use_extended_regex = TRUE;
      }
    }
    fclose (fptr);

    if (doprg == TRUE)
      Masterkill.totgrps = 0;
    else
    {
      /* SECOND PASS - call routine */
      if (Masterkill.totgrps > 0)
      {
	if (pass_two (&Masterkill, debug) == RETVAL_ERROR)
          retval = FALSE;
      }
      /* THIRD PASS - process master delete stuff */
      if (retval != FALSE &&
          parse_a_file (N_KILLFILE, "Master", &Masterkill.master, debug,
                        Masterkill.ignore_postfix, Masterkill.use_extended_regex) != RETVAL_OK)
        retval = FALSE;
    }
  }

  if (retval == FALSE)
    free_killfile (&Masterkill);  /* just in case any memory got allocated */
  else if (debug == TRUE)
    print_debug (&Masterkill);
  return (retval == FALSE ? NULL : &Masterkill);
}

/*-------------------------------------------------------------------------------------------*/
int pass_two (PKillStruct killp, int debug)
{
  int    retval = RETVAL_OK;
  FILE  *fptr;
  char   buf[MAXLINLEN];
  int    grpon = 0;
  size_t i;
  char  *grpname, *grpfile, *delkeep;

  grpname = grpfile = delkeep = NULL;

  /* SECOND PASS - now that we know how many, we can allocate the
   * space for them. And then have parse_a_file read em in.
   */
  killp->grps = calloc (killp->totgrps, sizeof(Group));
  if (!killp->grps)
  {
    retval = RETVAL_ERROR;
    error_log (ERRLOG_REPORT, killf_phrases[1], NULL);
  }
  else if ((fptr = fopen(full_path(killp->ignore_postfix,FP_DATADIR,N_KILLFILE),"r")) == NULL)
  {
    MyPerror (full_path (killp->ignore_postfix, FP_DATADIR, N_KILLFILE));
    retval = RETVAL_ERROR;
  }
  else
  {
    while (retval == RETVAL_OK && fgets(buf,MAXLINLEN,fptr))
    {
      if (!strncmp (buf, Params[PARAM_GROUP].name, Params[PARAM_GROUP].len))
      {
        /* now parse the line for the 3 required elements
         * keep/delete group_name filename
         */
	delkeep = &buf[Params[PARAM_GROUP].len];
        if (!strncmp (delkeep, GROUP_KEEP, strlen (GROUP_KEEP)))
          killp->grps[grpon].delkeep = DELKEEP_KEEP;

        else if (!strncmp (delkeep, GROUP_DELETE, strlen (GROUP_DELETE)))
          killp->grps[grpon].delkeep = DELKEEP_DELETE;

	else
          retval = RETVAL_ERROR;

	if (retval == RETVAL_OK)
	{
	  grpname = strchr (delkeep, ' ');	/* find the space */
          if (!grpname)
            retval = RETVAL_ERROR;
          else
	  {
	    ++grpname;		/* move past space */
	    grpfile = strchr (grpname, ' ');
            if (!grpfile)
              retval = RETVAL_ERROR;
            else
	    {
	      *grpfile = '\0';	/* truncate the group name for easier copying later */
	      ++grpfile;
	    }
	    /* nuke newline */
	    i = strlen (grpfile) - 1;
	    if (grpfile[i] == '\n')
              grpfile[i] = '\0';
          }

	}
	if (retval == RETVAL_ERROR)
          error_log (ERRLOG_REPORT, killf_phrases[2], buf, NULL);
        else
	{			/* have all three params, put them in place and parse the file */

          killp->grps[grpon].group = strdup (grpname);
          if (!killp->grps[grpon].group)
	  {
	    error_log (ERRLOG_REPORT, killf_phrases[0], NULL);
	    retval = RETVAL_ERROR;
	  }
	  else
	  {
	    /* get, ignoring postfix, so use absolute filename on group line */
            if (parse_a_file(grpfile, grpname, &killp->grps[grpon].match,
                             debug, FP_GET_NOPOSTFIX, killp->use_extended_regex) == RETVAL_ERROR)
	    {
	      /* whoops couldn't open file, ignore */
	      free (killp->grps[grpon].group);
	      grpon--;		/* so that we reuse this grp entry */
	      killp->totgrps--;
	    }
          }
	}
	grpon++;		/* finished with this group */
      }
    }
    fclose (fptr);
  }
  return (retval);
}

/*--------------------------------------------------------------------------*/
void free_killfile (PKillStruct master)
{
  int i;

  if (master)
  {
    /* first kill off killprg if its there */
    if (master->killfunc == chk_msg_kill_fork)
      killprg_closeit (master);

    /* close off log, if opened */
    if (master->logfp)
      fclose (master->logfp);

    free_node (master->master);
    if (master->totgrps > 0)
    {
      for (i = 0; i < master->totgrps; i++)
      {
	free_node (master->grps[i].match);
	free (master->grps[i].group);
      }
      free (master->grps);
    }
  }
}

/*--------------------------------------------------------------------*/
void free_node (OneKill node)
{
  pmy_regex curr, next;

  curr = node.list;

  if (node.header)
  {
#ifdef HAVE_REGEX_H
    if (node.header->ptrs)
      regfree (node.header->ptrs);
#endif
    if (node.header->header)
      free (node.header->header);

    if (node.header->string)
      free (node.header->string);
  }
  while (curr)
  {
#ifdef HAVE_REGEX_H
    if (curr->ptrs)
      regfree (curr->ptrs);
#endif
    if (curr->header)
      free (curr->header);

    if (curr->string)
      free (curr->string);

    next = curr->next;
    free (curr);
    curr = next;
  }
}

/*-------------------------------------------------------------------------*/
int get_one_article_kill (PMaster master, int logcount)
{   
  char        buf[MAXLINLEN + 1], *inbuf;
  const char *tname;
  char        fname[PATH_MAX + 1];
  int         retval, x;
  DWORD       len;
  FILE       *fptr;
  PKillStruct killp;

  killp  = master->killp;
  retval = RETVAL_OK;

  killp->pbody   = NULL; /* since we haven't downloaded anything yet for this article */
  killp->bodylen = 0;

  retval = get_chunk_mem (master, &len, CHUNK_HEADER, &inbuf);

  /* the killfunc pointer points to either chk_msg_kill() or
   * chk_msg_kill_fork(). Do we have to download this sucker, is it
   * mandatory?
   */
  if (retval == RETVAL_OK && ((master->curr)->mandatory == MANDATORY_YES ||
                              (*killp->killfunc) (master, killp, inbuf, len) == FALSE))
  {
    if (master->MultiFile == TRUE)
    {
      /* open file.
       * file name will be ####-#### ex 001-166 (nron,total)
       */
      sprintf (buf, "%0*d-%d", logcount, master->itemon, master->nritems);

      /* the strcpy to avoid wiping out fname in second call to full_path
       */
      strcpy (fname, full_path (FP_GET, FP_MSGDIR, buf));
      strcat (buf, N_TMP_EXTENSION);	/* add temp file extension */
      tname = full_path (FP_GET, FP_TMPDIR, buf);	/* temp file name */
      if (master->debug == TRUE)
        do_debug ("File name = \"%s\" temp = \"%s\"", fname, tname);

      fptr = fopen (tname, "w");
      if (!fptr)
      {
	MyPerror (tname);
	retval = RETVAL_ERROR;
      }
      else
      {
	/* write the header */
        x = fwrite (inbuf, sizeof(inbuf[0]), len, fptr);
        fputs ("\n", fptr);
	if (x != len)
	{
	  retval = RETVAL_ERROR;
	  error_log (ERRLOG_REPORT, killf_phrases[9], NULL);
	}
	if (retval == RETVAL_OK)
	{
	  /* have we already downloaded the body? */
          if (killp->pbody)
	  {
	    inbuf = killp->pbody;
	    len = killp->bodylen;
	  }
	  else
            retval = get_chunk_mem (master, &len, CHUNK_BODY, &inbuf);

	  if (retval == RETVAL_OK)
	  {
	    x = fwrite (inbuf, sizeof (inbuf[0]), len, fptr);
	    if (x != len)
	    {
	      retval = RETVAL_ERROR;
	      error_log (ERRLOG_REPORT, killf_phrases[9], NULL);
	    }
	  }
	}
        fclose (fptr);
	if (retval != RETVAL_OK)
          unlink (tname);

	/* now rename it to the permanent file name */
	else
	{
	  move_file (tname, fname);
          if ((master->batch == BATCH_LIHAVE || master->batch == BATCH_INNFEED) && master->innfeed)
	  {
	    /* write path name and msgid to file */
	    fprintf (master->innfeed, "%s %s\n", fname, (master->curr)->msgnr);
	    fflush (master->innfeed);	/* so it gets written sooner */
	  }
	}
      }
    }
    else
    {
      fputs (inbuf, stdout);
      fputs ("\n", stdout);
      retval = get_chunk_mem (master, &len, CHUNK_BODY, &inbuf);
      if (retval == RETVAL_OK)
      {
	fwrite (inbuf, sizeof (inbuf[0]), len, stdout);
        /* this is needed as a separator in stdout version
         */
	fputs (".\n", stdout);
      }
    }
    if (retval == RETVAL_OK)
      master->nrgot++;
  }
  if (retval == RETVAL_UNEXPECTEDANS)
    retval = RETVAL_OK;         /* so don't abort */

  return (retval);
}


/*
 * this routine gets the header or body into memory, keeping separate
 * buffers for each
 */
int get_chunk_mem (PMaster master, unsigned long *size, int which, char **retbuf)
{
  static char *header_buf  = NULL;
  static char *body_buf    = NULL;
  static int   header_size = 8192;
  static DWORD body_size   = KILL_BODY_BUF_SIZE;

  int   done, partial, len, i, retval;
  char *inbuf, *newbuf, *buf;
  const char *cmd;
  DWORD temp, bufsize, currbuf;  /* bufsize = alloced memory, currbuf = what
                                  * retrieved so far
                                  */
  done    = FALSE;
  partial = FALSE;
  currbuf = 0;
  retval  = RETVAL_OK;

  if (which == CHUNK_HEADER)
  {
    buf = header_buf;
    bufsize = header_size;
  }
  else
  {
    buf = body_buf;
    bufsize = body_size;
  }

  if (!buf)
  {
    buf = malloc (bufsize);
    if (!buf)
    {
      error_log (ERRLOG_REPORT, killf_phrases[0], NULL);
      retval = RETVAL_ERROR;
    }
  }

  if (buf)
  {
    /* build command */
    if (which == CHUNK_HEADER)
    {
      cmd = build_command (master, "head", master->curr);
      i = 221;
    }
    else
    {
      cmd = build_command (master, "body", master->curr);
      i = 222;
    }
    if ((retval = send_command (master, cmd, NULL, i)) != RETVAL_OK)
    {
      free (buf);
      buf = NULL;
    }
  }

  while (buf && done == FALSE)
  {
    len = sgetline (master->sockfd, &inbuf);
    (void) TimerFunc (TIMER_ADDBYTES, len, NULL);
    if (len < 0)
    {
      free (buf);
      buf = NULL;
      done = TRUE;
      retval = RETVAL_ERROR;
    }
    else if (partial == FALSE && inbuf[0] == '.')
    {
      if (len == 2 && inbuf[1] == '\n')
        done = TRUE;

      else if (master->MultiFile == TRUE)
      {
        /* handle double dots IAW RFC977 2.4.1.
         * don't do if we aren't doing multifile, since
         * stdout needs the .. to distinguish dots and EOM
         */
	inbuf++;		/* move past first dot */
	len--;
      }
    }
    if (done == FALSE)
    {
      while ((len + currbuf) > bufsize && buf)
      {
        /* buffer not big enough realloc
         * how much do we increase buf
         * we do this test so in case KILL_CHUNK_BUF_INCREASE < len, we
         * don't get a buffer overflow
         */
	temp = (len > KILL_CHUNK_BUF_INCREASE) ? len : KILL_CHUNK_BUF_INCREASE;
	if (master->debug == TRUE)
          do_debug ("Re-allocing buffer from %lu to %lu\n", bufsize, bufsize+temp);

	bufsize += temp;
        newbuf   = realloc (buf, bufsize);
        if (!newbuf)
	{
	  free (buf);
	  buf = NULL;
	  currbuf = 0;
	  error_log (ERRLOG_REPORT, killf_phrases[0], NULL);
	  retval = RETVAL_ERROR;
	}
	else
          buf = newbuf;
      }
      if (buf)
      {
	/* put string in buffer, use memmove in case of nulls */
	memmove (buf + currbuf, inbuf, len);
	currbuf += len;
	partial = (len == MAXLINLEN && inbuf[len - 1] != '\n') ? TRUE : FALSE;
      }
    }
  }
  /* now save the values for next time */
  if (which == CHUNK_HEADER)
  {
    header_buf = buf;
    header_size = bufsize;
  }
  else
  {
    body_buf = buf;
    body_size = bufsize;
  }
  /* return the length */
  *size = currbuf;

  /* make sure buf is NULL terminated
   * this is needed so logging is correct
   * since using memmove() above, we don't copy the ending NULL
   */
  if (buf)
    buf[currbuf] = '\0';

  *retbuf = buf;
  return (retval);
}


/*
 * chk_msg_kill - return TRUE if kill article, FALSE if keep
 * if kill article, add it to killlog
 */
int chk_msg_kill (PMaster master, PKillStruct killp, char *headerbuf, int buflen)
{
  int   killyn, i, del, keep, match, masterkill;
  const char *group = "Master";
  char *why, *goodwhy;

  goodwhy = why = killf_reasons[REASON_NONE];
  killyn  = FALSE;

  /* first check against master delete */
  masterkill = check_a_group (master, &(killp->master), headerbuf, &why);
  if (masterkill == TRUE && killp->grp_override == FALSE)
    killyn = masterkill;
  else
  {
    /* okay now have to parse group line
     * then check to see if I have group keep/deletes for each group
     * default actions
     */
    keep = FALSE;
    del  = FALSE;
    for (i = 0; i < killp->totgrps; i++)
    {
      if (check_newsgroups (headerbuf, killp->grps[i].group) == TRUE)
      {
	/* bingo this article matches one of our group check it */
	match = check_a_group (master, &(killp->grps[i].match), headerbuf, &why);
	if (killp->grps[i].delkeep == DELKEEP_KEEP)
	{
	  /* matched keep group */
	  if (match == TRUE)
            keep = TRUE;
          else
	  {
	    del = TRUE;
	    group = killp->grps[i].group;
	    goodwhy = killf_reasons[REASON_NOKEEP];
	  }
	}
	else
	{
	  if (match == TRUE)
	  {
	    del = TRUE;
	    goodwhy = why;
	    group = killp->grps[i].group;
	  }
	  else
            keep = TRUE;
        }
      }
    }
    /* now determine if we kill or keep this sucker */
    if (keep == FALSE && del == FALSE)
    {
      /* no group matches, do what masterkill says to do */
      killyn = masterkill;
    }
    else if (keep != del)
    {
      /* only matched one group, figure out which */
      killyn = (del == TRUE) ? TRUE : FALSE;
      why = goodwhy;
    }
    else
    {
      /* matched both, use TIEBREAKER */
      why = killf_reasons[REASON_TIE];
      killyn = killp->tie_delete;
    }
  }
  if (master->debug == TRUE && killyn == TRUE)
    do_debug ("killing: %s: %s: %s", group, why, headerbuf);

  if (killyn == TRUE && killp->logyn != KILL_LOG_NONE)
  {
    /* Log it. Only open this sucker once
     */
    if (!killp->logfp)
    {
      killp->logfp = fopen (full_path (FP_GET, FP_TMPDIR, master->kill_log_name), "a");
      if (!killp->logfp)
        MyPerror (killf_phrases[3]);
    }
    if (killp->logfp)
    {
      /* first print our one-line reason */
      print_phrases (killp->logfp, killf_phrases[4], group, why,
                     (master->curr)->msgnr, NULL);

      if (killp->logyn == KILL_LOG_LONG)
      {
        /* Print the header as well. The nl so I get a blank line between em
         */
	print_phrases (killp->logfp, "%v1%\n", headerbuf, NULL);
      }
    }
  }
  return (killyn);
}

/*-----------------------------------------------------------------------*/
int check_a_group (PMaster master, POneKill killp, char *headerbuf, char **why)
{
  int         i, match = FALSE;
  char       *startline, *tptr;
  pmy_regex   curr;
  static char reason[MAXLINLEN];
  PKillStruct temp;

  /* check hilines first */
  if (killp->hilines > 0)
  {
    startline = strstr (headerbuf, Params[PARAM_HILINE].header);
    if (startline)
    {
      i = 0;			/* just in case */
      sscanf (startline + Params[PARAM_HILINE].headerlen, "%d", &i);
      if (killp->hilines < i)
      {
	/* JACKPOT */
	match = TRUE;
	*why = killf_reasons[REASON_TOOMANYLINES];
      }
    }
  }
  /* now check low lines */
  if (match == FALSE && killp->lowlines > 0)
  {
    startline = strstr (headerbuf, Params[PARAM_LOWLINE].header);
    if (startline)
    {
      i = 0;			/* just in case */
      sscanf (startline + Params[PARAM_LOWLINE].headerlen, "%d", &i);
      if (i < killp->lowlines)
      {
	/* JACKPOT */
	match = TRUE;
	*why = killf_reasons[REASON_NOTENUFLINES];
      }
    }
  }
  /* now check nrgrps */
  if (match == FALSE && killp->maxgrps > 0)
  {
    startline = strstr (headerbuf, Params[PARAM_NRGRPS].header);
    if (startline)
    {
      /* count the nr of commas in the group line */
      i = 1;			/* have at least one group */
      tptr = startline;
      while (i <= killp->maxgrps && *tptr != '\n' && *tptr != '\0')
      {
	/* some news server use space vice comma for separator */
	if (*tptr == COMMA || *tptr == ' ')
          i++;
        tptr++;
      }
      if (i > killp->maxgrps)
      {
	match = TRUE;
	*why = killf_reasons[REASON_NRGRPS];
      }
    }
  }
  curr = killp->list;

  while (match == FALSE && curr)
  {
    if (regex_check (headerbuf, curr, master->debug) == TRUE)
    {
      match = TRUE;
      sprintf (reason, "%s %s %s ", killf_reasons[REASON_HEADER],
               curr->header, curr->string);
      *why = reason;
    }
    curr = curr->next;
  }
  curr = killp->header;
  while (match == FALSE && curr)
  {
    /* scan the entire header */
    if (regex_block (headerbuf, curr, master->debug) == TRUE)
    {
      match = TRUE;
      sprintf (reason, "%s %s", killf_reasons[REASON_HEADER], curr->string);
      *why = reason;
    }
    curr = curr->next;
  }
  if (match == FALSE && (killp->bodybig > 0 || killp->bodysmall > 0 || killp->body))
  {
    /* may have to download the header first to
     * have a pointer to the master killstruct
     */
    temp = master->killp;
    if (!temp->pbody)
    {
      /* have to get the body first */
      get_chunk_mem (master, &(temp->bodylen), CHUNK_BODY, &temp->pbody);
    }
    if (killp->bodybig > 0 && temp->bodylen > killp->bodybig)
    {
      match = TRUE;
      sprintf (reason, "%s %lu", killf_reasons[REASON_BODYBIG], temp->bodylen);
      *why = reason;
    }
    else if (killp->bodysmall > 0 && temp->bodylen < killp->bodysmall)
    {
      match = TRUE;
      sprintf (reason, "%s %lu", killf_reasons[REASON_BODYSMALL], temp->bodylen);
      *why = reason;
    }
    else
    {
      curr = killp->body;
      while (match == FALSE && curr)
      {				/* scan the body */
	if (regex_block (temp->pbody, curr, master->debug) == TRUE)
	{
	  match = TRUE;
	  sprintf (reason, "%s %s", killf_reasons[REASON_BODY], curr->string);
	  *why = reason;
	}
	curr = curr->next;
      }
    }
  }
  return (match);
}

/*------------------------------------------------------------------------*/
int check_newsgroups (char *header, const char *whichgroup)
{
 /* search Newsgroup headerline for whichgroup
  * if in return TRUE else return FALSE;
  */

  int   match = FALSE;
  char *startline, *ptr;
  const char *ptr_group;

  startline = strstr (header, NEWSGROUP_HEADER);
  if (startline)
  {
    ptr = startline + strlen (NEWSGROUP_HEADER);
    while (match == FALSE && *ptr != '\0' && *ptr != '\n')
    {
      ptr_group = whichgroup;
      while (*ptr == *ptr_group && *ptr != '\0')
      {
	ptr++;
	ptr_group++;
      }
      if (*ptr_group == '*')
      {
	/* wildcard match, they match so far, so they match */
	match = TRUE;
      }
      else if ((*ptr_group == '\0') && (*ptr == COMMA || *ptr == ' ' || *ptr == '\n' || *ptr == '\0'))
      {
       /* if we are at the end of both group names then we have a match
        * we check for space even though its not in the standard, apparently
        * some news server use it.
        */
	match = TRUE;
      }
      else
      {
	/* advance to next group on line */
	while (*ptr != COMMA && *ptr != '\n' && *ptr != '\0')
          ptr++;
        if (*ptr == COMMA)
	{
	  /* advance past it */
	  ptr++;
	}
      }
    }
  }
  return (match);
}

/*---------------------------------------------------------------------*/
int parse_a_file (const char *fname, const char *group, POneKill mykill,
                  int debug, int ignore_prefix, int use_e_regex)
{
  FILE     *fptr;
  char      buf[MAXLINLEN + 1];
  int       i, match;
  int       retval = RETVAL_OK;
  pmy_regex curr;

  mykill->list   = NULL;
  mykill->header = NULL;

  /* first initialized the killstruct */
  mykill->hilines   = mykill->lowlines = mykill->maxgrps = 0;
  mykill->quote     = KILLFILE_QUOTE;
  mykill->non_regex = KILLFILE_NONREGEX;
#ifdef HAVE_REGEX_H
  mykill->use_extended = use_e_regex;
#endif

  /* now read in the killfile and parse it */
  fptr = fopen (full_path (ignore_prefix, FP_DATADIR, fname), "r");
  if (!fptr)
  {
    MyPerror (full_path (ignore_prefix, FP_DATADIR, fname));
    retval = RETVAL_ERROR;
  }
  else
  {
    if (debug == TRUE)
    {
      do_debug ("Pass 3 kill file: %s\n",
                full_path(ignore_prefix,FP_DATADIR,fname));
      do_debug ("Pass 3 options: group=%s, use_extended_regex=%s, ignore_prefix=%s\n",
		group, TRUE_STR (use_e_regex), TRUE_STR (ignore_prefix));
    }
    while (fgets (buf, MAXLINLEN, fptr))
    {
      buf[MAXLINLEN] = '\0';	/* just in case */

      i = strlen (buf);
      /* strip nls off so they don't get added to regex scan, etc.. */
      if (buf[i - 1] == '\n')
          buf[i - 1] = '\0';

      if (debug == TRUE)
        do_debug ("pass 3 kill file line: %s\n", buf);

      if (buf[0] != KILLFILE_COMMENT_CHAR)
      {
	/* skip any comment lines */
	match = FALSE;
	for (i = 0; i < NR_PARAMS; i++)
	{
          if (!strncmp (buf, Params[i].name, Params[i].len))
	  {
	    match = TRUE;	/* so don't try header field scan later */
	    switch (i)
            {
              case PARAM_HILINE:
                   sscanf (&buf[Params[PARAM_HILINE].len], "%d", &mykill->hilines);
		   break;
              case PARAM_LOWLINE:
                   sscanf (&buf[Params[PARAM_LOWLINE].len], "%d", &mykill->lowlines);
		   break;
              case PARAM_NRGRPS:
                   sscanf (&buf[Params[PARAM_NRGRPS].len], "%d", &mykill->maxgrps);
		   break;
              case PARAM_QUOTE:
		   if (buf[Params[PARAM_QUOTE].len] == '\0')
                        error_log (ERRLOG_REPORT, "%s\n", killf_phrases[6], NULL);
                   else mykill->quote = buf[Params[PARAM_QUOTE].len];
		   break;
              case PARAM_NONREGEX:
		   if (buf[Params[PARAM_NONREGEX].len] == '\0')
                        error_log (ERRLOG_REPORT, "%s\n", killf_phrases[12], NULL);
                   else mykill->non_regex = buf[Params[PARAM_NONREGEX].len];
                   break;
              case PARAM_HDRSCAN:
		   if (buf[Params[PARAM_HDRSCAN].len] == '\0')
                     error_log (ERRLOG_REPORT, "%s\n", killf_phrases[7], NULL);
                   else
		   {
                     curr = regex_scan (buf, mykill->quote, debug,
                                        use_e_regex, mykill->non_regex);
                     if (curr)
                       add_to_linkedlist (&mykill->header, curr);
                   }
		   break;
              case PARAM_BODY:
		   if (buf[Params[PARAM_BODY].len] == '\0')
                     error_log (ERRLOG_REPORT, "%s\n", killf_phrases[10], NULL);
                   else
		   {
                     curr = regex_scan (buf, mykill->quote, debug,
                                        use_e_regex, mykill->non_regex);
                     if (curr)
                       add_to_linkedlist (&(mykill->body), curr);
                   }
		   break;
              case PARAM_BODYBIG:
                   sscanf (&buf[Params[PARAM_BODYBIG].len], "%lu", &mykill->bodybig);
		   break;
              case PARAM_BODYSMALL:
                   sscanf (&buf[Params[PARAM_BODYSMALL].len], "%lu", &mykill->bodysmall);
		   break;
              case PARAM_E_REGEX:
		   /* in case specified for this file only */
		   use_e_regex = TRUE;
#ifdef HAVE_REGEX_H
                   mykill->use_extended = TRUE;   /*  save for printing in debug */
#endif
		   break;
              default:       /* handle all other lines aka do nothing */
		   break;
            }
	  }
	}
	if (match == FALSE)
	{
	  /* it's a different line in the header add to linked-list */
          curr = regex_scan (buf, mykill->quote, debug, use_e_regex,
                             mykill->non_regex);
          if (curr)
            add_to_linkedlist (&(mykill->list), curr);
        }
      }				/* comment line stuff */
    }
    fclose (fptr);
  }
  return (retval);
}

/*--------------------------------------------------------------------*/
void add_to_linkedlist (pmy_regex * head_ptr, pmy_regex addon)
{   
  /* this routine adds a pointer the end of a linked list */
  pmy_regex curr;

  if (*head_ptr == NULL)
  {
    /* put on top of list */
    *head_ptr = addon;
  }
  else
  {
    curr = *head_ptr;
    while (curr->next)
      curr = curr->next;
    curr->next = addon;
  }
}


/*
 * scan a line and build the compiled regex pointers to call regexex() with
 */
pmy_regex regex_scan (char *linein, char quotechar, int debug, int use_e_regex, char non_regex)
{
  char *tptr, *startline;
  int   len, x, y, case_sens = FALSE, flags = FALSE;

#ifdef HAVE_REGEX_H
  char  errmsg[256];
  int   i, j, useregex = TRUE;
#endif

  pmy_regex which = NULL;
  int err = FALSE;

  if (linein)
  {
    /* first find the : in the line to skip the parameter part */
    tptr = linein;
    while (*tptr != '\0' && *tptr != ':')
      tptr++;

    startline = tptr + 1;
    len = startline - linein;

    /* do we treat this string as a non-regex?? */
    if (*startline == non_regex)
    {
      startline++;
#ifdef HAVE_REGEX_H
      useregex = FALSE;
#endif
    }
    /* case sensitive search?? */
    if (*startline == quotechar)
    {
      startline++;
      case_sens = TRUE;
    }
#ifdef HAVE_REGEX_H
    if (useregex == TRUE)
    {
      flags = REG_NOSUB;
      if (case_sens == FALSE)
        flags |= REG_ICASE;

      if (use_e_regex == TRUE)
        flags |= REG_EXTENDED;

      useregex = FALSE;
      /* test to see whether or not we need to use regex */
      for (i = 0; i < strlen (regex_chars) && useregex != TRUE; i++)
        for (j = 0; j < strlen (startline) && useregex != TRUE; j++)
          if (startline[j] == regex_chars[i])
            useregex = TRUE;
    }

#endif
    /* now count each of em, so that can alloc my array of regex_t */
    if (*tptr != '\0')
    {
      which = malloc (sizeof(my_regex));
      if (!which)
        error_log (ERRLOG_REPORT, killf_phrases[5], NULL);
      else
      {
	/* first save the string for later printing, if necessary */
        which->string = strdup (startline);
        if (!which->string)
	{
	  err = TRUE;
	  error_log (ERRLOG_REPORT, killf_phrases[5], NULL);
	}
	else
	{
          which->next           = NULL;   /* initialize */
	  which->case_sensitive = case_sens;
#ifdef HAVE_REGEX_H
	  which->ptrs = NULL;	/* just to be on safe side */
#endif
          which->header = calloc (sizeof(char), len+1);
          if (!which->header)
	  {
	    error_log (ERRLOG_REPORT, killf_phrases[5], NULL);
	    err = TRUE;
	  }
	  else
	  {
	    strncpy (which->header, linein, len);
	    which->header[len] = '\0';	/* just in case */
#ifdef HAVE_REGEX_H
	    if (useregex == TRUE)
	    {
              which->ptrs = malloc (sizeof(regex_t));
              if (!which->ptrs)
                error_log (ERRLOG_REPORT, killf_phrases[5], NULL);
              else
	      {
		if (debug == TRUE)
                  do_debug ("Regcomping -%s-\n", startline);

                err = regcomp (which->ptrs, startline, flags);
                if (err)
		{
		  /* whoops */
                  regerror (err, which->ptrs, errmsg, sizeof(errmsg));
                  error_log (ERRLOG_REPORT, killf_phrases[11],
                             startline, errmsg, NULL);
		  err = TRUE;
		}
	      }
	    }
	    else
	    {
#endif        /* not using regex, fill in skiparray
               * first fill in max skip for all chars (len of string)
               * See Algorithms/Sedgewick pg 287-289 for explanation
               */
	      y = strlen (which->string);

	      for (x = 0; x < sizeof (which->skiparray); x++)
                which->skiparray[x] = y;

	      /* now go back and fill in those in our search string */
	      if (case_sens == FALSE)
	      {
		/* case-insensitive search */
		for (x = 0; x < y; x++)
		{
		  /* have to match both upper and lower case */
                  char xx = which->string[x];
                  which->skiparray [(BYTE)toupper(xx)] = y - 1 - x;
                  which->skiparray [(BYTE)tolower(xx)] = y - 1 - x;
		}
	      }
	      else
	      {
		for (x = 1; x < y; x++)
                  which->skiparray [(BYTE)which->string[x]] = y - 1 - x;
              }

#ifdef HAVE_REGEX_H
	    }
#endif
	  }
	}
      }

    }
  }
  if (which && err == TRUE)
  {
    /* error, free up everything */
#ifdef HAVE_REGEX_H
    if (which->ptrs)
      free (which->ptrs);
#endif
    if (which->string)
      free (which->string);

    if (which->header)
      free (which->header);

    if (which)
    {
      free (which);
      which = NULL;
    }
  }
  return (which);
}

/*----------------------------------------------------------------*/
int regex_check (char *headerbuf, pmy_regex expr, int debug)
{
  int   match = FALSE;
  char *startline, *endline;

  if (expr->header)
  {
    startline = strstr (headerbuf, expr->header);
    if (startline)
    {
      endline = strchr (startline, '\n');
      /* end this line, so we only search the right header line */
      if (endline)
        *endline = '\0';

      /* the +strlen(expr->header) so the header field and the space
       * immediately following it aren't included in the search
       */
      if (debug == TRUE)
        do_debug ("checking -%s- for -%s-\n", startline+strlen(expr->header)+1,
		  expr->string);
#ifdef HAVE_REGEX_H
      if (expr->ptrs)
      {
        if (regexec (expr->ptrs,startline+strlen(expr->header)+1,0,NULL,0) == 0)
          match = TRUE;
      }
      else
      {
#endif
	match = find_string (debug, expr, startline + strlen (expr->header) + 1);
#ifdef HAVE_REGEX_H
      }
#endif
      /* put the nl back on that we deleted */
      if (endline)
        *endline = '\n';
    }
  }
  if (debug == TRUE)
    do_debug ("Returning match=%s\n", (match == TRUE) ? "True" : "False");

  return (match);
}

/*--------------------------------------------------------------------*/
int regex_block (char *what, pmy_regex where, int debug)
{    
  /* find a string in a block */
  int match = FALSE;

  if (debug == TRUE && where->string)
    do_debug ("Checking for -%s-\n", where->string);

#ifdef HAVE_REGEX_H
  if (where->ptrs)
  {
    if (regexec (where->ptrs, what, 0, NULL, 0) == 0)
      match = TRUE;
  }
  else
  {
#endif
    match = find_string (debug, where, what);
#ifdef HAVE_REGEX_H
  }
#endif
  return (match);
}

/*---------------------------------------------------------------------*/
int find_string (int debug, pmy_regex expr, char *str)
{ 
  /* the search mechanism is based on the Boyer-Moor Algorithms
   * (pg 286- of Algorithms book)
   */
  int   match = FALSE;
  char *ptr;             /* pointer to pattern */
  int   lenp;            /* length of pattern */
  int   lens;            /* length of string we are searching  */
  int   i;               /* current location in string */
  int   j;               /* current location in pattern */
  int   t;               /* value of skip array for string location */

  ptr  = expr->string;
  lenp = strlen (ptr);
  lens = strlen (str);

  if (expr->case_sensitive == TRUE)
  {
    /* do case-sensitive search */
    if (debug == TRUE)
      do_debug ("Case-sensitive search for %s\n", ptr);

    i = j = lenp - 1;
    do
    {
      if (str[i] == ptr[j])
      {
	i--;
	j--;
      }
      else
      {
        t  = expr->skiparray[(unsigned char) str[i]];
	i += (lenp - j > t) ? (lenp - j) : t;
        j  = lenp - 1;
      }
    }
    while (j >= 0 && i < lens);
    if (j < 0)
      match = TRUE;
  }
  else
  {
    /* do case-insensitive search */
    if (debug == TRUE)
      do_debug ("Case-insensitive search for --%s--\n", ptr);

    i = j = lenp - 1;
    do
    {
      if (tolower (str[i]) == tolower (ptr[j]))
      {
	i--;
	j--;
      }
      else
      {
        t  = expr->skiparray[(unsigned char) str[i]];
	i += (lenp - j > t) ? (lenp - j) : t;
        j  = lenp - 1;
      }
    }
    while (j >= 0 && i < lens);
    if (j < 0)
      match = TRUE;
  }
  return (match);
}

/*--------------------------------------------------------------------*/
void print_debug (PKillStruct master)
{
  int i;

  /* print out status of master killstruct to debug file
   */
  do_debug ("Master KillStruct:\n");
  do_debug ("logyn=%s\n",             TRUE_STR (master->logyn));
  do_debug ("grp_override=%s\n",      TRUE_STR (master->grp_override));
  do_debug ("tie_delete=%s\n",        TRUE_STR (master->tie_delete));
  do_debug ("totgrps=%d\n",           master->totgrps);
  do_debug ("ignore_postfix=%s\n",    TRUE_STR (master->ignore_postfix));
  do_debug ("use_extended_regex=%s\n",TRUE_STR (master->use_extended_regex));

  do_debug ("Master kill group");
  debug_one_kill (&(master->master));

  for (i = 0; i < master->totgrps; i++)
  {
    do_debug ("Group %d = %s\n", i, master->grps[i].group);
    do_debug ("--delkeep =%s\n", master->grps[i].delkeep == DELKEEP_KEEP ?
              "keep" : "delete");
    debug_one_kill (&(master->grps[i].match));
  }
  do_debug ("End of Killstruct debug\n");
}

/*------------------------------------------------------------------------*/
void debug_one_kill (POneKill ptr)
{
  pmy_regex temp;

  do_debug ("--hilines =%d\n",  ptr->hilines);
  do_debug ("--lowlines=%d\n",  ptr->lowlines);
  do_debug ("--maxgrps=%d\n",   ptr->maxgrps);
  do_debug ("--bodysize>%lu\n", ptr->bodybig);
  do_debug ("--bodysize<%lu\n", ptr->bodysmall);
  do_debug ("--quote=%c\n",     ptr->quote);
  do_debug ("--non_regex=%c\n", ptr->non_regex);
#ifdef HAVE_REGEX_H
  do_debug ("--use_extended_regex=%s\n", TRUE_STR (ptr->use_extended));
#endif
  temp = ptr->header;

  while (temp)
  {
#ifdef HAVE_REGEX_H
    do_debug ("--header scan is %sregex\n", temp->ptrs == NULL ? "non-" : "");
#endif
    do_debug ("--Scan is %scase-sensitive\n", temp->case_sensitive == TRUE ? "" : "non-");
    if (temp->string)
      do_debug ("--header scan=%s\n", temp->string);

    temp = temp->next;
  }
  temp = ptr->body;
  while (temp)
  {
#ifdef HAVE_REGEX_H
    do_debug ("--body scan is %sregex\n", temp->ptrs == NULL ? "non-" : "");
#endif
    do_debug ("--Scan is %scase-sensitive\n", temp->case_sensitive == TRUE ? "" : "non-");
    if (temp->string)
      do_debug ("--body  scan=%s\n", temp->string);

    temp = temp->next;
  }
  temp = ptr->list;

  while (temp)
  {
#ifdef HAVE_REGEX_H
    do_debug ("--header match is %sregex\n", temp->ptrs == NULL ? "non-" : "");
#endif
    do_debug ("--Scan is %scase-sensitive\n", temp->case_sensitive == TRUE ? "" : "non-");
    do_debug ("--header match= %s %s\n", temp->header, temp->string);
    temp = temp->next;
  }
  do_debug ("--end of Killfile Group--\n");
}
