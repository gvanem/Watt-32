#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "suck.h"
#include "config.h"
#include "both.h"
#include "chkhist.h"
#include "utils.h"
#include "phrases.h"
#include "timer.h"


/*
 * In order to not check the entire linked list every time, let's try
 * something different.
 * We'll build an array of pointers to the linked list, based on the first
 * character of the article id then we only have to check the article ids
 * that match the first character, saving searching thru the list
 */

#define NR_LETTERS 128    /* nr of chars indexed, only 128 since rfc
                           * doesn't support > 128
                           */

void chkhistory (PMaster master)
{
  FILE  *fhist;
  char   linein[MAXLINLEN + 1], *ptr;
  int    i, j, x, found, nrfound = 0;
  PList  curr, prev, *lets;
  long   tlet[NR_LETTERS];
  struct {
    long nr;
    long start;
  } letters[NR_LETTERS];

  fhist = fopen (HISTORY_FILE, "r");
  if (!fhist)
     error_log (ERRLOG_REPORT, chkh_phrases[0], HISTORY_FILE, NULL);
  else
  {
    print_phrases (master->msgs, chkh_phrases[1], NULL);
    fflush (master->msgs);	/* so msg gets printed */
    TimerFunc (TIMER_START, 0L, NULL);

    /* pass one. count the letters, so we can build the array and
     * also index it
     */
    /* initialize the arrays */
    for (i = 0; i < NR_LETTERS; i++)
      letters[i].nr = letters[i].start = tlet[i] = 0;

    curr = master->head;
    while (curr)
    {
      letters[((curr->msgnr[1]) % NR_LETTERS)].nr++;
      /* the % is in case a wacko article id slips by */
      curr = curr->next;
    }
    /* now build the array with the starting points for each
     * nr 0 starts on zero so skip it
     */
    for (i = 1; i < NR_LETTERS; i++)
      letters[i].start = letters[i - 1].start + letters[i - 1].nr;

    /* pass two. first malloc the array
     */
    lets = calloc (master->nritems, sizeof(PList));
    if (!lets)
      error_log (ERRLOG_REPORT, chkh_phrases[4], NULL);
    else
    {
      /* now put them in place */
      curr = master->head;
      while (curr)
      {
	j = (curr->msgnr[1]) % NR_LETTERS;
	i = letters[j].start + tlet[j];
	tlet[j]++;		/* so go to next slot */
	lets[i] = curr;
	curr = curr->next;
      }
      /* pass three. now we can read history file and check against
       * first letter only
       */
      while (fgets (linein, MAXLINLEN, fhist))
      {
        j   = (linein[1]) % NR_LETTERS;
        ptr = strchr (linein, '>');
        if (!ptr)
          error_log (ERRLOG_REPORT, chkh_phrases[2], linein, NULL);
        else
	{
	  *(ptr + 1) = '\0';
	  /* now that we've isolated the article nr, search our list */
	  found = FALSE;
	  for (i = 0; i < letters[j].nr && found == FALSE; i++)
	  {
	    x = letters[j].start + i;
            if (lets[x] && cmp_msgid (linein, lets[x]->msgnr) == TRUE)
	    {
	      nrfound++;
	      found = TRUE;
	      /* now flag it for deletion */
	      lets[x]->msgnr[0] = '\0';		/* no more article nr */
	      lets[x] = NULL;	/* so don't check it again */
	    }
	  }
	}
      }
      /* pass four. now go thru and delete em
       */
      curr = master->head;
      prev = NULL;
      while (curr)
      {
        if (!curr->msgnr[0])
	{
	  /* nuke it */
	  master->nritems--;
          if (!prev)
	  {
	    /* remove master node */
	    master->head = curr->next;
	    free_one_node (curr);
	    curr = master->head;
	  }
	  else
	  {
	    prev->next = curr->next;
	    free_one_node (curr);
	    curr = prev->next;
	  }
	}
	else
	{
	  prev = curr;
	  curr = curr->next;
	}
      }
      free (lets);
    }
    TimerFunc (TIMER_TIMEONLY, 0l, master->msgs);
    fclose (fhist);
    print_phrases (master->msgs, chkh_phrases[3], str_int (nrfound), NULL);
  }
}
