
/*
 *@@sourcefile timer.c:
 *      XTimers, which can be used to avoid excessive PM
 *      timer usage.
 *
 *      These timers operate similar to PM timers started
 *      with WinStartTimer. These are implemented thru a
 *      separate thread (fntTimersThread) which posts
 *      WM_TIMER messages regularly.
 *
 *      Instead of WinStartTimer, use tmrStartXTimer.
 *      Instead of WinStopTimer, use tmrStopXTimer.
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
 *          automatically cleaned up. The timer thread does
 *          detect invalid windows and removes them from the
 *          timers list before posting, but to be on the safe
 *          side, always call tmrStopAllTimers when WM_DESTROY
 *          comes into a window which has used timers.
 *
 *      Function prefixes:
 *      --  tmr*   timer functions
 *
 *@@header "helpers\timer.h"
 *@@added V0.9.7 (2000-12-04) [umoeller]
 */

/*
 *      Copyright (C) 2000 Ulrich M�ller.
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
#define INCL_DOSMISC
#define INCL_DOSERRORS

#define INCL_WINWINDOWMGR
#define INCL_WINMESSAGEMGR
#define INCL_WINTIMER
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
 *      see timer.c.
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
    USHORT     usTimerID;           // timer ID, as passed to tmrStartXTimer
    HWND       hwndTarget;          // target window, as passed to tmrStartXTimer
    ULONG      ulTimeout;           // timer's timeout (in ms)
    ULONG      ulNextFire;          // next time scalar (from dtGetULongTime) to fire at
} XTIMER, *PXTIMER;

/* ******************************************************************
 *
 *   Global variables
 *
 ********************************************************************/

// timers thread
// HMTX                G_hmtxTimers = NULLHANDLE;  // timers lock mutex
// THREADINFO          G_tiTimers = {0};           // timers thread (only running
                                                // if any timers were requested)
// BOOL                G_fTimersThreadRunning = FALSE;
// LINKLIST            G_llTimers;         // linked list of XTIMER pointers

/*
 *@@ fntTimersThread:
 *      the actual thread which fires the timers by
 *      posting WM_TIMER messages to the respecive
 *      target windows when a timer has elapsed.
 *
 *      This thread is dynamically started when the
 *      first timer is started thru tmrStartXTimer.
 *      It is automatically stopped (to be precise:
 *      it terminates itself) when the last timer
 *      is stopped thru tmrStopXTimer, which then
 *      sets the thread's fExit flag to TRUE.
 *
 *@@changed V0.9.7 (2000-12-08) [umoeller]: got rid of dtGetULongTime
 */

/* void _Optlink fntTimersThread(PTHREADINFO ptiMyself)
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
        // ULONG ulNesting = 0;

        ULONG ulTimeNow;

        DosSleep(ulInterval);

        // minimum interval: 100 ms; this is lowered
        // if we find any timers in the list which
        // have a lower timeout to make sure we can
        // fire at a lower interval...
        ulInterval = 100;

        // DosEnterMustComplete(&ulNesting);

        TRY_LOUD(excpt1)
        {
            fLocked = LockTimers();
            if (fLocked)
            {
            } // end if (fLocked)
        }
        CATCH(excpt1) { } END_CATCH();

        if (fLocked)
        {
            UnlockTimers();
            fLocked = FALSE;
        }

        // DosExitMustComplete(&ulNesting);

    } // end while (!ptiMyself->fExit)

    WinTerminate(hab);

    #ifdef __DEBUG__
        DosBeep(1500, 30);
    #endif
} */

/* ******************************************************************
 *
 *   Private functions
 *
 ********************************************************************/

/*
 *@@ FindTimer:
 *      returns the XTIMER structure from the global
 *      linked list which matches the given window
 *      _and_ timer ID.
 *
 *      Internal function.
 */

PXTIMER FindTimer(PXTIMERSET pSet,
                  HWND hwnd,
                  USHORT usTimerID)
{
    if (pSet && pSet->pvllXTimers)
    {
        PLINKLIST pllXTimers = (PLINKLIST)pSet->pvllXTimers;
        PLISTNODE pNode = lstQueryFirstNode(pllXTimers);
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
 */

VOID RemoveTimer(PXTIMERSET pSet,
                 PXTIMER pTimer)
{
    if (pSet && pSet->pvllXTimers)
    {
        PLINKLIST pllXTimers = (PLINKLIST)pSet->pvllXTimers;
        lstRemoveItem(pllXTimers,
                      pTimer);       // auto-free!
        /* if (lstCountItems(&G_llTimers) == 0)
            // no more timers left:
            // stop the main timer
            thrClose(&G_tiTimers); */
    }
}

/* ******************************************************************
 *
 *   Exported functions
 *
 ********************************************************************/

/*
 *@@ tmrCreateSet:
 *      creates a "timer set" for use with the XTimer functions.
 *      This is the first step if you want to use the XTimers.
 *
 *      Use tmrDestroySet to free all resources again.
 *
 *@@added V0.9.9 (2001-02-28) [umoeller]
 */

PXTIMERSET tmrCreateSet(HWND hwndOwner,
                        USHORT usPMTimerID)
{
    PXTIMERSET pSet = NULL;

    // _Pmpf((__FUNCTION__ ": entering"));

    pSet = (PXTIMERSET)malloc(sizeof(*pSet));
    if (pSet)
    {
        pSet->hab = WinQueryAnchorBlock(hwndOwner);
        pSet->hwndOwner = hwndOwner;
        pSet->idPMTimer = usPMTimerID;
        pSet->idPMTimerRunning = 0;

        pSet->pvllXTimers = (PVOID)lstCreate(TRUE);
    }

    // _Pmpf((__FUNCTION__ ": leaving, returning 0x%lX", pSet));

    return (pSet);
}

/*
 *@@ tmrDestroySet:
 *      destroys a timer set previously created using
 *      tmrCreateSet.
 *
 *      Of course, this will stop all XTimers which
 *      might still be running with this set.
 *
 *@@added V0.9.9 (2001-02-28) [umoeller]
 */

VOID tmrDestroySet(PXTIMERSET pSet)
{
    // _Pmpf((__FUNCTION__ ": entering"));

    if (pSet)
    {
        if (pSet->pvllXTimers)
        {
            PLINKLIST pllXTimers = (PLINKLIST)pSet->pvllXTimers;

            PLISTNODE pTimerNode;

            while (pTimerNode = lstQueryFirstNode(pllXTimers))
            {
                PXTIMER pTimer = (PXTIMER)pTimerNode->pItemData;
                RemoveTimer(pSet, pTimer);
            }

            lstFree(pllXTimers);
        }

        free(pSet);
    }

    // _Pmpf((__FUNCTION__ ": leaving"));
}

/*
 *@@ tmrTimerTick:
 *      implements a PM timer tick.
 *
 *      When your window procs receives WM_TIMER for the
 *      one PM timer which is supposed to trigger all the
 *      XTimers, it must call this function. This will
 *      evaluate all XTimers on the list and "fire" them
 *      by calling the window procs directly with the
 *      WM_TIMER message.
 *
 *@@added V0.9.9 (2001-02-28) [umoeller]
 */

VOID tmrTimerTick(PXTIMERSET pSet)
{
    // _Pmpf((__FUNCTION__ ": entering"));

    if (pSet && pSet->pvllXTimers)
    {
        PLINKLIST pllXTimers = (PLINKLIST)pSet->pvllXTimers;
        // go thru all XTimers and see which one
        // has elapsed; for all of these, post WM_TIMER
        // to the target window proc
        PLISTNODE pTimerNode = lstQueryFirstNode(pllXTimers);

        // stop the PM timer for now; we'll restart it later
        WinStopTimer(pSet->hab,
                     pSet->hwndOwner,
                     pSet->idPMTimer);
        pSet->idPMTimerRunning = 0;

        if (pTimerNode)
        {
            // we have timers:
            BOOL    fFoundInvalid = FALSE;

            ULONG   ulInterval = 100,
                    ulTimeNow = 0;

            // linked list of timers found to be invalid;
            // this holds LISTNODE pointers from the global
            // list to be removed
            LINKLIST    llInvalidTimers;
            lstInit(&llInvalidTimers,
                    FALSE);     // no auto-free

            // get current time
            DosQuerySysInfo(QSV_MS_COUNT, QSV_MS_COUNT,
                            &ulTimeNow, sizeof(ulTimeNow));

            while (pTimerNode)
            {
                PXTIMER pTimer = (PXTIMER)pTimerNode->pItemData;

                if (pTimer->ulNextFire < ulTimeNow)
                {
                    // this timer has elapsed:
                    // fire!
                    if (WinIsWindow(pSet->hab,
                                    pTimer->hwndTarget))
                    {
                        // window still valid:
                        // get the window's window proc
                        PFNWP pfnwp = (PFNWP)WinQueryWindowPtr(pTimer->hwndTarget,
                                                               QWP_PFNWP);
                        // call the window proc DIRECTLY
                        pfnwp(pTimer->hwndTarget,
                              WM_TIMER,
                              (MPARAM)pTimer->usTimerID,
                              0);
                        pTimer->ulNextFire = ulTimeNow + pTimer->ulTimeout;
                    }
                    else
                    {
                        // window has been destroyed:
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
                    lstRemoveNode(pllXTimers,
                                  pNode2Remove);
                    pNodeNode = pNodeNode->pNext;
                }
                lstClear(&llInvalidTimers);
            }

            if (lstCountItems(pllXTimers))
            {
                // any timers left:
                // restart timer with the timeout calculated above
                pSet->idPMTimerRunning = WinStartTimer(pSet->hab,
                                                       pSet->hwndOwner,
                                                       pSet->idPMTimer,
                                                       ulInterval);
                /* _Pmpf(("  %d timers left, restarted PM timer == %d, interval %d",
                            lstCountItems(pllXTimers),
                            pSet->idPMTimerRunning,
                            ulInterval));
                _Pmpf(("  pSet->hab: 0x%lX, hwndOwner = 0x%lX, usPMTimerID = %d",
                        pSet->hab,
                        pSet->hwndOwner,
                        pSet->idPMTimer)); */
            }
        } // end else if (!pTimerNode)
    }

    // _Pmpf((__FUNCTION__ ": leaving"));
}

/*
 *@@ tmrStartXTimer:
 *      starts an XTimer.
 *
 *      Any window can request an XTimer using
 *      this function. This operates similar to
 *      WinStartTimer, except that the number of
 *      XTimers is not limited.
 *
 *      Returns a new timer or resets an existing
 *      timer (if usTimerID is already used with
 *      hwnd). Use tmrStopXTimer to stop the timer.
 *
 *      The timer is _not_ stopped automatically
 *      when the window is destroyed.
 *
 *@@changed V0.9.7 (2000-12-08) [umoeller]: got rid of dtGetULongTime
 */

USHORT XWPENTRY tmrStartXTimer(PXTIMERSET pSet,
                               HWND hwnd,
                               USHORT usTimerID,
                               ULONG ulTimeout)
{
    USHORT  usrc = 0;

    // _Pmpf((__FUNCTION__ ": entering"));

    if (pSet && pSet->pvllXTimers)
    {
        PLINKLIST pllXTimers = (PLINKLIST)pSet->pvllXTimers;

        if ((hwnd) && (ulTimeout))
        {
            PXTIMER pTimer;

            // check if this timer exists already
            pTimer = FindTimer(pSet,
                               hwnd,
                               usTimerID);
            if (pTimer)
            {
                // exists already: reset only
                ULONG ulTimeNow;
                DosQuerySysInfo(QSV_MS_COUNT, QSV_MS_COUNT,
                                &ulTimeNow, sizeof(ulTimeNow));
                pTimer->ulNextFire = ulTimeNow + ulTimeout;
                usrc = pTimer->usTimerID;
                // _Pmpf(("  timer existed, reset"));
            }
            else
            {
                // new timer needed:
                pTimer = (PXTIMER)malloc(sizeof(XTIMER));
                if (pTimer)
                {
                    ULONG ulTimeNow;
                    DosQuerySysInfo(QSV_MS_COUNT, QSV_MS_COUNT,
                                    &ulTimeNow, sizeof(ulTimeNow));
                    pTimer->usTimerID = usTimerID;
                    pTimer->hwndTarget = hwnd;
                    pTimer->ulTimeout = ulTimeout;
                    pTimer->ulNextFire = ulTimeNow + ulTimeout;

                    lstAppendItem(pllXTimers,
                                  pTimer);
                    usrc = pTimer->usTimerID;

                    // _Pmpf(("  new timer created"));
                }
            }

            if (usrc)
            {
                // timer created or reset:
                // start main PM timer
                pSet->idPMTimerRunning = WinStartTimer(pSet->hab,
                                                       pSet->hwndOwner,
                                                       pSet->idPMTimer,
                                                       100);
                // _Pmpf(("  started PM timer %d", pSet->idPMTimerRunning));
            }
        } // if ((hwnd) && (ulTimeout))
    }

    // _Pmpf((__FUNCTION__ ": leaving, returning %d", usrc));

    return (usrc);
}

/*
 *@@ tmrStopXTimer:
 *      similar to WinStopTimer, this stops the
 *      specified timer (which must have been
 *      started with the same hwnd and usTimerID
 *      using tmrStartXTimer).
 *
 *      Returns TRUE if the timer was stopped.
 */

BOOL XWPENTRY tmrStopXTimer(PXTIMERSET pSet,
                            HWND hwnd,
                            USHORT usTimerID)
{
    BOOL brc = FALSE;
    if (pSet && pSet->pvllXTimers)
    {
        PLINKLIST pllXTimers = (PLINKLIST)pSet->pvllXTimers;
        BOOL fLocked = FALSE;

        PXTIMER pTimer = FindTimer(pSet,
                                   hwnd,
                                   usTimerID);
        if (pTimer)
        {
            RemoveTimer(pSet, pTimer);
            brc = TRUE;
        }
    }

    return (brc);
}


