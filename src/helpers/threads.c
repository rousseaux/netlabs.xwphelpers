
/*
 *@@sourcefile threads.c:
 *      contains helper functions for creating, destroying, and
 *      synchronizing threads, including PM threads with a
 *      message queue which is created automatically.
 *
 *      Usage: All OS/2 programs.
 *
 *      Function prefixes (new with V0.81):
 *      --  thr*        Thread helper functions
 *
 *      This file is new with V0.81 and contains all the thread
 *      functions that used to be in helpers.c.
 *
 *      Use thrCreate() to start a thread.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@header "helpers\threads.h"
 */

/*
 *      Copyright (C) 1997-2000 Ulrich M�ller.
 *      This file is part of the XWorkplace source package.
 *      XWorkplace is free software; you can redistribute it and/or modify
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
#define INCL_DOSSEMAPHORES
#define INCL_DOSERRORS
#include <os2.h>

#include <string.h>
#include <stdlib.h>

#include "setup.h"                      // code generation and debugging options

#include "helpers\threads.h"

#pragma hdrstop

/*
 *@@category: Helpers\Control program helpers\Thread management
 */

/*
 *@@ thr_fntGeneric:
 *      generic thread function used by thrCreate.
 *      This in turn calls the actual thread function
 *      specified with thrCreate.
 *
 *@@added V0.9.2 (2000-03-05) [umoeller]
 */

VOID _Optlink thr_fntGeneric(PVOID ptiMyself)
{
    PTHREADINFO pti = (PTHREADINFO)ptiMyself;

    if (pti)
    {
        if (pti->pfRunning)
            // set "running" flag
            *(pti->pfRunning) = TRUE;

        if (pti->flFlags & THRF_WAIT)
            // "Wait" flag set: thrCreate is then
            // waiting on the wait event sem posted
            DosPostEventSem(pti->hevRunning);

        if (pti->flFlags & THRF_PMMSGQUEUE)
        {
            // create msg queue
            if ((pti->hab = WinInitialize(0)))
            {
                if ((pti->hmq = WinCreateMsgQueue(pti->hab, 4000)))
                {
                    // run thread func
                    ((PTHREADFUNC)pti->pThreadFunc)(pti);

                    WinDestroyMsgQueue(pti->hmq);

                }
                WinTerminate(pti->hab);
            }
        }
        else
            // no msgqueue:
            ((PTHREADFUNC)pti->pThreadFunc)(pti);

        // thread func returns:
        pti->fExitComplete = TRUE;
        pti->tid = NULLHANDLE;

        if (pti->flFlags & THRF_WAIT)
            // "Wait" flag set: delete semaphore
            DosCloseEventSem(pti->hevRunning);

        if (pti->pfRunning)
            // clear "running" flag
            *(pti->pfRunning) = FALSE;
    }
}

/*
 *@@ thrCreate:
 *      this function fills a THREADINFO structure in *pti
 *      and starts a new thread using _beginthread.
 *
 *      You must pass the thread function in pfn, which will
 *      then be executed. The thread will be passed a pointer
 *      to the THREADINFO structure as its thread parameter.
 *      The ulData field in that structure is set to ulData
 *      here. Use whatever you like.
 *
 *      The thread function must be declared like this:
 *
 +          void _Optlink fntWhatever(PTHREADINFO ptiMyself)
 *
 *      You should manually specify _Optlink because if the
 *      function is prototyped somewhere, VAC will automatically
 *      modify the function's linkage, and you'll run into
 *      compiler warnings.
 *
 *      ptiMyself is then a pointer to the THREADINFO structure.
 *      ulData may be obtained like this:
 +          ULONG ulData = ((PTHREADINFO)ptiMyself)->ulData;
 *
 *      thrCreate does not call your thread func directly,
 *      but only through the thr_fntGeneric wrapper to
 *      provide additional functionality. As a consequence,
 *      in your own thread function, NEVER call _endthread
 *      explicitly, because this would skip the exit processing
 *      (cleanup) in thr_fntGeneric. Instead, just fall out of
 *      your thread function.
 *
 *      This function does NOT check whether a thread is
 *      already running in *pti. If it is, that information
 *      will be lost.
 *
 *      flFlags can be any combination of the following:
 *
 *      -- THRF_PMMSGQUEUE: creates a PM message queue on the
 *         thread. Your thread function will find the HAB and
 *         the HMQ in its THREADINFO. These are automatically
 *         destroyed when the thread terminates.
 *
 *      -- THRF_WAIT: if this is set, thrCreate does not
 *         return to the caller until your thread function
 *         has successfully started running. This is done by
 *         waiting on an event semaphore which is automatically
 *         posted by thr_fntGeneric. This is useful for the
 *         typical PM "Worker" thread where you need to disable
 *         menu items on thread 1 while the thread is running.
 *
 *@@changed V0.9.0 [umoeller]: default stack size raised for Watcom (thanks, R�diger Ihle)
 *@@changed V0.9.0 [umoeller]: _beginthread is now only called after all variables have been set (thanks, R�diger Ihle)
 *@@changed V0.9.2 (2000-03-04) [umoeller]: added stack size parameter
 *@@changed V0.9.2 (2000-03-06) [umoeller]: now using thr_fntGeneric; thrGoodbye is no longer needed
 *@@changed V0.9.3 (2000-04-29) [umoeller]: removed stack size param; added fCreateMsgQueue
 *@@changed V0.9.3 (2000-05-01) [umoeller]: added pbRunning and flFlags
 *@@changed V0.9.5 (2000-08-26) [umoeller]: now using PTHREADINFO
 */

BOOL thrCreate(PTHREADINFO pti,     // out: THREADINFO data
               PTHREADFUNC pfn,     // in: _Optlink thread function
               PBOOL pfRunning,     // out: variable set to TRUE while thread is running;
                                    // ptr can be NULL
               ULONG flFlags,       // in: THRF_* flags
               ULONG ulData)        // in: user data to be stored in THREADINFO
{
    BOOL            rc = FALSE;

    if (pti)
    {
        // we arrive here if *ppti was NULL or (*ppti->tid == NULLHANDLE),
        // i.e. the thread is not already running.
        // _beginthread is contained both in the VAC++ and EMX
        // C libraries with this syntax.
        memset(pti, 0, sizeof(THREADINFO));
        pti->cbStruct = sizeof(THREADINFO);
        pti->pThreadFunc = (PVOID)pfn;
        pti->pfRunning = pfRunning;
        pti->flFlags = flFlags;
        pti->ulData = ulData;

        rc = TRUE;

        if (flFlags & THRF_WAIT)
            // "Wait" flag set: create an event semaphore which
            // will be posted by thr_fntGeneric
            if (DosCreateEventSem(NULL,     // unnamed
                                  &pti->hevRunning,
                                  0,        // unshared
                                  FALSE)    // not posted (reset)
                    != NO_ERROR)
                rc = FALSE;

        pti->fExit = FALSE;

        if (rc)
        {
            pti->tid = _beginthread(        // moved, V0.9.0 (hint: R�diger Ihle)
                                    thr_fntGeneric, // changed V0.9.2 (2000-03-06) [umoeller]
                                    0,      // unused compatibility param
                                    3*96000, // plenty of stack
                                    pti);   // parameter passed to thread
            rc = (pti->tid != 0);

            if (rc)
                if (flFlags & THRF_WAIT)
                {
                    // "Wait" flag set: wait on event semaphore
                    // posted by thr_fntGeneric
                    DosWaitEventSem(pti->hevRunning,
                                    SEM_INDEFINITE_WAIT);
                }
        }
    }

    return (rc);
}

/*
 *@@ thrRunSync:
 *      runs the specified thread function synchronously.
 *
 *      This is a wrapper around thrCreate. However, this
 *      function does not return until the thread function
 *      finishes. This creates a modal message loop on the
 *      calling thread so that the PM message queue is not
 *      blocked while the thread is running. Naturally this
 *      implies that the calling thread has a message queue.
 *
 *      As a special requirement, your thread function (pfn)
 *      must post WM_USER to THREADINFO.hwndNotify just before
 *      exiting. The mp1 value of WM_USER will then be returned
 *      by this function.
 *
 *@@added V0.9.5 (2000-08-26) [umoeller]
 */

ULONG thrRunSync(HAB hab,               // in: anchor block of calling thread
                 PTHREADFUNC pfn,       // in: thread function
                 ULONG ulData)          // in: data for thread function
{
    ULONG ulrc = 0;
    QMSG qmsg;
    BOOL fQuit = FALSE;
    HWND hwndNotify = WinCreateWindow(HWND_OBJECT,
                                      WC_BUTTON,
                                      (PSZ)"",
                                      0,
                                      0,0,0,0,
                                      0,
                                      HWND_BOTTOM,
                                      0,
                                      0,
                                      NULL);
    THREADINFO  ti = {0};
    thrCreate(&ti,
              pfn,
              NULL,
              THRF_PMMSGQUEUE,
              ulData);
    ti.hwndNotify = hwndNotify;

    while (WinGetMsg(hab,
                     &qmsg, 0, 0, 0))
    {
        // current message for our object window?
        if (    (qmsg.hwnd == hwndNotify)
             && (qmsg.msg == WM_USER)
           )
        {
            fQuit = TRUE;
            ulrc = (ULONG)qmsg.mp1;
        }

        WinDispatchMsg(hab, &qmsg);
        if (fQuit)
            break;
    }

    // we must wait for the thread to finish, or
    // otherwise THREADINFO is deleted from the stack
    // before the thread exits... will crash!
    thrWait(&ti);

    WinDestroyWindow(hwndNotify);
    return (ulrc);
}

/*
 *@@ thrClose:
 *      this functions sets the "fExit" flag in
 *      THREADINFO to TRUE.
 *
 *      The thread should monitor this flag
 *      periodically and then terminate itself.
 */

BOOL thrClose(PTHREADINFO pti)
{
    if (pti)
    {
        pti->fExit = TRUE;
        return (TRUE);
    }
    return (FALSE);
}

/*
 *@@ thrWait:
 *      this function waits for a thread to end by calling
 *      DosWaitThread. Note that this blocks the calling
 *      thread, so only use this function when you're sure
 *      the thread will actually terminate.
 *
 *      Returns FALSE if the thread wasn't running or TRUE
 *      if it was and has terminated.
 *
 *@@changed V0.9.0 [umoeller]: now checking for whether pti->tid is still != 0
 */

BOOL thrWait(PTHREADINFO pti)
{
    if (pti)
        if (pti->tid)
        {
            DosWaitThread(&(pti->tid), DCWW_WAIT);
            pti->tid = NULLHANDLE;
            return (TRUE);
        }
    return (FALSE);
}

/*
 *@@ thrFree:
 *      this is a combination of thrClose and
 *      thrWait, i.e. this func does not return
 *      until the specified thread has ended.
 */

BOOL thrFree(PTHREADINFO pti)
{
    if (pti->tid)
    {
        thrClose(pti);
        thrWait(pti);
    }
    return (TRUE);
}

/*
 *@@ thrKill:
 *      just like thrFree, but the thread is
 *      brutally killed, using DosKillThread.
 *
 *      Note: DO NOT USE THIS. DosKillThread
 *      cannot clean up the C runtime. In the
 *      worst case, this hangs the system
 *      because the runtime hasn't released
 *      a semaphore or something like that.
 */

BOOL thrKill(PTHREADINFO pti)
{
    if (pti->tid)
    {
        DosResumeThread(pti->tid);
            // this returns an error if the thread
            // is not suspended, but otherwise the
            // system might hang
        DosKillThread(pti->tid);
    }
    return (TRUE);
}

/*
 *@@ thrQueryID:
 *      returns thread ID or NULLHANDLE if
 *      the specified thread is not or no
 *      longer running.
 */

TID thrQueryID(const THREADINFO* pti)
{
    if (pti)
        if (!(pti->fExitComplete))
            return (pti->tid);

    return (NULLHANDLE);
}

/*
 *@@ thrQueryPriority:
 *      returns the priority of the calling thread.
 *      The low byte of the low word is a hexadecimal value
 *      representing a rank (value 0 to 31) within a priority class.
 *      Class values, found in the high byte of the low word, are
 *      as follows:
 *      --  0x01  idle
 *      --  0x02  regular
 *      --  0x03  time-critical
 *      --  0x04  server
 *
 *      Note: This cannot retrieve the priority of a
 *      thread other than the one on which this function
 *      is running. Use prc16QueryThreadInfo for that.
 */

ULONG thrQueryPriority(VOID)
{
    PTIB    ptib;
    PPIB    ppib;
    if (DosGetInfoBlocks(&ptib, &ppib) == NO_ERROR)
        if (ptib)
            if (ptib->tib_ptib2)
                return (ptib->tib_ptib2->tib2_ulpri);
    return (0);
}

