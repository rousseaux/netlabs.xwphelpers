
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
#include "helpers\ensure.h"
#include "helpers\standards.h"
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
 *      -- ERROR_INVALID_EXE_SIGNATURE (191): header is
 *              neither plain DOS, nor NE, nor LX, nor PE.
 *              The given file probably isn't even an
 *              executable. This you will get if you
 *              pass in COM, BAT, or CMD files.
 *
 *      -- ERROR_BAD_EXE_FORMAT (193): header was
 *              recognized, but the header data was
 *              not understood.
 *
 *      -- ERROR_INVALID_PARAMETER: ppExec is NULL.
 *
 *      plus those of DosOpen, DosSetFilePtr, and
 *      DosRead.
 *
 *      The following executable types are supported
 *      (see EXECUTABLE for details):
 *
 *      --  Plain DOS 3.x executable without new header.
 *
 *      --  New Executable (NE), used by Win16 and
 *          16-bit OS/2 and still many of today's drivers.
 *
 *      --  Linear Executable (LX), OS/2 2.x and above.
 *
 *      --  Portable Executable (PE), used by Win32.
 *
 *      V0.9.12 adds support for NOSTUB executables,
 *      which are new-style executables (NE or LX)
 *      without a leading DOS header. The executable
 *      then starts directly with the NE or LX header.
 *      I am not sure whether PE supports such things
 *      as well... if so, it should be supported too.
 *
 *      @@todo:
 *
 *          win95 \WINDOWS\extract.exe is NE with a non-standard format
 *          win16 \WINDOWS\EXPAND.EXE
 *          win16 \WINDOWS\MSD.EXE"
 *
 *@@added V0.9.0 [umoeller]
 *@@changed V0.9.1 (2000-02-13) [umoeller]: fixed 32-bits flag
 *@@changed V0.9.7 (2000-12-20) [lafaix]: fixed ulNewHeaderOfs
 *@@changed V0.9.10 (2001-04-08) [lafaix]: added PE support
 *@@changed V0.9.10 (2001-04-08) [umoeller]: now setting ppExec only if NO_ERROR is returned
 *@@changed V0.9.12 (2001-05-03) [umoeller]: added support for NOSTUB newstyle executables
 *@@changed V0.9.16 (2001-12-08) [umoeller]: now using OPEN_SHARE_DENYWRITE
 *@@changed V0.9.16 (2001-12-08) [umoeller]: fLibrary was never set, works for LX and NE now
 *@@changed V0.9.16 (2001-12-08) [umoeller]: some speed optimizations, changed some return codes
 */

APIRET doshExecOpen(const char* pcszExecutable,
                    PEXECUTABLE* ppExec)
{
    APIRET  arc = NO_ERROR;

    ULONG   ulAction = 0;
    HFILE   hFile;
    PEXECUTABLE pExec = NULL;

    if (!ppExec)
        return (ERROR_INVALID_PARAMETER);

    if (!(pExec = (PEXECUTABLE)malloc(sizeof(EXECUTABLE))))
        return (ERROR_NOT_ENOUGH_MEMORY);

    memset(pExec, 0, sizeof(EXECUTABLE));

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
                           // | OPEN_SHARE_DENYNONE
                           | OPEN_SHARE_DENYWRITE       // changed V0.9.16 (2001-12-08) [umoeller]
                           | OPEN_ACCESS_READONLY,      // read-only mode
                        NULL)))                         // no EAs
    {
        // file opened successfully:

        // read old DOS EXE header
        if (!(pExec->pDosExeHeader = (PDOSEXEHEADER)malloc(sizeof(DOSEXEHEADER))))
            arc = ERROR_NOT_ENOUGH_MEMORY;
        else
        {
            pExec->cbDosExeHeader = sizeof(DOSEXEHEADER);
            if (!(arc = doshReadAt(hFile,
                                   0,
                                   FILE_BEGIN,
                                   &pExec->cbDosExeHeader,      // in/out
                                   (PBYTE)pExec->pDosExeHeader)))
            {
                ULONG ulNewHeaderOfs = 0;       // V0.9.12 (2001-05-03) [umoeller]
                BOOL  fLoadNewHeader = FALSE;

                // now check if we really have a DOS header
                if (pExec->pDosExeHeader->usDosExeID != 0x5a4d)
                {
                    // arc = ERROR_INVALID_EXE_SIGNATURE;

                    // V0.9.12 (2001-05-03) [umoeller]
                    // try loading new header directly; there are
                    // drivers which were built with NOSTUB, and
                    // the exe image starts out with the NE or LX
                    // image directly
                    fLoadNewHeader = TRUE;
                            // ulNewHeaderOfs is 0 now

                    // remove the DOS header info, since we have none
                    // V0.9.12 (2001-05-03) [umoeller]
                    FREE(pExec->pDosExeHeader);
                }
                else
                {
                    // we have a DOS header:
                    if (pExec->pDosExeHeader->usRelocTableOfs < 0x40)
                    {
                        // neither LX nor PE nor NE:
                        pExec->ulOS = EXEOS_DOS3;
                        pExec->ulExeFormat = EXEFORMAT_OLDDOS;
                    }
                    else
                    {
                        // we have a new header offset:
                        fLoadNewHeader = TRUE;
                        ulNewHeaderOfs = pExec->pDosExeHeader->ulNewHeaderOfs;
                    }
                }

                if (fLoadNewHeader)
                {
                    // either LX or PE or NE:
                    // read in new header...
                    // ulNewHeaderOfs is now either 0 (if no DOS header
                    // was found) or pDosExeHeader->ulNewHeaderOfs
                    // V0.9.12 (2001-05-03) [umoeller]
                    ULONG   cbRead;
                    PBYTE   pbHeader;

                    // now, we used to read in the first two chars
                    // to check out if we have PE or LX or NE and
                    // then read the header accordingly... but
                    // that wasn't terribly efficient. So load
                    // a chunk of data and then do a realloc()
                    // instead.

                    // take the largest of LXHEADER and NEHEADER and PEHEADER
                    cbRead = sizeof(LXHEADER);
                    if (sizeof(NEHEADER) > cbRead)
                        cbRead = sizeof(NEHEADER);
                    if (sizeof(PEHEADER) > cbRead)
                        cbRead = sizeof(PEHEADER);

                    if (!(pbHeader = (PBYTE)malloc(cbRead)))
                        arc = ERROR_NOT_ENOUGH_MEMORY;
                    else if (!(arc = doshReadAt(hFile,
                                                ulNewHeaderOfs,
                                                FILE_BEGIN,
                                                &cbRead,
                                                pbHeader)))
                    {
                        PBYTE   pbCheckOS = NULL;

                        PSZ     achNewHeaderType = (PSZ)pbHeader;

                        if (!memcmp(achNewHeaderType, "NE", 2))
                        {
                            // New Executable:
                            pExec->ulExeFormat = EXEFORMAT_NE;

                            if (cbRead < sizeof(NEHEADER))
                            {
                                arc = ERROR_BAD_EXE_FORMAT;
                                FREE(pbHeader);
                            }
                            else
                            {
                                pExec->pNEHeader = (PNEHEADER)realloc(pbHeader,
                                                                      sizeof(NEHEADER));
                                pExec->cbNEHeader = sizeof(NEHEADER);
                                pbCheckOS = &pExec->pNEHeader->bTargetOS;
                                // set library flag V0.9.16 (2001-12-08) [umoeller]
                                if (pExec->pNEHeader->usFlags & 0x8000)
                                    // library:
                                    pExec->fLibrary = TRUE;
                            }
                        }
                        else if (    (!memcmp(achNewHeaderType, "LX", 2))
                                  || (!memcmp(achNewHeaderType, "LE", 2))
                                            // this is used by SMARTDRV.EXE
                                )
                        {
                            // OS/2 Linear Executable:
                            pExec->ulExeFormat = EXEFORMAT_LX;

                            if (cbRead < sizeof(LXHEADER))
                            {
                                arc = ERROR_BAD_EXE_FORMAT;
                                FREE(pbHeader);
                            }
                            else
                            {
                                pExec->pLXHeader = (PLXHEADER)realloc(pbHeader,
                                                                      sizeof(LXHEADER));
                                // read in LX header
                                pExec->cbLXHeader = sizeof(LXHEADER);
                                pbCheckOS = (PBYTE)(&pExec->pLXHeader->usTargetOS);
                                // set library flag V0.9.16 (2001-12-08) [umoeller]
                                if (pExec->pLXHeader->ulFlags & 0x8000)
                                    // library:
                                    pExec->fLibrary = TRUE;
                            }
                        }
                        else if (!memcmp(achNewHeaderType, "PE", 2))
                        {
                            pExec->ulExeFormat = EXEFORMAT_PE;

                            if (cbRead < sizeof(PEHEADER))
                            {
                                arc = ERROR_BAD_EXE_FORMAT;
                                FREE(pbHeader);
                            }
                            else
                            {
                                // PE has a standard header of 24 bytes
                                // plus an extended header, so check what
                                // we've got
                                ULONG cbPE =   24
                                             + ((PPEHEADER)pbHeader)->usHeaderSize;
                                pExec->pPEHeader = (PPEHEADER)realloc(pbHeader,
                                                                      cbPE);

                                pExec->cbPEHeader = cbPE;
                                pExec->ulOS = EXEOS_WIN32;
                                pExec->f32Bits = TRUE;

                                // we have the first 24 bytes already, so
                                // go for the next chunk
                                if (cbRead = pExec->pPEHeader->usHeaderSize)
                                {
                                    if (!(arc = doshReadAt(hFile,
                                                           ulNewHeaderOfs + 24,
                                                           FILE_BEGIN,
                                                           &cbRead,
                                                           (PBYTE)pExec->pPEHeader + 24)))
                                    {
                                    }
                                    else
                                        arc = ERROR_BAD_EXE_FORMAT;
                                }
                            }
                        }
                        else
                            // strange type:
                            arc = ERROR_INVALID_EXE_SIGNATURE;

                        if (pbCheckOS)
                        {
                            // BYTE to check for operating system
                            // (NE and LX):
                            switch (*pbCheckOS)
                            {
                                case NEOS_OS2:
                                    pExec->ulOS = EXEOS_OS2;
                                    if (pExec->ulExeFormat == EXEFORMAT_LX)
                                        pExec->f32Bits = TRUE;
                                break;

                                case NEOS_WIN16:
                                    pExec->ulOS = EXEOS_WIN16;
                                break;

                                case NEOS_DOS4:
                                    pExec->ulOS = EXEOS_DOS4;
                                break;

                                case NEOS_WIN386:
                                    pExec->ulOS = EXEOS_WIN386;
                                    pExec->f32Bits = TRUE;
                                break;
                            }
                        }
                    }
                }
            } // end if (!(arc = DosSetFilePtr(hFile,
        } // end if pExec->pDosExeHeader = (PDOSEXEHEADER)malloc(sizeof(DOSEXEHEADER));

        // store exec's HFILE
        pExec->hfExe = hFile;
    } // end if (!(arc = DosOpen((PSZ)pcszExecutable,

    if (arc != NO_ERROR)
        // error: clean up
        doshExecClose(&pExec);
    else
        *ppExec = pExec;

    return (arc);
}

/*
 *@@ ParseBldLevel:
 *      called from doshExecQueryBldLevel to parse
 *      the BLDLEVEL string.
 *
 *      On entry, caller has copied the string into
 *      pExec->pszDescription. The string is
 *      null-terminated.
 *
 *      The BLDLEVEL string comes in two flavors.
 *
 *      --  The standard format is:
 *
 +              @#VENDOR:VERSION#@DESCRIPTION
 *
 *          DESCRIPTION can have leading spaces, but
 *          need to have them.
 *
 *      --  However, there is an extended version
 *          in that the DESCRIPTION field is split
 *          up even more. The marker for this seems
 *          to be that the description starts out
 *          with "##1##".
 *
 +              ##1## DATETIME BUILDMACHINE:ASD:LANG:CTRY:REVISION:UNKNOWN:FIXPAK@@DESCRIPTION
 *
 *          The problem is that the DATETIME field comes
 *          in several flavors. IBM uses things like
 *
 +              "Thu Nov 30 15:30:37 2000 BWBLD228"
 *
 *          while DANIS506.ADD has
 *
 +              "15.12.2000 18:22:57      Nachtigall"
 *
 *          Looks like the date/time string is standardized
 *          to have 24 characters then.
 *
 *@@added V0.9.12 (2001-05-18) [umoeller]
 *@@changed V0.9.12 (2001-05-19) [umoeller]: added extended BLDLEVEL support
 */

VOID ParseBldLevel(PEXECUTABLE pExec)
{
    const char  // *pStartOfAuthor = 0,
                *pStartOfVendor = 0;

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
            const char *pEndOfVendor = strchr(pStartOfVendor + 2,
                                              ':');
            if (pEndOfVendor)
            {
                pExec->pszVendor = strhSubstr(pStartOfVendor + 2,
                                              pEndOfVendor);
                pExec->pszVersion = strhSubstr(pEndOfVendor + 1,
                                               pStartOfInfo);
                // skip "@#" in DESCRIPTION string
                pStartOfInfo += 2;

                // now check if we have extended DESCRIPTION V0.9.12 (2001-05-19) [umoeller]
                if (    (strlen(pStartOfInfo) > 6)
                     && (!memcmp(pStartOfInfo, "##1##", 5))
                   )
                {
                    // yes: parse that beast
                    const char *p = pStartOfInfo + 5;

                    // get build date/time
                    if (strlen(p) > 24)
                    {
                        // date/time seems to be fixed 24 chars in length
                        if (pExec->pszBuildDateTime = (PSZ)malloc(25))
                        {
                            memcpy(pExec->pszBuildDateTime,
                                   p,
                                   24);
                            pExec->pszBuildDateTime[24] = '\0';

                            p += 24;

                            // now we're at the colon-separated
                            // strings, first of which is the
                            // build machine;
                            // skip leading spaces
                            while (*p == ' ')
                                p++;

                            if (*p)
                            {
                                char **papsz[] =
                                    {
                                        &pExec->pszBuildMachine,
                                        &pExec->pszASD,
                                        &pExec->pszLanguage,
                                        &pExec->pszCountry,
                                        &pExec->pszRevision,
                                        &pExec->pszUnknown,
                                        &pExec->pszFixpak
                                    };
                                ULONG ul;

                                for (ul = 0;
                                     ul < sizeof(papsz) / sizeof(papsz[0]);
                                     ul++)
                                {
                                    BOOL fStop = FALSE;
                                    const char *pNextColon = strchr(p, ':'),
                                               *pDoubleAt = strstr(p, "@@");
                                    if (!pNextColon)
                                    {
                                        // last item:
                                        if (pDoubleAt)
                                            pNextColon = pDoubleAt;
                                        else
                                            pNextColon = p + strlen(p);

                                        fStop = TRUE;
                                    }

                                    if (    (fStop)
                                         || (    (pNextColon)
                                              && (    (!pDoubleAt)
                                                   || (pNextColon < pDoubleAt)
                                                 )
                                            )
                                       )
                                    {
                                        if (pNextColon > p + 1)
                                            *(papsz[ul]) = strhSubstr(p, pNextColon);
                                    }
                                    else
                                        break;

                                    if (fStop)
                                        break;

                                    p = pNextColon + 1;
                                }
                            }
                        }
                    }

                    pStartOfInfo = strstr(p,
                                          "@@");
                    if (pStartOfInfo)
                        pStartOfInfo += 2;
                }

                // -- if we had no extended DESCRIPTION,
                //    pStartOfInfo points to regular description now
                // -- if we parse the extended DESCRIPTION above,
                //    pStartOfInfo points to after @@ now
                // -- if we had an error, pStartOfInfo is NULL
                if (pStartOfInfo)
                {
                    // add the regular DESCRIPTION then
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
 *@@changed V0.9.12 (2001-05-18) [umoeller]: extracted ParseBldLevel
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
                                memcpy(pExec->pszDescription,
                                       pszNameTable + 1,        // skip length byte
                                       *pszNameTable);          // length byte
                                // terminate string
                                *(pExec->pszDescription + (*pszNameTable)) = 0;

                                ParseBldLevel(pExec);
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
 *@@changed V0.9.9 (2001-04-05) [lafaix]: rewritten error checking code
 *@@changed V0.9.10 (2001-04-10) [lafaix]: added Win16 and Win386 support
 *@@changed V0.9.10 (2001-04-13) [lafaix]: removed 127 characters limit
 *@@changed V0.9.12 (2001-05-03) [umoeller]: adjusted for new NOSTUB support
 */

APIRET doshExecQueryImportedModules(PEXECUTABLE pExec,
                                    PFSYSMODULE *ppaModules,    // out: modules array
                                    PULONG pcModules)           // out: array item count
{
    if (    (pExec)
         && (    (pExec->ulOS == EXEOS_OS2)
              || (pExec->ulOS == EXEOS_WIN16)
              || (pExec->ulOS == EXEOS_WIN386)
            )
       )
    {
        ENSURE_BEGIN;
        ULONG       cModules = 0;
        PFSYSMODULE paModules = NULL;
        int i;

        ULONG ulNewHeaderOfs = 0;       // V0.9.12 (2001-05-03) [umoeller]

        if (pExec->pDosExeHeader)
            // executable has DOS stub: V0.9.12 (2001-05-03) [umoeller]
            ulNewHeaderOfs = pExec->pDosExeHeader->ulNewHeaderOfs;

        if (pExec->ulExeFormat == EXEFORMAT_LX)
        {
            // 32-bit OS/2 executable:
            cModules = pExec->pLXHeader->ulImportModTblCnt;

            if (cModules)
            {
                ULONG   cb = sizeof(FSYSMODULE) * cModules; // V0.9.9 (2001-04-03) [umoeller]
                ULONG   ulDummy;

                paModules = (PFSYSMODULE)malloc(cb);
                if (!paModules)
                    ENSURE_FAIL(ERROR_NOT_ENOUGH_MEMORY); // V0.9.9 (2001-04-03) [umoeller]

                memset(paModules, 0, cb);   // V0.9.9 (2001-04-03) [umoeller]

                ENSURE_SAFE(DosSetFilePtr(pExec->hfExe,
                                          pExec->pLXHeader->ulImportModTblOfs
                                            + ulNewHeaderOfs, // V0.9.12 (2001-05-03) [umoeller]
                                          FILE_BEGIN,
                                          &ulDummy));

                for (i = 0; i < cModules; i++)
                {
                    BYTE bLen = 0;

                    // reading the length of the module name
                    ENSURE_SAFE(DosRead(pExec->hfExe, &bLen, 1, &ulDummy));

                    // reading the module name
                    ENSURE_SAFE(DosRead(pExec->hfExe,
                                        paModules[i].achModuleName,
                                        bLen,
                                        &ulDummy));

                    // module names are not null terminated, so we must
                    // do it now
                    paModules[i].achModuleName[bLen] = 0;
                } // end for
            }
        } // end LX
        else if (pExec->ulExeFormat == EXEFORMAT_NE)
        {
            // 16-bit executable:
            cModules = pExec->pNEHeader->usModuleTblEntries;

            if (cModules)
            {
                ULONG cb = sizeof(FSYSMODULE) * cModules;

                paModules = (PFSYSMODULE)malloc(cb);
                if (!paModules)
                    ENSURE_FAIL(ERROR_NOT_ENOUGH_MEMORY);  // V0.9.9 (2001-04-03) [umoeller]

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

                    ENSURE_SAFE(DosSetFilePtr(pExec->hfExe,
                                              pExec->pNEHeader->usModRefTblOfs
                                                + ulNewHeaderOfs // V0.9.12 (2001-05-03) [umoeller]
                                                + sizeof(usOfs) * i,
                                              FILE_BEGIN,
                                              &ulDummy));

                    ENSURE_SAFE(DosRead(pExec->hfExe, &usOfs, 2, &ulDummy));

                    ENSURE_SAFE(DosSetFilePtr(pExec->hfExe,
                                              pExec->pNEHeader->usImportTblOfs
                                                + ulNewHeaderOfs // V0.9.12 (2001-05-03) [umoeller]
                                                + usOfs,
                                              FILE_BEGIN,
                                              &ulDummy));

                    ENSURE_SAFE(DosRead(pExec->hfExe, &bLen, 1, &ulDummy));

                    ENSURE_SAFE(DosRead(pExec->hfExe,
                                        paModules[i].achModuleName,
                                        bLen,
                                        &ulDummy));

                    paModules[i].achModuleName[bLen] = 0;
                } // end for
            }
        } // end NE
        else
            ENSURE_FAIL(ERROR_INVALID_EXE_SIGNATURE); // V0.9.9 (2001-04-03) [umoeller]

        // no error: output data
        *ppaModules = paModules;
        *pcModules = cModules;

        ENSURE_FINALLY;
            // if we had an error above, clean up
            free(paModules);
        ENSURE_END;
    }
    else
        ENSURE_FAIL(ERROR_INVALID_EXE_SIGNATURE); // V0.9.9 (2001-04-03) [umoeller]

    ENSURE_OK;
}

/*
 *@@ doshExecFreeImportedModules:
 *      frees resources allocated by doshExecQueryImportedModules.
 *
 *@@added V0.9.9 (2001-03-11)
 */

APIRET doshExecFreeImportedModules(PFSYSMODULE paModules)
{
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
 *@@changed V0.9.9 (2001-04-05) [lafaix]: rewritten error checking code
 *@@changed V0.9.12 (2001-05-03) [umoeller]: adjusted for new NOSTUB support
 */

APIRET ScanLXEntryTable(PEXECUTABLE pExec,
                        PFSYSFUNCTION paFunctions,
                        PULONG pcEntries)        // out: entry table entry count; ptr can be NULL
{
    ULONG  ulDummy;
    USHORT usOrdinal = 1,
           usCurrent = 0;
    int    i;

    ULONG ulNewHeaderOfs = 0; // V0.9.12 (2001-05-03) [umoeller]

    if (pExec->pDosExeHeader)
        // executable has DOS stub: V0.9.12 (2001-05-03) [umoeller]
        ulNewHeaderOfs = pExec->pDosExeHeader->ulNewHeaderOfs;

    ENSURE(DosSetFilePtr(pExec->hfExe,
                         pExec->pLXHeader->ulEntryTblOfs
                           + ulNewHeaderOfs, // V0.9.12 (2001-05-03) [umoeller]
                         FILE_BEGIN,
                         &ulDummy));

    while (TRUE)
    {
        BYTE   bCnt,
               bType,
               bFlag;

        ENSURE(DosRead(pExec->hfExe, &bCnt, 1, &ulDummy));

        if (bCnt == 0)
            // end of the entry table
            break;

        ENSURE(DosRead(pExec->hfExe, &bType, 1, &ulDummy));

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
                ENSURE(DosSetFilePtr(pExec->hfExe,
                                     sizeof(USHORT),
                                     FILE_CURRENT,
                                     &ulDummy));

                for (i = 0; i < bCnt; i ++)
                {
                    ENSURE(DosRead(pExec->hfExe, &bFlag, 1, &ulDummy));

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

                    ENSURE(DosSetFilePtr(pExec->hfExe,
                                         sizeof(USHORT),
                                         FILE_CURRENT,
                                         &ulDummy));

                } // end for
            break;

            /*
             * 286 call gate entries
             *
             * the bundle type is followed by the object number
             * and by bCnt bFlag+usOffset+usCallGate entries
             *
             */

            case 2:
                ENSURE(DosSetFilePtr(pExec->hfExe,
                                     sizeof(USHORT),
                                     FILE_CURRENT,
                                     &ulDummy));

                for (i = 0; i < bCnt; i ++)
                {
                    ENSURE(DosRead(pExec->hfExe, &bFlag, 1, &ulDummy));

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

                    ENSURE(DosSetFilePtr(pExec->hfExe,
                                         sizeof(USHORT) + sizeof(USHORT),
                                         FILE_CURRENT,
                                         &ulDummy));

                } // end for
            break;

            /*
             * 32-bit entries
             *
             * the bundle type is followed by the object number
             * and by bCnt bFlag+ulOffset entries
             *
             */

            case 3:
                ENSURE(DosSetFilePtr(pExec->hfExe,
                                     sizeof(USHORT),
                                     FILE_CURRENT,
                                     &ulDummy));

                for (i = 0; i < bCnt; i ++)
                {
                    ENSURE(DosRead(pExec->hfExe, &bFlag, 1, &ulDummy));

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

                    ENSURE(DosSetFilePtr(pExec->hfExe,
                                         sizeof(ULONG),
                                         FILE_CURRENT,
                                         &ulDummy));
                } // end for
            break;

            /*
             * forwarder entries
             *
             * the bundle type is followed by a reserved word
             * and by bCnt bFlag+usModOrd+ulOffsOrdNum entries
             *
             */

            case 4:
                ENSURE(DosSetFilePtr(pExec->hfExe,
                                     sizeof(USHORT),
                                     FILE_CURRENT,
                                     &ulDummy));

                for (i = 0; i < bCnt; i ++)
                {
                    ENSURE(DosSetFilePtr(pExec->hfExe,
                                         sizeof(BYTE) + sizeof(USHORT) + sizeof(ULONG),
                                         FILE_CURRENT,
                                         &ulDummy));

                    if (paFunctions)
                    {
                        paFunctions[usCurrent].ulOrdinal = usOrdinal;
                        paFunctions[usCurrent].ulType = 4;
                        paFunctions[usCurrent].achFunctionName[0] = 0;
                    }
                    usCurrent++;

                    usOrdinal++;
                } // end for
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

                    usCurrent++;
                }
                ENSURE_FAIL(ERROR_INVALID_LIST_FORMAT);
                    // whatever
                    // V0.9.9 (2001-04-03) [umoeller]
        } // end switch (bType & 0x7F)
    } // end while (TRUE)

    if (pcEntries)
       *pcEntries = usCurrent;

    ENSURE_OK;
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
 *@@changed V0.9.9 (2001-04-05) [lafaix]: rewritten error checking code
 *@@changed V0.9.12 (2001-05-03) [umoeller]: adjusted for new NOSTUB support
 */

APIRET ScanNEEntryTable(PEXECUTABLE pExec,
                        PFSYSFUNCTION paFunctions,
                        PULONG pcEntries)        // out: entry table entry count; ptr can be NULL
{
    ULONG  ulDummy;
    USHORT usOrdinal = 1,
           usCurrent = 0;
    int    i;

    ULONG ulNewHeaderOfs = 0;

    if (pExec->pDosExeHeader)
        // executable has DOS stub: V0.9.12 (2001-05-03) [umoeller]
        ulNewHeaderOfs = pExec->pDosExeHeader->ulNewHeaderOfs;

    ENSURE(DosSetFilePtr(pExec->hfExe,
                         pExec->pNEHeader->usEntryTblOfs
                           + ulNewHeaderOfs, // V0.9.12 (2001-05-03) [umoeller]
                         FILE_BEGIN,
                         &ulDummy));

    while (TRUE)
    {
        BYTE bCnt,
             bType,
             bFlag;

        ENSURE(DosRead(pExec->hfExe, &bCnt, 1, &ulDummy));

        if (bCnt == 0)
            // end of the entry table
            break;

        ENSURE(DosRead(pExec->hfExe, &bType, 1, &ulDummy));

        if (bType)
        {
            for (i = 0; i < bCnt; i++)
            {
                ENSURE(DosRead(pExec->hfExe,
                               &bFlag,
                               1,
                               &ulDummy));

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
                {
                    // moveable segment
                    ENSURE(DosSetFilePtr(pExec->hfExe,
                                         5,
                                         FILE_CURRENT,
                                         &ulDummy));
                }
                else
                {
                    // fixed segment or constant (0xFE)
                    ENSURE(DosSetFilePtr(pExec->hfExe,
                                         2,
                                         FILE_CURRENT,
                                         &ulDummy));
                }

            } // end for
        }
        else
            usOrdinal += bCnt;
    } // end while (TRUE)

    if (pcEntries)
        *pcEntries = usCurrent;

    ENSURE_OK;
}

/*
 *@@ Compare:
 *      binary search helper
 *
 *@@added V0.9.9 (2001-04-01) [lafaix]
 *@@changed V0.9.9 (2001-04-07) [umoeller]: added _Optlink, or this won't compile as C++
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
 *@@changed V0.9.9 (2001-04-05) [lafaix]: removed the 127 char limit
 *@@changed V0.9.9 (2001-04-05) [lafaix]: rewritten error checking code
 */

APIRET ScanNameTable(PEXECUTABLE pExec,
                     ULONG cFunctions,
                     PFSYSFUNCTION paFunctions)
{
    ULONG   ulDummy;

    USHORT        usOrdinal;
    PFSYSFUNCTION pFunction;

    while (TRUE)
    {
        BYTE   bLen;
        CHAR   achName[256];
        // int    i;

        ENSURE(DosRead(pExec->hfExe, &bLen, 1, &ulDummy));

        if (bLen == 0)
            // end of the name table
            break;

        ENSURE(DosRead(pExec->hfExe, &achName, bLen, &ulDummy));
        achName[bLen] = 0;

        ENSURE(DosRead(pExec->hfExe, &usOrdinal, sizeof(USHORT), &ulDummy));

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

    ENSURE_OK;
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
 *@@changed V0.9.9 (2001-04-05) [lafaix]: rewritten error checking code
 *@@changed V0.9.10 (2001-04-10) [lafaix]: added Win16 and Win386 support
 *@@changed V0.9.12 (2001-05-03) [umoeller]: adjusted for new NOSTUB support
 */

APIRET doshExecQueryExportedFunctions(PEXECUTABLE pExec,
                                      PFSYSFUNCTION *ppaFunctions,  // out: functions array
                                      PULONG pcFunctions)           // out: array item count
{
    if (    (pExec)
         && (    (pExec->ulOS == EXEOS_OS2)
              || (pExec->ulOS == EXEOS_WIN16)
              || (pExec->ulOS == EXEOS_WIN386)
            )
       )
    {
        ENSURE_BEGIN;
        ULONG         cFunctions = 0;
        PFSYSFUNCTION paFunctions = NULL;

        ULONG ulDummy;

        ULONG ulNewHeaderOfs = 0; // V0.9.12 (2001-05-03) [umoeller]

        if (pExec->pDosExeHeader)
            // executable has DOS stub: V0.9.12 (2001-05-03) [umoeller]
            ulNewHeaderOfs = pExec->pDosExeHeader->ulNewHeaderOfs;

        if (pExec->ulExeFormat == EXEFORMAT_LX)
        {
            // It's a 32bit OS/2 executable

            // the number of exported entry points is not stored
            // in the executable header; we have to count them in
            // the entry table

            ENSURE(ScanLXEntryTable(pExec, NULL, &cFunctions));

            // we now have the number of exported entries; let us
            // build them

            if (cFunctions)
            {
                ULONG cb = sizeof(FSYSFUNCTION) * cFunctions;

                paFunctions = (PFSYSFUNCTION)malloc(cb);
                if (!paFunctions)
                    ENSURE_FAIL(ERROR_NOT_ENOUGH_MEMORY);

                // we rescan the entry table (the cost is not as bad
                // as it may seem, due to disk caching)

                ENSURE_SAFE(ScanLXEntryTable(pExec, paFunctions, NULL));

                // we now scan the resident name table entries

                ENSURE_SAFE(DosSetFilePtr(pExec->hfExe,
                                          pExec->pLXHeader->ulResdNameTblOfs
                                            + ulNewHeaderOfs, // V0.9.12 (2001-05-03) [umoeller]
                                          FILE_BEGIN,
                                          &ulDummy));

                ENSURE_SAFE(ScanNameTable(pExec, cFunctions, paFunctions));

                // we now scan the non-resident name table entries,
                // whose offset is _from the begining of the file_

                ENSURE_SAFE(DosSetFilePtr(pExec->hfExe,
                                          pExec->pLXHeader->ulNonResdNameTblOfs,
                                          FILE_BEGIN,
                                          &ulDummy));

                ENSURE_SAFE(ScanNameTable(pExec, cFunctions, paFunctions));
            } // end if (cFunctions)
        }
        else if (pExec->ulExeFormat == EXEFORMAT_NE)
        {
            // it's a "new" segmented 16bit executable

            // here too the number of exported entry points
            // is not stored in the executable header; we
            // have to count them in the entry table

            ENSURE(ScanNEEntryTable(pExec, NULL, &cFunctions));

            // we now have the number of exported entries; let us
            // build them

            if (cFunctions)
            {
                // USHORT usOrdinal = 1;
                       // usCurrent = 0;

                paFunctions = (PFSYSFUNCTION)malloc(sizeof(FSYSFUNCTION) * cFunctions);
                if (!paFunctions)
                    ENSURE_FAIL(ERROR_NOT_ENOUGH_MEMORY);

                // we rescan the entry table (the cost is not as bad
                // as it may seem, due to disk caching)

                ENSURE_SAFE(ScanNEEntryTable(pExec, paFunctions, NULL));

                // we now scan the resident name table entries

                ENSURE_SAFE(DosSetFilePtr(pExec->hfExe,
                                          pExec->pNEHeader->usResdNameTblOfs
                                            + ulNewHeaderOfs, // V0.9.12 (2001-05-03) [umoeller]
                                          FILE_BEGIN,
                                          &ulDummy));

                ENSURE_SAFE(ScanNameTable(pExec, cFunctions, paFunctions));

                // we now scan the non-resident name table entries,
                // whose offset is _from the begining of the file_

                ENSURE_SAFE(DosSetFilePtr(pExec->hfExe,
                                          pExec->pNEHeader->ulNonResdTblOfs,
                                          FILE_BEGIN,
                                          &ulDummy));

                ENSURE_SAFE(ScanNameTable(pExec, cFunctions, paFunctions));
            }
        }
        else
            ENSURE_FAIL(ERROR_INVALID_EXE_SIGNATURE); // V0.9.9 (2001-04-03) [umoeller]

        // no error: output data
        *ppaFunctions = paFunctions;
        *pcFunctions = cFunctions;

        ENSURE_FINALLY;
            // if we had an error above, clean up
            free(paFunctions);
        ENSURE_END;
    }
    else
        ENSURE_FAIL(ERROR_INVALID_EXE_SIGNATURE); // V0.9.9 (2001-04-03) [umoeller]

    ENSURE_OK;
}

/*
 *@@ doshExecFreeExportedFunctions:
 *      frees resources allocated by doshExecQueryExportedFunctions.
 *
 *@@added V0.9.9 (2001-03-11)
 */

APIRET doshExecFreeExportedFunctions(PFSYSFUNCTION paFunctions)
{
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
 *@@changed V0.9.10 (2001-04-10) [lafaix]: added Win16 and Win386 support
 *@@changed V0.9.12 (2001-05-03) [umoeller]: adjusted for new NOSTUB support
 */

APIRET doshExecQueryResources(PEXECUTABLE pExec,     // in: executable from doshExecOpen
                              PFSYSRESOURCE *ppaResources,   // out: res's array
                              PULONG pcResources)    // out: array item count
{
    if (    (pExec)
         && (    (pExec->ulOS == EXEOS_OS2)
              || (pExec->ulOS == EXEOS_WIN16)
              || (pExec->ulOS == EXEOS_WIN386)
            )
       )
    {
        ENSURE_BEGIN;
        ULONG           cResources = 0;
        PFSYSRESOURCE   paResources = NULL;

        ULONG           ulNewHeaderOfs = 0; // V0.9.12 (2001-05-03) [umoeller]

        if (pExec->pDosExeHeader)
            // executable has DOS stub: V0.9.12 (2001-05-03) [umoeller]
            ulNewHeaderOfs = pExec->pDosExeHeader->ulNewHeaderOfs;

        if (pExec->ulExeFormat == EXEFORMAT_LX)
        {
            // 32-bit OS/2 executable:
            PLXHEADER pLXHeader = pExec->pLXHeader;
            if (cResources = pLXHeader->ulResTblCnt)
            {
                #pragma pack(1)     // V0.9.9 (2001-04-02) [umoeller]
                struct rsrc32               // Resource Table Entry
                {
                    unsigned short  type;   // Resource type
                    unsigned short  name;   // Resource name
                    unsigned long   cb;     // Resource size
                    unsigned short  obj;    // Object number
                    unsigned long   offset; // Offset within object
                } rs;

                struct o32_obj                    // Flat .EXE object table entry
                {
                    unsigned long   o32_size;     // Object virtual size
                    unsigned long   o32_base;     // Object base virtual address
                    unsigned long   o32_flags;    // Attribute flags
                    unsigned long   o32_pagemap;  // Object page map index
                    unsigned long   o32_mapsize;  // Number of entries in object page map
                    unsigned long   o32_reserved; // Reserved
                } ot;
                #pragma pack() // V0.9.9 (2001-04-03) [umoeller]

                ULONG cb = sizeof(FSYSRESOURCE) * cResources;
                ULONG ulDummy;
                int i;
                ULONG ulCurOfs;

                paResources = (PFSYSRESOURCE)malloc(cb);
                if (!paResources)
                    ENSURE_FAIL(ERROR_NOT_ENOUGH_MEMORY);

                memset(paResources, 0, cb); // V0.9.9 (2001-04-03) [umoeller]

                ENSURE_SAFE(DosSetFilePtr(pExec->hfExe,
                                          pLXHeader->ulResTblOfs
                                            + ulNewHeaderOfs, // V0.9.12 (2001-05-03) [umoeller]
                                          FILE_BEGIN,
                                          &ulDummy));

                for (i = 0; i < cResources; i++)
                {
                    ENSURE_SAFE(DosRead(pExec->hfExe, &rs, 14, &ulDummy));

                    paResources[i].ulID = rs.name;
                    paResources[i].ulType = rs.type;
                    paResources[i].ulSize = rs.cb;
                    paResources[i].ulFlag = rs.obj; // Temp storage for Object
                                                    // number.  Will be filled
                                                    // with resource flag
                                                    // later.
                }

                for (i = 0; i < cResources; i++)
                {
                    ULONG ulOfsThis =   pLXHeader->ulObjTblOfs
                                      + ulNewHeaderOfs // V0.9.12 (2001-05-03) [umoeller]
                                      + (   sizeof(ot)
                                          * (paResources[i].ulFlag - 1));

                    ENSURE_SAFE(DosSetFilePtr(pExec->hfExe,
                                              ulOfsThis,
                                              FILE_BEGIN,
                                              &ulDummy));

                    ENSURE_SAFE(DosRead(pExec->hfExe, &ot, sizeof(ot), &ulDummy));

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
                } // end for
            } // end if (cResources)
        } // end if (pExec->ulExeFormat == EXEFORMAT_LX)
        else if (pExec->ulExeFormat == EXEFORMAT_NE)
        {
            PNEHEADER pNEHeader = pExec->pNEHeader;

            if (pExec->ulOS == EXEOS_OS2)
            {
                // 16-bit OS/2 executable:
                cResources = pNEHeader->usResSegmCount;

                if (cResources)
                {
                    #pragma pack(1)     // V0.9.9 (2001-04-02) [umoeller]
                    struct {unsigned short type; unsigned short name;} rti;
                    struct new_seg                          // New .EXE segment table entry
                    {
                        unsigned short      ns_sector;      // File sector of start of segment
                        unsigned short      ns_cbseg;       // Number of bytes in file
                        unsigned short      ns_flags;       // Attribute flags
                        unsigned short      ns_minalloc;    // Minimum allocation in bytes
                    } ns;
                    #pragma pack()

                    ULONG cb = sizeof(FSYSRESOURCE) * cResources;
                    ULONG ulDummy;
                    int i;

                    paResources = (PFSYSRESOURCE)malloc(cb);
                    if (!paResources)
                        ENSURE_FAIL(ERROR_NOT_ENOUGH_MEMORY);

                    memset(paResources, 0, cb);     // V0.9.9 (2001-04-03) [umoeller]

                    // we first read the resources IDs and types

                    ENSURE_SAFE(DosSetFilePtr(pExec->hfExe,
                                              pNEHeader->usResTblOfs
                                                + ulNewHeaderOfs, // V0.9.12 (2001-05-03) [umoeller]
                                              FILE_BEGIN,
                                              &ulDummy));

                    for (i = 0; i < cResources; i++)
                    {
                        ENSURE_SAFE(DosRead(pExec->hfExe, &rti, sizeof(rti), &ulDummy));

                        paResources[i].ulID = rti.name;
                        paResources[i].ulType = rti.type;
                    }

                    // we then read their sizes and flags

                    for (i = 0; i < cResources; i++)
                    {
                        ENSURE_SAFE(DosSetFilePtr(pExec->hfExe,
                                                  ulNewHeaderOfs // V0.9.12 (2001-05-03) [umoeller]
                                                    + pNEHeader->usSegTblOfs
                                                    + (sizeof(ns)
                                                    * (  pNEHeader->usSegTblEntries
                                                       - pNEHeader->usResSegmCount
                                                       + i)),
                                                    FILE_BEGIN,
                                                    &ulDummy));

                        ENSURE_SAFE(DosRead(pExec->hfExe, &ns, sizeof(ns), &ulDummy));

                        paResources[i].ulSize = ns.ns_cbseg;

                        paResources[i].ulFlag  = (ns.ns_flags & OBJPRELOAD) ? RNPRELOAD : 0;
                        paResources[i].ulFlag |= (ns.ns_flags & OBJSHARED) ? RNPURE : 0;
                        paResources[i].ulFlag |= (ns.ns_flags & OBJDISCARD) ? RNMOVE : 0;
                        paResources[i].ulFlag |= (ns.ns_flags & OBJDISCARD) ? 4096 : 0;
                    }
                } // end if (cResources)
            }
            else
            {
                // 16-bit Windows executable
                USHORT usAlignShift;
                ULONG  ulDummy;

                ENSURE(DosSetFilePtr(pExec->hfExe,
                                     pNEHeader->usResTblOfs
                                       + ulNewHeaderOfs, // V0.9.12 (2001-05-03) [umoeller]
                                     FILE_BEGIN,
                                     &ulDummy));

                ENSURE(DosRead(pExec->hfExe,
                               &usAlignShift,
                               sizeof(usAlignShift),
                               &ulDummy));

                while (TRUE)
                {
                    USHORT usTypeID;
                    USHORT usCount;

                    ENSURE(DosRead(pExec->hfExe,
                                   &usTypeID,
                                   sizeof(usTypeID),
                                   &ulDummy));

                    if (usTypeID == 0)
                        break;

                    ENSURE(DosRead(pExec->hfExe,
                                   &usCount,
                                   sizeof(usCount),
                                   &ulDummy));

                    ENSURE(DosSetFilePtr(pExec->hfExe,
                                         sizeof(ULONG),
                                         FILE_CURRENT,
                                         &ulDummy));

                    cResources += usCount;

                    // first pass, skip NAMEINFO table
                    ENSURE(DosSetFilePtr(pExec->hfExe,
                                         usCount*6*sizeof(USHORT),
                                         FILE_CURRENT,
                                         &ulDummy));
                }

                if (cResources)
                {
                    USHORT usCurrent = 0;
                    ULONG cb = sizeof(FSYSRESOURCE) * cResources;

                    paResources = (PFSYSRESOURCE)malloc(cb);
                    if (!paResources)
                        ENSURE_FAIL(ERROR_NOT_ENOUGH_MEMORY);

                    memset(paResources, 0, cb);

                    ENSURE_SAFE(DosSetFilePtr(pExec->hfExe,
                                              pNEHeader->usResTblOfs
                                                + ulNewHeaderOfs,
                                              FILE_BEGIN,
                                              &ulDummy));

                    ENSURE_SAFE(DosRead(pExec->hfExe,
                                        &usAlignShift,
                                        sizeof(usAlignShift),
                                        &ulDummy));

                    while (TRUE)
                    {
                        USHORT usTypeID;
                        USHORT usCount;
                        int i;

                        ENSURE_SAFE(DosRead(pExec->hfExe,
                                            &usTypeID,
                                            sizeof(usTypeID),
                                            &ulDummy));

                        if (usTypeID == 0)
                            break;

                        ENSURE_SAFE(DosRead(pExec->hfExe,
                                            &usCount,
                                            sizeof(usCount),
                                            &ulDummy));

                        ENSURE_SAFE(DosSetFilePtr(pExec->hfExe,
                                                  sizeof(ULONG),
                                                  FILE_CURRENT,
                                                  &ulDummy));

                        // second pass, read NAMEINFO table
                        for (i = 0; i < usCount; i++)
                        {
                            USHORT usLength,
                                   usFlags,
                                   usID;

                            ENSURE_SAFE(DosSetFilePtr(pExec->hfExe,
                                                      sizeof(USHORT),
                                                      FILE_CURRENT,
                                                      &ulDummy));

                            ENSURE_SAFE(DosRead(pExec->hfExe,
                                                &usLength,
                                                sizeof(USHORT),
                                                &ulDummy));
                            ENSURE_SAFE(DosRead(pExec->hfExe,
                                                &usFlags,
                                                sizeof(USHORT),
                                                &ulDummy));
                            ENSURE_SAFE(DosRead(pExec->hfExe,
                                                &usID,
                                                sizeof(USHORT),
                                                &ulDummy));

                            ENSURE_SAFE(DosSetFilePtr(pExec->hfExe,
                                                      2*sizeof(USHORT),
                                                      FILE_CURRENT,
                                                      &ulDummy));

                            // !!! strings ids and types not handled yet
                            // !!! 15th bit is used to denotes strings
                            // !!! offsets [lafaix]
                            paResources[usCurrent].ulType = usTypeID ^ 0x8000;
                            paResources[usCurrent].ulID = usID ^ 0x8000;
                            paResources[usCurrent].ulSize = usLength << usAlignShift;
                            paResources[usCurrent].ulFlag = usFlags & 0x70;

                            usCurrent++;
                        }
                    }
                }
            }
        } // end else if (pExec->ulExeFormat == EXEFORMAT_NE)
        else
            ENSURE_FAIL(ERROR_INVALID_EXE_SIGNATURE); // V0.9.9 (2001-04-03) [umoeller]

        *ppaResources = paResources;
        *pcResources = cResources;

        ENSURE_FINALLY;
            // if we had an error above, clean up
            free(paResources);
        ENSURE_END;
    }
    else
        ENSURE_FAIL(ERROR_INVALID_EXE_SIGNATURE); // V0.9.9 (2001-04-03) [umoeller]

    ENSURE_OK;
}

/*
 *@@ doshExecFreeResources:
 *      frees resources allocated by doshExecQueryResources.
 *
 *@@added V0.9.7 (2000-12-18) [lafaix]
 */

APIRET doshExecFreeResources(PFSYSRESOURCE paResources)
{
    free(paResources);
    return (NO_ERROR);
}

/*
 *@@ doshLoadLXMaps:
 *      loads the three main LX maps into the given
 *      EXECUTABLE structure.
 *
 *      This loads:
 *
 *      1)  the LX resource table;
 *
 *      2)  the LX object table;
 *
 *      3)  the LX object _page_ table (object map).
 *
 *      Note that this is not automatically called
 *      by doshExecOpen to save time, since the LX
 *      maps are not needed for all the other exe
 *      functions.
 *
 *      This returns:
 *
 *      --  NO_ERROR: all three LX maps were loaded,
 *          and pExec->fLXMapsLoaded was set to TRUE.
 *
 *      --  ERROR_INVALID_PARAMETER
 *
 *      --  ERROR_INVALID_EXE_SIGNATURE: pExec does
 *          not specify an LX executable.
 *
 *      --  ERROR_NO_DATA: at least one of the structs
 *          does not exist.
 *
 *      --  ERROR_NOT_ENOUGH_MEMORY
 *
 *      plus the error codes of doshReadAt.
 *
 *      Call doshFreeLXMaps to clean up explicitly, but
 *      that func automatically gets called by doshExecClose.
 *
 *@@added V0.9.16 (2001-12-08) [umoeller]
 */

APIRET doshLoadLXMaps(PEXECUTABLE pExec)
{
    APIRET arc;

    PLXHEADER pLXHeader;

    if (!pExec)
        arc = ERROR_INVALID_PARAMETER;
    else if (pExec->fLXMapsLoaded)
        // already loaded:
        arc = NO_ERROR;
    else if (    (pExec->ulExeFormat != EXEFORMAT_LX)
              || (!(pLXHeader = pExec->pLXHeader))
            )
        arc = ERROR_INVALID_EXE_SIGNATURE;
    else
    {
        ULONG ulNewHeaderOfs = 0;
        ULONG cb;

        if (pExec->pDosExeHeader)
            // executable has DOS stub: V0.9.12 (2001-05-03) [umoeller]
            ulNewHeaderOfs = pExec->pDosExeHeader->ulNewHeaderOfs;

        // resource table
        if (    (!(arc = doshAllocArray(pLXHeader->ulResTblCnt,
                                        sizeof(RESOURCETABLEENTRY),
                                        (PBYTE*)&pExec->pRsTbl,
                                        &cb)))
             && (!(arc = doshReadAt(pExec->hfExe,
                                    pLXHeader->ulResTblOfs
                                      + ulNewHeaderOfs,
                                    FILE_BEGIN,
                                    &cb,
                                    (PBYTE)pExec->pRsTbl)))
            )
        {
            // object table
            if (    (!(arc = doshAllocArray(pLXHeader->ulObjCount,
                                            sizeof(OBJECTTABLEENTRY),
                                            (PBYTE*)&pExec->pObjTbl,
                                            &cb)))
                 && (!(arc = doshReadAt(pExec->hfExe,
                                        pLXHeader->ulObjTblOfs
                                          + ulNewHeaderOfs,
                                        FILE_BEGIN,
                                        &cb,
                                        (PBYTE)pExec->pObjTbl)))
               )
            {
                // object page table
                if (    (!(arc = doshAllocArray(pLXHeader->ulPageCount,
                                                sizeof(OBJECTPAGETABLEENTRY),
                                                (PBYTE*)&pExec->pObjPageTbl,
                                                &cb)))
                     && (!(arc = doshReadAt(pExec->hfExe,
                                            pLXHeader->ulObjPageTblOfs
                                              + ulNewHeaderOfs,
                                            FILE_BEGIN,
                                            &cb,
                                            (PBYTE)pExec->pObjPageTbl)))
                   )
                {
                }
            }
        }

        if (!arc)
            pExec->fLXMapsLoaded = TRUE;
        else
            doshFreeLXMaps(pExec);
    }

    return (arc);
}

/*
 *@@ doshFreeLXMaps:
 *      frees only the LX maps allocated by doshLoadLXMaps.
 *      This gets called automatically by doshExecClose.
 *
 *@@added V0.9.16 (2001-12-08) [umoeller]
 */

VOID doshFreeLXMaps(PEXECUTABLE pExec)
{
    FREE(pExec->pRsTbl);
    FREE(pExec->pObjTbl);
    FREE(pExec->pObjPageTbl);
    pExec->fLXMapsLoaded = FALSE;
}

/*
 *@@ doshLoadOS2NEMaps:
 *
 *      This works only if
 *
 *      --  ulExeFormat == EXEFORMAT_NE and
 *
 *      --  ulOS == EXEOS_OS2,
 *
 *      but not with Win16 NE executables.
 *
 *@@added V0.9.16 (2001-12-08) [umoeller]
 */

APIRET doshLoadOS2NEMaps(PEXECUTABLE pExec)
{
    APIRET arc;

    PNEHEADER pNEHeader;

    if (!pExec)
        arc = ERROR_INVALID_PARAMETER;
    else if (pExec->fOS2NEMapsLoaded)
        // already loaded:
        arc = NO_ERROR;
    else if (    (pExec->ulExeFormat != EXEFORMAT_NE)
              || (pExec->ulOS != EXEOS_OS2)
              || (!(pNEHeader = pExec->pNEHeader))
            )
        arc = ERROR_INVALID_EXE_SIGNATURE;
    else
    {
        ULONG ulNewHeaderOfs = 0;
        ULONG cb;

        if (pExec->pDosExeHeader)
            // executable has DOS stub: V0.9.12 (2001-05-03) [umoeller]
            ulNewHeaderOfs = pExec->pDosExeHeader->ulNewHeaderOfs;

        // resource table
        if (    (!(arc = doshAllocArray(pNEHeader->usResSegmCount,
                                        sizeof(OS2NERESTBLENTRY),
                                        (PBYTE*)&pExec->paOS2NEResTblEntry,
                                        &cb)))
             && (!(arc = doshReadAt(pExec->hfExe,
                                    pNEHeader->usResTblOfs
                                      + ulNewHeaderOfs,
                                    FILE_BEGIN,
                                    &cb,
                                    (PBYTE)pExec->paOS2NEResTblEntry)))
            )
        {
            // resource segments
            if (    (!(arc = doshAllocArray(pNEHeader->usResSegmCount,
                                            sizeof(OS2NESEGMENT),
                                            (PBYTE*)&pExec->paOS2NESegments,
                                            &cb)))
                 && (!(arc = doshReadAt(pExec->hfExe,
                                        pNEHeader->usResTblOfs
                                          + ulNewHeaderOfs
                                          - cb, // pNEHeader->usResSegmCount * sizeof(struct new_seg)
                                        FILE_BEGIN,
                                        &cb,
                                        (PBYTE)pExec->paOS2NESegments)))
                )
            {
            }
        }

        if (!arc)
            pExec->fOS2NEMapsLoaded = TRUE;
        else
            doshFreeNEMaps(pExec);
    }

    return (arc);
}

/*
 *@@ doshFreeNEMaps:
 *
 *@@added V0.9.16 (2001-12-08) [umoeller]
 */

VOID doshFreeNEMaps(PEXECUTABLE pExec)
{
    FREE(pExec->paOS2NEResTblEntry);
    FREE(pExec->paOS2NESegments);
    pExec->fOS2NEMapsLoaded = FALSE;
}

/*
 *@@ doshExecClose:
 *      this closes an executable opened with doshExecOpen.
 *      Always call this function if NO_ERROR was returned by
 *      doshExecOpen.
 *
 *      This automaticall calls doshFreeLXMaps.
 *
 *@@added V0.9.0 [umoeller]
 *@@changed V0.9.16 (2001-12-08) [umoeller]: fixed memory leaks
 *@@changed V0.9.16 (2001-12-08) [umoeller]: changed prototype to null the pExec ptr
 */

APIRET doshExecClose(PEXECUTABLE *ppExec)
{
    APIRET arc = NO_ERROR;
    PEXECUTABLE pExec;
    if (    (ppExec)
         && (pExec = *ppExec)
       )
    {
        char **papsz[] =
            {
                (char**)&pExec->pDosExeHeader,
                (char**)&pExec->pNEHeader,
                (char**)&pExec->pLXHeader,
                (char**)&pExec->pPEHeader,

                &pExec->pszDescription,
                &pExec->pszVendor,
                &pExec->pszVersion,
                &pExec->pszInfo,

                &pExec->pszBuildDateTime,
                &pExec->pszBuildMachine,
                &pExec->pszASD,
                &pExec->pszLanguage,
                &pExec->pszCountry,
                &pExec->pszRevision,
                &pExec->pszUnknown,
                &pExec->pszFixpak
            };
        ULONG ul;

        doshFreeLXMaps(pExec);
        doshFreeNEMaps(pExec);

        // fixed the memory leaks with the missing fields,
        // turned this into a loop
        for (ul = 0;
             ul < sizeof(papsz) / sizeof(papsz[0]);
             ul++)
        {
            PSZ pThis;
            if (pThis = *papsz[ul])
            {
                free(pThis);
                pThis = NULL;
            }
        }

        if (pExec->hfExe)
            arc = DosClose(pExec->hfExe);

        free(pExec);
        *ppExec = NULL;
    }
    else
        arc = ERROR_INVALID_PARAMETER;

    return (arc);
}

/*
 *@@ CopyToBuffer:
 *      little helper for copying a string to
 *      a target buffer with length checking.
 *
 *      Returns:
 *
 *      --  NO_ERROR
 *
 *      --  ERROR_BUFFER_OVERFLOW if pszTarget does
 *          not have enough room to hold pcszSource
 *          (including the null terminator).
 *
 *@@added V0.9.16 (2001-10-08) [umoeller]
 */

APIRET CopyToBuffer(PSZ pszTarget,      // out: target buffer
                    PCSZ pcszSource,    // in: source string
                    ULONG cbTarget)     // in: size of target buffer
{
    ULONG ulLength = strlen(pcszSource);
    if (ulLength < cbTarget)
    {
        memcpy(pszTarget,
               pcszSource,
               ulLength + 1);
        return (NO_ERROR);
    }

    return(ERROR_BUFFER_OVERFLOW);
}

/*
 *@@ doshSearchPath:
 *      replacement for DosSearchPath.
 *
 *      This looks along all directories which are
 *      specified in the value of the given environment
 *      variable if pcszFile is found.
 *
 *      As opposed to the stupid DosSearchPath, this
 *      ignores subdirectories in the path particles.
 *      For example, DosSearchPath would usually not
 *      find an INSTALL file because \OS2 contains
 *      an INSTALL directory, or NETSCAPE because
 *      \OS2\INSTALL contains a NETSCAPE directory.
 *
 *      Returns:
 *
 *      --  NO_ERROR: pszExecutable has received the
 *          full path of pcszFile.
 *
 *      --  ERROR_FILE_NOT_FOUND: pcszFile was not found
 *          in the specified path (or is a directory).
 *
 *      --  ERROR_BUFFER_OVERFLOW: pcszFile was found, but
 *          the pszExecutable buffer is too small to hold
 *          the full path.
 *
 *@@added V0.9.16 (2001-10-08) [umoeller]
 */

APIRET doshSearchPath(const char *pcszPath,     // in: path variable name (e.g. "PATH")
                      const char *pcszFile,     // in: file to look for (e.g. "LVM.EXE")
                      PSZ pszExecutable,        // out: full path (e.g. "F:\os2\lvm.exe")
                      ULONG cbExecutable)       // in: sizeof (*pszExecutable)
{
    APIRET arc = NO_ERROR;

    // get the PATH value
    PCSZ pcszPathValue;
    if (!(arc = DosScanEnv((PSZ)pcszPath,
#if __cplusplus
                           &pcszPathValue)))
#else
                           (PSZ*)&pcszPathValue)))
#endif
    {
        // run thru the path components
        PSZ pszPathCopy;
        if (pszPathCopy = strdup(pcszPathValue))
        {
            PSZ pszToken = strtok(pszPathCopy, ";");
            while (pszToken)
            {
                CHAR szFileMask[2*CCHMAXPATH];
                FILESTATUS3 fs3;

                sprintf(szFileMask,
                        "%s\\%s",
                        pszToken,           // path particle
                        pcszFile);          // e.g. "netscape"

                if (    (!(arc = DosQueryPathInfo(szFileMask,
                                                  FIL_STANDARD,
                                                  &fs3,
                                                  sizeof(fs3))))
                     // make sure it's not a directory
                     // and that it's not hidden
                     && (!(fs3.attrFile & (FILE_DIRECTORY | FILE_HIDDEN)))
                   )
                {
                    // copy
                    arc = CopyToBuffer(pszExecutable,
                                       szFileMask,
                                       cbExecutable);
                    // and stop
                    break;
                }
                else
                    arc = ERROR_FILE_NOT_FOUND;
                    // and search on

                pszToken = strtok(NULL, ";");
            };

            free(pszPathCopy);
        }
        else
            arc = ERROR_NOT_ENOUGH_MEMORY;
    }

    return (arc);
}

/*
 * FindFile:
 *      helper for doshFindExecutable.
 *
 *added V0.9.11 (2001-04-25) [umoeller]
 *@@changed V0.9.16 (2001-10-08) [umoeller]: rewrote second half for DosSearchPath replacement, which returns directories too
 */

APIRET FindFile(const char *pcszCommand,      // in: command (e.g. "lvm")
                PSZ pszExecutable,            // out: full path (e.g. "F:\os2\lvm.exe")
                ULONG cbExecutable)           // in: sizeof (*pszExecutable)
{
    APIRET arc = NO_ERROR;
    FILESTATUS3 fs3;

    if (    (strchr(pcszCommand, '\\'))
         || (strchr(pcszCommand, ':'))
       )
    {
        // looks like this is qualified:
        arc = DosQueryPathInfo((PSZ)pcszCommand,
                               FIL_STANDARD,
                               &fs3,
                               sizeof(fs3));
        if (!arc)
            if (!(fs3.attrFile & FILE_DIRECTORY))
                arc = CopyToBuffer(pszExecutable,
                                   pcszCommand,
                                   cbExecutable);
            else
                // directory:
                arc = ERROR_INVALID_EXE_SIGNATURE;
    }
    else
    {
        // non-qualified:
        /* arc = DosSearchPath(SEARCH_IGNORENETERRS
                                | SEARCH_ENVIRONMENT
                                | SEARCH_CUR_DIRECTORY,
                            "PATH",
                            (PSZ)pcszCommand,
                            pszExecutable,
                            cbExecutable); */
            // The above is not useable. It returns directories
            // on the path... for example, it returns \OS2\INSTALL\NETSCAPE
            // if netscape is looked for. So we search manually... sigh.
            // V0.9.16 (2001-10-08) [umoeller]
        arc = doshSearchPath("PATH",
                             pcszCommand,
                             pszExecutable,
                             cbExecutable);
    }

    return (arc);
}

/*
 *@@ doshFindExecutable:
 *      this attempts to find an executable by doing the
 *      following:
 *
 *      1)  If pcszCommand appears to be qualified (i.e. contains
 *          a backslash), this checks for whether the file exists.
 *          If it is a directory, ERROR_INVALID_EXE_SIGNATURE is
 *          returned.
 *
 *      2)  If pcszCommand contains no backslash, this searches
 *          all directories on the PATH in order to find the full
 *          path of the executable. Starting with V0.9.16, we
 *          use doshSearchPath for that.
 *
 *      papcszExtensions determines if additional searches are to be
 *      performed if the file doesn't exist (case 1) or doshSearchPath
 *      returned ERROR_FILE_NOT_FOUND (case 2).
 *      This must point to an array of strings specifying the extra
 *      extensions to search for.
 *
 *      If both papcszExtensions and cExtensions are null, no
 *      extra searches are performed.
 *
 *      Returns:
 *
 *      --  NO_ERROR: pszExecutable has received the full path of
 *          the executable found by DosSearchPath.
 *
 *      --  ERROR_FILE_NOT_FOUND
 *
 *      --  ERROR_BUFFER_OVERFLOW: pcszCommand was found, but
 *          the pszExecutable buffer is too small to hold
 *          the full path.
 *
 *      Example:
 *
 +      const char *aExtensions[] = {  "EXE",
 +                                     "COM",
 +                                     "CMD"
 +                                  };
 +      CHAR szExecutable[CCHMAXPATH];
 +      APIRET arc = doshFindExecutable("lvm",
 +                                      szExecutable,
 +                                      sizeof(szExecutable),
 +                                      aExtensions,
 +                                      3);
 *
 *@@added V0.9.9 (2001-03-07) [umoeller]
 *@@changed V0.9.11 (2001-04-25) [umoeller]: this never worked for qualified pcszCommand's, fixed
 */

APIRET doshFindExecutable(const char *pcszCommand,      // in: command (e.g. "lvm")
                          PSZ pszExecutable,            // out: full path (e.g. "F:\os2\lvm.exe")
                          ULONG cbExecutable,           // in: sizeof (*pszExecutable)
                          const char **papcszExtensions, // in: array of extensions (without dots)
                          ULONG cExtensions)            // in: array item count
{
    APIRET arc = FindFile(pcszCommand,
                          pszExecutable,
                          cbExecutable);

    if (    (arc == ERROR_FILE_NOT_FOUND)           // not found?
         && (cExtensions)                    // any extra searches wanted?
       )
    {
        // try additional things then
        PSZ psz2 = (PSZ)malloc(strlen(pcszCommand) + 20);
        if (psz2)
        {
            ULONG   ul;
            for (ul = 0;
                 ul < cExtensions;
                 ul++)
            {
                const char *pcszExtThis = papcszExtensions[ul];
                sprintf(psz2,
                        "%s.%s",
                        pcszCommand,
                        pcszExtThis);
                arc = FindFile(psz2,
                               pszExecutable,
                               cbExecutable);
                if (arc != ERROR_FILE_NOT_FOUND)
                    break;
            }

            free(psz2);
        }
        else
            arc = ERROR_NOT_ENOUGH_MEMORY;
    }

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
 *@@added V0.9.0 [umoeller]
 */

UINT doshQueryDiskCount(VOID)
{
    USHORT usCount = 0;
    DosPhysicalDisk(INFO_COUNT_PARTITIONABLE_DISKS, &usCount, 2, 0, 0);
    return (usCount);
}

/*
 *@@ doshType2FSName:
 *      this returns a static, zero-terminated string
 *      for the given FS type, or NULL if the type
 *      is unknown.
 *
 *@@added V0.9.0 [umoeller]
 *@@changed V0.9.16 (2001-10-08) [umoeller]: rewritten
 */

const char* doshType2FSName(unsigned char bFSType)  // in: FS type
{
    switch (bFSType)
    {
        case 0x00: return "empty";
        case 0x01: return "DOS 12-bit FAT < 10 Mb";
        case 0x02: return "XENIX root file system";
        case 0x03: return "XENIX /usr file system (obsolete)";
        case 0x04: return "DOS 16-bit FAT < 32 Mb";
        case 0x05: return "DOS 3.3+ extended partition";
        case 0x06: return "DOS 3.31+ 16-bit FAT > 32 Mb";
        case 0x07: return "HPFS/NTFS/QNX/Advanced Unix";
        case 0x08: return "OS/2 1.0-1.3/AIX/Commodore/DELL";
        case 0x09: return "AIX data/Coherent";
        case 0x0A: return "OS/2 Boot Manager/OPUS/Coherent Swap";
        case 0x0B: return "Windows95 with 32-bit FAT";
        case 0x0C: return "Windows95 with 32-bit FAT (LBA)";
        case 0x0E: return "Windows 95 VFAT (06h plus LBA)";
        case 0x0F: return "Windows 95 VFAT (05h plus LBA)";
        case 0x10: return "OPUS";
        case 0x11: return "OS/2 Boot Manager hidden 12-bit FAT";
        case 0x12: return "Compaq Diagnostics";
        case 0x14: return "OS/2 Boot Manager hidden sub-32M 16-bit FAT";
        case 0x16: return "OS/2 Boot Manager hidden over-32M 16-bit FAT";
        case 0x17: return "OS/2 Boot Manager hidden HPFS";
        case 0x18: return "AST special Windows swap file (\"Zero-Volt Suspend\")";
        // case 0x21: reserved
        // case 0x23: reserved
        case 0x24: return "NEC MS-DOS 3.x";
        // case 0x26: reserved
        // case 0x31: reserved
        // case 0x33: reserved
        // case 0x34: reserved
        // case 0x36: reserved
        case 0x38: return "Theos";
        case 0x3C: return "PowerQuest PartitionMagic recovery partition";
        case 0x40: return "VENIX 80286";
        case 0x41: return "Personal RISC Boot";
        case 0x42: return "SFS (Secure File System) by Peter Gutmann";
        case 0x50: return "OnTrack Disk Manager, read-only";
        case 0x51: return "OnTrack Disk Manager, read/write";
        case 0x52: return "CP/M or Microport System V/386";
        case 0x53: return "OnTrack Disk Manager, write-only???";
        case 0x54: return "OnTrack Disk Manager (DDO)";
        case 0x56: return "GoldenBow VFeature";
        case 0x61: return "SpeedStor";
        case 0x63: return "Unix SysV/386, 386/ix or Mach, MtXinu BSD 4.3 on Mach or GNU HURD";
        case 0x64: return "Novell NetWare 286";
        case 0x65: return "Novell NetWare (3.11)";
        case 0x67:
        case 0x68:
        case 0x69: return "Novell";
        case 0x70: return "DiskSecure Multi-Boot";
        // case 0x71: reserved
        // case 0x73: reserved
        // case 0x74: reserved
        case 0x75: return "PC/IX";
        // case 0x76: reserved
        case 0x80: return "Minix v1.1 - 1.4a";
        case 0x81: return "Minix v1.4b+ or Linux or Mitac Advanced Disk Manager";
        case 0x82: return "Linux Swap or Prime";
        case 0x83: return "Linux native file system (ext2fs/xiafs)";
        case 0x84: return "OS/2-renumbered type 04h (hidden DOS C: drive)";
        case 0x86: return "FAT16 volume/stripe set (Windows NT)";
        case 0x87: return "HPFS Fault-Tolerant mirrored partition or NTFS volume/stripe set";
        case 0x93: return "Amoeba file system";
        case 0x94: return "Amoeba bad block table";
        case 0xA0: return "Phoenix NoteBIOS Power Management \"Save-to-Disk\" partition";
        // case 0xA1: reserved
        // case 0xA3: reserved
        // case 0xA4: reserved
        case 0xA5: return "FreeBSD, BSD/386";
        // case 0xA6: reserved
        // case 0xB1: reserved
        // case 0xB3: reserved
        // case 0xB4: reserved
        // case 0xB6: reserved
        case 0xB7: return "BSDI file system (secondarily swap)";
        case 0xB8: return "BSDI swap (secondarily file system)";
        case 0xC1: return "DR DOS 6.0 LOGIN.EXE-secured 12-bit FAT";
        case 0xC4: return "DR DOS 6.0 LOGIN.EXE-secured 16-bit FAT";
        case 0xC6: return "DR DOS 6.0 LOGIN.EXE-secured Huge partition or NT corrupted FAT16 volume/stripe set";
        case 0xC7: return "Syrinx Boot or corrupted NTFS volume/stripe set";
        case 0xD8: return "CP/M-86";
        case 0xDB: return "CP/M, Concurrent CP/M, Concurrent DOS, Convergent Technologies OS";
        case 0xE1: return "SpeedStor 12-bit FAT extended partition";
        case 0xE3: return "DOS read-only or Storage Dimensions";
        case 0xE4: return "SpeedStor 16-bit FAT extended partition";
        // case 0xE5: reserved
        // case 0xE6: reserved
        case 0xF1: return "Storage Dimensions";
        case 0xF2: return "DOS 3.3+ secondary partition";
        // case 0xF3: reserved
        case 0xF4: return "SpeedStor or Storage Dimensions";
        // case 0xF6: reserved
        case 0xFE: return "LANstep or IBM PS/2 IML";
        case 0xFF: return "Xenix bad block table";
    }

    return NULL;
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
 *@@added V0.9.0 [umoeller]
 */

APIRET AppendPartition(PARTITIONINFO **pppiFirst,
                       PARTITIONINFO **pppiThis,    // in/out: partition info; pointer will be advanced
                       PUSHORT posCount,            // in/out: partition count
                       BYTE bDisk,                  // in: disk of partition
                       const char *pszBootName,     // in: boot partition name
                       CHAR cLetter,                // in/out: drive letter
                       BYTE bFsType,                // in: file system type
                       BOOL fPrimary,               // in: primary?
                       BOOL fBootable,
                       ULONG ulSectors)             // in: no. of sectors
{
    APIRET arc = NO_ERROR;
    PPARTITIONINFO ppiNew = NEW(PARTITIONINFO);
    if (ppiNew)
    {
        ZERO(ppiNew);

        // store data
        ppiNew->bDisk = bDisk;
        if ((fBootable) && (pszBootName) )
        {
            memcpy(ppiNew->szBootName, pszBootName, 8);
            ppiNew->szBootName[8] = 0;
        }
        else
            ppiNew->szBootName[0] = 0;
        ppiNew->cLetter = cLetter;
        ppiNew->bFSType = bFsType;
        ppiNew->pcszFSType = doshType2FSName(bFsType);
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

#ifndef __XWPLITE__

/*
 *@@ doshReadSector:
 *      reads a physical disk sector.
 *
 *      If NO_ERROR is returned, the sector contents
 *      have been stored in *buff.
 *
 *      Originally contributed by Dmitry A. Steklenev.
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
 *      Originally contributed by Dmitry A. Steklenev.
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
 *      Originally contributed by Dmitry A. Steklenev.
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
 *      disks to find the boot manager partition.
 *
 *      Returns:
 *
 *      -- NO_ERROR: boot manager found; in that case,
 *                   information about the boot manager
 *                   is written into *pusDisk, *pusPart,
 *                   *BmInfo. Any of these pointers can
 *                   be NULL if you're not interested.
 *
 *      -- ERROR_NOT_SUPPORTED (50): boot manager not installed.
 *
 *      Originally contributed by Dmitry A. Steklenev.
 *
 *@@added V0.9.0 [umoeller]
 */

APIRET doshGetBootManager(USHORT   *pusDisk,    // out: if != NULL, boot manager disk (1, 2, ...)
                          USHORT   *pusPart,    // out: if != NULL, index of bmgr primary partition (0-3)
                          PAR_INFO *pBmInfo)    // out: if != NULL, boot manager partition info
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
        if ((arc = doshReadSector(usDisk,
                                  &MBoot,
                                  0,            // head
                                  0,            // cylinder
                                  1)))          // sector
            return (arc);

        // scan primary partitions for whether
        // BootManager partition exists
        for (usPrim = 0; usPrim < 4; usPrim++)
        {
            if (MBoot.sPrtnInfo[usPrim].bFileSysCode == 0x0A)
            {
                // this is boot manager:
                if (pBmInfo)
                    *pBmInfo = MBoot.sPrtnInfo[usPrim];
                if (pusPart)
                    *pusPart = usPrim;
                if (pusDisk)
                    *pusDisk = usDisk;
                // stop scanning
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
 *      Returns:
 *
 *      -- ERROR_INVALID_PARAMETER: BMInfo is NULL.
 *
 *      Originally contributed by Dmitry A. Steklenev.
 *
 *@@added V0.9.0 [umoeller]
 */

APIRET GetPrimaryPartitions(PARTITIONINFO **pppiFirst,
                            PARTITIONINFO **pppiThis,
                            PUSHORT posCount,       // in/out: partition count
                            PCHAR pcLetter,         // in/out: drive letter counter
                            UINT BmDisk,            // in: physical disk (1, 2, 3, ...) of boot manager or null
                            PAR_INFO* pBmInfo,      // in: info returned by doshGetBootManager or NULL
                            UINT iDisk)             // in: system's physical disk count
{
    APIRET  arc = NO_ERROR;

    if (!pBmInfo)
        arc = ERROR_INVALID_PARAMETER;
    else
    {
        SYS_INFO        MName[32];  // Name Space from Boot Manager
        memset(&MName, 0, sizeof(MName));

        // read boot manager name table;
        // this is in the boot manager primary partition
        // at sector offset 3 (?!?)
        if (!(arc = doshReadSector(BmDisk,
                                   &MName,
                                   // head, cylinder, sector of bmgr primary partition:
                                   pBmInfo->bBeginHead,
                                   GetCyl(pBmInfo->rBeginSecCyl),
                                   GetSec(pBmInfo->rBeginSecCyl) + 3)))
        {
            // got bmgr name table:
            MBR_INFO        MBoot;      // Master Boot
            USHORT          i;

            // read master boot record of this disk
            if (!(arc = doshReadSector(iDisk,
                                       &MBoot,
                                       0,           // head
                                       0,           // cylinder
                                       1)))         // sector
            {
                for (i = 0;
                     i < 4;     // there can be only four primary partitions
                     i++)
                {
                    // skip unused partition, BootManager or Extended partition
                    if (    (MBoot.sPrtnInfo[i].bFileSysCode)  // skip unused
                        &&  (MBoot.sPrtnInfo[i].bFileSysCode != 0x0A) // skip boot manager
                        &&  (MBoot.sPrtnInfo[i].bFileSysCode != 0x05) // skip extended partition
                       )
                    {
                        BOOL fBootable = (    (pBmInfo)
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
 *      Originally contributed by Dmitry A. Steklenev.
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
             && (MBoot.sPrtnInfo[i].bFileSysCode != 0x0A)
           )
        {
            BOOL    fBootable = FALSE;
            BOOL    fAssignLetter = FALSE;

            // special work around extended partition
            if (MBoot.sPrtnInfo[i].bFileSysCode == 0x05)
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
            if (    (MBoot.sPrtnInfo[i].bFileSysCode < 0x75)
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
 *      Originally contributed by Dmitry A. Steklenev.
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
        if (MBoot.sPrtnInfo[i].bFileSysCode == 0x05)
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
 *@@ ReadFDiskPartitions:
 *      helper for doshGetPartitionsList for non-LVM
 *      systems.
 *
 *      Originally contributed by Dmitry A. Steklenev.
 *
 *@@added V0.9.16 (2001-10-08) [umoeller]
 */

APIRET ReadFDiskPartitions(PARTITIONINFO **ppPartitionInfos,
                           USHORT *pcPartitions,
                           PUSHORT pusContext)              // out: error context
{
    APIRET          arc = NO_ERROR;

    PAR_INFO        BmInfo;     // BootManager partition
    USHORT          usBmDisk;     // BootManager disk
    USHORT          cDisks = doshQueryDiskCount();    // physical disks count
    USHORT          i;

    CHAR            cLetter = 'C';  // first drive letter

    PARTITIONINFO   *ppiTemp = NULL;

    if (cDisks > 8)              // Not above 8 disks
        cDisks = 8;

    // get boot manager disk and info
    if ((arc = doshGetBootManager(&usBmDisk,
                                  NULL,
                                  &BmInfo)) != NO_ERROR)
    {
        *pusContext = 1;
    }
    else
    {
        // on each disk, read primary partitions
        for (i = 1; i <= cDisks; i++)
        {
            if ((arc = GetPrimaryPartitions(ppPartitionInfos,
                                            &ppiTemp,
                                            pcPartitions,
                                            &cLetter,
                                            usBmDisk,
                                            usBmDisk ? &BmInfo : 0,
                                            i)))
            {
                *pusContext = 2;
            }
        }

        if (!arc && usBmDisk)
        {
            // boot manager found:
            // on each disk, read extended partition
            // with logical drives
            for (i = 1; i <= cDisks; i++)
            {
                if ((arc = GetExtendedPartition(ppPartitionInfos,
                                                &ppiTemp,
                                                pcPartitions,
                                                &cLetter,
                                                &BmInfo,
                                                i)))
                {
                    *pusContext = 3;
                }
            }
        }
    } // end else if ((arc = doshGetBootManager(&usBmDisk,

    return (arc);
}

#endif

/*
 *@@ CleanPartitionInfos:
 *
 *@@added V0.9.9 (2001-04-07) [umoeller]
 */

VOID CleanPartitionInfos(PPARTITIONINFO ppiThis)
{
    while (ppiThis)
    {
        PPARTITIONINFO ppiNext = ppiThis->pNext;
        free(ppiThis);
        ppiThis = ppiNext;
    }
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
 *      What this function returns depends on whether
 *      LVM is installed.
 *
 *      --  If LVM.DLL is found on the LIBPATH, this opens
 *          the LVM engine and returns the info from the
 *          LVM engine in the PARTITIONINFO structures.
 *          The partitions are then sorted by disk in
 *          ascending order.
 *
 *      --  Otherwise, we parse the partition tables
 *          manually. The linked list then starts out with
 *          all the primary partitions, followed by the
 *          logical drives in the extended partitions.
 *          This function attempts to guess the correct drive
 *          letters and stores these with the PARTITIONINFO
 *          items, but there's no guarantee that this is
 *          correct. We correctly ignore Linux partitions here
 *          and give all primary partitions the C: letter, but
 *          I have no idea what happens with NTFS partitions,
 *          since I have none.
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
 *      --  0: something else.
 *
 *      Originally contributed by Dmitry A. Steklenev.
 *
 *@@added V0.9.0 [umoeller]
 *@@changed V0.9.9 (2001-04-07) [umoeller]: added transparent LVM support; changed prototype
 *@@changed V0.9.9 (2001-04-07) [umoeller]: fixed memory leaks on errors
 */

APIRET doshGetPartitionsList(PPARTITIONSLIST *ppList,
                             PUSHORT pusContext)                // out: error context
{
    APIRET          arc = NO_ERROR;

    PLVMINFO        pLVMInfo = NULL;

    PARTITIONINFO   *pPartitionInfos = NULL; // linked list of all partitions
    USHORT          cPartitions = 0;        // bootable partition count

    if (!ppList)
        return (ERROR_INVALID_PARAMETER);

    if (!(arc = doshQueryLVMInfo(&pLVMInfo)))
    {
        // LVM installed:
        arc = doshReadLVMPartitions(pLVMInfo,         // in: LVM info
                                    &pPartitionInfos, // out: partitions array
                                    &cPartitions);      // out: partitions count
        // copied to output below

        if (arc)
        {
            // error: start over
            doshFreeLVMInfo(pLVMInfo);
            CleanPartitionInfos(pPartitionInfos);
            pPartitionInfos = NULL;
            cPartitions = 0;
        }
    }

#ifndef __XWPLITE__
    if (arc)
        // LVM not installed, or failed:
        // parse partitions manually
        arc = ReadFDiskPartitions(&pPartitionInfos,
                                  &cPartitions,
                                  pusContext);
#endif

    if (!arc)
    {
        // no error so far:
        *pusContext = 0;

        *ppList = NEW(PARTITIONSLIST);
        if (!(*ppList))
            arc = ERROR_NOT_ENOUGH_MEMORY;
        else
        {
            ZERO(*ppList);

            (*ppList)->pPartitionInfo = pPartitionInfos;
            (*ppList)->cPartitions = cPartitions;

            _Pmpf((__FUNCTION__ ": returning %d partitions", cPartitions));
        }
    }

    if (arc)
        CleanPartitionInfos(pPartitionInfos);

    _Pmpf((__FUNCTION__ ": exiting, arc = %d", arc));

    return (arc);
}

/*
 *@@ doshFreePartitionsList:
 *      this frees the resources allocated by
 *      doshGetPartitionsList.
 *
 *@@added V0.9.0 [umoeller]
 */

APIRET doshFreePartitionsList(PPARTITIONSLIST ppList)
{
    if (!ppList)
        return (ERROR_INVALID_PARAMETER);
    else
    {
        CleanPartitionInfos(ppList->pPartitionInfo);
        doshFreeLVMInfo(ppList->pLVMInfo);
        free(ppList);
    }

    return (NO_ERROR);
}

/********************************************************************
 *
 *   LVM declarations
 *
 ********************************************************************/

/*
 *@@category: Helpers\Control program helpers\Partitions info\Quick LVM Interface
 *      functions for transparently interfacing LVM.DLL.
 */

typedef unsigned char       BOOLEAN;
typedef unsigned short int  CARDINAL16;
typedef unsigned long       CARDINAL32;
typedef unsigned int        CARDINAL;
typedef unsigned long       DoubleWord;

#ifdef ADDRESS
#undef ADDRESS
#endif

typedef void* ADDRESS;

#pragma pack(1)

#define DISK_NAME_SIZE          20
#define FILESYSTEM_NAME_SIZE    20
#define PARTITION_NAME_SIZE     20
#define VOLUME_NAME_SIZE        20

/*
 *@@ Drive_Control_Record:
 *      invariant for a disk drive.
 *
 *@@added V0.9.9 (2001-04-07) [umoeller]
 */

typedef struct _Drive_Control_Record
{
    CARDINAL32   Drive_Number;                      // OS/2 Drive Number for this drive.
    CARDINAL32   Drive_Size;                        // The total number of sectors on the drive.
    DoubleWord   Drive_Serial_Number;               // The serial number assigned to this drive.  For info. purposes only.
    ADDRESS      Drive_Handle;                      // Handle used for operations on the disk that this record corresponds to.
    CARDINAL32   Cylinder_Count;                    // The number of cylinders on the drive.
    CARDINAL32   Heads_Per_Cylinder;                // The number of heads per cylinder for this drive.
    CARDINAL32   Sectors_Per_Track;                 // The number of sectors per track for this drive.
    BOOLEAN      Drive_Is_PRM;                      // Set to TRUE if this drive is a PRM.
    BYTE         Reserved[3];                       // Alignment.
} Drive_Control_Record;

/*
 *@@ Drive_Control_Array:
 *      returned by the Get_Drive_Control_Data function
 *
 *@@added V0.9.9 (2001-04-07) [umoeller]
 */

typedef struct _Drive_Control_Array
{
    Drive_Control_Record *   Drive_Control_Data;    // An array of drive control records.
    CARDINAL32               Count;                 // The number of entries in the array of drive control records.
} Drive_Control_Array;

/*
 *@@ Drive_Information_Record:
 *      defines the information that can be changed for a specific disk drive.
 *
 *@@added V0.9.9 (2001-04-07) [umoeller]
 */

typedef struct _Drive_Information_Record
{
    CARDINAL32   Total_Available_Sectors;           // The number of sectors on the disk which are not currently assigned to a partition.
    CARDINAL32   Largest_Free_Block_Of_Sectors;     // The number of sectors in the largest contiguous block of available sectors.
    BOOLEAN      Corrupt_Partition_Table;           // If TRUE, then the partitioning information found on the drive is incorrect!
    BOOLEAN      Unusable;                          // If TRUE, the drive's MBR is not accessible and the drive can not be partitioned.
    BOOLEAN      IO_Error;                          // If TRUE, then the last I/O operation on this drive failed!
    BOOLEAN      Is_Big_Floppy;                     // If TRUE, then the drive is a PRM formatted as a big floppy (i.e. the old style removable media support).
    char         Drive_Name[DISK_NAME_SIZE];        // User assigned name for this disk drive.
} Drive_Information_Record;

/*
 *@@ Partition_Information_Record:
 *
 *@@added V0.9.9 (2001-04-07) [umoeller]
 */

typedef struct _Partition_Information_Record
{
    ADDRESS      Partition_Handle;
            // The handle used to perform operations on this partition.
    ADDRESS      Volume_Handle;
            // If this partition is part of a volume, this will be the handle of
            // the volume.  If this partition is NOT part of a volume, then this
            // handle will be 0.
    ADDRESS      Drive_Handle;
            // The handle for the drive this partition resides on.
    DoubleWord   Partition_Serial_Number;
            // The serial number assigned to this partition.
    CARDINAL32   Partition_Start;
            // The LBA of the first sector of the partition.
    CARDINAL32   True_Partition_Size;
            // The total number of sectors comprising the partition.
    CARDINAL32   Usable_Partition_Size;
            // The size of the partition as reported to the IFSM.  This is the
            // size of the partition less any LVM overhead.
    CARDINAL32   Boot_Limit;
            // The maximum number of sectors from this block of free space that
            // can be used to create a bootable partition if you allocate from the
            // beginning of the block of free space.
    BOOLEAN      Spanned_Volume;
            // TRUE if this partition is part of a multi-partition volume.
    BOOLEAN      Primary_Partition;
            // True or False.  Any non-zero value here indicates that this partition
            // is a primary partition.  Zero here indicates that this partition is
            // a "logical drive" - i.e. it resides inside of an extended partition.
    BYTE         Active_Flag;
            // 80 = Partition is marked as being active.
            // 0 = Partition is not active.
    BYTE         OS_Flag;
            // This field is from the partition table.  It is known as the OS flag,
            // the Partition Type Field, Filesystem Type, and various other names.
            //      Values of interest
            //      If this field is: (values are in hex)
            //      07 = The partition is a compatibility partition formatted for use
            //      with an installable filesystem, such as HPFS or JFS.
            //      00 = Unformatted partition
            //      01 = FAT12 filesystem is in use on this partition.
            //      04 = FAT16 filesystem is in use on this partition.
            //      0A = OS/2 Boot Manager Partition
            //      35 = LVM partition
            //      84 = OS/2 FAT16 partition which has been relabeled by Boot Manager to "Hide" it.
    BYTE         Partition_Type;
            // 0 = Free Space
            // 1 = LVM Partition (Part of an LVM Volume.)
            // 2 = Compatibility Partition
            // All other values are reserved for future use.
    BYTE         Partition_Status;
            // 0 = Free Space
            // 1 = In Use - i.e. already assigned to a volume.
            // 2 = Available - i.e. not currently assigned to a volume.
    BOOLEAN      On_Boot_Manager_Menu;
            // Set to TRUE if this partition is not part of a Volume yet is on the
            // Boot Manager Menu.
    BYTE         Reserved;
            // Alignment.
    char         Volume_Drive_Letter;
            // The drive letter assigned to the volume that this partition is a part of.
    char         Drive_Name[DISK_NAME_SIZE];
            // User assigned name for this disk drive.
    char         File_System_Name[FILESYSTEM_NAME_SIZE];
            // The name of the filesystem in use on this partition, if it is known.
    char         Partition_Name[PARTITION_NAME_SIZE];
            // The user assigned name for this partition.
    char         Volume_Name[VOLUME_NAME_SIZE];
            // If this partition is part of a volume, then this will be the
            // name of the volume that this partition is a part of.  If this
            // record represents free space, then the Volume_Name will be
            // "FREE SPACE xx", where xx is a unique numeric ID generated by
            // LVM.DLL.  Otherwise it will be an empty string.
} Partition_Information_Record;

// The following defines are for use with the Partition_Type field in the
// Partition_Information_Record.
#define FREE_SPACE_PARTITION     0
#define LVM_PARTITION            1
#define COMPATIBILITY_PARTITION  2

// The following defines are for use with the Partition_Status field in the
// Partition_Information_Record.
#define PARTITION_IS_IN_USE      1
#define PARTITION_IS_AVAILABLE   2
#define PARTITION_IS_FREE_SPACE  0

/*
 *@@ Partition_Information_Array:
 *      returned by various functions in the LVM Engine.
 *
 *@@added V0.9.9 (2001-04-07) [umoeller]
 */

typedef struct _Partition_Information_Array
{
    Partition_Information_Record * Partition_Array; // An array of Partition_Information_Records.
    CARDINAL32                     Count;           // The number of entries in the Partition_Array.
} Partition_Information_Array;

/*
 *@@ Volume_Information_Record:
 *      variable information for a volume.
 *
 *@@added V0.9.9 (2001-04-07) [umoeller]
 */

typedef struct _Volume_Information_Record
{
    CARDINAL32 Volume_Size;
            // The number of sectors comprising the volume.
    CARDINAL32 Partition_Count;
            // The number of partitions which comprise this volume.
    CARDINAL32 Drive_Letter_Conflict;
            // 0   indicates that the drive letter preference for this volume is unique.
            // 1   indicates that the drive letter preference for this volume
            //       is not unique, but this volume got its preferred drive letter anyway.
            // 2   indicates that the drive letter preference for this volume
            //       is not unique, and this volume did NOT get its preferred drive letter.
            // 4   indicates that this volume is currently "hidden" - i.e. it has
            //       no drive letter preference at the current time.
    BOOLEAN    Compatibility_Volume;
            // TRUE if this is for a compatibility volume, FALSE otherwise.
    BOOLEAN    Bootable;
            // Set to TRUE if this volume appears on the Boot Manager menu, or if it is
            // a compatibility volume and its corresponding partition is the first active
            // primary partition on the first drive.
    char       Drive_Letter_Preference;
            // The drive letter that this volume desires to be.
    char       Current_Drive_Letter;
            // The drive letter currently used to access this volume.
            // May be different than Drive_Letter_Preference if there was a conflict ( i.e. Drive_Letter_Preference
            // is already in use by another volume ).
    char       Initial_Drive_Letter;
            // The drive letter assigned to this volume by the operating system
            // when LVM was started. This may be different from the
            // Drive_Letter_Preference if there were conflicts, and
            // may be different from the Current_Drive_Letter.  This
            // will be 0x0 if the Volume did not exist when the LVM Engine
            // was opened (i.e. it was created during this LVM session).
    BOOLEAN    New_Volume;
            // Set to FALSE if this volume existed before the LVM Engine was
            // opened.  Set to TRUE if this volume was created after the LVM
            // Engine was opened.
    BYTE       Status;
            // 0 = None.
            // 1 = Bootable
            // 2 = Startable
            // 3 = Installable.
    BYTE       Reserved_1;
    char       Volume_Name[VOLUME_NAME_SIZE];
            // The user assigned name for this volume.
    char       File_System_Name[FILESYSTEM_NAME_SIZE];
            // The name of the filesystem in use on this partition, if it
            // is known.
} Volume_Information_Record;

#pragma pack()

/********************************************************************
 *
 *   Quick LVM Interface API
 *
 ********************************************************************/

/*
 *@@ LVMINFOPRIVATE:
 *      private structure used by doshQueryLVMInfo.
 *      This is what the LVMINFO pointer really
 *      points to.
 *
 *@@added V0.9.9 (2001-04-07) [umoeller]
 */

typedef struct _LVMINFOPRIVATE
{
    LVMINFO             LVMInfo;            // public structure (dosh.h)

    // function pointers resolved from LVM.DLL

    void (* _System     Open_LVM_Engine)(BOOLEAN Ignore_CHS,
                                         CARDINAL32 *Error_Code);

    void (* _System     Free_Engine_Memory)(ADDRESS Object);

    void (* _System     Close_LVM_Engine)(void);

    Drive_Control_Array (* _System
                        Get_Drive_Control_Data)(CARDINAL32 *Error_Code);

    Drive_Information_Record (* _System
                        Get_Drive_Status)(ADDRESS Drive_Handle,
                                          CARDINAL32 *Error_Code);

    Partition_Information_Array (* _System
                        Get_Partitions)(ADDRESS Handle,
                                        CARDINAL32 *Error_Code);

    Volume_Information_Record (*_System
                        Get_Volume_Information)(ADDRESS Volume_Handle,
                                                CARDINAL32 *Error_Code);

} LVMINFOPRIVATE, *PLVMINFOPRIVATE;

#define LVM_ERROR_FIRST             20000

/*
 *@@ doshQueryLVMInfo:
 *      creates an LVMINFO structure if LVM is installed.
 *      Returns that structure (which the caller must free
 *      using doshFreeLVMInfo) or NULL if LVM.DLL was not
 *      found along the LIBPATH.
 *
 *@@added V0.9.9 (2001-04-07) [umoeller]
 */

APIRET doshQueryLVMInfo(PLVMINFO *ppLVMInfo)
{
    APIRET          arc = NO_ERROR;
    CHAR            szError[100];
    PLVMINFOPRIVATE pLVMInfo = NULL;
    HMODULE         hmodLVM = NULLHANDLE;

    if (!(arc = DosLoadModule(szError,
                              sizeof(szError),
                              "LVM",
                              &hmodLVM)))
    {
        // got LVM.DLL:
        pLVMInfo = NEW(LVMINFOPRIVATE);
        if (!pLVMInfo)
            arc = ERROR_NOT_ENOUGH_MEMORY;
        else
        {
            // array of function pointers to be resolved from LVM.DLL
            RESOLVEFUNCTION     aFunctions[] =
                    {
                        "Open_LVM_Engine", (PFN*)&pLVMInfo->Open_LVM_Engine,
                        "Free_Engine_Memory", (PFN*)&pLVMInfo->Free_Engine_Memory,
                        "Close_LVM_Engine", (PFN*)&pLVMInfo->Close_LVM_Engine,
                        "Get_Drive_Control_Data", (PFN*)&pLVMInfo->Get_Drive_Control_Data,
                        "Get_Drive_Status", (PFN*)&pLVMInfo->Get_Drive_Status,
                        "Get_Partitions", (PFN*)&pLVMInfo->Get_Partitions,
                        "Get_Volume_Information", (PFN*)&pLVMInfo->Get_Volume_Information
                    };
            ULONG               ul;

            ZERO(pLVMInfo);

            pLVMInfo->LVMInfo.hmodLVM = hmodLVM;

            // now resolve function pointers
            for (ul = 0;
                 ul < ARRAYITEMCOUNT(aFunctions);
                 ul++)
            {
                PRESOLVEFUNCTION pFuncThis = &aFunctions[ul];
                arc = DosQueryProcAddr(hmodLVM,
                                       0,               // ordinal, ignored
                                       (PSZ)pFuncThis->pcszFunctionName,
                                       pFuncThis->ppFuncAddress);
                if (!pFuncThis->ppFuncAddress)
                    arc = ERROR_INVALID_NAME;

                if (arc)
                    break;
            }
        }
    }

    if (arc)
        doshFreeLVMInfo((PLVMINFO)pLVMInfo);
    else
        *ppLVMInfo = (PLVMINFO)pLVMInfo;

    return (arc);
}

/*
 *@@ doshReadLVMPartitions:
 *      using the LVMINFO parameter from doshQueryLVMInfo,
 *      builds an array of PARTITIONINFO structures with
 *      the data returned from LVM.DLL.
 *
 *@@added V0.9.9 (2001-04-07) [umoeller]
 */

APIRET doshReadLVMPartitions(PLVMINFO pInfo,         // in: LVM info
                             PPARTITIONINFO *ppPartitionInfo, // out: partitions array
                             PUSHORT pcPartitions)      // out: partitions count
{
    APIRET          arc = NO_ERROR;
    CARDINAL32      Error = 0;

    PARTITIONINFO   *pPartitionInfos = NULL, // linked list of all partitions
                    *ppiTemp = NULL;
    USHORT          cPartitions = 0;        // bootable partition count
    PLVMINFOPRIVATE pLVMInfo = (PLVMINFOPRIVATE)pInfo;

    _Pmpf((__FUNCTION__ ": entering"));

    if (!pLVMInfo)
        return (ERROR_INVALID_PARAMETER);

    // initialize LVM engine
    pLVMInfo->Open_LVM_Engine(TRUE,
                              &Error);

    _Pmpf(("  Open_LVM_Engine Error: %d"));

    if (!Error)
    {
        Drive_Control_Array DCA = pLVMInfo->Get_Drive_Control_Data(&Error);
                        // member records to be freed

        _Pmpf(("  Get_Drive_Control_Data Error: %d, drive count: %d", Error, DCA.Count));

        if (    (!Error)
             && (DCA.Count)
           )
        {
            // DCA.Drive_Control_Data now contains drive information records;
            // this must be freed
            ULONG   ulDisk;

            for (ulDisk = 0;
                 ulDisk < DCA.Count;
                 ulDisk++)
            {
                Drive_Control_Record *pDriveControlRecord
                    = &DCA.Drive_Control_Data[ulDisk];
                ADDRESS hDrive = pDriveControlRecord->Drive_Handle;

                /* Drive_Information_Record pDriveInfoRecord
                    = pLVMInfo->Get_Drive_Status(hDrive,
                                                 &Error);

                _Pmpf(("  drive %d Get_Drive_Status Error: %d", ulDisk, Error));

                if (!Error) */
                {
                    Partition_Information_Array PIA
                        = pLVMInfo->Get_Partitions(hDrive,
                                                   &Error);

                    _Pmpf(("    Get_Partitions Error: %d", Error));

                    if (!Error)
                    {
                        // PIA.Partition_Array now contains
                        // Partition_Information_Record; must be freed

                        // now go thru partitions of this drive
                        ULONG ulPart;
                        for (ulPart = 0;
                             ulPart < PIA.Count;
                             ulPart++)
                        {
                            Partition_Information_Record *pPartition
                                = &PIA.Partition_Array[ulPart];
                            Volume_Information_Record VolumeInfo;

                            const char  *pcszBootName = NULL; // for now
                            BOOL        fBootable = FALSE;

                            if (pPartition->Volume_Handle)
                            {
                                // this partition is part of a volume:
                                // only then can it be bootable...
                                // get the volume info
                                VolumeInfo
                                    = pLVMInfo->Get_Volume_Information(pPartition->Volume_Handle,
                                                                       &Error);
                                pcszBootName = VolumeInfo.Volume_Name;

                                fBootable = (VolumeInfo.Status == 1);
                            }


                            if (arc = AppendPartition(&pPartitionInfos,
                                                      &ppiTemp,
                                                      &cPartitions,
                                                      ulDisk + 1,
                                                      pcszBootName,
                                                      pPartition->Volume_Drive_Letter,
                                                      pPartition->OS_Flag,  // FS type
                                                      pPartition->Primary_Partition,
                                                      fBootable,
                                                      pPartition->True_Partition_Size))
                                break;
                        }

                        // clean up partitions
                        pLVMInfo->Free_Engine_Memory(PIA.Partition_Array);
                    }
                }
                /* else
                    // error:
                    break; */
            }

            // clean up drive data
            pLVMInfo->Free_Engine_Memory(DCA.Drive_Control_Data);
        }
    }

    // close LVM
    pLVMInfo->Close_LVM_Engine();

    if (Error)
    {
        // if we got an error, return it with the
        // LVM error offset
        arc = LVM_ERROR_FIRST + Error;

        CleanPartitionInfos(pPartitionInfos);
    }

    if (!arc)
    {
        *ppPartitionInfo = pPartitionInfos;
        *pcPartitions = cPartitions;
    }

    _Pmpf((__FUNCTION__ ": exiting, arg = %d", arc));

    return (arc);
}

/*
 *@@ doshFreeLVMInfo:
 *
 *@@added V0.9.9 (2001-04-07) [umoeller]
 */

VOID doshFreeLVMInfo(PLVMINFO pInfo)
{
    if (pInfo)
    {
        if (pInfo->hmodLVM)
            DosFreeModule(pInfo->hmodLVM);

        free(pInfo);
    }
}
