/*!\file gettod.h
 */
#ifndef _w32_GETTIMEOFDAY_H
#define _w32_GETTIMEOFDAY_H

extern void set_utc_offset (void);

#if (DOSX) && defined(HAVE_UINT64)
extern BOOL get_tv_from_tsc (const struct ulong_long *tsc,
                             struct timeval *tv);
#endif

#if defined(_WIN32) || defined(WIN32)
  extern uint64      FILETIME_to_unix_epoch (const FILETIME *ft);
  extern double      FILETIME_sec (const FILETIME *filetime);
  extern const char *ULONGLONG_to_ctime (ULONGLONG ts);
  extern const char *SYSTEMTIME_to_str (const SYSTEMTIME *st);
#endif

#endif  /* _w32_GETTIMEOFDAY_H */
