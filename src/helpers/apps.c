
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
#define INCL_DOSEXCEPTIONS
#define INCL_DOSMODULEMGR
#define INCL_DOSSESMGR
#define INCL_DOSERRORS

#define INCL_WINPROGRAMLIST     // needed for PROGDETAILS, wppgm.h
#define INCL_WINERRORS
#define INCL_SHLERRORS
#include <os2.h>

#include <stdio.h>
#include <setjmp.h>             // needed for except.h
#include <assert.h>             // needed for except.h

#include "setup.h"                      // code generation and debugging options

#include "helpers\dosh.h"
#include "helpers\except.h"             // exception handling
#include "helpers\prfh.h"
#include "helpers\standards.h"          // some standard macros
#include "helpers\stringh.h"
#include "helpers\winh.h"
#include "helpers\xstring.h"

#include "helpers\apps.h"

/*
 *@@category: Helpers\PM helpers\Application helpers
 */

/* ******************************************************************
 *
 *   Environment helpers
 *
 ********************************************************************/

/*
 *@@ appQueryEnvironmentLen:
 *      returns the total length of the passed in environment
 *      string buffer, including the terminating two null bytes.
 *
 *@@added V0.9.16 (2002-01-09) [umoeller]
 */

ULONG appQueryEnvironmentLen(PCSZ pcszEnvironment)
{
    ULONG   cbEnvironment = 0;
    if (pcszEnvironment)
    {
        PCSZ    pVarThis = pcszEnvironment;
        // go thru the environment strings; last one has two null bytes
        while (*pVarThis)
        {
            ULONG ulLenThis = strlen(pVarThis) + 1;
            cbEnvironment += ulLenThis;
            pVarThis += ulLenThis;
        }

        cbEnvironment++;        // last null byte
    }

    return cbEnvironment;
}

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
 *@@changed V0.9.12 (2001-05-27) [umoeller]: moved from dosh2.c to apps.c
 */

APIRET appParseEnvironment(const char *pcszEnv,
                           PDOSENVIRONMENT pEnv)        // out: new environment
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

        pEnv->cVars = 0;
        pEnv->papszVars = 0;

        if (cVars)
        {
            ULONG cbArray = sizeof(PSZ) * cVars;
            PSZ *papsz;
            if (!(papsz = (PSZ*)malloc(cbArray)))
                arc = ERROR_NOT_ENOUGH_MEMORY;
            else
            {
                PSZ *ppszTarget = papsz;
                memset(papsz, 0, cbArray);
                pszVarThis = (PSZ)pcszEnv;
                while (*pszVarThis)
                {
                    ULONG ulThisLen;
                    if (!(*ppszTarget = strhdup(pszVarThis, &ulThisLen)))
                    {
                        arc = ERROR_NOT_ENOUGH_MEMORY;
                        break;
                    }
                    (pEnv->cVars)++;
                    ppszTarget++;
                    pszVarThis += ulThisLen + 1;
                }

                pEnv->papszVars = papsz;
            }
        }
    }

    return arc;
}

/*
 *@@ appGetEnvironment:
 *      calls appParseEnvironment for the current
 *      process environment, which is retrieved from
 *      the info blocks.
 *
 *      Returns:
 *
 *      --  NO_ERROR:
 *
 *      --  ERROR_INVALID_PARAMETER
 *
 *      --  ERROR_BAD_ENVIRONMENT: no environment found in
 *          info blocks.
 *
 *@@added V0.9.4 (2000-07-19) [umoeller]
 *@@changed V0.9.12 (2001-05-27) [umoeller]: moved from dosh2.c to apps.c
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
            PSZ pszEnv;
            if (pszEnv = ppib->pib_pchenv)
                arc = appParseEnvironment(pszEnv, pEnv);
            else
                arc = ERROR_BAD_ENVIRONMENT;
        }
    }

    return arc;
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
 *@@changed V0.9.12 (2001-05-27) [umoeller]: moved from dosh2.c to apps.c
 *@@changed V0.9.16 (2002-01-01) [umoeller]: removed extra heap allocation
 */

PSZ* appFindEnvironmentVar(PDOSENVIRONMENT pEnv,
                           PSZ pszVarName)
{
    PSZ     *ppszRet = 0;

    if (    (pEnv)
         && (pEnv->papszVars)
         && (pszVarName)
       )
    {
        ULONG   ul = 0;
        ULONG   ulVarNameLen = 0;

        PSZ     pFirstEqual;
        // rewrote all the following for speed V0.9.16 (2002-01-01) [umoeller]
        if (pFirstEqual = strchr(pszVarName, '='))
            // VAR=VALUE
            //    ^ pFirstEqual
            ulVarNameLen = pFirstEqual - pszVarName;
        else
            ulVarNameLen = strlen(pszVarName);

        for (ul = 0;
             ul < pEnv->cVars;
             ul++)
        {
            PSZ pszThis = pEnv->papszVars[ul];
            if (pFirstEqual = strchr(pszThis, '='))
            {
                ULONG ulLenThis = pFirstEqual - pszThis;
                if (    (ulLenThis == ulVarNameLen)
                     && (!memicmp(pszThis,
                                  pszVarName,
                                  ulVarNameLen))
                   )
                {
                    ppszRet = &pEnv->papszVars[ul];
                    break;
                }
            }
        }
    }

    return ppszRet;
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
 *@@changed V0.9.12 (2001-05-27) [umoeller]: moved from dosh2.c to apps.c
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
            PSZ *ppszEnvLine;
            if (ppszEnvLine = appFindEnvironmentVar(pEnv, pszNewEnv))
                // was set already: replace
                arc = strhStore(ppszEnvLine,
                                pszNewEnv,
                                NULL);
            else
            {
                // not set already:
                PSZ *ppszNew = NULL;

                // allocate new array, with one new entry
                // fixed V0.9.12 (2001-05-26) [umoeller], this crashed
                PSZ *papszNew;

                if (!(papszNew = (PSZ*)malloc(sizeof(PSZ) * (pEnv->cVars + 1))))
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

                    free(pEnv->papszVars);      // was missing V0.9.12 (2001-05-21) [umoeller]
                    pEnv->papszVars = papszNew;
                    pEnv->cVars++;
                    *ppszNew = strdup(pszNewEnv);
                }
            }
        }
    }

    return arc;
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
 *@@changed V0.9.12 (2001-05-27) [umoeller]: moved from dosh2.c to apps.c
 */

APIRET appConvertEnvironment(PDOSENVIRONMENT pEnv,
                             PSZ *ppszEnv,     // out: environment string
                             PULONG pulSize)  // out: size of block allocated in *ppszEnv; ptr can be NULL
{
    APIRET  arc = NO_ERROR;
    if (    (!pEnv)
         || (!pEnv->papszVars)
       )
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

        if (!(*ppszEnv = (PSZ)malloc(cbNeeded)))
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

    return arc;
}

/*
 *@@ appFreeEnvironment:
 *      frees memory allocated by appGetEnvironment.
 *
 *@@added V0.9.4 (2000-07-19) [umoeller]
 *@@changed V0.9.12 (2001-05-27) [umoeller]: moved from dosh2.c to apps.c
 */

APIRET appFreeEnvironment(PDOSENVIRONMENT pEnv)
{
    APIRET  arc = NO_ERROR;
    if (    (!pEnv)
         || (!pEnv->papszVars)
       )
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

    return arc;
}

/* ******************************************************************
 *
 *   Application information
 *
 ********************************************************************/

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
 *      --  PROG_DLL
 *
 *      --  PROG_WINDOWEDVDM
 *
 *      --  PROG_PM
 *
 *      --  PROG_31_ENHSEAMLESSCOMMON
 *
 *      --  PROG_WINDOWABLEVIO
 *
 *      --  PROG_DEFAULT
 *
 *@@added V0.9.9 (2001-03-07) [umoeller]
 *@@changed V0.9.12 (2001-05-27) [umoeller]: moved from winh.c to apps.c
 *@@changed V0.9.14 (2001-08-07) [pr]: use FAPPTYP_* constants
 *@@changed V0.9.16 (2001-12-08) [umoeller]: added checks for batch files, other optimizations
 */

APIRET appQueryAppType(const char *pcszExecutable,
                       PULONG pulDosAppType,            // out: DOS app type
                       PULONG pulWinAppType)            // out: PROG_* app type
{
    APIRET arc;

/*
   #define FAPPTYP_NOTSPEC         0x0000
   #define FAPPTYP_NOTWINDOWCOMPAT 0x0001
   #define FAPPTYP_WINDOWCOMPAT    0x0002
   #define FAPPTYP_WINDOWAPI       0x0003
   #define FAPPTYP_BOUND           0x0008
   #define FAPPTYP_DLL             0x0010
   #define FAPPTYP_DOS             0x0020
   #define FAPPTYP_PHYSDRV         0x0040  // physical device driver
   #define FAPPTYP_VIRTDRV         0x0080  // virtual device driver
   #define FAPPTYP_PROTDLL         0x0100  // 'protected memory' dll
   #define FAPPTYP_WINDOWSREAL     0x0200  // Windows real mode app
   #define FAPPTYP_WINDOWSPROT     0x0400  // Windows protect mode app
   #define FAPPTYP_WINDOWSPROT31   0x1000  // Windows 3.1 protect mode app
   #define FAPPTYP_32BIT           0x4000
*/

    ULONG   ulWinAppType = PROG_DEFAULT;

    if (!(arc = DosQueryAppType((PSZ)pcszExecutable, pulDosAppType)))
    {
        // clear the 32-bit flag
        // V0.9.16 (2001-12-08) [umoeller]
        ULONG ulDosAppType = (*pulDosAppType) & ~FAPPTYP_32BIT,
              ulLoAppType = ulDosAppType & 0xFFFF;

        if (ulDosAppType & FAPPTYP_PHYSDRV)            // 0x40
            ulWinAppType = PROG_PDD;
        else if (ulDosAppType & FAPPTYP_VIRTDRV)       // 0x80
            ulWinAppType = PROG_VDD;
        else if ((ulDosAppType & 0xF0) == FAPPTYP_DLL) // 0x10
            // DLL bit set
            ulWinAppType = PROG_DLL;
        else if (ulDosAppType & FAPPTYP_DOS)           // 0x20
            // DOS bit set?
            ulWinAppType = PROG_WINDOWEDVDM;
        else if ((ulDosAppType & FAPPTYP_WINDOWAPI) == FAPPTYP_WINDOWAPI) // 0x0003)
            // "Window-API" == PM
            ulWinAppType = PROG_PM;
        else if (ulLoAppType == FAPPTYP_WINDOWSREAL)
            ulWinAppType = PROG_31_ENHSEAMLESSCOMMON;  // @@todo really?
        else if (   (ulLoAppType == FAPPTYP_WINDOWSPROT31) // 0x1000) // windows program (?!?)
                 || (ulLoAppType == FAPPTYP_WINDOWSPROT) // ) // windows program (?!?)
                )
            ulWinAppType = PROG_31_ENHSEAMLESSCOMMON;  // PROG_31_ENH;
        else if ((ulDosAppType & FAPPTYP_WINDOWAPI /* 0x03 */ ) == FAPPTYP_WINDOWCOMPAT) // 0x02)
            ulWinAppType = PROG_WINDOWABLEVIO;
        else if ((ulDosAppType & FAPPTYP_WINDOWAPI /* 0x03 */ ) == FAPPTYP_NOTWINDOWCOMPAT) // 0x01)
            ulWinAppType = PROG_FULLSCREEN;
    }

    if (ulWinAppType == PROG_DEFAULT)
    {
        // added checks for batch files V0.9.16 (2001-12-08) [umoeller]
        PCSZ pcszExt;
        if (pcszExt = doshGetExtension(pcszExecutable))
        {
            if (!stricmp(pcszExt, "BAT"))
            {
                ulWinAppType = PROG_WINDOWEDVDM;
                arc = NO_ERROR;
            }
            else if (!stricmp(pcszExt, "CMD"))
            {
                ulWinAppType = PROG_WINDOWABLEVIO;
                arc = NO_ERROR;
            }
        }
    }

    *pulWinAppType = ulWinAppType;

    return arc;
}

/*
 *@@ PROGTYPESTRING:
 *
 *@@added V0.9.16 (2002-01-13) [umoeller]
 */

typedef struct _PROGTYPESTRING
{
    PROGCATEGORY    progc;
    PCSZ            pcsz;
} PROGTYPESTRING, *PPROGTYPESTRING;

PROGTYPESTRING G_aProgTypes[] =
    {
        PROG_DEFAULT, "PROG_DEFAULT",
        PROG_FULLSCREEN, "PROG_FULLSCREEN",
        PROG_WINDOWABLEVIO, "PROG_WINDOWABLEVIO",
        PROG_PM, "PROG_PM",
        PROG_GROUP, "PROG_GROUP",
        PROG_VDM, "PROG_VDM",
            // same as PROG_REAL, "PROG_REAL",
        PROG_WINDOWEDVDM, "PROG_WINDOWEDVDM",
        PROG_DLL, "PROG_DLL",
        PROG_PDD, "PROG_PDD",
        PROG_VDD, "PROG_VDD",
        PROG_WINDOW_REAL, "PROG_WINDOW_REAL",
        PROG_30_STD, "PROG_30_STD",
            // same as PROG_WINDOW_PROT, "PROG_WINDOW_PROT",
        PROG_WINDOW_AUTO, "PROG_WINDOW_AUTO",
        PROG_30_STDSEAMLESSVDM, "PROG_30_STDSEAMLESSVDM",
            // same as PROG_SEAMLESSVDM, "PROG_SEAMLESSVDM",
        PROG_30_STDSEAMLESSCOMMON, "PROG_30_STDSEAMLESSCOMMON",
            // same as PROG_SEAMLESSCOMMON, "PROG_SEAMLESSCOMMON",
        PROG_31_STDSEAMLESSVDM, "PROG_31_STDSEAMLESSVDM",
        PROG_31_STDSEAMLESSCOMMON, "PROG_31_STDSEAMLESSCOMMON",
        PROG_31_ENHSEAMLESSVDM, "PROG_31_ENHSEAMLESSVDM",
        PROG_31_ENHSEAMLESSCOMMON, "PROG_31_ENHSEAMLESSCOMMON",
        PROG_31_ENH, "PROG_31_ENH",
        PROG_31_STD, "PROG_31_STD",

// Warp 4 toolkit defines, whatever these were designed for...
#ifndef PROG_DOS_GAME
    #define PROG_DOS_GAME            (PROGCATEGORY)21
#endif
#ifndef PROG_WIN_GAME
    #define PROG_WIN_GAME            (PROGCATEGORY)22
#endif
#ifndef PROG_DOS_MODE
    #define PROG_DOS_MODE            (PROGCATEGORY)23
#endif

        PROG_DOS_GAME, "PROG_DOS_GAME",
        PROG_WIN_GAME, "PROG_WIN_GAME",
        PROG_DOS_MODE, "PROG_DOS_MODE",

        // added this V0.9.16 (2001-12-08) [umoeller]
        PROG_WIN32, "PROG_WIN32"
    };

/*
 *@@ appDescribeAppType:
 *      returns a "PROG_*" string for the given
 *      program type. Useful for WPProgram setup
 *      strings and such.
 *
 *@@added V0.9.16 (2001-10-06)
 */

PCSZ appDescribeAppType(PROGCATEGORY progc)        // in: from PROGDETAILS.progc
{
    ULONG ul;
    for (ul = 0;
         ul < ARRAYITEMCOUNT(G_aProgTypes);
         ul++)
    {
        if (G_aProgTypes[ul].progc == progc)
            return G_aProgTypes[ul].pcsz;
    }

    return NULL;
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
            return 2;

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
            return 1;
    }

    return 0;
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
 *@@changed V0.9.12 (2001-05-27) [umoeller]: moved from winh.c to apps.c
 */

static VOID CallBatchCorrectly(PPROGDETAILS pProgDetails,
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
 *@@changed V0.9.19 (2002-03-28) [umoeller]: now returning APIRET
 */

APIRET appQueryDefaultWin31Environment(PSZ *ppsz)
{
    APIRET arc = NO_ERROR;
    PSZ pszReturn = NULL;
    ULONG ulSize = 0;

    // get default environment (from Win-OS/2 settings object) from OS2.INI
    PSZ pszDefEnv;
    if (pszDefEnv = prfhQueryProfileData(HINI_USER,
                                         "WINOS2",
                                         "PM_GlobalWindows31Settings",
                                         &ulSize))
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

            *ppsz = pszReturn;
        }
        else
            arc = ERROR_NOT_ENOUGH_MEMORY;

        free(pszDefEnv);
    }
    else
        arc = ERROR_BAD_ENVIRONMENT;

    return arc;
}

#ifdef _PMPRINTF_

static void DumpMemoryBlock(PBYTE pb,       // in: start address
                     ULONG ulSize,   // in: size of block
                     ULONG ulIndent) // in: how many spaces to put
                                     //     before each output line
{
    TRY_QUIET(excpt1)
    {
        PBYTE   pbCurrent = pb;                 // current byte
        ULONG   ulCount = 0,
                ulCharsInLine = 0;              // if this grows > 7, a new line is started
        CHAR    szTemp[1000];
        CHAR    szLine[400] = "",
                szAscii[30] = "         ";      // ASCII representation; filled for every line
        PSZ     pszLine = szLine,
                pszAscii = szAscii;

        for (pbCurrent = pb;
             ulCount < ulSize;
             pbCurrent++, ulCount++)
        {
            if (ulCharsInLine == 0)
            {
                memset(szLine, ' ', ulIndent);
                pszLine += ulIndent;
            }
            pszLine += sprintf(pszLine, "%02lX ", (ULONG)*pbCurrent);

            if ( (*pbCurrent > 31) && (*pbCurrent < 127) )
                // printable character:
                *pszAscii = *pbCurrent;
            else
                *pszAscii = '.';
            pszAscii++;

            ulCharsInLine++;
            if (    (ulCharsInLine > 7)         // 8 bytes added?
                 || (ulCount == ulSize-1)       // end of buffer reached?
               )
            {
                // if we haven't had eight bytes yet,
                // fill buffer up to eight bytes with spaces
                ULONG   ul2;
                for (ul2 = ulCharsInLine;
                     ul2 < 8;
                     ul2++)
                    pszLine += sprintf(pszLine, "   ");

                sprintf(szTemp, "%04lX:  %s  %ss",
                                (ulCount & 0xFFFFFFF8),  // offset in hex
                                szLine,         // bytes string
                                szAscii);       // ASCII string

                _Pmpf(("%s", szTemp));

                // restart line buffer
                pszLine = szLine;

                // clear ASCII buffer
                strcpy(szAscii, "         ");
                pszAscii = szAscii;

                // reset line counter
                ulCharsInLine = 0;
            }
        }

    }
    CATCH(excpt1)
    {
        _Pmpf(("Crash in " __FUNCTION__ ));
    } END_CATCH();
}

#endif

/*
 *@@ appBuildProgDetails:
 *      extracted code from appStartApp to fix the
 *      given PROGDETAILS data to support the typical
 *      WPS stuff and allocate a single block of
 *      shared memory containing all the data.
 *
 *      This is now used by XWP's progOpenProgram
 *      directly as a temporary fix for all the
 *      session hangs.
 *
 *      As input, this takes a PROGDETAILS structure,
 *      which is converted in various ways. In detail,
 *      this supports:
 *
 *      -- starting "*" executables (command prompts
 *         for OS/2, DOS, Win-OS/2);
 *
 *      -- starting ".CMD" and ".BAT" files as
 *         PROGDETAILS.pszExecutable; for those, we
 *         convert the executable and parameters to
 *         start CMD.EXE or COMMAND.COM with the "/C"
 *         parameter instead;
 *
 *      -- starting apps which are not fully qualified
 *         and therefore assumed to be on the PATH
 *         (for which doshSearchPath("PATH") is called).
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
 *          is NULL, the new app inherits the default environment
 *          from the shell.
 *
 *      --  However, if you specify an environment, you _must_
 *          specify a complete environment. This function
 *          will not merge environments. Use
 *          appSetEnvironmentVar to change environment
 *          variables in a complete environment set.
 *
 *      --  If PROGDETAILS specifies a Win-OS/2 session
 *          and PROGDETAILS.pszEnvironment is empty,
 *          this uses the default Win-OS/2 environment
 *          from OS2.INI. See appQueryDefaultWin31Environment.
 *
 *      Even though this isn't clearly said in PMREF,
 *      PROGDETAILS.swpInitial is important:
 *
 *      -- To start a session minimized, set fl to SWP_MINIMIZE.
 *
 *      -- To start a VIO session with auto-close disabled,
 *         set the half-documented SWP_NOAUTOCLOSE flag (0x8000)
 *         This flag is now in the newer toolkit headers.
 *
 *      In addition, this supports the following session
 *      flags with ulFlags if PROG_DEFAULT is specified:
 *
 *      --  APP_RUN_FULLSCREEN: start a fullscreen session
 *          for VIO, DOS, and Win-OS/2 programs. Otherwise
 *          we start a windowed or (share) a seamless session.
 *          Ignored if the program is PM.
 *
 *      --  APP_RUN_ENHANCED: for Win-OS/2 sessions, use
 *          enhanced mode.
 *          Ignored if the program is not Win-OS/2.
 *
 *      --  APP_RUN_STANDARD: for Win-OS/2 sessions, use
 *          standard mode.
 *          Ignored if the program is not Win-OS/2.
 *
 *      --  APP_RUN_SEPARATE: for Win-OS/2 sessions, use
 *          a separate session.
 *          Ignored if the program is not Win-OS/2.
 *
 *      If NO_ERROR is returned, *ppDetails receives a
 *      new buffer of shared memory containing all the
 *      data packed together.
 *
 *      The shared memory is allocated unnamed and
 *      with OBJ_GETTABLE. It is the responsibility
 *      of the caller to call DosFreeMem on that buffer.
 *
 *      Returns:
 *
 *      --  NO_ERROR
 *
 *      --  ERROR_INVALID_PARAMETER: pcProgDetails or
 *          ppDetails is NULL; or PROGDETAILS.pszExecutable is NULL.
 *
 *      --  ERROR_FILE_NOT_FOUND, ERROR_PATH_NOT_FOUND:
 *          PROGDETAILS.pszExecutable and/or PROGDETAILS.pszStartupDir
 *          are invalid.
 *          A NULL PROGDETAILS.pszStartupDir is supported though.
 *
 *      --  ERROR_BAD_FORMAT
 *
 *      --  ERROR_BAD_ENVIRONMENT: environment is larger than 60.000 bytes.
 *
 *      --  ERROR_NOT_ENOUGH_MEMORY
 *
 *      plus the error codes from doshQueryPathAttr, doshSearchPath,
 *      appParseEnvironment, appSetEnvironmentVar, and appConvertEnvironment.
 *
 *@@added V0.9.18 (2002-03-27) [umoeller]
 *@@changed V0.9.19 (2002-03-28) [umoeller]: now allocating contiguous buffer
 */

APIRET appBuildProgDetails(PPROGDETAILS *ppDetails,           // out: shared mem with fixed program spec (req.)
                           const PROGDETAILS *pcProgDetails,  // in: program spec (req.)
                           ULONG ulFlags)                     // in: APP_RUN_* flags or 0
{
    APIRET          arc = NO_ERROR;

    XSTRING         strExecutablePatched,
                    strParamsPatched;
    PSZ             pszWinOS2Env = 0;

    PROGDETAILS     Details;

    *ppDetails = NULL;

    if (!pcProgDetails && !ppDetails)
        return (ERROR_INVALID_PARAMETER);

    /*
     * part 1:
     *      fix up the PROGDETAILS fields
     */

    xstrInit(&strExecutablePatched, 0);
    xstrInit(&strParamsPatched, 0);

    memcpy(&Details, pcProgDetails, sizeof(PROGDETAILS));
            // pointers still point into old prog details buffer
    Details.Length = sizeof(PROGDETAILS);
    Details.progt.fbVisible = SHE_VISIBLE;

    // all this only makes sense if this contains something...
    // besides, this crashed on string comparisons V0.9.9 (2001-01-27) [umoeller]
    if (    (!Details.pszExecutable)
         || (!Details.pszExecutable[0])
       )
        arc = ERROR_INVALID_PARAMETER;
    else
    {
        ULONG           ulIsWinApp;

        // memset(&Details.swpInitial, 0, sizeof(SWP));
        // this wasn't a good idea... WPProgram stores stuff
        // in here, such as the "minimize on startup" -> SWP_MINIMIZE

        // duplicate parameters...
        // we need this for string manipulations below...
        if (    (Details.pszParameters)
             && (Details.pszParameters[0])    // V0.9.18
           )
            xstrcpy(&strParamsPatched,
                    Details.pszParameters,
                    0);

        #ifdef DEBUG_PROGRAMSTART
            _Pmpf((__FUNCTION__ ": old progc: 0x%lX", pcProgDetails->progt.progc));
            _Pmpf(("  pszTitle: %s", (Details.pszTitle) ? Details.pszTitle : NULL));
            _Pmpf(("  pszIcon: %s", (Details.pszIcon) ? Details.pszIcon : NULL));
        #endif

        // program type fixups
        switch (Details.progt.progc)        // that's a ULONG
        {
            case ((ULONG)-1):       // we get that sometimes...
            case PROG_DEFAULT:
            {
                // V0.9.12 (2001-05-26) [umoeller]
                ULONG ulDosAppType;
                appQueryAppType(Details.pszExecutable,
                                &ulDosAppType,
                                &Details.progt.progc);
            }
            break;
        }

        // set session type from option flags
        if (ulFlags & APP_RUN_FULLSCREEN)
        {
            if (Details.progt.progc == PROG_WINDOWABLEVIO)
                Details.progt.progc = PROG_FULLSCREEN;
            else if (Details.progt.progc == PROG_WINDOWEDVDM)
                Details.progt.progc = PROG_VDM;
        }

        if (ulIsWinApp = appIsWindowsApp(Details.progt.progc))
        {
            if (ulFlags & APP_RUN_FULLSCREEN)
                Details.progt.progc = (ulFlags & APP_RUN_ENHANCED)
                                                ? PROG_31_ENH
                                                : PROG_31_STD;
            else
            {
                if (ulFlags & APP_RUN_STANDARD)
                    Details.progt.progc = (ulFlags & APP_RUN_SEPARATE)
                                                ? PROG_31_STDSEAMLESSVDM
                                                : PROG_31_STDSEAMLESSCOMMON;
                else if (ulFlags & APP_RUN_ENHANCED)
                    Details.progt.progc = (ulFlags & APP_RUN_SEPARATE)
                                                ? PROG_31_ENHSEAMLESSVDM
                                                : PROG_31_ENHSEAMLESSCOMMON;
            }

            // re-run V0.9.16 (2001-10-19) [umoeller]
            ulIsWinApp = appIsWindowsApp(Details.progt.progc);
        }

        if (!arc)
        {
            /*
             * command lines fixups:
             *
             */

            if (!strcmp(Details.pszExecutable, "*"))
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
                    Details.pszExecutable = "WINOS2.COM";
                    // this is a DOS app, so fix this to DOS fullscreen
                    Details.progt.progc = PROG_VDM;
                }
                else
                    // for all other executable types
                    // (including OS/2 and DOS sessions),
                    // set pszExecutable to NULL; this will
                    // have WinStartApp start a cmd shell
                    Details.pszExecutable = NULL;

            } // end if (strcmp(pProgDetails->pszExecutable, "*") == 0)
            else
            {
                // check if the executable is fully qualified; if so,
                // check if the executable file exists
                if (    (Details.pszExecutable[1] == ':')
                     && (strchr(Details.pszExecutable, '\\'))
                   )
                {
                    ULONG ulAttr;
                    if (!(arc = doshQueryPathAttr(Details.pszExecutable,
                                                  &ulAttr)))
                    {
                        // make sure startup dir is really a directory
                        if (Details.pszStartupDir)
                        {
                            // it is valid to specify a startup dir of "C:"
                            if (    (strlen(Details.pszStartupDir) > 2)
                                 && (!(arc = doshQueryPathAttr(Details.pszStartupDir,
                                                               &ulAttr)))
                                 && (!(ulAttr & FILE_DIRECTORY))
                               )
                                arc = ERROR_PATH_NOT_FOUND;
                        }
                    }
                }
                else
                {
                    // _not_ fully qualified: look it up on the PATH then
                    // V0.9.16 (2001-12-06) [umoeller]
                    CHAR    szFQExecutable[CCHMAXPATH];
                    if (!(arc = doshSearchPath("PATH",
                                               Details.pszExecutable,
                                               szFQExecutable,
                                               sizeof(szFQExecutable))))
                    {
                        // alright, found it:
                        xstrcpy(&strExecutablePatched, szFQExecutable, 0);
                        Details.pszExecutable = strExecutablePatched.psz;
                    }
                }

                if (!arc)
                {
                    PSZ pszExtension;
                    switch (Details.progt.progc)
                    {
                        /*
                         *  .CMD files fixups
                         *
                         */

                        case PROG_FULLSCREEN:       // OS/2 fullscreen
                        case PROG_WINDOWABLEVIO:    // OS/2 window
                        {
                            if (    (pszExtension = doshGetExtension(Details.pszExecutable))
                                 && (!stricmp(pszExtension, "CMD"))
                               )
                            {
                                CallBatchCorrectly(&Details,
                                                   &strParamsPatched,
                                                   "OS2_SHELL",
                                                   "CMD.EXE");
                            }
                        }
                        break;

                        case PROG_VDM:              // DOS fullscreen
                        case PROG_WINDOWEDVDM:      // DOS window
                        {
                            if (    (pszExtension = doshGetExtension(Details.pszExecutable))
                                 && (!stricmp(pszExtension, "BAT"))
                               )
                            {
                                CallBatchCorrectly(&Details,
                                                   &strParamsPatched,
                                                   NULL,
                                                   "COMMAND.COM");
                            }
                        }
                        break;
                    } // end switch (Details.progt.progc)
                }
            }
        }

        if (!arc)
        {
            if (    (ulIsWinApp)
                 && (    (Details.pszEnvironment == NULL)
                      || (!strlen(Details.pszEnvironment))
                    )
               )
            {
                // this is a windoze app, and caller didn't bother
                // to give us an environment:
                // we MUST set one then, or we'll get the strangest
                // errors, up to system hangs. V0.9.12 (2001-05-26) [umoeller]

                DOSENVIRONMENT Env = {0};

                // get standard WIN-OS/2 environment
                PSZ pszTemp;
                if (!(arc = appQueryDefaultWin31Environment(&pszTemp)))
                {
                    if (!(arc = appParseEnvironment(pszTemp,
                                                    &Env)))
                    {
                        // now override KBD_CTRL_BYPASS=CTRL_ESC
                        if (    (!(arc = appSetEnvironmentVar(&Env,
                                                              "KBD_CTRL_BYPASS=CTRL_ESC",
                                                              FALSE)))        // add last
                             && (!(arc = appConvertEnvironment(&Env,
                                                               &pszWinOS2Env,   // freed at bottom
                                                               NULL)))
                           )
                            Details.pszEnvironment = pszWinOS2Env;

                        appFreeEnvironment(&Env);
                    }

                    free(pszTemp);
                }
            }

            if (!arc)
            {
                // if no title is given, use the executable
                if (!Details.pszTitle)
                    Details.pszTitle = Details.pszExecutable;

                // make sure params have a leading space
                // V0.9.18 (2002-03-27) [umoeller]
                if (strParamsPatched.ulLength)
                {
                    if (strParamsPatched.psz[0] != ' ')
                    {
                        XSTRING str2;
                        xstrInit(&str2, 0);
                        xstrcpy(&str2, " ", 1);
                        xstrcats(&str2, &strParamsPatched);
                        xstrcpys(&strParamsPatched, &str2);
                        xstrClear(&str2);
                                // we really need xstrInsert or something
                    }
                    Details.pszParameters = strParamsPatched.psz;
                }
                else
                    // never pass null pointers
                    Details.pszParameters = "";

                // never pass null pointers
                if (!Details.pszIcon)
                    Details.pszIcon = "";

                // never pass null pointers
                if (!Details.pszStartupDir)
                    Details.pszStartupDir = "";

            }
        }
    }

    /*
     * part 2:
     *      pack the fixed PROGDETAILS fields
     */

    if (!arc)
    {
        ULONG   cb,
                cbTitle,
                cbExecutable,
                cbParameters,
                cbStartupDir,
                cbIcon,
                cbEnvironment;

        // allocate a chunk of tiled memory from OS/2 to make sure
        // this is aligned on a 64K memory (backed up by a 16-bit
        // LDT selector); if it is not, and the environment
        // crosses segments, it gets truncated!!
        cb = sizeof(PROGDETAILS);
        if (cbTitle = strhSize(Details.pszTitle))
            cb += cbTitle;

        if (cbExecutable = strhSize(Details.pszExecutable))
            cb += cbExecutable;

        if (cbParameters = strhSize(Details.pszParameters))
            cb += cbParameters;

        if (cbStartupDir = strhSize(Details.pszStartupDir))
            cb += cbStartupDir;

        if (cbIcon = strhSize(Details.pszIcon))
            cb += cbIcon;

        if (cbEnvironment = appQueryEnvironmentLen(Details.pszEnvironment))
            cb += cbEnvironment;

        if (cb > 60000)     // to be on the safe side
            arc = ERROR_BAD_ENVIRONMENT; // 10;
        else
        {
            PPROGDETAILS pNewProgDetails;
            // alright, allocate the shared memory now
            if (!(arc = DosAllocSharedMem((PVOID*)&pNewProgDetails,
                                          NULL,
                                          cb,
                                          PAG_COMMIT | OBJ_GETTABLE | OBJ_TILE | PAG_EXECUTE | PAG_READ | PAG_WRITE)))
            {
                // and copy stuff
                PBYTE pThis;

                memset(pNewProgDetails, 0, cb);

                pNewProgDetails->Length = sizeof(PROGDETAILS);

                pNewProgDetails->progt.progc = Details.progt.progc;

                pNewProgDetails->progt.fbVisible = Details.progt.fbVisible;
                memcpy(&pNewProgDetails->swpInitial, &Details.swpInitial, sizeof(SWP));

                // start copying into buffer right after PROGDETAILS
                pThis = (PBYTE)(pNewProgDetails + 1);

                // handy macro to avoid typos
                #define COPY(id) if (cb ## id) { \
                    memcpy(pThis, Details.psz ## id, cb ## id); \
                    pNewProgDetails->psz ## id = pThis; \
                    pThis += cb ## id; }

                COPY(Title);
                COPY(Executable);
                COPY(Parameters);
                COPY(StartupDir);
                COPY(Icon);
                COPY(Environment);

                *ppDetails = pNewProgDetails;
            }
        }
    }

    xstrClear(&strParamsPatched);
    xstrClear(&strExecutablePatched);

    if (pszWinOS2Env)
        free(pszWinOS2Env);

    return arc;
}

/*
 *@@ CallDosStartSession:
 *
 *@@added V0.9.18 (2002-03-27) [umoeller]
 */

static APIRET CallDosStartSession(HAPP *phapp,
                                  const PROGDETAILS *pNewProgDetails, // in: program spec (req.)
                                  ULONG cbFailingName,
                                  PSZ pszFailingName)
{
    APIRET      arc = NO_ERROR;

    BOOL        fCrit = FALSE,
                fResetDir = FALSE;
    CHAR        szCurrentDir[CCHMAXPATH];

    ULONG       sid,
                pid;
    STARTDATA   SData;
    SData.Length  = sizeof(STARTDATA);
    SData.Related = SSF_RELATED_INDEPENDENT; // SSF_RELATED_CHILD;
    // per default, try to start this in the foreground
    SData.FgBg    = SSF_FGBG_FORE;
    SData.TraceOpt = SSF_TRACEOPT_NONE;

    SData.PgmTitle = pNewProgDetails->pszTitle;
    SData.PgmName = pNewProgDetails->pszExecutable;
    SData.PgmInputs = pNewProgDetails->pszParameters;

    SData.TermQ = NULL;
    SData.Environment = pNewProgDetails->pszEnvironment;
    SData.InheritOpt = SSF_INHERTOPT_PARENT;    // ignored

    switch (pNewProgDetails->progt.progc)
    {
        case PROG_FULLSCREEN:
            SData.SessionType = SSF_TYPE_FULLSCREEN;
        break;

        case PROG_WINDOWABLEVIO:
            SData.SessionType = SSF_TYPE_WINDOWABLEVIO;
        break;

        case PROG_PM:
            SData.SessionType = SSF_TYPE_PM;
            SData.FgBg = SSF_FGBG_BACK;     // otherwise we get ERROR_SMG_START_IN_BACKGROUND
        break;

        case PROG_VDM:
            SData.SessionType = SSF_TYPE_VDM;
        break;

        case PROG_WINDOWEDVDM:
            SData.SessionType = SSF_TYPE_WINDOWEDVDM;
        break;

        default:
            SData.SessionType = SSF_TYPE_DEFAULT;
    }

    SData.IconFile = 0;
    SData.PgmHandle = 0;

    SData.PgmControl = 0;

    if (pNewProgDetails->progt.fbVisible == SHE_VISIBLE)
        SData.PgmControl |= SSF_CONTROL_VISIBLE;

    if (pNewProgDetails->swpInitial.fl & SWP_HIDE)
        SData.PgmControl |= SSF_CONTROL_INVISIBLE;

    if (pNewProgDetails->swpInitial.fl & SWP_MAXIMIZE)
        SData.PgmControl |= SSF_CONTROL_MAXIMIZE;
    if (pNewProgDetails->swpInitial.fl & SWP_MINIMIZE)
    {
        SData.PgmControl |= SSF_CONTROL_MINIMIZE;
        // use background then
        SData.FgBg = SSF_FGBG_BACK;
    }
    if (pNewProgDetails->swpInitial.fl & SWP_MOVE)
        SData.PgmControl |= SSF_CONTROL_SETPOS;
    if (pNewProgDetails->swpInitial.fl & SWP_NOAUTOCLOSE)
        SData.PgmControl |= SSF_CONTROL_NOAUTOCLOSE;

    SData.InitXPos  = pNewProgDetails->swpInitial.x;
    SData.InitYPos  = pNewProgDetails->swpInitial.y;
    SData.InitXSize = pNewProgDetails->swpInitial.cx;
    SData.InitYSize = pNewProgDetails->swpInitial.cy;

    SData.Reserved = 0;
    SData.ObjectBuffer  = pszFailingName;
    SData.ObjectBuffLen = cbFailingName;

    // now, if a required module cannot be found,
    // DosStartSession still returns ERROR_FILE_NOT_FOUND
    // (2), but pszFailingName will be set to something
    // meaningful... so set it to a null string first
    // and we can then check if it has changed
    if (pszFailingName)
        *pszFailingName = '\0';

    TRY_QUIET(excpt1)
    {
        if (    (pNewProgDetails->pszStartupDir)
             && (pNewProgDetails->pszStartupDir[0])
           )
        {
            fCrit = !DosEnterCritSec();
            if (    (!(arc = doshQueryCurrentDir(szCurrentDir)))
                 && (!(arc = doshSetCurrentDir(pNewProgDetails->pszStartupDir)))
               )
                fResetDir = TRUE;
        }

        if (    (!arc)
             && (!(arc = DosStartSession(&SData, &sid, &pid)))
           )
        {
            // app started:
            // compose HAPP from that
            *phapp = sid;
        }
        else if (pszFailingName && *pszFailingName)
            // DosStartSession has set this to something
            // other than NULL: then use error code 1804,
            // as cmd.exe does
            arc = 1804;
    }
    CATCH(excpt1)
    {
        arc = ERROR_PROTECTION_VIOLATION;
    } END_CATCH();

    if (fResetDir)
        doshSetCurrentDir(szCurrentDir);

    if (fCrit)
        DosExitCritSec();

    #ifdef DEBUG_PROGRAMSTART
        _Pmpf(("   DosStartSession returned %d, pszFailingName: \"%s\"",
                  arc, pszFailingName));
    #endif

    return arc;
}

/*
 *@@ CallWinStartApp:
 *      wrapper around WinStartApp which copies all the
 *      parameters into a contiguous block of tiled memory.
 *
 *      This might fix some of the problems with truncated
 *      environments we were having because apparently the
 *      WinStartApp thunking to 16-bit doesn't always work.
 *
 *@@added V0.9.18 (2002-02-13) [umoeller]
 *@@changed V0.9.18 (2002-03-27) [umoeller]: made failing modules work
 */

static APIRET CallWinStartApp(HAPP *phapp,            // out: application handle if NO_ERROR is returned
                              HWND hwndNotify,        // in: notify window or NULLHANDLE
                              const PROGDETAILS *pcProgDetails, // in: program spec (req.)
                              ULONG cbFailingName,
                              PSZ pszFailingName)
{
    APIRET arc = NO_ERROR;

    if (!pcProgDetails)
        return (ERROR_INVALID_PARAMETER);

    if (pszFailingName)
        *pszFailingName = '\0';

    if (!(*phapp = WinStartApp(hwndNotify,
                                        // receives WM_APPTERMINATENOTIFY
                               (PPROGDETAILS)pcProgDetails,
                               pcProgDetails->pszParameters,
                               NULL,            // "reserved", PMREF says...
                               SAF_INSTALLEDCMDLINE)))
                                    // we MUST use SAF_INSTALLEDCMDLINE
                                    // or no Win-OS/2 session will start...
                                    // whatever is going on here... Warp 4 FP11

                                    // do not use SAF_STARTCHILDAPP, or the
                                    // app will be terminated automatically
                                    // when the calling process terminates!
    {
        // cannot start app:
        PERRINFO pei;

        #ifdef DEBUG_PROGRAMSTART
            _Pmpf((__FUNCTION__ ": WinStartApp failed"));
        #endif

        // unfortunately WinStartApp doesn't
        // return meaningful codes like DosStartSession, so
        // try to see what happened

        if (pei = WinGetErrorInfo(0))
        {
            #ifdef DEBUG_PROGRAMSTART
                _Pmpf(("  WinGetErrorInfo returned 0x%lX, errorid 0x%lX, %d",
                            pei,
                            pei->idError,
                            ERRORIDERROR(pei->idError)));
            #endif

            switch (ERRORIDERROR(pei->idError))
            {
                case PMERR_DOS_ERROR: //  (0x1200)
                {
                    /*
                    PUSHORT pausMsgOfs = (PUSHORT)(((PBYTE)pei) + pei->offaoffszMsg);
                    PULONG  pulData    = (PULONG)(((PBYTE)pei) + pei->offBinaryData);
                    PSZ     pszMsg     = (PSZ)(((PBYTE)pei) + *pausMsgOfs);

                    CHAR szMsg[1000];
                    sprintf(szMsg, "cDetail: %d\nmsg: %s\n*pul: %d",
                            pei->cDetailLevel,
                            pszMsg,
                            *(pulData - 1));

                    WinMessageBox(HWND_DESKTOP,
                                  NULLHANDLE,
                                  szMsg,
                                  "Error",
                                  0,
                                  MB_OK | MB_MOVEABLE);

                    // Very helpful. The message is "UNK 1200 E",
                    // where I assume "UNK" means "unknown", which is
                    // exactly what I was trying to find out. Oh my.
                    // And cDetailLevel is always 1, which isn't terribly
                    // helpful either. V0.9.18 (2002-03-27) [umoeller]
                    // WHO THE &%õ$ CREATED THESE APIS?

                    */

                    // this is probably the case where the module
                    // couldn't be loaded, so try DosStartSession
                    // to get a meaningful return code... note that
                    // this cannot handle hwndNotify then
                    /* arc = CallDosStartSession(phapp,
                                              pcProgDetails,
                                              cbFailingName,
                                              pszFailingName); */
                    arc = ERROR_FILE_NOT_FOUND;
                }
                break;

                case PMERR_INVALID_APPL: //  (0x1530)
                        // Attempted to start an application whose type is not
                        // recognized by OS/2.
                        // This we get also if the executable doesn't exist...
                        // V0.9.18 (2002-03-27) [umoeller]
                    // arc = ERROR_INVALID_EXE_SIGNATURE;
                    arc = ERROR_FILE_NOT_FOUND;
                break;

                case PMERR_INVALID_PARAMETERS: //  (0x1208)
                        // An application parameter value is invalid for
                        // its converted PM type. For  example: a 4-byte
                        // value outside the range -32 768 to +32 767 cannot be
                        // converted to a SHORT, and a negative number cannot
                        // be converted to a ULONG or USHORT.
                    arc = ERROR_INVALID_DATA;
                break;

                case PMERR_STARTED_IN_BACKGROUND: //  (0x1532)
                        // The application started a new session in the
                        // background.
                    arc = ERROR_SMG_START_IN_BACKGROUND;
                break;

                case PMERR_INVALID_WINDOW: // (0x1206)
                        // The window specified with a Window List call
                        // is not a valid frame window.

                default:
                    arc = ERROR_BAD_FORMAT;
                break;
            }

            WinFreeErrorInfo(pei);
        }
    }

    return arc;
}

/*
 *@@ appStartApp:
 *      wrapper around WinStartApp which fixes the
 *      specified PROGDETAILS to (hopefully) work
 *      work with all executable types.
 *
 *      This first calls appBuildProgDetails (see
 *      remarks there) and then calls WinStartApp.
 *
 *      Since this calls WinStartApp in turn, this
 *      requires a message queue on the calling thread.
 *
 *      Note that this also does minimal checking on
 *      the specified parameters so it can return something
 *      more meaningful than FALSE like WinStartApp.
 *      As a result, you get a DOS error code now (V0.9.16).
 *
 *      Most importantly:
 *
 *      --  ERROR_INVALID_THREADID: not running on thread 1.
 *          See remarks below.
 *
 *      --  ERROR_NOT_ENOUGH_MEMORY
 *
 *      plus the many error codes from appBuildProgDetails,
 *      which gets called in turn.
 *
 *      <B>About enforcing thread 1</B>
 *
 *      OK, after long, long debugging hours, I have found
 *      that WinStartApp hangs the system in the following
 *      cases hard:
 *
 *      --  If a Win-OS/2 session is started and WinStartApp
 *          is _not_ on thread 1. For this reason, we check
 *          if the caller is trying to start a Win-OS/2
 *          session and return ERROR_INVALID_THREADID if
 *          this is not running on thread 1.
 *
 *      --  By contrast, there are many situations where
 *          calling WinStartApp from within the Workplace
 *          process will hang the system, most notably
 *          with VIO sessions. I have been unable to figure
 *          out why this happens, so XWorkplace now uses
 *          its daemon to call WinStartApp instead.
 *
 *          As a word of wisdom, do not call this from
 *          within the Workplace process. For some strange
 *          reason though, the XWorkplace "Run" dialog
 *          (which uses this) _does_ work. Whatever.
 *
 *@@added V0.9.6 (2000-10-16) [umoeller]
 *@@changed V0.9.7 (2000-12-10) [umoeller]: PROGDETAILS.swpInitial no longer zeroed... this broke VIOs
 *@@changed V0.9.7 (2000-12-17) [umoeller]: PROGDETAILS.pszEnvironment no longer zeroed
 *@@changed V0.9.9 (2001-01-27) [umoeller]: crashed if PROGDETAILS.pszExecutable was NULL
 *@@changed V0.9.12 (2001-05-26) [umoeller]: fixed PROG_DEFAULT
 *@@changed V0.9.12 (2001-05-27) [umoeller]: moved from winh.c to apps.c
 *@@changed V0.9.14 (2001-08-07) [pr]: removed some env. strings for Win. apps.
 *@@changed V0.9.14 (2001-08-23) [pr]: added session type options
 *@@changed V0.9.16 (2001-10-19) [umoeller]: added prototype to return APIRET
 *@@changed V0.9.16 (2001-10-19) [umoeller]: added thread-1 check
 *@@changed V0.9.16 (2001-12-06) [umoeller]: now using doshSearchPath for finding pszExecutable if not qualified
 *@@changed V0.9.16 (2002-01-04) [umoeller]: removed error report if startup directory was drive letter only
 *@@changed V0.9.16 (2002-01-04) [umoeller]: added more detailed error reports and *FailingName params
 *@@changed V0.9.18 (2002-02-13) [umoeller]: added CallWinStartApp to fix possible memory problems
 *@@changed V0.9.18 (2002-03-27) [umoeller]: no longer returning ERROR_INVALID_THREADID, except for Win-OS/2 sessions
 *@@changed V0.9.18 (2002-03-27) [umoeller]: extracted appBuildProgDetails
 *@@changed V0.9.19 (2002-03-28) [umoeller]: adjusted for new appBuildProgDetails
 */

APIRET appStartApp(HWND hwndNotify,        // in: notify window or NULLHANDLE
                   const PROGDETAILS *pcProgDetails, // in: program spec (req.)
                   ULONG ulFlags,          // in: APP_RUN_* flags  or 0
                   HAPP *phapp,            // out: application handle if NO_ERROR is returned
                   ULONG cbFailingName,
                   PSZ pszFailingName)
{
    APIRET          arc;

    PPROGDETAILS    pDetails;

    if (!phapp)
        return (ERROR_INVALID_PARAMETER);

    if (!(arc = appBuildProgDetails(&pDetails,
                                    pcProgDetails,
                                    ulFlags)))
    {
        if (pszFailingName)
            strhncpy0(pszFailingName, pDetails->pszExecutable, cbFailingName);

        if (    (appIsWindowsApp(pDetails->progt.progc))
             && (doshMyTID() != 1)          // V0.9.16 (2001-10-19) [umoeller]
           )
            arc = ERROR_INVALID_THREADID;
        else
            arc = CallWinStartApp(phapp,
                                  hwndNotify,
                                  pDetails,
                                  cbFailingName,
                                  pszFailingName);

        DosFreeMem(pDetails);

    } // end if (ProgDetails.pszExecutable)

    #ifdef DEBUG_PROGRAMSTART
        _Pmpf((__FUNCTION__ ": returning %d", arc));
    #endif

    return arc;
}

/*
 *@@ appWaitForApp:
 *      waits for the specified application to terminate
 *      and returns its exit code.
 *
 *@@added V0.9.9 (2001-03-07) [umoeller]
 */

BOOL appWaitForApp(HWND hwndNotify,     // in: notify window
                   HAPP happ,           // in: app to wait for
                   PULONG pulExitCode)  // out: exit code (ptr can be NULL)
{
    BOOL brc = FALSE;

    if (happ)
    {
        // app started:
        // enter a modal message loop until we get the
        // WM_APPTERMINATENOTIFY for happ. Then we
        // know the app is done.
        HAB     hab = WinQueryAnchorBlock(hwndNotify);
        QMSG    qmsg;
        // ULONG   ulXFixReturnCode = 0;
        while (WinGetMsg(hab, &qmsg, NULLHANDLE, 0, 0))
        {
            if (    (qmsg.msg == WM_APPTERMINATENOTIFY)
                 && (qmsg.hwnd == hwndNotify)
                 && (qmsg.mp1 == (MPARAM)happ)
               )
            {
                // xfix has terminated:
                // get xfix return code from mp2... this is:
                // -- 0: everything's OK, continue.
                // -- 1: handle section was rewritten, restart Desktop
                //       now.
                if (pulExitCode)
                    *pulExitCode = (ULONG)qmsg.mp2;
                brc = TRUE;
                // do not dispatch this
                break;
            }

            WinDispatchMsg(hab, &qmsg);
        }
    }

    return brc;
}

/*
 *@@ appQuickStartApp:
 *      shortcut for simply starting an app and
 *      waiting until it's finished.
 *
 *      On errors, NULLHANDLE is returned.
 *
 *      If pulReturnCode != NULL, it receives the
 *      return code of the app.
 *
 *@@added V0.9.16 (2001-10-19) [umoeller]
 */

HAPP appQuickStartApp(const char *pcszFile,
                      ULONG ulProgType,         // e.g. PROG_PM
                      const char *pcszArgs,
                      PULONG pulExitCode)
{
    PROGDETAILS    pd = {0};
    HAPP           happ,
                   happReturn = NULLHANDLE;
    CHAR           szDir[CCHMAXPATH] = "";
    PCSZ           p;
    HWND           hwndObject;

    pd.Length = sizeof(pd);
    pd.progt.progc = ulProgType;
    pd.progt.fbVisible = SHE_VISIBLE;
    pd.pszExecutable = (PSZ)pcszFile;
    pd.pszParameters = (PSZ)pcszArgs;
    if (p = strrchr(pcszFile, '\\'))
    {
        strhncpy0(szDir,
                  pcszFile,
                  p - pcszFile);
        pd.pszStartupDir = szDir;
    }

    if (    (hwndObject = winhCreateObjectWindow(WC_STATIC, NULL))
         && (!appStartApp(hwndObject,
                          &pd,
                          0,
                          &happ,
                          0,
                          NULL))
       )
    {
        if (appWaitForApp(hwndObject,
                          happ,
                          pulExitCode))
            happReturn = happ;
    }

    return happReturn;
}
