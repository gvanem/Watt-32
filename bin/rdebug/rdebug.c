#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>   /* sockaddr_in, htons, in_addr */
#include <netdb.h>        /* hostent, gethostby*, getservby* */
#include <arpa/inet.h>

#ifdef WATT32
#include <tcp.h>
#include <dos.h>

#define read  read_s
#define write write_s
#define close close_s
#endif

#include "getopt.h"

extern int errno;

#ifdef WATT32
#define USE_PORT_IO      /* read/write I/O ports */
#define USE_DIRECT_MEM   /* peek/poke of memory */
#endif

#ifdef __DJGPP__
#include <pc.h>
#include <go32.h>
#include <sys/farptr.h>
#define peekb(s,o)     _farpeekb (_dos_ds, (o)+((s)<<4))
#define pokeb(s,o,x)   _farpokeb (_dos_ds, (o)+((s)<<4), x)
#endif

#ifdef __WATCOMC__
#include <conio.h>
#define peekb(s,o)     *(volatile char*) ((o) + ((s)<<4))
#define pokeb(s,o,x)   *(volatile char*) ((o) + ((s)<<4)) = (x)
#define inportb(p)     inp(p)
#define outportb(p,x)  outp(p,x)
#endif

#ifdef __HIGHC__
#include <conio.h>
#include <pharlap.h>
#define peekb(s,o)     PeekRealByte (((s) << 16) + (o))
#define pokeb(s,o,x)   PokeRealByte (((s) << 16) + (o), (x))
#define inportb(p)     inp(p)
#define outportb(p,x)  outp(p,x)
#endif

#define DEBUG(a) fprintf(stderr, a)
//#define DEBUG(a)

#define RDEBUG_PORT 10101

int port = RDEBUG_PORT, verbose = 0;
char *whitespace = " \t\r\n";

void err(const char *s)
{
#if 0
  fprintf(stderr, "Error: %s %s\n", s, strerror(errno));
#else
  perror (s);
#endif
  exit(1);
}

int setup_server(int port)
{
  struct sockaddr_in addrin;
  int addrinlen = sizeof(addrin);
  int sock;

  if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    err("socket: ");
  DEBUG("Setting up server: socket, ");
  addrin.sin_family = AF_INET;
  addrin.sin_port = htons(port);
  addrin.sin_addr.s_addr = 0;
  if (bind(sock, (struct sockaddr *)&addrin, addrinlen) < 0)
    err("bind: ");
  DEBUG("bind, ");
  if (listen(sock, 2) < 0)
    err("listen: ");
  DEBUG("listen\n");
  return (sock);
}

int read_line(int net, char *buf, int buflen)
{
  char c;
  char *p;

  for (p = buf; p < &buf[buflen-1]; p++) {
    p[1] = '\0';
    if (read(net, &c, 1) <= 0)
      return (0);
    if ((*p = c) == '\n')
      break;
  }
  return (p - buf + 1);
}

int parse_line(char *buf, char **argv, int maxarg)
{
  int i;

  for (i = 0; i < maxarg; i++) {
    buf += strspn(buf, whitespace);
    if (*buf == '\0')
      break;
    argv[i] = buf;
    buf += strcspn(buf, whitespace);
  }
  return (i);
}

void in_port(int net, int argc, unsigned long arg1)
{
  char  reply[5];

  if (argc < 2)
    write(net, "?\r\n", 3);
  else {
#ifdef USE_PORT_IO
    sprintf(reply, "%02X\r\n", inportb((int)arg1) & 0xFF);
#else
    sprintf(reply, "%02X\r\n", (int)arg1 & 0xFF);
#endif
    write(net, reply, sizeof(reply)-1);
  }
}

void out_port(int net, int argc, unsigned long arg1, unsigned long arg2)
{
  if (argc < 3)
    write(net, "?\r\n", 3);
  else {
#ifdef USE_PORT_IO
    outportb((int)arg1, (int)arg2);
#endif
    write(net, "0\r\n", 3);
  }
}

void dump_mem(int net, int argc, unsigned long arg1, unsigned long arg2)
{
  int i;
  unsigned long l, loc;
  char line[16*3+3], *p;  /* 16 values + \r\n\0 */

  if (argc < 3)
    arg2 = 1;
  for (l = 0, i = 0, p = line, *p = '\0'; l < arg2; l++) {
    loc = arg1 + l;
#ifdef USE_DIRECT_MEM
    sprintf(p, "%02X ", peekb((unsigned int)(loc >> 4), (unsigned int)(loc & 0xF)) & 0xFF);
#else
    sprintf(p, "%02X ", (int)loc & 0xFF);
#endif
    i++;
    p += 3;
    if (i >= 16) {
      sprintf(p-1, "\r\n");
      write(net, line, strlen(line));
      i = 0;
      p = line;
    }
  }
  if (i > 0) {
    sprintf(p-1, "\r\n");
    write(net, line, strlen(line));
  }
}

void enter_mem(int net, int argc, unsigned long arg1, unsigned long arg2)
{
  if (argc < 3)
    write(net, "?\r\n", 3);
  else {
#ifdef USE_DIRECT_MEM
    pokeb((unsigned int)(arg1 >> 4), (unsigned int)(arg1 & 0xF), (char)arg2);
#endif
    write(net, "0\r\n", 3);
  }
}

void poll_port(int net, int argc, long arg1, long arg2, long arg3)
{
  char reply[5];

  if (argc < 4)
    write(net, "?\r\n", 3);
  else {
#ifdef USE_PORT_IO
      int i, v;
                do {
      i = v = inportb((int)arg1);
      i ^= (int)arg2;
      i &= (int)arg3;
    } while (i == 0);
#endif
    sprintf(reply, "%02X\r\n", v & 0xFF);
    write(net, reply, sizeof(reply)-1);
  }
}

void command_loop(int net)
{
  int argc;
  char buf[128];
  char *argv[4], *end;
  unsigned long arg1, arg2, arg3;

  DEBUG("Entering command loop\n");
  while (read_line(net, buf, sizeof(buf)) > 0) {
    argc = parse_line(buf, argv, sizeof(argv)/sizeof(argv[0]));
    if (argc <= 0)
       continue;

    arg1 = arg2 = arg3 = 0;
    if (argc >= 2) {
      arg1 = strtoul(argv[1], &end, 0);
      if (*end != '\0' && strchr(whitespace, *end) == NULL)
        argc = 1; /* bad arg1 */
    }
    if (argc >= 3) {
      arg2 = strtoul(argv[2], &end, 0);
      if (*end != '\0' && strchr(whitespace, *end) == NULL)
        argc = 2; /* bad arg2 */
    }
    if (argc >= 4) {
      arg3 = strtoul(argv[2], &end, 0);
      if (*end != '\0' && strchr(whitespace, *end) == NULL)
        argc = 3; /* bad arg3 */
    }

    switch (tolower(*argv[0])) {
    case 'i':
      in_port(net, argc, arg1);
      break;
    case 'o':
      out_port(net, argc, arg1, arg2);
      break;
    case 'd':
      dump_mem(net, argc, arg1, arg2);
      break;
    case 'e':
      enter_mem(net, argc, arg1, arg2);
      break;
    case 'p':
      poll_port(net, argc, arg1, arg2, arg3);
      break;
    default:
      write(net, "?\r\n", 3);
      break;
    }
  }
  close(net);
  DEBUG("Exiting command loop\n");
}

int main(int argc, char *argv[])
{
  int sock, net, c;
  struct sockaddr_in addrin;
  int addrinlen = sizeof(addrin);

  while ((c = getopt(argc, argv, "p:v")) != EOF) {
    switch (c)
    {
    case 'p':
      port = atoi(optarg);
      if (port == 0)
        port = RDEBUG_PORT;
      break;
    case 'v':
      verbose = 1;
      break;
    default:
      fprintf(stderr, "Usage: rdebug [-p port] [-v]\n");
      break;
    } 
  }
#ifdef WATT32
  if (verbose)
    dbug_init();
#endif

  sock = setup_server(port);
  while ((net = accept(sock, (struct sockaddr *)&addrin, &addrinlen)) >= 0) {
    fprintf(stderr, "Connection from %s:%u\n",
            inet_ntoa(addrin.sin_addr), ntohs(addrin.sin_port));
    command_loop(net);
  }
  close(sock);
  return (0);
}

