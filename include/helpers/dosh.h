/* $Id$ */


/*
 *@@sourcefile dosh.h:
 *      header file for dosh.c. See remarks there.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@include #define INCL_DOSPROCESS
 *@@include #define INCL_DOSDEVIOCTL    // for doshQueryDiskParams only
 *@@include #include <os2.h>
 *@@include #include "dosh.h"
 */

/*      This file Copyright (C) 1997-2000 Ulrich MФller,
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

#if __cplusplus
extern "C" {
#endif

#ifndef DOSH_HEADER_INCLUDED
    #define DOSH_HEADER_INCLUDED

    /* ******************************************************************
     *
     *   Miscellaneous
     *
     ********************************************************************/

    CHAR doshGetChar(VOID);

    BOOL doshQueryShiftState(VOID);

    BOOL doshIsWarp4(VOID);

    PSZ doshQuerySysErrorMsg(APIRET arc);

    /* ******************************************************************
     *
     *   Memory helpers
     *
     ********************************************************************/

    PVOID doshAllocSharedMem(ULONG ulSize,
                             const char* pcszName);

    PVOID doshRequestSharedMem(const char *pcszName);

    /* ******************************************************************
     *
     *   Drive helpers
     *
     ********************************************************************/

    VOID doshEnumDrives(PSZ pszBuffer,
                        const char *pcszFileSystem,
                        BOOL fSkipRemoveables);

    CHAR doshQueryBootDrive(VOID);

    APIRET doshAssertDrive(ULONG ulLogicalDrive);

    APIRET doshSetLogicalMap(ULONG ulLogicalDrive);

    APIRET doshQueryDiskFree(ULONG ulLogicalDrive,
                             double *pdFree);

    APIRET doshQueryDiskFSType(ULONG ulLogicalDrive,
                               PSZ pszBuf,
                               ULONG cbBuf);

    APIRET doshIsFixedDisk(ULONG  ulLogicalDrive,
                           PBOOL  pfFixed);

    #ifdef INCL_DOSDEVIOCTL

        // flags for DRIVEPARMS.usDeviceAttrs (see DSK_GETDEVICEPARAMS in CPREF):
        #define DEVATTR_REMOVEABLE  0x0001      // drive is removeable
        #define DEVATTR_CHANGELINE  0x0002      // media has been removed since last I/O operation
        #define DEVATTR_GREATER16MB 0x0004      // physical device driver supports physical addresses > 16 MB

        #pragma pack(1)

        /*
         *@@ DRIVEPARAMS:
         *      structure used for doshQueryDiskParams.
         */

        typedef struct _DRIVEPARAMS
        {
            BIOSPARAMETERBLOCK bpb;
                        // BIOS parameter block. This is the first sector
                        // (at byte 0) in each partition. This is defined
                        // in the OS2 headers as follows:

                        /*
                        typedef struct _BIOSPARAMETERBLOCK {
                          USHORT     usBytesPerSector;
                                        //  Number of bytes per sector.
                          BYTE       bSectorsPerCluster;
                                        //  Number of sectors per cluster.
                          USHORT     usReservedSectors;
                                        //  Number of reserved sectors.
                          BYTE       cFATs;
                                        //  Number of FATs.
                          USHORT     cRootEntries;
                                        //  Number of root directory entries.
                          USHORT     cSectors;
                                        //  Number of sectors.
                          BYTE       bMedia;
                                        //  Media descriptor.
                          USHORT     usSectorsPerFAT;
                                        //  Number of secctors per FAT.
                          USHORT     usSectorsPerTrack;
                                        //  Number of sectors per track.
                          USHORT     cHeads;
                                        //  Number of heads.
                          ULONG      cHiddenSectors;
                                        //  Number of hidden sectors.
                          ULONG      cLargeSectors;
                                        //  Number of large sectors.
                          BYTE       abReserved[6];
                                        //  Reserved.
                          USHORT     cCylinders;
                                        //  Number of cylinders defined for the physical
                                        // device.
                          BYTE       bDeviceType;
                                        //  Physical layout of the specified device.
                          USHORT     fsDeviceAttr;
                                        //  A bit field that returns flag information
                                        //  about the specified drive.
                        } BIOSPARAMETERBLOCK; */

            USHORT  usCylinders;
                        // no. of cylinders
            UCHAR   ucDeviceType;
                        // device type; according to the IBM Control
                        // Program Reference (see DSK_GETDEVICEPARAMS),
                        // this value can be:
                        // --  0:  48 TPI low-density diskette drive
                        // --  1:  96 TPI high-density diskette drive
                        // --  2:  3.5-inch 720KB diskette drive
                        // --  3:  8-Inch single-density diskette drive
                        // --  4:  8-Inch double-density diskette drive
                        // --  5:  Fixed disk
                        // --  6:  Tape drive
                        // --  7:  Other (includes 1.44MB 3.5-inch diskette drive)
                        // --  8:  R/W optical disk
                        // --  9:  3.5-inch 4.0MB diskette drive (2.88MB formatted)
            USHORT  usDeviceAttrs;
                        // DEVATTR_* flags
        } DRIVEPARAMS, *PDRIVEPARAMS;
        #pragma pack()

        APIRET doshQueryDiskParams(ULONG ulLogicalDrive,
                                   PDRIVEPARAMS pdp);
    #endif

    APIRET doshQueryDiskLabel(ULONG ulLogicalDrive,
                              PSZ pszVolumeLabel);

    APIRET doshSetDiskLabel(ULONG ulLogicalDrive,
                            PSZ pszNewLabel);

    /* ******************************************************************
     *
     *   Module handling helpers
     *
     ********************************************************************/

    /*
     *@@ RESOLVEFUNCTION:
     *      one of these structures each define
     *      a single function import to doshResolveImports.
     *
     *@@added V0.9.3 (2000-04-25) [umoeller]
     */

    typedef struct _RESOLVEFUNCTION
    {
        const char  *pcszFunctionName;
        PFN         *ppFuncAddress;
    } RESOLVEFUNCTION, *PRESOLVEFUNCTION;

    APIRET doshResolveImports(PSZ pszModuleName,
                              HMODULE *phmod,
                              PRESOLVEFUNCTION paResolves,
                              ULONG cResolves);

    /* ******************************************************************
     *
     *   Performance Counters (CPU Load)
     *
     ********************************************************************/

    #define CMD_PERF_INFO           0x41
    #define CMD_KI_ENABLE           0x60
    #define CMD_KI_DISABLE          0x61
    #ifndef CMD_KI_RDCNT
        #define CMD_KI_RDCNT            0x63
        typedef APIRET APIENTRY FNDOSPERFSYSCALL(ULONG ulCommand,
                                                 ULONG ulParm1,
                                                 ULONG ulParm2,
                                                 ULONG ulParm3);
        typedef FNDOSPERFSYSCALL *PFNDOSPERFSYSCALL;
    #endif

    typedef struct _CPUUTIL
    {
        ULONG ulTimeLow;     // low 32 bits of time stamp
        ULONG ulTimeHigh;    // high 32 bits of time stamp
        ULONG ulIdleLow;     // low 32 bits of idle time
        ULONG ulIdleHigh;    // high 32 bits of idle time
        ULONG ulBusyLow;     // low 32 bits of busy time
        ULONG ulBusyHigh;    // high 32 bits of busy time
        ULONG ulIntrLow;     // low 32 bits of interrupt time
        ULONG ulIntrHigh;    // high 32 bits of interrupt time
    } CPUUTIL, *PCPUUTIL;

    // macro to convert 8-byte (low, high) time value to double
    #define LL2F(high, low) (4294967296.0*(high)+(low))

    /*
     *@@ DOSHPERFSYS:
     *      structure used with doshPerfOpen.
     *
     *@@added V0.9.7 (2000-12-02) [umoeller]
     */

    typedef struct _DOSHPERFSYS
    {
        // output: no. of processors on the system
        ULONG       cProcessors;
        // output: one CPU load for each CPU
        PLONG       palLoads;

        // each of the following ptrs points to an array of cProcessors items
        PCPUUTIL    paCPUUtils;     // CPUUTIL structures
        double      *padBusyPrev;   // previous "busy" calculations
        double      *padTimePrev;   // previous "time" calculations

        // private stuff
        HMODULE     hmod;
        BOOL        fInitialized;
        PFNDOSPERFSYSCALL pDosPerfSysCall;
    } DOSHPERFSYS, *PDOSHPERFSYS;

    APIRET doshPerfOpen(PDOSHPERFSYS *ppPerfSys);

    APIRET doshPerfGet(PDOSHPERFSYS pPerfSys);

    APIRET doshPerfClose(PDOSHPERFSYS *ppPerfSys);

    /* ******************************************************************
     *
     *   File helpers
     *
     ********************************************************************/

    PSZ doshGetExtension(const char *pcszFilename);

    BOOL doshIsFileOnFAT(const char* pcszFileName);

    APIRET doshIsValidFileName(const char* pcszFile,
                               BOOL fFullyQualified);

    BOOL doshMakeRealName(PSZ pszTarget, PSZ pszSource, CHAR cReplace, BOOL fIsFAT);

    ULONG doshQueryFileSize(HFILE hFile);

    ULONG doshQueryPathSize(PSZ pszFile);

    APIRET doshQueryPathAttr(const char* pcszFile,
                             PULONG pulAttr);

    APIRET doshSetPathAttr(const char* pcszFile,
                           ULONG ulAttr);

    APIRET doshLoadTextFile(const char *pcszFile,
                            PSZ* ppszContent);

    PSZ doshCreateBackupFileName(const char* pszExisting);

    APIRET doshWriteTextFile(const char* pszFile,
                             const char* pszContent,
                             PULONG pulWritten,
                             PSZ pszBackup);

    HFILE doshOpenLogFile(const char* pcszFilename);

    APIRET doshWriteToLogFile(HFILE hfLog, const char* pcsz);

    /* ******************************************************************
     *
     *   Directory helpers
     *
     ********************************************************************/

    BOOL doshQueryDirExist(const char *pcszDir);

    APIRET doshCreatePath(PSZ pszPath,
                          BOOL fHidden);

    APIRET doshQueryCurrentDir(PSZ pszBuf);

    APIRET doshSetCurrentDir(const char *pcszDir);

    #define DOSHDELDIR_RECURSE      0x0001
    #define DOSHDELDIR_DELETEFILES  0x0002

    APIRET doshDeleteDir(const char *pcszDir,
                         ULONG flFlags,
                         PULONG pulDirs,
                         PULONG pulFiles);

    /* ******************************************************************
     *
     *   Process helpers
     *
     ********************************************************************/

    APIRET doshExecVIO(const char *pcszExecWithArgs,
                       PLONG plExitCode);

    APIRET doshQuickStartSession(const char *pcszPath,
                                 const char *pcszParams,
                                 BOOL fForeground,
                                 USHORT usPgmCtl,
                                 BOOL fWait,
                                 PULONG pulSID,
                                 PPID ppid);

    /* ******************************************************************
     *
     *   Environment helpers
     *
     ********************************************************************/

    /*
     *@@ DOSENVIRONMENT:
     *      structure holding an array of environment
     *      variables (papszVars). This is initialized
     *      doshGetEnvironment,
     *
     *@@added V0.9.4 (2000-07-19) [umoeller]
     */

    typedef struct _DOSENVIRONMENT
    {
        ULONG       cVars;              // count of vars in papzVars
        PSZ         *papszVars;         // array of PSZ's to environment strings (VAR=VALUE)
    } DOSENVIRONMENT, *PDOSENVIRONMENT;

    APIRET doshParseEnvironment(const char *pcszEnv,
                                PDOSENVIRONMENT pEnv);

    APIRET doshGetEnvironment(PDOSENVIRONMENT pEnv);

    PSZ* doshFindEnvironmentVar(PDOSENVIRONMENT pEnv,
                                PSZ pszVarName);

    APIRET doshSetEnvironmentVar(PDOSENVIRONMENT pEnv,
                                 PSZ pszNewEnv,
                                 BOOL fAddFirst);

    APIRET doshConvertEnvironment(PDOSENVIRONMENT pEnv,
                                  PSZ *ppszEnv,
                                  PULONG pulSize);

    APIRET doshFreeEnvironment(PDOSENVIRONMENT pEnv);

    /********************************************************************
     *
     *   Executable helpers
     *
     ********************************************************************/

    /*
     *@@ DOSEXEHEADER:
     *      old DOS EXE header at offset 0
     *      in any EXE file.
     *
     *@@changed V0.9.7 (2000-12-20) [umoeller]: fixed NE offset
     */

    #pragma pack(1)
    typedef struct _DOSEXEHEADER
    {
         USHORT usDosExeID;             // 00: DOS exeid (0x5a4d)
         USHORT usFileLenMod512;        // 02: filelen mod 512
         USHORT usFileLenDiv512;        // 04: filelen div 512
         USHORT usSegFix;               // 06: count of segment adds to fix
         USHORT usHdrPargCnt;           // 08: size of header in paragrphs
         USHORT usMinAddPargCnt;        // 0a: minimum addtl paragraphs count
         USHORT usMaxAddPargCnt;        // 0c: max addtl paragraphs count
         USHORT usSSStartup;            // 0e: SS at startup
         USHORT usSPStartup;            // 10: SP at startup
         USHORT usHdrChecksum;          // 12: header checksum
         USHORT usIPStartup;            // 14: IP at startup
         USHORT usCodeSegOfs;           // 16: code segment offset from EXE start
         USHORT usRelocTableOfs;        // 18: reloc table ofs.header (Win: >= 0x40)
         USHORT usOverlayNo;            // 1a: overlay no.
         ULONG  ulLinkerVersion;        // 1c: linker version (if 0x18 > 0x28)
         ULONG  ulUnused1;              // 20: unused
         ULONG  ulUnused2;              // 24: exe.h says the following fields are
         ULONG  ulUnused3;              // 28:   'behavior bits'
         ULONG  ulUnused4;              // 3c:
         ULONG  ulUnused5;              // 30:
         ULONG  ulUnused6;              // 34:
         ULONG  ulUnused7;              // 38:
         ULONG  ulNewHeaderOfs;         // new header ofs (if 0x18 > 0x40)
                    // fixed this from USHORT, thanks Martin Lafaix
                    // V0.9.7 (2000-12-20) [umoeller]
    } DOSEXEHEADER, *PDOSEXEHEADER;

    // NE and LX OS types
    #define NEOS_UNKNOWN        0
    #define NEOS_OS2            1   // Win16 SDK says: "reserved"...
    #define NEOS_WIN16          2
    #define NEOS_DOS4           3   // Win16 SDK says: "reserved"...
    #define NEOS_WIN386         4   // Win16 SDK says: "reserved"...

    /*
     *@@ NEHEADER:
     *      linear executable (LX) header format,
     *      which comes after the DOS header in the
     *      EXE file (at DOSEXEHEADER.usNewHeaderOfs).
     */

    typedef struct _NEHEADER
    {
        CHAR      achNE[2];             // 00: NE
        BYTE      bLinkerVersion;       // 02: linker version
        BYTE      bLinkerRevision;      // 03: linker revision
        USHORT    usEntryTblOfs;        // 04: ofs from this to entrytable
        USHORT    usEntryTblLen;        // 06: length of entrytable
        ULONG     ulChecksum;           // 08: MS: reserved, OS/2: checksum
        USHORT    usFlags;              // 0c: flags
        USHORT    usAutoDataSegNo;      // 0e: auto-data seg no.
        USHORT    usInitlHeapSize;      // 10: initl. heap size
        USHORT    usInitlStackSize;     // 12: initl. stack size
        ULONG     ulCSIP;               // 14: CS:IP
        ULONG     ulSSSP;               // 18: SS:SP
        USHORT    usSegTblEntries;      // 1c: segment tbl entry count
        USHORT    usModuleTblEntries;   // 1e: module ref. table entry count
        USHORT    usNonResdTblLen;      // 20: non-resd. name tbl length
        USHORT    usSegTblOfs;          // 22: segment tbl ofs
        USHORT    usResTblOfs;          // 24: resource tbl ofs
        USHORT    usResdNameTblOfs;     // 26: resd. name tbl ofs
        USHORT    usModRefTblOfs;       // 28: module ref. table ofs
        USHORT    usImportTblOfs;       // 2a: import tbl ofs
        ULONG     ulNonResdTblOfs;      // 2c: non-resd. name tbl ofs
        USHORT    usMoveableEntires;    // 30: moveable entry pts. count
        USHORT    usLogicalSectShift;   // 32: logcl. sector shift
        USHORT    usResSegmCount;       // 34: resource segm. count
        BYTE      bTargetOS;            // 36: target OS (NEOS_* flags)
        BYTE      bFlags2;              // 37: addtl. flags
        USHORT    usFastLoadOfs;        // 38: fast-load area ofs
        USHORT    usFastLoadLen;        // 3a: fast-load area length
        USHORT    usReserved;           // 3c: MS: 'reserved'
        USHORT    usReqWinVersion;      // 3e: Win-only: min. Win version
    } NEHEADER, *PNEHEADER;

    /*
     *@@ LXHEADER:
     *      linear executable (LX) header format,
     *      which comes after the DOS header in the
     *      EXE file (at DOSEXEHEADER.usNewHeaderOfs).
     */

    typedef struct _LXHEADER
    {
        CHAR        achLX[2];           // 00: LX or LE
            /* this is "LX" for 32-bit OS/2 programs, but
               "LE" for MS-DOS progs which use this format
               (e.g. WINDOWS\SMARTDRV.EXE) */
        BYTE      fByteBigEndian;       // 02: byte ordering (1 = big endian)
        BYTE      fWordBigEndian;       // 03: word ordering (1 = big endian)
        ULONG     ulFormatLevel;        // 04: format level
        USHORT    usCPU;                // 08: CPU type
        USHORT    usTargetOS;           // 0a: OS type (NEOS_* flags)
        ULONG     ulModuleVersion;      // 0c: module version
        ULONG     ulFlags;              // 10: module flags
        ULONG     ulPageCount;          // 14: no. of pages in module
        ULONG     ulEIPRelObj;          // 18: object to which EIP is relative
        ULONG     ulEIPEntryAddr;       // 1c: EIP entry addr
        ULONG     ulESPObj;             // 20: ESP object
        ULONG     ulESP;                // 24: ESP
        ULONG     ulPageSize;           // 28: page size (4K)
        ULONG     ulPageLeftShift;      // 2c: page left-shift
        ULONG     ulFixupTblLen;        // 30: fixup section total size
        ULONG     ulFixupTblChecksum;   // 34: fixup section checksum
        ULONG     ulLoaderLen;          // 38: size req. for loader section
        ULONG     ulLoaderChecksum;     // 3c: loader section checksum
        ULONG     ulObjTblOfs;          // 40: object table offset
        ULONG     ulObjCount;           // 44: object count
        ULONG     ulObjPageTblOfs;      // 48: object page table ofs
        ULONG     ulObjIterPagesOfs;    // 4c: object iter pages ofs
        ULONG     ulResTblOfs;          // 50: resource table ofs
        ULONG     ulResTblCnt;          // 54: resource entry count
        ULONG     ulResdNameTblOfs;     // 58: resident name tbl ofs
        ULONG     ulEntryTblOfs;        // 5c: entry tbl ofs
        ULONG     ulModDirectivesOfs;   // 60: module directives ofs
        ULONG     ulModDirectivesCnt;   // 64: module directives count
        ULONG     ulFixupPagePageTblOfs;// 68: fixup page tbl ofs
        ULONG     ulFixupRecTblOfs;     // 6c: fixup record tbl ofs
        ULONG     ulImportModTblOfs;    // 70: import modl tbl ofs
        ULONG     ulImportModTblCnt;    // 74: import modl tbl count
        ULONG     ulImportProcTblOfs;   // 78: import proc tbl ofs
        ULONG     ulPerPageCSOfs;       // 7c: per page checksum ofs
        ULONG     ulDataPagesOfs;       // 80: data pages ofs
        ULONG     ulPreloadPagesCnt;    // 84: preload pages count
        ULONG     ulNonResdNameTblOfs;  // 88: non-resdnt name tbl ofs
        ULONG     ulNonResdNameTblLen;  // 8c: non-resdnt name tbl length
        ULONG     ulNonResdNameTblCS;   // 90: non-res name tbl checksum
        ULONG     ulAutoDataSegObjCnt;  // 94: auto dataseg object count
        ULONG     ulDebugOfs;           // 98: debug info ofs
        ULONG     ulDebugLen;           // 9c: debug info length
        ULONG     ulInstancePrelCnt;    // a0: instance preload count
        ULONG     ulinstanceDemdCnt;    // a0: instance demand count
        ULONG     ulHeapSize16;         // a8: heap size (16-bit)
        ULONG     ulStackSize;          // ac: stack size
    } LXHEADER, *PLXHEADER;

    #pragma pack()

    // EXE format
    #define EXEFORMAT_OLDDOS        1
    #define EXEFORMAT_NE            2
    #define EXEFORMAT_PE            3
    #define EXEFORMAT_LX            4
    #define EXEFORMAT_TEXT_BATCH    5
    #define EXEFORMAT_TEXT_REXX     6

    // target OS (in NE and LX)
    #define EXEOS_DOS3              1
    #define EXEOS_DOS4              2       // there is a flag for this in NE
    #define EXEOS_OS2               3
    #define EXEOS_WIN16             4
    #define EXEOS_WIN386            5       // according to IBM, there are flags
                                            // for this both in NE and LX
    #define EXEOS_WIN32             6

    /*
     *@@ EXECUTABLE:
     *      structure used with all the doshExec*
     *      functions.
     */

    typedef struct _EXECUTABLE
    {
        HFILE               hfExe;

        /* All the following fields are set by
           doshExecOpen if NO_ERROR is returned. */

        // old DOS EXE header (always valid)
        PDOSEXEHEADER       pDosExeHeader;
        ULONG               cbDosExeHeader;

        // New Executable (NE) header, if ulExeFormat == EXEFORMAT_NE
        PNEHEADER           pNEHeader;
        ULONG               cbNEHeader;

        // Linear Executable (LX) header, if ulExeFormat == EXEFORMAT_LX
        PLXHEADER           pLXHeader;
        ULONG               cbLXHeader;

        // module analysis (always set):
        ULONG               ulExeFormat;
                // one of:
                // EXEFORMAT_OLDDOS        1
                // EXEFORMAT_NE            2
                // EXEFORMAT_PE            3
                // EXEFORMAT_LX            4
                // EXEFORMAT_TEXT_BATCH    5
                // EXEFORMAT_TEXT_REXX     6

        BOOL                fLibrary,
                            f32Bits;
        ULONG               ulOS;
                // one of:
                // EXEOS_DOS3              1
                // EXEOS_DOS4              2       // there is a flag for this in NE
                // EXEOS_OS2               3
                // EXEOS_WIN16             4
                // EXEOS_WIN386            5       // according to IBM, there are flags
                //                                 // for this both in NE and LX
                // EXEOS_WIN32             6

        /* The following fields are only set after
           an extra call to doshExecQueryBldLevel. */

        PSZ                 pszDescription;     // whole string (first non-res name tbl entry)
        PSZ                 pszVendor;          // vendor substring (if IBM BLDLEVEL format)
        PSZ                 pszVersion;         // version substring (if IBM BLDLEVEL format)
        PSZ                 pszInfo;            // module info substring (if IBM BLDLEVEL format)

    } EXECUTABLE, *PEXECUTABLE;

    APIRET doshExecOpen(const char* pcszExecutable,
                        PEXECUTABLE* ppExec);

    APIRET doshExecClose(PEXECUTABLE pExec);

    APIRET doshExecQueryBldLevel(PEXECUTABLE pExec);

    /********************************************************************
     *
     *   Partition functions
     *
     ********************************************************************/

    #define DOSH_PARTITIONS_LIMIT   10

    #define PAR_UNUSED      0x00    // Unused
    #define PAR_FAT12SMALL  0x01    // DOS FAT 12-bit < 10 Mb
    #define PAR_XENIXROOT   0x02    // XENIX root
    #define PAR_XENIXUSER   0x03    // XENIX user
    #define PAR_FAT16SMALL  0x04    // DOS FAT 16-bit < 32 Mb
    #define PAR_EXTENDED    0x05    // Extended partition
    #define PAR_FAT16BIG    0x06    // DOS FAT 16-bit > 32 Mb
    #define PAR_HPFS        0x07    // OS/2 HPFS
    #define PAR_AIXBOOT     0x08    // AIX bootable partition
    #define PAR_AIXDATA     0x09    // AIX bootable partition
    #define PAR_BOOTMANAGER 0x0A    // OS/2 Boot Manager
    #define PAR_WINDOWS95   0x0B    // Windows 95 32-bit FAT
    #define PAR_WINDOWS95LB 0x0C    // Windows 95 32-bit FAT with LBA
    #define PAR_VFAT16BIG   0x0E    // LBA VFAT (same as 06h but using LBA-mode)
    #define PAR_VFAT16EXT   0x0F    // LBA VFAT (same as 05h but using LBA-mode)
    #define PAR_OPUS        0x10    // OPUS
    #define PAR_HID12SMALL  0x11    // OS/2 hidden DOS FAT 12-bit
    #define PAR_COMPAQDIAG  0x12    // Compaq diagnostic
    #define PAR_HID16SMALL  0x14    // OS/2 hidden DOS FAT 16-bit
    #define PAR_HID16BIG    0x16    // OS/2 hidden DOS FAT 16-bit
    #define PAR_HIDHPFS     0x17    // OS/2 hidden HPFS
    #define PAR_WINDOWSSWP  0x18    // AST Windows Swap File
    #define PAR_NECDOS      0x24    // NEC MS-DOS 3.x
    #define PAR_THEOS       0x38    // THEOS
    #define PAR_VENIX       0x40    // VENIX
    #define PAR_RISCBOOT    0x41    // Personal RISC boot
    #define PAR_SFS         0x42    // SFS
    #define PAR_ONTRACK     0x50    // Ontrack
    #define PAR_ONTRACKEXT  0x51    // Ontrack extended partition
    #define PAR_CPM         0x52    // CP/M
    #define PAR_UNIXSYSV    0x63    // UNIX SysV/386
    #define PAR_NOVELL_64   0x64    // Novell
    #define PAR_NOVELL_65   0x65    // Novell
    #define PAR_NOVELL_67   0x67    // Novell
    #define PAR_NOVELL_68   0x68    // Novell
    #define PAR_NOVELL_69   0x69    // Novell
    #define PAR_PCIX        0x75    // PCIX
    #define PAR_MINIX       0x80    // MINIX
    #define PAR_LINUX       0x81    // Linux
    #define PAR_LINUXSWAP   0x82    // Linux Swap Partition
    #define PAR_LINUXFILE   0x83    // Linux File System
    #define PAR_FREEBSD     0xA5    // FreeBSD
    #define PAR_BBT         0xFF    // BBT

    // one-byte alignment
    #pragma pack(1)

    /*
     *@@ PAR_INFO:
     *      partition table
     */

    typedef struct _PAR_INFO
    {
        BYTE      bBootFlag;          // 0=not active, 80H = active (boot this partition
        BYTE      bBeginHead;         // partition begins at this head...
        USHORT    rBeginSecCyl;       //  ...and this sector and cylinder (see below)
        BYTE      bFileSysCode;       // file system type
        BYTE      bEndHead;           // partition ends at this head...
        USHORT    bEndSecCyl;         // ...and this sector and cylinder (see below)
        ULONG     lBeginAbsSec;       // partition begins at this absolute sector #
        ULONG     lTotalSects;        // total sectors in this partition
    } PAR_INFO, *PPAR_INFO;

    /*
     *@@ MBR_INFO:
     *      master boot record table.
     *      This has the four primary partitions.
     */

    typedef struct _MBR_INFO                 // MBR
    {
        BYTE      aBootCode[0x1BE];   // abBootCode master boot executable code
        PAR_INFO  sPrtnInfo[4];       // primary partition entries
        USHORT    wPrtnTblSig;        // partition table signature (aa55H)
    } MBR_INFO, *PMBR_INFO;

    /*
     *@@ SYS_INFO:
     *
     */

    typedef struct _SYS_INFO                 // информация о загружаемой системе
    {
        BYTE      startable;          // & 0x80
        BYTE      unknown[3];         // unknown
        BYTE      bootable;           // & 0x01
        BYTE      name[8];            // имя раздела
        BYTE      reservd[3];         // unknown
    } SYS_INFO, *PSYS_INFO;

    /*
     *@@ SYE_INFO:
     *
     */

    typedef struct _SYE_INFO                 // информация о загружаемой системе для
    {                               // расширенных разделов
        BYTE      bootable;           // & 0x01
        BYTE      name[8];            // имя раздела
    } SYE_INFO, *PSYE_INFO;

    /*
     *@@ EXT_INFO:
     *
     */

    typedef struct _EXT_INFO
    {
        BYTE      aBootCode[0x18A];   // abBootCode master boot executable code
        SYE_INFO  sBmNames[4];        // System Names
        BYTE      bReserved[16];      // reserved
        PAR_INFO  sPrtnInfo[4];       // partitioms entrys
        USHORT    wPrtnTblSig;        // partition table signature (aa55H)
    } EXT_INFO, *PEXT_INFO;

    typedef struct _PARTITIONINFO *PPARTITIONINFO;

    /*
     *@@ PARTITIONINFO:
     *      informational structure returned
     *      by doshGetPartitionsList. One of
     *      these items is created for each
     *      bootable partition.
     */

    typedef struct _PARTITIONINFO
    {
        BYTE  bDisk;                 // drive number
        CHAR  cLetter;               // probable drive letter or ' ' if none
        BYTE  bFSType;               // file system type
        CHAR  szFSType[10];          // file system name (created by us)
        BOOL  fPrimary;              // primary partition?
        BOOL  fBootable;             // bootable by Boot Manager?
        CHAR  szBootName[9];         // Boot Manager name, if (fBootable)
        ULONG ulSize;                // size MBytes
        PPARTITIONINFO pNext;        // next info or NULL if last
    } PARTITIONINFO;

    UINT doshQueryDiskCount(VOID);

    APIRET doshReadSector(USHORT disk,
                          void *buff,
                          USHORT head,
                          USHORT cylinder,
                          USHORT sector);

    // restore original alignment
    #pragma pack()

    char* doshType2FSName(unsigned char bFSType);

    APIRET doshGetBootManager(USHORT   *pusDisk,
                              USHORT   *pusPart,
                              PAR_INFO *BmInfo);

    APIRET doshGetPartitionsList(PPARTITIONINFO *ppPartitionInfo,
                                 PUSHORT pusPartitionCount,
                                 PUSHORT pusContext);

    APIRET doshFreePartitionsList(PPARTITIONINFO pPartitionInfo);

#endif

#if __cplusplus
}
#endif

