
/*
 *@@sourcefile stringh.h:
 *      header file for stringh.c. See notes there.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@include #define INCL_DOSDATETIME
 *@@include #include <os2.h>
 *@@include #include "stringh.h"
 */

/*
 *      Copyright (C) 1997-2000 Ulrich M”ller.
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

#ifndef STRINGH_HEADER_INCLUDED
    #define STRINGH_HEADER_INCLUDED

    PSZ strhcpy(PSZ string1, const char *string2);

    #if defined(__DEBUG_MALLOC_ENABLED__) && !defined(DONT_REPLACE_STRINGH_MALLOC) // setup.h, helpers\memdebug.c
        PSZ strhdupDebug(const char *pszSource,
                         const char *pcszSourceFile,
                         unsigned long ulLine,
                         const char *pcszFunction);
        #define strhdup(a) strhdupDebug((a), __FILE__, __LINE__, __FUNCTION__)
    #else
        PSZ strhdup(const char *pszSource);
    #endif

    int strhcmp(const char *p1, const char *p2);

    int strhicmp(const char *p1, const char *p2);

    PSZ strhistr(const char *string1, const char *string2);

    ULONG strhncpy0(PSZ pszTarget,
                    const char *pszSource,
                    ULONG cbSource);

    ULONG strhCount(const char *pszSearch, CHAR c);

    BOOL strhIsDecimal(PSZ psz);

    #if defined(__DEBUG_MALLOC_ENABLED__) && !defined(DONT_REPLACE_STRINGH_MALLOC) // setup.h, helpers\memdebug.c
        PSZ strhSubstrDebug(const char *pBegin,      // in: first char
                            const char *pEnd,        // in: last char (not included)
                            const char *pcszSourceFile,
                            unsigned long ulLine,
                            const char *pcszFunction);
        #define strhSubstr(a, b) strhSubstrDebug((a), (b), __FILE__, __LINE__, __FUNCTION__)
    #else
        PSZ strhSubstr(const char *pBegin, const char *pEnd);
    #endif

    PSZ strhExtract(PSZ pszBuf,
                    CHAR cOpen,
                    CHAR cClose,
                    PSZ *ppEnd);

    PSZ strhQuote(PSZ pszBuf,
                  CHAR cQuote,
                  PSZ *ppEnd);

    ULONG strhStrip(PSZ psz);

    PSZ strhins(const char *pcszBuffer,
                ULONG ulInsertOfs,
                const char *pcszInsert);

    ULONG strhFindReplace(PSZ *ppszBuf,
                          PULONG pulOfs,
                          const char *pcszSearch,
                          const char *pcszReplace);

    ULONG strhWords(PSZ psz);

    PSZ APIENTRY strhThousandsULong(PSZ pszTarget, ULONG ul, CHAR cThousands);
    typedef PSZ APIENTRY STRHTHOUSANDSULONG(PSZ pszTarget, ULONG ul, CHAR cThousands);
    typedef STRHTHOUSANDSULONG *PSTRHTHOUSANDSULONG;

    PSZ strhThousandsDouble(PSZ pszTarget, double dbl, CHAR cThousands);

    PSZ strhVariableDouble(PSZ pszTarget, double dbl, PSZ pszUnits,
                           CHAR cThousands);

    VOID strhFileDate(PSZ pszBuf,
                      FDATE *pfDate,
                      ULONG ulDateFormat,
                      CHAR cDateSep);

    VOID strhFileTime(PSZ pszBuf,
                      FTIME *pfTime,
                      ULONG ulTimeFormat,
                      CHAR cTimeSep);

    VOID APIENTRY strhDateTime(PSZ pszDate,
                               PSZ pszTime,
                               DATETIME *pDateTime,
                               ULONG ulDateFormat,
                               CHAR cDateSep,
                               ULONG ulTimeFormat,
                               CHAR cTimeSep);
    typedef VOID APIENTRY STRHDATETIME(PSZ pszDate,
                               PSZ pszTime,
                               DATETIME *pDateTime,
                               ULONG ulDateFormat,
                               CHAR cDateSep,
                               ULONG ulTimeFormat,
                               CHAR cTimeSep);
    typedef STRHDATETIME *PSTRHDATETIME;

    #define STRH_BEGIN_CHARS    "\x0d\x0a "
    #define STRH_END_CHARS      "\x0d\x0a /-"

    BOOL strhGetWord(PSZ *ppszStart,
                     const char *pLimit,
                     const char *pcszBeginChars,
                     const char *pcszEndChars, //  = "\x0d\x0a /-";
                     PSZ *ppszEnd);

    BOOL strhIsWord(const char *pcszBuf,
                    const char *p,
                    ULONG cbSearch,
                    const char *pcszBeginChars,
                    const char *pcszEndChars);

    PSZ strhFindWord(const char *pszBuf,
                     const char *pszSearch,
                     const char *pcszBeginChars,
                     const char *pcszEndChars);

    PSZ strhFindEOL(const char *pcszSearchIn, ULONG *pulOffset);

    PSZ strhFindNextLine(PSZ pszSearchIn, PULONG pulOffset);

    BOOL strhBeautifyTitle(PSZ psz);

    PSZ strhFindAttribValue(const char *pszSearchIn, const char *pszAttrib);

    PSZ strhGetNumAttribValue(const char *pszSearchIn,
                              const char *pszTag,
                              PLONG pl);

    PSZ strhGetTextAttr(const char *pszSearchIn, const char *pszTag, PULONG pulOffset);

    PSZ strhFindEndOfTag(const char *pszBeginOfTag);

    ULONG strhGetBlock(const char *pszSearchIn,
                       PULONG pulSearchOffset,
                       PSZ pszTag,
                       PSZ *ppszBlock,
                       PSZ *ppszAttribs,
                       PULONG pulOfsBeginTag,
                       PULONG pulOfsBeginBlock);

    /* ******************************************************************
     *
     *   Miscellaneous
     *
     ********************************************************************/

    VOID XWPENTRY strhArrayAppend(PSZ *ppszRoot,
                                  const char *pcszNew,
                                  ULONG cbNew,
                                  PULONG pcbRoot);

    PSZ strhCreateDump(PBYTE pb,
                       ULONG ulSize,
                       ULONG ulIndent);

    /* ******************************************************************
     *
     *   Wildcard matching
     *
     ********************************************************************/

    #define FNM_MATCH           0
    #define FNM_NOMATCH         1
    #define FNM_ERR             2

    #define FNM_NOESCAPE        16
    #define FNM_PATHNAME        32
    #define FNM_PERIOD          64

    #define FNM_STYLE_MASK      15

    #define FNM_POSIX           0
    #define FNM_OS2             1
    #define FNM_DOS             2

    #define FNM_IGNORECASE      128
    #define FNM_PATHPREFIX      256

    BOOL strhMatchOS2(const char *pcszMask, const char *pcszName);

    BOOL strhMatchExt(const char *pcszMask,
                      const char *pcszName,
                      unsigned flags);

    /* ******************************************************************
     *
     *   Fast string searches
     *
     ********************************************************************/

    void* strhmemfind(const void *in_block,
                      size_t block_size,
                      const void *in_pattern,
                      size_t pattern_size,
                      size_t *shift,
                      BOOL *repeat_find);

    char* strhtxtfind (const char *string,
                       const char *pattern);

#endif

#if __cplusplus
}
#endif

