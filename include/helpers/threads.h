
/*
 *@@sourcefile threads.h:
 *      header file for treads.c. See remarks there.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@include #define INCL_DOSPROCESS
 *@@include #include <os2.h>
 *@@include #include "threads.h"
 */

/*
 *      Copyright (C) 1997-2000 Ulrich M”ller.
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

#ifndef THREADS_HEADER_INCLUDED
    #define THREADS_HEADER_INCLUDED

    #define THRF_PMMSGQUEUE     0x0001
    #define THRF_WAIT           0x0002
    #define THRF_TRANSIENT      0x0004
    #define THRF_WAIT_EXPLICIT  0x0008          // V0.9.9 (2001-03-14) [umoeller]

    /*
     *@@ THREADINFO:
     *      thread info structure passed to secondary threads.
     */

    typedef struct _THREADINFO
    {
            // data maintained by thr* functions
            ULONG   cbStruct;
            void*   pThreadFunc;    // as passed to thrCreate, really a PTHREADFUNC
            PBOOL   pfRunning;      // as passed to thrCreate
            const char *pcszThreadName; // as passed to thrCreate
            ULONG   flFlags;        // as passed to thrCreate
            ULONG   ulData;         // as passed to thrCreate

            TID     tid;
            ULONG   hevRunning;     // this is a HEV really

            // data maintained by thr_fntGeneric
            HAB     hab;            // for PM threads
            HMQ     hmq;            // for PM threads
            BOOL    fExitComplete;  // TRUE if thr_fntGeneric is exiting

            // data to be maintained by application
            BOOL    fExit;
            ULONG   ulResult;
            ULONG   ulFuncInfo;
            HWND    hwndNotify;
    } THREADINFO, *PTHREADINFO;

    typedef void _Optlink THREADFUNC (PTHREADINFO);
    typedef THREADFUNC *PTHREADFUNC;

    ULONG thrCreate(PTHREADINFO pti,
                    PTHREADFUNC pfn,
                    PBOOL pfRunning,
                    const char *pcszThreadName,
                    ULONG flFlags,
                    ULONG ulData);

    ULONG thrRunSync(HAB hab,
                     PTHREADFUNC pfn,
                     const char *pcszThreadName,
                     ULONG ulData);

    PTHREADINFO thrListThreads(PULONG pcThreads);

    BOOL thrFindThread(PTHREADINFO pti,
                       ULONG tid);

    BOOL thrClose(PTHREADINFO pti);

    BOOL thrWait(PTHREADINFO pti);

    BOOL thrFree(PTHREADINFO pti);

    BOOL thrKill(PTHREADINFO pti);

    TID thrQueryID(const THREADINFO* pti);

    ULONG thrQueryPriority(VOID);

#endif

#if __cplusplus
}
#endif

