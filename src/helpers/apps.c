
/*
 *@@sourcefile apps.c:
 *      contains program helpers (environments, application start).
 *
 *      This file is new with V0.9.12 and contains functions
 *      previously in winh.c and dosh2.c.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@header "helpers\apps.h"
 *@@added V0.9.12 (2001-05-26) [umoeller]
 */

/*
 *      Copyright (C) 1997-2001 Ulrich M”ller.
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

#define OS2EMX_PLAIN_CHAR
    // this is needed for "os2emx.h"; if this is defined,
    // emx will define PSZ as _signed_ char, otherwise
    // as unsigned char

#define INCL_DOSPROCESS
#define INCL_DOSSESMGR
#define INCL_DOSERRORS

#define INCL_WINPROGRAMLIST     // needed for PROGDETAILS, wppgm.h
#include <os2.h>

#include <stdio.h>

#include "setup.h"                      // code generation and debugging options

#include "helpers\apps.h"
#include "helpers\dosh.h"
#include "helpers\prfh.h"
#include "helpers\stringh.h"
#include "helpers\xstring.h"

/*
 *@@category: Helpers\PM helpers\Application helpers
 */

/* ******************************************************************
 *
 *   Environment helpers
 *
 ********************************************************************/

/*
 *@@ appParseEnvironment:
 *      this takes one of those ugly environment strings
 *      as used by DosStartSession and WinStartApp (with
 *      lots of zero-terminated strings one after another
 *      and a duplicate zero byte as a terminator) as
 *      input and splits it into an array of separate
 *      strings in pEnv.
 *
 *      The newly allocated strings are stored in in
 *      pEnv->papszVars. The array count is stored in
 *      pEnv->cVars.
 *
 *      Each environment variable will be copied into
 *      one newly allocated string in the array. Use
 *      appFreeEnvironment to free the memory allocated
 *      by this function.
 *
 *      Use the following code to browse thru the array:
 +
 +          DOSENVIRONMENT Env = {0};
 +          if (appParseEnvironment(pszEnv,
 +                                   &Env)
 +                  == NO_ERROR)
 +          {
 +              if (Env.papszVars)
 +              {
 +                  PSZ *ppszThis = Env.papszVars;
 +                  for (ul = 0;
 +                       ul < Env.cVars;
 +                       ul++)
 +                  {
 +                      PSZ pszThis = *ppszThis;
 +                      // pszThis now has something like PATH=C:\TEMP
 +                      // ...
 +                      // next environment string
 +                      ppszThis++;
 +                  }
 +              }
 +              appFreeEnvironment(&Env);
 +          }
 *
 *@@added V0.9.4 (2000-08-02) [umoeller]
 */

APIRET appParseEnvironment(const char *pcszEnv,
                           PDOSENVIRONMENT pEnv)
{
    APIRET arc = NO_ERROR;
    if (!pcszEnv)
        arc = ERROR_INVALID_PARAMETER;
    else
    {
        PSZ     pszVarThis = (PSZ)pcszEnv;
        ULONG   cVars = 0;
        // count strings
        while (*pszVarThis)
        {
            cVars++;
            pszVarThis += strlen(pszVarThis) + 1;
        }

        pEnv->cVars = cVars;
        pEnv->papszVars = 0;

        if (cVars)
        {
            PSZ *papsz = (PSZ*)malloc(sizeof(PSZ) * cVars);
            if (!papsz)
                arc = ERROR_NOT_ENOUGH_MEMORY;
            else
            {
                PSZ *ppszTarget = papsz;
                memset(papsz, 0, sizeof(PSZ) * cVars);
                pszVarThis = (PSZ)pcszEnv;
                while (*pszVarThis)
                {
                    *ppszTarget = strdup(pszVarThis);
                    ppszTarget++;
                    pszVarThis += strlen(pszVarThis) + 1;
                }

                pEnv->papszVars = papsz;
            }
        }
    }

    return (arc);
}

/*
 *@@ appGetEnvironment:
 *      calls appParseEnvironment for the current
 *      process environment, which is retrieved from
 *      the info blocks.
 *
 *@@added V0.9.4 (2000-07-19) [umoeller]
 */

APIRET appGetEnvironment(PDOSENVIRONMENT pEnv)
{
    APIRET  arc = NO_ERROR;
    if (!pEnv)
        arc = ERROR_INVALID_PARAMETER;
    else
    {
        PTIB    ptib = 0;
        PPIB    ppib = 0;
        arc = DosGetInfoBlocks(&ptib, &ppib);
        if (arc == NO_ERROR)
        {
            PSZ pszEnv = ppib->pib_pchenv;
            if (pszEnv)
                arc = appParseEnvironment(pszEnv, pEnv);
            else
                arc = ERROR_BAD_ENVIRONMENT;
        }
    }

    return (arc);
}

/*
 *@@ appFindEnvironmentVar:
 *      returns the PSZ* in the pEnv->papszVars array
 *      which specifies the environment variable in pszVarName.
 *
 *      With pszVarName, you can either specify the variable
 *      name only ("VARNAME") or a full environment string
 *      ("VARNAME=BLAH"). In any case, only the variable name
 *      is compared.
 *
 *      Returns NULL if no such variable name was found in
 *      the array.
 *
 *@@added V0.9.4 (2000-07-19) [umoeller]
 *@@changed V0.9.12 (2001-05-21) [umoeller]: fixed memory leak
 */

PSZ* appFindEnvironmentVar(PDOSENVIRONMENT pEnv,
                            PSZ pszVarName)
{
    PSZ *ppszRet = 0;
    if (pEnv)
    {
        if ((pEnv->papszVars) && (pszVarName))
        {
            PSZ     *ppszThis = pEnv->papszVars;
            PSZ     pszThis;
            ULONG   ul = 0;
            ULONG   ulVarNameLen = 0;

            PSZ     pszSearch = NULL; // receives "VARNAME="
            PSZ     pFirstEqual = strchr(pszVarName, '=');
            if (pFirstEqual)
                pszSearch = strhSubstr(pszVarName, pFirstEqual + 1);
            else
            {
                ulVarNameLen = strlen(pszVarName);
                pszSearch = (PSZ)malloc(ulVarNameLen + 2);
                memcpy(pszSearch, pszVarName, ulVarNameLen);
                *(pszSearch + ulVarNameLen) = '=';
                *(pszSearch + ulVarNameLen + 1) = 0;
            }

            ulVarNameLen = strlen(pszSearch);

            for (ul = 0;
                 ul < pEnv->cVars;
                 ul++)
            {
                pszThis = *ppszThis;

                if (strnicmp(*ppszThis, pszSearch, ulVarNameLen) == 0)
                {
                    ppszRet = ppszThis;
                    break;
                }

                // next environment string
                ppszThis++;
            }

            free(pszSearch);        // was missing V0.9.12 (2001-05-21) [umoeller]
        }
    }

    return (ppszRet);
}

/*
 *@@ appSetEnvironmentVar:
 *      sets an environment variable in the specified
 *      environment, which must have been initialized
 *      using appGetEnvironment first.
 *
 *      pszNewEnv must be a full environment string
 *      in the form "VARNAME=VALUE".
 *
 *      If "VARNAME" has already been set to something
 *      in the string array in pEnv, that array item
 *      is replaced.
 *
 *      OTOH, if "VARNAME" has not been set yet, a new
 *      item is added to the array, and pEnv->cVars is
 *      raised by one. In that case, fAddFirst determines
 *      whether the new array item is added to the front
 *      or the tail of the environment list.
 *
 *@@added V0.9.4 (2000-07-19) [umoeller]
 *@@changed V0.9.7 (2000-12-17) [umoeller]: added fAddFirst
 *@@changed V0.9.12 (2001-05-21) [umoeller]: fixed memory leak
 *@@changed V0.9.12 (2001-05-26) [umoeller]: fixed crash if !fAddFirst
 */

APIRET appSetEnvironmentVar(PDOSENVIRONMENT pEnv,
                            PSZ pszNewEnv,
                            BOOL fAddFirst)
{
    APIRET  arc = NO_ERROR;
    if ((!pEnv) || (!pszNewEnv))
        arc = ERROR_INVALID_PARAMETER;
    else
    {
        if (!pEnv->papszVars)
        {
            // no variables set yet:
            pEnv->papszVars = (PSZ*)malloc(sizeof(PSZ));
            pEnv->cVars = 1;

            *(pEnv->papszVars) = strdup(pszNewEnv);
        }
        else
        {
            PSZ *ppszEnvLine = appFindEnvironmentVar(pEnv, pszNewEnv);
            if (ppszEnvLine)
            {
                // was set already: replace
                free(*ppszEnvLine);
                *ppszEnvLine = strdup(pszNewEnv);
                if (!(*ppszEnvLine))
                    arc = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                // not set already:
                PSZ *ppszNew = NULL;

                // allocate new array, with one new entry
                // fixed V0.9.12 (2001-05-26) [umoeller], this crashed
                PSZ *papszNew = (PSZ*)malloc(sizeof(PSZ) * (pEnv->cVars + 1));

                if (!papszNew)
                    arc = ERROR_NOT_ENOUGH_MEMORY;
                else
                {
                    if (fAddFirst)
                    {
                        // add as first entry:
                        // overwrite first entry
                        ppszNew = papszNew;
                        // copy old entries
                        memcpy(papszNew + 1,                // second new entry
                               pEnv->papszVars,             // first old entry
                               sizeof(PSZ) * pEnv->cVars);
                    }
                    else
                    {
                        // append at the tail:
                        // overwrite last entry
                        ppszNew = papszNew + pEnv->cVars;
                        // copy old entries
                        memcpy(papszNew,                    // first new entry
                               pEnv->papszVars,             // first old entry
                               sizeof(PSZ) * pEnv->cVars);
                    }

                    if (pEnv->papszVars)
                        free(pEnv->papszVars);      // was missing V0.9.12 (2001-05-21) [umoeller]

                    pEnv->papszVars = papszNew;
                    pEnv->cVars++;
                    *ppszNew = strdup(pszNewEnv);
                }
            }
        }
    }

    return (arc);
}

/*
 *@@ appConvertEnvironment:
 *      converts an environment initialized by appGetEnvironment
 *      to the string format required by WinStartApp and DosExecPgm,
 *      that is, one memory block is allocated in *ppszEnv and all
 *      strings in pEnv->papszVars are copied to that block. Each
 *      string is terminated with a null character; the last string
 *      is terminated with two null characters.
 *
 *      Use free() to free the memory block allocated by this
 *      function in *ppszEnv.
 *
 *@@added V0.9.4 (2000-07-19) [umoeller]
 */

APIRET appConvertEnvironment(PDOSENVIRONMENT pEnv,
                             PSZ *ppszEnv,     // out: environment string
                             PULONG pulSize)  // out: size of block allocated in *ppszEnv; ptr can be NULL
{
    APIRET  arc = NO_ERROR;
    if (!pEnv)
        arc = ERROR_INVALID_PARAMETER;
    else
    {
        if (!pEnv->papszVars)
            arc = ERROR_INVALID_PARAMETER;
        else
        {
            // count memory needed for all strings
            ULONG   cbNeeded = 0,
                    ul = 0;
            PSZ     *ppszThis = pEnv->papszVars;

            for (ul = 0;
                 ul < pEnv->cVars;
                 ul++)
            {
                cbNeeded += strlen(*ppszThis) + 1; // length of string plus null terminator

                // next environment string
                ppszThis++;
            }

            cbNeeded++;     // for another null terminator

            *ppszEnv = (PSZ)malloc(cbNeeded);
            if (!(*ppszEnv))
                arc = ERROR_NOT_ENOUGH_MEMORY;
            else
            {
                PSZ     pTarget = *ppszEnv;
                if (pulSize)
                    *pulSize = cbNeeded;
                ppszThis = pEnv->papszVars;

                // now copy each string
                for (ul = 0;
                     ul < pEnv->cVars;
                     ul++)
                {
                    PSZ pSource = *ppszThis;

                    while ((*pTarget++ = *pSource++))
                        ;

                    // *pTarget++ = 0;     // append null terminator per string

                    // next environment string
                    ppszThis++;
                }

                *pTarget++ = 0;     // append second null terminator
            }
        }
    }

    return (arc);
}

/*
 *@@ appFreeEnvironment:
 *      frees memory allocated by appGetEnvironment.
 *
 *@@added V0.9.4 (2000-07-19) [umoeller]
 */

APIRET appFreeEnvironment(PDOSENVIRONMENT pEnv)
{
    APIRET  arc = NO_ERROR;
    if (!pEnv)
        arc = ERROR_INVALID_PARAMETER;
    else
    {
        if (!pEnv->papszVars)
            arc = ERROR_INVALID_PARAMETER;
        else
        {
            PSZ     *ppszThis = pEnv->papszVars;
            PSZ     pszThis;
            ULONG   ul = 0;

            for (ul = 0;
                 ul < pEnv->cVars;
                 ul++)
            {
                pszThis = *ppszThis;
                free(pszThis);
                // *ppszThis = NULL;
                // next environment string
                ppszThis++;
            }

            free(pEnv->papszVars);
            pEnv->cVars = 0;
        }
    }

    return (arc);
}

/* ******************************************************************
 *
 *   Application start
 *
 ********************************************************************/

/*
 *@@ CallBatchCorrectly:
 *      fixes the specified PROGDETAILS for
 *      command files in the executable part
 *      by inserting /C XXX into the parameters
 *      and setting the executable according
 *      to an environment variable.
 *
 *@@added V0.9.6 (2000-10-16) [umoeller]
 *@@changed V0.9.7 (2001-01-15) [umoeller]: now using XSTRING
 */

VOID CallBatchCorrectly(PPROGDETAILS pProgDetails,
                        PXSTRING pstrParams,        // in/out: modified parameters (reallocated)
                        const char *pcszEnvVar,     // in: env var spec'g command proc
                                                    // (e.g. "OS2_SHELL"); can be NULL
                        const char *pcszDefProc)    // in: def't command proc (e.g. "CMD.EXE")
{
    // XXX.CMD file as executable:
    // fix args to /C XXX.CMD

    PSZ     pszOldParams = NULL;
    ULONG   ulOldParamsLength = pstrParams->ulLength;
    if (ulOldParamsLength)
        // we have parameters already:
        // make a backup... we'll append that later
        pszOldParams = strdup(pstrParams->psz);

    // set new params to "/C filename.cmd"
    xstrcpy(pstrParams, "/C ", 0);
    xstrcat(pstrParams,
            pProgDetails->pszExecutable,
            0);

    if (pszOldParams)
    {
        // .cmd had params:
        // append space and old params
        xstrcatc(pstrParams, ' ');
        xstrcat(pstrParams,
                pszOldParams,
                ulOldParamsLength);
        free(pszOldParams);
    }

    // set executable to $(OS2_SHELL)
    pProgDetails->pszExecutable = NULL;
    if (pcszEnvVar)
        pProgDetails->pszExecutable = getenv(pcszEnvVar);
    if (!pProgDetails->pszExecutable)
        pProgDetails->pszExecutable = (PSZ)pcszDefProc;
                // should be on PATH
}

/*
 *@@ appQueryAppType:
 *      returns the Control Program (Dos) and
 *      Win* PROG_* application types for the
 *      specified executable. Essentially, this
 *      is a wrapper around DosQueryAppType.
 *
 *      pcszExecutable must be fully qualified.
 *      You can use doshFindExecutable to qualify
 *      it.
 *
 *      This returns the APIRET of DosQueryAppType.
 *      If this is NO_ERROR; *pulDosAppType receives
 *      the app type of DosQueryAppType. In addition,
 *      *pulWinAppType is set to one of the following:
 *
 *      --  PROG_FULLSCREEN
 *
 *      --  PROG_PDD
 *
 *      --  PROG_VDD
 *
 *      --  PROG_XWP_DLL: new apptype defined in winh.h for
 *          dynamic link libraries.
 *
 *      --  PROG_WINDOWEDVDM
 *
 *      --  PROG_PM
 *
 *      --  PROG_31_ENH
 *
 *      --  PROG_WINDOWABLEVIO
 *
 *@@added V0.9.9 (2001-03-07) [umoeller]
 */

APIRET appQueryAppType(const char *pcszExecutable,
                        PULONG pulDosAppType,
                        PULONG pulWinAppType)
{
    APIRET arc = DosQueryAppType((PSZ)pcszExecutable, pulDosAppType);
    if (arc == NO_ERROR)
    {
        ULONG _ulDosAppType = *pulDosAppType;

        if (_ulDosAppType == 0)
            *pulWinAppType = PROG_FULLSCREEN;
        else if (_ulDosAppType & 0x40)
            *pulWinAppType = PROG_PDD;
        else if (_ulDosAppType & 0x80)
            *pulWinAppType = PROG_VDD;
        else if ((_ulDosAppType & 0xF0) == 0x10)
            // DLL bit set
            *pulWinAppType = PROG_XWP_DLL;
        else if (_ulDosAppType & 0x20)
            // DOS bit set?
            *pulWinAppType = PROG_WINDOWEDVDM;
        else if ((_ulDosAppType & 0x0003) == 0x0003) // "Window-API" == PM
            *pulWinAppType = PROG_PM;
        else if (   ((_ulDosAppType & 0xFFFF) == 0x1000) // windows program (?!?)
                 || ((_ulDosAppType & 0xFFFF) == 0x0400) // windows program (?!?)
                )
            *pulWinAppType = PROG_31_ENH;
            // *pulWinAppType = PROG_31_ENHSEAMLESSVDM;
        else if ((_ulDosAppType & 0x03) == 0x02)
            *pulWinAppType = PROG_WINDOWABLEVIO;
        else if ((_ulDosAppType & 0x03) == 0x01)
            *pulWinAppType = PROG_FULLSCREEN;
    }

    return (arc);
}

/*
 *@@ appIsWindowsApp:
 *      checks the specified program category
 *      (PROGDETAILS.progt.progc) for whether
 *      it represents a Win-OS/2 application.
 *
 *      Returns:
 *
 *      -- 0: no windows app (it's VIO, OS/2
 *            or DOS fullscreen, or PM).
 *
 *      -- 1: Win-OS/2 standard app.
 *
 *      -- 2: Win-OS/2 enhanced-mode app.
 *
 *@@added V0.9.12 (2001-05-26) [umoeller]
 */

ULONG appIsWindowsApp(ULONG ulProgCategory)
{
    switch (ulProgCategory)
    {
        case PROG_31_ENHSEAMLESSVDM:        // 17
        case PROG_31_ENHSEAMLESSCOMMON:     // 18
        case PROG_31_ENH:                   // 19
            return (2);

#ifndef PROG_30_STD
    #define PROG_30_STD (PROGCATEGORY)11
#endif

#ifndef PROG_30_STDSEAMLESSVDM
    #define PROG_30_STDSEAMLESSVDM (PROGCATEGORY)13
#endif

        case PROG_WINDOW_REAL:              // 10
        case PROG_30_STD:                   // 11
        case PROG_WINDOW_AUTO:              // 12
        case PROG_30_STDSEAMLESSVDM:        // 13
        case PROG_30_STDSEAMLESSCOMMON:     // 14
        case PROG_31_STDSEAMLESSVDM:        // 15
        case PROG_31_STDSEAMLESSCOMMON:     // 16
        case PROG_31_STD:                   // 20
            return (1);
    }

    return (0);
}

/*
 *@@ appQueryDefaultWin31Environment:
 *      returns the default Win-OS/2 3.1 environment
 *      from OS2.INI, which you can then merge with
 *      your process environment to be able to
 *      start Win-OS/2 sessions properly with
 *      appStartApp.
 *
 *      Caller must free() the return value.
 *
 *@@added V0.9.12 (2001-05-26) [umoeller]
 */

PSZ appQueryDefaultWin31Environment(VOID)
{
    PSZ pszReturn = NULL;
    ULONG ulSize = 0;
    // get default environment (from Win-OS/2 settings object)
    // from OS2.INI
    PSZ pszDefEnv = prfhQueryProfileData(HINI_USER,
                                         "WINOS2",
                                         "PM_GlobalWindows31Settings",
                                         &ulSize);
    if (pszDefEnv)
    {
        if (pszReturn = (PSZ)malloc(ulSize + 2))
        {
            PSZ p;
            memset(pszReturn, 0, ulSize + 2);
            memcpy(pszReturn, pszDefEnv, ulSize);

            for (p = pszReturn;
                 p < pszReturn + ulSize;
                 p++)
                if (*p == ';')
                    *p = 0;

            // okay.... now we got an OS/2-style environment
            // with 0, 0, 00 strings
        }

        free(pszDefEnv);
    }

    return (pszReturn);
}

/*
 *@@ appStartApp:
 *      wrapper around WinStartApp which fixes the
 *      specified PROGDETAILS to (hopefully) work
 *      work with all executable types.
 *
 *      This fixes the executable info to support:
 *
 *      -- starting "*" executables (command prompts
 *         for OS/2, DOS, Win-OS/2);
 *
 *      -- starting ".CMD" and ".BAT" files as
 *         PROGDETAILS.pszExecutable.
 *
 *      Unless it is "*", PROGDETAILS.pszExecutable must
 *      be a proper file name. The full path may be omitted
 *      if it is on the PATH, but the extension (.EXE etc.)
 *      must be given. You can use doshFindExecutable to
 *      find executables if you don't know the extension.
 *
 *      This also handles and merges special and default
 *      environments for the app to be started. The
 *      following should be respected:
 *
 *      --  As with WinStartApp, if PROGDETAILS.pszEnvironment
 *          is NULL, the new app inherits a default environment
 *          from the shell.
 *
 +      --  However, if you specify an environment, you _must_
 *          specify a complemete environment. This function
 *          will not merge environments. Use
 *          appSetEnvironmentVar to change environment
 *          variables in a complete environment set.
 *
 *      --  If PROGDETAILS specifies a Win-OS/2 session
 *          and PROGDETAILS.pszEnvironment is empty,
 *          this merges the current process environment
 *          with the default Win-OS/2 environment.
 *          See appQueryDefaultWin31Environment.
 *
 *      Even though this isn't clearly said in PMREF,
 *      PROGDETAILS.swpInitial is important:
 *
 *      -- To start a session minimized, set SWP_MINIMIZE.
 *
 *      -- To start a VIO session auto-close disabled, set
 *         the half-documented SWP_NOAUTOCLOSE flag (0x8000)
 *         This flag is now in the newer toolkit headers.
 *
 *      Since this calls WinStartApp in turn, this
 *      requires a message queue on the calling thread.
 *
 *@@added V0.9.6 (2000-10-16) [umoeller]
 *@@changed V0.9.7 (2000-12-10) [umoeller]: PROGDETAILS.swpInitial no longer zeroed... this broke VIOs
 *@@changed V0.9.7 (2000-12-17) [umoeller]: PROGDETAILS.pszEnvironment no longer zeroed
 *@@changed V0.9.9 (2001-01-27) [umoeller]: crashed if PROGDETAILS.pszExecutable was NULL
 *@@changed V0.9.12 (2001-05-26) [umoeller]: fixed PROG_DEFAULT
 */

HAPP appStartApp(HWND hwndNotify,                  // in: notify window (as with WinStartApp)
                 const PROGDETAILS *pcProgDetails) // in: program data
{
    HAPP            happ = NULLHANDLE;
    XSTRING         strParamsPatched;
    PROGDETAILS     ProgDetails;

    PSZ             pszWinOS2Env = 0;

    memcpy(&ProgDetails, pcProgDetails, sizeof(PROGDETAILS));
            // pointers still point into old prog details buffer
    ProgDetails.Length = sizeof(PROGDETAILS);
    ProgDetails.progt.fbVisible = SHE_VISIBLE;
    // ProgDetails.pszEnvironment = 0;

    // all this only makes sense if this contains something...
    // besides, this crashed on string comparisons V0.9.9 (2001-01-27) [umoeller]
    if (ProgDetails.pszExecutable)
    {
        ULONG ulIsWinApp;

        // memset(&ProgDetails.swpInitial, 0, sizeof(SWP));
        // this wasn't a good idea... WPProgram stores stuff
        // in here, such as the "minimize on startup" -> SWP_MINIMIZE

        // duplicate parameters...
        // we need this for string manipulations below...
        if (ProgDetails.pszParameters)
            xstrInitCopy(&strParamsPatched,
                         ProgDetails.pszParameters,
                         100);
        else
            // no old params:
            xstrInit(&strParamsPatched, 100);

        // _Pmpf((__FUNCTION__ ": old progc: 0x%lX", pcProgDetails->progt.progc));
        // _Pmpf(("  pszTitle: %s", (ProgDetails.pszTitle) ? ProgDetails.pszTitle : NULL));
        // _Pmpf(("  pszIcon: %s", (ProgDetails.pszIcon) ? ProgDetails.pszIcon : NULL));

        // program type fixups
        switch (ProgDetails.progt.progc)        // that's a ULONG
        {
            case ((ULONG)-1):       // we get that sometimes...
            case PROG_DEFAULT:
            {
                // V0.9.12 (2001-05-26) [umoeller]
                ULONG ulDosAppType;
                appQueryAppType(ProgDetails.pszExecutable,
                                &ulDosAppType,
                                &ProgDetails.progt.progc);
            }
            break;
        }

        ulIsWinApp = appIsWindowsApp(ProgDetails.progt.progc);

        // now try again...

        /*
         * command lines fixups:
         *
         */

        if (strcmp(ProgDetails.pszExecutable, "*") == 0)
        {
            /*
             * "*" for command sessions:
             *
             */

            if (ulIsWinApp == 2)
            {
                // enhanced Win-OS/2 session:
                PSZ psz = NULL;
                if (strParamsPatched.ulLength)
                    // "/3 " + existing params
                    psz = strdup(strParamsPatched.psz);

                xstrcpy(&strParamsPatched, "/3 ", 0);

                if (psz)
                {
                    xstrcat(&strParamsPatched, psz, 0);
                    free(psz);
                }
            }

            if (ulIsWinApp)
            {
                // cheat: WinStartApp doesn't support NULL
                // for Win-OS2 sessions, so manually start winos2.com
                ProgDetails.pszExecutable = "WINOS2.COM";
                // this is a DOS app, so fix this to DOS fullscreen
                ProgDetails.progt.progc = PROG_VDM;
            }
            else
                // for all other executable types
                // (including OS/2 and DOS sessions),
                // set pszExecutable to NULL; this will
                // have WinStartApp start a cmd shell
                ProgDetails.pszExecutable = NULL;

        } // end if (strcmp(pProgDetails->pszExecutable, "*") == 0)
        else
            switch (ProgDetails.progt.progc)
            {
                /*
                 *  .CMD files fixups
                 *
                 */

                case PROG_FULLSCREEN:       // OS/2 fullscreen
                case PROG_WINDOWABLEVIO:    // OS/2 window
                {
                    PSZ pszExtension = doshGetExtension(ProgDetails.pszExecutable);
                    if (pszExtension)
                    {
                        if (stricmp(pszExtension, "CMD") == 0)
                        {
                            CallBatchCorrectly(&ProgDetails,
                                               &strParamsPatched,
                                               "OS2_SHELL",
                                               "CMD.EXE");
                        }
                    }
                break; }

                case PROG_VDM:              // DOS fullscreen
                case PROG_WINDOWEDVDM:      // DOS window
                {
                    PSZ pszExtension = doshGetExtension(ProgDetails.pszExecutable);
                    if (pszExtension)
                    {
                        if (stricmp(pszExtension, "BAT") == 0)
                        {
                            CallBatchCorrectly(&ProgDetails,
                                               &strParamsPatched,
                                               NULL,
                                               "COMMAND.COM");
                        }
                    }
                break; }
            } // end switch (ProgDetails.progt.progc)

        if (    (ulIsWinApp)
             && (    (ProgDetails.pszEnvironment == NULL)
                  || (!strlen(ProgDetails.pszEnvironment))
                )
           )
        {
            // this is a windoze app, and caller didn't bother
            // to give us an environment:
            // we MUST set one then, or we'll get the strangest
            // errors, up to system hangs. V0.9.12 (2001-05-26) [umoeller]

            DOSENVIRONMENT Env = {0};

            // get standard WIN-OS/2 environment
            PSZ pszTemp = appQueryDefaultWin31Environment();

            /* if (!appParseEnvironment(pszTemp,
                                     &Env)) */
            {
                // now override KBD_CTRL_BYPASS=CTRL_ESC
                appSetEnvironmentVar(&Env,
                                     "KBD_CTRL_BYPASS=CTRL_ESC",
                                     FALSE);        // add last
                appSetEnvironmentVar(&Env,
                                     "KBD_ALTHOME_BYPASS=1",
                                     FALSE);        // add last
                appSetEnvironmentVar(&Env,
                                     "INT_DURING_IO=1",
                                     FALSE);        // add last
                appSetEnvironmentVar(&Env,
                                     "HW_TIMER=1",
                                     FALSE);        // add last
                appSetEnvironmentVar(&Env,
                                     "MOUSE_EXCLUSIVE_ACCESS=0",
                                     FALSE);        // add last
                appSetEnvironmentVar(&Env,
                                     "VIDEO_RETRACE_EMULATION=1",
                                     FALSE);        // add last
                appSetEnvironmentVar(&Env,
                                     "VIDEO_SWITCH_NOTIFICATION=1",
                                     FALSE);        // add last
                appSetEnvironmentVar(&Env,
                                     "VIDEO_8514A_XGA_IOTRAP=0",
                                     FALSE);        // add last

                if (!appConvertEnvironment(&Env,
                                           &pszWinOS2Env,   // freed at bottom
                                           NULL))
                    ProgDetails.pszEnvironment = pszWinOS2Env;

                appFreeEnvironment(&Env);
            }

            free(pszTemp);
        }

        if (!ProgDetails.pszTitle)
            ProgDetails.pszTitle = ProgDetails.pszExecutable;

        ProgDetails.pszParameters = strParamsPatched.psz;

        _Pmpf(("progt.progc: %d", ProgDetails.progt.progc));
        _Pmpf(("progt.fbVisible: 0x%lX", ProgDetails.progt.fbVisible));
        _Pmpf(("progt.pszTitle: \"%s\"", (ProgDetails.pszTitle) ? ProgDetails.pszTitle : "NULL"));
        _Pmpf(("exec: \"%s\"", (ProgDetails.pszExecutable) ? ProgDetails.pszExecutable : "NULL"));
        _Pmpf(("params: \"%s\"", (ProgDetails.pszParameters) ? ProgDetails.pszParameters : "NULL"));
        _Pmpf(("startup: \"%s\"", (ProgDetails.pszStartupDir) ? ProgDetails.pszStartupDir : "NULL"));
        _Pmpf(("pszIcon: \"%s\"", (ProgDetails.pszIcon) ? ProgDetails.pszIcon : "NULL"));
        _Pmpf(("environment: "));
        {
            PSZ pszThis = ProgDetails.pszEnvironment;
            while (pszThis && *pszThis)
            {
                _Pmpf(("  \"%s\"", pszThis));
                pszThis += strlen(pszThis) + 1;
            }
        }

        _Pmpf(("swpInitial.fl = 0x%lX, x = %d, y = %d, cx = %d, cy = %d:",
                    ProgDetails.swpInitial.fl,
                    ProgDetails.swpInitial.x,
                    ProgDetails.swpInitial.y,
                    ProgDetails.swpInitial.cx,
                    ProgDetails.swpInitial.cy));
        _Pmpf(("  behind = %d, hwnd = %d, res1 = %d, res2 = %d",
                    ProgDetails.swpInitial.hwndInsertBehind,
                    ProgDetails.swpInitial.hwnd,
                    ProgDetails.swpInitial.ulReserved1,
                    ProgDetails.swpInitial.ulReserved2));

        /* if (WinMessageBox(HWND_DESKTOP,
                          NULLHANDLE,
                          (ProgDetails.pszExecutable) ? ProgDetails.pszExecutable : "NULL",
                          "Start?",
                          0,
                          MB_YESNO | MB_MOVEABLE)
                  == MBID_YES) */
        happ = WinStartApp(hwndNotify,
                                    // receives WM_APPTERMINATENOTIFY
                           &ProgDetails,
                           strParamsPatched.psz,
                           NULL,            // "reserved", PMREF says...
                           SAF_INSTALLEDCMDLINE);
                                // we MUST use SAF_INSTALLEDCMDLINE
                                // or no Win-OS/2 session will start...
                                // whatever is going on here... Warp 4 FP11

                                // do not use SAF_STARTCHILDAPP, or the
                                // app will be terminated automatically
                                // when the WPS terminates!

        // _Pmpf((__FUNCTION__ ": got happ 0x%lX", happ));

        xstrClear(&strParamsPatched);
        if (pszWinOS2Env)
            free(pszWinOS2Env);
    } // end if (ProgDetails.pszExecutable)

    return (happ);
}


