#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_Windows) || !defined(WATT32)
#error "This program is not for Winsock2"
#endif

#define WATT32_BUILD  /* Since "sysdep.h" needs some internals */

#include <sys/socket.h>
#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_packet.h>

#include "sysdep.h"

static const char *pkt_type (int type)
{
  switch (type)
  {
    case PACKET_HOST:
         return ("host");
    case PACKET_BROADCAST:
         return ("bcast");
    case PACKET_MULTICAST:
         return ("mcast");
    case PACKET_OTHERHOST:
         return ("other");
    case PACKET_OUTGOING:
         return ("out");
    case PACKET_LOOPBACK:
         return ("loop");
    case PACKET_FASTROUTE:
         return ("fastr");
  }
  return ("??");
}

static void print_header (void)
{
  puts ("sk proto: type , eth-src");
}

static void packet_recv (int sk)
{
  struct sockaddr_ll         addr;
  const struct ether_header  eth;
  int   addr_len = sizeof(addr);
  const char *eth_src = NULL;

  memset ((void*)&eth, '\0', sizeof(eth));
  recvfrom (sk, (char*)&eth, sizeof(eth), 0,
            (struct sockaddr*)&addr, &addr_len);

#if 0 /* todo: Add the name from '/etc/ethers' */
  eth_src = ether_ntohost (eth.ether_shost);
#endif

  printf ("%d  %04Xh: %-5s %02X:%02X:%02X:%02X:%02X:%02X %s\n",
          sk, ntohs(addr.sll_protocol), pkt_type(addr.sll_pkttype),
          eth.ether_shost[0], eth.ether_shost[1],
          eth.ether_shost[2], eth.ether_shost[3],
          eth.ether_shost[4], eth.ether_shost[5],
          eth_src ? eth_src : "");
}

int main (int argc, char **argv)
{
  BOOL quit = FALSE;
  int  ch, s1, s2;

  printf ("Linux style AF_PACKET example. Press ESC or 'q' to quit\n");

  if (argc >= 2 && !strcmp(argv[1], "-d"))
  {
    /* A "debug.filter = none" in '$(WATTCP.CFG)/wattcp.cfg'
     * is need to see everything that comes in.
     * All TCP-packets would belong to 'NO_SOCKETS'.
     */
    dbug_init();
  }

  s1 = socket (AF_PACKET, SOCK_PACKET, 0);
  if (s1 < 0)
  {
    PERROR ("socket");
    return (-1);
  }

  s2 = socket (AF_PACKET, SOCK_PACKET, 0);
  if (s2 < 0)
  {
    PERROR ("socket");
    close (s1);
    return (-1);
  }

  print_header();

  while (!quit)
  {
    struct timeval tv = { 1, 0 };
    fd_set rd;
    int    num;

    FD_ZERO (&rd);
    FD_SET (s1, &rd);
    FD_SET (s2, &rd);
    FD_SET (STDIN_FILENO, &rd);
    num = max (s1, s2) + 1;

    if (select_s(num, &rd, NULL, NULL, &tv) < 0)
    {
      PERROR ("select_s");
      quit = TRUE;
    }

    if (FD_ISSET(STDIN_FILENO, &rd))
    {
      ch = getch();
      if (ch == 'q' || ch == 27)
         quit = TRUE;
    }

    if (FD_ISSET(s1, &rd))
       packet_recv (s1);

    if (FD_ISSET(s2, &rd))
       packet_recv (s2);
  }

  close (s1);
  close (s2);
  return (0);
}

