
/*
 *@@sourcefile apps.h:
 *      header file for apps.c. See remarks there.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@include #define INCL_WINPROGRAMLIST
 *@@include #include <os2.h>
 *@@include #include "apps.h"
 */

/*      This file Copyright (C) 1997-2001 Ulrich M”ller.
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

#ifndef APPS_HEADER_INCLUDED
    #define APPS_HEADER_INCLUDED

    /* ******************************************************************
     *
     *   Environment helpers
     *
     ********************************************************************/

    /*
     *@@ DOSENVIRONMENT:
     *      structure holding an array of environment
     *      variables (papszVars). This is initialized
     *      appGetEnvironment,
     *
     *@@added V0.9.4 (2000-07-19) [umoeller]
     */

    typedef struct _DOSENVIRONMENT
    {
        ULONG       cVars;              // count of vars in papzVars
        PSZ         *papszVars;         // array of PSZ's to environment strings (VAR=VALUE)
    } DOSENVIRONMENT, *PDOSENVIRONMENT;

    APIRET appParseEnvironment(const char *pcszEnv,
                                PDOSENVIRONMENT pEnv);

    APIRET appGetEnvironment(PDOSENVIRONMENT pEnv);

    PSZ* appFindEnvironmentVar(PDOSENVIRONMENT pEnv,
                                PSZ pszVarName);

    APIRET appSetEnvironmentVar(PDOSENVIRONMENT pEnv,
                                 PSZ pszNewEnv,
                                 BOOL fAddFirst);

    APIRET appConvertEnvironment(PDOSENVIRONMENT pEnv,
                                  PSZ *ppszEnv,
                                  PULONG pulSize);

    APIRET appFreeEnvironment(PDOSENVIRONMENT pEnv);

    /* ******************************************************************
     *
     *   Application start
     *
     ********************************************************************/

    #ifdef INCL_WINPROGRAMLIST
        // additional PROG_* flags for appQueryAppType
        #define PROG_XWP_DLL            998      // dynamic link library

        APIRET appQueryAppType(const char *pcszExecutable,
                                PULONG pulDosAppType,
                                PULONG pulWinAppType);

        ULONG appIsWindowsApp(ULONG ulProgCategory);

        PSZ appQueryDefaultWin31Environment(VOID);

        #define APP_RUN_FULLSCREEN      0x0001
        #define APP_RUN_ENHANCED        0x0002
        #define APP_RUN_STANDARD        0x0004
        #define APP_RUN_SEPARATE        0x0008

        HAPP XWPENTRY appStartApp(HWND hwndNotify,
                                  const PROGDETAILS *pcProgDetails,
                                  ULONG ulFlags);
    #endif

#endif

#if __cplusplus
}
#endif

