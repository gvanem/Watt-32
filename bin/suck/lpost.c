
#include <stdio.h>
#include <errno.h>

#include "suck.h"
#include "config.h"

/*
 * get news data from stdin and post it to the local server
 */

int main (int argc, char **argv)
{
  FILE  *pfp = NULL;
  char   line[1024];
  int    count = 0, verbose = 0, retval = 0;
  size_t len;

  if (argc > 1)
    verbose = 1;

  while (gets (line) && retval == 0)
  {
    len = strlen (line);
    if (!pfp)
    {
      if (verbose)
        printf ("posting article %d\n", ++count);
      pfp = popen (RNEWS, "w");
    }
    if (!pfp)
    {
      perror ("Error: cannot open rnews: ");
      retval = -1;
    }
    else if (line[0] == '.' && len == 1)
    {
      /* end of article */
      if (verbose)
        printf ("end of article %d\n", count);

      if (pfp)
      {
	pclose (pfp);
	pfp = NULL;
      }
    }
    else
    {
      fputs (line, pfp);
      putc ('\n', pfp);
    }
  }				/* end while */
  return (retval);
}
