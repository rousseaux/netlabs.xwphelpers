
/*
 *@@sourcefile xstring.h:
 *      header file for xstring.c. See notes there.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@include #define INCL_DOSDATETIME
 *@@include #include <os2.h>
 *@@include #include "xstring.h"
 */

/*
 *      Copyright (C) 1999-2000 Ulrich M”ller.
 *      This file is part of the XWorkplace source package.
 *      XWorkplace is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published
 *      by the Free Software Foundation, in version 2 as it comes in the
 *      "COPYING" file of the XWorkplace main distribution.
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 */

#if __cplusplus
extern "C" {
#endif

#ifndef XSTRING_HEADER_INCLUDED
    #define XSTRING_HEADER_INCLUDED

    /*
     *@@ XSTR:
     *      string type for the strhx* functions.
     *      That's a simple pointer to a PSZ.
     */

    typedef PSZ* XSTR;

    /*
     *@@ xstrIsString:
     *      returns TRUE if psz contains something.
     *
     *@@added V0.9.6 (2000-10-16) [umoeller]
     */

    #define xstrIsString(psz) ( (psz != 0) && (*(psz) != 0) )

    ULONG xstrcpy(XSTR ppszBuf,
                  const char *pszString);

    #ifdef __XWPMEMDEBUG__ // setup.h, helpers\memdebug.c
        ULONG xstrcatDebug(XSTR ppszBuf,
                           const char *pszString,
                           const char *file,
                           unsigned long line,
                           const char *function);
        #define xstrcat(a, b) xstrcatDebug((a), (b), __FILE__, __LINE__, __FUNCTION__)
    #else
        ULONG xstrcat(XSTR ppszBuf,
                       const char *pszString);
    #endif

    #ifdef __XWPMEMDEBUG__ // setup.h, helpers\memdebug.c
        ULONG xstrrplDebug(PSZ *ppszBuf,
                           ULONG ulOfs,
                           const char *pszSearch,
                           const char *pszReplace,
                           PULONG pulAfterOfs,
                           const char *file,
                           unsigned long line,
                           const char *function);
        #define xstrrpl(a, b, c, d, e) xstrrplDebug((a), (b), (c), (d), (e), __FILE__, __LINE__, __FUNCTION__)
    #else
        ULONG xstrrpl(PSZ *ppszBuf,
                      ULONG ulOfs,
                      const char *pszSearch,
                      const char *pszReplace,
                      PULONG pulAfterOfs);
    #endif

    PSZ xstrins(PSZ pszBuffer,
                 ULONG ulInsertOfs,
                 const char *pszInsert);

#endif

#if __cplusplus
}
#endif

