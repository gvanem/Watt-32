#include <string.h>
#include <stdio.h>

struct cfgopt {
       const char *name;
       const char *macro;
       const char *help;
     };

enum optval {
     OPT_UNDEFINED = 0,
     OPT_ENABLE,
     OPT_DISABLE
   };

static const struct cfgopt options[] =
{
  { "debug",            "DEBUG",            "Include debug code." },
  { "multicast",        "MULTICAST",        "Include IP multicast code by Jim Martin." },
  { "bind",             "BIND",             "Include Bind resolver code." },
  { "bsd-api",          "BSD_API",          "Include BSD-sockets." },
  { "bsd-fatal",        "BSD_FATAL",        "Exit application on BSD-socket fatal errors." },
  { "bootp",            "BOOTP",            "Include BOOTP client code." },
  { "dhcp",             "DHCP",             "Include DHCP boot client code." },
  { "rarp",             "RARP",             "Include RARP boot client code.  Contributed by Dan Kegel." },
  { "geoip",            "GEOIP",            "Include GeoIP support.  From Tor's geoip.c." },
  { "ipv6",             "IPV6",             "Include IPv6 dual-stack support." },
  { "language",         "LANGUAGE",         "Include Language translation code." },
  { "fragments",        "FRAGMENTS",        "Include IP-fragment handling." },
  { "statistics",       "STATISTICS",       "Include protocol statistics count." },
  { "stackwalker",      "STACKWALKER",      "Use StackWalker for allocations and crash-dumps." },
  { "fsext",            "FSEXT",            "Use djgpp's File Extensions for file I/O functions." },
  { "loopback",         "LOOPBACK",         "Use the simple loopback device." },
  { "embedded",         "EMBEDDED",         "Make an embeddable (ROM-able) target.  See note in config.h." },
  { "buffered-io",      "BUFFERED_IO",      "Use buffered file I/O in pcconfig.c.  Otherwise use a simple read-ahead cache." },
  { "tftp",             "TFTP",             "Include TFTP protocol for simple file retrival." },
  { "udp-only",         "UDP_ONLY",         "Exclude all stuff related to the TCP protocol." },
  { "tcp-sack",         "TCP_SACK",         "Include TCP Selective ACK feature (not yet)." },
  { "echo-disc",        "ECHO_DISC",        "Include echo/discard servers." },
  { "pppoe",            "PPPOE",            "PPP-over-Ethernet encapsulation." },
  { "fast-pkt",         "FAST_PKT",         "Use faster all real-mode PKTDRVR receiver (DOSX only)." },
  { "dead-gwd",         "DEAD_GWD",         "Dead Gateway detection in ARP module." },
  { "gzip",             "GZIP",             "Use gzip compression for PPP and/or pcap debug file.  Also for SCTP." },
  { "profiler",         "PROFILER",         "Include simple execution profiler (only DOSX with 64-bit types)." },
  { "tcp-md5",          "TCP_MD5",          "TCP MD5 signature option." },
  { "idna",             "IDNA",             "Support national characters in DNS name lookups." },
  { "dynip-cli",        "DYNIP_CLI",        "Support DynDNS.org name/IP client." },
  { "sctp",             "SCTP",             "Stream Control Transfer Protocol; experimental." }
};

#define NUM_OPTIONS (sizeof (options) / sizeof (struct cfgopt))

static enum optval option_values[NUM_OPTIONS] = { };

static int skip (const char* prefix, const char** arg)
{
  const size_t len = strlen (prefix);
  if (strncmp (prefix, *arg, len) == 0)
  {
    *arg += len;
    return (1);
  }
  return (0);
}

static void usage (void)
{
  const char * const prefix = "  --[en|dis]able-";
  const int wrap_at         = 80;
  const int name_len        = 13;
  const int option_len      = strlen (prefix) + name_len;
  const int help_len        = wrap_at - option_len;
  const char *p, *end;
  int i;

  printf ("Available options:\n");
  for (i = 0; i < NUM_OPTIONS; ++i)
  {
    p = options[i].help;
    while (*p)
    {
      end = p + strlen (p);
      if (end - p > help_len)
      {
        /* Find last space character before wrap point. */
        const char *q = p;
        while (1)
        {
          q = strchr (q, ' ');
          if (q && q - p < help_len)
             end = q;
          else break;
          ++q;
        }
      }

      /* Print option name. */
      if (p == options[i].help)
         printf ("%s%-*s", prefix, name_len, options[i].name);
      else
         printf ("%*c", option_len, ' ');

      /* Print description. */
      printf ("%.*s\n", (int)(end - p), p);

      p = end;

      /* Skip spaces. */
      while (*p == ' ') ++p;
    }
  }
}

int main (int argc, char** argv)
{
  int i, j;
  int check_only = 0;
  enum optval val;
  const char *p;

  for (i = 1; i < argc; ++i)
  {
    p = argv[i];

    if (!skip ("--", &p))
       goto invalid;

    if (strcmp ("help", p) == 0)
    {
      usage ();
      return (1);
    }

    if (strcmp ("check", p) == 0)
    {
      /* Only check if arguments are recognized. */
      check_only = 1;
      continue;
    }

    if (skip ("enable-", &p))
       val = OPT_ENABLE;
    else if (skip ("disable-", &p))
       val = OPT_DISABLE;
    else goto invalid;

    for (j = 0; j < NUM_OPTIONS; ++j)
    {
      if (strcmp (options[j].name, p) == 0)
      {
        option_values[j] = val;
        break;
      }
    }
    if (j == NUM_OPTIONS)
       goto invalid;
  }

  if (check_only)
     return (0);

  for (j = 0; j < NUM_OPTIONS; ++j)
  {
    const enum optval val = option_values[j];
    const char * const macro = options[j].macro;

    if (val != OPT_UNDEFINED)
    {
      printf ("#undef USE_%s\n", macro);
      if (val == OPT_ENABLE)
         printf ("#define USE_%s\n", macro);
    }
  }
  return (0);

invalid:
  if (!check_only)
     fprintf (stderr, "Unrecognized option: %s\n", argv[i]);
  return (2);
}
