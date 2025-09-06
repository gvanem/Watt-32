#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#define LENGTHLEN 8
#define DEBUG     1

#if DEBUG
  #define N_DEBUG "debug.child"
  #define DEBUGF(args) _do_debug args
  void _do_debug (const char *fmt,...);
#else
  #define DEBUGF(args) ((void)0)
#endif

int
main (int argc, char *argv[])
{
  char buf[4096];
  int len, i, nron;

  nron = 0;
  do
  {
    i = read (0, buf, LENGTHLEN);
    buf[LENGTHLEN] = '\0';
    DEBUGF (("Got %d bytes '%s'\n", i, buf));
    len = atoi (buf);
    if (len > 0)
    {
      nron++;
      i = read (0, buf, len);
      DEBUGF (("Got %d bytes\n", i));
      if ((nron % 2) == 0)
      {
        i = write (1, "0\n", 2);
        DEBUGF (("Wrote %d bytes\n", i));
      }
      else
      {
        i = write (1, "1\n", 2);
        DEBUGF (("Wrote %d bytes\n", i));
      }
    }
  }
  while (len > 0);
}

#if DEBUG
void _do_debug (const char *fmt, ...)
{
  FILE   *fptr = NULL;
  va_list args;

  if ((fptr = fopen(N_DEBUG, "a")) == NULL)
     fptr = stderr;

  va_start (args, fmt);
  vfprintf (fptr, fmt, args);
  va_end (args);

  if (fptr != stderr)
     fclose (fptr);
}
#endif
