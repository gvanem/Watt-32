
#include <stdio.h>
#include <time.h>

#include "suck.h"
#include "config.h"
#include "timer.h"
#include "phrases.h"
#include "both.h"

/*
 * Return elapsed time, in seconds
 */
static double get_elapsed (struct timeval *start)
{
  struct timeval curr;
  double elapsed;

  gettimeofday (&curr, NULL);
  elapsed  = curr.tv_sec - start->tv_sec;
  elapsed += ((double) (curr.tv_usec - start->tv_usec)) / 1000000.0;
  return (elapsed);
}  

double TimerFunc (int which_function, long nradd, FILE * fpi)
{
  static struct timeval start;
  static long nrbytes = 0L;
  char   strmins[32], strsecs[32], strbps[32];
  char   strbytes[32];
  double elapsed, bps, retval = 0.0;
  long   mins;

  switch (which_function)
  {
    case TIMER_START:
	 nrbytes = 0L;
         gettimeofday (&start, NULL);
         break;
    case TIMER_ADDBYTES:
	 nrbytes += nradd;
	 break;
    case TIMER_GET_BPS:
	 elapsed = get_elapsed (&start);
	 retval = (elapsed > 0.0) ? nrbytes / elapsed : 0.0;
	 break;
    case TIMER_DISPLAY:
         if (nrbytes > 0)
	 {
	   elapsed = get_elapsed (&start);
	   bps = (elapsed > 0.0) ? nrbytes / elapsed : 0.0;
	   sprintf (strbps, "%.1f", bps);
	   print_phrases (fpi, timer_phrases[2], strbps, NULL);
	 }
         break;
    case TIMER_TIMEONLY:
         elapsed = get_elapsed (&start);
	 mins = ((long) elapsed) / 60;	/* get minutes */
	 elapsed -= (mins * 60);	/* subtract to get remainder */
	 sprintf (strmins, "%ld", mins);
	 sprintf (strsecs, "%.2f", elapsed);
	 print_phrases (fpi, timer_phrases[0], strmins, strsecs, NULL);
         break;
    case TIMER_TOTALS:
	 sprintf (strbytes, "%ld", nrbytes);
         elapsed = get_elapsed (&start);
	 bps = (elapsed > 0.0 && nrbytes > 0) ? nrbytes / elapsed : 0.0;
	 mins = ((long) elapsed) / 60;	/* get minutes */
	 elapsed -= (mins * 60);	/* subtract to get remainder */
	 sprintf (strmins, "%ld", mins);
	 sprintf (strsecs, "%.2f", elapsed);
	 sprintf (strbps, "%.1f", bps);
	 print_phrases (fpi, timer_phrases[1], strbytes, strmins, strsecs, strbps, NULL);
	 break;
    default:
	 /* ignore invalid commands */
	 break;
  }
  return (retval);
}


