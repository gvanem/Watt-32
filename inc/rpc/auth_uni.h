/*!\file rpc/auth_uni.h
 * RPC authentication.
 */

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

/*
 * auth_unix.h, Protocol for UNIX style authentication parameters for RPC
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

/*
 * The system is very weak.  The client uses no encryption for  it
 * credentials and only sends null verifiers.  The server sends backs
 * null verifiers or optionally a verifier that suggests a new short hand
 * for the credentials.
 */

#ifndef __RPC_AUTH_UNIX_H
#define __RPC_AUTH_UNIX_H

#include <rpc/auth.h>

/* The machine name is part of a credential; it may not exceed 255 bytes */
#define MAX_MACHINE_NAME 255

/* gids compose part of a credential; there may not be more than 16 of them */
#define NGRPS 16

/*
 * Unix style credentials.
 */
struct authunix_parms {
    u_long   aup_time;
    char    *aup_machname;
    int  aup_uid;
    int  aup_gid;
    u_int    aup_len;
    int *aup_gids;
};

__BEGIN_DECLS

extern bool_t xdr_authunix_parms (XDR *, struct authunix_parms *);

__END_DECLS

/*
 * If a response verifier has flavor AUTH_SHORT,
 * then the body of the response verifier encapsulates the following structure;
 * again it is serialized in the obvious fashion.
 */
struct short_hand_verf {
       struct opaque_auth new_cred;
};

#endif
