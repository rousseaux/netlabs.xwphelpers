
/*
 *@@sourcefile dosh.c:
 *      dosh.c contains Control Program helper functions.
 *
 *      This file has miscellaneous system functions,
 *      drive helpers, file helpers, and partition functions.
 *
 *      Usage: All OS/2 programs.
 *
 *      Function prefixes (new with V0.81):
 *      --  dosh*   Dos (Control Program) helper functions
 *
 *      These funcs are forward-declared in dosh.h, which
 *      must be #include'd first.
 *
 *      The resulting dosh.obj object file can be linked
 *      against any application object file. As opposed to
 *      the code in dosh2.c, it does not require any other
 *      code from the helpers.
 *
 *      dosh.obj can also be used with the VAC subsystem
 *      library (/rn compiler option).
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@header "helpers\dosh.h"
 */

/*
 *      This file Copyright (C) 1997-2000 Ulrich M”ller.
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

#define INCL_KBD
#include <os2.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "setup.h"                      // code generation and debugging options

#include "helpers\dosh.h"

#pragma hdrstop

const CHAR  G_acDriveLetters[28] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ";

/*
 *@@category: Helpers\Control program helpers\Miscellaneous
 *      Miscellaneous helpers in dosh.c that didn't fit into any other
 *      category.
 */

/* ******************************************************************
 *
 *   Miscellaneous
 *
 ********************************************************************/

/*
 *@@ doshGetChar:
 *      reads a single character from the keyboard.
 *      Useful for VIO sessions, since there's great
 *      confusion between the various C dialects about
 *      how to use getc(), and getc() doesn't work
 *      with the VAC subsystem library.
 *
 *@@added V0.9.4 (2000-07-27) [umoeller]
 */

CHAR doshGetChar(VOID)
{
    // CHAR    c;
    // ULONG   ulRead = 0;

    KBDKEYINFO kki;
    KbdCharIn(&kki,
              0, // wait
              0);

    return (kki.chChar);
}

/*
 *@@ doshQueryShiftState:
 *      returns TRUE if any of the SHIFT keys are
 *      currently pressed. Useful for checks during
 *      WM_COMMAND messages from menus.
 *
 *@@changed V0.9.5 (2000-09-27) [umoeller]: added error checking
 */

BOOL doshQueryShiftState(VOID)
{
    BOOL            brc = FALSE;
    APIRET          arc = NO_ERROR;
    HFILE           hfKbd;
    ULONG           ulAction;

    arc = DosOpen("KBD$", &hfKbd, &ulAction, 0,
                  FILE_NORMAL, FILE_OPEN,
                  OPEN_ACCESS_READONLY | OPEN_SHARE_DENYWRITE,
                  (PEAOP2)NULL);
    if (arc == NO_ERROR)
    {
        SHIFTSTATE      ShiftState;
        ULONG           cbDataLen = sizeof(ShiftState);

        arc = DosDevIOCtl(hfKbd, IOCTL_KEYBOARD, KBD_GETSHIFTSTATE,
                          NULL, 0, NULL,      // no parameters
                          &ShiftState, cbDataLen, &cbDataLen);
        if (arc == NO_ERROR)
            brc = ((ShiftState.fsState & 3) != 0);

        DosClose(hfKbd);
    }

    return brc;
}


/*
 *@@ doshIsWarp4:
 *      returns TRUE only if at least OS/2 Warp 4 is running.
 *
 *@@changed V0.9.2 (2000-03-05) [umoeller]: reported TRUE on Warp 3 also; fixed
 *@@changed V0.9.6 (2000-10-16) [umoeller]: patched for speed
 */

BOOL doshIsWarp4(VOID)
{
    static BOOL s_brc = FALSE;
    static BOOL s_fQueried = FALSE;
    if (!s_fQueried)
    {
        // first call:
        ULONG       aulBuf[3];

        DosQuerySysInfo(QSV_VERSION_MAJOR,      // 11
                        QSV_VERSION_MINOR,      // 12
                        &aulBuf, sizeof(aulBuf));
        // Warp 3 is reported as 20.30
        // Warp 4 is reported as 20.40
        // Aurora is reported as 20.45

        if     (    (aulBuf[0] > 20)        // major > 20; not the case with Warp 3, 4, 5
                 || (   (aulBuf[0] == 20)   // major == 20 and minor >= 40
                     && (aulBuf[1] >= 40)
                    )
               )
            s_brc = TRUE;

        s_fQueried = TRUE;
    }

    return (s_brc);
}

/*
 *@@ doshQuerySysErrorMsg:
 *      this retrieves the error message for a system error
 *      (APIRET) from the system error message file (OSO001.MSG).
 *      This file better be on the DPATH (it normally is).
 *
 *      This returns the string in the "SYSxxx: blahblah" style,
 *      which is normally displayed on the command line when
 *      errors occur.
 *
 *      The error message is returned in a newly allocated
 *      buffer, which should be free()'d afterwards.
 *
 *      Returns NULL upon errors.
 */

PSZ doshQuerySysErrorMsg(APIRET arc)    // in: DOS error code
{
    PSZ     pszReturn = 0;
    CHAR    szDosError[1000];
    ULONG   cbDosError = 0;
    DosGetMessage(NULL, 0,       // no string replacements
                  szDosError, sizeof(szDosError),
                  arc,
                  "OSO001.MSG",        // default OS/2 message file
                  &cbDosError);
    if (cbDosError > 2)
    {
        szDosError[cbDosError - 2] = 0;
        pszReturn = strdup(szDosError);
    }
    return (pszReturn);
}

/*
 *@@category: Helpers\Control program helpers\Shared memory management
 *      helpers for allocating and requesting shared memory.
 */

/* ******************************************************************
 *
 *   Memory helpers
 *
 ********************************************************************/

/*
 *@@ doshAllocSharedMem:
 *      wrapper for DosAllocSharedMem which has
 *      a malloc()-like syntax. Just due to my
 *      lazyness.
 *
 *      Note that ulSize is always rounded up to the
 *      next 4KB value, so don't use this hundreds of times.
 *
 *      Returns NULL upon errors. Possible errors include
 *      that a memory block calle pcszName has already been
 *      allocated.
 *
 *      Use DosFreeMem(pvrc) to free the memory. The memory
 *      will only be freed if no other process has requested
 *      access.
 *
 *@@added V0.9.3 (2000-04-18) [umoeller]
 */

PVOID doshAllocSharedMem(ULONG ulSize,      // in: requested mem block size (rounded up to 4KB)
                         const char* pcszName) // in: name of block ("\\SHAREMEM\\xxx") or NULL
{
    PVOID   pvrc = NULL;
    APIRET  arc = DosAllocSharedMem((PVOID*)(&pvrc),
                                    (PSZ)pcszName,
                                    ulSize,
                                    PAG_COMMIT | PAG_READ | PAG_WRITE);
    if (arc == NO_ERROR)
        return (pvrc);

    return (NULL);
}

/*
 *@@ doshRequestSharedMem:
 *      requests access to a block of named shared memory
 *      allocated by doshAllocSharedMem.
 *
 *      Returns NULL upon errors.
 *
 *      Use DosFreeMem(pvrc) to free the memory. The memory
 *      will only be freed if no other process has requested
 *      access.
 *
 *@@added V0.9.3 (2000-04-19) [umoeller]
 */

PVOID doshRequestSharedMem(const char *pcszName)
{
    PVOID pvrc = NULL;
    APIRET arc = DosGetNamedSharedMem((PVOID*)(pvrc),
                                      (PSZ)pcszName,
                                      PAG_READ | PAG_WRITE);
    if (arc == NO_ERROR)
        return (pvrc);

    return (NULL);
}

/*
 *@@category: Helpers\Control program helpers\Drive management
 *      functions for managing drives... enumerating, testing,
 *      querying etc.
 */

/* ******************************************************************
 *
 *   Drive helpers
 *
 ********************************************************************/

/*
 *@@ doshEnumDrives:
 *      this function enumerates all valid drive letters on
 *      the system by composing a string of drive letters
 *      in the buffer pointed to by pszBuffer, which should
 *      be 27 characters in size to hold information for
 *      all drives. The buffer will be null-terminated.
 *
 *      If (pcszFileSystem != NULL), only drives matching
 *      the specified file system type (e.g. "HPFS") will
 *      be enumerated. If (pcszFileSystem == NULL), all
 *      drives will be enumerated.
 *
 *      If (fSkipRemovables == TRUE), removeable drives will
 *      be skipped. This applies to floppy, CD-ROM, and
 *      virtual floppy drives. This will start the search
 *      at drive letter C: so that drives A: and B: will
 *      never be checked (to avoid the hardware bumps).
 *
 *      Otherwise, the search starts at drive A:. Still,
 *      removeable drives will only be added if valid media
 *      is inserted.
 *
 *@@changed V0.9.4 (2000-07-03) [umoeller]: this stopped at the first invalid drive letter; fixed
 *@@changed V0.9.4 (2000-07-03) [umoeller]: added fSkipRemoveables
 */

VOID doshEnumDrives(PSZ pszBuffer,      // out: drive letters
                    const char *pcszFileSystem,  // in: FS's to match or NULL
                    BOOL fSkipRemoveables) // in: if TRUE, only non-removeable disks will be returned
{
    CHAR szName[5] = "";
    ULONG ulLogicalDrive = 1, // start with drive A:
          ulFound = 0;        // found drives count
    APIRET arc             = NO_ERROR; // return code

    if (fSkipRemoveables)
        // start with drive C:
        ulLogicalDrive = 3;

    // go thru the drives, start with C: (== 3), stop after Z: (== 26)
    while (ulLogicalDrive <= 26)
    {
        UCHAR nonRemovable=0;
        ULONG parmSize=2;
        ULONG dataLen=1;

        #pragma pack(1)
        struct
        {
            UCHAR dummy,drive;
        } parms;
        #pragma pack()

        parms.drive=(UCHAR)(ulLogicalDrive-1);
        arc = DosDevIOCtl((HFILE)-1,
                          IOCTL_DISK,
                          DSK_BLOCKREMOVABLE,
                          &parms,
                          parmSize,
                          &parmSize,
                          &nonRemovable,
                          1,
                          &dataLen);

        /* _Pmpf(("  ul = %d, Drive %c: arc = %d nonRemoveable = %d",
                    ulLogicalDrive,
                    G_acDriveLetters[ulLogicalDrive],
                    arc,
                    nonRemovable)); */

        if (    // fixed disk and non-removeable
                ((arc == NO_ERROR) && (nonRemovable))
                // or network drive:
             || (arc == ERROR_NOT_SUPPORTED)
           )
        {
            ULONG  ulOrdinal       = 0;     // ordinal of entry in name list
            BYTE   fsqBuffer[sizeof(FSQBUFFER2) + (3 * CCHMAXPATH)] = {0};
            ULONG  cbBuffer   = sizeof(fsqBuffer);        // Buffer length)
            PFSQBUFFER2 pfsqBuffer = (PFSQBUFFER2)fsqBuffer;

            szName[0] = G_acDriveLetters[ulLogicalDrive];
            szName[1] = ':';
            szName[2] = '\0';

            arc = DosQueryFSAttach(szName,          // logical drive of attached FS
                                   ulOrdinal,       // ignored for FSAIL_QUERYNAME
                                   FSAIL_QUERYNAME, // return data for a Drive or Device
                                   pfsqBuffer,      // returned data
                                   &cbBuffer);      // returned data length

            if (arc == NO_ERROR)
            {
                // The data for the last three fields in the FSQBUFFER2
                // structure are stored at the offset of fsqBuffer.szName.
                // Each data field following fsqBuffer.szName begins
                // immediately after the previous item.
                CHAR* pszFSDName = (PSZ)&(pfsqBuffer->szName) + (pfsqBuffer->cbName) + 1;
                if (pcszFileSystem == NULL)
                {
                    // enum-all mode: always copy
                    pszBuffer[ulFound] = szName[0]; // drive letter
                    ulFound++;
                }
                else if (strcmp(pszFSDName, pcszFileSystem) == 0)
                {
                    pszBuffer[ulFound] = szName[0]; // drive letter
                    ulFound++;
                }
            }
        }

        ulLogicalDrive++;
    } // end while (G_acDriveLetters[ulLogicalDrive] <= 'Z')

    pszBuffer[ulFound] = '\0';
}

/*
 *@@ doshQueryBootDrive:
 *      returns the letter of the boot drive as a
 *      single (capital) character, which is useful for
 *      constructing file names using sprintf and such.
 */

CHAR doshQueryBootDrive(VOID)
{
    ULONG ulBootDrive;
    DosQuerySysInfo(QSV_BOOT_DRIVE, QSV_BOOT_DRIVE,
                    &ulBootDrive,
                    sizeof(ulBootDrive));
    return (G_acDriveLetters[ulBootDrive]);
}

/*
 *@@ doshAssertDrive:
 *      this checks for whether the given drive
 *      is currently available without provoking
 *      those ugly white "Drive not ready" popups.
 *
 *      This returns (from my testing):
 *      -- NO_ERROR: drive is available.
 *      -- ERROR_INVALID_DRIVE (15): drive letter does not exist.
 *      -- ERROR_NOT_READY (21): drive exists, but is not ready
 *                  (e.g. CD-ROM drive without CD inserted).
 *      -- ERROR_NOT_SUPPORTED (50): this is returned by the RAMFS.IFS
 *                  file system; apparently, the IFS doesn't support
 *                  DASD opening.
 *
 *@@changed V0.9.1 (99-12-13) [umoeller]: rewritten, prototype changed. Now using DosOpen on the drive instead of DosError.
 *@@changed V0.9.1 (2000-01-08) [umoeller]: DosClose was called even if DosOpen failed, which messed up OS/2 error handling.
 *@@changed V0.9.1 (2000-02-09) [umoeller]: this didn't work for network drives, including RAMFS; fixed.
 *@@changed V0.9.3 (2000-03-28) [umoeller]: added check for network drives, which weren't working
 *@@changed V0.9.4 (2000-08-03) [umoeller]: more network fixes
 */

APIRET doshAssertDrive(ULONG ulLogicalDrive) // in: 1 for A:, 2 for B:, 3 for C:, ...
{
    CHAR    szDrive[3] = "C:";
    HFILE   hfDrive = 0;
    ULONG   ulTemp = 0;
    APIRET  arc;

    szDrive[0] = 'A' + ulLogicalDrive - 1;

    arc = DosOpen(szDrive,   // "C:", "D:", ...
                  &hfDrive,
                  &ulTemp,
                  0,
                  FILE_NORMAL,
                  OPEN_ACTION_FAIL_IF_NEW
                         | OPEN_ACTION_OPEN_IF_EXISTS,
                  OPEN_FLAGS_DASD
                         | OPEN_FLAGS_FAIL_ON_ERROR
                         | OPEN_FLAGS_NOINHERIT     // V0.9.6 (2000-11-25) [pr]
                         | OPEN_ACCESS_READONLY
                         | OPEN_SHARE_DENYNONE,
                  NULL);

    // _Pmpf((__FUNCTION__ ": DosOpen(OPEN_FLAGS_DASD) returned %d", arc));

    switch (arc)
    {
        case ERROR_NETWORK_ACCESS_DENIED: // 65
            // added V0.9.3 (2000-03-27) [umoeller];
            // according to user reports, this is returned
            // by all network drives, which apparently don't
            // support DASD DosOpen
        case ERROR_ACCESS_DENIED: // 5
            // added V0.9.4 (2000-07-10) [umoeller]
            // LAN drives still didn't work... apparently
            // the above only works for NFS drives
        case ERROR_PATH_NOT_FOUND: // 3
            // added V0.9.4 (2000-08-03) [umoeller]:
            // this is returned by some other network types...
            // sigh...
        case ERROR_NOT_SUPPORTED: // 50
        {
            // this is returned by file systems which don't
            // support DASD DosOpen;
            // use some other method then, this isn't likely
            // to fail -- V0.9.1 (2000-02-09) [umoeller]
            FSALLOCATE  fsa;
            arc = DosQueryFSInfo(ulLogicalDrive,
                                 FSIL_ALLOC,
                                 &fsa,
                                 sizeof(fsa));
        break; }

        case NO_ERROR:
            DosClose(hfDrive);
        break;
    }

    return (arc);

    /* FSALLOCATE  fsa;
    APIRET arc = NO_ERROR;

    if (fSuppress)
    {
        DosError(FERR_DISABLEHARDERR | FERR_DISABLEEXCEPTION);
        DosEnterCritSec();
    }

    arc = DosQueryFSInfo(ulLogicalDrive, FSIL_ALLOC, &fsa, sizeof(fsa));

    if (fSuppress)
    {
        DosError(FERR_ENABLEHARDERR | FERR_ENABLEEXCEPTION);
        DosExitCritSec();
    }

    return (arc); */
}

/*
 *@@ doshSetLogicalMap:
 *       sets the mapping of logical floppy drives onto a single
 *       physical floppy drive.
 *       This means selecting either drive A: or drive B: to refer
 *       to the physical drive.
 *
 *@@added V0.9.6 (2000-11-24) [pr]
 */

APIRET doshSetLogicalMap(ULONG ulLogicalDrive)
{
    CHAR    name[3] = "?:";
    ULONG   fd = 0,
            action = 0,
            paramsize = 0,
            datasize = 0;
    APIRET    rc = NO_ERROR;
    USHORT    data,
              param;

    name[0] = doshQueryBootDrive();
    rc = DosOpen(name,
                 &fd,
                 &action,
                 0,
                 0,
                 OPEN_ACTION_FAIL_IF_NEW
                          | OPEN_ACTION_OPEN_IF_EXISTS,
                 OPEN_FLAGS_DASD
                       | OPEN_FLAGS_FAIL_ON_ERROR
                       | OPEN_FLAGS_NOINHERIT
                       | OPEN_ACCESS_READONLY
                       | OPEN_SHARE_DENYNONE,
                 0);
    if (rc == NO_ERROR)
    {
        param = 0;
        data = (USHORT)ulLogicalDrive;
        paramsize = sizeof(param);
        datasize = sizeof(data);
        rc = DosDevIOCtl(fd,
                         IOCTL_DISK, DSK_SETLOGICALMAP,
                         &param, paramsize, &paramsize,
                         &data, datasize, &datasize);
        DosClose(fd);
    }

    return(rc);
}

/*
 *@@ doshQueryDiskFree:
 *       returns the number of bytes remaining on the disk
 *       specified by the given logical drive.
 *
 *       Note: This returns a "double" value, because a ULONG
 *       can only hold values of some 4 billion, which would
 *       lead to funny results for drives > 4 GB.
 *
 *@@changed V0.9.0 [umoeller]: fixed another > 4 GB bug (thanks to Rdiger Ihle)
 *@@changed V0.9.7 (2000-12-01) [umoeller]: changed prototype
 */

APIRET doshQueryDiskFree(ULONG ulLogicalDrive, // in: 1 for A:, 2 for B:, 3 for C:, ...
                         double *pdFree)
{
    APIRET      arc = NO_ERROR;
    FSALLOCATE  fsa;
    double      dbl = -1;

    arc = DosQueryFSInfo(ulLogicalDrive, FSIL_ALLOC, &fsa, sizeof(fsa));
    if (arc == NO_ERROR)
        *pdFree = ((double)fsa.cSectorUnit * fsa.cbSector * fsa.cUnitAvail);
                   // ^ fixed V0.9.0

    return (arc);
}

/*
 *@@ doshQueryDiskFSType:
 *       copies the file-system type of the given disk object
 *       (HPFS, FAT, CDFS etc.) to pszBuf.
 *       Returns the DOS error code.
 *
 *@@changed V0.9.1 (99-12-12) [umoeller]: added cbBuf to prototype
 */

APIRET doshQueryDiskFSType(ULONG ulLogicalDrive, // in:  1 for A:, 2 for B:, 3 for C:, ...
                           PSZ pszBuf,           // out: buffer for FS type
                           ULONG cbBuf)          // in: size of that buffer
{
    APIRET arc = NO_ERROR;
    CHAR szName[5];

    BYTE   fsqBuffer[sizeof(FSQBUFFER2) + (3 * CCHMAXPATH)] = {0};
    ULONG  cbBuffer   = sizeof(fsqBuffer);        // Buffer length)
    PFSQBUFFER2 pfsqBuffer = (PFSQBUFFER2)fsqBuffer;

    // compose "D:"-type string from logical drive letter
    szName[0] = G_acDriveLetters[ulLogicalDrive];
    szName[1] = ':';
    szName[2] = '\0';

    arc = DosQueryFSAttach(szName,          // logical drive of attached FS ("D:"-style)
                           0,               // ulOrdinal, ignored for FSAIL_QUERYNAME
                           FSAIL_QUERYNAME, // return name for a drive or device
                           pfsqBuffer,      // buffer for returned data
                           &cbBuffer);      // sizeof(*pfsqBuffer)

    if (arc == NO_ERROR)
    {
        if (pszBuf)
        {
            // The data for the last three fields in the FSQBUFFER2
            // structure are stored at the offset of fsqBuffer.szName.
            // Each data field following fsqBuffer.szName begins
            // immediately after the previous item.
            strcpy(pszBuf,
                   (CHAR*)(&pfsqBuffer->szName) + pfsqBuffer->cbName + 1);
        }
    }

    return (arc);
}

/*
 *@@ doshIsFixedDisk:
 *      checks whether a disk is fixed or removeable.
 *      ulLogicalDrive must be 1 for drive A:, 2 for B:, ...
 *      The result is stored in *pfFixed.
 *      Returns DOS error code.
 *
 *      Warning: This uses DosDevIOCtl, which has proved
 *      to cause problems with some device drivers for
 *      removeable disks.
 */

APIRET doshIsFixedDisk(ULONG  ulLogicalDrive,   // in: 1 for A:, 2 for B:, 3 for C:, ...
                       PBOOL  pfFixed)          // out: TRUE for fixed disks
{
    APIRET arc = ERROR_INVALID_DRIVE;

    if (ulLogicalDrive)
    {
        // parameter packet
        #pragma pack(1)
        struct {
            UCHAR command, drive;
        } parms;
        #pragma pack()

        // data packet
        UCHAR ucNonRemoveable;

        ULONG ulParmSize = sizeof(parms);
        ULONG ulDataSize = sizeof(ucNonRemoveable);

        parms.drive = (UCHAR)(ulLogicalDrive-1);
        arc = DosDevIOCtl((HFILE)-1,
                          IOCTL_DISK,
                          DSK_BLOCKREMOVABLE,
                          &parms,
                          ulParmSize,
                          &ulParmSize,
                          &ucNonRemoveable,
                          ulDataSize,
                          &ulDataSize);

        if (arc == NO_ERROR)
            *pfFixed = (BOOL)ucNonRemoveable;
    }

    return (arc);
}

/*
 *@@ doshQueryDiskParams:
 *      this retrieves more information about a given drive,
 *      which is stored in the specified DRIVEPARAMS structure
 *      (dosh.h).
 *
 *      Warning: This uses DosDevIOCtl, which has proved
 *      to cause problems with some device drivers for
 *      removeable disks.
 *
 *      This returns the DOS error code of DosDevIOCtl.
 *
 *@@added V0.9.0 [umoeller]
 */

APIRET doshQueryDiskParams(ULONG ulLogicalDrive,        // in:  1 for A:, 2 for B:, 3 for C:, ...
                           PDRIVEPARAMS pdp)            // out: drive parameters
{
    APIRET arc = ERROR_INVALID_DRIVE;

    if (ulLogicalDrive)
    {
        #pragma pack(1)
        // parameter packet
        struct {
            UCHAR command, drive;
        } parms;
        #pragma pack()

        ULONG ulParmSize = sizeof(parms);
        ULONG ulDataSize = sizeof(DRIVEPARAMS);

        parms.command = 1; // read currently inserted media
        parms.drive=(UCHAR)(ulLogicalDrive-1);

        arc = DosDevIOCtl((HFILE)-1,
                          IOCTL_DISK,
                          DSK_GETDEVICEPARAMS,
                          // parameter packet:
                          &parms, ulParmSize, &ulParmSize,
                          // data packet: DRIVEPARAMS structure
                          pdp,    ulDataSize, &ulDataSize);
    }

    return (arc);
}

/*
 *@@ doshQueryDiskLabel:
 *      this returns the label of a disk into
 *      *pszVolumeLabel, which must be 12 bytes
 *      in size.
 *
 *      This function was added because the Toolkit
 *      information for DosQueryFSInfo is only partly
 *      correct. On OS/2 2.x, that function does not
 *      take an FSINFO structure as input, but a VOLUMELABEL.
 *      On Warp, this does take an FSINFO.
 *
 *      DosSetFSInfo is even worse. See doshSetDiskLabel.
 *
 *      See http://zebra.asta.fh-weingarten.de/os2/Snippets/Bugi6787.HTML
 *      for details.
 *
 *@@added V0.9.0 [umoeller]
 */

APIRET doshQueryDiskLabel(ULONG ulLogicalDrive,         // in:  1 for A:, 2 for B:, 3 for C:, ...
                          PSZ pszVolumeLabel)           // out: volume label (must be 12 chars in size)
{
    APIRET      arc;

    #ifdef __OS2V2X__
        VOLUMELABEL FSInfoBuf;
    #else
        FSINFO      FSInfoBuf;
    #endif

    arc = DosQueryFSInfo(ulLogicalDrive,
                         FSIL_VOLSER,
                         &FSInfoBuf,
                         sizeof(FSInfoBuf)); // depends

    #ifdef __OS2V2X__
        strcpy(pszVolumeLabel, FSInfoBuf.szVolLabel);
    #else
        strcpy(pszVolumeLabel, FSInfoBuf.vol.szVolLabel);
    #endif

    return (arc);
}

/*
 *@@ doshSetDiskLabel:
 *      this sets the label of a disk.
 *
 *      This function was added because the Toolkit
 *      information for DosSetFSInfo is flat out wrong.
 *      That function does not take an FSINFO structure
 *      as input, but a VOLUMELABEL. As a result, using
 *      that function with the Toolkit's calling specs
 *      results in ERROR_LABEL_TOO_LONG always.
 *
 *      See http://zebra.asta.fh-weingarten.de/os2/Snippets/Bugi6787.HTML
 *      for details.
 *
 *@@added V0.9.0 [umoeller]
 */

APIRET doshSetDiskLabel(ULONG ulLogicalDrive,        // in:  1 for A:, 2 for B:, 3 for C:, ...
                        PSZ pszNewLabel)
{
    VOLUMELABEL FSInfoBuf;

    // check length; 11 chars plus null byte allowed
    FSInfoBuf.cch = (BYTE)strlen(pszNewLabel);
    if (FSInfoBuf.cch < sizeof(FSInfoBuf.szVolLabel))
    {
        strcpy(FSInfoBuf.szVolLabel, pszNewLabel);

        return (DosSetFSInfo(ulLogicalDrive,
                             FSIL_VOLSER,
                             &FSInfoBuf,
                             sizeof(FSInfoBuf)));
    }
    else
        return (ERROR_LABEL_TOO_LONG);
}

/*
 *@@category: Helpers\Control program helpers\File management
 */

/* ******************************************************************
 *
 *   File helpers
 *
 ********************************************************************/

/*
 *@@ doshGetExtension:
 *      finds the file name extension of pszFilename,
 *      which can be a file name only or a fully
 *      qualified filename.
 *
 *      This returns a pointer into pszFilename to
 *      the character after the last dot.
 *
 *      Returns NULL if not found (e.g. if the filename
 *      has no dot in it).
 *
 *@@added V0.9.6 (2000-10-16) [umoeller]
 *@@changed V0.9.7 (2000-12-10) [umoeller]: fixed "F:filename.ext" case
 */

PSZ doshGetExtension(const char *pcszFilename)
{
    PSZ pReturn = NULL;

    if (pcszFilename)
    {
        // find filename
        const char *p2 = strrchr(pcszFilename + 2, '\\'),
                            // works on "C:\blah" or "\\unc\blah"
                   *pStartOfName = NULL,
                   *pExtension = NULL;

        if (p2)
            pStartOfName = p2 + 1;
        else
        {
            // no backslash found:
            // maybe only a drive letter was specified:
            if (*(pcszFilename + 1) == ':')
                // yes:
                pStartOfName = pcszFilename + 2;
            else
                // then this is not qualified at all...
                // use start of filename
                pStartOfName = (PSZ)pcszFilename;
        }

        // find last dot in filename
        pExtension = strrchr(pStartOfName, '.');
        if (pExtension)
            pReturn = (PSZ)pExtension + 1;
    }

    return (pReturn);
}

/*
 *@@ doshIsFileOnFAT:
 *      returns TRUE if pszFileName resides on
 *      a FAT drive. Note that pszFileName must
 *      be fully qualified (i.e. the drive letter
 *      must be the first character), or this will
 *      return garbage.
 */

BOOL doshIsFileOnFAT(const char* pcszFileName)
{
    BOOL brc = FALSE;
    CHAR szName[5];

    APIRET rc              = NO_ERROR; // return code
    BYTE   fsqBuffer[sizeof(FSQBUFFER2) + (3 * CCHMAXPATH)] = {0};
    ULONG  cbBuffer   = sizeof(fsqBuffer);        // Buffer length)
    PFSQBUFFER2 pfsqBuffer = (PFSQBUFFER2)fsqBuffer;

    szName[0] = pcszFileName[0];    // copy drive letter
    szName[1] = ':';
    szName[2] = '\0';

    rc = DosQueryFSAttach(szName,          // logical drive of attached FS
                          0,               // ulOrdinal, ignored for FSAIL_QUERYNAME
                          FSAIL_QUERYNAME, // return data for a Drive or Device
                          pfsqBuffer,      // returned data
                          &cbBuffer);      // returned data length

    if (rc == NO_ERROR)
    {
        // The data for the last three fields in the FSQBUFFER2
        // structure are stored at the offset of fsqBuffer.szName.
        // Each data field following fsqBuffer.szName begins
        // immediately after the previous item.
        if (strncmp((PSZ)&(pfsqBuffer->szName) + pfsqBuffer->cbName + 1,
                    "FAT",
                    3)
               == 0)
            brc = TRUE;
    }

    return (brc);
}

/*
 *@@ doshQueryFileSize:
 *      returns the size of an already opened file
 *      or 0 upon errors.
 *      Use doshQueryPathSize to query the size of
 *      any file.
 */

ULONG doshQueryFileSize(HFILE hFile)
{
    FILESTATUS3 fs3;
    if (DosQueryFileInfo(hFile, FIL_STANDARD, &fs3, sizeof(fs3)))
        return (0);
    else
        return (fs3.cbFile);
}

/*
 *@@ doshQueryPathSize:
 *      returns the size of any file,
 *      or 0 if the file could not be
 *      found.
 *      Use doshQueryFileSize instead to query the
 *      size if you have a HFILE.
 */

ULONG doshQueryPathSize(PSZ pszFile)
{
    FILESTATUS3 fs3;
    if (DosQueryPathInfo(pszFile, FIL_STANDARD, &fs3, sizeof(fs3)))
        return (0);
    else
        return (fs3.cbFile);
}

/*
 *@@ doshQueryPathAttr:
 *      returns the file attributes of pszFile,
 *      which can be fully qualified. The
 *      attributes will be stored in *pulAttr.
 *      pszFile can also specify a directory,
 *      although not all attributes make sense
 *      for directories.
 *
 *      fAttr can be:
 *      --  FILE_ARCHIVED
 *      --  FILE_READONLY
 *      --  FILE_SYSTEM
 *      --  FILE_HIDDEN
 *
 *      This returns the APIRET of DosQueryPathAttr.
 *      *pulAttr is only valid if NO_ERROR is
 *      returned.
 *
 *@@added V0.9.0 [umoeller]
 */

APIRET doshQueryPathAttr(const char* pcszFile,      // in: file or directory name
                         PULONG pulAttr)            // out: attributes
{
    FILESTATUS3 fs3;
    APIRET arc = DosQueryPathInfo((PSZ)pcszFile,
                                  FIL_STANDARD,
                                  &fs3,
                                  sizeof(fs3));
    if (arc == NO_ERROR)
    {
        if (pulAttr)
            *pulAttr = fs3.attrFile;
    }

    return (arc);
}

/*
 *@@ doshSetPathAttr:
 *      sets the file attributes of pszFile,
 *      which can be fully qualified.
 *      pszFile can also specify a directory,
 *      although not all attributes make sense
 *      for directories.
 *
 *      fAttr can be:
 *      --  FILE_ARCHIVED
 *      --  FILE_READONLY
 *      --  FILE_SYSTEM
 *      --  FILE_HIDDEN
 *
 *      Note that this simply sets all the given
 *      attributes; the existing attributes
 *      are lost.
 *
 *      This returns the APIRET of DosQueryPathInfo.
 */

APIRET doshSetPathAttr(const char* pcszFile,    // in: file or directory name
                       ULONG ulAttr)            // in: new attributes
{
    FILESTATUS3 fs3;
    APIRET rc = DosQueryPathInfo((PSZ)pcszFile,
                                 FIL_STANDARD,
                                 &fs3,
                                 sizeof(fs3));

    if (rc == NO_ERROR)
    {
        fs3.attrFile = ulAttr;
        rc = DosSetPathInfo((PSZ)pcszFile,
                            FIL_STANDARD,
                            &fs3,
                            sizeof(fs3),
                            DSPI_WRTTHRU);
    }
    return (rc);
}

/*
 *@@ doshLoadTextFile:
 *      reads a text file from disk, allocates memory
 *      via malloc() and sets a pointer to this
 *      buffer (or NULL upon errors).
 *
 *      This returns the APIRET of DosOpen and DosRead.
 *      If any error occured, no buffer was allocated.
 *      Otherwise, you should free() the buffer when
 *      no longer needed.
 *
 *@@changed V0.9.7 (2001-01-15) [umoeller]: renamed from doshReadTextFile
 */

APIRET doshLoadTextFile(const char *pcszFile,  // in: file name to read
                        PSZ* ppszContent)      // out: newly allocated buffer with file's content
{
    ULONG   ulSize,
            ulBytesRead = 0,
            ulAction, ulLocal;
    HFILE   hFile;
    PSZ     pszContent = NULL;

    APIRET arc = DosOpen((PSZ)pcszFile,
                         &hFile,
                         &ulAction,                      // action taken
                         5000L,                          // primary allocation size
                         FILE_ARCHIVED | FILE_NORMAL,    // file attribute
                         OPEN_ACTION_OPEN_IF_EXISTS,     // open flags
                         OPEN_FLAGS_NOINHERIT
                            | OPEN_SHARE_DENYNONE
                            | OPEN_ACCESS_READONLY,      // read-only mode
                         NULL);                          // no EAs

    if (arc == NO_ERROR)
    {
        ulSize = doshQueryFileSize(hFile);
        pszContent = (PSZ)malloc(ulSize+1);
        arc = DosSetFilePtr(hFile,
                            0L,
                            FILE_BEGIN,
                            &ulLocal);
        arc = DosRead(hFile,
                      pszContent,
                      ulSize,
                      &ulBytesRead);
        DosClose(hFile);
        *(pszContent+ulBytesRead) = 0;

        // set output buffer pointer
        *ppszContent = pszContent;
    }
    else
        *ppszContent = 0;

    return (arc);
}

/*
 *@@ doshCreateBackupFileName:
 *      creates a valid backup filename of pszExisting
 *      with a numerical file name extension which does
 *      not exist in the directory where pszExisting
 *      resides.
 *      Returns a PSZ to a new buffer which was allocated
 *      using malloc().
 *
 *      <B>Example:</B> returns "C:\CONFIG.002" for input
 *      "C:\CONFIG.SYS" if "C:\CONFIG.001" already exists.
 *
 *@@changed V0.9.1 (99-12-13) [umoeller]: this crashed if pszExisting had no file extension
 */

PSZ doshCreateBackupFileName(const char* pszExisting)
{
    CHAR    szFilename[CCHMAXPATH];
    PSZ     pszLastDot;
    ULONG   ulCount = 1;
    CHAR    szCount[5];

    strcpy(szFilename, pszExisting);
    pszLastDot = strrchr(szFilename, '.');
    if (!pszLastDot)
        // no dot in filename:
        pszLastDot = szFilename + strlen(szFilename);
    do
    {
        sprintf(szCount, ".%03lu", ulCount);
        strcpy(pszLastDot, szCount);
        ulCount++;
    } while (doshQueryPathSize(szFilename) != 0);

    return (strdup(szFilename));
}

/*
 *@@ doshWriteTextFile:
 *      writes a text file to disk; pszFile must contain the
 *      whole path and filename.
 *
 *      An existing file will be backed up if (pszBackup != NULL),
 *      using doshCreateBackupFileName. In that case, pszBackup
 *      receives the name of the backup created, so that buffer
 *      should be CCHMAXPATH in size.
 *
 *      If (pszbackup == NULL), an existing file will be overwritten.
 *
 *      On success (NO_ERROR returned), *pulWritten receives
 *      the no. of bytes written.
 *
 *@@changed V0.9.3 (2000-05-01) [umoeller]: optimized DosOpen; added error checking; changed prototype
 *@@changed V0.9.3 (2000-05-12) [umoeller]: added pszBackup
 */

APIRET doshWriteTextFile(const char* pszFile,        // in: file name
                         const char* pszContent,     // in: text to write
                         PULONG pulWritten,          // out: bytes written (ptr can be NULL)
                         PSZ pszBackup)              // in/out: create-backup?
{
    APIRET  arc = NO_ERROR;
    ULONG   ulWritten = 0;

    if ((!pszFile) || (!pszContent))
        arc = ERROR_INVALID_PARAMETER;
    else
    {
        ULONG   ulAction = 0,
                ulLocal = 0;
        HFILE   hFile = 0;

        ULONG ulSize = strlen(pszContent);      // exclude 0 byte

        if (pszBackup)
        {
            PSZ     pszBackup2 = doshCreateBackupFileName(pszFile);
            DosCopy((PSZ)pszFile,
                    pszBackup2,
                    DCPY_EXISTING); // delete existing
            strcpy(pszBackup, pszBackup2);
            free(pszBackup2);
        }

        arc = DosOpen((PSZ)pszFile,
                      &hFile,
                      &ulAction,                      // action taken
                      ulSize,                         // primary allocation size
                      FILE_ARCHIVED | FILE_NORMAL,    // file attribute
                      OPEN_ACTION_CREATE_IF_NEW
                         | OPEN_ACTION_REPLACE_IF_EXISTS,  // open flags
                      OPEN_FLAGS_NOINHERIT
                         | OPEN_FLAGS_SEQUENTIAL         // sequential, not random access
                         | OPEN_SHARE_DENYWRITE          // deny write mode
                         | OPEN_ACCESS_WRITEONLY,        // write mode
                      NULL);                          // no EAs

        if (arc == NO_ERROR)
        {
            arc = DosSetFilePtr(hFile,
                                0L,
                                FILE_BEGIN,
                                &ulLocal);
            if (arc == NO_ERROR)
            {
                arc = DosWrite(hFile,
                               (PVOID)pszContent,
                               ulSize,
                               &ulWritten);
                if (arc == NO_ERROR)
                    arc = DosSetFileSize(hFile, ulSize);
            }

            DosClose(hFile);
        }
    } // end if ((pszFile) && (pszContent))

    if (pulWritten)
        *pulWritten = ulWritten;

    return (arc);
}

/*
 *@@ doshOpenLogFile:
 *      this opens a log file in the root directory of
 *      the boot drive; it is titled pszFilename, and
 *      the file handle is returned.
 */

HFILE doshOpenLogFile(const char* pcszFilename)
{
    APIRET  rc;
    CHAR    szFileName[CCHMAXPATH];
    HFILE   hfLog;
    ULONG   ulAction;
    ULONG   ibActual;

    sprintf(szFileName, "%c:\\%s", doshQueryBootDrive(), pcszFilename);
    rc = DosOpen(szFileName,
                 &hfLog,
                 &ulAction,
                 0,             // file size
                 FILE_NORMAL,
                 OPEN_ACTION_CREATE_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
                 OPEN_ACCESS_WRITEONLY | OPEN_SHARE_DENYWRITE,
                 (PEAOP2)NULL);
    if (rc == NO_ERROR)
    {
        DosSetFilePtr(hfLog, 0, FILE_END, &ibActual);
        return (hfLog);
    }
    else
        return (0);
}

/*
 * doshWriteToLogFile
 *      writes a string to a log file, adding a
 *      leading timestamp.
 */

APIRET doshWriteToLogFile(HFILE hfLog, const char* pcsz)
{
    if (hfLog)
    {
        DATETIME dt;
        CHAR szTemp[2000];
        ULONG   cbWritten;
        DosGetDateTime(&dt);
        sprintf(szTemp, "Time: %02d:%02d:%02d %s",
            dt.hours, dt.minutes, dt.seconds,
            pcsz);
        return (DosWrite(hfLog, (PVOID)szTemp, strlen(szTemp), &cbWritten));
    }
    else return (ERROR_INVALID_HANDLE);
}

/*
 *@@category: Helpers\Control program helpers\Directory management
 *      directory helpers (querying, creating, deleting etc.).
 */

/* ******************************************************************
 *
 *   Directory helpers
 *
 ********************************************************************/

/*
 *@@ doshQueryDirExist:
 *      returns TRUE if the given directory
 *      exists.
 */

BOOL doshQueryDirExist(const char *pcszDir)
{
    FILESTATUS3 fs3;
    APIRET arc = DosQueryPathInfo((PSZ)pcszDir,
                                  FIL_STANDARD,
                                  &fs3,
                                  sizeof(fs3));
    if (arc == NO_ERROR)
        // file found:
        return ((fs3.attrFile & FILE_DIRECTORY) != 0);
    else
        return FALSE;
}

/*
 *@@ doshCreatePath:
 *      this creates the specified directory.
 *      As opposed to DosCreateDir, this
 *      function can create several directories
 *      at the same time, if the parent
 *      directories do not exist yet.
 */

APIRET doshCreatePath(PSZ pszPath,
                      BOOL fHidden) // in: if TRUE, the new directories will get FILE_HIDDEN
{
    APIRET  arc0 = NO_ERROR;
    CHAR    path[CCHMAXPATH];
    CHAR    *cp, c;
    ULONG   cbPath;

    strcpy(path, pszPath);
    cbPath = strlen(path);

    if (path[cbPath] != '\\')
    {
        path[cbPath] = '\\';
        path[cbPath+1] = 0;
    }

    cp = path;
    // advance past the drive letter only if we have one
    if (*(cp+1) == ':')
        cp += 3;

    // go!
    while (*cp != 0)
    {
        if (*cp == '\\')
        {
            c = *cp;
            *cp = 0;
            if (!doshQueryDirExist(path))
            {
                APIRET arc = DosCreateDir(path,
                                          0);   // no EAs
                if (arc != NO_ERROR)
                {
                    arc0 = arc;
                    break;
                }
                else
                    if (fHidden)
                    {
                        // hide the directory we just created
                        doshSetPathAttr(path, FILE_HIDDEN);
                    }
            }
            *cp = c;
        }
        cp++;
    }
    return (arc0);
}

/*
 *@@ doshQueryCurrentDir:
 *      writes the current directory into
 *      the specified buffer, which should be
 *      CCHMAXPATH in size.
 *
 *      As opposed to DosQueryCurrentDir, this
 *      includes the drive letter.
 *
 *@@added V0.9.4 (2000-07-22) [umoeller]
 */

APIRET doshQueryCurrentDir(PSZ pszBuf)
{
    APIRET arc = NO_ERROR;
    ULONG   ulCurDisk = 0;
    ULONG   ulMap = 0;
    arc = DosQueryCurrentDisk(&ulCurDisk, &ulMap);
    if (arc == NO_ERROR)
    {
        ULONG   cbBuf = CCHMAXPATH - 3;
        *pszBuf = G_acDriveLetters[ulCurDisk];
        *(pszBuf + 1) = ':';
        *(pszBuf + 2) = '\\';
        arc = DosQueryCurrentDir(0, pszBuf + 3, &cbBuf);
    }

    return (arc);
}

/*
 *@@ doshDeleteDir:
 *      deletes a directory. As opposed to DosDeleteDir,
 *      this removes empty subdirectories and/or files
 *      as well.
 *
 *      flFlags can be any combination of the following:
 *
 *      -- DOSHDELDIR_RECURSE: recurse into subdirectories.
 *
 *      -- DOSHDELDIR_DELETEFILES: delete all regular files
 *              which are found on the way.
 *
 *              THIS IS NOT IMPLEMENTED YET.
 *
 *      If 0 is specified, this effectively behaves just as
 *      DosDeleteDir.
 *
 *      If you specify DOSHDELDIR_RECURSE only, only empty
 *      subdirectories are deleted as well.
 *
 *      If you specify DOSHDELDIR_RECURSE | DOSHDELDIR_DELETEFILES,
 *      this removes an entire directory tree, including all
 *      subdirectories and files.
 *
 *@@added V0.9.4 (2000-07-01) [umoeller]
 */

APIRET doshDeleteDir(const char *pcszDir,
                     ULONG flFlags,
                     PULONG pulDirs,        // out: directories found
                     PULONG pulFiles)       // out: files found
{
    APIRET          arc = NO_ERROR,
                    arcReturn = NO_ERROR;

    HDIR            hdirFindHandle = HDIR_CREATE;
    FILEFINDBUF3    ffb3     = {0};      // returned from FindFirst/Next
    ULONG           ulResultBufLen = sizeof(FILEFINDBUF3);
    ULONG           ulFindCount    = 1;        // look for 1 file at a time

    CHAR            szFileMask[2*CCHMAXPATH];
    sprintf(szFileMask, "%s\\*", pcszDir);

    // find files
    arc = DosFindFirst(szFileMask,
                       &hdirFindHandle,     // directory search handle
                       FILE_ARCHIVED | FILE_DIRECTORY | FILE_SYSTEM
                         | FILE_HIDDEN | FILE_READONLY,
                                     // search attributes; include all, even dirs
                       &ffb3,               // result buffer
                       ulResultBufLen,      // result buffer length
                       &ulFindCount,        // number of entries to find
                       FIL_STANDARD);       // return level 1 file info

    if (arc == NO_ERROR)
    {
        // keep finding the next file until there are no more files
        while (arc == NO_ERROR)     // != ERROR_NO_MORE_FILES
        {
            if (ffb3.attrFile & FILE_DIRECTORY)
            {
                // we found a directory:

                // ignore the pseudo-directories
                if (    (strcmp(ffb3.achName, ".") != 0)
                     && (strcmp(ffb3.achName, "..") != 0)
                   )
                {
                    // real directory:
                    if (flFlags & DOSHDELDIR_RECURSE)
                    {
                        // recurse!
                        CHAR szSubDir[2*CCHMAXPATH];
                        sprintf(szSubDir, "%s\\%s", pcszDir, ffb3.achName);
                        arcReturn = doshDeleteDir(szSubDir,
                                                  flFlags,
                                                  pulDirs,
                                                  pulFiles);
                        // this removes ffb3.achName as well
                    }
                    else
                    {
                        // directory, but no recursion:
                        // report "access denied"
                        // (this is what OS/2 reports with DosDeleteDir as well)
                        arcReturn = ERROR_ACCESS_DENIED;  // 5
                        (*pulDirs)++;
                    }
                }
            }
            else
            {
                // it's a file:
                arcReturn = ERROR_ACCESS_DENIED;
                (*pulFiles)++;
            }

            if (arc == NO_ERROR)
            {
                ulFindCount = 1;                      // reset find count
                arc = DosFindNext(hdirFindHandle,      // directory handle
                                  &ffb3,         // result buffer
                                  ulResultBufLen,      // result buffer length
                                  &ulFindCount);       // number of entries to find
            }
        } // endwhile

        DosFindClose(hdirFindHandle);    // close our find handle
    }

    if (arcReturn == NO_ERROR)
        // success so far:
        // delete our directory now
        arc = DosDeleteDir((PSZ)pcszDir);

    return (arcReturn);
}

/*
 *@@category: Helpers\Control program helpers\Module handling
 *      helpers for importing functions from a module (DLL).
 */

/* ******************************************************************
 *
 *   Module handling helpers
 *
 ********************************************************************/

/*
 *@@ doshResolveImports:
 *      this function loads the module called pszModuleName
 *      and resolves imports dynamically using DosQueryProcAddress.
 *
 *      To specify the functions to be imported, a RESOLVEFUNCTION
 *      array is used. In each of the array items, specify the
 *      name of the function and a pointer to a function pointer
 *      where to store the resolved address.
 *
 *@@added V0.9.3 (2000-04-29) [umoeller]
 */

APIRET doshResolveImports(PSZ pszModuleName,    // in: DLL to load
                          HMODULE *phmod,       // out: module handle
                          PRESOLVEFUNCTION paResolves, // in/out: function resolves
                          ULONG cResolves)      // in: array item count (not array size!)
{
    CHAR    szName[CCHMAXPATH];
    APIRET arc = DosLoadModule(szName,
                               sizeof(szName),
                               pszModuleName,
                               phmod);
    if (arc == NO_ERROR)
    {
        ULONG  ul;
        for (ul = 0;
             ul < cResolves;
             ul++)
        {
            arc = DosQueryProcAddr(*phmod,
                                   0,               // ordinal, ignored
                                   (PSZ)paResolves[ul].pcszFunctionName,
                                   paResolves[ul].ppFuncAddress);

            /* _Pmpf(("Resolved %s to 0x%lX, rc: %d",
                    paResolves[ul].pcszFunctionName,
                    *paResolves[ul].ppFuncAddress,
                    arc)); */
            if (arc != NO_ERROR)
                break;
        }
    }

    return (arc);
}

/*
 *@@category: Helpers\Control program helpers\Performance (CPU load) helpers
 *      helpers around DosPerfSysCall.
 */

/* ******************************************************************
 *
 *   Performance Counters (CPU Load)
 *
 ********************************************************************/

/*
 *@@ doshPerfOpen:
 *      initializes the OS/2 DosPerfSysCall API for
 *      the calling thread.
 *
 *      Note: This API is not supported on all OS/2
 *      versions. I believe it came up with some Warp 4
 *      fixpak. The API is resolved dynamically by
 *      this function (using DosQueryProcAddr). Only
 *      if NO_ERROR is returned, you may call doshPerfGet
 *      afterwards.
 *
 *      This properly initializes the internal counters
 *      which the OS/2 kernel uses for this API. Apparently,
 *      with newer kernels (FP13/14), IBM has chosen to no
 *      longer do this automatically, which is the reason
 *      why many "pulse" utilities display garbage with these
 *      fixpaks.
 *
 *      After NO_ERROR is returned, DOSHPERFSYS.cProcessors
 *      contains the no. of processors found on the system.
 *      All pointers in DOSHPERFSYS then point to arrays
 *      which have exactly cProcessors array items.
 *
 *      Call doshPerfClose to clean up resources allocated
 *      by this function.
 *
 *      Example code:
 *
 +      PDOSHPERFSYS pPerf = NULL;
 +      APIRET arc = doshPerfOpen(&pPerf);
 +      if (arc == NO_ERROR)
 +      {
 +          // this should really be in a timer
 +          ULONG   ulCPU;
 +          arc = doshPerfGet(&pPerf);
 +          // go thru all CPUs
 +          for (ulCPU = 0; ulCPU < pPerf->cProcessors; ulCPU++)
 +          {
 +              LONG lLoadThis = pPerf->palLoads[ulCPU];
 +              ...
 +          }
 +
 +          ...
 +
 +          // clean up
 +          doshPerfClose(&pPerf);
 +      }
 +
 *
 *@@added V0.9.7 (2000-12-02) [umoeller]
 */

APIRET doshPerfOpen(PDOSHPERFSYS *ppPerfSys)  // out: new DOSHPERFSYS structure
{
    APIRET  arc = NO_ERROR;

    // allocate DOSHPERFSYS structure
    *ppPerfSys = (PDOSHPERFSYS)malloc(sizeof(DOSHPERFSYS));
    if (!*ppPerfSys)
        arc = ERROR_NOT_ENOUGH_MEMORY;
    else
    {
        // initialize structure
        PDOSHPERFSYS pPerfSys = *ppPerfSys;
        memset(pPerfSys, 0, sizeof(*pPerfSys));

        // resolve DosPerfSysCall API entry
        arc = DosLoadModule(NULL, 0, "DOSCALLS", &pPerfSys->hmod);
        if (arc == NO_ERROR)
        {
            arc = DosQueryProcAddr(pPerfSys->hmod,
                                   976,
                                   "DosPerfSysCall",
                                   (PFN*)(&pPerfSys->pDosPerfSysCall));
            if (arc == NO_ERROR)
            {
                // OK, we got the API: initialize!
                arc = pPerfSys->pDosPerfSysCall(CMD_KI_ENABLE, 0, 0, 0);
                if (arc == NO_ERROR)
                {
                    pPerfSys->fInitialized = TRUE;
                            // call CMD_KI_DISABLE later

                    arc = pPerfSys->pDosPerfSysCall(CMD_PERF_INFO,
                                                    0,
                                                    (ULONG)(&pPerfSys->cProcessors),
                                                    0);
                    if (arc == NO_ERROR)
                    {
                        ULONG   ul = 0;

                        // allocate arrays
                        pPerfSys->paCPUUtils = (PCPUUTIL)calloc(pPerfSys->cProcessors,
                                                                sizeof(CPUUTIL));
                        if (!pPerfSys->paCPUUtils)
                            arc = ERROR_NOT_ENOUGH_MEMORY;
                        else
                        {
                            pPerfSys->padBusyPrev = (double*)malloc(pPerfSys->cProcessors * sizeof(double));
                            if (!pPerfSys->padBusyPrev)
                                arc = ERROR_NOT_ENOUGH_MEMORY;
                            else
                            {
                                pPerfSys->padTimePrev
                                    = (double*)malloc(pPerfSys->cProcessors * sizeof(double));
                                if (!pPerfSys->padTimePrev)
                                    arc = ERROR_NOT_ENOUGH_MEMORY;
                                else
                                {
                                    pPerfSys->palLoads = (PLONG)malloc(pPerfSys->cProcessors * sizeof(LONG));
                                    if (!pPerfSys->palLoads)
                                        arc = ERROR_NOT_ENOUGH_MEMORY;
                                    else
                                    {
                                        for (ul = 0; ul < pPerfSys->cProcessors; ul++)
                                        {
                                            pPerfSys->padBusyPrev[ul] = 0.0;
                                            pPerfSys->padTimePrev[ul] = 0.0;
                                            pPerfSys->palLoads[ul] = 0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } // end if (arc == NO_ERROR)
        } // end if (arc == NO_ERROR)

        if (arc != NO_ERROR)
        {
            doshPerfClose(ppPerfSys);
        }
    } // end else if (!*ppPerfSys)

    return (arc);
}

/*
 *@@ doshPerfGet:
 *      calculates a current snapshot of the system load,
 *      compared with the load which was calculated on
 *      the previous call.
 *
 *      If you want to continually measure the system CPU
 *      load, this is the function you will want to call
 *      regularly -- e.g. with a timer once per second.
 *
 *      Call this ONLY if doshPerfOpen returned NO_ERROR,
 *      or you'll get crashes.
 *
 *      If this call returns NO_ERROR, you get a LONG
 *      CPU load for each CPU in the system in the
 *      DOSHPERFSYS.palLoads array (in per-mille, 0-1000).
 *
 *      For example, if there are two CPUs, after this call,
 *
 *      -- DOSHPERFSYS.palLoads[0] contains the load of
 *         the first CPU,
 *
 *      -- DOSHPERFSYS.palLoads[1] contains the load of
 *         the second CPU.
 *
 *      See doshPerfOpen for example code.
 *
 *@@added V0.9.7 (2000-12-02) [umoeller]
 */

APIRET doshPerfGet(PDOSHPERFSYS pPerfSys)
{
    APIRET arc = NO_ERROR;
    if (!pPerfSys->pDosPerfSysCall)
        arc = ERROR_INVALID_PARAMETER;
    else
    {
        arc = pPerfSys->pDosPerfSysCall(CMD_KI_RDCNT,
                                        (ULONG)pPerfSys->paCPUUtils,
                                        0, 0);
        if (arc == NO_ERROR)
        {
            // go thru all processors
            ULONG ul = 0;
            for (; ul < pPerfSys->cProcessors; ul++)
            {
                PCPUUTIL    pCPUUtilThis = &pPerfSys->paCPUUtils[ul];

                double      dTime = LL2F(pCPUUtilThis->ulTimeHigh,
                                         pCPUUtilThis->ulTimeLow);
                double      dBusy = LL2F(pCPUUtilThis->ulBusyHigh,
                                         pCPUUtilThis->ulBusyLow);

                double      *pdBusyPrevThis = &pPerfSys->padBusyPrev[ul];
                double      *pdTimePrevThis = &pPerfSys->padTimePrev[ul];

                // avoid division by zero
                double      dTimeDelta = (dTime - *pdTimePrevThis);
                if (dTimeDelta)
                    pPerfSys->palLoads[ul]
                        = (LONG)( (double)(   (dBusy - *pdBusyPrevThis)
                                            / dTimeDelta
                                            * 1000.0
                                          )
                                );
                else
                    pPerfSys->palLoads[ul] = 0;

                *pdTimePrevThis = dTime;
                *pdBusyPrevThis = dBusy;
            }
        }
    }

    return (arc);
}

/*
 *@@ doshPerfClose:
 *      frees all resources allocated by doshPerfOpen.
 *
 *@@added V0.9.7 (2000-12-02) [umoeller]
 *@@changed V0.9.9 (2001-02-06) [umoeller]: removed disable; this broke the WarpCenter
 */

APIRET doshPerfClose(PDOSHPERFSYS *ppPerfSys)
{
    APIRET arc = NO_ERROR;
    PDOSHPERFSYS pPerfSys = *ppPerfSys;
    if (!pPerfSys)
        arc = ERROR_INVALID_PARAMETER;
    else
    {
        /* if (pPerfSys->fInitialized)
            pPerfSys->pDosPerfSysCall(CMD_KI_DISABLE,
                                      0, 0, 0); */

        if (pPerfSys->paCPUUtils)
            free(pPerfSys->paCPUUtils);
        if (pPerfSys->padBusyPrev)
            free(pPerfSys->padBusyPrev);
        if (pPerfSys->padTimePrev)
            free(pPerfSys->padTimePrev);

        if (pPerfSys->hmod)
            DosFreeModule(pPerfSys->hmod);
        free(pPerfSys);
        *ppPerfSys = NULL;
    }

    return (arc);
}

/*
 *@@category: Helpers\Control program helpers\Process management
 *      helpers for starting subprocesses.
 */

/* ******************************************************************
 *
 *   Process helpers
 *
 ********************************************************************/

/*
 *@@ doshFindExecutable:
 *      this searches the PATH for the specified pcszCommand
 *      by calling DosSearchPath.
 *
 *      papcszExtensions determines if additional searches are to be
 *      performed if DosSearchPath returns ERROR_FILE_NOT_FOUND.
 *      This must point to an array of strings specifying the extra
 *      extensions to search for.
 *
 *      If both papcszExtensions and cExtensions are null, no
 *      extra searches are performed.
 *
 *      If this returns NO_ERROR, pszExecutable receives
 *      the full path of the executable found by DosSearchPath.
 *      Otherwise ERROR_FILE_NOT_FOUND is returned.
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
 */

APIRET doshFindExecutable(const char *pcszCommand,      // in: command (e.g. "lvm")
                          PSZ pszExecutable,            // out: full path (e.g. "F:\os2\lvm.exe")
                          ULONG cbExecutable,           // in: sizeof (*pszExecutable)
                          const char **papcszExtensions, // in: array of extensions (without dots)
                          ULONG cExtensions)            // in: array item count
{
    APIRET arc = DosSearchPath(SEARCH_IGNORENETERRS | SEARCH_ENVIRONMENT | SEARCH_CUR_DIRECTORY,
                               "PATH",
                               (PSZ)pcszCommand,
                               pszExecutable,
                               cbExecutable);
    if (    (arc == ERROR_FILE_NOT_FOUND)           // not found?
         && (cExtensions)                    // any extra searches wanted?
       )
    {
        // try additional things then
        PSZ psz2 = malloc(strlen(pcszCommand) + 20);
        if (psz2)
        {
            ULONG   ul;
            for (ul = 0;
                 ul < cExtensions;
                 ul++)
            {
                const char *pcszExtThis = papcszExtensions[ul];
                sprintf(psz2, "%s.%s", pcszCommand, pcszExtThis);
                arc = DosSearchPath(SEARCH_IGNORENETERRS | SEARCH_ENVIRONMENT | SEARCH_CUR_DIRECTORY,
                                    "PATH",
                                    psz2,
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
 *@@ doshExecVIO:
 *      executes cmd.exe with the /c parameter
 *      and pcszExecWithArgs. This is equivalent
 *      to the C library system() function,
 *      except that an OS/2 error code is returned
 *      and that this works with the VAC subsystem
 *      library.
 *
 *      This uses DosExecPgm and handles the sick
 *      argument passing.
 *
 *      If NO_ERROR is returned, *plExitCode receives
 *      the exit code of the process. If the process
 *      was terminated abnormally, *plExitCode receives:
 *
 *      -- -1: hard error halt
 *      -- -2: 16-bit trap
 *      -- -3: DosKillProcess
 *      -- -4: 32-bit exception
 *
 *@@added V0.9.4 (2000-07-27) [umoeller]
 */

APIRET doshExecVIO(const char *pcszExecWithArgs,
                   PLONG plExitCode)            // out: exit code (ptr can be NULL)
{
    APIRET arc = NO_ERROR;

    if (strlen(pcszExecWithArgs) > 255)
        arc = ERROR_INSUFFICIENT_BUFFER;
    {
        CHAR szObj[CCHMAXPATH];
        RESULTCODES res = {0};
        CHAR szBuffer[300];

        // DosExecPgm expects two args in szBuffer:
        // --  cmd.exe\0
        // --  then the actual argument, terminated by two 0 bytes.
        memset(szBuffer, 0, sizeof(szBuffer));
        strcpy(szBuffer, "cmd.exe\0");
        sprintf(szBuffer + 8, "/c %s",
                pcszExecWithArgs);

        arc = DosExecPgm(szObj, sizeof(szObj),
                         EXEC_SYNC,
                         szBuffer,
                         0,
                         &res,
                         "cmd.exe");
        if ((arc == NO_ERROR) && (plExitCode))
        {
            if (res.codeTerminate == 0)
                // normal exit:
                *plExitCode = res.codeResult;
            else
                *plExitCode = -((LONG)res.codeTerminate);
        }
    }

    return (arc);
}

/*
 *@@ doshQuickStartSession:
 *      this is a shortcut to DosStartSession w/out having to
 *      deal with all the messy parameters.
 *
 *      This one starts pszPath as a child session and passes
 *      pszParams to it.
 *
 *      In usPgmCtl, OR any or none of the following (V0.9.0):
 *      --  SSF_CONTROL_NOAUTOCLOSE (0x0008): do not close automatically.
 *              This bit is used only for VIO Windowable apps and ignored
 *              for all other applications.
 *      --  SSF_CONTROL_MINIMIZE (0x0004)
 *      --  SSF_CONTROL_MAXIMIZE (0x0002)
 *      --  SSF_CONTROL_INVISIBLE (0x0001)
 *      --  SSF_CONTROL_VISIBLE (0x0000)
 *
 *      Specifying 0 will therefore auto-close the session and make it
 *      visible.
 *
 *      If (fWait), this function will create a termination queue
 *      and not return until the child session has ended. Otherwise
 *      the function will return immediately, and the SID/PID of
 *      the child session can be found in *pulSID and *ppid.
 *
 *      Returns the error code of DosStartSession.
 *
 *      Note: According to CPREF, calling DosStartSession calls
 *      DosExecPgm in turn for all full-screen, VIO, and PM sessions.
 *
 *@@changed V0.9.0 [umoeller]: prototype changed to include usPgmCtl
 *@@changed V0.9.1 (99-12-30) [umoeller]: queue was sometimes not closed. Fixed.
 *@@changed V0.9.3 (2000-05-03) [umoeller]: added fForeground
 */

APIRET doshQuickStartSession(const char *pcszPath,       // in: program to start
                             const char *pcszParams,     // in: parameters for program
                             BOOL fForeground,  // in: if TRUE, session will be in foreground
                             USHORT usPgmCtl,   // in: STARTDATA.PgmControl
                             BOOL fWait,        // in: wait for termination?
                             PULONG pulSID,     // out: session ID (req.)
                             PPID ppid)         // out: process ID (req.)
{
    APIRET      arc;
    // queue stuff
    const char  *pcszQueueName = "\\queues\\kfgstart.que";
    HQUEUE      hq = 0;
    PID         qpid = 0;
    STARTDATA   SData;
    CHAR        szObjBuf[CCHMAXPATH];

    if (fWait)
    {
        if ((arc = DosCreateQueue(&hq,
                                  QUE_FIFO | QUE_CONVERT_ADDRESS,
                                  (PSZ)pcszQueueName))
                    != NO_ERROR)
            return (arc);

        if ((arc = DosOpenQueue(&qpid, &hq, (PSZ)pcszQueueName)) != NO_ERROR)
            return (arc);
    }

    SData.Length  = sizeof(STARTDATA);
    SData.Related = SSF_RELATED_CHILD; //INDEPENDENT;
    SData.FgBg    = (fForeground) ? SSF_FGBG_FORE : SSF_FGBG_BACK;
            // V0.9.3 (2000-05-03) [umoeller]
    SData.TraceOpt = SSF_TRACEOPT_NONE;

    SData.PgmTitle = (PSZ)pcszPath;       // title for window
    SData.PgmName = (PSZ)pcszPath;
    SData.PgmInputs = (PSZ)pcszParams;

    SData.TermQ = (fWait) ? "\\queues\\kfgstart.que" : NULL;
    SData.Environment = 0;
    SData.InheritOpt = SSF_INHERTOPT_PARENT;
    SData.SessionType = SSF_TYPE_DEFAULT;
    SData.IconFile = 0;
    SData.PgmHandle = 0;

    SData.PgmControl = usPgmCtl;

    SData.InitXPos  = 30;
    SData.InitYPos  = 40;
    SData.InitXSize = 200;
    SData.InitYSize = 140;
    SData.Reserved = 0;
    SData.ObjectBuffer  = (CHAR*)&szObjBuf;
    SData.ObjectBuffLen = (ULONG)sizeof(szObjBuf);

    arc = DosStartSession(&SData, pulSID, ppid);

    if (arc == NO_ERROR)
    {
        if (fWait)
        {
            REQUESTDATA rqdata;
            ULONG       DataLength = 0;
            PULONG      DataAddress;
            BYTE        elpri;

            rqdata.pid = qpid;
            DosReadQueue(hq,                // in: queue handle
                         &rqdata,           // out: pid and ulData
                         &DataLength,       // out: size of data returned
                         (PVOID*)&DataAddress, // out: data returned
                         0,                 // in: remove first element in queue
                         0,                 // in: wait for queue data (block thread)
                         &elpri,            // out: element's priority
                         0);                // in: event semaphore to be posted
        }
    }

    if (hq)
        DosCloseQueue(hq);

    return (arc);
}


