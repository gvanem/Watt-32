
const char *default_both_phrases[] = {
           "No hostname given!\n",                    /* 0 */
           "Using Port %v1%\n",
           "Could not get host information",
           "Official host name: %v1%\n",
           "Alias %v1%\n",
           "Unsupported address type\n",              /* 5 */
           "Socket Failed",
           "Connect Failed",
           "Connected to %v1%\n",
           "%v1%: Errno %v2%\n",
           "Socket error: Timed out on Read\n",       /* 10 */
           "Socket error: No data to read\n",
           "Socket error",
           "Unable to block signal %v1%\n",
           "Unable to block SIGPIPE, if child dies, we will die\n",
           "Out of memory reading %v1%, ignoring\n",  /* 15 */
           "Out of memory reading argument file\n",
           "Address: %v1%\n"
         };

int nr_both_phrases = sizeof(default_both_phrases) / sizeof(default_both_phrases[0]);

