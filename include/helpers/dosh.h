
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
 *@@include #include "helpers\dosh.h"
 */

/*      This file Copyright (C) 1997-2001 Ulrich M”ller,
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

    ULONG doshIsWarp4(VOID);

    PSZ doshQuerySysErrorMsg(APIRET arc);

    ULONG doshQuerySysUptime(VOID);

    /* ******************************************************************
     *
     *   Memory helpers
     *
     ********************************************************************/

    PVOID doshMalloc(ULONG cb,
                     APIRET *parc);

    APIRET doshAllocArray(ULONG c,
                          ULONG cbArrayItem,
                          PBYTE *ppv,
                          PULONG pcbAllocated);

    PVOID doshAllocSharedMem(ULONG ulSize,
                             const char* pcszName);

    PVOID doshRequestSharedMem(PCSZ pcszName);

    /* ******************************************************************
     *
     *   Drive helpers
     *
     ********************************************************************/

    APIRET doshIsFixedDisk(ULONG  ulLogicalDrive,
                           PBOOL  pfFixed);

    #ifdef INCL_DOSDEVIOCTL

        // flags for DRIVEPARMS.usDeviceAttrs (see DSK_GETDEVICEPARAMS in CPREF):
        #define DEVATTR_REMOVEABLE  0x0001      // drive is removeable
        #define DEVATTR_CHANGELINE  0x0002      // media has been removed since last I/O operation
        #define DEVATTR_GREATER16MB 0x0004      // physical device driver supports physical addresses > 16 MB

        // #pragma pack(1)

        /*
         * DRIVEPARAMS:
         *      structure used for doshQueryDiskParams.
         * removed this, we can directly use BIOSPARAMETERBLOCK
         * V0.9.13 (2001-06-14) [umoeller]
         */

        /* typedef struct _DRIVEPARAMS
        {
            BIOSPARAMETERBLOCK bpb;
                        // BIOS parameter block. This is the first sector
                        // (at byte 0) in each partition. This is defined
                        // in the OS2 headers as follows:

                        typedef struct _BIOSPARAMETERBLOCK {
                    0     USHORT     usBytesPerSector;
                                        //  Number of bytes per sector.
                    2     BYTE       bSectorsPerCluster;
                                        //  Number of sectors per cluster.
                    3     USHORT     usReservedSectors;
                                        //  Number of reserved sectors.
                    5     BYTE       cFATs;
                                        //  Number of FATs.
                    6     USHORT     cRootEntries;
                                        //  Number of root directory entries.
                    8     USHORT     cSectors;
                                        //  Number of sectors.
                    10    BYTE       bMedia;
                                        //  Media descriptor.
                    11    USHORT     usSectorsPerFAT;
                                        //  Number of secctors per FAT.
                    13    USHORT     usSectorsPerTrack;
                                        //  Number of sectors per track.
                    15    USHORT     cHeads;
                                        //  Number of heads.
                    17    ULONG      cHiddenSectors;
                                        //  Number of hidden sectors.
                    21    ULONG      cLargeSectors;
                                        //  Number of large sectors.
                    25    BYTE       abReserved[6];
                                        //  Reserved.
                    31    USHORT     cCylinders;
                                        //  Number of cylinders defined for the physical
                                        // device.
                    33    BYTE       bDeviceType;
                                        //  Physical layout of the specified device.
                    34    USHORT     fsDeviceAttr;
                                        //  A bit field that returns flag information
                                        //  about the specified drive.
                        } BIOSPARAMETERBLOCK;

            // removed the following fields... these are already
            // in the extended BPB structure, as defined in the
            // Toolkit's BIOSPARAMETERBLOCK struct. Checked this,
            // the definition is the same for the Toolkit 3 and 4.5.

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
                        // --  7:  Other (includes 1.44MB 3.5-inch diskette drive
                        //         and CD-ROMs)
                        // --  8:  R/W optical disk
                        // --  9:  3.5-inch 4.0MB diskette drive (2.88MB formatted)
            USHORT  usDeviceAttrs;
                        // DEVATTR_* flags
        } DRIVEPARAMS, *PDRIVEPARAMS;
        #pragma pack() */

        APIRET doshQueryDiskParams(ULONG ulLogicalDrive,
                                   PBIOSPARAMETERBLOCK pdp);

        BOOL XWPENTRY doshIsCDROM(PBIOSPARAMETERBLOCK pdp);

    #endif

    APIRET XWPENTRY doshHasAudioCD(ULONG ulLogicalDrive,
                                   HFILE hfDrive,
                                   BOOL fMixedModeCD,
                                   PBOOL pfAudio);

    VOID XWPENTRY doshEnumDrives(PSZ pszBuffer,
                                 PCSZ pcszFileSystem,
                                 BOOL fSkipRemoveables);
    typedef VOID XWPENTRY DOSHENUMDRIVES(PSZ pszBuffer,
                                         PCSZ pcszFileSystem,
                                         BOOL fSkipRemoveables);
    typedef DOSHENUMDRIVES *PDOSHENUMDRIVES;

    CHAR doshQueryBootDrive(VOID);

    #define ERROR_AUDIO_CD_ROM      10000

    #define ASSERTFL_MIXEDMODECD    0x0001

    APIRET doshAssertDrive(ULONG ulLogicalDrive,
                           ULONG fl);

    APIRET doshSetLogicalMap(ULONG ulLogicalDrive);

    APIRET XWPENTRY doshQueryDiskSize(ULONG ulLogicalDrive, double *pdSize);
    typedef APIRET XWPENTRY DOSHQUERYDISKSIZE(ULONG ulLogicalDrive, double *pdSize);
    typedef DOSHQUERYDISKSIZE *PDOSHQUERYDISKSIZE;

    APIRET XWPENTRY doshQueryDiskFree(ULONG ulLogicalDrive, double *pdFree);
    typedef APIRET XWPENTRY DOSHQUERYDISKFREE(ULONG ulLogicalDrive, double *pdFree);
    typedef DOSHQUERYDISKFREE *PDOSHQUERYDISKFREE;

    APIRET XWPENTRY doshQueryDiskFSType(ULONG ulLogicalDrive, PSZ pszBuf, ULONG cbBuf);
    typedef APIRET XWPENTRY DOSHQUERYDISKFSTYPE(ULONG ulLogicalDrive, PSZ pszBuf, ULONG cbBuf);
    typedef DOSHQUERYDISKFSTYPE *PDOSHQUERYDISKFSTYPE;

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
     *@@changed V0.9.9 (2001-03-14) [umoeller]: added interrupt load
     */

    typedef struct _DOSHPERFSYS
    {
        // output: no. of processors on the system
        ULONG       cProcessors;
        // output: one CPU load for each CPU
        PLONG       palLoads;

        // output: one CPU interrupt load for each CPU
        PLONG       palIntrs;

        // each of the following ptrs points to an array of cProcessors items
        PCPUUTIL    paCPUUtils;     // CPUUTIL structures
        double      *padBusyPrev;   // previous "busy" calculations
        double      *padTimePrev;   // previous "time" calculations
        double      *padIntrPrev;   // previous "intr" calculations

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
     *   File name parsing
     *
     ********************************************************************/

    APIRET doshGetDriveSpec(PCSZ pcszFullFile,
                            PSZ pszDrive,
                            PULONG pulDriveLen,
                            PBOOL pfIsUNC);

    PSZ doshGetExtension(PCSZ pcszFilename);

    /* ******************************************************************
     *
     *   File helpers
     *
     ********************************************************************/

    BOOL doshIsFileOnFAT(const char* pcszFileName);

    APIRET doshIsValidFileName(const char* pcszFile,
                               BOOL fFullyQualified);

    BOOL doshMakeRealName(PSZ pszTarget, PSZ pszSource, CHAR cReplace, BOOL fIsFAT);

    APIRET doshQueryFileSize(HFILE hFile,
                             PULONG pulSize);

    APIRET doshQueryPathSize(PCSZ pcszFile,
                             PULONG pulSize);

    APIRET doshQueryPathAttr(const char* pcszFile,
                             PULONG pulAttr);

    APIRET doshSetPathAttr(const char* pcszFile,
                           ULONG ulAttr);

    /* APIRET doshOpenExisting(PCSZ pcszFilename,
                            ULONG ulOpenFlags,
                            HFILE *phf);
       */

    /*
     *@@ XFILE:
     *
     *@@added V0.9.16 (2001-10-19) [umoeller]
     */

    typedef struct _XFILE
    {
        HFILE       hf;
        ULONG       hmtx;       // a HMTX really

        PSZ         pszFilename;    // as given to doshOpen
        ULONG       flOpenMode;     // as given to doshOpen

        ULONG       cbInitial,  // intial file size on open (can be 0 if new)
                    cbCurrent;  // current file size (raised with each write)

        PBYTE       pbCache;    // if != NULL, cached data from doshReadAt
        ULONG       cbCache,    // size of data in cbCache
                    ulReadFrom; // file offset where pbCache was read from
    } XFILE, *PXFILE;

    #define XOPEN_READ_EXISTING           0x0001
    #define XOPEN_READWRITE_EXISTING      0x0002
    #define XOPEN_READWRITE_APPEND        0x0003
    #define XOPEN_READWRITE_NEW           0x0004
    #define XOPEN_ACCESS_MASK             0xffff

    #define XOPEN_BINARY              0x10000000

    APIRET doshOpen(PCSZ pcszFilename,
                    ULONG flOpenMode,
                    PULONG pcbFile,
                    PXFILE *ppFile);

    #define DRFL_NOCACHE            0x0001
    #define DRFL_FAILIFLESS         0x0002

    APIRET doshReadAt(PXFILE pFile,
                      ULONG ulOffset,
                      PULONG pcb,
                      PBYTE pbData,
                      ULONG fl);

    APIRET doshWrite(PXFILE pFile,
                     ULONG cb,
                     PCSZ pbData);

    APIRET doshWriteAt(PXFILE pFile,
                       ULONG ulOffset,
                       ULONG cb,
                       PCSZ pbData);

    APIRET doshWriteLogEntry(PXFILE pFile,
                             const char* pcszFormat,
                             ...);

    APIRET doshClose(PXFILE *ppFile);

    APIRET doshLoadTextFile(PCSZ pcszFile,
                            PSZ* ppszContent,
                            PULONG pcbRead);

    PSZ doshCreateBackupFileName(const char* pszExisting);

    APIRET doshCreateTempFileName(PSZ pszTempFileName,
                                  PCSZ pcszDir,
                                  PCSZ pcszPrefix,
                                  PCSZ pcszExt);

    APIRET doshWriteTextFile(const char* pszFile,
                             const char* pszContent,
                             PULONG pulWritten,
                             PSZ pszBackup);


    /* ******************************************************************
     *
     *   Directory helpers
     *
     ********************************************************************/

    BOOL doshQueryDirExist(PCSZ pcszDir);

    APIRET doshCreatePath(PCSZ pcszPath,
                          BOOL fHidden);

    APIRET doshQueryCurrentDir(PSZ pszBuf);

    APIRET doshSetCurrentDir(PCSZ pcszDir);

    #define DOSHDELDIR_RECURSE      0x0001
    #define DOSHDELDIR_DELETEFILES  0x0002

    APIRET doshDeleteDir(PCSZ pcszDir,
                         ULONG flFlags,
                         PULONG pulDirs,
                         PULONG pulFiles);

    /* ******************************************************************
     *
     *   Process helpers
     *
     ********************************************************************/

    ULONG XWPENTRY doshMyPID(VOID);
    typedef ULONG XWPENTRY DOSHMYPID(VOID);
    typedef DOSHMYPID *PDOSHMYPID;

    ULONG XWPENTRY doshMyTID(VOID);
    typedef ULONG XWPENTRY DOSHMYTID(VOID);
    typedef DOSHMYTID *PDOSHMYTID;

    APIRET doshExecVIO(PCSZ pcszExecWithArgs,
                       PLONG plExitCode);

    APIRET doshQuickStartSession(PCSZ pcszPath,
                                 PCSZ pcszParams,
                                 BOOL fForeground,
                                 USHORT usPgmCtl,
                                 BOOL fWait,
                                 PULONG pulSID,
                                 PPID ppid,
                                 PUSHORT pusReturn);

    APIRET doshSearchPath(PCSZ pcszPath,
                          PCSZ pcszFile,
                          PSZ pszExecutable,
                          ULONG cbExecutable);

    APIRET doshFindExecutable(PCSZ pcszCommand,
                              PSZ pszExecutable,
                              ULONG cbExecutable,
                              PCSZ *papcszExtensions,
                              ULONG cExtensions);

    /********************************************************************
     *
     *   Partition functions
     *
     ********************************************************************/

    /*
     *@@ LVMINFO:
     *      informational structure created by
     *      doshQueryLVMInfo.
     *
     *@@added V0.9.9 (2001-04-07) [umoeller]
     */

    typedef struct _LVMINFO
    {
        HMODULE hmodLVM;

    } LVMINFO, *PLVMINFO;

    /* #define DOSH_PARTITIONS_LIMIT   10

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
    */

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

    typedef struct _MBR_INFO          // MBR
    {
        BYTE      aBootCode[0x1BE];   // abBootCode master boot executable code
        PAR_INFO  sPrtnInfo[4];       // primary partition entries
        USHORT    wPrtnTblSig;        // partition table signature (aa55H)
    } MBR_INFO, *PMBR_INFO;

    /*
     *@@ SYS_INFO:
     *
     */

    typedef struct _SYS_INFO
    {
        BYTE      startable;
        BYTE      unknown[3];
        BYTE      bootable;
        BYTE      name[8];
        BYTE      reservd[3];
    } SYS_INFO, *PSYS_INFO;

    /*
     *@@ SYE_INFO:
     *
     */

    typedef struct _SYE_INFO
    {
        BYTE      bootable;
        BYTE      name[8];
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
        BYTE    bDisk;                 // drive number
        CHAR    cLetter;               // probable drive letter or ' ' if none
        BYTE    bFSType;               // file system type
        PCSZ    pcszFSType;            // file system name (as returned by
                                       // doshType2FSName, can be NULL!)
        BOOL    fPrimary;              // primary partition?
        BOOL    fBootable;             // bootable by Boot Manager?
        CHAR    szBootName[9];         // Boot Manager name, if (fBootable)
        ULONG   ulSize;                // size MBytes
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

    const char* doshType2FSName(unsigned char bFSType);

    APIRET doshGetBootManager(USHORT   *pusDisk,
                              USHORT   *pusPart,
                              PAR_INFO *BmInfo);

    typedef struct _PARTITIONSLIST
    {
        PLVMINFO        pLVMInfo;           // != NULL if LVM is installed

        // partitions array
        PPARTITIONINFO  pPartitionInfo;
        USHORT          cPartitions;
    } PARTITIONSLIST, *PPARTITIONSLIST;

    APIRET doshGetPartitionsList(PPARTITIONSLIST *ppList,
                                 PUSHORT pusContext);

    APIRET doshFreePartitionsList(PPARTITIONSLIST ppList);

    APIRET doshQueryLVMInfo(PLVMINFO *ppLVMInfo);

    APIRET doshReadLVMPartitions(PLVMINFO pInfo,
                                 PPARTITIONINFO *ppPartitionInfo,
                                 PUSHORT pcPartitions);

    VOID doshFreeLVMInfo(PLVMINFO pInfo);

    /* ******************************************************************
     *
     *   Wildcard matching
     *
     ********************************************************************/

    BOOL doshMatch(const char *pcszMask,
                   const char *pcszName);

#endif

#if __cplusplus
}
#endif

