
/*
 *@@sourcefile procstat.c:
 *      functions for querying process information.
 *      This is an easy-to-use interface to the
 *      messy 16-bit DosQProcStatus function.
 *
 *      Usage: All OS/2 programs.
 *
 *      Function prefixes (new with V0.81):
 *      --  prc*   Query Process helper functions
 *
 *      Based on Kai Uwe Rommel's "dosqproc" package
 *      available at Hobbes:
 *      Kai Uwe Rommel - Wed 25-Mar-1992
 *                       Sat 13-Aug-1994
 *
 *      Note: If you link against procstat.obj, you
 *      need to import the following in your .DEF file:
 *
 +          IMPORTS
 +              DOSQPROCSTATUS = DOSCALLS.154
 +              DosQuerySysState = DOSCALLS.368
 *
 *      or linking will fail.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@header "helpers\procstat.h"
 */

/*
 *      Copyright (C) 1992-1994 Kai Uwe Rommel.
 *      Copyright (C) 1998-2000 Ulrich M”ller.
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
#define INCL_DOSERRORS
#include <os2.h>

#include <stdlib.h>      // already #include'd
#include <string.h>      // already #include'd
#include <stdio.h>

#include "setup.h"                      // code generation and debugging options

#include "helpers\procstat.h"

#pragma hdrstop

/*
 *@@category: Helpers\Control program helpers\Process status\16-bit DosQProcStat
 */

/********************************************************************
 *
 *   DosQProcStat (16-bit) interface
 *
 ********************************************************************/

/*
 *@@ prc16GetInfo:
 *      nifty interface to DosQProcStat (16-bit).
 *      This returns the head of a newly
 *      allocated buffer which has plenty
 *      of pointers for subsequent browsing.
 *
 *      Use prc16FreeInfo to free the buffer.
 *
 *@@added V0.9.3 (2000-05-05) [umoeller]
 */

PQPROCSTAT16 prc16GetInfo(APIRET *parc)     // out: error, ptr can be NULL
{
    APIRET arc = NO_ERROR;
    PQPROCSTAT16 pps = (PQPROCSTAT16)malloc(0x8000);
    if (!pps)
        arc = ERROR_NOT_ENOUGH_MEMORY;
    else
    {
        arc = DosQProcStatus(pps, 0x8000);
        if (arc != NO_ERROR)
            free(pps);
    }

    if (parc)
        *parc = arc;

    return (pps);
}

/*
 *@@ prc16FreeInfo:
 *      frees memory allocated by prc16GetInfo.
 *
 *@@added V0.9.3 (2000-05-05) [umoeller]
 */

VOID prc16FreeInfo(PQPROCSTAT16 pInfo)
{
    free(pInfo);
}

/*
 *@@ prc16FindProcessFromName:
 *      searches the specified buffer for a process
 *      with the specified name and returns a pointer
 *      to its data within pInfo.
 *
 *      Returns NULL if not found.
 *
 *@@added V0.9.3 (2000-05-05) [umoeller]
 */

PQPROCESS16 prc16FindProcessFromName(PQPROCSTAT16 pInfo,    // in: from prc16GetInfo
                                     const char *pcszName)  // in: e.g. "pmshell.exe"
{
    PQPROCESS16 pProcess,
                pReturn = NULL;
    for ( pProcess = (PQPROCESS16)PTR(pInfo->ulProcesses, 0);
          pProcess->ulType != 3;
          pProcess = (PQPROCESS16)PTR(pProcess->ulThreadList,
                                      pProcess->usThreads * sizeof(QTHREAD16))
        )
    {
        CHAR    szModuleName[CCHMAXPATH];
        if (DosQueryModuleName(pProcess->usHModule,
                               sizeof(szModuleName),
                               szModuleName)
                == NO_ERROR)
        {
            // the module name is fully qualified, so find the
            // file name (after the last backslash)
            PSZ pLastBackslash = strrchr(szModuleName, '\\');
            if (pLastBackslash)
                // found:
                if (stricmp(pLastBackslash + 1, pcszName) == 0)
                {
                    // matches:
                    pReturn = pProcess;
                    break;
                }
        }
    }

    return (pReturn);
}

/*
 *@@ prc16FindProcessFromPID:
 *      searches the specified buffer for a process
 *      with the specified PID and returns a pointer
 *      to its data within pInfo.
 *
 *      Returns NULL if not found.
 *
 *V0.9.5 (2000-09-29) [umoeller]
 */

PQPROCESS16 prc16FindProcessFromPID(PQPROCSTAT16 pInfo, // in: from prc16GetInfo
                                    ULONG ulPID)        // in: PID
{
    PQPROCESS16 pProcess,
                pReturn = NULL;
    for ( pProcess = (PQPROCESS16)PTR(pInfo->ulProcesses, 0);
          pProcess->ulType != 3;
          pProcess = (PQPROCESS16)PTR(pProcess->ulThreadList,
                                      pProcess->usThreads * sizeof(QTHREAD16))
        )
    {
        if (pProcess->usPID == ulPID)
        {
            pReturn = pProcess;
            break;
        }
    }

    return (pReturn);
}

/********************************************************************
 *
 *   DosQProcStat (16-bit) helpers
 *
 ********************************************************************/

/*
 * prcReport16:
 *      fill PRCPROCESS structure
 */

VOID prcReport16(PQPROCESS16 pProcess, PPRCPROCESS pprcp)
{
    // USHORT usPriority = 0;
    PQTHREAD16 pThread;
    int i;

    DosQueryModuleName(pProcess->usHModule,
                       sizeof(pprcp->szModuleName),
                       pprcp->szModuleName);
    // DosGetPrty(PRTYS_PROCESS, &(pprcp->usPriority), pProcess->usPID);

    // sum up CPU time for process
    for (pprcp->ulCPU = 0,
                i = 0,
                pThread = (PQTHREAD16)PTR(pProcess->ulThreadList, 0);
         i < pProcess->usThreads;
         i++, pThread++ )
    {
        pprcp->ulCPU += (pThread->ulSysTime + pThread->ulUserTime);
    }

    pprcp->usPID            = pProcess->usPID;
    pprcp->usParentPID      = pProcess->usParentPID;
    pprcp->usThreads        = pProcess->usThreads;
    pprcp->ulSID            = pProcess->ulSID;
    pprcp->ulSessionType    = pProcess->ulSessionType;
    pprcp->ulStatus         = pProcess->ulStatus;
}

/*
 *@@ prc16QueryProcessInfo:
 *      this searches for a given process ID (usPID) and
 *      fills a given PRCPROCESS structure with lots of
 *      information about this process.
 *      Returns FALSE upon errors, e.g. if no process
 *      of that ID is found.
 */

BOOL prc16QueryProcessInfo(USHORT usPID,  // in: PID to query
                           PPRCPROCESS pprcp) // out: process info
{
    BOOL rc = FALSE;
    PQPROCESS16 pProcess;
    PQPROCSTAT16 pps = (PQPROCSTAT16)malloc(0x8000);
    DosQProcStatus(pps, 0x8000);

    for ( pProcess = (PQPROCESS16)PTR(pps->ulProcesses, 0);
          pProcess->ulType != 3;
          pProcess = (PQPROCESS16)PTR(pProcess->ulThreadList,
                                      pProcess->usThreads * sizeof(QTHREAD16))
        )
    {
        if (pProcess->usPID == usPID)
        {
            prcReport16(pProcess, pprcp);
            rc = TRUE;
            break;
        }
    }

    free(pps);
    return (rc);
}

/*
 *@@ prc16ForEachProcess:
 *      this calls a given callback func for each running
 *      process. The callback must be a FNWP, which will be
 *      passed the following parameters for each call:
 *      --  HWND hwnd:       like hwnd passed to this func
 *      --  ULONG msg:       like msg passed to this func
 *      --  MPARAM mp1:      like mp1 passed to this func
 *      --  PPRCPROCESS: mp2 pointer to a PRCPROCESS struct for each process
 *
 *      This function returns the number of running processes on the
 *      system. If pfnwpCallback is NULL, only this number will be
 *      returned, so you can use this as a process counter too.
 */

ULONG prc16ForEachProcess(PFNWP pfnwpCallback, HWND hwnd, ULONG ulMsg, MPARAM mp1)
{
    ULONG ulrc = 0;
    PQPROCESS16 pProcess;
    PRCPROCESS prcp;
    PQPROCSTAT16 pps = (PQPROCSTAT16)malloc(0x8000);
    DosQProcStatus(pps, 0x8000);

    for ( pProcess = (PQPROCESS16)PTR(pps->ulProcesses, 0);
          pProcess->ulType != 3;
          pProcess = (PQPROCESS16)PTR(pProcess->ulThreadList,
                     pProcess->usThreads * sizeof(QTHREAD16))
        )
    {
        if (pfnwpCallback)
        {
            prcReport16(pProcess, &prcp);
            (*pfnwpCallback)(hwnd, ulMsg, mp1, &prcp);
        }
        ulrc++;
    }

    free(pps);
    return (ulrc);
}

/*
 *@@ prc16QueryThreadCount:
 *      returns the total number of running threads
 *      in the given process. If pid == 0, the
 *      total thread count for the system is returned.
 */

ULONG prc16QueryThreadCount(USHORT usPID)
{
    ULONG       ulrc = 0;
    PQPROCSTAT16  pps = (PQPROCSTAT16)malloc(0x8000);
    DosQProcStatus(pps, 0x8000);

    if (usPID)
    {
        // process query:
        PQPROCESS16 pProcess;
        for ( pProcess = (PQPROCESS16)PTR(pps->ulProcesses, 0);
              pProcess->ulType != 3;
              pProcess = (PQPROCESS16)PTR(pProcess->ulThreadList,
                                          pProcess->usThreads * sizeof(QTHREAD16))
            )
        {
            if (pProcess->usPID == usPID)
            {
                ulrc = pProcess->usThreads;
                break;
            }
        }
    }
    else
    {
        // global query:
        PQGLOBAL16   pg;
        pg = (PQGLOBAL16)PTR(pps->ulGlobal, 0);
        ulrc = pg->ulThreads;
    }

    free(pps);
    return (ulrc);
}

/*
 *@@ prc16QueryThreadInfo:
 *      this searches for a given thread in a given process
 *      and fills a given PRCTHREAD structure with lots of
 *      information about that thread.
 *
 *      Returns FALSE upon errors.
 *
 *      Note: This function loops thru all processes which
 *      are currently running and is therefore not terribly
 *      fast. Use economically.
 */

BOOL prc16QueryThreadInfo(USHORT usPID, USHORT usTID, PPRCTHREAD pprct)
{
    BOOL        brc = FALSE;
    PQPROCSTAT16  pps = (PQPROCSTAT16)malloc(0x8000);
    PQPROCESS16 pProcess;
    DosQProcStatus(pps, 0x8000);

    // find process:
    for ( pProcess = (PQPROCESS16)PTR(pps->ulProcesses, 0);
          pProcess->ulType != 3;
          pProcess = (PQPROCESS16)PTR(pProcess->ulThreadList,
                                      pProcess->usThreads * sizeof(QTHREAD16))
        )
    {
        if (pProcess->usPID == usPID)
        {
            PQTHREAD16 pThread;
            int i;
            // process found: find thread
            for ( i = 0, pThread = (PQTHREAD16)PTR(pProcess->ulThreadList, 0);
                  i < pProcess->usThreads;
                  i++, pThread++ )
            {
                if (pThread->usTID == usTID)
                {
                    // thread found:
                    pprct->usTID          = pThread->usTID;
                    pprct->usThreadSlotID = pThread->usThreadSlotID;
                    pprct->ulBlockID      = pThread->ulBlockID;
                    pprct->ulPriority     = pThread->ulPriority;
                    pprct->ulSysTime      = pThread->ulSysTime;
                    pprct->ulUserTime     = pThread->ulUserTime;
                    pprct->ucStatus       = pThread->ucStatus;

                    brc = TRUE;

                    break; // thread-for loop
                }
            } // end for thread
            break; // process-for loop
        }
    } // end for process

    free(pps);
    return (brc);
}

/*
 *@@ prcQueryPriority:
 *      shortcut to prc16QueryThreadInfo if you want the priority only.
 *
 *      Returns -1 upon errors.
 *
 *      Note: This function loops thru all processes which
 *      are currently running and is therefore not terribly
 *      fast. Use economically.
 */

ULONG prc16QueryThreadPriority(USHORT usPID,
                               USHORT usTID)
{
    PRCTHREAD prct;
    ULONG ulrc = -1;
    if (prc16QueryThreadInfo(usPID, usTID, &prct))
        ulrc = prct.ulPriority;
    return (ulrc);
}

/*
 *@@category: Helpers\Control program helpers\Process status\32-bit DosQuerySysState
 */

/********************************************************************
 *
 *   DosQuerySysState (32-bit) interface
 *
 ********************************************************************/

/*
 *@@ prc32GetInfo:
 *      nifty interface to DosQuerySysState,
 *      the 32-bit version of DosQProcStat.
 *      This returns the head of a newly
 *      allocated buffer which has plenty
 *      of pointers for subsequent browing.
 *
 *      Use prc32FreeInfo to free the buffer.
 *
 *@@added V0.9.1 (2000-02-12) [umoeller]
 *@@changed V0.9.3 (2000-05-01) [umoeller]: now using DosAllocMem
 */

PQTOPLEVEL32 prc32GetInfo(APIRET *parc)     // out: error, ptr can be NULL
{
    #define BUFSIZE 128000l
    PCHAR pBuf = NULL; // (PCHAR)malloc(BUFSIZE);

    if (DosAllocMem((PVOID*)&pBuf,
                    BUFSIZE,
                    PAG_READ | PAG_WRITE | PAG_COMMIT | OBJ_TILE)
            == NO_ERROR)
        if (pBuf)
        {
            APIRET arc = DosQuerySysState(0x1f,
                                          0, 0, 0,
                                          (PCHAR)pBuf,
                                          BUFSIZE);
            if (parc)
                *parc = arc;

            if (arc == NO_ERROR)
                return ((PQTOPLEVEL32)pBuf);
            else
                DosFreeMem(pBuf);
        }

    return (NULL);
}

/*
 *@@ prc32FreeInfo:
 *      frees the memory allocated by prc32GetInfo.
 *
 *@@added V0.9.1 (2000-02-12) [umoeller]
 *@@changed V0.9.3 (2000-05-01) [umoeller]: now using DosAllocMem
 */

VOID prc32FreeInfo(PQTOPLEVEL32 pInfo)
{
    DosFreeMem(pInfo);;
}

/*
 *@@ prc32FindProcessFromName:
 *
 *@@added V0.9.2 (2000-03-05) [umoeller]
 */

PQPROCESS32 prc32FindProcessFromName(PQTOPLEVEL32 pInfo,
                                     const char *pcszName) // in: e.g. "pmshell.exe"
{
    PQPROCESS32 pProcThis = pInfo->pProcessData;
    while (pProcThis && pProcThis->rectype == 1)
    {
        int i;
        PQTHREAD32  t = pProcThis->pThreads;
        PQMODULE32 pModule = prc32FindModule(pInfo,
                                             pProcThis->usHModule);

        if (pModule)
        {
            // the module name is fully qualified, so find the
            // file name (after the last backslash)
            if (pModule->pcName)
            {
                PSZ pLastBackslash = strrchr(pModule->pcName, '\\');
                if (pLastBackslash)
                    // found:
                    if (stricmp(pLastBackslash + 1, pcszName) == 0)
                        // matches:
                        break;
            }
        }

        // for next process, skip the threads info;
        // the next process block comes after the
        // threads
        for (i=0;
             i < pProcThis->usThreadCount;
             i++,t++)
            ;

        pProcThis = (PQPROCESS32)t;
    }

    if (pProcThis->rectype == 1)
        return (pProcThis);
    else
        return (NULL);
}

/*
 *@@ prc32FindSem16:
 *      attempts to find the specified 16-bit semaphore
 *      in the specified info buffer.
 *
 *      The return value points into the pInfo buffer.
 *      Returns NULL if not found.
 *
 *@@added V0.9.1 (2000-02-12) [umoeller]
 */

PQSEMA32 prc32FindSem16(PQTOPLEVEL32 pInfo,     // in: as returned by prc32GetInfo
                          USHORT usSemID)       // in: as in QPROCESS32.pausSem16
{
    PQSEM16STRUC32  pSemData = pInfo->pSem16Data;
    PQSEMA32        pSemThis = &pSemData->sema;
    ULONG           i = 0;

    while (pSemThis)
    {
        _Pmpf(("prc32FindSem16: found usIndex 0x%lX", pSemThis->usIndex));
        if (/* pSemThis->usIndex */ i == usSemID)
            return (pSemThis);

        i++;
        pSemThis = pSemThis->pNext;
    }

    return (NULL);
}

/*
 *@@ prc32FindSem32:
 *      attempts to find the specified 32-bit semaphore
 *      in the specified info buffer. This might fail
 *      because the data isn't always complete.
 *
 *      The return value points into the pInfo buffer.
 *      Returns NULL if not found.
 *
 *@@added V0.9.1 (2000-02-12) [umoeller]
 */

PQSEM32STRUC32 prc32FindSem32(PQTOPLEVEL32 pInfo,     // in: as returned by prc32GetInfo
                              USHORT usSemID)         // in: as in QPROCESS32.pausSem16
{
    // PQSEM32STRUC32  pSemThis = pInfo->pSem32Data;

    /* while (pSemThis)
    {
        if (pSemThis->usIndex == usSemID)
            return (pSemThis);

        pSemThis = pSemThis->pNext;
    } */

    return (NULL);
}

/*
 *@@ prc32FindShrMem:
 *      attempts to find the specified shared memory
 *      block description.
 *
 *      The return value points into the pInfo buffer.
 *      Returns NULL if not found.
 *
 *@@added V0.9.1 (2000-02-12) [umoeller]
 */

PQSHRMEM32 prc32FindShrMem(PQTOPLEVEL32 pInfo,  // in: as returned by prc32GetInfo
                           USHORT usShrMemID)   // in: as in QPROCESS32.pausShrMems
{
    PQSHRMEM32 pShrMem = pInfo->pShrMemData;
    while (pShrMem)
    {
        if (pShrMem->usHandle == usShrMemID)
            return (pShrMem);
        pShrMem = pShrMem->pNext;
    }

    return (NULL);
}

/*
 *@@ prc32FindModule:
 *      attempts to find the specified module description.
 *
 *      The return value points into the pInfo buffer.
 *      Returns NULL if not found.
 *
 *@@added V0.9.1 (2000-02-12) [umoeller]
 */

PQMODULE32 prc32FindModule(PQTOPLEVEL32 pInfo,  // in: as returned by prc32GetInfo
                           USHORT usHModule)
{
    PQMODULE32 pModule = pInfo->pModuleData;
    while (pModule)
    {
        if (pModule->usHModule == usHModule)
            return (pModule);
        pModule = pModule->pNext;
    }

    return (NULL);
}

/*
 *@@ prc32FindFileData:
 *
 *
 *@@added V0.9.1 (2000-02-12) [umoeller]
 */

PQFILEDATA32 prc32FindFileData(PQTOPLEVEL32 pInfo,  // in: as returned by prc32GetInfo
                               USHORT usFileID)     // in: as in QPROCESS32.pausFds
{
    PQFILEDATA32 pFile = pInfo->pFileData;
    while (     (pFile)
             && (pFile->rectype == 8)  // this is necessary, we'll crash otherwise!!
          )
    {
            if (pFile->filedata->sfn == usFileID)
                return (pFile);

        pFile = pFile->pNext;
    }

    return (NULL);
}


