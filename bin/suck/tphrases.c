
const char *default_test_phrases[] = {
           "Need date time args (YYMMDD HHMMSS)\n",
           "Invalid date time args %v1% %v2%\n",
           "No Error Log name provided\n",
           "testhost: Status Log",
           "No port number provided\n",
           "No Userid Specified\n",
           "No Password Specified\n",
           "Invalid argument:%v1%\n",
           "Error talking to host %v1%\n",
           "Weird Response to Authorization: %v1%\n",
           "Authorization Denied\n",
           "No Phrases file provided\n",
           "No Status Log name provided\n",
           "No timeout value provided\n"
         };

int nr_test_phrases = sizeof (default_test_phrases) / sizeof (default_test_phrases[0]);
