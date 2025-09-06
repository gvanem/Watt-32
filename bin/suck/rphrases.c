
/*
 * the default phrases
 */
const char *default_rpost_phrases[] =
{
  "Usage: %v1% hostname [@filename] [-s | -S name] [-e | -E name] [-b batch_file -p dir_prefix]",
  "-U userid -P passwd] [-M] [-N portnr] [-T timeout] [-u] [-f filter_args] <RETURN>\n",
  "\tIf used, filter_args MUST be last arguments on line\n",
  "Sorry, Can't post to this host\n",
  "Closing connection to %v1%\n",
  "Bad luck.  You can't use this server.\n",
  "Duplicate article, unable to post\n",
  "Malfunction, Unable to post Article!\n",
  "Invalid argument: %v1%\n",
  "Using Built-In default %v1%\n",
  "No infile specification, aborting\n",
  "No file to process, aborting\n",
  "Empty file, skipping\n",
  "Deleting batch file: %v1%\n",
  "Execl",
  "Fork",
  "Weird Response to Authorization: %v1%\n",
  "Authorization Denied",
  "*** Unexpected response to command: %v1%\n%v2%\n",
  "Invalid argument: %v1%\n",
  "No Host Name Specified\n",
  "No Batch file Specified\n",
  "No prefix Supplied\n",
  "No args to use for filter\n",
  "No Error Log name provided\n",
  "rpost: Status Log",
  "No Status Log name provided\n",
  "No Userid Specified\n",
  "No Password Specified\n",
  "No NNRP port Specified\n",
  "Invalid argument: %v1%, ignoring\n",
  "No language file specified\n",
  "No userid or password provided, unable to auto-authenticate"
};

int nr_rpost_phrases = sizeof (default_rpost_phrases) /
                       sizeof (default_rpost_phrases[0]);
