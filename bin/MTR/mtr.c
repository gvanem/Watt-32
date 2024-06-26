/*
    mtr  --  a network diagnostic tool
    Copyright (C) 1997,1998  Matt Kimball

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <sys/types.h>
#include <config.h>

#if defined(_WIN32) && !defined(USE_WATT32)
  #include <winsock.h>
#else
  #include <netdb.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "mtr-curs.h"
#include "getopt.h"
#include "display.h"
#include "dns.h"
#include "report.h"
#include "net.h"


#ifndef HAVE_SETEUID
/* HPUX doesn't have seteuid, but setuid works fine in that case for us */
#define seteuid setuid
#endif

int DisplayMode;
int display_mode;
int Interactive = 1;
int PrintVersion = 0;
int PrintHelp = 0;
int MaxPing = 16;
float WaitTime = 1.0;
char *Hostname = NULL;
char *InterfaceAddress = NULL;
char LocalHostname[128];
int debug = 0;
int dns = 1;
int packetsize = MINPACKET;

void parse_arg(int argc, char **argv) {
  int opt;
  static struct option long_options[] = {
    { "version", 0, 0, 'v' },
    { "help", 0, 0, 'h' },
    { "report", 0, 0, 'r' },
    { "report-cycles", 1, 0, 'c' },
    { "curses", 0, 0, 't' },
    { "gtk", 0, 0, 'g' },
    { "interval", 1, 0, 'i' },
    { "psize", 1, 0, 'p' },
    { "no-dns", 0, 0, 'n' },
    { "split", 0, 0, 's' },     /* BL */
    { "address", 1, 0, 'a' },
    { "raw", 0, 0, 'l' },
    {"debug", 0, 0, 'd'},
    { 0, 0, 0, 0 }
  };

  opt = 0;
  while(1) {
    opt = getopt_long(argc, argv, "a:dhvrc:tgklnsi:p:", long_options, NULL);
    if(opt == -1)
      break;

    switch(opt) {
    case 'v':
      PrintVersion = 1;
      break;
    case 'h':
      PrintHelp = 1;
      break;
    case 'd':
      debug = 1;
      break;
    case 'r':
      DisplayMode = DisplayReport;
      break;
    case 'c':
      MaxPing = atoi (optarg);
      break;
    case 'p':
      packetsize = atoi (optarg);
      break;
    case 't':
      DisplayMode = DisplayCurses;
      break;
    case 'a':
      InterfaceAddress = optarg;
      break;
    case 'g':
      DisplayMode = DisplayGTK;
      break;
    case 's':                 /* BL */
      DisplayMode = DisplaySplit;
      break;
    case 'l':
      DisplayMode = DisplayRaw;
      break;
    case 'n':
      dns = 0;
      break;
    case 'i':
      WaitTime = atof (optarg);
      if (WaitTime <= 0.0) {
         fprintf (stderr, "mtr: wait time must be positive\n");
         exit (1);
      }
      break;
    }
  }

  if (DisplayMode != DisplayRaw && DisplayMode != DisplayReport)
     debug = 0;

  if ((DisplayMode == DisplayReport) || (DisplayMode == DisplayRaw))
    Interactive = 0;

  if(optind > argc - 1)
    return;

#if 0 /* test */
  if (optind < argc)
    {
      printf ("non-option ARGV-elements: ");
      while (optind < argc)
        printf ("argv[optind]: \"%s\"\n", argv[optind++]);
      printf ("\n");
    }
  exit(0);
#endif

  Hostname = argv[optind++];

  if (argc > optind)
    packetsize = atoi(argv[optind]);
}


void parse_mtr_options (char *string)
{
  int argc;
  char *argv[128], *p;

  if (!string) return;

  argv[0] = "mtr";
  argc = 1;
  p = strtok (string, " \t");
  while (p && (argc < (sizeof(argv)/sizeof(argv[0])))) {
    argv[argc++] = p;
    p = strtok (NULL, " \t");
  }
  if (p) {
    fprintf (stderr, "Warning: extra arguments ignored: %s", p);
  }

  parse_arg (argc, argv);
  optind = 0;
}


int main(int argc, char **argv) {
  int traddr;
  struct hostent *host;
  int net_preopen_result;

#ifdef USE_WATT32 /* must call dbug_init() before socket() in net_preopen() */
  if (argc > 1 && (!strncmp(argv[1],"-d",2) || !strcmp(argv[1],"--debug")))
     dbug_init();
#endif

  /*  Get the raw sockets first thing, so we can drop to user euid immediately  */

  net_preopen_result = net_preopen ();

  display_detect(&argc, &argv);

  parse_mtr_options (getenv ("MTR_OPTIONS"));

  parse_arg(argc, argv);

  if(PrintVersion) {
    printf("mtr " VERSION "\n");
    exit(0);
  }

  if(PrintHelp) {
    printf("usage: %s [-dhvrctglsni] [--help] [--version] [--report]\n"
	   "\t\t[--report-cycles=COUNT] [--curses] [--gtk]\n"
           "\t\t[--raw] [--split] [--no-dns] [--address interface]\n" /* BL */
           "\t\t[--psize=bytes/-p=bytes]\n"            /* ok */
	   "\t\t[--interval=SECONDS] HOSTNAME [PACKETSIZE]\n", argv[0]);
    exit(0);
  }
  if (Hostname == NULL) Hostname = "localhost";

  if(gethostname(LocalHostname, sizeof(LocalHostname))) {
	strcpy(LocalHostname, "UNKNOWNHOST");
  }

  if(net_preopen_result != 0) {
    printf("mtr: Unable to get raw socket.  (Executable not suid?)\n");
    exit(1);
  }


  if(InterfaceAddress) { /* Mostly borrowed from ping(1) code */
    int i1, i2, i3, i4;
    char dummy;
    extern int sendsock; /* from net.c:118 */
    extern struct sockaddr_in sourceaddress; /* from net.c:120 */

    sourceaddress.sin_family = AF_INET;
    sourceaddress.sin_port = 0;
    sourceaddress.sin_addr.s_addr = 0;

    if(sscanf(InterfaceAddress, "%u.%u.%u.%u%c", &i1, &i2, &i3, &i4, &dummy) != 4) {
      printf("mtr: bad interface address: %s\n", InterfaceAddress);
      exit(1);
    }
    else {
      unsigned char*ptr;
      ptr = (unsigned char*)&sourceaddress.sin_addr;
      ptr[0] = i1;
      ptr[1] = i2;
      ptr[2] = i3;
      ptr[3] = i4;
    }

    if(bind(sendsock, (struct sockaddr*)&sourceaddress, sizeof(sourceaddress)) == -1) {
      perror("mtr: failed to bind to interface");
      exit(1);
    }
  }

  host = gethostbyname(Hostname);
  if(host == NULL) {
#ifndef NO_HERROR
    herror("mtr");
#else
    printf("mtr: error looking up \"%s\"\n", Hostname);
#endif
    exit(1);
  }

  traddr = *(int *)host->h_addr;

  if(net_open(traddr) != 0) {
    printf("mtr: Unable to get raw socket.  (Executable not suid?)\n");
    exit(1);
  }

  if (DisplayMode != DisplayRaw)
    display_open();
  dns_open();

  display_mode = 0;
  display_loop();

  net_end_transit();
  display_close();
  net_close();

  return 0;
}


