
/*
 *@@sourcefile dosh2.c:
 *      dosh.c contains more Control Program helper functions.
 *
 *      This file is new with V0.9.4 (2000-07-26) [umoeller].
 *
 *      As opposed to the functions in dosh.c, these require
 *      linking against other helpers. As a result, these have
 *      been separated from dosh.c to allow linking against
 *      dosh.obj only.
 *
 *      Function prefixes:
 *      --  dosh*   Dos (Control Program) helper functions
 *
 *      This has the same header as dosh.c, dosh.h.
 *
 *      The partition functions in this file are based on
 *      code which has kindly been provided by Dmitry A. Steklenev.
 *      See doshGetPartitionsList for how to use these.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@header "helpers\dosh.h"
 *@@added V0.9.4 (2000-07-27) [umoeller]
 */

/*
 *      This file Copyright (C) 1997-2000 Ulrich M攍ler,
 *                                        Dmitry A. Steklenev.
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

#define INCL_DOSMODULEMGR
#define INCL_DOSPROCESS
#define INCL_DOSSESMGR
#define INCL_DOSQUEUES
#define INCL_DOSMISC
#define INCL_DOSDEVICES
#define INCL_DOSDEVIOCTL
#define INCL_DOSERRORS
#include <os2.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "setup.h"                      // code generation and debugging options

#include "helpers\dosh.h"
#include "helpers\stringh.h"

#pragma hdrstop

/*
 *@@category: Helpers\Control program helpers\Miscellaneous
 */

/* ******************************************************************
 *
 *   Miscellaneous
 *
 ********************************************************************/

/*
 *@@ doshIsValidFileName:
 *      this returns NO_ERROR only if pszFile is a valid file name.
 *      This may include a full path.
 *
 *      If a drive letter is specified, this checks for whether
 *      that drive is a FAT drive and adjust the checks accordingly,
 *      i.e. 8+3 syntax (per path component).
 *
 *      If no drive letter is specified, this check is performed
 *      for the current drive.
 *
 *      This also checks if pszFileNames contains characters which
 *      are invalid for the current drive.
 *
 *      Note: this performs syntactic checks only. This does not
 *      check for whether the specified path components exist.
 *      However, it _is_ checked for whether the given drive
 *      exists.
 *
 *      This func is especially useful to check filenames that
 *      have been entered by the user in a "Save as" dialog.
 *
 *      If an error is found, the corresponding DOS error code
 *      is returned:
 *      --  ERROR_INVALID_DRIVE
 *      --  ERROR_FILENAME_EXCED_RANGE  (on FAT: no 8+3 filename)
 *      --  ERROR_INVALID_NAME          (invalid character)
 *      --  ERROR_CURRENT_DIRECTORY     (if fFullyQualified: no full path specified)
 *
 *@@changed V0.9.2 (2000-03-11) [umoeller]: added fFullyQualified
 */

APIRET doshIsValidFileName(const char* pcszFile,
                           BOOL fFullyQualified)    // in: if TRUE, pcszFile must be fully q'fied
{
    APIRET  arc = NO_ERROR;
    CHAR    szPath[CCHMAXPATH+4] = " :";
    CHAR    szComponent[CCHMAXPATH];
    PSZ     p1, p2;
    BOOL    fIsFAT = FALSE;
    PSZ     pszInvalid;

    if (fFullyQualified)    // V0.9.2 (2000-03-11) [umoeller]
    {
        if (    (*(pcszFile + 1) != ':')
             || (*(pcszFile + 2) != '\\')
           )
            arc = ERROR_CURRENT_DIRECTORY;
    }

    // check drive first
    if (*(pcszFile + 1) == ':')
    {
        CHAR cDrive = toupper(*pcszFile);
        double d;
        // drive specified:
        strcpy(szPath, pcszFile);
        szPath[0] = toupper(*pcszFile);
        arc = doshQueryDiskFree(cDrive - 'A' + 1, &d);
    }
    else
    {
        // no drive specified: take current
        ULONG   ulDriveNum = 0,
                ulDriveMap = 0;
        arc = DosQueryCurrentDisk(&ulDriveNum, &ulDriveMap);
        szPath[0] = ((UCHAR)ulDriveNum) + 'A' - 1;
        szPath[1] = ':';
        strcpy(&szPath[2], pcszFile);
    }

    if (arc == NO_ERROR)
    {
        fIsFAT = doshIsFileOnFAT(szPath);

        pszInvalid = (fIsFAT)
                        ? "<>|+=:;,\"/[] "  // invalid characters in FAT
                        : "<>|:\"/";        // invalid characters in IFS's

        // now separate path components
        p1 = &szPath[2];       // advance past ':'

        do {

            if (*p1 == '\\')
                p1++;

            p2 = strchr(p1, '\\');
            if (p2 == NULL)
                p2 = p1 + strlen(p1);

            if (p1 != p2)
            {
                LONG    lDotOfs = -1,
                        lAfterDot = -1;
                ULONG   cbFile,
                        ul;
                PSZ     pSource = szComponent;

                strncpy(szComponent, p1, p2-p1);
                szComponent[p2-p1] = 0;
                cbFile = strlen(szComponent);

                // now check each path component
                for (ul = 0; ul < cbFile; ul++)
                {
                    if (fIsFAT)
                    {
                        // on FAT: only 8 characters allowed before dot
                        if (*pSource == '.')
                        {
                            lDotOfs = ul;
                            lAfterDot = 0;
                            if (ul > 7)
                                return (ERROR_FILENAME_EXCED_RANGE);
                        }
                    }
                    // and check for invalid characters
                    if (strchr(pszInvalid, *pSource) != NULL)
                        return (ERROR_INVALID_NAME);

                    pSource++;

                    // on FAT, allow only three chars after dot
                    if (fIsFAT)
                        if (lAfterDot != -1)
                        {
                            lAfterDot++;
                            if (lAfterDot > 3)
                                return (ERROR_FILENAME_EXCED_RANGE);
                        }
                }

                // we are still missing the case of a FAT file
                // name without extension; if so, check whether
                // the file stem is <= 8 chars
                if (fIsFAT)
                    if (lDotOfs == -1)  // dot not found:
                        if (cbFile > 8)
                            return (ERROR_FILENAME_EXCED_RANGE);
            }

            // go for next component
            p1 = p2+1;
        } while (*p2);
    }

    return (arc);
}

/*
 *@@ doshMakeRealName:
 *      this copies pszSource to pszTarget, replacing
 *      all characters which are not supported by file
 *      systems with cReplace.
 *
 *      pszTarget must be at least the same size as pszSource.
 *      If (fIsFAT), the file name will be made FAT-compliant (8+3).
 *
 *      Returns TRUE if characters were replaced.
 *
 *@@changed V0.9.0 (99-11-06) [umoeller]: now replacing "*" too
 */

BOOL doshMakeRealName(PSZ pszTarget,    // out: new real name
                      PSZ pszSource,    // in: filename to translate
                      CHAR cReplace,    // in: replacement char for invalid
                                        //     characters (e.g. '!')
                      BOOL fIsFAT)      // in: make-FAT-compatible flag
{
    ULONG ul,
          cbSource = strlen(pszSource);
    LONG  lDotOfs = -1,
          lAfterDot = -1;
    BOOL  brc = FALSE;
    PSZ   pSource = pszSource,
          pTarget = pszTarget;

    const char *pcszInvalid = (fIsFAT)
                                   ? "*<>|+=:;,\"/\\[] "  // invalid characters in FAT
                                   : "*<>|:\"/\\"; // invalid characters in IFS's

    for (ul = 0; ul < cbSource; ul++)
    {
        if (fIsFAT)
        {
            // on FAT: truncate filename if neccessary
            if (*pSource == '.')
            {
                lDotOfs = ul;
                lAfterDot = 0;
                if (ul > 7) {
                    // only 8 characters allowed before dot,
                    // so set target ptr to dot pos
                    pTarget = pszTarget+8;
                }
            }
        }
        // and replace invalid characters
        if (strchr(pcszInvalid, *pSource) == NULL)
            *pTarget = *pSource;
        else
        {
            *pTarget = cReplace;
            brc = TRUE;
        }
        pTarget++;
        pSource++;

        // on FAT, allow only three chars after dot
        if (fIsFAT)
            if (lAfterDot != -1)
            {
                lAfterDot++;
                if (lAfterDot > 3)
                    break;
            }
    }
    *pTarget = '\0';

    if (fIsFAT)
    {
        // we are still missing the case of a FAT file
        // name without extension; if so, check whether
        // the file stem is <= 8 chars
        if (lDotOfs == -1)  // dot not found:
            if (cbSource > 8)
                *(pszTarget+8) = 0; // truncate

        // convert to upper case
        strupr(pszTarget);
    }

    return (brc);
}

/*
 *@@ doshSetCurrentDir:
 *      sets the current working directory
 *      to the given path.
 *
 *      As opposed to DosSetCurrentDir, this
 *      one will change the current drive
 *      also, if one is specified.
 *
 *@@changed V0.9.9 (2001-04-04) [umoeller]: this returned an error even if none occured, fixed
 */

APIRET doshSetCurrentDir(const char *pcszDir)
{
    APIRET  arc = NO_ERROR;
    if (!pcszDir)
        return (ERROR_INVALID_PARAMETER);
    {
        if (*pcszDir != 0)
            if (*(pcszDir+1) == ':')
            {
                // drive given:
                CHAR    cDrive = toupper(*(pcszDir));
                // change drive
                arc = DosSetDefaultDisk( (ULONG)(cDrive - 'A' + 1) );
                        // 1 = A:, 2 = B:, ...
            }

        arc = DosSetCurrentDir((PSZ)pcszDir);
    }

    return (arc);       // V0.9.9 (2001-04-04) [umoeller]
}

/*
 *@@category: Helpers\Control program helpers\Environment management
 *      helpers for managing those ugly environment string arrays
 *      that are used with DosStartSession and WinStartApp.
 */

/* ******************************************************************
 *
 *   Environment helpers
 *
 ********************************************************************/

/*
 *@@ doshParseEnvironment:
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
 *      doshFreeEnvironment to free the memory allocated
 *      by this function.
 *
 *      Use the following code to browse thru the array:
 +
 +          DOSENVIRONMENT Env = {0};
 +          if (doshParseEnvironment(pszEnv,
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
 +              doshFreeEnvironment(&Env);
 +          }
 *
 *@@added V0.9.4 (2000-08-02) [umoeller]
 */

APIRET doshParseEnvironment(const char *pcszEnv,
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
 *@@ doshGetEnvironment:
 *      calls doshParseEnvironment for the current
 *      process environment, which is retrieved from
 *      the info blocks.
 *
 *@@added V0.9.4 (2000-07-19) [umoeller]
 */

APIRET doshGetEnvironment(PDOSENVIRONMENT pEnv)
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
                arc = doshParseEnvironment(pszEnv, pEnv);
            else
                arc = ERROR_BAD_ENVIRONMENT;
        }
    }

    return (arc);
}

/*
 *@@ doshFindEnvironmentVar:
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
 */

PSZ* doshFindEnvironmentVar(PDOSENVIRONMENT pEnv,
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
        }
    }

    return (ppszRet);
}

/*
 *@@ doshSetEnvironmentVar:
 *      sets an environment variable in the specified
 *      environment, which must have been initialized
 *      using doshGetEnvironment first.
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
 */

APIRET doshSetEnvironmentVar(PDOSENVIRONMENT pEnv,
                             PSZ pszNewEnv,
                             BOOL fAddFirst)
{
    APIRET  arc = NO_ERROR;
    if (!pEnv)
        arc = ERROR_INVALID_PARAMETER;
    else
    {
        if ((!pEnv->papszVars) || (!pszNewEnv))
            arc = ERROR_INVALID_PARAMETER;
        else
        {
            PSZ *ppszEnvLine = doshFindEnvironmentVar(pEnv, pszNewEnv);
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
                PSZ *ppszNew = NULL;
                PSZ *papszNew = NULL;
                // not set already:
                if (fAddFirst)
                {
                    // add as first entry:
                    papszNew = (PSZ*)malloc(sizeof(PSZ) * (pEnv->cVars + 1));
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
                    // reallocate array and add new string
                    papszNew = (PSZ*)realloc(pEnv->papszVars,
                                             sizeof(PSZ) * (pEnv->cVars + 1));
                    // overwrite last entry
                    ppszNew = papszNew + pEnv->cVars;
                }

                if (!papszNew)
                    arc = ERROR_NOT_ENOUGH_MEMORY;
                else
                {
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
 *@@ doshConvertEnvironment:
 *      converts an environment initialized by doshGetEnvironment
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

APIRET doshConvertEnvironment(PDOSENVIRONMENT pEnv,
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
 *@@ doshFreeEnvironment:
 *      frees memory allocated by doshGetEnvironment.
 *
 *@@added V0.9.4 (2000-07-19) [umoeller]
 */

APIRET doshFreeEnvironment(PDOSENVIRONMENT pEnv)
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

/*
 *@@category: Helpers\Control program helpers\Executable info
 *      these functions can retrieve BLDLEVEL information,
 *      imported modules information, exported functions information,
 *      and resources information from any executable module. See
 *      doshExecOpen.
 */

/********************************************************************
 *
 *   Executable functions
 *
 ********************************************************************/

/*
 *@@ doshExecOpen:
 *      this opens the specified executable file
 *      (which can be an .EXE, .COM, .DLL, or
 *      driver file) for use with the other
 *      doshExec* functions.
 *
 *      If no error occurs, NO_ERROR is returned
 *      and a pointer to a new EXECUTABLE structure
 *      is stored in *ppExec. Consider this pointer a
 *      handle and pass it to doshExecClose to clean
 *      up.
 *
 *      If NO_ERROR is returned, all the fields through
 *      ulOS are set in EXECUTABLE. The psz* fields
 *      which follow afterwards require an additional
 *      call to doshExecQueryBldLevel.
 *
 *      NOTE: If NO_ERROR is returned, the executable
 *      file has been opened by this function. It will
 *      only be closed when you call doshExecClose.
 *
 *      If errors occur, this function returns the
 *      following error codes:
 *
 *      -- ERROR_NOT_ENOUGH_MEMORY: malloc() failed.
 *
 *      -- ERROR_INVALID_EXE_SIGNATURE: specified file
 *              has no DOS EXE header, or it does, but
 *              the extended header is neither LX nor
 *              NE nor PE.
 *
 *      -- ERROR_INVALID_PARAMETER: ppExec is NULL.
 *
 *      plus those of DosOpen, DosSetFilePtr, and
 *      DosRead.
 *
 *@@added V0.9.0 [umoeller]
 *@@changed V0.9.1 (2000-02-13) [umoeller]: fixed 32-bits flag
 *@@changed V0.9.7 (2000-12-20) [lafaix]: fixed ulNewHeaderOfs
 */

APIRET doshExecOpen(const char* pcszExecutable,
                    PEXECUTABLE* ppExec)
{
    APIRET  arc = NO_ERROR;

    ULONG   ulAction = 0;
    HFILE   hFile;

    if (!ppExec)
        return (ERROR_INVALID_PARAMETER);

    *ppExec = (PEXECUTABLE)malloc(sizeof(EXECUTABLE));
    if (!(*ppExec))
        return (ERROR_NOT_ENOUGH_MEMORY);

    memset((*ppExec), 0, sizeof(EXECUTABLE));

    if (!(arc = DosOpen((PSZ)pcszExecutable,
                        &hFile,
                        &ulAction,                      // out: action taken
                        0,                              // in: new file (ignored for read-mode)
                        0,                              // in: new file attribs (ignored)
                        // open-flags
                        OPEN_ACTION_FAIL_IF_NEW
                           | OPEN_ACTION_OPEN_IF_EXISTS,
                        // open-mode
                        OPEN_FLAGS_FAIL_ON_ERROR        // report errors to caller
                           | OPEN_FLAGS_SEQUENTIAL
                           | OPEN_FLAGS_NOINHERIT
                           | OPEN_SHARE_DENYNONE
                           | OPEN_ACCESS_READONLY,      // read-only mode
                        NULL)))                         // no EAs
    {
        // file opened successfully:

        ULONG           ulLocal = 0;

        // read old DOS EXE header
        (*ppExec)->pDosExeHeader = (PDOSEXEHEADER)malloc(sizeof(DOSEXEHEADER));
        if (!((*ppExec)->pDosExeHeader))
            arc = ERROR_NOT_ENOUGH_MEMORY;
        else
        {
            ULONG   ulBytesRead = 0;

            if (    (!(arc = DosSetFilePtr(hFile,
                                           0L,
                                           FILE_BEGIN,
                                           &ulLocal)))      // out: new offset
                 && (!(arc = DosRead(hFile,
                                     (*ppExec)->pDosExeHeader,
                                     sizeof(DOSEXEHEADER),
                                     &((*ppExec)->cbDosExeHeader))))
               )
            {
                // now check if we really have a DOS header
                if ((*ppExec)->pDosExeHeader->usDosExeID != 0x5a4d)
                    arc = ERROR_INVALID_EXE_SIGNATURE;
                else
                {
                    // we have a DOS header:
                    if ((*ppExec)->pDosExeHeader->usRelocTableOfs < 0x40)
                    {
                        // neither LX nor PE nor NE:
                        (*ppExec)->ulOS = EXEOS_DOS3;
                        (*ppExec)->ulExeFormat = EXEFORMAT_OLDDOS;
                    }
                    else
                    {
                        // either LX or PE or NE:
                        // read more bytes from position specified in header
                        CHAR    achNewHeaderType[2] = "";

                        if (    (!(arc = DosSetFilePtr(hFile,
                                                       (*ppExec)->pDosExeHeader->ulNewHeaderOfs,
                                                       FILE_BEGIN,
                                                       &ulLocal)))
                                // read two chars to find out header type
                             && (!(arc = DosRead(hFile,
                                                 &achNewHeaderType,
                                                 sizeof(achNewHeaderType),
                                                 &ulBytesRead)))
                           )
                        {
                            PBYTE   pbCheckOS = NULL;

                            // reset file ptr
                            DosSetFilePtr(hFile,
                                          (*ppExec)->pDosExeHeader->ulNewHeaderOfs,
                                          FILE_BEGIN,
                                          &ulLocal);

                            if (memcmp(achNewHeaderType, "NE", 2) == 0)
                            {
                                // New Executable:
                                (*ppExec)->ulExeFormat = EXEFORMAT_NE;
                                // allocate NE header
                                (*ppExec)->pNEHeader = (PNEHEADER)malloc(sizeof(NEHEADER));
                                if (!((*ppExec)->pNEHeader))
                                    arc = ERROR_NOT_ENOUGH_MEMORY;
                                else
                                    // read in NE header
                                    if (!(arc = DosRead(hFile,
                                                        (*ppExec)->pNEHeader,
                                                        sizeof(NEHEADER),
                                                        &((*ppExec)->cbNEHeader))))
                                        if ((*ppExec)->cbNEHeader == sizeof(NEHEADER))
                                            pbCheckOS = &((*ppExec)->pNEHeader->bTargetOS);
                            }
                            else if (   (memcmp(achNewHeaderType, "LX", 2) == 0)
                                     || (memcmp(achNewHeaderType, "LE", 2) == 0)
                                                // this is used by SMARTDRV.EXE
                                    )
                            {
                                // OS/2 Linear Executable:
                                (*ppExec)->ulExeFormat = EXEFORMAT_LX;
                                // allocate LX header
                                (*ppExec)->pLXHeader = (PLXHEADER)malloc(sizeof(LXHEADER));
                                if (!((*ppExec)->pLXHeader))
                                    arc = ERROR_NOT_ENOUGH_MEMORY;
                                else
                                    // read in LX header
                                    if (!(arc = DosRead(hFile,
                                                        (*ppExec)->pLXHeader,
                                                        sizeof(LXHEADER),
                                                        &((*ppExec)->cbLXHeader))))
                                        if ((*ppExec)->cbLXHeader == sizeof(LXHEADER))
                                            pbCheckOS = (PBYTE)(&((*ppExec)->pLXHeader->usTargetOS));
                            }
                            else if (memcmp(achNewHeaderType, "PE", 2) == 0)
                            {
                                (*ppExec)->ulExeFormat = EXEFORMAT_PE;
                                (*ppExec)->ulOS = EXEOS_WIN32;
                                (*ppExec)->f32Bits = TRUE;

                                // can't parse this yet
                            }
                            else
                                // strange type:
                                arc = ERROR_INVALID_EXE_SIGNATURE;

                            if (pbCheckOS)
                                // BYTE to check for operating system
                                // (NE and LX):
                                switch (*pbCheckOS)
                                {
                                    case NEOS_OS2:
                                        (*ppExec)->ulOS = EXEOS_OS2;
                                        if ((*ppExec)->ulExeFormat == EXEFORMAT_LX)
                                            (*ppExec)->f32Bits = TRUE;
                                    break;

                                    case NEOS_WIN16:
                                        (*ppExec)->ulOS = EXEOS_WIN16;
                                    break;

                                    case NEOS_DOS4:
                                        (*ppExec)->ulOS = EXEOS_DOS4;
                                    break;

                                    case NEOS_WIN386:
                                        (*ppExec)->ulOS = EXEOS_WIN386;
                                        (*ppExec)->f32Bits = TRUE;
                                    break;
                                }
                        } // end if (!(arc = DosSetFilePtr(hFile,
                    }
                }
            } // end if (!(arc = DosSetFilePtr(hFile,
        } // end if (*ppExec)->pDosExeHeader = (PDOSEXEHEADER)malloc(sizeof(DOSEXEHEADER));

        // store exec's HFILE
        (*ppExec)->hfExe = hFile;
    } // end if (!(arc = DosOpen((PSZ)pcszExecutable,

    if (arc != NO_ERROR)
        // error: clean up
        doshExecClose(*ppExec);

    return (arc);
}

/*
 *@@ doshExecClose:
 *      this closes an executable opened with doshExecOpen.
 *      Always call this function if NO_ERROR was returned by
 *      doshExecOpen.
 *
 *@@added V0.9.0 [umoeller]
 */

APIRET doshExecClose(PEXECUTABLE pExec)
{
    APIRET arc = NO_ERROR;
    if (pExec)
    {
        if (pExec->pDosExeHeader)
            free(pExec->pDosExeHeader);
        if (pExec->pNEHeader)
            free(pExec->pNEHeader);
        if (pExec->pLXHeader)
            free(pExec->pLXHeader);

        if (pExec->pszDescription)
            free(pExec->pszDescription);
        if (pExec->pszVendor)
            free(pExec->pszVendor);
        if (pExec->pszVersion)
            free(pExec->pszVersion);
        if (pExec->pszInfo)
            free(pExec->pszInfo);

        if (pExec->hfExe)
            arc = DosClose(pExec->hfExe);

        free(pExec);
    }
    else
        arc = ERROR_INVALID_PARAMETER;

    return (arc);
}

/*
 *@@ doshExecQueryBldLevel:
 *      this retrieves buildlevel information for an
 *      LX or NE executable previously opened with
 *      doshExecOpen.
 *
 *      BuildLevel information must be contained in the
 *      DESCRIPTION field of an executable's module
 *      definition (.DEF) file. In order to be readable
 *      by BLDLEVEL.EXE (which ships with OS/2), this
 *      string must have the following format:
 *
 +          Description '@#AUTHOR:VERSION#@ DESCRIPTION'
 *
 *      Example:
 *
 +          Description '@#Ulrich M攍ler:0.9.0#@ XWorkplace Sound Support Module'
 *
 *      The "Description" entry always ends up as the
 *      very first entry in the non-resident name table
 *      in LX and NE executables. So this is what we retrieve
 *      here.
 *
 *      If the first entry in that table exists, NO_ERROR is
 *      returned and at least the pszDescription field in
 *      EXECUTABLE is set to that information.
 *
 *      If that string is in IBM BLDLEVEL format, the string
 *      is automatically parsed, and the pszVendor, pszVersion,
 *      and pszInfo fields are also set. In the above examples,
 *      this would return the following information:
 +          pszVendor = "Ulrich M攍ler"
 +          pszVersion = "0.9.0"
 +          pszInfo = "XWorkplace Sound Support Module"
 *
 *      If that string is not in BLDLEVEL format, only pszDescription
 *      will be set. The other fields remain NULL.
 *
 *      This returns the following errors:
 *
 *      -- ERROR_INVALID_PARAMETER: pExec invalid
 *
 *      -- ERROR_INVALID_EXE_SIGNATURE (191): pExec is not in LX or NE format
 *
 *      -- ERROR_INVALID_DATA (13): non-resident name table not found.
 *
 *      -- ERROR_NOT_ENOUGH_MEMORY: malloc() failed.
 *
 *      plus the error codes of DosSetFilePtr and DosRead.
 *
 *@@added V0.9.0 [umoeller]
 *@@changed V0.9.0 (99-10-22) [umoeller]: NE format now supported
 *@@changed V0.9.1 (99-12-06): fixed memory leak
 *@@changed V0.9.9 (2001-04-04) [umoeller]: added more error checking
 */

APIRET doshExecQueryBldLevel(PEXECUTABLE pExec)
{
    APIRET      arc = NO_ERROR;

    if (!pExec)
        arc = ERROR_INVALID_PARAMETER;
    else
    {
        ULONG       ulNRNTOfs = 0;

        if (pExec->ulExeFormat == EXEFORMAT_LX)
        {
            // OK, LX format:
            // check if we have a non-resident name table
            if (pExec->pLXHeader == NULL)
                arc = ERROR_INVALID_DATA;
            else if (pExec->pLXHeader->ulNonResdNameTblOfs == 0)
                arc = ERROR_INVALID_DATA;
            else
                ulNRNTOfs = pExec->pLXHeader->ulNonResdNameTblOfs;
        }
        else if (pExec->ulExeFormat == EXEFORMAT_NE)
        {
            // OK, NE format:
            // check if we have a non-resident name table
            if (pExec->pNEHeader == NULL)
                arc = ERROR_INVALID_DATA;
            else if (pExec->pNEHeader->ulNonResdTblOfs == 0)
                arc = ERROR_INVALID_DATA;
            else
                ulNRNTOfs = pExec->pNEHeader->ulNonResdTblOfs;
        }
        else
            // neither LX nor NE: stop
            arc = ERROR_INVALID_EXE_SIGNATURE;

        if (    (!arc)
             && (ulNRNTOfs)
           )
        {
            ULONG       ulLocal = 0,
                        ulBytesRead = 0;

            // move EXE file pointer to offset of non-resident name table
            // (from LX header)
            if (!(arc = DosSetFilePtr(pExec->hfExe,     // file is still open
                                      ulNRNTOfs,      // ofs determined above
                                      FILE_BEGIN,
                                      &ulLocal)))
            {
                // allocate memory as necessary
                PSZ pszNameTable = (PSZ)malloc(2001); // should suffice, because each entry
                                                      // may only be 255 bytes in length
                if (!pszNameTable)
                    arc = ERROR_NOT_ENOUGH_MEMORY;
                else
                {
                    if (!(arc = DosRead(pExec->hfExe,
                                        pszNameTable,
                                        2000,
                                        &ulBytesRead)))
                    {
                        if (*pszNameTable == 0)
                            // first byte (length byte) is null:
                            arc = ERROR_INVALID_DATA;
                        else
                        {
                            // now copy the string, which is in Pascal format
                            pExec->pszDescription = (PSZ)malloc((*pszNameTable) + 1);    // addt'l null byte
                            if (!pExec->pszDescription)
                                arc = ERROR_NOT_ENOUGH_MEMORY;
                            else
                            {
                                const char  *pStartOfAuthor = 0,
                                            *pStartOfVendor = 0;

                                memcpy(pExec->pszDescription,
                                       pszNameTable + 1,        // skip length byte
                                       *pszNameTable);          // length byte
                                // terminate string
                                *(pExec->pszDescription + (*pszNameTable)) = 0;

                                // now parse the damn thing:
                                // @#VENDOR:VERSION#@ DESCRIPTION
                                // but skip the first byte, which has the string length
                                pStartOfVendor = strstr(pExec->pszDescription,
                                                        "@#");
                                if (pStartOfVendor)
                                {
                                    const char *pStartOfInfo = strstr(pStartOfVendor + 2,
                                                                      "#@");
                                    if (pStartOfInfo)
                                    {
                                        PSZ pEndOfVendor = strchr(pStartOfVendor + 2,
                                                                  ':');
                                        if (pEndOfVendor)
                                        {
                                            pExec->pszVendor = strhSubstr(pStartOfVendor + 2,
                                                                          pEndOfVendor);
                                            pExec->pszVersion = strhSubstr(pEndOfVendor + 1,
                                                                           pStartOfInfo);
                                            // skip "@#" in info string
                                            pStartOfInfo += 2;
                                            // skip leading spaces in info string
                                            while (*pStartOfInfo == ' ')
                                                pStartOfInfo++;
                                            if (*pStartOfInfo)  // V0.9.9 (2001-04-04) [umoeller]
                                                // and copy until end of string
                                                pExec->pszInfo = strdup(pStartOfInfo);
                                        }
                                    }
                                }
                            }
                        }
                    }

                    free(pszNameTable);
                } // end if PSZ pszNameTable = (PSZ)malloc(2001);
            }
        }
    } // end if (!pExec)

    return (arc);
}

/*
 *@@ doshExecQueryImportedModules:
 *      returns an array of FSYSMODULE structure describing all
 *      imported modules.
 *
 *      *pcModules receives the # of items in the array (not the
 *      array size!).  Use doshFreeImportedModules to clean up.
 *
 *      This returns a standard OS/2 error code, which might be
 *      any of the codes returned by DosSetFilePtr and DosRead.
 *      In addition, this may return:
 *
 *      --  ERROR_NOT_ENOUGH_MEMORY
 *
 *      --  ERROR_INVALID_EXE_SIGNATURE: exe is in a format other
 *          than LX or NE, which is not understood by this function.
 *
 *      Even if NO_ERROR is returned, the array pointer might still
 *      be NULL if the module contains no such data.
 *
 *@@added V0.9.9 (2001-03-11) [lafaix]
 *@@changed V0.9.9 (2001-04-03) [umoeller]: added tons of error checking, changed prototype to return APIRET
 */

APIRET doshExecQueryImportedModules(PEXECUTABLE pExec,
                                    PFSYSMODULE *ppaModules,    // out: modules array
                                    PULONG pcModules)           // out: array item count
{
    APIRET      arc = NO_ERROR;

    if (    (pExec)
         && (pExec->ulOS == EXEOS_OS2)
       )
    {
        ULONG       cModules = 0;
        PFSYSMODULE paModules = NULL;
        int i;

        if (pExec->ulExeFormat == EXEFORMAT_LX)
        {
            // 32-bit OS/2 executable:
            if (cModules = pExec->pLXHeader->ulImportModTblCnt)
            {
                ULONG cb = sizeof(FSYSMODULE) * cModules; // V0.9.9 (2001-04-03) [umoeller]

                paModules = (PFSYSMODULE)malloc(cb);
                if (!paModules)
                    arc = ERROR_NOT_ENOUGH_MEMORY; // V0.9.9 (2001-04-03) [umoeller]
                else
                {
                    ULONG ulDummy;

                    memset(paModules, 0, cb);   // V0.9.9 (2001-04-03) [umoeller]

                    // V0.9.9 (2001-04-03) [umoeller]:
                    // Martin, I added error checking to all the below
                    // Dos* calls. You can't just read around a file
                    // and assume it will always be valid... especially
                    // if you fill dynamically allocated memory.

                    if (!(arc = DosSetFilePtr(pExec->hfExe,
                                              pExec->pLXHeader->ulImportModTblOfs
                                                + pExec->pDosExeHeader->ulNewHeaderOfs,
                                              FILE_BEGIN,
                                              &ulDummy)))
                    {
                        for (i = 0; i < cModules; i++)
                        {
                             BYTE bLen = 0;

                             // reading the length of the module name
                             if (!(arc = DosRead(pExec->hfExe,
                                                 &bLen,
                                                 1,
                                                 &ulDummy)))
                             {
                                 // At most 127 bytes
                                 bLen &= 0x7F;

                                 // reading the module name
                                 if (!(arc = DosRead(pExec->hfExe,
                                                     paModules[i].achModuleName,
                                                     bLen,
                                                     &ulDummy)))
                                    // module names are not null terminated, so we must
                                    // do it now
                                    paModules[i].achModuleName[bLen] = 0;
                            }

                            if (arc)
                                break; // V0.9.9 (2001-04-03) [umoeller]

                        } // end for
                    }
                }
            }
        } // end LX
        else if (pExec->ulExeFormat == EXEFORMAT_NE)
        {
            // 16-bit OS/2 executable:
            if (cModules = pExec->pNEHeader->usModuleTblEntries)
            {
                ULONG cb = sizeof(FSYSMODULE) * cModules;

                paModules = (PFSYSMODULE)malloc(cb);
                if (!paModules)
                    arc = ERROR_NOT_ENOUGH_MEMORY;  // V0.9.9 (2001-04-03) [umoeller]
                else
                {
                    memset(paModules, 0, cb);   // V0.9.9 (2001-04-03) [umoeller]

                    for (i = 0; i < cModules; i ++)
                    {
                        BYTE bLen;
                        USHORT usOfs;
                        ULONG ulDummy;

                        // the module reference table contains offsets
                        // relative to the import table; we hence read
                        // the offset in the module reference table, and
                        // then we read the name in the import table

                        if (    (!(arc = DosSetFilePtr(pExec->hfExe,
                                                       pExec->pNEHeader->usModRefTblOfs
                                                         + pExec->pDosExeHeader->ulNewHeaderOfs
                                                         + sizeof(usOfs) * i,
                                                       FILE_BEGIN,
                                                       &ulDummy)))
                             && (!(arc = DosRead(pExec->hfExe,
                                                 &usOfs,
                                                 2,
                                                 &ulDummy)))
                             && (!(arc = DosSetFilePtr(pExec->hfExe,
                                                       pExec->pNEHeader->usImportTblOfs
                                                         + pExec->pDosExeHeader->ulNewHeaderOfs
                                                         + usOfs,
                                                       FILE_BEGIN,
                                                       &ulDummy)))
                             && (!(arc = DosRead(pExec->hfExe,
                                                 &bLen,
                                                 1,
                                                 &ulDummy)))
                            )
                        {
                            bLen &= 0x7F;

                            if (!(arc = DosRead(pExec->hfExe,
                                                paModules[i].achModuleName,
                                                bLen,
                                                &ulDummy)))
                                paModules[i].achModuleName[bLen] = 0;
                        }

                        if (arc)
                            break;  // V0.9.9 (2001-04-03) [umoeller]
                    } // end for
                }
            }
        }
        else
            arc = ERROR_INVALID_EXE_SIGNATURE; // V0.9.9 (2001-04-03) [umoeller]

        if (arc)
        {
            // if we had an error above, clean up
            if (paModules)
                free(paModules);
        }
        else
        {
            // no error: output data
            *ppaModules = paModules;
            *pcModules = cModules;
        }
    }
    else
        arc = ERROR_INVALID_EXE_SIGNATURE; // V0.9.9 (2001-04-03) [umoeller]

    return (arc);
}

/*
 *@@ doshExecFreeImportedModules:
 *      frees resources allocated by doshExecQueryImportedModules.
 *
 *@@added V0.9.9 (2001-03-11)
 */

APIRET doshExecFreeImportedModules(PFSYSMODULE paModules)
{
    if (paModules)          // V0.9.9 (2001-04-04) [umoeller]
        free(paModules);
    return (NO_ERROR);
}

/*
 *@@ ScanLXEntryTable:
 *      returns the number of exported entries in the entry table.
 *
 *      If paFunctions is not NULL, then successive entries are
 *      filled with the found type and ordinal values.
 *
 *@@added V0.9.9 (2001-03-30) [lafaix]
 *@@changed V0.9.9 (2001-04-03) [umoeller]: added tons of error checking, changed prototype to return APIRET
 */

APIRET ScanLXEntryTable(PEXECUTABLE pExec,
                        PFSYSFUNCTION paFunctions,
                        PULONG pcEntries)        // out: entry table entry count; ptr can be NULL
{
    APIRET arc = NO_ERROR;

    ULONG  ulDummy;

    if (!(arc = DosSetFilePtr(pExec->hfExe,
                              pExec->pLXHeader->ulEntryTblOfs
                                + pExec->pDosExeHeader->ulNewHeaderOfs,
                              FILE_BEGIN,
                              &ulDummy)))
    {
        USHORT usOrdinal = 1,
               usCurrent = 0;
        int    i;

        while (!arc)            // V0.9.9 (2001-04-03) [umoeller]
        {
            BYTE   bCnt,
                   bType,
                   bFlag;

            if (!(arc = DosRead(pExec->hfExe,
                                &bCnt,
                                1,
                                &ulDummy)))
            {
                if (bCnt == 0)
                    // end of the entry table
                    break;

                if (!(arc = DosRead(pExec->hfExe,
                                    &bType,
                                    1,
                                    &ulDummy)))
                {
                    switch (bType & 0x7F)
                    {
                        /*
                         * unused entries
                         *
                         */

                        case 0:
                           usOrdinal += bCnt;
                        break;

                        /*
                         * 16-bit entries
                         *
                         * the bundle type is followed by the object number
                         * and by bCnt bFlag+usOffset entries
                         *
                         */

                        case 1:
                            if (!(arc = DosSetFilePtr(pExec->hfExe,
                                                      sizeof(USHORT),
                                                      FILE_CURRENT,
                                                      &ulDummy)))
                            {
                                for (i = 0; i < bCnt; i ++)
                                {
                                    if (!(arc = DosRead(pExec->hfExe,
                                                        &bFlag,
                                                        1,
                                                        &ulDummy)))
                                    {
                                        if (bFlag & 0x01)
                                        {
                                            if (paFunctions)
                                            {
                                                paFunctions[usCurrent].ulOrdinal = usOrdinal;
                                                paFunctions[usCurrent].ulType = 1;
                                                paFunctions[usCurrent].achFunctionName[0] = 0;
                                            }
                                            usCurrent++;
                                        }

                                        usOrdinal++;

                                        arc = DosSetFilePtr(pExec->hfExe,
                                                            sizeof(USHORT),
                                                            FILE_CURRENT,
                                                            &ulDummy);
                                    }

                                    if (arc)
                                        break; // V0.9.9 (2001-04-03) [umoeller]

                                } // end for
                            }
                        break;

                        /*
                         * 286 call gate entries
                         *
                         * the bundle type is followed by the object number
                         * and by bCnt bFlag+usOffset+usCallGate entries
                         *
                         */

                        case 2:
                            if (!(arc = DosSetFilePtr(pExec->hfExe,
                                                      sizeof(USHORT),
                                                      FILE_CURRENT,
                                                      &ulDummy)))
                            {
                                for (i = 0; i < bCnt; i ++)
                                {
                                    if (!(arc = DosRead(pExec->hfExe,
                                                        &bFlag,
                                                        1,
                                                        &ulDummy)))
                                    {
                                        if (bFlag & 0x01)
                                        {
                                            if (paFunctions)
                                            {
                                                paFunctions[usCurrent].ulOrdinal = usOrdinal;
                                                paFunctions[usCurrent].ulType = 2;
                                                paFunctions[usCurrent].achFunctionName[0] = 0;
                                            }
                                            usCurrent++;
                                        }

                                        usOrdinal++;

                                        arc = DosSetFilePtr(pExec->hfExe,
                                                            sizeof(USHORT) + sizeof(USHORT),
                                                            FILE_CURRENT,
                                                            &ulDummy);
                                    }

                                    if (arc)
                                        break; // V0.9.9 (2001-04-03) [umoeller]

                                } // end for
                            }
                        break;

                        /*
                         * 32-bit entries
                         *
                         * the bundle type is followed by the object number
                         * and by bCnt bFlag+ulOffset entries
                         *
                         */

                        case 3:
                            if (!(arc = DosSetFilePtr(pExec->hfExe,
                                                      sizeof(USHORT),
                                                      FILE_CURRENT,
                                                      &ulDummy)))
                            {
                                for (i = 0; i < bCnt; i ++)
                                {
                                    if (!(arc = DosRead(pExec->hfExe,
                                                        &bFlag,
                                                        1,
                                                        &ulDummy)))
                                    {
                                        if (bFlag & 0x01)
                                        {
                                            if (paFunctions)
                                            {
                                                paFunctions[usCurrent].ulOrdinal = usOrdinal;
                                                paFunctions[usCurrent].ulType = 3;
                                                paFunctions[usCurrent].achFunctionName[0] = 0;
                                            }
                                            usCurrent++;
                                        }

                                        usOrdinal++;

                                        arc = DosSetFilePtr(pExec->hfExe,
                                                            sizeof(ULONG),
                                                            FILE_CURRENT,
                                                            &ulDummy);
                                    }

                                    if (arc)
                                        break; // V0.9.9 (2001-04-03) [umoeller]
                                } // end for
                            }
                        break;

                        /*
                         * forwarder entries
                         *
                         * the bundle type is followed by a reserved word
                         * and by bCnt bFlag+usModOrd+ulOffsOrdNum entries
                         *
                         */

                        case 4:
                            if (!(arc = DosSetFilePtr(pExec->hfExe,
                                                      sizeof(USHORT),
                                                      FILE_CURRENT,
                                                      &ulDummy)))
                            {
                                for (i = 0; i < bCnt; i ++)
                                {
                                    if (!(arc = DosSetFilePtr(pExec->hfExe,
                                                              sizeof(BYTE) + sizeof(USHORT) + sizeof(ULONG),
                                                              FILE_CURRENT,
                                                              &ulDummy)))
                                    {
                                        if (paFunctions)
                                        {
                                            paFunctions[usCurrent].ulOrdinal = usOrdinal;
                                            paFunctions[usCurrent].ulType = 4;
                                            paFunctions[usCurrent].achFunctionName[0] = 0;
                                        }
                                        usCurrent++;

                                        usOrdinal++;
                                    }

                                    if (arc)
                                        break; // V0.9.9 (2001-04-03) [umoeller]
                                } // end for
                            }
                        break;

                        /*
                         * unknown bundle type
                         *
                         * we don't know how to handle this bundle, so we must
                         * stop parsing the entry table here (as we don't know the
                         * bundle size); if paFunctions is not null, we fill it with
                         * informative data
                         */

                        default:
                            if (paFunctions)
                            {
                                paFunctions[usCurrent].ulOrdinal = usOrdinal;
                                paFunctions[usCurrent].ulType = bType;
                                sprintf(paFunctions[usCurrent].achFunctionName,
                                        "Unknown bundle type encountered (%d).  Aborting entry table scan.",
                                        bType);

                                arc = ERROR_INVALID_LIST_FORMAT;
                                    // whatever
                                    // V0.9.9 (2001-04-03) [umoeller]
                            }

                            usCurrent++;
                    } // end switch (bType & 0x7F)
                }
            }
        } // end while (!arc)

        if (!arc)
            if (pcEntries)
                *pcEntries = usCurrent;
    }

    return (arc);
}

/*
 *@@ ScanNEEntryTable:
 *      returns the number of exported entries in the entry table.
 *
 *      if paFunctions is not NULL, then successive entries are
 *      filled with the found type and ordinal values.
 *
 *@@added V0.9.9 (2001-03-30) [lafaix]
 *@@changed V0.9.9 (2001-04-03) [umoeller]: added tons of error checking, changed prototype to return APIRET
 */

APIRET ScanNEEntryTable(PEXECUTABLE pExec,
                        PFSYSFUNCTION paFunctions,
                        PULONG pcEntries)        // out: entry table entry count; ptr can be NULL
{
    APIRET arc = NO_ERROR;
    ULONG  ulDummy;

    if (!(arc = DosSetFilePtr(pExec->hfExe,
                              pExec->pNEHeader->usEntryTblOfs
                                + pExec->pDosExeHeader->ulNewHeaderOfs,
                              FILE_BEGIN,
                              &ulDummy)))
    {
        USHORT usOrdinal = 1,
               usCurrent = 0;
        int    i;

        while (!arc)        // V0.9.9 (2001-04-03) [umoeller]
        {
            BYTE bCnt,
                 bType,
                 bFlag;

            if (!(arc = DosRead(pExec->hfExe,
                                &bCnt,
                                1,
                                &ulDummy)))
            {
                if (bCnt == 0)
                    // end of the entry table
                    break;

                if (!(arc = DosRead(pExec->hfExe,
                                    &bType,
                                    1,
                                    &ulDummy)))
                {
                    for (i = 0; i < bCnt; i++)
                    {
                        if (!(arc = DosRead(pExec->hfExe,
                                            &bFlag,
                                            1,
                                            &ulDummy)))
                        {
                            if (bFlag & 0x01)
                            {
                                if (paFunctions)
                                {
                                    paFunctions[usCurrent].ulOrdinal = usOrdinal;
                                    paFunctions[usCurrent].ulType = 1; // 16-bit entry
                                    paFunctions[usCurrent].achFunctionName[0] = 0;
                                }
                                usCurrent++;
                            }

                            usOrdinal++;

                            if (bType == 0xFF)
                                // moveable segment
                                arc = DosSetFilePtr(pExec->hfExe,
                                                    5,
                                                    FILE_CURRENT,
                                                    &ulDummy);
                            else
                                // fixed segment
                                arc = DosSetFilePtr(pExec->hfExe,
                                                    2,
                                                    FILE_CURRENT,
                                                    &ulDummy);
                        }

                        if (arc)
                            break; // V0.9.9 (2001-04-03) [umoeller]

                    } // end for
                }
            }
        } // end while (!arc)

        if (!arc)
            if (pcEntries)
                *pcEntries = usCurrent;
    }

    return (arc);
}

/*
 *@@ Compare:
 *      binary search helper
 *
 *@@added V0.9.9 (2001-04-01) [lafaix]
 */

int _Optlink Compare(const void *key,
                     const void *element)
{
    USHORT        usOrdinal = *((PUSHORT) key);
    PFSYSFUNCTION pFunction = (PFSYSFUNCTION)element;

    if (usOrdinal > pFunction->ulOrdinal)
        return (1);
    else if (usOrdinal < pFunction->ulOrdinal)
        return (-1);
    else
        return (0);
}

/*
 *@@ ScanNameTable:
 *      scans a resident or non-resident name table, and fills the
 *      appropriate paFunctions entries when it encounters exported
 *      entries names.
 *
 *      This functions works for both NE and LX executables.
 *
 *@@added V0.9.9 (2001-03-30) [lafaix]
 *@@changed V0.9.9 (2001-04-02) [lafaix]: the first entry is special
 *@@changed V0.9.9 (2001-04-03) [umoeller]: added tons of error checking, changed prototype to return APIRET
 */

APIRET ScanNameTable(PEXECUTABLE pExec,
                     ULONG cFunctions,
                     PFSYSFUNCTION paFunctions)
{
    APIRET  arc = NO_ERROR;
    ULONG   ulDummy;

    USHORT        usOrdinal;
    PFSYSFUNCTION pFunction;
    BOOL          bFirst = TRUE;

    while (!arc)        // V0.9.9 (2001-04-03) [umoeller]
    {
        BYTE   bLen;
        CHAR   achName[256];
        int    i;

        if (!(arc = DosRead(pExec->hfExe,
                            &bLen,
                            1,
                            &ulDummy)))
        {
            if (bLen == 0)
                // end of the name table
                break;

            // the LX docs says that len is limited to 127 (the 8th bit being
            // reserved for future use); but does this applies to 16bits
            // tables too?
            // in any case, we must skip the first entry (module name in the
            // resident name table, and module description in non-resident
            // name table)
            if (bFirst)
                bFirst = FALSE;
            else
                bLen &= 0x7F;

            if (!(arc = DosRead(pExec->hfExe,
                                &achName,
                                bLen,
                                &ulDummy)))
            {
                achName[bLen] = 0;

                if (!(arc = DosRead(pExec->hfExe,
                                    &usOrdinal,
                                    sizeof(USHORT),
                                    &ulDummy)))
                {
                    if ((pFunction = (PFSYSFUNCTION)bsearch(&usOrdinal,
                                                            paFunctions,
                                                            cFunctions,
                                                            sizeof(FSYSFUNCTION),
                                                            Compare)))
                    {
                        memcpy(pFunction->achFunctionName,
                               achName,
                               bLen+1);
                    }
                }
            }
        }
    }

    return (arc);
}

/*
 *@@ doshExecQueryExportedFunctions:
 *      returns an array of FSYSFUNCTION structure describing all
 *      exported functions.
 *
 *      *pcFunctions receives the # of items in the array (not the
 *      array size!).  Use doshFreeExportedFunctions to clean up.
 *
 *      Note that the returned array only contains entry for exported
 *      functions.  Empty export entries are _not_ included.
 *
 *      This returns a standard OS/2 error code, which might be
 *      any of the codes returned by DosSetFilePtr and DosRead.
 *      In addition, this may return:
 *
 *      --  ERROR_NOT_ENOUGH_MEMORY
 *
 *      --  ERROR_INVALID_EXE_SIGNATURE: exe is in a format other
 *          than LX or NE, which is not understood by this function.
 *
 *      --  If ERROR_INVALID_LIST_FORMAT is returned, the format of an
 *          export entry wasn't understood here.
 *
 *      Even if NO_ERROR is returned, the array pointer might still
 *      be NULL if the module contains no such data.
 *
 *@@added V0.9.9 (2001-03-11) [lafaix]
 *@@changed V0.9.9 (2001-04-03) [umoeller]: added tons of error checking, changed prototype to return APIRET
 */

APIRET doshExecQueryExportedFunctions(PEXECUTABLE pExec,
                                      PFSYSFUNCTION *ppaFunctions,  // out: functions array
                                      PULONG pcFunctions)           // out: array item count
{
    APIRET arc = NO_ERROR;

    if (    (pExec)
         && (pExec->ulOS == EXEOS_OS2)
       )
    {
        ULONG         cFunctions = 0;
        PFSYSFUNCTION paFunctions = NULL;

        ULONG ulDummy;

        if (pExec->ulExeFormat == EXEFORMAT_LX)
        {
            // It's a 32bit OS/2 executable

            // the number of exported entry points is not stored
            // in the executable header; we have to count them in
            // the entry table

            if (!(arc = ScanLXEntryTable(pExec,
                                         NULL,
                                         &cFunctions)))
            {
                // we now have the number of exported entries; let us
                // build them

                if (cFunctions)
                {
                    ULONG cb = sizeof(FSYSFUNCTION) * cFunctions;

                    paFunctions = (PFSYSFUNCTION)malloc(cb);
                    if (!paFunctions)
                        arc = ERROR_NOT_ENOUGH_MEMORY;  // V0.9.9 (2001-04-03) [umoeller]
                    else
                    {
                        // we rescan the entry table (the cost is not as bad
                        // as it may seem, due to disk caching)

                        if (    (!(arc = ScanLXEntryTable(pExec, paFunctions, NULL)))
                                // we now scan the resident name table entries
                             && (!(arc = DosSetFilePtr(pExec->hfExe,
                                                       pExec->pLXHeader->ulResdNameTblOfs
                                                         + pExec->pDosExeHeader->ulNewHeaderOfs,
                                                       FILE_BEGIN,
                                                       &ulDummy)))
                             && (!(arc = ScanNameTable(pExec, cFunctions, paFunctions)))
                                // we now scan the non-resident name table entries,
                                // whose offset is _from the begining of the file_
                             && (!(arc = DosSetFilePtr(pExec->hfExe,
                                                       pExec->pLXHeader->ulNonResdNameTblOfs,
                                                       FILE_BEGIN,
                                                       &ulDummy)))
                           )
                        {
                            arc = ScanNameTable(pExec, cFunctions, paFunctions);
                        }
                    }
                } // end if (cFunctions)
            }
        }
        else if (pExec->ulExeFormat == EXEFORMAT_NE)
        {
            // It's a 16bit OS/2 executable

            // here too the number of exported entry points
            // is not stored in the executable header; we
            // have to count them in the entry table

            if (!(arc = ScanNEEntryTable(pExec,
                                         NULL,
                                         &cFunctions)))
            {
                // we now have the number of exported entries; let us
                // build them

                if (cFunctions)
                {
                    USHORT usOrdinal = 1,
                           usCurrent = 0;

                    paFunctions = (PFSYSFUNCTION)malloc(sizeof(FSYSFUNCTION) * cFunctions);
                    if (!paFunctions)
                        arc = ERROR_NOT_ENOUGH_MEMORY;
                    else
                    {
                        // we rescan the entry table (the cost is not as bad
                        // as it may seem, due to disk caching)

                        if (    (!(arc = ScanNEEntryTable(pExec, paFunctions, NULL)))
                                // we now scan the resident name table entries
                             && (!(arc = DosSetFilePtr(pExec->hfExe,
                                                       pExec->pNEHeader->usResdNameTblOfs
                                                         + pExec->pDosExeHeader->ulNewHeaderOfs,
                                                       FILE_BEGIN,
                                                       &ulDummy)))
                             && (!(arc = ScanNameTable(pExec, cFunctions, paFunctions)))
                                // we now scan the non-resident name table entries,
                                // whose offset is _from the begining of the file_
                             && (!(arc = DosSetFilePtr(pExec->hfExe,
                                                       pExec->pNEHeader->ulNonResdTblOfs,
                                                       FILE_BEGIN,
                                                       &ulDummy)))
                           )
                        {
                             arc = ScanNameTable(pExec, cFunctions, paFunctions);
                        }
                    }
                }
            }
        }
        else
            arc = ERROR_INVALID_EXE_SIGNATURE; // V0.9.9 (2001-04-03) [umoeller]

        if (arc) // V0.9.9 (2001-04-03) [umoeller]
        {
            // if we had an error above, clean up
            if (paFunctions)
                free(paFunctions);
        }
        else
        {
            // no error: output data
            *ppaFunctions = paFunctions;
            *pcFunctions = cFunctions;
        }
    }
    else
        arc = ERROR_INVALID_EXE_SIGNATURE; // V0.9.9 (2001-04-03) [umoeller]

    return (arc);
}

/*
 *@@ doshExecFreeExportedFunctions:
 *      frees resources allocated by doshExecQueryExportedFunctions.
 *
 *@@added V0.9.9 (2001-03-11)
 */

APIRET doshExecFreeExportedFunctions(PFSYSFUNCTION paFunctions)
{
    if (paFunctions)       // V0.9.9 (2001-04-04) [umoeller]
        free(paFunctions);
    return (NO_ERROR);
}

/*
 *@@ doshExecQueryResources:
 *      returns an array of FSYSRESOURCE structures describing all
 *      available resources in the module.
 *
 *      *pcResources receives the no. of items in the array
 *      (not the array size!). Use doshExecFreeResources to clean up.
 *
 *      This returns a standard OS/2 error code, which might be
 *      any of the codes returned by DosSetFilePtr and DosRead.
 *      In addition, this may return:
 *
 *      --  ERROR_NOT_ENOUGH_MEMORY
 *
 *      --  ERROR_INVALID_EXE_SIGNATURE: exe is in a format other
 *          than LX or NE, which is not understood by this function.
 *
 *      Even if NO_ERROR is returned, the array pointer might still
 *      be NULL if the module contains no such data.
 *
 *@@added V0.9.7 (2000-12-18) [lafaix]
 *@@changed V0.9.9 (2001-04-03) [umoeller]: added tons of error checking, changed prototype to return APIRET
 */

APIRET doshExecQueryResources(PEXECUTABLE pExec,     // in: executable from doshExecOpen
                              PFSYSRESOURCE *ppaResources,   // out: res's array
                              PULONG pcResources)    // out: array item count
{
    APIRET          arc = NO_ERROR;

    if (    (pExec)
         && (pExec->ulOS == EXEOS_OS2)
       )
    {
        ULONG           cResources = 0;
        PFSYSRESOURCE   paResources = NULL;

        if (pExec->ulExeFormat == EXEFORMAT_LX)
        {
            // 32-bit OS/2 executable:
            if (cResources = pExec->pLXHeader->ulResTblCnt)
            {
                ULONG cb = sizeof(FSYSRESOURCE) * cResources;
                paResources = (PFSYSRESOURCE)malloc(cb);
                if (!paResources)
                    arc = ERROR_NOT_ENOUGH_MEMORY;
                else
                {
                    ULONG ulDummy;

                    memset(paResources, 0, cb); // V0.9.9 (2001-04-03) [umoeller]

                    // V0.9.9 (2001-04-03) [umoeller]:
                    // Martin, I added error checking to all the below
                    // Dos* calls. You can't just read around a file
                    // and assume it will always be valid... especially
                    // if you fill dynamically allocated memory.
                    if (!(arc = DosSetFilePtr(pExec->hfExe,
                                              pExec->pLXHeader->ulResTblOfs
                                                + pExec->pDosExeHeader->ulNewHeaderOfs,
                                              FILE_BEGIN,
                                              &ulDummy)))
                    {
                        int i;

                        // V0.9.9 (2001-04-03) [umoeller]:
                        // Besides, packing was missing.
                        #pragma pack(1)     // V0.9.9 (2001-04-02) [umoeller]
                        struct rsrc32               /* Resource Table Entry */
                        {
                            unsigned short  type;   /* Resource type */
                            unsigned short  name;   /* Resource name */
                            unsigned long   cb;     /* Resource size */
                            unsigned short  obj;    /* Object number */
                            unsigned long   offset; /* Offset within object */
                        } rs;

                        struct o32_obj                    /* Flat .EXE object table entry */
                        {
                            unsigned long   o32_size;     /* Object virtual size */
                            unsigned long   o32_base;     /* Object base virtual address */
                            unsigned long   o32_flags;    /* Attribute flags */
                            unsigned long   o32_pagemap;  /* Object page map index */
                            unsigned long   o32_mapsize;  /* Number of entries in object page map */
                            unsigned long   o32_reserved; /* Reserved */
                        } ot;
                        #pragma pack() // V0.9.9 (2001-04-03) [umoeller]

                        for (i = 0; i < cResources; i++)
                        {
                            arc = DosRead(pExec->hfExe,
                                          &rs,
                                          14,
                                          &ulDummy);
                            if (arc)
                                break;      // V0.9.9 (2001-04-03) [umoeller]
                            else
                            {
                                paResources[i].ulID = rs.name;
                                paResources[i].ulType = rs.type;
                                paResources[i].ulSize = rs.cb;
                                paResources[i].ulFlag = rs.obj; // Temp storage for Object
                                                                // number.  Will be filled
                                                                // with resource flag
                                                                // later.
                            }
                        }

                        if (!arc)       // V0.9.9 (2001-04-03) [umoeller]
                        {
                            for (i = 0; i < cResources; i++)
                            {
                                ULONG ulOfsThis
                                    =   pExec->pLXHeader->ulObjTblOfs
                                      + pExec->pDosExeHeader->ulNewHeaderOfs
                                      + (   sizeof(ot)
                                          * (paResources[i].ulFlag - 1));

                                if (!(arc = DosSetFilePtr(pExec->hfExe,
                                                          ulOfsThis,
                                                          FILE_BEGIN,
                                                          &ulDummy)))
                                {
                                    if (!(arc = DosRead(pExec->hfExe,
                                                        &ot,
                                                        sizeof(ot),
                                                        &ulDummy)))
                                    {
                                        paResources[i].ulFlag  = ((ot.o32_flags & OBJWRITE)
                                                                        ? 0
                                                                        : RNPURE);
                                        paResources[i].ulFlag |= ((ot.o32_flags & OBJDISCARD)
                                                                        ? 4096
                                                                        : 0);
                                        paResources[i].ulFlag |= ((ot.o32_flags & OBJSHARED)
                                                                        ? RNMOVE
                                                                        : 0);
                                        paResources[i].ulFlag |= ((ot.o32_flags & OBJPRELOAD)
                                                                        ? RNPRELOAD
                                                                        : 0);
                                    }
                                }

                                if (arc)
                                    break; // V0.9.9 (2001-04-03) [umoeller]
                            } // end for
                        }
                    } // end if !DosSetFilePtr(pExec->hfExe,
                } // end if paResources = (PFSYSRESOURCE)malloc(sizeof(FSYSRESOURCE) * cResources);
            } // end if (cResources)
        } // end if (pExec->ulExeFormat == EXEFORMAT_LX)
        else if (pExec->ulExeFormat == EXEFORMAT_NE)
        {
            // 16-bit OS/2 executable:
            if (cResources = pExec->pNEHeader->usResSegmCount)
            {
                #pragma pack(1)     // V0.9.9 (2001-04-02) [umoeller]
                struct {unsigned short type; unsigned short name;} rti;
                struct new_seg                          /* New .EXE segment table entry */
                {
                    unsigned short      ns_sector;      /* File sector of start of segment */
                    unsigned short      ns_cbseg;       /* Number of bytes in file */
                    unsigned short      ns_flags;       /* Attribute flags */
                    unsigned short      ns_minalloc;    /* Minimum allocation in bytes */
                } ns;
                #pragma pack()

                ULONG cb = sizeof(FSYSRESOURCE) * cResources;

                paResources = (PFSYSRESOURCE)malloc(cb);
                if (!paResources)
                    arc = ERROR_NOT_ENOUGH_MEMORY;
                else
                {
                    ULONG ulDummy;

                    memset(paResources, 0, cb);     // V0.9.9 (2001-04-03) [umoeller]

                    // we first read the resources IDs and types
                    if (!(arc = DosSetFilePtr(pExec->hfExe,
                                              pExec->pNEHeader->usResTblOfs
                                                 + pExec->pDosExeHeader->ulNewHeaderOfs,
                                              FILE_BEGIN,
                                              &ulDummy)))
                    {
                        int i;

                        for (i = 0; i < cResources; i++)
                        {
                            arc = DosRead(pExec->hfExe, &rti, sizeof(rti), &ulDummy);
                            if (arc)
                                break;
                            else
                            {
                                paResources[i].ulID = rti.name;
                                paResources[i].ulType = rti.type;
                            }
                        }

                        if (!arc)
                        {
                            // we then read their sizes and flags
                            for (i = 0; i < cResources; i++)
                            {
                                if (!(arc = DosSetFilePtr(pExec->hfExe,
                                                          pExec->pDosExeHeader->ulNewHeaderOfs
                                                                 + pExec->pNEHeader->usSegTblOfs
                                                                 + (sizeof(ns)
                                                                     * (  pExec->pNEHeader->usSegTblEntries
                                                                        - pExec->pNEHeader->usResSegmCount
                                                                        + i)),
                                                          FILE_BEGIN,
                                                          &ulDummy)))
                                {
                                    if (!(arc = DosRead(pExec->hfExe,
                                                        &ns,
                                                        sizeof(ns),
                                                        &ulDummy)))
                                    {
                                        paResources[i].ulSize = ns.ns_cbseg;

                                        paResources[i].ulFlag  = (ns.ns_flags & OBJPRELOAD) ? RNPRELOAD : 0;
                                        paResources[i].ulFlag |= (ns.ns_flags & OBJSHARED) ? RNPURE : 0;
                                        paResources[i].ulFlag |= (ns.ns_flags & OBJDISCARD) ? RNMOVE : 0;
                                        paResources[i].ulFlag |= (ns.ns_flags & OBJDISCARD) ? 4096 : 0;
                                    }
                                }

                                if (arc)
                                    break; // V0.9.9 (2001-04-04) [umoeller]
                            } // end for
                        }
                    } // end if !arc = DosSetFilePtr(pExec->hfExe,
                } // end if paResources = (PFSYSRESOURCE)malloc(sizeof(FSYSRESOURCE) * cResources);
            } // end if (cResources)
        } // end else if (pExec->ulExeFormat == EXEFORMAT_NE)
        else
            arc = ERROR_INVALID_EXE_SIGNATURE; // V0.9.9 (2001-04-03) [umoeller]

        if (arc) // V0.9.9 (2001-04-03) [umoeller]
        {
            // if we had an error above, clean up
            if (paResources)
                free(paResources);
        }
        else
        {
            // no error: output data
            *ppaResources = paResources;
            *pcResources = cResources;
        }
    }
    else
        arc = ERROR_INVALID_EXE_SIGNATURE; // V0.9.9 (2001-04-03) [umoeller]

    return (arc);
}

/*
 *@@ doshExecFreeResources:
 *      frees resources allocated by doshExecQueryResources.
 *
 *@@added V0.9.7 (2000-12-18) [lafaix]
 */

APIRET doshExecFreeResources(PFSYSRESOURCE paResources)
{
    if (paResources)        // V0.9.9 (2001-04-04) [umoeller]
        free(paResources);
    return (NO_ERROR);
}

/*
 *@@category: Helpers\Control program helpers\Partitions info
 *      functions for retrieving partition information directly
 *      from the partition tables on the disk. See doshGetPartitionsList.
 */

/********************************************************************
 *
 *   Partition functions
 *
 ********************************************************************/

/*
 *@@ doshQueryDiskCount:
 *      returns the no. of physical disks installed
 *      on the system.
 *
 *      Based on code (C) Dmitry A. Steklenev.
 *
 *@@added V0.9.0 [umoeller]
 */

UINT doshQueryDiskCount(VOID)
{
    USHORT count = 0;

    DosPhysicalDisk(INFO_COUNT_PARTITIONABLE_DISKS, &count, 2, 0, 0);
    return (count);
}

/*
 *@@ doshReadSector:
 *      reads a physical disk sector.
 *
 *      If NO_ERROR is returned, the sector contents
 *      have been stored in *buff.
 *
 *      Based on code (C) Dmitry A. Steklenev.
 *
 *@@added V0.9.0 [umoeller]
 *@@changed V0.9.9 (2001-04-04) [umoeller]: added more error checking
 */

APIRET doshReadSector(USHORT disk,      // in: physical disk no. (1, 2, 3, ...)
                      void *buff,
                      USHORT head,
                      USHORT cylinder,
                      USHORT sector)
{
    APIRET  arc;
    HFILE   dh = 0;
    char    dn[256];

    sprintf(dn, "%u:", disk);
    if (!(arc = DosPhysicalDisk(INFO_GETIOCTLHANDLE, &dh, 2, dn, 3)))
    {
        TRACKLAYOUT DiskIOParm;
        ULONG IOCtlDataLength = sizeof(DiskIOParm);
        ULONG IOCtlParmLength = 512;

        DiskIOParm.bCommand = 0;
        DiskIOParm.usHead = head;
        DiskIOParm.usCylinder = cylinder;
        DiskIOParm.usFirstSector = 0;
        DiskIOParm.cSectors = 1;
        DiskIOParm.TrackTable[0].usSectorNumber = sector;
        DiskIOParm.TrackTable[0].usSectorSize = 512;

        arc = DosDevIOCtl(dh,
                          IOCTL_PHYSICALDISK, PDSK_READPHYSTRACK,
                          &DiskIOParm, IOCtlParmLength, &IOCtlParmLength,
                          buff       , IOCtlDataLength, &IOCtlDataLength);

        DosPhysicalDisk(INFO_FREEIOCTLHANDLE, 0, 0, &dh, 2);
    }

    return (arc);
}

/*
 *@@ doshType2FSName:
 *      this returns a static, zero-terminated string
 *      for the given FS type. This is always 7 bytes
 *      in length.
 *
 *      Values for operating system indicator:
 *      --  00h  empty
 *      --  01h  DOS 12-bit FAT
 *      --  02h  XENIX root file system
 *      --  03h  XENIX /usr file system (obsolete)
 *      --  04h  DOS 16-bit FAT (up to 32M)
 *      --  05h  DOS 3.3+ extended partition
 *      --  06h  DOS 3.31+ Large File System (16-bit FAT, over 32M)
 *      --  07h  QNX
 *      --  07h  OS/2 HPFS
 *      --  07h  Windows NT NTFS
 *      --  07h  Advanced Unix
 *      --  08h  OS/2 (v1.0-1.3 only)
 *      --  08h  AIX bootable partition, SplitDrive
 *      --  08h  Commodore DOS
 *      --  08h  DELL partition spanning multiple drives
 *      --  09h  AIX data partition
 *      --  09h  Coherent filesystem
 *      --  0Ah  OS/2 Boot Manager
 *      --  0Ah  OPUS
 *      --  0Ah  Coherent swap partition
 *      --  0Bh  Windows95 with 32-bit FAT
 *      --  0Ch  Windows95 with 32-bit FAT (using LBA-mode INT 13 extensions)
 *      --  0Eh  logical-block-addressable VFAT (same as 06h but using LBA-mode INT 13)
 *      --  0Fh  logical-block-addressable VFAT (same as 05h but using LBA-mode INT 13)
 *      --  10h  OPUS
 *      --  11h  OS/2 Boot Manager hidden 12-bit FAT partition
 *      --  12h  Compaq Diagnostics partition
 *      --  14h  (resulted from using Novell DOS 7.0 FDISK to delete Linux Native part)
 *      --  14h  OS/2 Boot Manager hidden sub-32M 16-bit FAT partition
 *      --  16h  OS/2 Boot Manager hidden over-32M 16-bit FAT partition
 *      --  17h  OS/2 Boot Manager hidden HPFS partition
 *      --  18h  AST special Windows swap file ("Zero-Volt Suspend" partition)
 *      --  21h  officially listed as reserved
 *      --  23h  officially listed as reserved
 *      --  24h  NEC MS-DOS 3.x
 *      --  26h  officially listed as reserved
 *      --  31h  officially listed as reserved
 *      --  33h  officially listed as reserved
 *      --  34h  officially listed as reserved
 *      --  36h  officially listed as reserved
 *      --  38h  Theos
 *      --  3Ch  PowerQuest PartitionMagic recovery partition
 *      --  40h  VENIX 80286
 *      --  41h  Personal RISC Boot
 *      --  42h  SFS (Secure File System) by Peter Gutmann
 *      --  50h  OnTrack Disk Manager, read-only partition
 *      --  51h  OnTrack Disk Manager, read/write partition
 *      --  51h  NOVEL
 *      --  52h  CP/M
 *      --  52h  Microport System V/386
 *      --  53h  OnTrack Disk Manager, write-only partition???
 *      --  54h  OnTrack Disk Manager (DDO)
 *      --  56h  GoldenBow VFeature
 *      --  61h  SpeedStor
 *      --  63h  Unix SysV/386, 386/ix
 *      --  63h  Mach, MtXinu BSD 4.3 on Mach
 *      --  63h  GNU HURD
 *      --  64h  Novell NetWare 286
 *      --  65h  Novell NetWare (3.11)
 *      --  67h  Novell
 *      --  68h  Novell
 *      --  69h  Novell
 *      --  70h  DiskSecure Multi-Boot
 *      --  71h  officially listed as reserved
 *      --  73h  officially listed as reserved
 *      --  74h  officially listed as reserved
 *      --  75h  PC/IX
 *      --  76h  officially listed as reserved
 *      --  80h  Minix v1.1 - 1.4a
 *      --  81h  Minix v1.4b+
 *      --  81h  Linux
 *      --  81h  Mitac Advanced Disk Manager
 *      --  82h  Linux Swap partition
 *      --  82h  Prime
 *      --  83h  Linux native file system (ext2fs/xiafs)
 *      --  84h  OS/2-renumbered type 04h partition (related to hiding DOS C: drive)
 *      --  86h  FAT16 volume/stripe set (Windows NT)
 *      --  87h  HPFS Fault-Tolerant mirrored partition
 *      --  87h  NTFS volume/stripe set
 *      --  93h  Amoeba file system
 *      --  94h  Amoeba bad block table
 *      --  A0h  Phoenix NoteBIOS Power Management "Save-to-Disk" partition
 *      --  A1h  officially listed as reserved
 *      --  A3h  officially listed as reserved
 *      --  A4h  officially listed as reserved
 *      --  A5h  FreeBSD, BSD/386
 *      --  A6h  officially listed as reserved
 *      --  B1h  officially listed as reserved
 *      --  B3h  officially listed as reserved
 *      --  B4h  officially listed as reserved
 *      --  B6h  officially listed as reserved
 *      --  B7h  BSDI file system (secondarily swap)
 *      --  B8h  BSDI swap partition (secondarily file system)
 *      --  C1h  DR DOS 6.0 LOGIN.EXE-secured 12-bit FAT partition
 *      --  C4h  DR DOS 6.0 LOGIN.EXE-secured 16-bit FAT partition
 *      --  C6h  DR DOS 6.0 LOGIN.EXE-secured Huge partition
 *      --  C6h  corrupted FAT16 volume/stripe set (Windows NT)
 *      --  C7h  Syrinx Boot
 *      --  C7h  corrupted NTFS volume/stripe set
 *      --  D8h  CP/M-86
 *      --  DBh  CP/M, Concurrent CP/M, Concurrent DOS
 *      --  DBh  CTOS (Convergent Technologies OS)
 *      --  E1h  SpeedStor 12-bit FAT extended partition
 *      --  E3h  DOS read-only
 *      --  E3h  Storage Dimensions
 *      --  E4h  SpeedStor 16-bit FAT extended partition
 *      --  E5h  officially listed as reserved
 *      --  E6h  officially listed as reserved
 *      --  F1h  Storage Dimensions
 *      --  F2h  DOS 3.3+ secondary partition
 *      --  F3h  officially listed as reserved
 *      --  F4h  SpeedStor
 *      --  F4h  Storage Dimensions
 *      --  F6h  officially listed as reserved
 *      --  FEh  LANstep
 *      --  FEh  IBM PS/2 IML
 *      --  FFh  Xenix bad block table
 *
 *      Note: for partition type 07h, one should inspect the partition boot record
 *            for the actual file system type
 *
 *      Based on code (C) Dmitry A. Steklenev.
 *
 *@@added V0.9.0 [umoeller]
 */

const char* doshType2FSName(unsigned char bFSType)  // in: FS type
{
    PSZ zFSName = NULL;

    switch (bFSType)
    {
        case PAR_UNUSED:
            zFSName = "UNUSED ";
            break;
        case PAR_FAT12SMALL:
            zFSName = "FAT-12 ";
            break;
        case PAR_XENIXROOT:
            zFSName = "XENIX  ";
            break;
        case PAR_XENIXUSER:
            zFSName = "XENIX  ";
            break;
        case PAR_FAT16SMALL:
            zFSName = "FAT-16 ";
            break;
        case PAR_EXTENDED:
            zFSName = "EXTEND ";
            break;
        case PAR_FAT16BIG:
            zFSName = "BIGDOS ";
            break;
        case PAR_HPFS:
            zFSName = "HPFS   ";
            break;
        case PAR_AIXBOOT:
            zFSName = "AIX    ";
            break;
        case PAR_AIXDATA:
            zFSName = "AIX    ";
            break;
        case PAR_BOOTMANAGER:
            zFSName = "BOOTMNG";
            break;
        case PAR_WINDOWS95:
            zFSName = "WIN95  ";
            break;
        case PAR_WINDOWS95LB:
            zFSName = "WIN95  ";
            break;
        case PAR_VFAT16BIG:
            zFSName = "VFAT   ";
            break;
        case PAR_VFAT16EXT:
            zFSName = "VFAT   ";
            break;
        case PAR_OPUS:
            zFSName = "OPUS   ";
            break;
        case PAR_HID12SMALL:
            zFSName = "FAT-12*";
            break;
        case PAR_COMPAQDIAG:
            zFSName = "COMPAQ ";
            break;
        case PAR_HID16SMALL:
            zFSName = "FAT-16*";
            break;
        case PAR_HID16BIG:
            zFSName = "BIGDOS*";
            break;
        case PAR_HIDHPFS:
            zFSName = "HPFS*  ";
            break;
        case PAR_WINDOWSSWP:
            zFSName = "WINSWAP";
            break;
        case PAR_NECDOS:
            zFSName = "NECDOS ";
            break;
        case PAR_THEOS:
            zFSName = "THEOS  ";
            break;
        case PAR_VENIX:
            zFSName = "VENIX  ";
            break;
        case PAR_RISCBOOT:
            zFSName = "RISC   ";
            break;
        case PAR_SFS:
            zFSName = "SFS    ";
            break;
        case PAR_ONTRACK:
            zFSName = "ONTRACK";
            break;
        case PAR_ONTRACKEXT:
            zFSName = "ONTRACK";
            break;
        case PAR_CPM:
            zFSName = "CP/M   ";
            break;
        case PAR_UNIXSYSV:
            zFSName = "UNIX   ";
            break;
        case PAR_NOVELL_64:
            zFSName = "NOVELL ";
            break;
        case PAR_NOVELL_65:
            zFSName = "NOVELL ";
            break;
        case PAR_NOVELL_67:
            zFSName = "NOVELL ";
            break;
        case PAR_NOVELL_68:
            zFSName = "NOVELL ";
            break;
        case PAR_NOVELL_69:
            zFSName = "NOVELL ";
            break;
        case PAR_PCIX:
            zFSName = "PCIX   ";
            break;
        case PAR_MINIX:
            zFSName = "MINIX  ";
            break;
        case PAR_LINUX:
            zFSName = "LINUX  ";
            break;
        case PAR_LINUXSWAP:
            zFSName = "LNXSWP ";
            break;
        case PAR_LINUXFILE:
            zFSName = "LINUX  ";
            break;
        case PAR_FREEBSD:
            zFSName = "FREEBSD";
            break;
        case PAR_BBT:
            zFSName = "BBT    ";
            break;

        default:
            zFSName = "       ";
            break;
    }
    return zFSName;
}

/*
 * AppendPartition:
 *      this appends the given partition information to
 *      the given partition list. To do this, a new
 *      PARTITIONINFO structure is created and appended
 *      in a list (managed thru the PARTITIONINFO.pNext
 *      items).
 *
 *      pppiThis must be a pointer to a pointer to a PARTITIONINFO.
 *      With each call of this function, this pointer is advanced
 *      to point to the newly created PARTITIONINFO, so before
 *      calling this function for the first time,
 *
 *      Based on code (C) Dmitry A. Steklenev.
 *
 *@@added V0.9.0 [umoeller]
 */

APIRET AppendPartition(PARTITIONINFO **pppiFirst,
                       PARTITIONINFO **pppiThis,    // in/out: partition info; pointer will be advanced
                       PUSHORT posCount,            // in/out: partition count
                       BYTE bDisk,                  // in: disk of partition
                       char *pszBootName,           // in: boot partition name
                       CHAR cLetter,                // in/out: drive letter
                       BYTE bFsType,                // in: file system type
                       BOOL fPrimary,               // in: primary?
                       BOOL fBootable,
                       ULONG ulSectors)             // in: no. of sectors
{
    APIRET arc = NO_ERROR;
    PPARTITIONINFO ppiNew = (PPARTITIONINFO)malloc(sizeof(PARTITIONINFO));
    if (ppiNew)
    {
        // store data
        ppiNew->bDisk = bDisk;
        if (fBootable)
        {
            memcpy(ppiNew->szBootName, pszBootName, 8);
            ppiNew->szBootName[8] = 0;
        }
        else
            ppiNew->szBootName[0] = 0;
        ppiNew->cLetter = cLetter;
        ppiNew->bFSType = bFsType;
        strcpy(ppiNew->szFSType,
               doshType2FSName(bFsType));
        ppiNew->fPrimary = fPrimary;
        ppiNew->fBootable = fBootable;
        ppiNew->ulSize = ulSectors / 2048;

        ppiNew->pNext = NULL;

        (*posCount)++;

        if (*pppiFirst == (PPARTITIONINFO)NULL)
        {
            // first call:
            *pppiFirst = ppiNew;
            *pppiThis = ppiNew;
        }
        else
        {
            // append to list
            (**pppiThis).pNext = ppiNew;
            *pppiThis = ppiNew;
        }
    }
    else
        arc = ERROR_NOT_ENOUGH_MEMORY;

    return (arc);
}

// Sector and Cylinder values are actually 6 bits and 10 bits:
//
//   1 1 1 1 1 1
//  󵕚򪹀򪑦򬯎򬇴򫠚򪹀�1履�
//  砪 c c c c c c c C c S s s s s s�
//  滥聊聊聊聊聊聊聊聊聊聊聊聊聊聊聊�
//
// The high two bits of the second byte are used as the high bits
// of a 10-bit value.  This allows for as many as 1024 cylinders
// and 64 sectors per cylinder.

/*
 * GetCyl:
 *      get cylinder number.
 *
 *      Based on code (C) Dmitry A. Steklenev.
 *
 *@@added V0.9.0 [umoeller]
 */

static USHORT GetCyl(USHORT rBeginSecCyl)
{
    return (   (rBeginSecCyl & 0x00C0) << 2)
             + ((rBeginSecCyl & 0xFF00) >> 8);
}

/*
 * GetSec:
 *      get sector number.
 *
 *      Based on code (C) Dmitry A. Steklenev.
 *
 *@@added V0.9.0 [umoeller]
 */

static USHORT GetSec(USHORT rBeginSecCyl)
{
    return rBeginSecCyl & 0x003F;
}

/*
 *@@ doshGetBootManager:
 *      this goes thru the master boot records on all
 *      disks to find the boot manager partitions.
 *
 *      Returns:
 *      -- NO_ERROR: boot manager found; in that case,
 *                   information about the boot manager
 *                   is written into *pusDisk, *pusPart,
 *                   *BmInfo. Any of these pointers can
 *                   be NULL if you're not interested.
 *      -- ERROR_NOT_SUPPORTED (50): boot manager not installed.
 *
 *      Based on code (C) Dmitry A. Steklenev.
 *
 *@@added V0.9.0 [umoeller]
 */

APIRET doshGetBootManager(USHORT   *pusDisk,    // out: if != NULL, boot manager disk (1, 2, ...)
                          USHORT   *pusPart,    // out: if != NULL, index of bmgr primary partition (0-3)
                          PAR_INFO *BmInfo)     // out: if != NULL, boot manager partition info
{
    APIRET          arc = NO_ERROR;
    USHORT          count = doshQueryDiskCount();    // Physical disk number
    MBR_INFO        MBoot;      // Master Boot
    USHORT          usDisk;

    if (count > 8)              // Not above 8 disks
        count = 8;

    for (usDisk = 1; usDisk <= count; usDisk++)
    {
        USHORT usPrim = 0;

        // for each disk, read the MBR, which has the
        // primary partitions
        if ((arc = doshReadSector(usDisk, &MBoot,
                                  0, 0, 1)))
            return (arc);

        // Lookup BootManager partition
        for (usPrim = 0; usPrim < 4; usPrim++)
        {
            if (MBoot.sPrtnInfo[usPrim].bFileSysCode == 0x0A)
            {
                if (BmInfo)
                    *BmInfo = MBoot.sPrtnInfo[usPrim];
                if (pusPart)
                    *pusPart = usPrim;
                if (pusDisk)
                    *pusDisk = usDisk;
                return (NO_ERROR);
            }
        }
    }

    return (ERROR_NOT_SUPPORTED);
}

/*
 * GetPrimaryPartitions:
 *      this returns the primary partitions.
 *
 *      This gets called from doshGetPartitionsList.
 *
 *      Returns 0 upon errors.
 *
 *      Based on code (C) Dmitry A. Steklenev.
 *
 *@@added V0.9.0 [umoeller]
 */

APIRET GetPrimaryPartitions(PARTITIONINFO **pppiFirst,
                            PARTITIONINFO **pppiThis,
                            PUSHORT posCount,    // in/out: partition count
                            PCHAR pcLetter,      // in/out: drive letter counter
                            UINT BmDisk,         // in: physical disk (1, 2, 3, ...) of boot manager or null
                            PAR_INFO* BmInfo,    // in: info returned by doshGetBootManager or NULL
                            UINT iDisk)          // in: system's physical disk count
{
    APIRET  arc = NO_ERROR;

    if (!BmInfo)
        arc = ERROR_INVALID_PARAMETER;
    else
    {
        SYS_INFO        MName[32];  // Name Space from Boot Manager
        memset(&MName, 0, sizeof(MName));

        // read boot manager name table
        if (!(arc = doshReadSector(BmDisk,
                                   &MName,
                                   BmInfo->bBeginHead,
                                   GetCyl(BmInfo->rBeginSecCyl),
                                   GetSec(BmInfo->rBeginSecCyl) + 3)))
        {
            MBR_INFO        MBoot;      // Master Boot
            USHORT          i;

            // read master boot record of this disk
            if (!(arc = doshReadSector(iDisk, &MBoot, 0, 0, 1)))
            {
                for (i = 0;
                     i < 4;     // there can be only four primary partitions
                     i++)
                {
                    // skip unused partition, BootManager or Extended partition
                    if (    (MBoot.sPrtnInfo[i].bFileSysCode)  // skip unused
                        &&  (MBoot.sPrtnInfo[i].bFileSysCode != PAR_BOOTMANAGER) // skip boot manager
                        &&  (MBoot.sPrtnInfo[i].bFileSysCode != PAR_EXTENDED) // skip extended
                       )
                    {
                        BOOL fBootable = (  (BmInfo)
                                         && (MName[(iDisk-1) * 4 + i].bootable & 0x01)
                                         );
                        // store this partition
                        if ((arc = AppendPartition(pppiFirst,
                                                   pppiThis,
                                                   posCount,
                                                   iDisk,
                                                   (fBootable)
                                                     ? (char*)&MName[(iDisk - 1) * 4 + i].name
                                                     : "",
                                                   *pcLetter,
                                                   MBoot.sPrtnInfo[i].bFileSysCode,
                                                   TRUE,        // primary
                                                   fBootable,
                                                   MBoot.sPrtnInfo[i].lTotalSects)))
                            return (arc);
                    }
                }
            }
        }
    }

    return (arc);
}

/*
 * GetLogicalDrives:
 *      this returns info for the logical drives
 *      in the extended partition. This gets called
 *      from GetExtendedPartition.
 *
 *      This gets called from GetExtendedPartition.
 *
 *      Based on code (C) Dmitry A. Steklenev.
 *
 *@@added V0.9.0 [umoeller]
 */

APIRET GetLogicalDrives(PARTITIONINFO **pppiFirst,
                        PARTITIONINFO **pppiThis,
                        PUSHORT posCount,
                        PCHAR pcLetter,
                        PAR_INFO* PrInfo,                    // in: MBR entry of extended partition
                        UINT PrDisk,
                        PAR_INFO* BmInfo)
{
    APIRET          arc = NO_ERROR;
    EXT_INFO        MBoot;      // Master Boot
    USHORT          i;

    if ((arc = doshReadSector(PrDisk,
                              &MBoot,
                              PrInfo->bBeginHead,
                              GetCyl(PrInfo->rBeginSecCyl),
                              GetSec(PrInfo->rBeginSecCyl))))
        return (arc);

    for (i = 0; i < 4; i++)
    {
        // skip unused partition or BootManager partition
        if (    (MBoot.sPrtnInfo[i].bFileSysCode)
             && (MBoot.sPrtnInfo[i].bFileSysCode != PAR_BOOTMANAGER)
           )
        {
            BOOL    fBootable = FALSE;
            BOOL    fAssignLetter = FALSE;

            // special work around extended partition
            if (MBoot.sPrtnInfo[i].bFileSysCode == PAR_EXTENDED)
            {
                if ((arc = GetLogicalDrives(pppiFirst,
                                            pppiThis,
                                            posCount,
                                            pcLetter,
                                            &MBoot.sPrtnInfo[i],
                                            PrDisk,
                                            BmInfo)))
                    return (arc);

                continue;
            }

            // raise driver letter if OS/2 would recognize this drive
            if (    (MBoot.sPrtnInfo[i].bFileSysCode < PAR_PCIX)
               )
                fAssignLetter = TRUE;

            if (fAssignLetter)
                (*pcLetter)++;

            fBootable = (   (BmInfo)
                         && ((MBoot.sBmNames[i].bootable & 0x01) != 0)
                        );

            if ((arc = AppendPartition(pppiFirst,
                                       pppiThis,
                                       posCount,
                                       PrDisk,
                                       (fBootable)
                                         ? (char*)&MBoot.sBmNames[i].name
                                         : "",
                                       (fAssignLetter)
                                         ? *pcLetter
                                         : ' ',
                                       MBoot.sPrtnInfo[i].bFileSysCode,
                                       FALSE,        // primary
                                       fBootable,    // bootable
                                       MBoot.sPrtnInfo[i].lTotalSects)))
                return (arc);
        }
    }

    return (NO_ERROR);
}

/*
 * GetExtendedPartition:
 *      this finds the extended partition on the given
 *      drive and calls GetLogicalDrives in turn.
 *
 *      This gets called from doshGetPartitionsList.
 *
 *      Based on code (C) Dmitry A. Steklenev.
 *
 *@@added V0.9.0 [umoeller]
 */

APIRET GetExtendedPartition(PARTITIONINFO **pppiFirst,
                            PARTITIONINFO **pppiThis,
                            PUSHORT posCount,
                            PCHAR pcLetter,
                            PAR_INFO* BmInfo,
                            UINT iDisk)                // in: disk to query
{
    APIRET          arc = NO_ERROR;
    MBR_INFO        MBoot;      // Master Boot
    USHORT          i;

    if ((arc = doshReadSector(iDisk, &MBoot, 0, 0, 1)))
        return (arc);

    // go thru MBR entries to find extended partition
    for (i = 0;
         i < 4;
         i++)
    {
        if (MBoot.sPrtnInfo[i].bFileSysCode == PAR_EXTENDED)
        {
            if ((arc = GetLogicalDrives(pppiFirst,
                                        pppiThis,
                                        posCount,
                                        pcLetter,
                                        &MBoot.sPrtnInfo[i],
                                        iDisk,
                                        BmInfo)))
                return (arc);
        }
    }

    return (NO_ERROR);
}

/*
 *@@ doshGetPartitionsList:
 *      this returns lots of information about the
 *      partitions on all physical disks, which is
 *      read directly from the MBRs and partition
 *      tables.
 *
 *      If NO_ERROR is returned by this function,
 *      *ppPartitionInfo points to a linked list of
 *      PARTITIONINFO structures, which has
 *      *pusPartitionCount items.
 *
 *      In that case, use doshFreePartitionsList to
 *      free the resources allocated by this function.
 *
 *      The linked list starts out with all the primary
 *      partitions, followed by the logical drives in
 *      the extended partitions. This function attempts
 *      to guess the correct drive letters and stores
 *      these with the PARTITIONINFO items, but there's
 *      no guarantee that this is correct. We correctly
 *      ignore Linux partitions here and give all primary
 *      partitions the C: letter, but I have no idea
 *      what happens with NTFS partitions, since I have
 *      none.
 *
 *      If an error != NO_ERROR is returned, *pusContext
 *      will be set to one of the following:
 *
 *      --  1: boot manager not found
 *
 *      --  2: primary partitions error
 *
 *      --  3: secondary partitions error
 *
 *      Based on code (C) Dmitry A. Steklenev.
 *
 *@@added V0.9.0 [umoeller]
 */

APIRET doshGetPartitionsList(PPARTITIONINFO *ppPartitionInfo,   // out: partition info array
                             PUSHORT pusPartitionCount,         // out: count of items in **ppPartitionInfo
                             PUSHORT pusContext)                // out: error context
{
    APIRET          arc = NO_ERROR;
    PAR_INFO        BmInfo;     // BootManager partition
    USHORT          usBmDisk;     // BootManager disk
    USHORT          cDisks = doshQueryDiskCount();    // physical disks count
    USHORT          i;

    PARTITIONINFO   *pPartitionInfos = NULL, // linked list of all partitions
                    *ppiTemp = NULL;
    USHORT          osCount;        // bootable partition count
    CHAR            cLetter = 'C';  // first drive letter

    if (cDisks > 8)              // Not above 8 disks
        cDisks = 8;

    // get boot manager disk and info
    if ((arc = doshGetBootManager(&usBmDisk,
                                  NULL,
                                  &BmInfo)) != NO_ERROR)
    {
        *pusContext = 1;
        return (arc);
    }
    // on each disk, read primary partitions
    for (i = 1; i <= cDisks; i++)
    {
        if ((arc = GetPrimaryPartitions(&pPartitionInfos,
                                        &ppiTemp,
                                        &osCount,
                                        &cLetter,
                                        usBmDisk,
                                        usBmDisk ? &BmInfo : 0,
                                        i)))
        {
            *pusContext = 2;
            return (arc);
        }
    }

    if (usBmDisk)
    {
        // boot manager found:
        // on each disk, read extended partition
        // with logical drives
        for (i = 1; i <= cDisks; i++)
        {
            if ((arc = GetExtendedPartition(&pPartitionInfos,
                                            &ppiTemp,
                                            &osCount,
                                            &cLetter,
                                            &BmInfo,
                                            i)))
            {
                *pusContext = 3;
                return (arc);
            }
        }
    }

    *ppPartitionInfo = pPartitionInfos;
    *pusPartitionCount = osCount;

    return (NO_ERROR);  // 0
}

/*
 *@@ doshFreePartitionsList:
 *      this frees the resources allocated by
 *      doshGetPartitionsList.
 *
 *@@added V0.9.0 [umoeller]
 */

APIRET doshFreePartitionsList(PPARTITIONINFO pPartitionInfo)
{
    PPARTITIONINFO ppiThis = NULL;
    ppiThis = pPartitionInfo;
    while (ppiThis)
    {
        PPARTITIONINFO ppiNext = ppiThis->pNext;
        free(ppiThis);
        ppiThis = ppiNext;
    }
    return (NO_ERROR);
}


