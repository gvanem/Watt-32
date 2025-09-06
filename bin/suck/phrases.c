const char *default_timer_phrases[] =
{
  "Elapsed Time = %v1% mins %v2% seconds\n",
  "%v1% Bytes received in %v2% mins %v3% secs, BPS = %v4%\n",
  "%v1% BPS",
  "%v1% Bytes received\n"
};

const char *default_chkh_phrases[] =
{
  "Can not open %v1%, Skipping\n",
  "Processing History File ",
  "Bogus line, ignoring: %v1%",
  "Processed history, %v1% dupes removed\n",
  "Out of Memory, skipping history check",
  "Processing History Database"
};

const char *default_dedupe_phrases[] =
{
  "Deduping ",
  "Out of Memory, skipping Dedupe\n",
  "Deduped, %v1% items remaining, %v2% dupes removed.\n",
};

const char *default_killf_reasons[] =
{
  "No Reason - major screwup",
  "Over Hi-Line parameter",
  "Under Low-Line parameter",
  "Too many Newsgroups",
  "Not matched in Group Keep file",
  "Multiple Matches/Tiebreaker Used",
  "Header match:",
  "Body match:",
  "Body Size exceeded, size=",
  "Body Size too small, size="
};

const char *default_killf_phrases[] =
{
  "Out of memory\n",
  "Out of memory, can't use killfiles\n",
  "Invalid parameter line %v1%\n",
  "Unable to log killed article\n",
  "ARTICLE KILLED: group %v1% - %v2% - MsgID %v3%\n",
  "Out of memory, skipping killfile regex processing\n",
  "No character to process for quote, ignoring\n",
  "Nothing to scan for in header scan, ignoring\n",
  "Out of memory, skipping killfile header scan processing\n",
  "Unable to write article to disk\n",
  "Nothing to scan for in body scan, ignoring\n",
  "Invalid regular expression, ignoring, %v1% - %v2%\n"
  "No character to process for non-regex, ignoring\n",
};

const char *default_killp_phrases[] =
{
  "Out of memory processing killprg args",
  "Setting up pipes",
  "Out of memory processing path",
  "%v1%: Unable to find, sorry",
  "Unable to write to child process",
  "Unable to read from child process",
  "Process Error",
  "Child Process died",
  "Child process closed pipe, waiting for child to end\n",
  "Child exited with a status of %v1%\n",
  "Child exited due to signal %v1%\n",
  "Child died, cause of death unknown\n",
  "killprg: Unable to log killed article",
  "killprg: ARTICLE KILLED: MsgID %v1%\n",

};

const char *default_sucku_phrases[] =
{
  "%v1%: Not a directory\n",
  "%v1%: Write permission denied\n",
  "Lock File %v1%, Invalid PID, aborting\n",
  "Lock File %v1%, stale PID, removed\n",
  "Lock File %v1%, stale PID, Unable to remove, aborting\n",
  "Lock file %v1%, PID exists, aborting\n",
  "Unable to create Lock File %v1%\n",
  "Error writing to file %v1%\n",
  "Error reading from file %v1%\n",
  "Invalid call to move_file(), notify programer\n"
};

const char *default_suck_phrases[] =
{
  "suck: status log",
  "Attempting to connect to %v1%\n",
  "Initiating restart at article %v1% of %v2%\n",
  "No articles\n",
  "Closed connection to %v1%\n",
  "BPS",
  "No local host name\n",
  "Cleaning up after myself\n",
  "Skipping Line: %v1%",
  "Invalid Line: %v1%",
  "Invalid number for maxread field: %v1% : ignoring\n",
  "GROUP <%v1%> not found on host\n",
  "GROUP <%v1%>, unexpected response, %v2%\n",
  "%v1% - %v2%...High Article Nr is low, did host reset its counter?\n",
  "%v1% - limiting # of articles to %v2%\n",
  "%v1% - %v2% articles %v3%-%v4%\n",
  "%v1% Articles to download\n",
  "Processing Supplemental List\n",
  "Supplemental Invalid Line: %v1%, ignoring\n",
  "Supplemental List Processed, %v1% articles added, %v2% articles to download\n",
  "Total articles to download: %v1%\n",
  "Invalid Article Nr, skipping: %v1%\n",
  "Out of Memory, aborting\n",
  "Whoops, %v1% exists, %v2% doesn't, unable to restart.\n",
  "Signal received, will finish downloading article.\n",
  "Notify programmer, problem with pause_signal()\n",
  "\nGot Pause Signal, swapping pause values\n",
  "Weird Response to Authorization: %v1%",
  "Authorization Denied",
  "***Unexpected response to command, %v1%\n%v2%\n",
  "Moving newsrc to backup",
  "Moving newrc to newsrc",
  "Removing Sorted File",
  "Removing Supplemental Article file",
  "Invalid argument: %v1%\n",
  "No rnews batch file size provided\n",
  "No postfix provided\n",
  "No directory name provided\n",
  "Invalid directory\n",
  "Invalid Batch arg: %v1%\n",
  "No Batch file name provided\n",
  "No Error Log name provided\n",
  "No Status Log name provided\n",
  "No Userid provided\n",
  "No Passwd provided\n",
  "No Port Number provided\n",
  "Invalid pause arguments\n",
  "No phrase file provided\n",
  "GROUP command not recognized, try the -M option\n",
  "No number of reconnects provided\n",
  "No host to post to provided\n",
  "No host name specified\n",
  "No timeout value specified\n",
  "No Article Number provided, can't use MsgNr mode\n",
  "Switching to MsgID mode\n",
  "Userid or Password not provided, unable to auto-authenticate\n",
  "No local active file provided\n",
  "Error writing article to file\n",
  "Can't pre-batch files, no batch mode specified\n",
  "Broken connection, aborting\n",
  "Restart: Skipping first article\n",
  "No kill log file name provied\n",
};

const char *default_batch_phrases[] =
{
  "Calling lmove\n",
  "Forking lmove\n",
  "Running lmove\n",
  "Building INN Batch File\n",
  "Building RNews Batch File(s)\n",
  "Posting Messages to %v1%\n",
  "No batchfile to process\n",
  "Invalid batchfile line %v1%, ignoring",
  "Can't post: %v1% : reason : %v2%",
  "Error posting: %v1% : reason : %v2%",
  "%v1% Messages Posted\n",
  "Article Not Wanted, deleting: %v1% - %v2%",
  "Article Rejected, deleting: %v1% - %v2%",
};

const char *default_active_phrases[] =
{
  "Unable to get active list from local host\n",
  "Out of memory reading local list\n",
  "Loading active file from %v1%\n",
  "Invalid group line in sucknewsrc, ignoring: %v1%\n",
  "Deleted group in sucknewsrc, ignoring: %v1%",
  "%v1% Articles to download\n",
  "No sucknewsrc to read, creating\n",
  "Out of memory reading ignore list, skipping\n",
  "Adding new groups from local active file to sucknewsrc\n",
  "Reading current sucknewsrc\n",
  "New Group - adding to sucknewsrc: %v1%\n",
  "Unable to open %v1%\n",
  "Active-ignore - error in expression %v1% - %v2%\n",
};

#define DIM(x) sizeof(x)   / sizeof(x[0])

int nr_suck_phrases   = DIM (default_suck_phrases);
int nr_timer_phrases  = DIM (default_timer_phrases);
int nr_chkh_phrases   = DIM (default_chkh_phrases);
int nr_dedupe_phrases = DIM (default_dedupe_phrases);
int nr_killf_reasons  = DIM (default_killf_reasons);
int nr_killf_phrases  = DIM (default_killf_phrases);
int nr_killp_phrases  = DIM (default_killp_phrases);
int nr_sucku_phrases  = DIM (default_sucku_phrases);
int nr_active_phrases = DIM (default_active_phrases);
int nr_batch_phrases  = DIM (default_batch_phrases);

