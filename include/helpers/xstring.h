
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
 *      This file is part of the "XWorkplace helpers" source package.
 *      This is free software; you can redistribute it and/or modify
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

    #ifndef XWPENTRY
        #error You must define XWPENTRY to contain the standard linkage for the XWPHelpers.
    #endif

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

    void XWPENTRY xstrInit(PXSTRING pxstr, ULONG ulPreAllocate);
    typedef void XWPENTRY XSTRINIT(PXSTRING pxstr, ULONG ulPreAllocate);
    typedef XSTRINIT *PXSTRINIT;

    void XWPENTRY xstrInitSet(PXSTRING pxstr, PSZ pszNew);
    typedef void XWPENTRY XSTRINITSET(PXSTRING pxstr, PSZ pszNew);
    typedef XSTRINITSET *PXSTRINITSET;

    void XWPENTRY xstrInitCopy(PXSTRING pxstr, const char *pcszSource, ULONG ulExtraAllocate);
    typedef void XWPENTRY XSTRINITCOPY(PXSTRING pxstr, const char *pcszSource, ULONG ulExtraAllocate);
    typedef XSTRINITCOPY *PXSTRINITCOPY;

    void XWPENTRY xstrClear(PXSTRING pxstr);
    typedef void XWPENTRY XSTRCLEAR(PXSTRING pxstr);
    typedef XSTRCLEAR *PXSTRCLEAR;

    ULONG XWPENTRY xstrReserve(PXSTRING pxstr, ULONG ulBytes);
    typedef ULONG XWPENTRY XSTRRESERVE(PXSTRING pxstr, ULONG ulBytes);
    typedef XSTRRESERVE *PXSTRRESERVE;

    PXSTRING XWPENTRY xstrCreate(ULONG ulPreAllocate);
    typedef PXSTRING XWPENTRY XSTRCREATE(ULONG ulPreAllocate);
    typedef XSTRCREATE *PXSTRCREATE;

    VOID XWPENTRY xstrFree(PXSTRING pxstr);
    typedef VOID XWPENTRY XSTRFREE(PXSTRING pxstr);
    typedef XSTRFREE *PXSTRFREE;

    ULONG XWPENTRY xstrset(PXSTRING pxstr, PSZ pszNew);
    typedef ULONG XWPENTRY XSTRSET(PXSTRING pxstr, PSZ pszNew);
    typedef XSTRSET *PXSTRSET;

    ULONG XWPENTRY xstrcpy(PXSTRING pxstr, const char *pcszSource, ULONG ulSourceLength);
    typedef ULONG XWPENTRY XSTRCPY(PXSTRING pxstr, const char *pcszSource, ULONG ulSourceLength);
    typedef XSTRCPY *PXSTRCPY;

    ULONG XWPENTRY xstrcpys(PXSTRING pxstr, const XSTRING *pcstrSource);
    typedef ULONG XWPENTRY XSTRCPYS(PXSTRING pxstr, const XSTRING *pcstrSource);
    typedef XSTRCPYS *PXSTRCPYS;

    ULONG XWPENTRY xstrcat(PXSTRING pxstr, const char *pcszSource, ULONG ulSourceLength);
    typedef ULONG XWPENTRY XSTRCAT(PXSTRING pxstr, const char *pcszSource, ULONG ulSourceLength);
    typedef XSTRCAT *PXSTRCAT;

    ULONG XWPENTRY xstrcatc(PXSTRING pxstr, CHAR c);
    typedef ULONG XWPENTRY XSTRCATC(PXSTRING pxstr, CHAR c);
    typedef XSTRCATC *PXSTRCATC;

    ULONG XWPENTRY xstrcats(PXSTRING pxstr, const XSTRING *pcstrSource);
    typedef ULONG XWPENTRY XSTRCATS(PXSTRING pxstr, const XSTRING *pcstrSource);
    typedef XSTRCATS *PXSTRCATS;

    /*
     *@@ xstrIsString:
     *      returns TRUE if psz contains something.
     *
     *@@added V0.9.6 (2000-10-16) [umoeller]
     */

    #define xstrIsString(psz) ( (psz != 0) && (*(psz) != 0) )

    ULONG XWPENTRY xstrrpl(PXSTRING pxstr,
                           ULONG ulFirstReplOfs,
                           ULONG cReplLen,
                           const XSTRING *pstrReplaceWith);
    typedef ULONG XWPENTRY XSTRRPL(PXSTRING pxstr,
                                   ULONG ulFirstReplOfs,
                                   ULONG cReplLen,
                                   const XSTRING *pstrReplaceWith);
    typedef XSTRRPL *PXSTRRPL;

    PSZ XWPENTRY xstrFindWord(const XSTRING *pxstr,
                              ULONG ulOfs,
                              const XSTRING *pstrFind,
                              size_t *pShiftTable,
                              PBOOL pfRepeatFind,
                              const char *pcszBeginChars,
                              const char *pcszEndChars);
    typedef PSZ XWPENTRY XSTRFINDWORD(const XSTRING *pxstr,
                                      ULONG ulOfs,
                                      const XSTRING *pstrFind,
                                      size_t *pShiftTable,
                                      PBOOL pfRepeatFind,
                                      const char *pcszBeginChars,
                                      const char *pcszEndChars);
    typedef XSTRFINDWORD *PXSTRFINDWORD;

    ULONG XWPENTRY xstrFindReplace(PXSTRING pxstr,
                                   PULONG pulOfs,
                                   const XSTRING *pstrSearch,
                                   const XSTRING *pstrReplace,
                                   size_t *pShiftTable,
                                   PBOOL pfRepeatFind);
    typedef ULONG XWPENTRY XSTRFINDREPLACE(PXSTRING pxstr,
                                           PULONG pulOfs,
                                           const XSTRING *pstrSearch,
                                           const XSTRING *pstrReplace,
                                           size_t *pShiftTable,
                                           PBOOL pfRepeatFind);
    typedef XSTRFINDREPLACE *PXSTRFINDREPLACE;

    ULONG XWPENTRY xstrFindReplaceC(PXSTRING pxstr,
                                    PULONG pulOfs,
                                    const char *pcszSearch,
                                    const char *pcszReplace);
    typedef ULONG XWPENTRY XSTRFINDREPLACEC(PXSTRING pxstr,
                                            PULONG pulOfs,
                                            const char *pcszSearch,
                                            const char *pcszReplace);
    typedef XSTRFINDREPLACEC *PXSTRFINDREPLACEC;

    ULONG XWPENTRY xstrEncode(PXSTRING pxstr, const char *pcszEncode);
    typedef ULONG XWPENTRY XSTRENCODE(PXSTRING pxstr, const char *pcszEncode);
    typedef XSTRENCODE *PXSTRENCODE;

    ULONG XWPENTRY xstrDecode(PXSTRING pxstr);
    typedef ULONG XWPENTRY XSTRDECODE(PXSTRING pxstr);
    typedef XSTRDECODE *PXSTRDECODE;

    // V0.9.9 (2001-01-29) [lafaix]: constants added
    #define CRLF2LF TRUE
    #define LF2CRLF FALSE

    VOID XWPENTRY xstrConvertLineFormat(PXSTRING pxstr, BOOL fToCFormat);
    typedef VOID XWPENTRY XSTRCONVERTLINEFORMAT(PXSTRING pxstr, BOOL fToCFormat);
    typedef XSTRCONVERTLINEFORMAT *PXSTRCONVERTLINEFORMAT;

#endif

#if __cplusplus
}
#endif

