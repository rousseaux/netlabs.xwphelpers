
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
#define INCL_DOSMODULEMGR
#define INCL_DOSSESMGR
#define INCL_DOSERRORS

#define INCL_WINPROGRAMLIST     // needed for PROGDETAILS, wppgm.h
#define INCL_WINERRORS
#define INCL_SHLERRORS
#include <os2.h>

#include <stdio.h>

#include "setup.h"                      // code generation and debugging options

#include "helpers\apps.h"
#include "helpers\dosh.h"
#include "helpers\prfh.h"
#include "helpers\standards.h"          // some standard macros
#include "helpers\stringh.h"
#include "helpers\winh.h"
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

    return (cbEnvironment);
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

    return (arc);
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

    return (arc);
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

    return (arc);
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
                       PULONG pulDosAppType,
                       PULONG pulWinAppType)
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

    return (arc);
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
            return (G_aProgTypes[ul].pcsz);
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
 *@@ CallWinStartApp:
 *      wrapper around WinStartApp which copies all the
 *      parameters into a contiguous block of tiled memory.
 *
 *      This might fix some of the problems with truncated
 *      environments we were having because apparently the
 *      WinStartApp thunking to 16-bit doesn't always work.
 *
 *@@added V0.9.18 (2002-02-13) [umoeller]
 */

static APIRET CallWinStartApp(HAPP *phapp,            // out: application handle if NO_ERROR is returned
                              HWND hwndNotify,        // in: notify window or NULLHANDLE
                              const PROGDETAILS *pcProgDetails, // in: program spec (req.)
                              PCSZ pcszParamsPatched)
{
    ULONG   cb,
            cbTitle,
            cbExecutable,
            cbParameters,
            cbStartupDir,
            cbIcon,
            cbEnvironment;

    APIRET  arc = NO_ERROR;

    /*
    if (WinMessageBox(HWND_DESKTOP,
                      NULLHANDLE,
                      (ProgDetails.pszExecutable) ? ProgDetails.pszExecutable : "NULL",
                      "Start?",
                      0,
                      MB_YESNO | MB_MOVEABLE)
              != MBID_YES)
        return (ERROR_INTERRUPT);
    */

    // allocate a chunk of tiled memory from OS/2 to make sure
    // this is aligned on a 64K memory (backed up by a 16-bit
    // LDT selector)
    cb = sizeof(PROGDETAILS);
    if (cbTitle = strhSize(pcProgDetails->pszTitle))
        cb += cbTitle;
    if (cbExecutable = strhSize(pcProgDetails->pszExecutable))
        cb += cbExecutable;
    if (cbParameters = strhSize(pcProgDetails->pszParameters))
        cb += cbParameters;
    if (cbStartupDir = strhSize(pcProgDetails->pszStartupDir))
        cb += cbStartupDir;
    if (cbIcon = strhSize(pcProgDetails->pszIcon))
        cb += cbIcon;
    if (cbEnvironment = appQueryEnvironmentLen(pcProgDetails->pszEnvironment))
        cb += cbEnvironment;

    if (cb > 60000)     // to be on the safe side
        arc = ERROR_BAD_ENVIRONMENT; // 10;
    else
    {
        PPROGDETAILS pNewProgDetails;
        if (!(arc = DosAllocMem((PVOID*)&pNewProgDetails,
                                cb,
                                PAG_COMMIT | OBJ_TILE | PAG_READ | PAG_WRITE)))
        {
            // alright, copy stuff
            PBYTE pThis;

            memset(pNewProgDetails, 0, sizeof(PROGDETAILS));

            pNewProgDetails->Length = sizeof(PROGDETAILS);
            pNewProgDetails->progt.progc = pcProgDetails->progt.progc;
            pNewProgDetails->progt.fbVisible = pcProgDetails->progt.fbVisible;
            memcpy(&pNewProgDetails->swpInitial, &pcProgDetails->swpInitial, sizeof(SWP));

            pThis = (PBYTE)(pNewProgDetails + 1);

            if (cbTitle)
            {
                memcpy(pThis, pcProgDetails->pszTitle, cbTitle);
                pNewProgDetails->pszTitle = pThis;
                pThis += cbTitle;
            }

            if (cbExecutable)
            {
                memcpy(pThis, pcProgDetails->pszExecutable, cbExecutable);
                pNewProgDetails->pszExecutable = pThis;
                pThis += cbExecutable;
            }

            if (cbParameters)
            {
                memcpy(pThis, pcProgDetails->pszParameters, cbParameters);
                pNewProgDetails->pszParameters = pThis;
                pThis += cbParameters;
            }

            if (cbStartupDir)
            {
                memcpy(pThis, pcProgDetails->pszStartupDir, cbStartupDir);
                pNewProgDetails->pszStartupDir = pThis;
                pThis += cbStartupDir;
            }

            if (cbIcon)
            {
                memcpy(pThis, pcProgDetails->pszIcon, cbIcon);
                pNewProgDetails->pszIcon = pThis;
                pThis += cbIcon;
            }

            if (cbEnvironment)
            {
                memcpy(pThis, pcProgDetails->pszEnvironment, cbEnvironment);
                pNewProgDetails->pszEnvironment = pThis;
                pThis += cbEnvironment;
            }

            #ifdef DEBUG_PROGRAMSTART
                _Pmpf((__FUNCTION__ ": progt.progc: %d", pNewProgDetails->progt.progc));
                _Pmpf(("    progt.fbVisible: 0x%lX", pNewProgDetails->progt.fbVisible));
                _Pmpf(("    progt.pszTitle: \"%s\"", (pNewProgDetails->pszTitle) ? pNewProgDetails->pszTitle : "NULL"));
                _Pmpf(("    exec: \"%s\"", (pNewProgDetails->pszExecutable) ? pNewProgDetails->pszExecutable : "NULL"));
                _Pmpf(("    params: \"%s\"", (pNewProgDetails->pszParameters) ? pNewProgDetails->pszParameters : "NULL"));
                _Pmpf(("    startup: \"%s\"", (pNewProgDetails->pszStartupDir) ? pNewProgDetails->pszStartupDir : "NULL"));
                _Pmpf(("    pszIcon: \"%s\"", (pNewProgDetails->pszIcon) ? pNewProgDetails->pszIcon : "NULL"));
                _Pmpf(("    environment: "));
                {
                    PSZ pszThis = pNewProgDetails->pszEnvironment;
                    while (pszThis && *pszThis)
                    {
                        _Pmpf(("      \"%s\"", pszThis));
                        pszThis += strlen(pszThis) + 1;
                    }
                }

                _Pmpf(("    swpInitial.fl = 0x%lX, x = %d, y = %d, cx = %d, cy = %d:",
                            pNewProgDetails->swpInitial.fl,
                            pNewProgDetails->swpInitial.x,
                            pNewProgDetails->swpInitial.y,
                            pNewProgDetails->swpInitial.cx,
                            pNewProgDetails->swpInitial.cy));
                _Pmpf(("    behind = %d, hwnd = %d, res1 = %d, res2 = %d",
                            pNewProgDetails->swpInitial.hwndInsertBehind,
                            pNewProgDetails->swpInitial.hwnd,
                            pNewProgDetails->swpInitial.ulReserved1,
                            pNewProgDetails->swpInitial.ulReserved2));
            #endif

            if (!(*phapp = WinStartApp(hwndNotify,
                                                // receives WM_APPTERMINATENOTIFY
                                       pNewProgDetails,
                                       pNewProgDetails->pszParameters,
                                       NULL,            // "reserved", PMREF says...
                                       SAF_INSTALLEDCMDLINE)))
                                            // we MUST use SAF_INSTALLEDCMDLINE
                                            // or no Win-OS/2 session will start...
                                            // whatever is going on here... Warp 4 FP11

                                            // do not use SAF_STARTCHILDAPP, or the
                                            // app will be terminated automatically
                                            // when the WPS terminates!
            {
                // cannot start app:
                #ifdef DEBUG_PROGRAMSTART
                    _Pmpf((__FUNCTION__ ": WinStartApp failed"));
                #endif

                arc = ERROR_FILE_NOT_FOUND;
                // unfortunately WinStartApp doesn't
                // return meaningful codes like DosStartSession, so
                // try to see what happened
                /*
                switch (ERRORIDERROR(WinGetLastError(0)))
                {
                    case PMERR_DOS_ERROR: //  (0x1200)
                    {
                        arc = ERROR_FILE_NOT_FOUND;

                        // this is probably the case where the module
                        // couldn't be loaded, so try DosStartSession
                        // to get a meaningful return code... note that
                        // this cannot handle hwndNotify then
                        RESULTCODES result;
                        arc = DosExecPgm(pszFailingName,
                                         cbFailingName,
                                         EXEC_ASYNC,
                                         NULL, // ProgDetails.pszParameters,
                                         NULL, // ProgDetails.pszEnvironment,
                                         &result,
                                         ProgDetails.pszExecutable);
                        ULONG sid, pid;
                        STARTDATA   SData;
                        SData.Length  = sizeof(STARTDATA);
                        SData.Related = SSF_RELATED_CHILD; //INDEPENDENT;
                        SData.FgBg    = SSF_FGBG_FORE;
                        SData.TraceOpt = SSF_TRACEOPT_NONE;

                        SData.PgmTitle = ProgDetails.pszTitle;
                        SData.PgmName = ProgDetails.pszExecutable;
                        SData.PgmInputs = ProgDetails.pszParameters;

                        SData.TermQ = NULL;
                        SData.Environment = ProgDetails.pszEnvironment;
                        SData.InheritOpt = SSF_INHERTOPT_PARENT;    // ignored
                        SData.SessionType = SSF_TYPE_DEFAULT;
                        SData.IconFile = 0;
                        SData.PgmHandle = 0;

                        SData.PgmControl = SSF_CONTROL_VISIBLE;

                        SData.InitXPos  = 30;
                        SData.InitYPos  = 40;
                        SData.InitXSize = 200;
                        SData.InitYSize = 140;
                        SData.Reserved = 0;
                        SData.ObjectBuffer  = pszFailingName;
                        SData.ObjectBuffLen = cbFailingName;

                        arc = DosStartSession(&SData, &sid, &pid);
                    }
                    break;

                    case PMERR_INVALID_APPL: //  (0x1530)
                            // Attempted to start an application whose type is not
                            // recognized by OS/2.
                        arc = ERROR_INVALID_EXE_SIGNATURE;
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
                */
            }

            DosFreeMem(pNewProgDetails);
        }
    }

    return (arc);
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
 *         PROGDETAILS.pszExecutable;
 *
 *      -- starting apps which are not fully qualified
 *         and therefore assumed to be on the PATH.
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
 *      --  However, if you specify an environment, you _must_
 *          specify a complete environment. This function
 *          will not merge environments. Use
 *          appSetEnvironmentVar to change environment
 *          variables in a complete environment set.
 *
 *      --  If PROGDETAILS specifies a Win-OS/2 session
 *          and PROGDETAILS.pszEnvironment is empty,
 *          this uses the default Win-OS/2 environment.
 *          See appQueryDefaultWin31Environment.
 *
 *      Even though this isn't clearly said in PMREF,
 *      PROGDETAILS.swpInitial is important:
 *
 *      -- To start a session minimized, set SWP_MINIMIZE.
 *
 *      -- To start a VIO session with auto-close disabled,
 *         set the half-documented SWP_NOAUTOCLOSE flag (0x8000)
 *         This flag is now in the newer toolkit headers.
 *
 *      In addition, this supports the following session
 *      flags with ulFlags if PROG_DEFAULT is specified:
 *
 *      --  APP_RUN_FULLSCREEN
 *
 *      --  APP_RUN_ENHANCED
 *
 *      --  APP_RUN_STANDARD
 *
 *      --  APP_RUN_SEPARATE
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
 *          Since this uses WinStartApp internally and
 *          WinStartApp completely hangs the session manager
 *          if a Win-OS/2 full-screen session is started from
 *          a thread that is NOT thread 1, this will now fail
 *          with this error for safety (V0.9.16).
 *
 *      --  ERROR_INVALID_PARAMETER: pcProgDetails or
 *          phapp is NULL; or PROGDETAILS.pszExecutable is NULL.
 *
 *      --  ERROR_FILE_NOT_FOUND, ERROR_PATH_NOT_FOUND:
 *          PROGDETAILS.pszExecutable and/or PROGDETAILS.pszStartupDir
 *          are invalid.
 *          A NULL PROGDETAILS.pszStartupDir is supported though.
 *
 *      --  ERROR_NOT_ENOUGH_MEMORY
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
 */

APIRET appStartApp(HWND hwndNotify,        // in: notify window or NULLHANDLE
                   const PROGDETAILS *pcProgDetails, // in: program spec (req.)
                   ULONG ulFlags,          // in: APP_RUN_* flags
                   HAPP *phapp,            // out: application handle if NO_ERROR is returned
                   ULONG cbFailingName,
                   PSZ pszFailingName)
{
    APIRET          arc = NO_ERROR;
    PROGDETAILS     ProgDetails;

    if (pszFailingName)
        *pszFailingName = '\0';

    if (!pcProgDetails || !phapp)
        return (ERROR_INVALID_PARAMETER);

    memcpy(&ProgDetails, pcProgDetails, sizeof(PROGDETAILS));
            // pointers still point into old prog details buffer
    ProgDetails.Length = sizeof(PROGDETAILS);
    ProgDetails.progt.fbVisible = SHE_VISIBLE;

    // all this only makes sense if this contains something...
    // besides, this crashed on string comparisons V0.9.9 (2001-01-27) [umoeller]
    if (    (!ProgDetails.pszExecutable)
         || (!(*(ProgDetails.pszExecutable)))
       )
        arc = ERROR_INVALID_PARAMETER;
    else if (doshMyTID() != 1)          // V0.9.16 (2001-10-19) [umoeller]
        arc = ERROR_INVALID_THREADID;
    else
    {
        ULONG           ulIsWinApp;

        CHAR            szFQExecutable[CCHMAXPATH];

        XSTRING         strParamsPatched;
        PSZ             pszWinOS2Env = 0;

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

        #ifdef DEBUG_PROGRAMSTART
            _Pmpf((__FUNCTION__ ": old progc: 0x%lX", pcProgDetails->progt.progc));
            _Pmpf(("  pszTitle: %s", (ProgDetails.pszTitle) ? ProgDetails.pszTitle : NULL));
            _Pmpf(("  pszIcon: %s", (ProgDetails.pszIcon) ? ProgDetails.pszIcon : NULL));
        #endif

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

        // set session type from option flags
        if (ulFlags & APP_RUN_FULLSCREEN)
        {
            if (ProgDetails.progt.progc == PROG_WINDOWABLEVIO)
                ProgDetails.progt.progc = PROG_FULLSCREEN;

            if (ProgDetails.progt.progc == PROG_WINDOWEDVDM)
                ProgDetails.progt.progc = PROG_VDM;
        }

        if (ulIsWinApp = appIsWindowsApp(ProgDetails.progt.progc))
        {
            if (ulFlags & APP_RUN_FULLSCREEN)
                ProgDetails.progt.progc = (ulFlags & APP_RUN_ENHANCED)
                                                ? PROG_31_ENH
                                                : PROG_31_STD;
            else
            {
                if (ulFlags & APP_RUN_STANDARD)
                    ProgDetails.progt.progc = (ulFlags & APP_RUN_SEPARATE)
                                                ? PROG_31_STDSEAMLESSVDM
                                                : PROG_31_STDSEAMLESSCOMMON;

                if (ulFlags & APP_RUN_ENHANCED)
                    ProgDetails.progt.progc = (ulFlags & APP_RUN_SEPARATE)
                                                ? PROG_31_ENHSEAMLESSVDM
                                                : PROG_31_ENHSEAMLESSCOMMON;
            }

            // re-run V0.9.16 (2001-10-19) [umoeller]
            ulIsWinApp = appIsWindowsApp(ProgDetails.progt.progc);
        }

        /*
         * command lines fixups:
         *
         */

        if (!strcmp(ProgDetails.pszExecutable, "*"))
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
        {
            // check if the executable is fully qualified; if so,
            // check if the executable file exists
            if (    (ProgDetails.pszExecutable[1] == ':')
                 && (strchr(ProgDetails.pszExecutable, '\\'))
               )
            {
                ULONG ulAttr;
                if (!(arc = doshQueryPathAttr(ProgDetails.pszExecutable,
                                              &ulAttr)))
                {
                    // make sure startup dir is really a directory
                    if (ProgDetails.pszStartupDir)
                    {
                        // it is valid to specify a startup dir of "C:"
                        if (    (strlen(ProgDetails.pszStartupDir) > 2)
                             && (!(arc = doshQueryPathAttr(ProgDetails.pszStartupDir,
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
                if (!(arc = doshSearchPath("PATH",
                                           ProgDetails.pszExecutable,
                                           szFQExecutable,
                                           sizeof(szFQExecutable))))
                    // alright, found it:
                    ProgDetails.pszExecutable = szFQExecutable;
            }

            if (!arc)
            {
                PSZ pszExtension;
                switch (ProgDetails.progt.progc)
                {
                    /*
                     *  .CMD files fixups
                     *
                     */

                    case PROG_FULLSCREEN:       // OS/2 fullscreen
                    case PROG_WINDOWABLEVIO:    // OS/2 window
                    {
                        if (    (pszExtension = doshGetExtension(ProgDetails.pszExecutable))
                             && (!stricmp(pszExtension, "CMD"))
                           )
                        {
                            CallBatchCorrectly(&ProgDetails,
                                               &strParamsPatched,
                                               "OS2_SHELL",
                                               "CMD.EXE");
                        }
                    break; }

                    case PROG_VDM:              // DOS fullscreen
                    case PROG_WINDOWEDVDM:      // DOS window
                    {
                        if (    (pszExtension = doshGetExtension(ProgDetails.pszExecutable))
                             && (!stricmp(pszExtension, "BAT"))
                           )
                        {
                            CallBatchCorrectly(&ProgDetails,
                                               &strParamsPatched,
                                               NULL,
                                               "COMMAND.COM");
                        }
                    break; }
                } // end switch (ProgDetails.progt.progc)
            }
        }

        if (!arc)
        {
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
                        ProgDetails.pszEnvironment = pszWinOS2Env;

                    appFreeEnvironment(&Env);
                }

                free(pszTemp);
            }

            if (!arc)
            {
                if (!ProgDetails.pszTitle)
                    ProgDetails.pszTitle = ProgDetails.pszExecutable;

                ProgDetails.pszParameters = strParamsPatched.psz;

                if (pszFailingName)
                    strhncpy0(pszFailingName, ProgDetails.pszExecutable, cbFailingName);

                arc = CallWinStartApp(phapp,
                                      hwndNotify,
                                      &ProgDetails,
                                      strParamsPatched.psz);
            }
        }

        xstrClear(&strParamsPatched);
        if (pszWinOS2Env)
            free(pszWinOS2Env);
    } // end if (ProgDetails.pszExecutable)

    #ifdef DEBUG_PROGRAMSTART
        _Pmpf((__FUNCTION__ ": returning %d", arc));
    #endif

    return (arc);
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

    return (brc);
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

    return (happReturn);
}
