
/*
 *@@sourcefile timer.c:
 *      XTimers, which can be used to avoid excessive PM
 *      timer usage.
 *
 *      The functions here allow you to share a single PM
 *      timer for many windows on the same thread. Basically,
 *      these start a "master PM timer", to whose WM_TIMER
 *      message your window procedure must respond by calling
 *      tmrTimerTick, which will "distribute" the timer tick
 *      to those XTimers which have elapsed.
 *
 *      So to use XTimers, do the following:
 *
 *      1.  Create a timer set with tmrCreateSet. Specify
 *          an owner window and the timer ID of the master
 *          PM timer.
 *
 *      2.  You can then start and stop XTimers for windows
 *          on the same thread by calling tmrStartXTimer and
 *          tmrStopXTimer, respectively.
 *
 *      3.  In the window proc of the owner window, respond
 *          to WM_TIMER for the master PM timer by calling
 *          tmrTimerTick. This will call the window procs
 *          of those windows with WM_TIMER messages directly
 *          whose XTimers have elapsed.
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
 *      --  tmrTimerTick (which you must call when the
 *          "master" WM_TIMER comes in) does not post WM_TIMER
 *          messages to the windows specified in the subtimers,
 *          but calls the window procedure of the target window
 *          directly. This makes sure that timers work even if
 *          some thread is hogging the SIQ.
 *
 *          This however requires that all XTimers must run on
 *          the same thread as the owner window of the master
 *          timer which was specified with tmrCreateSet.
 *
 *      --  Queue timers (with HWND == NULLHANDLE) are not
 *          supported.
 *
 *      --  When a window is destroyed, its timers are not
 *          automatically cleaned up. tmrTimerTick does
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
 *      These are stored in a linked list in
 *      an _XTIMERSET.
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

HMTX                G_hmtxTimers = NULLHANDLE;  // timers lock mutex

/* ******************************************************************
 *
 *   Private functions
 *
 ********************************************************************/

/*
 *@@ LockTimers:
 *
 *@@added V0.9.12 (2001-05-12) [umoeller]
 */

BOOL LockTimers(VOID)
{
    if (!G_hmtxTimers)
        return (!DosCreateMutexSem(NULL,
                                   &G_hmtxTimers,
                                   0,
                                   TRUE));      // request!
    else
        return (!WinRequestMutexSem(G_hmtxTimers, SEM_INDEFINITE_WAIT));
}

/*
 *@@ UnlockTimers:
 *
 *@@added V0.9.12 (2001-05-12) [umoeller]
 */

VOID UnlockTimers(VOID)
{
    DosReleaseMutexSem(G_hmtxTimers);
}

/*
 *@@ FindTimer:
 *      returns the XTIMER structure from the global
 *      linked list which matches the given window
 *      _and_ timer ID.
 *
 *      Internal function. Caller must hold the mutex.
 */

PXTIMER FindTimer(PXTIMERSET pSet,          // in: timer set (from tmrCreateSet)
                  HWND hwnd,                // in: timer target window
                  USHORT usTimerID)         // in: timer ID
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
 *      Internal function. Caller must hold the mutex.
 */

VOID RemoveTimer(PXTIMERSET pSet,       // in: timer set (from tmrCreateSet)
                 PXTIMER pTimer)        // in: timer to remove.
{
    if (pSet && pSet->pvllXTimers)
    {
        PLINKLIST pllXTimers = (PLINKLIST)pSet->pvllXTimers;
        lstRemoveItem(pllXTimers,
                      pTimer);       // auto-free!
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
 *      hwndOwner must specify the "owner window", the target
 *      window for the master PM timer. This window must respond
 *      to a WM_TIMER message with the specified usPMTimerID and
 *      invoke tmrTimerTick then.
 *
 *      Note that the master timer is not started until an XTimer
 *      is started.
 *
 *      Use tmrDestroySet to free all resources again.
 *
 *@@added V0.9.9 (2001-02-28) [umoeller]
 */

PXTIMERSET tmrCreateSet(HWND hwndOwner,         // in: owner window
                        USHORT usPMTimerID)
{
    PXTIMERSET pSet = NULL;

    pSet = (PXTIMERSET)malloc(sizeof(*pSet));
    if (pSet)
    {
        pSet->hab = WinQueryAnchorBlock(hwndOwner);
        pSet->hwndOwner = hwndOwner;
        pSet->idPMTimer = usPMTimerID;
        pSet->idPMTimerRunning = 0;
        pSet->ulPMTimeout = 0;

        pSet->pvllXTimers = (PVOID)lstCreate(TRUE);
    }

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
 *@@changed V0.9.12 (2001-05-12) [umoeller]: added mutex protection
 */

VOID tmrDestroySet(PXTIMERSET pSet)     // in: timer set (from tmrCreateSet)
{
    if (pSet)
    {
        if (pSet->pvllXTimers)
        {
            if (LockTimers())
            {
                PLINKLIST pllXTimers = (PLINKLIST)pSet->pvllXTimers;

                PLISTNODE pTimerNode;

                while (pTimerNode = lstQueryFirstNode(pllXTimers))
                {
                    PXTIMER pTimer = (PXTIMER)pTimerNode->pItemData;
                    RemoveTimer(pSet, pTimer);
                }

                lstFree(pllXTimers);

                UnlockTimers();
            }
        }

        if (pSet->idPMTimerRunning)
            WinStopTimer(pSet->hab,
                         pSet->hwndOwner,
                         pSet->idPMTimer);

        free(pSet);
    }
}

/*
 *@@ AdjustPMTimer:
 *      goes thru all XTimers in the sets and starts
 *      or stops the PM timer with a decent frequency.
 *
 *      Internal function. Caller must hold the mutex.
 *
 *@@added V0.9.9 (2001-03-07) [umoeller]
 */

VOID AdjustPMTimer(PXTIMERSET pSet)
{
    PLINKLIST pllXTimers = (PLINKLIST)pSet->pvllXTimers;
    PLISTNODE pNode = lstQueryFirstNode(pllXTimers);
    if (!pNode)
    {
        // no XTimers running:
        if (pSet->idPMTimerRunning)
        {
            // but PM timer running:
            // stop it
            WinStopTimer(pSet->hab,
                         pSet->hwndOwner,
                         pSet->idPMTimer);
            pSet->idPMTimerRunning = 0;
        }

        pSet->ulPMTimeout = 0;
    }
    else
    {
        // we have timers:
        ULONG ulOldPMTimeout = pSet->ulPMTimeout;
        pSet->ulPMTimeout = 1000;

        while (pNode)
        {
            PXTIMER pTimer = (PXTIMER)pNode->pItemData;

            if ( (pTimer->ulTimeout / 2) < pSet->ulPMTimeout )
                pSet->ulPMTimeout = pTimer->ulTimeout / 2;

            pNode = pNode->pNext;
        }

        if (    (pSet->idPMTimerRunning == 0)       // timer not running?
             || (pSet->ulPMTimeout != ulOldPMTimeout) // timeout changed?
           )
            // start or restart PM timer
            pSet->idPMTimerRunning = WinStartTimer(pSet->hab,
                                                   pSet->hwndOwner,
                                                   pSet->idPMTimer,
                                                   pSet->ulPMTimeout);
    }
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
 *@@changed V0.9.12 (2001-05-12) [umoeller]: added mutex protection
 */

VOID tmrTimerTick(PXTIMERSET pSet)      // in: timer set (from tmrCreateSet)
{
    if (pSet && pSet->pvllXTimers)
    {
        if (LockTimers())
        {
            PLINKLIST pllXTimers = (PLINKLIST)pSet->pvllXTimers;
            // go thru all XTimers and see which one
            // has elapsed; for all of these, post WM_TIMER
            // to the target window proc
            PLISTNODE pTimerNode = lstQueryFirstNode(pllXTimers);

            if (!pTimerNode)
            {
                // no timers left:
                if (pSet->idPMTimerRunning)
                {
                    // but PM timer running:
                    // stop it
                    WinStopTimer(pSet->hab,
                                 pSet->hwndOwner,
                                 pSet->idPMTimer);
                    pSet->idPMTimerRunning = 0;
                }

                pSet->ulPMTimeout = 0;
            }
            else
            {
                // we have timers:
                BOOL    fFoundInvalid = FALSE;

                ULONG   // ulInterval = 100,
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

                    AdjustPMTimer(pSet);
                }
            } // end else if (!pTimerNode)

            UnlockTimers();
        } // end if (LockTimers())
    } // end if (pSet && pSet->pvllXTimers)
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
 *@@changed V0.9.12 (2001-05-12) [umoeller]: added mutex protection
 */

USHORT XWPENTRY tmrStartXTimer(PXTIMERSET pSet, // in: timer set (from tmrCreateSet)
                               HWND hwnd,       // in: target window for XTimer
                               USHORT usTimerID, // in: timer ID for XTimer's WM_TIMER
                               ULONG ulTimeout) // in: XTimer's timeout
{
    USHORT  usrc = 0;

    // _Pmpf((__FUNCTION__ ": entering"));

    if (pSet && pSet->pvllXTimers)
    {
        if (LockTimers())
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
                    }
                }

                if (usrc)
                {
                    // timer created or reset:
                    AdjustPMTimer(pSet);
                }
            } // if ((hwnd) && (ulTimeout))

            UnlockTimers();
        }
    }

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
 *
 *@@changed V0.9.12 (2001-05-12) [umoeller]: added mutex protection
 */

BOOL XWPENTRY tmrStopXTimer(PXTIMERSET pSet,    // in: timer set (from tmrCreateSet)
                            HWND hwnd,
                            USHORT usTimerID)
{
    BOOL brc = FALSE;
    if (pSet && pSet->pvllXTimers)
    {
        if (LockTimers())
        {
            PLINKLIST pllXTimers = (PLINKLIST)pSet->pvllXTimers;
            BOOL fLocked = FALSE;

            PXTIMER pTimer = FindTimer(pSet,
                                       hwnd,
                                       usTimerID);
            if (pTimer)
            {
                RemoveTimer(pSet, pTimer);
                // recalculate
                AdjustPMTimer(pSet);
                brc = TRUE;
            }

            UnlockTimers();
        }
    }

    return (brc);
}


