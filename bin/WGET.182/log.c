/* Messages logging.
   Copyright (C) 1998, 2000, 2001 Free Software Foundation, Inc.

This file is part of GNU Wget.

GNU Wget is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GNU Wget is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wget; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

In addition, as a special exception, the Free Software Foundation
gives permission to link the code of its release of Wget with the
OpenSSL project's "OpenSSL" library (or with modified versions of it
that use the same license as the "OpenSSL" library), and distribute
the linked executables.  You must obey the GNU General Public License
in all respects for all of the code used other than "OpenSSL".  If you
modify this file, you may extend this exception to your version of the
file, but you are not obligated to do so.  If you do not wish to do
so, delete this exception statement from your version.  */

#include <config.h>

#include <stdio.h>
#ifdef HAVE_STRING_H
# include <string.h>
#else
# include <strings.h>
#endif
#include <stdlib.h>
#ifdef HAVE_STDARG_H
# define WGET_USE_STDARG
# include <stdarg.h>
#else
# include <varargs.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <assert.h>
#include <errno.h>

#include "wget.h"
#include "utils.h"

#ifndef errno
extern int errno;
#endif

/* This file impplement support for "logging".  Logging means printing
   output, plus several additional features:

   - Cataloguing output by importance.  You can specify that a log
   message is "verbose" or "debug", and it will not be printed unless
   in verbose or debug mode, respectively.

   - Redirecting the log to the file.  When Wget's output goes to the
   terminal, and Wget receives SIGHUP, all further output is
   redirected to a log file.  When this is the case, Wget can also
   print the last several lines of "context" to the log file so that
   it does not begin in the middle of a line.  For this to work, the
   logging code stores the last several lines of context.  Callers may
   request for certain output not to be stored.

   - Inhibiting output.  When Wget receives SIGHUP, but redirecting
   the output fails, logging is inhibited.  */


/* The file descriptor used for logging.  This is NULL before log_init
   is called; logging functions log to stderr then.  log_init sets it
   either to stderr or to a file pointer obtained from fopen().  If
   logging is inhibited, logfp is set back to NULL. */
static FILE *logfp;

/* If non-zero, it means logging is inhibited, i.e. nothing is printed
   or stored.  */
static int inhibit_logging;

/* Whether the last output lines are stored for use as context.  */
static int save_context_p;

/* Whether the log is flushed after each command. */
static int flush_log_p = 1;

/* Whether any output has been received while flush_log_p was 0. */
static int needs_flushing;

/* In the event of a hang-up, and if its output was on a TTY, Wget
   redirects its output to `wget-log'.

   For the convenience of reading this newly-created log, we store the
   last several lines ("screenful", hence the choice of 24) of Wget
   output, and dump them as context when the time comes.  */
#define SAVED_LOG_LINES 24

/* log_lines is a circular buffer that stores SAVED_LOG_LINES lines of
   output.  log_line_current always points to the position in the
   buffer that will be written to next.  When log_line_current reaches
   SAVED_LOG_LINES, it is reset to zero.

   The problem here is that we'd have to either (re)allocate and free
   strings all the time, or limit the lines to an arbitrary number of
   characters.  Instead of settling for either of these, we do both:
   if the line is smaller than a certain "usual" line length (128
   chars by default), a preallocated memory is used.  The rare lines
   that are longer than 128 characters are malloc'ed and freed
   separately.  This gives good performance with minimum memory
   consumption and fragmentation.  */

#define STATIC_LENGTH 128

static struct log_ln {
  char static_line[STATIC_LENGTH + 1]; /* statically allocated
                                          line. */
  char *malloced_line;		/* malloc'ed line, for lines of output
                                   larger than 80 characters. */
  char *content;		/* this points either to malloced_line
                                   or to the appropriate static_line.
                                   If this is NULL, it means the line
                                   has not yet been used. */
} log_lines[SAVED_LOG_LINES];

/* The current position in the ring. */
static int log_line_current = -1;

/* Whether the most recently written line was "trailing", i.e. did not
   finish with \n.  This is an important piece of information because
   the code is always careful to append data to trailing lines, rather
   than create new ones.  */
static int trailing_line;

static void check_redirect_output PARAMS ((void));

#define ROT_ADVANCE(num) do {			\
  if (++num >= SAVED_LOG_LINES)			\
    num = 0;					\
} while (0)

/* Free the log line index with NUM.  This calls free on
   ln->malloced_line if it's non-NULL, and it also resets
   ln->malloced_line and ln->content to NULL.  */

static void
free_log_line (int num)
{
  struct log_ln *ln = log_lines + num;
  if (ln->malloced_line)
    {
      xfree (ln->malloced_line);
      ln->malloced_line = NULL;
    }
  ln->content = NULL;
}

/* Append bytes in the range [start, end) to one line in the log.  The
   region is not supposed to contain newlines, except for the last
   character (at end[-1]).  */

static void
saved_append_1 (const char *start, const char *end)
{
  int len = end - start;
  if (!len)
    return;

  /* First, check whether we need to append to an existing line or to
     create a new one.  */
  if (!trailing_line)
    {
      /* Create a new line. */
      struct log_ln *ln;

      if (log_line_current == -1)
	log_line_current = 0;
      else
	free_log_line (log_line_current);
      ln = log_lines + log_line_current;
      if (len > STATIC_LENGTH)
	{
	  ln->malloced_line = strdupdelim (start, end);
	  ln->content = ln->malloced_line;
	}
      else
	{
	  memcpy (ln->static_line, start, len);
	  ln->static_line[len] = '\0';
	  ln->content = ln->static_line;
	}
    }
  else
    {
      /* Append to the last line.  If the line is malloc'ed, we just
         call realloc and append the new string.  If the line is
         static, we have to check whether appending the new string
         would make it exceed STATIC_LENGTH characters, and if so,
         convert it to malloc(). */
      struct log_ln *ln = log_lines + log_line_current;
      if (ln->malloced_line)
	{
	  /* Resize malloc'ed line and append. */
	  int old_len = strlen (ln->malloced_line);
	  ln->malloced_line = xrealloc (ln->malloced_line, old_len + len + 1);
	  memcpy (ln->malloced_line + old_len, start, len);
	  ln->malloced_line[old_len + len] = '\0';
	  /* might have changed due to realloc */
	  ln->content = ln->malloced_line;
	}
      else
	{
	  int old_len = strlen (ln->static_line);
	  if (old_len + len > STATIC_LENGTH)
	    {
	      /* Allocate memory and concatenate the old and the new
                 contents. */
	      ln->malloced_line = xmalloc (old_len + len + 1);
	      memcpy (ln->malloced_line, ln->static_line,
		      old_len);
	      memcpy (ln->malloced_line + old_len, start, len);
	      ln->malloced_line[old_len + len] = '\0';
	      ln->content = ln->malloced_line;
	    }
	  else
	    {
	      /* Just append to the old, statically allocated
                 contents.  */
	      memcpy (ln->static_line + old_len, start, len);
	      ln->static_line[old_len + len] = '\0';
	      ln->content = ln->static_line;
	    }
	}
    }
  trailing_line = !(end[-1] == '\n');
  if (!trailing_line)
    ROT_ADVANCE (log_line_current);
}

/* Log the contents of S, as explained above.  If S consists of
   multiple lines, they are logged separately.  If S does not end with
   a newline, it will form a "trailing" line, to which things will get
   appended the next time this function is called.  */

static void
saved_append (const char *s)
{
  while (*s)
    {
      const char *end = strchr (s, '\n');
      if (!end)
	end = s + strlen (s);
      else
	++end;
      saved_append_1 (s, end);
      s = end;
    }
}

/* Check X against opt.verbose and opt.quiet.  The semantics is as
   follows:

   * LOG_ALWAYS - print the message unconditionally;

   * LOG_NOTQUIET - print the message if opt.quiet is non-zero;

   * LOG_NONVERBOSE - print the message if opt.verbose is zero;

   * LOG_VERBOSE - print the message if opt.verbose is non-zero.  */
#define CHECK_VERBOSE(x)			\
  switch (x)					\
    {						\
    case LOG_ALWAYS:				\
      break;					\
    case LOG_NOTQUIET:				\
      if (opt.quiet)				\
	return;					\
      break;					\
    case LOG_NONVERBOSE:			\
      if (opt.verbose || opt.quiet)		\
	return;					\
      break;					\
    case LOG_VERBOSE:				\
      if (!opt.verbose)				\
	return;					\
    }

/* Returns the file descriptor for logging.  This is LOGFP, except if
   called before log_init, in which case it returns stderr.  This is
   useful in case someone calls a logging function before log_init.

   If logging is inhibited, return NULL.  */

static FILE *
get_log_fp (void)
{
  if (inhibit_logging)
    return NULL;
  if (logfp)
    return logfp;
  return stderr;
}

/* Log a literal string S.  The string is logged as-is, without a
   newline appended.  */

void
logputs (enum log_options o, const char *s)
{
  FILE *fp;

  check_redirect_output ();
  if ((fp = get_log_fp ()) == NULL)
    return;
  CHECK_VERBOSE (o);

  fputs (s, fp);
  if (save_context_p)
    saved_append (s);
  if (flush_log_p)
    logflush ();
  else
    needs_flushing = 1;
}

struct logvprintf_state {
  char *bigmsg;
  int expected_size;
  int allocated;
};

/* Print a message to the log.  A copy of message will be saved to
   saved_log, for later reusal by log_dump_context().

   It is not possible to code this function in a "natural" way, using
   a loop, because of the braindeadness of the varargs API.
   Specifically, each call to vsnprintf() must be preceded by va_start
   and followed by va_end.  And this is possible only in the function
   that contains the `...' declaration.  The alternative would be to
   use va_copy, but that's not portable.  */

static int
logvprintf (struct logvprintf_state *state, const char *fmt, va_list args)
{
  char smallmsg[128];
  char *write_ptr = smallmsg;
  int available_size = sizeof (smallmsg);
  int numwritten;
  FILE *fp = get_log_fp ();

  if (!save_context_p)
    {
      /* In the simple case just call vfprintf(), to avoid needless
         allocation and games with vsnprintf(). */
      vfprintf (fp, fmt, args);
      goto flush;
    }

  if (state->allocated != 0)
    {
      write_ptr = state->bigmsg;
      available_size = state->allocated;
    }

  /* The GNU coding standards advise not to rely on the return value
     of sprintf().  However, vsnprintf() is a relatively new function
     missing from legacy systems.  Therefore I consider it safe to
     assume that its return value is meaningful.  On the systems where
     vsnprintf() is not available, we use the implementation from
     snprintf.c which does return the correct value.  */
  numwritten = vsnprintf (write_ptr, available_size, fmt, args);

  /* vsnprintf() will not step over the limit given by available_size.
     If it fails, it will return either -1 (POSIX?) or the number of
     characters that *would have* been written, if there had been
     enough room.  In the former case, we double the available_size
     and malloc() to get a larger buffer, and try again.  In the
     latter case, we use the returned information to build a buffer of
     the correct size.  */

  if (numwritten == -1)
    {
      /* Writing failed, and we don't know the needed size.  Try
	 again with doubled size. */
      int newsize = available_size << 1;
      state->bigmsg = xrealloc (state->bigmsg, newsize);
      state->allocated = newsize;
      return 0;
    }
  else if (numwritten >= available_size)
    {
      /* Writing failed, but we know exactly how much space we
	 need. */
      int newsize = numwritten + 1;
      state->bigmsg = xrealloc (state->bigmsg, newsize);
      state->allocated = newsize;
      return 0;
    }

  /* Writing succeeded. */
  saved_append (write_ptr);
  fputs (write_ptr, fp);
  if (state->bigmsg)
    xfree (state->bigmsg);

 flush:
  if (flush_log_p)
    logflush ();
  else
    needs_flushing = 1;

  return 1;
}

/* Flush LOGFP.  Useful while flushing is disabled.  */
void
logflush (void)
{
  FILE *fp = get_log_fp ();
  if (fp)
    fflush (fp);
  needs_flushing = 0;
}

/* Enable or disable log flushing. */
void
log_set_flush (int flush)
{
  if (flush == flush_log_p)
    return;

  if (flush == 0)
    {
      /* Disable flushing by setting flush_log_p to 0. */
      flush_log_p = 0;
    }
  else
    {
      /* Reenable flushing.  If anything was printed in no-flush mode,
	 flush the log now.  */
      if (needs_flushing)
	logflush ();
      flush_log_p = 1;
    }
}

/* (Temporarily) disable storing log to memory.  Returns the old
   status of storing, with which this function can be called again to
   reestablish storing. */

int
log_set_save_context (int savep)
{
  int old = save_context_p;
  save_context_p = savep;
  return old;
}

#ifdef WGET_USE_STDARG
# define VA_START_1(arg1_type, arg1, args) va_start(args, arg1)
# define VA_START_2(arg1_type, arg1, arg2_type, arg2, args) va_start(args, arg2)
#else  /* not WGET_USE_STDARG */
# define VA_START_1(arg1_type, arg1, args) do {	\
  va_start (args);							\
  arg1 = va_arg (args, arg1_type);					\
} while (0)
# define VA_START_2(arg1_type, arg1, arg2_type, arg2, args) do {	\
  va_start (args);							\
  arg1 = va_arg (args, arg1_type);					\
  arg2 = va_arg (args, arg2_type);					\
} while (0)
#endif /* not WGET_USE_STDARG */

/* Portability with pre-ANSI compilers makes these two functions look
   like @#%#@$@#$.  */

#ifdef WGET_USE_STDARG
void
logprintf (enum log_options o, const char *fmt, ...)
#else  /* not WGET_USE_STDARG */
void
logprintf (va_alist)
     va_dcl
#endif /* not WGET_USE_STDARG */
{
  va_list args;
  struct logvprintf_state lpstate;
  int done;

#ifndef WGET_USE_STDARG
  enum log_options o;
  const char *fmt;

  /* Perform a "dry run" of VA_START_2 to get the value of O. */
  VA_START_2 (enum log_options, o, char *, fmt, args);
  va_end (args);
#endif

  check_redirect_output ();
  if (inhibit_logging)
    return;
  CHECK_VERBOSE (o);

  memset (&lpstate, '\0', sizeof (lpstate));
  do
    {
      VA_START_2 (enum log_options, o, char *, fmt, args);
      done = logvprintf (&lpstate, fmt, args);
      va_end (args);
    }
  while (!done);
}

#ifdef DEBUG
/* The same as logprintf(), but does anything only if opt.debug is
   non-zero.  */
#ifdef WGET_USE_STDARG
void
debug_logprintf (const char *fmt, ...)
#else  /* not WGET_USE_STDARG */
void
debug_logprintf (va_alist)
     va_dcl
#endif /* not WGET_USE_STDARG */
{
  if (opt.debug)
    {
      va_list args;
#ifndef WGET_USE_STDARG
      const char *fmt;
#endif
      struct logvprintf_state lpstate;
      int done;

      check_redirect_output ();
      if (inhibit_logging)
	return;

      memset (&lpstate, '\0', sizeof (lpstate));
      do
	{
	  VA_START_1 (char *, fmt, args);
	  done = logvprintf (&lpstate, fmt, args);
	  va_end (args);
	}
      while (!done);
    }
}
#endif /* DEBUG */

/* Open FILE and set up a logging stream.  If FILE cannot be opened,
   exit with status of 1.  */
void
log_init (const char *file, int appendp)
{
  if (file)
    {
      logfp = fopen (file, appendp ? "a" : "w");
      if (!logfp)
	{
	  perror (opt.lfilename);
	  exit (1);
	}
    }
  else
    {
      /* The log goes to stderr to avoid collisions with the output if
         the user specifies `-O -'.  #### Francois Pinard suggests
         that it's a better idea to print to stdout by default, and to
         stderr only if the user actually specifies `-O -'.  He says
         this inconsistency is harder to document, but is overall
         easier on the user.  */
      logfp = stderr;

      /* If the output is a TTY, enable storing, which will make Wget
         remember all the printed messages, to be able to dump them to
         a log file in case SIGHUP or SIGUSR1 is received (or
         Ctrl+Break is pressed under Windows).  */
      if (1
#ifdef HAVE_ISATTY
	  && isatty (fileno (logfp))
#endif
	  )
	{
	  save_context_p = 1;
	}
    }
}

/* Close LOGFP, inhibit further logging and free the memory associated
   with it.  */
void
log_close (void)
{
  int i;

  if (logfp)
    fclose (logfp);
  logfp = NULL;
  inhibit_logging = 1;
  save_context_p = 0;

  for (i = 0; i < SAVED_LOG_LINES; i++)
    free_log_line (i);
  log_line_current = -1;
  trailing_line = 0;
}

/* Dump saved lines to logfp. */
static void
log_dump_context (void)
{
  int num = log_line_current;
  FILE *fp = get_log_fp ();
  if (!fp)
    return;

  if (num == -1)
    return;
  if (trailing_line)
    ROT_ADVANCE (num);
  do
    {
      struct log_ln *ln = log_lines + num;
      if (ln->content)
	fputs (ln->content, fp);
      ROT_ADVANCE (num);
    }
  while (num != log_line_current);
  if (trailing_line)
    if (log_lines[log_line_current].content)
      fputs (log_lines[log_line_current].content, fp);
  fflush (fp);
}

/* When SIGHUP or SIGUSR1 are received, the output is redirected
   elsewhere.  Such redirection is only allowed once. */
enum { RR_NONE, RR_REQUESTED, RR_DONE } redirect_request = RR_NONE;
static const char *redirect_request_signal_name;

/* Redirect output to `wget-log'.  */

static void
redirect_output (void)
{
  char *logfile = unique_name (DEFAULT_LOGFILE);
  fprintf (stderr, _("\n%s received, redirecting output to `%s'.\n"),
	   redirect_request_signal_name, logfile);
  logfp = fopen (logfile, "w");
  if (!logfp)
    {
      /* Eek!  Opening the alternate log file has failed.  Nothing we
         can do but disable printing completely. */
      fprintf (stderr, _("%s: %s; disabling logging.\n"),
	       logfile, strerror (errno));
      inhibit_logging = 1;
    }
  else
    {
      /* Dump the context output to the newly opened log.  */
      log_dump_context ();
    }
  xfree (logfile);
  save_context_p = 0;
}

/* Check whether a signal handler requested the output to be
   redirected. */

static void
check_redirect_output (void)
{
  if (redirect_request == RR_REQUESTED)
    {
      redirect_request = RR_DONE;
      redirect_output ();
    }
}

/* Request redirection at a convenient time.  This may be called from
   a signal handler. */

void
log_request_redirect_output (const char *signal_name)
{
  if (redirect_request == RR_NONE && save_context_p)
    /* Request output redirection.  The request will be processed by
       check_redirect_output(), which is called from entry point log
       functions. */
    redirect_request = RR_REQUESTED;
  redirect_request_signal_name = signal_name;
}
