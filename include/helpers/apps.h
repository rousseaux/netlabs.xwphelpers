
/*
 *@@sourcefile apps.h:
 *      header file for apps.c. See remarks there.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@include #define INCL_WINPROGRAMLIST
 *@@include #include <os2.h>
 *@@include #include "helpers\apps.h"
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

    ULONG appQueryEnvironmentLen(PCSZ pcszEnvironment);

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

    #ifdef INCL_WINPROGRAMLIST
        // additional PROG_* flags for appQueryAppType
        // #define PROG_XWP_DLL            998      // dynamic link library
                    // removed, PROG_DLL exists already
                    // V0.9.16 (2001-10-06)

        #define PROG_WIN32              990     // added V0.9.16 (2001-12-08) [umoeller]

        APIRET appQueryAppType(const char *pcszExecutable,
                                PULONG pulDosAppType,
                                PULONG pulWinAppType);

        PCSZ appDescribeAppType(PROGCATEGORY progc);

        ULONG appIsWindowsApp(ULONG ulProgCategory);

    /* ******************************************************************
     *
     *   Application start
     *
     ********************************************************************/


        PSZ appQueryDefaultWin31Environment(VOID);

        #define APP_RUN_FULLSCREEN      0x0001
        #define APP_RUN_ENHANCED        0x0002
        #define APP_RUN_STANDARD        0x0004
        #define APP_RUN_SEPARATE        0x0008

        #ifdef XSTRING_HEADER_INCLUDED
        APIRET appFixProgDetails(PPROGDETAILS pDetails,
                                 const PROGDETAILS *pcProgDetails,
                                 ULONG ulFlags,
                                 PXSTRING pstrExecutablePatched,
                                 PXSTRING pstrParamsPatched,
                                 PSZ *ppszWinOS2Env);
        #endif

        APIRET XWPENTRY appStartApp(HWND hwndNotify,
                                    const PROGDETAILS *pcProgDetails,
                                    ULONG ulFlags,
                                    HAPP *phapp,
                                    ULONG cbFailingName,
                                    PSZ pszFailingName);

        BOOL XWPENTRY appWaitForApp(HWND hwndNotify,
                                    HAPP happ,
                                    PULONG pulExitCode);

        HAPP XWPENTRY appQuickStartApp(const char *pcszFile,
                                       ULONG ulProgType,
                                       const char *pcszArgs,
                                       PULONG pulExitCode);

    #endif

#endif

#if __cplusplus
}
#endif

