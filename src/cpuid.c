/*
 * CPU-model tool. Uses CPUID to figure out a more
 * exact name of the running processor. Supports these 32 and 64-bit
 * CPUs:
 *   GenuineIntel, AuthenticAMD and CentaurHauls.
 *
 * Supports GNU-C 4+ and MSVC v16+ compilers.
 *
 * Some of the below code has been taken from the MPIR Library. cpuid.c.
 *
 * Copyright 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006
 * Free Software Foundation, Inc.
 *
 * Copyright 2008 William Hart.
 *
 * Copyright 2009,2010,2011 Jason Moxham
 *
 * Copyright 2010 Gonzalo Tornaria
 *
 * The rest has been taken from my Watt-32 tcp/ip stack at:
 * http://www.watt-32.net
 *
 * G. Vanem <gvanem@yahoo.no> 2012.
 *
 * References:
 *   https://en.wikipedia.org/wiki/CPUID
 *   http://www.sandpile.org/x86/cpuid.htm
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "wattcp.h"
#include "cpumodel.h"

static DWORD get_cpuid2 (int level, void *result, unsigned line);

#define GET_CPUID2(level, result) get_cpuid2 (level, result, __LINE__)

static int  trace_level = 0;
static char vendor_str [13];

#if defined(__HIGHC__) || defined(BCC32_OLD)
  /*
   * High-C and old Borland compilers does not handle 'var-args' macros.
   * Fake it into a dummy var-arg function.
   */
  static void TRACE (int level, const char *fmt, ...)
  {
    ARGSUSED (color);
    ARGSUSED (fmt);
  }

#else
  #define TRACE(level, fmt, ...)                        \
          do {                                          \
            if (trace_level >= level) {                 \
               printf ("%s(%u): ", __FILE__, __LINE__); \
               printf (fmt, ##__VA_ARGS__);             \
            }                                           \
          } while (0)
#endif

static void print_reg (const char *reg_name, DWORD reg)
{
  BYTE a = loBYTE (reg);
  BYTE b = hiBYTE (reg);
  BYTE c = loBYTE (reg >> 16);
  BYTE d = hiBYTE (reg >> 16);

  if (trace_level >= 1)
     printf ("  %s: %08lX, %c%c%c%c\n", reg_name, reg,
             isprint(a) ? a : '.',
             isprint(b) ? b : '.',
             isprint(c) ? c : '.',
             isprint(d) ? d : '.');
}

static DWORD get_cpuid2 (int level, void *result, unsigned line)
{
  DWORD  eax = 0, ebx = 0, ecx = 0, edx = 0;
  DWORD *res = (DWORD*) result;

  get_cpuid (level, &eax, &ebx, &ecx, &edx);

  TRACE (1, "From get_cpuid (0x%08X) at line %u:\n", level, line);
  print_reg ("EAX", eax);
  print_reg ("EBX", ebx);
  print_reg ("ECX", ecx);
  print_reg ("EDX", edx);

  if (res)
  {
    res [0] = ebx;
    res [1] = ecx;
    res [2] = edx;
  }
  return (eax);
}

/*
 * Returns a specific name for an "GenuineIntel" based on family, model and features.
 */
static const char *get_Intel_model (int family, int model, const void *features)
{
  static char Intel_model [12];
  char features2 [12];
  int  feat, id_max;

  switch (family)
  {
    case 5:
         if (model <= 2)
            return ("Pentium");
         if (model >= 4)
            return ("PentiumMMX");
         break;

    case 6:
         if (model == 1)
            return ("PentiumPro");
         if (model <= 6)

            return ("Pentium2");
         if (model <= 13)
            return ("Pentium3");

         if (model == 14 || model == 16)
            return ("Core");

         if (model == 15 || model == 22)
            return ("Core2");

         if (model == 17 || model == 23 || model == 29)
            return ("Penryn");

         if (model == 25 || model == 37 || model == 44 || model == 47)
            return ("Westmere");

         if (model == 26 || model == 30 || model == 31 || model == 46)
            return ("Nehalem");

         if (model == 28 || model == 38 || model == 39 || model == 54 || model == 55)
            return ("Atom");

         if (model == 42)
         {
           feat = ((int*) features) [2];
           if (feat & 0x10000000)
              return ("Sandybridge");
           return ("Westmere");
         }

         if (model == 43 || model == 45)
            return ("Sandybridge");

         if (model == 58 || model == 62)
            return ("Ivybridge");

         if (model == 60 || model == 63 || model == 69 || model == 70)
            return ("Haswell");
         break;

    case 15:
         id_max = GET_CPUID2 (0x80000000, features2) & 0xff;
         if (id_max >= 1)
         {
           GET_CPUID2 (0x80000001, features2);
           if (features2[8] & 1)
              return ("Netburstlahf");
           return ("Netburst");
         }
         if (model <= 6)
            return ("Pentium4");
         feat = ((int*)features)[2];
         if (feat & 1)
            return ("Prescott");
         break;
  }
  Intel_model [0] = '?';
  return itoa (model, Intel_model+1, 10);
  return (NULL);
}

/*
 * Returns a specific name for an "AuthenticAMD" CPU based on family, model and features.
 */
static const char *get_AMD_model (int family, int model, const void *features)
{
  static char AMD_model [12];

  switch (family)
  {
    case 5:
         if (model <= 3)
            return ("K5");
         if (model <= 7)
            return ("K6");
         if (model <= 8)
            return ("K62");
         if (model <= 9)
            return ("K63");
         break;

    case 6:
         return ("K7");

    case 15:
         return ("K8");

    case 16:
         if (model == 2)
            return ("K10");
         if (model == 4 || model == 5 || model == 6 || model == 8 || model == 9 || model == 10)
            return ("K102");
         break;

    case 17:
         return ("K8");     /* Low power k8 */

    case 18:
         return ("K103");   /* Like k102 but with hardware divider, this is lano */

    case 20:
         return ("Bobcat"); /* Fusion of Bobcat and GPU */

    case 21:
         if (model == 1)
            return ("Bulldozer");
         if (model == 2 || model == 3 || model == 16 || model == 18 || model == 19)
            return ("Piledriver");
         break;

     case 22:
          return ("Jaguar");
  }
  (void) features;
  AMD_model [0] = '?';
  return itoa (model, AMD_model+1, 10);
}

/*
 * Returns a specific name for an "CentaurHauls" CPU based on family, model and features.
 */
static const char *get_Centaur_model (int family, int model, const void *features)
{
  if (family != 6)
     return (NULL);

  if (model == 15)
     return ("Nano");

  if (model < 9)
     return ("ViaC3");
  (void) features;
  return ("ViaC32");
}

/**
 * Get basic CPI information.
 * In functio 0, the 'vendor_str' gets returned in EBX, EDX and ECX.
 *
 * \ref https://en.wikipedia.org/wiki/CPUID#EAX=0:_Highest_Function_Parameter_and_Manufacturer_ID
 */
const char *cpu_get_model (void)
{
  char   vendor_str2 [13];
  char   features [12];
  int    family, ext_family;
  int    model,  ext_model, stepping, type;
  DWORD  id_max;
  DWORD  fms;    /* family, model, stepping values */

  id_max = GET_CPUID2 (0, vendor_str2);

  sprintf (vendor_str, "%.4s%.4s%.4s",
            vendor_str2,      /* EBX */
            vendor_str2+4+4,  /* EDX */
            vendor_str2+4);   /* ECX */

  fms = GET_CPUID2 (1, features);
  TRACE (1, "  id_max: %08lX, fms: 0x%08lX.\n", id_max, fms);

  family     = (fms >> 8) & 0x0F;
  ext_family = (fms >> 27) & 0xFF;
  model      = (fms >> 4) & 0x0F;
  ext_model  = (fms >> 10) & 0x0F;
  stepping   = (fms & 0x0F);
  type       = (fms >> 12) & 0x3;

  TRACE (1, "Found vendor_str: \"%s\".\n", vendor_str);
  TRACE (1, "Found family:     %d, Extended-family: %d.\n", family, ext_family);
  TRACE (1, "Found model:      %d, Extended-model: %d.\n", model, ext_model);
  TRACE (1, "Found stepping:   %d, type: 0x%X.\n", stepping, type);

  if (!strcmp(vendor_str, "GenuineIntel"))
     return get_Intel_model (family, model, &features);

  if (!strcmp(vendor_str, "AuthenticAMD"))
     return get_AMD_model (family, model, &features);

  if (!strcmp(vendor_str, "CentaurHauls"))
     return get_Centaur_model (family, model, &features);

  if (vendor_str[0])
     return (vendor_str);

  return (NULL);
}

#if 0
const char *cpu_get_freq_info1 (void)
{
  static char  result [102];
  char   info [13];
  DWORD  id_max = GET_CPUID2 (0, info);
  DWORD  eax = 0, ebx = 0, ecx = 0;

  if (id_max < 0x16)
     return (NULL);

  eax = GET_CPUID2 (0x16, info);
  ebx = *(DWORD*) &info [1];
  ecx = *(DWORD*) &info [2];

  sprintf (result, "Core base: %lu, Core max: %lu, Core bus: %lu (MHz)",
            eax & (1 << 15), ebx & (1 << 15), ecx & (1 << 15));
  return (result);
}

const char *cpu_get_freq_info2 (void)
{
  static char  result [112];
  char   info [13];
  DWORD  id_max = GET_CPUID2 (0x80000007, info);
  DWORD  edx;

  if (id_max < 0x80000007)
     return (NULL);

  GET_CPUID2 (0x80000007, info);
  edx = *(DWORD*) &info [3];

  sprintf (result, "EDX: 0x%08lX, FID: %lu, EffFreqRO: %lu, ProcFeedback: %lu",
            edx, (edx & 1), (edx & (1 << 10)), (edx & (1 << 11)));
  return (result);
}
#endif

const char *cpu_get_brand_info (void)
{
  char   info1 [13];
  char   info2 [13];
  char   info3 [13];
  DWORD  id_max = GET_CPUID2 (0x80000000, NULL);
  DWORD  eax [3];
  static char result [49];

  if (id_max < 0x80000004)
     return (NULL);

  eax [0] = GET_CPUID2 (0x80000002, info1);
  eax [1] = GET_CPUID2 (0x80000003, info2);
  eax [2] = GET_CPUID2 (0x80000004, info3);

  sprintf (result,
            "%.4s%.12s" "%.4s%.12s" "%.4s%.12s",
            (const char*) &eax[0], info1,
            (const char*) &eax[1], info2,
            (const char*) &eax[2], info3);
  return (result);
}

/*
 * If not included from pcdbug.c
 */
#if !defined(COMPILING_PCDBUG_C)
int main (int argc, char **argv)
{
  const char *cpu, *freq, *brand;

  if (argc > 1 && !strcmp(argv[1], "-d"))
     trace_level = 1;

  cpu = cpu_get_model();
  printf ("CPU-vendor: %s\n", vendor_str[0] ? vendor_str : "<unknown>");
  printf ("CPU-model:  %s\n", cpu ? cpu : "<unknown>");

  brand = cpu_get_brand_info();
  printf ("CPU-brand:  '%s'\n", brand ? brand : "<unknown>");

#if 0
  freq = cpu_get_freq_info1();
  printf ("CPU-freq1:  %s\n", freq ? freq : "<unknown>");

  freq = cpu_get_freq_info2();
  printf ("CPU-freq2:  %s\n", freq ? freq : "<unknown>");
#endif

  return (0);
}
#endif /* COMPILING_PCDBUG_C */

