
/*
 *@@sourcefile timer.c:
 *      XTimers, which can replace the PM timers.
 *
 *      These timers operate similar to PM timers started
 *      with WinStartTimer. These are implemented thru a
 *      separate thread (fntTimersThread) which posts
 *      WM_TIMER messages regularly.
 *
 *      Instead of WinStartTimer, use tmrStartTimer.
 *      Instead of WinStopTimer, use tmrStopTimer.
 *
 *      The main advantage of the XTimers is that these
 *      are not a limited resource (as opposed to PM timers).
 *      I don't know how many PM timers exist on the system,
 *      but PMREF says that the no. of remaining timers can
 *      be queried with WinQuerySysValue(SV_CTIMERS).
 *
 *      There are a few limitations with the XTimers though:
 *
 *      --  If you start a timer with a timeout < 100 ms,
 *          the first WM_TIMER might not appear before
 *          100 ms have elapsed. This may or be not the
 *          case, depending on whether other timers are
 *          running.
 *
 *      --  XTimers post WM_TIMER messages regardless of
 *          whether previous WM_TIMER messages have already
 *          been processed. For this reason, be careful with
 *          small timer timeouts, this might flood the
 *          message queue.
 *
 *      --  Queue timers (with HWND == NULLHANDLE) are not
 *          supported.
 *
 *      --  When a window is deleted, its timers are not
 *          automatically cleaned up. To be on the safe
 *          side, always call tmrStopAllTimers when
 *          WM_DESTROY comes into a window which has used
 *          timers.
 *
 *      Function prefixes:
 *      --  tmr*   timer functions
 *
 *@@header "helpers\timer.h"
 *@@added V0.9.7 (2000-12-04) [umoeller]
 */

/*
 *      Copyright (C) 2000 Ulrich M”ller.
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

// OS2 includes

#define OS2EMX_PLAIN_CHAR
    // this is needed for "os2emx.h"; if this is defined,
    // emx will define PSZ as _signed_ char, otherwise
    // as unsigned char

#define INCL_DOSEXCEPTIONS
#define INCL_DOSPROCESS
#define INCL_DOSSEMAPHORES
#define INCL_DOSERRORS
#include <os2.h>

#include <stdio.h>
#include <setjmp.h>

#include "setup.h"                      // code generation and debugging options

#include "helpers\datetime.h"
#include "helpers\except.h"
#include "helpers\linklist.h"
#include "helpers\threads.h"
#include "helpers\timer.h"

/*
 *@@category: Helpers\PM helpers\Timer replacements
 */

/* ******************************************************************
 *
 *   Private declarations
 *
 ********************************************************************/

/*
 *@@ XTIMER:
 *      one of these represents an XTimer.
 *      These are stored in a global linked list.
 */

typedef struct _XTIMER
{
    USHORT     usTimerID;           // timer ID, as passed to tmrStartTimer
    HWND       hwndTarget;          // target window, as passed to tmrStartTimer
    ULONG      ulTimeout;           // timer's timeout (in ms)
    ULONG      ulNextFire;          // next time scalar (from dtGetULongTime) to fire at
} XTIMER, *PXTIMER;

/* ******************************************************************
 *
 *   Global variables
 *
 ********************************************************************/

// timers thread
HMTX                G_hmtxTimers = NULLHANDLE;  // timers lock mutex
THREADINFO          G_tiTimers = {0};           // timers thread (only running
                                                // if any timers were requested)
BOOL                G_fTimersThreadRunning = FALSE;
LINKLIST            G_llTimers;         // linked list of XTIMER pointers

/* ******************************************************************
 *
 *   Timer helpers
 *
 ********************************************************************/

/*
 *@@ LockTimers:
 *      locks the global timer resources.
 *      You MUST call UnlockTimers after this.
 */

BOOL LockTimers(VOID)
{
    BOOL brc = FALSE;
    if (G_hmtxTimers == NULLHANDLE)
    {
        // this initializes all globals
        lstInit(&G_llTimers,
                TRUE);     // auto-free

        brc = (DosCreateMutexSem(NULL,  // unnamed
                                 &G_hmtxTimers,
                                 0,     // unshared
                                 TRUE)  // lock!
                        == NO_ERROR);
    }
    else
        brc = (DosRequestMutexSem(G_hmtxTimers, SEM_INDEFINITE_WAIT)
                    == NO_ERROR);
    return (brc);
}

/*
 *@@ UnlockTimers:
 *      the reverse to LockTimers.
 */

VOID UnlockTimers(VOID)
{
    DosReleaseMutexSem(G_hmtxTimers);
}

/*
 *@@ tmrOnKill:
 *      on-kill proc for exception handlers.
 *
 *@@added V0.9.7 (2000-12-09) [umoeller]
 */

VOID APIENTRY tmrOnKill(PEXCEPTIONREGISTRATIONRECORD2 pRegRec2)
{
    DosBeep(500, 500);
    UnlockTimers();
}

/*
 *@@ fntTimersThread:
 *      the actual thread which fires the timers by
 *      posting WM_TIMER messages to the respecive
 *      target windows when a timer has elapsed.
 *
 *      This thread is dynamically started when the
 *      first timer is started thru tmrStartTimer.
 *      It is automatically stopped (to be precise:
 *      it terminates itself) when the last timer
 *      is stopped thru tmrStopTimer, which then
 *      sets the thread's fExit flag to TRUE.
 */

void _Optlink fntTimersThread(PTHREADINFO ptiMyself)
{
    ULONG       ulInterval = 25;
    HAB         hab = WinInitialize(0);
    BOOL        fLocked = FALSE;

    // linked list of timers found to be invalid;
    // this holds LISTNODE pointers from the global
    // list to be removed
    LINKLIST    llInvalidTimers;
    lstInit(&llInvalidTimers,
            FALSE);     // no auto-free

    #ifdef __DEBUG__
        DosBeep(3000, 30);
    #endif

    // keep running while we have timers
    while (!ptiMyself->fExit)
    {
        ULONG ulNesting = 0;

        DosSleep(ulInterval);

        // minimum interval: 100 ms; this is lowered
        // if we find any timers in the list which
        // have a lower timeout to make sure we can
        // fire at a lower interval...
        ulInterval = 100;

        TRY_LOUD(excpt1)
        {
            DosEnterMustComplete(&ulNesting);

            fLocked = LockTimers();
            if (fLocked)
            {
                // go thru all XTimers and see which one
                // has elapsed; for all of these, post WM_TIMER
                // to the target window proc
                PLISTNODE pTimerNode = lstQueryFirstNode(&G_llTimers);
                if (!pTimerNode)
                    // no more timers left:
                    // terminate thread
                    ptiMyself->fExit = TRUE;
                else
                {
                    // we have timers:
                    BOOL      fFoundInvalid = FALSE;
                    while (pTimerNode)
                    {
                        PXTIMER pTimer = (PXTIMER)pTimerNode->pItemData;
                        ULONG ulTimeNow = dtGetULongTime();

                        if (pTimer->ulNextFire < ulTimeNow)
                        {
                            // this timer has elapsed:
                            // fire!
                            if (WinIsWindow(hab,
                                            pTimer->hwndTarget))
                            {
                                // window still valid:
                                WinPostMsg(pTimer->hwndTarget,
                                           WM_TIMER,
                                           (MPARAM)pTimer->usTimerID,
                                           0);
                                pTimer->ulNextFire = ulTimeNow + pTimer->ulTimeout;
                            }
                            else
                            {
                                // window has been destroyed:
                                #ifdef __DEBUG__
                                    DosBeep(100, 100);
                                #endif
                                lstAppendItem(&llInvalidTimers,
                                              (PVOID)pTimerNode);
                                fFoundInvalid = TRUE;
                            }
                        } // end if (pTimer->ulNextFire < ulTimeNow)

                        // adjust DosSleep interval
                        if (pTimer->ulTimeout < ulInterval)
                            ulInterval = pTimer->ulTimeout;

                        // next timer
                        pTimerNode = pTimerNode->pNext;
                    } // end while (pTimerNode)

                    // destroy invalid timers, if any
                    if (fFoundInvalid)
                    {
                        PLISTNODE pNodeNode = lstQueryFirstNode(&llInvalidTimers);
                        while (pNodeNode)
                        {
                            PLISTNODE pNode2Remove = (PLISTNODE)pNodeNode->pItemData;
                            lstRemoveNode(&G_llTimers,
                                          pNode2Remove);
                            pNodeNode = pNodeNode->pNext;
                        }
                        lstClear(&llInvalidTimers);
                    }
                } // end else if (!pTimerNode)
            } // end if (fLocked)
        }
        CATCH(excpt1) { } END_CATCH();

        if (fLocked)
        {
            UnlockTimers();
            fLocked = FALSE;
        }

        DosExitMustComplete(&ulNesting);

    } // end while (!ptiMyself->fExit)

    WinTerminate(hab);

    #ifdef __DEBUG__
        DosBeep(1500, 30);
    #endif
}

/*
 *@@ FindTimer:
 *      returns the XTIMER structure from the global
 *      linked list which matches the given window
 *      _and_ timer ID.
 *
 *      Internal function.
 *
 *      Preconditions:
 *
 *      -- The caller must call LockTimers() first.
 */

PXTIMER FindTimer(HWND hwnd,
                  USHORT usTimerID)
{
    PLISTNODE pNode = lstQueryFirstNode(&G_llTimers);
    while (pNode)
    {
        PXTIMER pTimer = (PXTIMER)pNode->pItemData;
        if (    (pTimer->usTimerID == usTimerID)
             && (pTimer->hwndTarget == hwnd)
           )
        {
            return (pTimer);
        }

        pNode = pNode->pNext;
    }
    return (NULL);
}

/*
 *@@ RemoveTimer:
 *      removes the specified XTIMER structure from
 *      the global linked list of running timers.
 *
 *      Internal function.
 *
 *      Preconditions:
 *
 *      -- The caller must call LockTimers() first.
 *
 *@@added V0.9.7 (2000-12-04) [umoeller]
 */

VOID RemoveTimer(PXTIMER pTimer)
{
    lstRemoveItem(&G_llTimers,
                  pTimer);       // auto-free!
    /* if (lstCountItems(&G_llTimers) == 0)
        // no more timers left:
        // stop the main timer
        thrClose(&G_tiTimers); */
}

/*
 *@@ tmrStartTimer:
 *      starts an XTimer.
 *
 *      Any window can request an XTimer using
 *      this function. This operates similar to
 *      WinStartTimer, except that the number of
 *      XTimers is not limited.
 *
 *      Returns a new timer or resets an existing
 *      timer (if usTimerID is already used with
 *      hwnd). Use tmrStopTimer to stop the timer.
 *
 *      The timer is _not_ stopped automatically
 *      when the widget is destroyed.
 *
 *@@added V0.9.7 (2000-12-04) [umoeller]
 */

USHORT APIENTRY tmrStartTimer(HWND hwnd,
                              USHORT usTimerID,
                              ULONG ulTimeout)
{
    USHORT  usrc = 0;
    BOOL    fLocked = FALSE;

    if ((hwnd) && (ulTimeout))
    {
        ULONG ulNesting = 0;
        DosEnterMustComplete(&ulNesting);

        TRY_LOUD(excpt1)
        {
            fLocked = LockTimers();
            if (fLocked)
            {
                PXTIMER pTimer;

                // if the timers thread is not yet running,
                // start it now (i.e. this is the first timer)
                if (!G_fTimersThreadRunning)
                {
                    // main timer not yet running:
                    thrCreate(&G_tiTimers,
                              fntTimersThread,
                              &G_fTimersThreadRunning,
                              THRF_WAIT,        // no msgq, but wait
                              0);
                    // raise priority
                    DosSetPriority(PRTYS_THREAD,
                                   PRTYC_REGULAR,  // 3
                                   PRTYD_MAXIMUM,
                                   G_tiTimers.tid);
                }

                // check if this timer exists already
                pTimer = FindTimer(hwnd,
                                   usTimerID);
                if (pTimer)
                {
                    // exists already: reset only
                    pTimer->ulNextFire = dtGetULongTime() + ulTimeout;
                    usrc = pTimer->usTimerID;
                }
                else
                {
                    // new timer needed:
                    pTimer = (PXTIMER)malloc(sizeof(XTIMER));
                    if (pTimer)
                    {
                        pTimer->usTimerID = usTimerID;
                        pTimer->hwndTarget = hwnd;
                        pTimer->ulTimeout = ulTimeout;
                        pTimer->ulNextFire = dtGetULongTime() + ulTimeout;

                        lstAppendItem(&G_llTimers,
                                      pTimer);
                        usrc = pTimer->usTimerID;
                    }
                }
            }
        }
        CATCH(excpt1) { } END_CATCH();

        // unlock the sems outside the exception handler
        if (fLocked)
        {
            UnlockTimers();
            fLocked = FALSE;
        }

        DosExitMustComplete(&ulNesting);
    } // if ((hwnd) && (ulTimeout))

    return (usrc);
}

/*
 *@@ tmrStopTimer:
 *      similar to WinStopTimer, this stops the
 *      specified timer (which must have been
 *      started with the same hwnd and usTimerID
 *      using tmrStartTimer).
 *
 *      Returns TRUE if the timer was stopped.
 *
 *@@added V0.9.7 (2000-12-04) [umoeller]
 */

BOOL APIENTRY tmrStopTimer(HWND hwnd,
                           USHORT usTimerID)
{
    BOOL brc = FALSE;
    BOOL fLocked = FALSE;

    ULONG ulNesting = 0;
    DosEnterMustComplete(&ulNesting);

    TRY_LOUD(excpt1)
    {
        fLocked = LockTimers();
        if (fLocked)
        {
            PXTIMER pTimer = FindTimer(hwnd,
                                       usTimerID);
            if (pTimer)
            {
                RemoveTimer(pTimer);
                brc = TRUE;
            }
        }
    }
    CATCH(excpt1) { } END_CATCH();

    // unlock the sems outside the exception handler
    if (fLocked)
    {
        UnlockTimers();
        fLocked = FALSE;
    }

    DosExitMustComplete(&ulNesting);

    return (brc);
}

/*
 *@@ tmrStopAllTimers:
 *      stops all timers which are running for the
 *      specified window. This is a useful helper
 *      that you should call during WM_DESTROY of
 *      a window that has started timers.
 *
 *@@added V0.9.7 (2000-12-04) [umoeller]
 */

VOID tmrStopAllTimers(HWND hwnd)
{
    BOOL fLocked = FALSE;

    ULONG ulNesting = 0;
    DosEnterMustComplete(&ulNesting);

    TRY_LOUD(excpt1)
    {
        fLocked = LockTimers();
        if (fLocked)
        {
            PLISTNODE pTimerNode = lstQueryFirstNode(&G_llTimers);
            while (pTimerNode)
            {
                PXTIMER pTimer = (PXTIMER)pTimerNode->pItemData;
                if (pTimer->hwndTarget == hwnd)
                {
                    RemoveTimer(pTimer);
                    // start over
                    pTimerNode = lstQueryFirstNode(&G_llTimers);
                }
                else
                    pTimerNode = pTimerNode->pNext;
            }
        }
    }
    CATCH(excpt1) { } END_CATCH();

    // unlock the sems outside the exception handler
    if (fLocked)
    {
        UnlockTimers();
        fLocked = FALSE;
    }

    DosExitMustComplete(&ulNesting);
}


