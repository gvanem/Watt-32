/*!\file arpa/nameser.h
 * Nameserver API.
 */

/*
 * Copyright (c) 1983, 1989, 1993
 *        The Regents of the University of California.  All rights reserved.
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
 *        This product includes software developed by the University of
 *        California, Berkeley and its contributors.
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
 *
 * -
 * Portions Copyright (c) 1993 by Digital Equipment Corporation.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies, and that
 * the name of Digital Equipment Corporation not be used in advertising or
 * publicity pertaining to distribution of the document or software without
 * specific, written prior permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 * -
 * --Copyright--
 */

#ifndef __ARPA_NAMESER_H
#define __ARPA_NAMESER_H

#ifndef __SYS_W32API_H
#include <sys/w32api.h>
#endif

#ifndef __SYS_PARAM_H
#include <sys/param.h>
#endif

#ifndef __SYS_WTYPES_H
#include <sys/wtypes.h>
#endif

#ifndef __SYS_CDEFS_H
#include <sys/cdefs.h>
#endif

/*
 * revision information.  this is the release date in YYYYMMDD format.
 * it can change every day so the right thing to do with it is use it
 * in preprocessor commands such as "#if (__BIND > 19931104)".  do not
 * compare for equality; rather, use it to determine whether your resolver
 * is new enough to contain a certain feature.
 */

#define __BIND    19960801  /* interface version stamp */

/*
 * Define constants based on rfc883
 */
#define PACKETSZ     512               /* maximum packet size */
#define MAXDNAME    1025               /* maximum domain name */
#define MAXCDNAME    255               /* maximum compressed domain name */
#define MAXLABEL      63               /* maximum length of domain label */
#define HFIXEDSZ      12               /* #/bytes of fixed data in header */
#define QFIXEDSZ       4               /* #/bytes of fixed data in query */
#define RRFIXEDSZ     10               /* #/bytes of fixed data in r record */
#define INT32SZ        4               /* for systems without 32-bit ints */
#define INT16SZ        2               /* for systems without 16-bit ints */
#define INADDRSZ       4               /* IPv4 T_A */
#define IN6ADDRSZ     16               /* IPv6 T_AAAA */


/*
 * Internet nameserver port number
 */
#define NAMESERVER_PORT  53

/*
 * Currently defined opcodes
 */
#define QUERY          0x0             /* standard query */
#define IQUERY         0x1             /* inverse query */
#define STATUS         0x2             /* nameserver status query */
#define NS_NOTIFY_OP   0x4             /* notify secondary of SOA change */

#ifdef ALLOW_UPDATES
        /* non standard - supports ALLOW_UPDATES stuff from Mike Schwartz */
# define UPDATEA       0x9             /* add resource record */
# define UPDATED       0xa             /* delete a specific resource record */
# define UPDATEDA      0xb             /* delete all named resource record */
# define UPDATEM       0xc             /* modify a specific resource record */
# define UPDATEMA      0xd             /* modify all named resource record */
# define ZONEINIT      0xe             /* initial zone transfer */
# define ZONEREF       0xf             /* incremental zone referesh */
#endif

/*
 * Currently defined response codes.
 * The <winerror.h> value is okay to use.
 */
#if !defined(WIN32) && !defined(_WIN32) && !defined(__WIN32__)
#define NOERROR        0               /* no error */
#endif

/* A fix for the 'c-ares' resolver library that has a:
 *   #ifndef NOERROR
 *   #define NOERROR ns_r_noerror
 *   #endif
 */
#define ns_r_noerror   0

#define FORMERR        1               /* format error */
#define SERVFAIL       2               /* server failure */
#define NXDOMAIN       3               /* non existent domain */
#define NOTIMP         4               /* not implemented */
#define REFUSED        5               /* query refused */

#ifdef ALLOW_UPDATES
        /* non standard */
# define NOCHANGE      0xf             /* update failed to change db */
#endif

/*
 * Type values for resources and queries
 */
#define T_A            1               /* host address */
#define T_NS           2               /* authoritative server */
#define T_MD           3               /* mail destination */
#define T_MF           4               /* mail forwarder */
#define T_CNAME        5               /* canonical name */
#define T_SOA          6               /* start of authority zone */
#define T_MB           7               /* mailbox domain name */
#define T_MG           8               /* mail group member */
#define T_MR           9               /* mail rename name */
#define T_NULL         10              /* null resource record */
#define T_WKS          11              /* well known service */
#define T_PTR          12              /* domain name pointer */
#define T_HINFO        13              /* host information */
#define T_MINFO        14              /* mailbox information */
#define T_MX           15              /* mail routing information */
#define T_TXT          16              /* text strings */
#define T_RP           17              /* responsible person */
#define T_AFSDB        18              /* AFS cell database */
#define T_X25          19              /* X_25 calling address */
#define T_ISDN         20              /* ISDN calling address */
#define T_RT           21              /* router */
#define T_NSAP         22              /* NSAP address */
#define T_NSAP_PTR     23              /* reverse NSAP lookup (deprecated) */
#define T_SIG          24              /* security signature */
#define T_KEY          25              /* security key */
#define T_PX           26              /* X.400 mail mapping */
#define T_GPOS         27              /* geographical position (withdrawn) */
#define T_AAAA         28              /* IP6 Address */
#define T_LOC          29              /* Location Information */
#define T_NXT          30              /* Next Valid Name in Zone */
#define T_EID          31              /* Endpoint identifier */
#define T_NIMLOC       32              /* Nimrod locator */
#define T_SRV          33              /* Server selection */
#define T_ATMA         34              /* ATM Address */
#define T_NAPTR        35              /* Naming Authority PoinTeR */
#define T_OPT          41              /* Extension mechanisms for DNS (EDNS) */
        /* non standard */
#define T_UINFO       100              /* user (finger) information */
#define T_UID         101              /* user ID */
#define T_GID         102              /* group ID */
#define T_UNSPEC      103              /* Unspecified format (binary data) */
        /* Query type values which do not appear in resource records */
#define T_IXFR        251              /* incremental zone transfer */
#define T_AXFR        252              /* transfer zone of authority */
#define T_MAILB       253              /* transfer mailbox records */
#define T_MAILA       254              /* transfer mail agent records */
#define T_ANY         255              /* wildcard match */
#define T_CAA         257              /* Certification Authority Authorization, RFC-6844 */
#define T_WINS        0xFF01           /* WINS name lookup */
#define T_WINSR       0xFF02           /* WINS reverse lookup */

/*
 * Values for class field
 */

#define C_IN          1                /* the arpa internet */
#define C_CHAOS       3                /* for chaos net (MIT) */
#define C_HS          4                /* for Hesiod name server (MIT) (XXX) */
        /* Query class values which do not appear in resource records */
#define C_ANY         255              /* wildcard match */

/*
 * Status return codes for T_UNSPEC conversion routines
 */
#define CONV_SUCCESS    0
#define CONV_OVERFLOW   (-1)
#define CONV_BADFMT     (-2)
#define CONV_BADCKSUM   (-3)
#define CONV_BADBUFLEN  (-4)

W32_CLANG_PACK_WARN_OFF()

#include <sys/pack_on.h>

/*
 * Structure for query header.  The order of the fields is machine- and
 * compiler-dependent, depending on the byte/bit order and the layout
 * of bit fields.  We use bit fields only in int variables, as this
 * is all ANSI requires.  This requires a somewhat confusing rearrangement.
 */

typedef struct {
        unsigned  id : 16;          /* query identification number */

        /* fields in third byte */
        unsigned  rd : 1;           /* recursion desired */
        unsigned  tc : 1;           /* truncated message */
        unsigned  aa : 1;           /* authoritative answer */
        unsigned  opcode : 4;       /* purpose of message */
        unsigned  qr : 1;           /* response flag */

        /* fields in fourth byte */
        unsigned  rcode : 4;        /* response code */
        unsigned  cd : 1;           /* checking disabled by resolver */
        unsigned  ad : 1;           /* authentic data from named */
        unsigned  unused : 1;       /* unused bits (MBZ as of 4.9.3a3) */
        unsigned  ra : 1;           /* recursion available */

        /* remaining bytes */
        unsigned  qdcount : 16;     /* number of question entries */
        unsigned  ancount : 16;     /* number of answer entries */
        unsigned  nscount : 16;     /* number of authority entries */
        unsigned  arcount : 16;     /* number of resource entries */
      } HEADER;


/*
 * Defines for handling compressed domain names
 */
#define INDIR_MASK  0xc0

/*
 * Structure for passing resource records around.
 */
struct rrec {
       u_short  r_zone;            /* zone number */
       u_short  r_class;           /* class number */
       u_short  r_type;            /* type number */
       u_long   r_ttl;             /* time to live */
       int      r_size;            /* size of data area */
       char    *r_data;            /* pointer to data */
     };

#include <sys/pack_off.h>

W32_CLANG_PACK_WARN_DEF()

__BEGIN_DECLS

W32_FUNC u_short W32_CALL _getshort (const u_char *);
W32_FUNC u_long  W32_CALL _getlong  (const u_char *);

__END_DECLS

/*
 * Inline versions of get/put short/long.  Pointer is advanced.
 *
 * These macros demonstrate the property of C whereby it can be
 * portable or it can be elegant but rarely both.
 */
#define GETSHORT(s, cp) do { \
        register u_char *t_cp = (u_char *)(cp); \
        (s) = ((u_short)t_cp[0] << 8) \
            | ((u_short)t_cp[1]); \
        (cp) += INT16SZ; \
      } while (0)

#define GETLONG(l, cp) do { \
        register u_char *t_cp = (u_char *)(cp); \
        (l) = ((u_long)t_cp[0] << 24) \
            | ((u_long)t_cp[1] << 16) \
            | ((u_long)t_cp[2] << 8) \
            | ((u_long)t_cp[3]); \
        (cp) += INT32SZ; \
      } while (0)

#define PUTSHORT(s, cp) do { \
        register u_short t_s = (u_short)(s); \
        register u_char *t_cp = (u_char *)(cp); \
        *t_cp++ = (u_char) (t_s >> 8); \
        *t_cp   = (u_char) t_s; \
        (cp) += INT16SZ; \
      } while (0)

#define PUTLONG(l, cp) do { \
        register u_long t_l = (u_long)(l); \
        register u_char *t_cp = (u_char *)(cp); \
        *t_cp++ = (u_char) (t_l >> 24); \
        *t_cp++ = (u_char) (t_l >> 16); \
        *t_cp++ = (u_char) (t_l >> 8); \
        *t_cp   = (u_char) t_l; \
        (cp) += INT32SZ; \
      } while (0)

#endif
