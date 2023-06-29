/*!\file net/if.h
 * Network interface data.
 */

/*
 * Copyright (c) 1982, 1986, 1989, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by the University of
 *  California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef __NET_IF_H
#define __NET_IF_H

#include <sys/socket.h>

#ifndef __SYS_QUEUE_H
#include <sys/queue.h>
#endif

#ifndef __SYS_MBUF_H
#include <sys/mbuf.h>
#endif

/*
 * Structures defining a network interface, providing a packet
 * transport mechanism (ala level 0 of the PUP protocols).
 *
 * Each interface accepts output datagrams of a specified maximum
 * length, and provides higher level routines with input datagrams
 * received from its medium.
 *
 * Output occurs when the routine if_output is called, with four parameters:
 *  (*ifp->if_output)(ifp, m, dst, rt)
 * Here m is the mbuf chain to be sent and dst is the destination address.
 * The output routine encapsulates the supplied datagram if necessary,
 * and then transmits it on its medium.
 *
 * On input, each interface unwraps the data received by it, and either
 * places it on the input queue of a internetwork datagram routine
 * and posts the associated software interrupt, or passes the datagram to a raw
 * packet input routine.
 *
 * Routines exist for locating interfaces by their addresses
 * or for locating a interface on a certain network, as well as more general
 * routing and gateway routines maintaining information used to locate
 * interfaces.  These routines live in the files if.c and route.c
 */
#ifndef __SYS_WTIME_H
#include <sys/wtime.h>
#endif

struct rtentry;
struct ether_header;
struct proc   { int dummy; };
struct socket { int dummy; };

/*
 * Structure defining statistics and other data kept regarding a network
 * interface.
 */
struct  if_data {
    /* generic interface information */
    u_char  ifi_type;           /* ethernet, tokenring, etc. */
    u_char  ifi_addrlen;        /* media address length */
    u_char  ifi_hdrlen;         /* media header length */
    u_long  ifi_mtu;            /* maximum transmission unit */
    u_long  ifi_metric;         /* routing metric (external only) */
    u_long  ifi_baudrate;       /* linespeed */
    /* volatile statistics */
    u_long  ifi_ipackets;       /* packets received on interface */
    u_long  ifi_ierrors;        /* input errors on interface */
    u_long  ifi_opackets;       /* packets sent on interface */
    u_long  ifi_oerrors;        /* output errors on interface */
    u_long  ifi_collisions;     /* collisions on csma interfaces */
    u_long  ifi_ibytes;         /* total number of octets received */
    u_long  ifi_obytes;         /* total number of octets sent */
    u_long  ifi_imcasts;        /* packets received via multicast */
    u_long  ifi_omcasts;        /* packets sent via multicast */
    u_long  ifi_iqdrops;        /* dropped on input, this interface */
    u_long  ifi_noproto;        /* destined for unsupported protocol */
    struct  timeval ifi_lastchange; /* last updated */
};

/*
 * Structure defining a queue for a network interface.
 *
 * (Would like to call this struct ``if'', but C isn't PL/1.)
 */
TAILQ_HEAD(ifnet_head, ifnet);          /* the actual queue head */

/*
 * Length of interface external name, including terminating '\0'.
 * Note: this is the same size as a generic device's external name.
 */
#define IFNAMSIZ    16

#ifndef IF_NAMESIZE
#define IF_NAMESIZE     IFNAMSIZ
#endif

struct ifqueue {
       struct  mbuf *ifq_head;
       struct  mbuf *ifq_tail;
       int     ifq_len;
       int     ifq_maxlen;
       int     ifq_drops;
     };

struct ifnet {                                 /* and the entries */
    void                  *if_softc;           /* lower-level data for this if */
    TAILQ_ENTRY(ifnet)     if_list;            /* all struct ifnets are chained */
    TAILQ_HEAD(xx, ifaddr) if_addrlist;        /* linked list of addresses per if */
    char                   if_xname[IFNAMSIZ]; /* external name (name + unit) */
    int                    if_pcount;          /* number of promiscuous listeners */
    caddr_t                if_bpf;             /* packet filter structure */
    u_short                if_index;           /* numeric abbreviation for this if */
    short                  if_timer;           /* time 'til if_watchdog called */
    short                  if_flags;           /* up/down, broadcast, etc. */
    short                  if__pad1;           /* be nice to m68k ports */
    struct  if_data        if_data;            /* statistics and other data about if */

    /* procedure handles */
    int (*if_output)        /* output routine (enqueue) */
                (struct ifnet *, struct mbuf *, struct sockaddr *,
                 struct rtentry *);
    void (*if_start)     /* initiate output routine */
                (struct ifnet *);
    int (*if_ioctl)     /* ioctl routine */
                (struct ifnet *, u_long, caddr_t);
    int (*if_reset)     /* XXX bus reset routine */
                (struct ifnet *);
    void    (*if_watchdog)      /* timer routine */
                (struct ifnet *);
    struct  ifqueue if_snd;          /* output queue */
    struct  sockaddr_dl *if_sadl;   /* pointer to our sockaddr_dl */
    u_int8_t *if_broadcastaddr;     /* linklevel broadcast bytestring */
};

#define if_mtu        if_data.ifi_mtu
#define if_type       if_data.ifi_type
#define if_addrlen    if_data.ifi_addrlen
#define if_hdrlen     if_data.ifi_hdrlen
#define if_metric     if_data.ifi_metric
#define if_baudrate   if_data.ifi_baudrate
#define if_ipackets   if_data.ifi_ipackets
#define if_ierrors    if_data.ifi_ierrors
#define if_opackets   if_data.ifi_opackets
#define if_oerrors    if_data.ifi_oerrors
#define if_collisions if_data.ifi_collisions
#define if_ibytes     if_data.ifi_ibytes
#define if_obytes     if_data.ifi_obytes
#define if_imcasts    if_data.ifi_imcasts
#define if_omcasts    if_data.ifi_omcasts
#define if_iqdrops    if_data.ifi_iqdrops
#define if_noproto    if_data.ifi_noproto
#define if_lastchange if_data.ifi_lastchange

#define IFF_UP          0x1       /* interface is up */
#define IFF_BROADCAST   0x2       /* broadcast address valid */
#define IFF_DEBUG       0x4       /* turn on debugging */
#define IFF_LOOPBACK    0x8       /* is a loopback net */
#define IFF_POINTOPOINT 0x10      /* interface is point-to-point link */
#define IFF_NOTRAILERS  0x20      /* avoid use of trailers */
#define IFF_RUNNING     0x40      /* resources allocated */
#define IFF_NOARP       0x80      /* no address resolution protocol */
#define IFF_PROMISC     0x100     /* receive all packets */
#define IFF_ALLMULTI    0x200     /* receive all multicast packets */
#define IFF_OACTIVE     0x400     /* transmission in progress */
#define IFF_SIMPLEX     0x800     /* can't hear own transmissions */
#define IFF_LINK0       0x1000    /* per link layer defined bit */
#define IFF_LINK1       0x2000    /* per link layer defined bit */
#define IFF_LINK2       0x4000    /* per link layer defined bit */
#define IFF_MULTICAST   0x8000    /* supports multicast */

/* flags set internally only: */
#define IFF_CANTCHANGE \
    (IFF_BROADCAST|IFF_POINTOPOINT|IFF_RUNNING|IFF_OACTIVE|\
        IFF_SIMPLEX|IFF_MULTICAST|IFF_ALLMULTI)

/*
 * Output queues (ifp->if_snd) and internetwork datagram level (pup level 1)
 * input routines have queues of messages stored on ifqueue structures
 * (defined above).  Entries are added to and deleted from these structures
 * by these macros, which should be called with ipl raised to splimp().
 */
#define IF_QFULL(ifq)       ((ifq)->ifq_len >= (ifq)->ifq_maxlen)
#define IF_DROP(ifq)        ((ifq)->ifq_drops++)
#define IF_ENQUEUE(ifq, m) { \
    (m)->m_nextpkt = 0; \
    if ((ifq)->ifq_tail == 0) \
        (ifq)->ifq_head = m; \
    else \
        (ifq)->ifq_tail->m_nextpkt = m; \
    (ifq)->ifq_tail = m; \
    (ifq)->ifq_len++; \
}
#define IF_PREPEND(ifq, m) { \
    (m)->m_nextpkt = (ifq)->ifq_head; \
    if ((ifq)->ifq_tail == 0) \
        (ifq)->ifq_tail = (m); \
    (ifq)->ifq_head = (m); \
    (ifq)->ifq_len++; \
}
#define IF_DEQUEUE(ifq, m) { \
    (m) = (ifq)->ifq_head; \
    if (m) { \
        if (((ifq)->ifq_head = (m)->m_nextpkt) == 0) \
            (ifq)->ifq_tail = 0; \
        (m)->m_nextpkt = 0; \
        (ifq)->ifq_len--; \
    } \
}

#define IFQ_MAXLEN  50
#define IFNET_SLOWHZ    1       /* granularity is 1 second */

/*
 * The ifaddr structure contains information about one address
 * of an interface.  They are maintained by the different address families,
 * are allocated and attached when an address is set, and are linked
 * together so all addresses for an interface can be located.
 */
struct ifaddr {
    struct  sockaddr   *ifa_addr;        /* address of interface */
    struct  sockaddr   *ifa_dstaddr;     /* other end of p-to-p link */
#define ifa_broadaddr   ifa_dstaddr      /* broadcast address interface */
    struct  sockaddr   *ifa_netmask;     /* used to determine subnet */
    struct  ifnet      *ifa_ifp;         /* back-pointer to interface */
    TAILQ_ENTRY(ifaddr) ifa_list;        /* list of addresses for interface */
    void              (*ifa_rtrequest)   /* check or clean routes (+ or -)'d */
                         (int, struct rtentry *, struct sockaddr *);
    u_short             ifa_flags;       /* mostly rt_flags for cloning */
    short               ifa_refcnt;      /* count of references */
    int                 ifa_metric;      /* cost of going out this interface */
};
#define IFA_ROUTE   RTF_UP      /* route installed */

/*
 * Message format for use in obtaining information about interfaces
 * from sysctl and the routing socket.
 */
struct if_msghdr {
    u_short         ifm_msglen;   /* to skip over non-understood messages */
    u_char          ifm_version;  /* future binary compatability */
    u_char          ifm_type;     /* message type */
    int             ifm_addrs;    /* like rtm_addrs */
    int             ifm_flags;    /* value of if_flags */
    u_short         ifm_index;    /* index for associated ifp */
    struct  if_data ifm_data;     /* statistics and other data about if */
};

/*
 * Message format for use in obtaining information about interface addresses
 * from sysctl and the routing socket.
 */
struct ifa_msghdr {
    u_short ifam_msglen;    /* to skip over non-understood messages */
    u_char  ifam_version;   /* future binary compatability */
    u_char  ifam_type;      /* message type */
    int     ifam_addrs;     /* like rtm_addrs */
    int     ifam_flags;     /* value of ifa_flags */
    u_short ifam_index;     /* index for associated ifp */
    int     ifam_metric;    /* value of ifa_metric */
};

/*
 * Interface request structure used for socket
 * ioctl's.  All interface ioctl's must have parameter
 * definitions which begin with ifr_name.  The
 * remainder may be interface specific.
 */
struct ifreq {
    char    ifr_name[IFNAMSIZ];     /* if name, e.g. "en0" */
    union {
        struct  sockaddr ifru_addr;
        struct  sockaddr ifru_dstaddr;
        struct  sockaddr ifru_hwaddr;
        struct  sockaddr ifru_broadaddr;
        short   ifru_flags;
        int ifru_metric;
        int ifru_mtu;
        caddr_t ifru_data;
    } ifr_ifru;
#define ifr_addr    ifr_ifru.ifru_addr  /* address */
#define ifr_dstaddr ifr_ifru.ifru_dstaddr   /* other end of p-to-p link */
#define ifr_hwaddr      ifr_ifru.ifru_hwaddr    /* hardware address */
#define ifr_broadaddr   ifr_ifru.ifru_broadaddr /* broadcast address */
#define ifr_flags   ifr_ifru.ifru_flags /* flags */
#define ifr_metric  ifr_ifru.ifru_metric    /* metric */
#define ifr_mtu     ifr_ifru.ifru_mtu   /* mtu */
#define ifr_media   ifr_ifru.ifru_metric    /* media options (overload) */
#define ifr_data    ifr_ifru.ifru_data  /* for use by interface */
};

struct ifaliasreq {
    char    ifra_name[IFNAMSIZ];        /* if name, e.g. "en0" */
    struct  sockaddr ifra_addr;
    struct  sockaddr ifra_dstaddr;
#define ifra_broadaddr  ifra_dstaddr
    struct  sockaddr ifra_mask;
};

struct ifmediareq {
        char    ifm_name[IFNAMSIZ];     /* if name, e.g. "en0" */
        int     ifm_current;            /* current media options */
        int     ifm_mask;               /* don't care mask */
        int     ifm_status;             /* media status */
        int     ifm_active;             /* active options */
        int     ifm_count;              /* # entries in ifm_ulist array */
        int     *ifm_ulist;             /* media words */
};

/*
 * Structure used in SIOCGIFCONF request.
 * Used to retrieve interface configuration
 * for machine (useful for programs which
 * must know all networks accessible).
 */
struct  ifconf {
    int ifc_len;        /* size of associated buffer */
    union {
        caddr_t ifcu_buf;
        struct  ifreq *ifcu_req;
    } ifc_ifcu;
#define ifc_buf ifc_ifcu.ifcu_buf   /* buffer address */
#define ifc_req ifc_ifcu.ifcu_req   /* array of structures returned */
};

#ifndef __NET_IF_ARP_H
#include <net/if_arp.h>
#endif

#endif
