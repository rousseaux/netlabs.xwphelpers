
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
 *      pszTarget must be at least the same size as pszSource.
 *      If (fIsFAT), the file name will be made FAT-compliant (8+3).
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
          pTarget = pszTarget,
          pszInvalid = (fIsFAT)
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
        if (strchr(pszInvalid, *pSource) == NULL)
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
 */

APIRET doshSetCurrentDir(const char *pcszDir)
{
    if (pcszDir)
    {
        if (*pcszDir != 0)
            if (*(pcszDir+1) == ':')
            {
                // drive given:
                CHAR    cDrive = toupper(*(pcszDir));
                APIRET  arc;
                // change drive
                arc = DosSetDefaultDisk( (ULONG)(cDrive - 'A' + 1) );
                        // 1 = A:, 2 = B:, ...
                if (arc != NO_ERROR)
                    return (arc);
            }

        return (DosSetCurrentDir((PSZ)pcszDir));
    }
    return (ERROR_INVALID_PARAMETER);
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
 *      these functions can retrieve BLDLEVEL information from
 *      any executable module. See doshExecOpen.
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
 *      handle and pass it to doshExecClose when the
 *      executable is no longer needed to free
 *      resources.
 *
 *      If NO_ERROR is returned, all the fields through
 *      ulOS are set in EXECUTABLE. The psz* fields
 *      which follow afterwards require an additional
 *      call to doshExecQueryBldLevel.
 *
 *      If errors occur, this function returns the
 *      following error codes:
 *      -- ERROR_NOT_ENOUGH_MEMORY: malloc() failed.
 *      -- ERROR_INVALID_EXE_SIGNATURE: specified file
 *              has no DOS EXE header.
 *      -- ERROR_INVALID_PARAMETER: ppExec is NULL.
 *
 *      plus those of DosOpen, DosSetFilePtr, and
 *      DosRead.
 *
 *@@added V0.9.0 [umoeller]
 *@@changed V0.9.1 (2000-02-13) [umoeller]: fixed 32-bits flag
 */

APIRET doshExecOpen(const char* pcszExecutable,
                    PEXECUTABLE* ppExec)
{
    ULONG   ulAction = 0;
    HFILE   hFile;
    APIRET  arc = DosOpen((PSZ)pcszExecutable,
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
                          NULL);                          // no EAs

    if (arc == NO_ERROR)
    {
        // file opened successfully:
        // create EXECUTABLE structure

        if (ppExec)
        {
            *ppExec = (PEXECUTABLE)malloc(sizeof(EXECUTABLE));
            if (*ppExec)
            {
                ULONG           ulLocal = 0;

                memset((*ppExec), 0, sizeof(EXECUTABLE));

                // read old DOS EXE header
                (*ppExec)->pDosExeHeader = (PDOSEXEHEADER)malloc(sizeof(DOSEXEHEADER));
                if ((*ppExec)->pDosExeHeader == NULL)
                    arc = ERROR_NOT_ENOUGH_MEMORY;
                else
                {
                    ULONG   ulBytesRead = 0;
                    arc = DosSetFilePtr(hFile,
                                        0L,
                                        FILE_BEGIN,
                                        &ulLocal);      // out: new offset
                    arc = DosRead(hFile,
                                  (*ppExec)->pDosExeHeader,
                                  sizeof(DOSEXEHEADER),
                                  &((*ppExec)->cbDosExeHeader));
                    // now check if we really have a DOS header
                    if ((*ppExec)->pDosExeHeader->usDosExeID != 0x5a4d)
                        arc = ERROR_INVALID_EXE_SIGNATURE;
                    else
                    {
                        // we have a DOS header:
                        if ((*ppExec)->pDosExeHeader->usRelocTableOfs < 0x40)
                        {
                            // not LX or PE or NE:
                            (*ppExec)->ulOS = EXEOS_DOS3;
                            (*ppExec)->ulExeFormat = EXEFORMAT_OLDDOS;
                        }
                        else
                        {
                            // either LX or PE or NE:
                            // read more bytes from position
                            // specified in header
                            arc = DosSetFilePtr(hFile,
                                                (*ppExec)->pDosExeHeader->ulNewHeaderOfs,
                                                FILE_BEGIN,
                                                &ulLocal);

                            if (arc == NO_ERROR)
                            {
                                PBYTE   pbCheckOS = NULL;

                                // read two chars to find out header type
                                CHAR    achNewHeaderType[2] = "";
                                arc = DosRead(hFile,
                                              &achNewHeaderType,
                                              sizeof(achNewHeaderType),
                                              &ulBytesRead);
                                // reset file ptr
                                DosSetFilePtr(hFile,
                                              (*ppExec)->pDosExeHeader->ulNewHeaderOfs,
                                              FILE_BEGIN,
                                              &ulLocal);

                                if (memcmp(achNewHeaderType, "NE", 2) == 0)
                                {
                                    // New Executable:
                                    (*ppExec)->ulExeFormat = EXEFORMAT_NE;
                                    // read NE header
                                    (*ppExec)->pNEHeader = (PNEHEADER)malloc(sizeof(NEHEADER));
                                    DosRead(hFile,
                                            (*ppExec)->pNEHeader,
                                            sizeof(NEHEADER),
                                            &((*ppExec)->cbNEHeader));
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
                                    // read LX header
                                    (*ppExec)->pLXHeader = (PLXHEADER)malloc(sizeof(LXHEADER));
                                    DosRead(hFile,
                                            (*ppExec)->pLXHeader,
                                            sizeof(LXHEADER),
                                            &((*ppExec)->cbLXHeader));
                                    if ((*ppExec)->cbLXHeader == sizeof(LXHEADER))
                                        pbCheckOS = (PBYTE)(&((*ppExec)->pLXHeader->usTargetOS));
                                }
                                else if (memcmp(achNewHeaderType, "PE", 2) == 0)
                                {
                                    (*ppExec)->ulExeFormat = EXEFORMAT_PE;
                                    (*ppExec)->ulOS = EXEOS_WIN32;
                                    (*ppExec)->f32Bits = TRUE;
                                }
                                else
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
                            }
                        }
                    }

                    // store exec's HFILE
                    (*ppExec)->hfExe = hFile;
                }

                if (arc != NO_ERROR)
                    // error: clean up
                    doshExecClose(*ppExec);

            } // end if (*ppExec)
            else
                arc = ERROR_NOT_ENOUGH_MEMORY;
        } // end if (ppExec)
        else
            arc = ERROR_INVALID_PARAMETER;
    } // end if (arc == NO_ERROR)

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

        DosClose(pExec->hfExe);

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
 *      -- ERROR_INVALID_PARAMETER: pExec invalid
 *      -- ERROR_INVALID_EXE_SIGNATURE (191): pExec is not in LX or NE format
 *      -- ERROR_INVALID_DATA (13): non-resident name table not found.
 *      -- ERROR_NOT_ENOUGH_MEMORY: malloc() failed.
 *
 *      plus the error codes of DosSetFilePtr and DosRead.
 *
 *@@added V0.9.0 [umoeller]
 *@@changed V0.9.0 (99-10-22) [umoeller]: NE format now supported
 *@@changed V0.9.1 (99-12-06): fixed memory leak
 */

APIRET doshExecQueryBldLevel(PEXECUTABLE pExec)
{
    APIRET      arc = NO_ERROR;
    PSZ         pszNameTable = NULL;
    ULONG       ulNRNTOfs = 0;

    do
    {
        ULONG       ulLocal = 0,
                    ulBytesRead = 0;
        PSZ         pStartOfAuthor = NULL;

        if (pExec == NULL)
        {
            arc = ERROR_INVALID_PARAMETER;
            break;
        }

        if (pExec->ulExeFormat == EXEFORMAT_LX)
        {
            // OK, LX format:
            // check if we have a non-resident name table
            if (pExec->pLXHeader == NULL)
            {
                arc = ERROR_INVALID_DATA;
                break;
            }
            if (pExec->pLXHeader->ulNonResdNameTblOfs == 0)
            {
                arc = ERROR_INVALID_DATA;
                break;
            }

            ulNRNTOfs = pExec->pLXHeader->ulNonResdNameTblOfs;
        }
        else if (pExec->ulExeFormat == EXEFORMAT_NE)
        {
            // OK, NE format:
            // check if we have a non-resident name table
            if (pExec->pNEHeader == NULL)
            {
                arc = ERROR_INVALID_DATA;
                break;
            }
            if (pExec->pNEHeader->ulNonResdTblOfs == 0)
            {
                arc = ERROR_INVALID_DATA;
                break;
            }

            ulNRNTOfs = pExec->pNEHeader->ulNonResdTblOfs;
        }
        else
        {
            // neither LX nor NE: stop
            arc = ERROR_INVALID_EXE_SIGNATURE;
            break;
        }

        if (ulNRNTOfs == 0)
        {
            // shouldn't happen
            arc = ERROR_INVALID_DATA;
            break;
        }

        // move EXE file pointer to offset of non-resident name table
        // (from LX header)
        arc = DosSetFilePtr(pExec->hfExe,     // file is still open
                            ulNRNTOfs,      // ofs determined above
                            FILE_BEGIN,
                            &ulLocal);
        if (arc != NO_ERROR)
            break;

        // allocate memory as necessary
        pszNameTable = (PSZ)malloc(2001); // should suffice, because each entry
                                    // may only be 255 bytes in length
        if (pszNameTable)
        {
            arc = DosRead(pExec->hfExe,
                          pszNameTable,
                          2000,
                          &ulBytesRead);
            if (arc != NO_ERROR)
                break;
            if (*pszNameTable == 0)
            {
                // first byte (length byte) is null:
                arc = ERROR_INVALID_DATA;
                free (pszNameTable);        // fixed V0.9.1 (99-12-06)
                break;
            }

            // now copy the string, which is in Pascal format
            pExec->pszDescription = (PSZ)malloc((*pszNameTable) + 1);    // addt'l null byte
            memcpy(pExec->pszDescription,
                   pszNameTable + 1,        // skip length byte
                   *pszNameTable);          // length byte
            // terminate string
            *(pExec->pszDescription + (*pszNameTable)) = 0;

            // _Pmpf(("pszDescription: %s", pExec->pszDescription));

            // now parse the damn thing:
            // @#AUTHOR:VERSION#@ DESCRIPTION
            // but skip the first byte, which has the string length
            pStartOfAuthor = strstr(pExec->pszDescription, "@#");
            if (pStartOfAuthor)
            {
                PSZ pStartOfInfo = strstr(pStartOfAuthor + 2, "#@");
                // _Pmpf(("Testing"));
                if (pStartOfInfo)
                {
                    PSZ pEndOfAuthor = strchr(pStartOfAuthor + 2, ':');
                    // _Pmpf(("pStartOfinfo: %s", pStartOfInfo));
                    if (pEndOfAuthor)
                    {
                        // _Pmpf(("pEndOfAuthor: %s", pEndOfAuthor));
                        pExec->pszVendor = strhSubstr(pStartOfAuthor + 2, pEndOfAuthor);
                        pExec->pszVersion = strhSubstr(pEndOfAuthor + 1, pStartOfInfo);
                        // skip "@#" in info string
                        pStartOfInfo += 2;
                        // skip leading spaces in info string
                        while (*pStartOfInfo == ' ')
                            pStartOfInfo++;
                        // and copy until end of string
                        pExec->pszInfo = strdup(pStartOfInfo);
                    }
                }
            }

            free(pszNameTable);
        }
        else
            arc = ERROR_NOT_ENOUGH_MEMORY;
    } while (FALSE);


    return (arc);
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
 */

APIRET doshReadSector(USHORT disk,      // in: physical disk no. (1, 2, 3, ...)
                      void *buff,
                      USHORT head,
                      USHORT cylinder,
                      USHORT sector)
{
    UINT   arc;
    HFILE  dh = 0;
    char   dn[256];
    // char   ms[256];

    sprintf( dn, "%u:", disk );
    arc = DosPhysicalDisk(INFO_GETIOCTLHANDLE, &dh, 2, dn, 3);

    if (arc)
        // error:
        return (arc);
    else
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

        if(arc)
        {
            // error:
            DosPhysicalDisk(INFO_FREEIOCTLHANDLE, 0, 0, &dh, 2);
            return (arc);
        }

        DosPhysicalDisk(INFO_FREEIOCTLHANDLE, 0, 0, &dh, 2);
    }
    return (NO_ERROR);
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

char* doshType2FSName(unsigned char bFSType)  // in: FS type
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
    return ((rBeginSecCyl & 0x00C0) << 2) +
        ((rBeginSecCyl & 0xFF00) >> 8);
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
    APIRET          arc = NO_ERROR;
    MBR_INFO        MBoot;      // Master Boot
    SYS_INFO        MName[32];  // Name Space from Boot Manager
    USHORT          i;

    memset(&MName, 0, sizeof(MName));

    if (BmInfo)
    {
        // read boot manager name table
        if ((arc = doshReadSector(BmDisk, &MName, BmInfo->bBeginHead,
                                  GetCyl(BmInfo->rBeginSecCyl),
                                  GetSec(BmInfo->rBeginSecCyl) + 3)))
            return (arc);
    }

    // read master boot record
    if ((arc = doshReadSector(iDisk, &MBoot, 0, 0, 1)))
        return (arc);

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
    return (NO_ERROR);
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

    if ((arc = doshReadSector(PrDisk, &MBoot, PrInfo->bBeginHead,
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

        /* // if BootManager installed and partition is bootable
        if (BmInfo)
        {
            if (MBoot.sBmNames[i].bootable & 0x01)
            {
            }
        }

        // if BootManager not installed
        else
        {
            if (arc = AppendPartition(pppiFirst,
                                      pppiThis,
                                      posCount,
                                      PrDisk,
                                      "",
                                      *pcLetter,
                                      MBoot.sPrtnInfo[i].bFileSysCode,
                                      FALSE,
                                      MBoot.sPrtnInfo[i].lTotalSects))
                return (arc);
        } */
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
 *      --  1: boot manager not found
 *      --  2: primary partitions error
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

    if (usBmDisk)
    {
        // boot manager found:
        // on each disk, read extended partition
        // with logical drives
        for (i = 1; i <= cDisks; i++)
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


