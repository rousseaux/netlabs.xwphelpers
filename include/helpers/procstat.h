
/*
 *@@sourcefile procstat.h:
 *      header file for procstat.c (querying process information).
 *      See remarks there.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@include #include <os2.h>
 *@@include #include "procstat.h"
 */

/*
 *      This file Copyright (C) 1992-99 Ulrich M”ller,
 *                                      Kai Uwe Rommel.
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

#ifndef PROCSTAT_HEADER_INCLUDED
    #define PROCSTAT_HEADER_INCLUDED

    #pragma pack(1)

    /********************************************************************
     *
     *   DosQProcStatus declarations (16-bit)
     *
     ********************************************************************/

    #define PTR(ptr, ofs)  ((void *) ((char *) (ptr) + (ofs)))

    /* DosQProcStatus() = DOSCALLS.154 */
    USHORT APIENTRY16 DosQProcStatus(PVOID pBuf, USHORT cbBuf);
    /* DosGetPrty = DOSCALLS.9 */
    USHORT APIENTRY16 DosGetPrty(USHORT usScope, PUSHORT pusPriority, USHORT pid);

    /*
     *@@ QPROCSTAT16:
     *      "header" structure returned by DosQProcStat,
     *      containing the offsets to the other data
     */

    typedef struct _QPROCSTAT16
    {
        ULONG  ulGlobal;        // offset to global data section (QGLOBAL16)
        ULONG  ulProcesses;     // offset to process data section (QPROCESS16)
        ULONG  ulSemaphores;    // offset to semaphore data section
        ULONG  ulUnknown1;
        ULONG  ulSharedMem;     // offset to shared mem data section
        ULONG  ulModules;       // offset to DLL data section (QMODULE16)
        ULONG  ulUnknown2;
        ULONG  ulUnknown3;
    } QPROCSTAT16, *PQPROCSTAT16;

    /*
     *@@ QGLOBAL16:
     *      at offset QPROCSTAT.ulGlobal, contains
     *      global system information (no. of threads)
     */

    typedef struct _QGLOBAL16
    {
        ULONG  ulThreads;       // total number of threads;
        ULONG  ulReserved1,
               ulReserved2;
    } QGLOBAL16, *PQGLOBAL16;

    /*
     *@@ QPROCESS16:
     *      DosQProcStat structure for process info
     */

    typedef struct _QPROCESS16
    {
        ULONG  ulType;          // 1 for processes
        ULONG  ulThreadList;    // ofs to array of QTHREAD16 structs
        USHORT usPID;
        USHORT usParentPID;
        ULONG  ulSessionType;
                // according to bsedos.h, the PROG_* types are identical
                // to the SSF_TYPE_* types, so we have:
                // -- PROG_DEFAULT              0
                // -- PROG_FULLSCREEN           1
                // -- PROG_WINDOWABLEVIO        2
                // -- PROG_PM                   3
                // -- PROG_GROUP                5
                // -- PROG_REAL                 4
                // -- PROG_VDM                  4
                // -- PROG_WINDOWEDVDM          7
                // -- PROG_DLL                  6
                // -- PROG_PDD                  8
                // -- PROG_VDD                  9
                // -- PROG_WINDOW_REAL          10
                // -- PROG_WINDOW_PROT          11
                // -- PROG_30_STD               11
                // -- PROG_WINDOW_AUTO          12
                // -- PROG_SEAMLESSVDM          13
                // -- PROG_30_STDSEAMLESSVDM    13
                // -- PROG_SEAMLESSCOMMON       14
                // -- PROG_30_STDSEAMLESSCOMMON 14
                // -- PROG_31_STDSEAMLESSVDM    15
                // -- PROG_31_STDSEAMLESSCOMMON 16
                // -- PROG_31_ENHSEAMLESSVDM    17
                // -- PROG_31_ENHSEAMLESSCOMMON 18
                // -- PROG_31_ENH               19
                // -- PROG_31_STD               20
                // -- PROG_RESERVED             255
        ULONG  ulStatus;        // see status #define's below
        ULONG  ulSID;           // session (screen group) ID
        USHORT usHModule;       // program module handle for process
        USHORT usThreads;       // # of TCBs in use in process
        ULONG  ulReserved1;
        ULONG  ulReserved2;
        USHORT usSemaphores;    // # of 16-bit semaphores
        USHORT usDLLs;          // # of linked DLLs
        USHORT usSharedMems;
        USHORT usReserved3;
        ULONG  ulSemList;       // offset to semaphore list
        ULONG  ulDLLList;       // offset to DLL list
        ULONG  ulSharedMemList; // offset to shared mem list
        ULONG  ulReserved4;
    } QPROCESS16, *PQPROCESS16;

    // process status flags
    #define STAT_EXITLIST 0x01  // processing exit list
    #define STAT_EXIT1    0x02  // exiting thread 1
    #define STAT_EXITALL  0x04  // whole process is exiting
    #define STAT_PARSTAT  0x10  // parent cares about termination
    #define STAT_SYNCH    0x20  // parent did exec-and-wait
    #define STAT_DYING    0x40  // process is dying
    #define STAT_EMBRYO   0x80  // process is in emryonic state

    /*
     *@@ QTHREAD16:
     *      DosQProcStat structure for thread info
     */

    typedef struct _QTHREAD16
    {
        ULONG  ulType;          // 0x100 for thread records
        USHORT usTID;           // thread ID
        USHORT usThreadSlotID;  // ???
        ULONG  ulBlockID;       // sleep id thread is sleeping on
        ULONG  ulPriority;
        ULONG  ulSysTime;
        ULONG  ulUserTime;
        UCHAR  ucStatus;        // TSTAT_* flags
        UCHAR  ucReserved1;
        USHORT usReserved2;
    } QTHREAD16, *PQTHREAD16;

    // thread status flags
    #define TSTAT_VOID          0   // uninitialized
    #define TSTAT_READY         1   // ready to run (waiting for CPU time)
    #define TSTAT_BLOCKED       2   // blocked on a block ID
    #define TSTAT_SUSPENDED     3   // DosSuspendThread
    #define TSTAT_CRITSEC       4   // blocked by another thread in a critical section
    #define TSTAT_RUNNING       5   // currently running
    #define TSTAT_READYBOOST    6   // ready, but apply I/O boost
    #define TSTAT_TSD           7   // thead waiting for thread swappable data (TSD)
    #define TSTAT_DELAYED       8   // delayed TKWakeup (almost ready)
    #define TSTAT_FROZEN        9   // frozen (FF_ICE)
    #define TSTAT_GETSTACK     10   // incoming thread swappable data (TSD)
    #define TSTAT_BADSTACK     11   // TSD failed to swap in

    /*
     *@@ QMODULE16:
     *      DosQProcStat structure for module info
     */

    typedef struct _QMODULE16
    {
        ULONG  nextmodule;
        USHORT modhandle;
        USHORT modtype;
        ULONG  submodules;
        ULONG  segments;
        ULONG  reserved;
        ULONG  namepointer;
        USHORT submodule[1];      // varying, see member submodules */
    } QMODULE16, *PQMODULE16;

    /*
     *@@ QSEMAPHORE16:
     *      DosQProcStat structure for semaphore info (16-bit only, I guess)
     */

    typedef struct _QSEMAPHORE16
    {
        ULONG  nextsem;
        USHORT owner;
        UCHAR  flag;
        UCHAR  refs;
        UCHAR  requests;
        UCHAR  reserved1;
        USHORT reserved2;
        USHORT index;
        USHORT dummy;
        UCHAR  name[1];       /* varying */
    } QSEMAPHORE16, *PQSEMAPHORE16;

    /*
     *@@ QSHAREDMEM16:
     *      DosQProcStat structure for shared memory info
     */

    typedef struct _QSHAREDMEM16
    {
        ULONG  nextseg;
        USHORT handle;            // handle for shared memory
        USHORT selector;          // shared memory selector
        USHORT refs;              // reference count
        UCHAR  name[1];           // varying
    } QSHAREDMEM16, *PQSHAREDMEM16;

    /********************************************************************
     *
     *   DosQuerySysState declarations (32-bit)
     *
     ********************************************************************/

    #define QS32_PROCESS      0x0001
    #define QS32_SEMAPHORE    0x0002
    #define QS32_MTE          0x0004
    #define QS32_FILESYS      0x0008
    #define QS32_SHMEMORY     0x0010
    #define QS32_DISK         0x0020
    #define QS32_HWCONFIG     0x0040
    #define QS32_NAMEDPIPE    0x0080
    #define QS32_THREAD       0x0100
    #define QS32_MODVER       0x0200

    #define QS32_STANDARD    (QS32_PROCESS | QS32_SEMAPHORE | QS32_MTE | QS32_FILESYS | QS32_SHMEMORY)

    APIRET APIENTRY DosQuerySysState(ULONG flQueries,
                                     ULONG arg1,
                                     ULONG arg2,
                                     ULONG _res_,
                                     PVOID buf,
                                     ULONG bufsz);

    /*
     *@@ QGLOBAL32:
     *      Pointed to by QTOPLEVEL32.
     */

    typedef struct _QGLOBAL32
    {
        ULONG   ulThreadCount;  // thread count
        ULONG   ulProcCount;    // process count
        ULONG   ulModuleCount;  // module count
    } QGLOBAL32, *PQGLOBAL32;

    /*
     *@@ QTHREAD32:
     *      Pointed to by QPROCESS32.
     */

    typedef struct _QTHREAD32
    {
        ULONG   rectype;        // 256 for thread
        USHORT  usTID;          // thread ID, process-specific
        USHORT  usSlotID;       // slot ID, this identifies the thread to the kernel
        ULONG   ulSleepID;      // something the kernel uses for blocking threads
        ULONG   ulPriority;     // priority flags
        ULONG   ulSystime;      // CPU time spent in system code
        ULONG   ulUsertime;     // CPU time spent in user code
        UCHAR   ucState;        // one of the following:
                    // -- TSTAT_READY   1
                    // -- TSTAT_BLOCKED 2
                    // -- TSTAT_RUNNING 5
                    // -- TSTAT_LOADED  9
        UCHAR   _reserved1_;    /* padding to ULONG */
        USHORT  _reserved2_;    /* padding to ULONG */
    } QTHREAD32, *PQTHREAD32;

    // found the following in the "OS/2 Debugging handbook"
    // (the identifiers are not official, but invented by
    // me; V0.9.1 (2000-02-12) [umoeller]):
    // these are the flags for QFDS32.flFlags
    #define FSF_CONSOLEINPUT            0x0001      // bit 0
    #define FSF_CONSOLEOUTPUT           0x0002      // bit 1
    #define FSF_NULLDEVICE              0x0004      // bit 2
    #define FSF_CLOCKDEVICE             0x0008      // bit 3
    // #define FSF_UNUSED1                 0x0010      // bit 4
    #define FSF_RAWMODE                 0x0020      // bit 5
    #define FSF_DEVICEIDNOTDIRTY        0x0040      // bit 6
    #define FSF_LOCALDEVICE             0x0080      // bit 7
    #define FSF_NO_SFT_HANDLE_ALLOCTD   0x0100      // bit 8
    #define FSF_THREAD_BLOCKED_ON_SF    0x0200      // bit 9
    #define FSF_THREAD_BUSY_ON_SF       0x0400      // bit 10
    #define FSF_NAMED_PIPE              0x0800      // bit 11
    #define FSF_SFT_USES_FCB            0x1000      // bit 12
    #define FSF_IS_PIPE                 0x2000      // bit 13;
                // then bit 11 determines whether this pipe is named or unnamed
    // #define FSF_UNUSED2                 0x4000      // bit 14
    #define FSF_REMOTE_FILE             0x8000      // bit 15
                // otherwise local file or device

    /*
     *@@ QFDS32:
     *      open file entry.
     *      Pointed to by QFILEDATA32.
     */

    typedef struct _QFDS32
    {
        USHORT  sfn;                // "system file number" of the file.
                                    // This is the same as in
                                    // the QPROCESS32.pausFds array,
                                    // so we can identify files opened
                                    // by a process. File handles returned
                                    // by DosOpen ("job file numbers", JFN's)
                                    // are mapped to SFN's for each process
                                    // individually.
        USHORT  usRefCount;
        ULONG   flFlags;            // FSF_* flags above
        ULONG   flAccess;           // fsOpenMode flags of DosOpen:
              /* #define OPEN_ACCESS_READONLY               0x0000
                 #define OPEN_ACCESS_WRITEONLY              0x0001
                 #define OPEN_ACCESS_READWRITE              0x0002
                 #define OPEN_SHARE_DENYREADWRITE           0x0010
                 #define OPEN_SHARE_DENYWRITE               0x0020
                 #define OPEN_SHARE_DENYREAD                0x0030
                 #define OPEN_SHARE_DENYNONE                0x0040
                 #define OPEN_FLAGS_NOINHERIT               0x0080
                 #define OPEN_FLAGS_NO_LOCALITY             0x0000
                 #define OPEN_FLAGS_SEQUENTIAL              0x0100
                 #define OPEN_FLAGS_RANDOM                  0x0200
                 #define OPEN_FLAGS_RANDOMSEQUENTIAL        0x0300
                 #define OPEN_FLAGS_NO_CACHE                0x1000
                 #define OPEN_FLAGS_FAIL_ON_ERROR           0x2000
                 #define OPEN_FLAGS_WRITE_THROUGH           0x4000
                 #define OPEN_FLAGS_DASD                    0x8000
                 #define OPEN_FLAGS_NONSPOOLED          0x00040000
                 #define OPEN_FLAGS_PROTECTED_HANDLE    0x40000000 */

        ULONG   ulFileSize;         // file size in bytes
        USHORT  hVolume;            // "volume handle"; apparently,
                                    // this identifies some kernel
                                    // structure, it's the same for
                                    // files on the same disk
        USHORT  attrib;             // attributes:
                                    // 0x20: 'A' (archived)
                                    // 0x10: 'D' (directory)
                                    // 0x08: 'L' (?!?)
                                    // 0x04: 'S' (system)
                                    // 0x02: 'H' (hidden)
                                    // 0x01: 'R' (read-only)
        USHORT  _reserved_;
    } QFDS32, *PQFDS32;

    /*
     *@@ QFILEDATA32:
     *      open files linked-list item.
     *
     *      First item is pointed to by QTOPLEVEL32.
     */

    typedef struct _QFILEDATA32
    {
        ULONG           rectype;        // 8 for file
        struct _QFILEDATA32 *pNext;
        ULONG           ulOpenCount;
        PQFDS32         filedata;
        char            acFilename[1];
    } QFILEDATA32, *PQFILEDATA32;

    /*
     *@@ QPROCESS32:
     *      process description structure.
     *
     *      Pointed to by QTOPLEVEL32.
     *
     *      Following this structure is an array
     *      of ulPrivSem32Count 32-bit semaphore
     *      descriptions.
     */

    typedef struct _QPROCESS32
    {
        ULONG       rectype;        // 1 for process
        PQTHREAD32  pThreads;       // thread data array; apperently with usThreadCount items
        USHORT      pid;            // process ID
        USHORT      ppid;           // parent process ID
        ULONG       ulProgType;
                // -- 0: Full screen protected mode.
                // -- 1: Real mode (probably DOS or Windoze).
                // -- 2: VIO windowable protected mode.
                // -- 3: Presentation manager protected mode.
                // -- 4: Detached protected mode.
        ULONG       ulState;    // one of the following:
                // -- STAT_EXITLIST 0x01
                // -- STAT_EXIT1    0x02
                // -- STAT_EXITALL  0x04
                // -- STAT_PARSTAT  0x10
                // -- STAT_SYNCH    0x20
                // -- STAT_DYING    0x40
                // -- STAT_EMBRYO   0x80
        ULONG       sessid;         // session ID
        USHORT      usHModule;      // module handle of main executable
        USHORT      usThreadCount;  // no. of threads
        ULONG       ulPrivSem32Count;  // count of 32-bit semaphores
        ULONG       ulPrivSem32s;   // should be ptr to 32-bit sems array; 0 always
        USHORT      usSem16Count;   // count of 16-bit semaphores in pausSem16 array
        USHORT      usModuleCount;  // count of DLLs owned by this process
        USHORT      usShrMemCount;  // count of shared memory items
        USHORT      usFdsCount;     // count of open files; this is mostly way too large
        PUSHORT     pausSem16;      // ptr to array of 16-bit semaphore handles;
                                    // has usSem16Count items
        PUSHORT     pausModules;    // ptr to array of modules (MTE);
                                    // has usModuleCount items
        PUSHORT     pausShrMems;    // ptr to array of shared mem items;
                                    // has usShrMemCount items
        PUSHORT     pausFds;        // ptr to array of file handles;
                                    // many of these are pseudo-file handles, but
                                    // will be the same as the QFDS32.sfn field,
                                    // so open files can be identified.
    } QPROCESS32, *PQPROCESS32;

    /*
     *@@ QSEMA32:
     *      Member of QSEM16STRUC32.
     */

    typedef struct _QSEMA32
    {
        struct _QSEMA32 *pNext;
        USHORT      usRefCount;
        UCHAR       ucSysFlags;
        UCHAR       ucSysProcCount;
        ULONG       _reserved1_;
        USHORT      usIndex;
        CHAR        acName[1];
    } QSEMA32, *PQSEMA32;

    /*
     *@@ QSEM16STRUC32:
     *      Apparently 16-bit semaphores.
     *
     *      Pointed to by QTOPLEVEL32.
     */

    typedef struct _QSEM16STRUC32
    {
        ULONG   rectype;        /**/
        ULONG   _reserved1_;
        USHORT  _reserved2_;
        USHORT  usSysSemIndex;
        ULONG   ulIndex;
        QSEMA32   sema;
    } QSEM16STRUC32, *PQSEM16STRUC32;

    /*
     *@@ QSEM32OWNER32:
     *      Pointed to by QSEM16SMUX32, QSEM16EV32, QSEM16MUX32.
     */

    typedef struct _QSEM32OWNER32
    {
        USHORT  pid;
        USHORT  usOpenCount;        // or owner?!?
    } QSEM32OWNER32, *PQSEM32OWNER32;

    /*
     *@@ QSEM32SMUX32:
     *      member of QSEM32STRUC32.
     */

    typedef struct _QSEM32SMUX32
    {
        PQSEM32OWNER32  pOwner;
        PCHAR           pszName;
        PVOID           semrecs;        // array of associated sema's
        USHORT          flags;
        USHORT          semreccnt;
        USHORT          waitcnt;
        USHORT          _reserved_;     // padding to ULONG
    } QSEM32SMUX32, *PQSEM32SMUX32;

    /*
     *@@ QSEM32EV32:
     *      describes an event semaphore.
     *      Member of QSEM32STRUC32.
     */

    typedef struct _QSEM32EV32
    {
        PQSEM32OWNER32  pOwner;
        PCHAR           pacName;
        PQSEM32SMUX32   pMux;
        USHORT          flags;
        USHORT          postcnt;
    } QSEM32EV32, *PQSEM32EV32;

    /*
     *@@ QSEM32MUX32:
     *      member of QSEM32STRUC32.
     */

    typedef struct _QSEM32MUX32
    {
        PQSEM32OWNER32  pOwner;
        PCHAR           name;
        PQSEM32SMUX32   mux;
        USHORT          flags;
        USHORT          usRefCount;
        USHORT          thrdnum;
        USHORT          _reserved_;     /* padding to ULONG */
    } QSEM32MUX32, *PQSEM32MUX32;

    /*
     *@@ QSEM32STRUC32:
     *      apparently describes a 32-bit semaphore.
     *
     *      Pointed to by QTOPLEVEL32.
     */

    typedef struct _QSEM32STRUC32
    {
        struct _QSEM32STRUC32 *pNext;
        QSEM32EV32    EventSem;
        QSEM32MUX32   MuxSem;
        QSEM32SMUX32  SmuxSem;
    } QSEM32STRUC32, *PQSEM32STRUC32;

    /*
     *@@ QSHRMEM32:
     *      describes a shared memory block.
     *
     *      Pointed to by QTOPLEVEL32.
     */

    typedef struct _QSHRMEM32
    {
        struct _QSHRMEM32 *pNext;
        USHORT      usHandle;
        USHORT      usSelector;
        USHORT      usRefCount;
        CHAR        acName[1];
    } QSHRMEM32, *PQSHRMEM32;

    /*
     *@@ QMODULE32:
     *      describes an executable module.
     *
     *      Pointed to by QTOPLEVEL32.
     */

    typedef struct _QMODULE32
    {
        struct _QMODULE32 *pNext;       // next module
        USHORT      usHModule;          // module handle
        USHORT      type;
        ULONG       ulRefCount;
        ULONG       ulSegmentCount;
        PVOID       _reserved_;
        PCHAR       pcName;             // module name (fully qualified)
        USHORT      ausModRef[1];       // array of module "references";
                                        // this has usRefCount items
                                        // and holds other modules (imports)
    } QMODULE32, *PQMODULE32;

    /*
     *@@ QTOPLEVEL32:
     *      head of the buffer returned by
     *      DosQuerySysState.
     */

    typedef struct _QTOPLEVEL32
    {
        PQGLOBAL32      pGlobalData;
        PQPROCESS32     pProcessData;
        PQSEM16STRUC32  pSem16Data;
        PQSEM32STRUC32  pSem32Data;     // not always present!
        PQSHRMEM32      pShrMemData;
        PQMODULE32      pModuleData;
        PVOID           _reserved2_;
        PQFILEDATA32    pFileData;      // only present in FP19 or later or W4
    } QTOPLEVEL32, *PQTOPLEVEL32;

    /********************************************************************
     *
     *   New procstat.c declarations
     *
     ********************************************************************/

    /*
     *@@ PRCPROCESS:
     *      additional, more lucid structure
     *      filled by prc16QueryProcessInfo.
     */

    typedef struct _PRCPROCESS
    {
        CHAR   szModuleName[CCHMAXPATH];    // module name
        USHORT usPID,                       // process ID
               usParentPID,                 // parent process ID
               usThreads;                   // thread count
        ULONG  ulSID;                       // session ID
        ULONG  ulSessionType;
        ULONG  ulStatus;
        ULONG  ulCPU;                       // CPU usage (sum of thread data)
    } PRCPROCESS, *PPRCPROCESS;

    /*
     *@@ PRCTHREAD:
     *      additional, more lucid structure
     *      filled by prc16QueryThreadInfo.
     */

    typedef struct _PRCTHREAD
    {
        USHORT usTID;           // thread ID
        USHORT usThreadSlotID;  // kernel thread slot ID
        ULONG  ulBlockID;       // sleep id thread is sleeping on
        ULONG  ulPriority;
        ULONG  ulSysTime;
        ULONG  ulUserTime;
        UCHAR  ucStatus;        // see status #define's below
    } PRCTHREAD, *PPRCTHREAD;

    #pragma pack()

   /********************************************************************
    *
    *   DosQProcStat (16-bit) interface
    *
    ********************************************************************/

    PQPROCSTAT16 prc16GetInfo(APIRET *parc);

    VOID prc16FreeInfo(PQPROCSTAT16 pInfo);

    PQPROCESS16 prc16FindProcessFromName(PQPROCSTAT16 pInfo,
                                         const char *pcszName);

    PQPROCESS16 prc16FindProcessFromPID(PQPROCSTAT16 pInfo,
                                        ULONG ulPID);

    /********************************************************************
     *
     *   DosQProcStat (16-bit) helpers
     *
     ********************************************************************/

    BOOL prc16QueryProcessInfo(USHORT usPID, PPRCPROCESS pprcp);

    ULONG prc16ForEachProcess(PFNWP pfnwpCallback, HWND hwnd, ULONG ulMsg, MPARAM mp1);

    ULONG prc16QueryThreadCount(USHORT usPID);

    BOOL prc16QueryThreadInfo(USHORT usPID, USHORT usTID, PPRCTHREAD pprct);

    ULONG prc16QueryThreadPriority(USHORT usPID, USHORT usTID);

    /********************************************************************
     *
     *   DosQuerySysState (32-bit) interface
     *
     ********************************************************************/

    PQTOPLEVEL32 prc32GetInfo(APIRET *parc);

    VOID prc32FreeInfo(PQTOPLEVEL32 pInfo);

    PQPROCESS32 prc32FindProcessFromName(PQTOPLEVEL32 pInfo,
                                         const char *pcszName);

    PQSEMA32 prc32FindSem16(PQTOPLEVEL32 pInfo,
                              USHORT usSemID);

    PQSEM32STRUC32 prc32FindSem32(PQTOPLEVEL32 pInfo,
                                  USHORT usSemID);

    PQSHRMEM32 prc32FindShrMem(PQTOPLEVEL32 pInfo,
                               USHORT usShrMemID);

    PQMODULE32 prc32FindModule(PQTOPLEVEL32 pInfo,
                               USHORT usHModule);

    PQFILEDATA32 prc32FindFileData(PQTOPLEVEL32 pInfo,
                                   USHORT usFileID);

#endif

#if __cplusplus
}
#endif

