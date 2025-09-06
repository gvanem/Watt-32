/*
 * the default phrases
 */
const char *default_lmove_phrases[] = {
  "Invalid argument: %v1%, ignoring\n",
  "No Error Log name provided\n",
  "lmove: Status Log",
  "No Status Log name provided\n",
  "No language file specified\n",
  "No Base Directory provided\n",                   /* 5 */
  "No active file name provided, using default.\n",
  "No Article Directory provided\n",
  "Out of memory loading active file\n",
  "Invalid line in active file, ignoring: %v1%",
  "Error with directory : %v1% : aborting\n",       /* 10 */
  "No Newsgroups line in file %v1%, ignoring\n",
  "Couldn't find valid newsgroups for file %v1%, ignoring\n",
  "Error writing to %v1%, aborting\n",
  "Lock File %v1%, Invalid PID, aborting\n",
  "Lock File %v1%, stale PID, removed\n",           /* 15 */
  "Lock File %v1%, stale PID, Unable to remove, aborting\n",
  "Lock file %v1%, PID exists, aborting\n",
  "Unable to create Lock File %v1%\n",
  "No configuration file name provided\n",
  "Invalid configuration file format\n",            /* 20 */
};

int nr_lmove_phrases = sizeof(default_lmove_phrases) / sizeof(default_lmove_phrases[0]);
