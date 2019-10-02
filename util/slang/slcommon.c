/* This file contains library-wide symbols that are always needed when one
 * links to the library.
 */
/*
Copyright (C) 2004-2011 John E. Davis

This file is part of the S-Lang Library.

The S-Lang Library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The S-Lang Library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
USA.
*/
#include "slinclud.h"

#include <errno.h>
#include "slang.h"
#include "_slang.h"

#if defined(__WIN32__)
# include <windows.h>
#endif

# define SLREALLOC_FUN	SLREALLOC
# define SLMALLOC_FUN	SLMALLOC
# define SLFREE_FUN	SLFREE

int SLang_Version = SLANG_VERSION;
SLFUTURE_CONST char *SLang_Version_String = SLANG_VERSION_STRING;

/* Mallocing 0 bytes leads to undefined behavior.  Some C libraries will
 * return NULL, and others return non-NULL.  Here, if 0 bytes is requested,
 * and the library malloc function returns NULL, then malloc will be retried
 * using 1 byte.
 */
char *SLmalloc (unsigned int len)
{
   char *p;

   if (NULL != (p = (char *) SLMALLOC_FUN (len)))
     return p;

   if (!len) p = (char *)SLMALLOC_FUN(1);

   return p;
}

void SLfree (char *p)
{
   if (p != NULL) SLFREE_FUN (p);
}

char *_SLcalloc (unsigned int nelems, unsigned int len)
{
   unsigned int nlen = nelems * len;

   if (nelems && (nlen/nelems != len))
     {
	return NULL;
     }
   return SLmalloc (nlen);
}

char *SLcalloc (unsigned int nelems, unsigned int len)
{
   char *p = _SLcalloc (nelems, len);
   if (p != NULL) memset (p, 0, len*nelems);
   return p;
}
