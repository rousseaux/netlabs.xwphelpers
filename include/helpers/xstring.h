
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
     *@@ XSTRING:
     *
     *@@added V0.9.6 (2000-11-01) [umoeller]
     */

    typedef struct _XSTRING
    {
        PSZ             psz;            // ptr to string or NULL
        ULONG           ulLength;       // length of *psz
        ULONG           cbAllocated;    // memory allocated in *psz
                                        // (>= ulLength + 1)
    } XSTRING, *PXSTRING;

    void xstrInit(PXSTRING pxstr,
                  ULONG ulPreAllocate);

    void xstrInitSet(PXSTRING pxstr,
                     PSZ pszNew);

    void xstrInitCopy(PXSTRING pxstr,
                      const char *pcszSource);

    void xstrClear(PXSTRING pxstr);

    PXSTRING xstrCreate(ULONG ulPreAllocate);

    VOID xstrFree(PXSTRING pxstr);

    ULONG xstrset(PXSTRING pxstr,
                  PSZ pszNew);

    ULONG xstrcpy(PXSTRING pxstr,
                  const char *pcszSource);

    ULONG xstrcat(PXSTRING pxstr,
                  const char *pcszSource);

    /*
     *@@ xstrIsString:
     *      returns TRUE if psz contains something.
     *
     *@@added V0.9.6 (2000-10-16) [umoeller]
     */

    #define xstrIsString(psz) ( (psz != 0) && (*(psz) != 0) )

    ULONG xstrrpl(PXSTRING pxstr,
                  ULONG ulOfs,
                  const XSTRING *pstrSearch,
                  const XSTRING *pstrReplace,
                  PULONG pulAfterOfs);

    ULONG xstrcrpl(PXSTRING pxstr,
                   ULONG ulOfs,
                   const char *pcszSearch,
                   const char *pcszReplace,
                   PULONG pulAfterOfs);
#endif

#if __cplusplus
}
#endif

