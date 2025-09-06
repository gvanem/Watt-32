#include <stdio.h>
#include <stdlib.h>

#include "suck.h"
#include "config.h"
#include "both.h"
#include "dedupe.h"
#include "utils.h"
#include "phrases.h"
#include "timer.h"

/*
 * this is almost an exact duplicate of the chkhistory() routine
 */

/*
 * We'll build an array of pointers to the linked list, based on a
 * character of the article id. Then we only have to check the article
 * ids that match the character, saving searching thru the list
 */

#define NR_LETTERS       128  /* nr of chars indexed, only 128 since rfc doesn't support > 128 */
#define LETTER_TO_CHECK  2    /* which letter to check in MSG_ID */

void dedupe_list (PMaster master)
{
  int   i, j, x, nrfound = 0;
  PList curr, prev, *lets;
  long  tlet[NR_LETTERS];
  struct {
    long nr;
    long start;
  } letters[NR_LETTERS];

  print_phrases (master->msgs, dedupe_phrases[0], NULL);
  fflush (master->msgs);	/* so msg gets printed */

  TimerFunc (TIMER_START, 0L, NULL);

  /* pass one. count the letters, so we can build the array and also
   * index it
   */

  /* initialize the arrays */
  for (i = 0; i < NR_LETTERS; i++)
    letters[i].nr = letters[i].start = tlet[i] = 0;

  curr = master->head;
  while (curr)
  {
    letters[((curr->msgnr[LETTER_TO_CHECK]) % NR_LETTERS)].nr++;
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
     error_log (ERRLOG_REPORT, dedupe_phrases[1], NULL);
  else
  {
    /* now put them in place */
    curr = master->head;
    while (curr)
    {
      j = (curr->msgnr[LETTER_TO_CHECK]) % NR_LETTERS;
      i = letters[j].start + tlet[j];
      tlet[j]++;		/* so go to next slot */
      lets[i] = curr;
      curr = curr->next;
    }

    /* pass three.
     * now we can work our way down the linked list
     * and check against letter only
     */
    curr = master->head;
    while (curr)
    {
      j = (curr->msgnr[LETTER_TO_CHECK]) % NR_LETTERS;
      for (i = 0; i < letters[j].nr; i++)
      {
	x = letters[j].start + i;
	/* need the curr != lets[x] so that we don't delete ourself */
        if (lets[x] && curr != lets[x] && cmp_msgid (curr->msgnr, lets[x]->msgnr) == TRUE)
	{
	  nrfound++;
	  if (master->debug == TRUE)
            do_debug ("Matched %s, nuking\n", curr->msgnr);

	  /* now flag it for deletion */
	  lets[x]->msgnr[0] = '\0';	/* no more article nr */
	  lets[x] = NULL;	/* so don't check it again */
	}
      }
      curr = curr->next;
    }
    /* pass four
     * now go thru and delete em
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
  print_phrases (master->msgs, dedupe_phrases[2], str_int(master->nritems),
                str_int(nrfound), NULL);
}
