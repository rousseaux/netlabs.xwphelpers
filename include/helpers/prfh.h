
/*
 *@@sourcefile prfh.h:
 *      header file for prfh.c. See remarks there.
 *
 *      This file is new with V0.82.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@include #define INCL_WINWINDOWMGR
 *@@include #define INCL_WINSHELLDATA
 *@@include #include <os2.h>
 *@@include #include <stdio.h>
 *@@include #include "prfh.h"
 */

/*      Copyright (C) 1997-2000 Ulrich M”ller.
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

#ifndef PRFH_HEADER_INCLUDED
    #define PRFH_HEADER_INCLUDED

    /* common error codes used by the prfh* functions */

    #define PRFERR_DATASIZE     10001   // couldn't query data size for key
    #define PRFERR_READ         10003   // couldn't read data from source (PrfQueryProfileData error)
    #define PRFERR_WRITE        10004   // couldn't write data to target (PrfWriteProfileData error)
    #define PRFERR_APPSLIST     10005   // couldn't query apps list
    #define PRFERR_KEYSLIST     10006   // couldn't query keys list
    #define PRFERR_ABORTED      10007   // aborted by user
    #define PRFERR_QUERY        10007   // PrfQueryProfile failed
    #define PRFERR_INVALID_FILE_NAME  10008   // profile names don't contain .INI
    #define PRFERR_INVALID_KEY  10009
    #define PRFERR_KEY_EXISTS   10010

    APIRET prfhQueryKeysForApp(HINI hIni,
                               const char *pcszApp,
                               PSZ *ppszKeys);

    #ifdef __DEBUG_MALLOC_ENABLED__ // setup.h, helpers\memdebug.c
        PSZ prfhQueryProfileDataDebug(HINI hIni,
                                      const char *pcszApp,
                                      const char *pcszKey,
                                      PULONG pcbBuf,
                                      const char *file,
                                      unsigned long line,
                                      const char *function);
        #define prfhQueryProfileData(a, b, c, d) prfhQueryProfileDataDebug((a), (b), (c), (d), __FILE__, __LINE__, __FUNCTION__)
    #else
        PSZ prfhQueryProfileData(HINI hIni,
                                 const char *pcszApp,
                                 const char *pcszKey,
                                 PULONG pcbBuf);
    #endif

    CHAR prfhQueryProfileChar(HINI hini,
                              const char *pcszApp,
                              const char *pcszKey,
                              CHAR cDefault);

    LONG prfhQueryColor(const char *pcszKeyName, const char *pcszDefault);

    /*
     *@@ COUNTRYSETTINGS:
     *      structure used for returning country settings
     *      with prfhQueryCountrySettings.
     */

    typedef struct _COUNTRYSETTINGS
    {
        ULONG   ulDateFormat,
                        // date format:
                        // --  0   mm.dd.yyyy  (English)
                        // --  1   dd.mm.yyyy  (e.g. German)
                        // --  2   yyyy.mm.dd  (Japanese)
                        // --  3   yyyy.dd.mm
                ulTimeFormat;
                        // time format:
                        // --  0   12-hour clock
                        // --  >0  24-hour clock
        CHAR    cDateSep,
                        // date separator (e.g. '/')
                cTimeSep,
                        // time separator (e.g. ':')
                cDecimal,
                        // decimal separator (e.g. '.')
                cThousands;
                        // thousands separator (e.g. ',')
    } COUNTRYSETTINGS, *PCOUNTRYSETTINGS;

    VOID prfhQueryCountrySettings(PCOUNTRYSETTINGS pcs);

    APIRET prfhCopyKey(HINI hiniSource,
                       const char *pcszSourceApp,
                       const char *pcszKey,
                       HINI hiniTarget,
                       const char *pcszTargetApp);

    APIRET prfhCopyApp(HINI hiniSource,
                       const char *pcszSourceApp,
                       HINI hiniTarget,
                       const char *pcszTargetApp,
                       PSZ pszErrorKey);

    ULONG prfhRenameKey(HINI hini,
                        const char *pcszOldApp,
                        const char *pcszOldKey,
                        const char *pcszNewApp,
                        const char *pcszNewKey);

    BOOL prfhSetUserProfile(HAB hab,
                            const char *pcszUserProfile);

    ULONG prfhINIError(ULONG ulOptions,
                       FILE* fLog,
                       PFNWP fncbError,
                       PSZ pszErrorString);

    ULONG prfhINIError2(ULONG ulOptions,
                        const char *pcszINI,
                        FILE* fLog,
                        PFNWP fncbError,
                        PSZ pszErrorString);

    BOOL prfhCopyProfile(HAB hab,
                         FILE* fLog,
                         HINI hOld,
                         PSZ pszOld,
                         PSZ pszNew,
                         PFNWP fncbUpdate,
                         HWND hwnd, ULONG msg, ULONG ulCount, ULONG ulMax,
                         PFNWP fncbError);

    APIRET prfhSaveINIs(HAB hab,
                        FILE* fLog,
                        PFNWP fncbUpdate,
                        HWND hwnd, ULONG msg,
                        PFNWP fncbError);

#endif

#if __cplusplus
}
#endif

