#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "suck.h"
#include "config.h"

/* create a text file with the default*phrases arrays in them
 * and create phrases.h
 */
#define PHRASES_H "phrases.h"

extern const char *default_both_phrases[];
extern const char *default_rpost_phrases[];
extern const char *default_test_phrases[];
extern const char *default_suck_phrases[];
extern const char *default_timer_phrases[];
extern const char *default_chkh_phrases[];
extern const char *default_dedupe_phrases[];
extern const char *default_killf_reasons[];
extern const char *default_killf_phrases[];
extern const char *default_killp_phrases[];
extern const char *default_sucku_phrases[];
extern const char *default_lmove_phrases[];
extern const char *default_active_phrases[];
extern const char *default_batch_phrases[];

extern int nr_both_phrases;
extern int nr_rpost_phrases;
extern int nr_test_phrases;
extern int nr_suck_phrases;
extern int nr_timer_phrases;
extern int nr_chkh_phrases;
extern int nr_dedupe_phrases;
extern int nr_killf_reasons;
extern int nr_killf_phrases;
extern int nr_killp_phrases;
extern int nr_sucku_phrases;
extern int nr_lmove_phrases;
extern int nr_active_phrases;
extern int nr_batch_phrases;

int do_one_array (FILE *, int, const char **);
int do_phrases_h (void);
int count_vars (const char **, int);

int main (int argc, char **argv)
{
  int   retval = 0;
  FILE *fpo;

  if (argc != 2)
  {
    fprintf (stderr, "Usage: %s outfile <RETURN>\n", argv[0]);
    retval = -1;
  }
  else if (!strcmp(argv[1],"phrases.h"))
  {
    retval = do_phrases_h();
  }
  else if ((fpo = fopen(argv[1],"w")) == NULL)
  {
    perror (argv[1]);
    retval = -1;
  }
  else
  {				/* first put version number */
    fprintf (fpo, "%s\n", SUCK_VERSION);

    retval = do_one_array (fpo, nr_both_phrases, default_both_phrases);
    if (retval == 0)
        retval = do_one_array (fpo, nr_rpost_phrases, default_rpost_phrases);

    if (retval == 0)
        retval = do_one_array (fpo, nr_test_phrases, default_test_phrases);

    if (retval == 0)
        retval = do_one_array (fpo, nr_suck_phrases, default_suck_phrases);

    if (retval == 0)
        retval = do_one_array (fpo, nr_timer_phrases, default_timer_phrases);

    if (retval == 0)
        retval = do_one_array (fpo, nr_chkh_phrases, default_chkh_phrases);

    if (retval == 0)
        retval = do_one_array (fpo, nr_dedupe_phrases, default_dedupe_phrases);

    if (retval == 0)
        retval = do_one_array (fpo, nr_killf_reasons, default_killf_reasons);

    if (retval == 0)
        retval = do_one_array (fpo, nr_killf_phrases, default_killf_phrases);

    if (retval == 0)
        retval = do_one_array (fpo, nr_killp_phrases, default_killp_phrases);

    if (retval == 0)
        retval = do_one_array (fpo, nr_sucku_phrases, default_sucku_phrases);

    if (retval == 0)
        retval = do_one_array (fpo, nr_lmove_phrases, default_lmove_phrases);

    if (retval == 0)
        retval = do_one_array (fpo, nr_active_phrases, default_active_phrases);

    if (retval == 0)
        retval = do_one_array (fpo, nr_batch_phrases, default_batch_phrases);

    fclose (fpo);
    if (retval == -1)
         perror (argv[1]);
    else printf ("Created %s\n", argv[1]);
  }
  return (retval);
}

int do_one_array (FILE * fpo, int nr, const char **phrases)
{
  int i, x;
  int retval = 0;

  for (i = 0; i < nr && retval == 0; i++)
  {
    fputc ('"', fpo);
    for (x = 0; x < strlen (phrases[i]); x++)
    {
      /* handle control codes */
      switch (phrases[i][x])
      {
        case '\r':
	     fputs ("\\r", fpo);
	     break;
        case '\t':
	     fputs ("\\t", fpo);
	     break;
        case '\n':
	     fputs ("\\n", fpo);
	     break;
        default:
	     fputc (phrases[i][x], fpo);
	     break;
      }
    }
    if (fputs ("\"\n", fpo) == EOF)
      retval = -1;
  }
  return (retval);
}

/*
 * generate phrases.h
 */
int do_phrases_h (void)
{ 
  int   i, maxvar, retval = 0;
  FILE *fpo = fopen (PHRASES_H, "w");

  if (!fpo)
  {
    perror (PHRASES_H);
    retval = -1;
  }
  else
  {
    maxvar = count_vars (default_both_phrases, nr_both_phrases);
    i      = count_vars (default_rpost_phrases,nr_rpost_phrases);
    if (i > maxvar)
      maxvar = i;

    i = count_vars (default_test_phrases, nr_test_phrases);
    if (i > maxvar)
      maxvar = i;

    i = count_vars (default_suck_phrases, nr_suck_phrases);
    if (i > maxvar)
      maxvar = i;

    i = count_vars (default_timer_phrases, nr_timer_phrases);
    if (i > maxvar)
      maxvar = i;

    i = count_vars (default_chkh_phrases, nr_chkh_phrases);
    if (i > maxvar)
      maxvar = i;

    i = count_vars (default_dedupe_phrases, nr_dedupe_phrases);
    if (i > maxvar)
      maxvar = i;

    i = count_vars (default_killf_reasons, nr_killf_reasons);
    if (i > maxvar)
      maxvar = i;

    i = count_vars (default_killf_phrases, nr_killf_phrases);
    if (i > maxvar)
      maxvar = i;

    i = count_vars (default_sucku_phrases, nr_sucku_phrases);
    if (i > maxvar)
      maxvar = i;

    i = count_vars (default_lmove_phrases, nr_lmove_phrases);
    if (i > maxvar)
      maxvar = i;

    i = count_vars (default_active_phrases, nr_active_phrases);
    if (i > maxvar)
      maxvar = i;

    i = count_vars (default_batch_phrases, nr_batch_phrases);
    if (i > maxvar)
      maxvar = i;

    fprintf (fpo, "/* AUTOMATICALLY GENERATED BY makephrases */\n");
    fprintf (fpo, "/* DO NOT EDIT */\n\n");
    fprintf (fpo, "#define NR_BOTH_PHRASES %d\n",   nr_both_phrases);
    fprintf (fpo, "#define NR_RPOST_PHRASES %d\n",  nr_rpost_phrases);
    fprintf (fpo, "#define NR_TEST_PHRASES %d\n",   nr_test_phrases);
    fprintf (fpo, "#define NR_SUCK_PHRASES %d\n",   nr_suck_phrases);
    fprintf (fpo, "#define NR_TIMER_PHRASES %d\n",  nr_timer_phrases);
    fprintf (fpo, "#define NR_CHKH_PHRASES %d\n",   nr_chkh_phrases);
    fprintf (fpo, "#define NR_DEDUPE_PHRASES %d\n", nr_dedupe_phrases);
    fprintf (fpo, "#define NR_KILLF_REASONS %d\n",  nr_killf_reasons);
    fprintf (fpo, "#define NR_KILLF_PHRASES %d\n",  nr_killf_phrases);
    fprintf (fpo, "#define NR_KILLP_PHRASES %d\n",  nr_killp_phrases);
    fprintf (fpo, "#define NR_SUCKU_PHRASES %d\n",  nr_sucku_phrases);
    fprintf (fpo, "#define NR_LMOVE_PHRASES %d\n",  nr_lmove_phrases);
    fprintf (fpo, "#define NR_ACTIVE_PHRASES %d\n", nr_active_phrases);
    fprintf (fpo, "#define NR_BATCH_PHRASES %d\n\n",nr_batch_phrases);

    fprintf (fpo, "#define PHRASES_MAX_NR_VARS %d\n\n", maxvar);

    fprintf (fpo, "extern char *default_both_phrases[];\n");
    fprintf (fpo, "extern char *default_rpost_phrases[];\n");
    fprintf (fpo, "extern char *default_test_phrases[];\n");
    fprintf (fpo, "extern char *default_suck_phrases[];\n");
    fprintf (fpo, "extern char *default_timer_phrases[];\n");
    fprintf (fpo, "extern char *default_chkh_phrases[];\n");
    fprintf (fpo, "extern char *default_dedupe_phrases[];\n");
    fprintf (fpo, "extern char *default_killf_reasons[];\n");
    fprintf (fpo, "extern char *default_killf_phrases[];\n");
    fprintf (fpo, "extern char *default_killp_phrases[];\n");
    fprintf (fpo, "extern char *default_sucku_phrases[];\n");
    fprintf (fpo, "extern char *default_lmove_phrases[];\n");
    fprintf (fpo, "extern char *default_active_phrases[];\n");
    fprintf (fpo, "extern char *default_batch_phrases[];\n");

    fprintf (fpo, "extern char **both_phrases;\n");
    fprintf (fpo, "extern char **rpost_phrases;\n");
    fprintf (fpo, "extern char **test_phrases;\n");
    fprintf (fpo, "extern char **suck_phrases;\n");
    fprintf (fpo, "extern char **timer_phrases;\n");
    fprintf (fpo, "extern char **chkh_phrases;\n");
    fprintf (fpo, "extern char **dedupe_phrases;\n");
    fprintf (fpo, "extern char **killf_reasons;\n");
    fprintf (fpo, "extern char **killf_phrases;\n");
    fprintf (fpo, "extern char **killp_phrases;\n");
    fprintf (fpo, "extern char **sucku_phrases;\n");
    fprintf (fpo, "extern char **lmove_phrases;\n");
    fprintf (fpo, "extern char **active_phrases;\n");
    fprintf (fpo, "extern char **batch_phrases;\n");

    fclose (fpo);

    printf ("Created phrases.h\n");
  }
  return (retval);
}

/*
 * find highest nr of vars in any string in the array
 * var == %v#%
 */
int count_vars (const char **phrases, int nr)
{
  int i, x, max, nrline, match, nrat, len;

  max = 0;
  for (i = 0; i < nr; i++)
  {
    x = 0;
    nrline = 0;
    len = strlen (phrases[i]);
    while (x < len - 3)
    {
      match = 0;
      if (phrases[i][x] == PHRASES_SEPARATOR && phrases[i][x+1] == PHRASES_VAR_CHAR)
      {
	nrat = x + 2;
	while (isdigit (phrases[i][nrat]))
          nrat++;

        if (x < len-3 && nrat > x+2 && phrases[i][nrat] == PHRASES_SEPARATOR)
	{
	  match = 1;
	  nrline++;
	  x = nrat + 1;
	}
      }
      if (match == 0)
        x++;
    }
    if (nrline > max)
      max = nrline;
  }
  return (max);
}
