
/*
 * memdebug.c:
 *      memory debugging helpers. Memory debugging is enabled
 *      if the __XWPMEMDEBUG__ #define is set in setup.h.
 *
 *      Several things are in here which might turn out to
 *      be useful:
 *
 *      -- Memory block dumping (memdDumpMemoryBlock).
 *
 *      -- Sophisticated heap debugging functions, which
 *         automatically replace malloc() and free() etc.
 *         when XWorkplace is compiled with debug code.
 *         These log every memory allocation made and log
 *         much more data compared to the VAC++ memory
 *         debugging functions. See HEAPITEM for details.
 *         Automatic heap checking is also included, using
 *         special "magic string" which are checked to
 *         detect overwrites.
 *
 *         To be able to trace memory errors, set the global
 *         variable G_pMemdLogFunc to a function which can
 *         write an error string to a meaningful place (the
 *         screen or a file). WARNING: While that error function
 *         is executed, the system might be in a global memory
 *         lock, so DON'T display a message box while in that
 *         function.
 *
 *         At present, malloc(), calloc(), strdup() and free()
 *         are supported. If you invoke free() on a memory block
 *         allocated by a function other than the above, you'll
 *         get a runtime error.
 *
 *         These debug functions have been added with V0.9.3
 *         and should now be compiler-independent.
 *
 *      -- A PM heap debugging window which shows the statuzs
 *         of the heap logging list. See memdCreateMemDebugWindow
 *         for details.
 *
 *      To enable memory debugging, do the following:
 *
 *      1) Include at least <stdlib.h> and <string.h>.
 *
 *      2) Include memdebug.h AFTER those two. This will remap
 *         the malloc() etc. calls.
 *
 *         If you don't want those replaced, add
 +              #define DONT_REPLACE_MALLOC
 *         before including memdebug.h.
 *
 *      That's all. XWorkplace's setup.h does this automatically
 *      if XWorkplace is compiled with debug code.
 *
 *@@added V0.9.1 (2000-02-12) [umoeller]
 */

/*
 *      Copyright (C) 2000 Ulrich M”ller.
 *      This program is part of the XWorkplace package.
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation, in version 2 as it comes in the COPYING
 *      file of the XWorkplace main distribution.
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 */

#define OS2EMX_PLAIN_CHAR
    // this is needed for "os2emx.h"; if this is defined,
    // emx will define PSZ as _signed_ char, otherwise
    // as unsigned char

#define INCL_DOSSEMAPHORES
#define INCL_DOSEXCEPTIONS
#define INCL_DOSPROCESS
#define INCL_DOSERRORS

#define INCL_WINWINDOWMGR
#define INCL_WINFRAMEMGR
#define INCL_WINCOUNTRY
#define INCL_WINSYS
#define INCL_WINMENUS
#define INCL_WINSTDCNR
#include <os2.h>

#include <stdio.h>
#include <string.h>
// #include <malloc.h>
#include <setjmp.h>

#define DONT_REPLACE_MALLOC             // we need the "real" malloc in this file
#include "setup.h"

#ifdef __XWPMEMDEBUG__

#include "helpers\cnrh.h"
#include "helpers\except.h"
// #include "helpers\memdebug.h"        // included by setup.h already
#include "helpers\stringh.h"
#include "helpers\winh.h"

/*
 *@@category: Helpers\C helpers\Heap debugging
 */

/* ******************************************************************
 *                                                                  *
 *   Global variables                                               *
 *                                                                  *
 ********************************************************************/

#define MEMBLOCKMAGIC_HEAD     "\210\203`H&cx$&%\254"
            // size must be a multiple of 4 or dword-alignment will fail;
            // there's an extra 0 byte at the end, so we have a size of 12
            // V0.9.3 (2000-04-17) [umoeller]
#define MEMBLOCKMAGIC_TAIL     "\250\210&%/dfjsk%#,dlhf\223"

/*
 *@@ HEAPITEM:
 *      informational structure created for each
 *      malloc() call by memdMalloc. These are stored
 *      in a global linked list (G_pHeapItemsRoot).
 *
 *      We cannot use the linklist.c functions for
 *      managing the linked list because these use
 *      malloc in turn, which would lead to infinite
 *      loops.
 *
 *@@added V0.9.3 (2000-04-11) [umoeller]
 */

typedef struct _HEAPITEM
{
    struct _HEAPITEM    *pNext;     // next item in linked list or NULL if last

    void                *pAfterMagic; // memory pointer returned by memdMalloc;
                                    // this points to after the magic string
    unsigned long       ulSize;     // size of *pData

    const char          *pcszSourceFile;    // as passed to memdMalloc
    unsigned long       ulLine;             // as passed to memdMalloc
    const char          *pcszFunction;      // as passed to memdMalloc

    DATETIME            dtAllocated;        // system date/time at time of memdMalloc call

    ULONG               ulTID;      // thread ID that memdMalloc was running on

    BOOL                fFreed;     // TRUE only after item has been freed by memdFree
} HEAPITEM, *PHEAPITEM;

HMTX            G_hmtxMallocList = NULLHANDLE;

PHEAPITEM       G_pHeapItemsRoot = NULL,
                G_pHeapItemsLast = NULL;

PSZ             G_pszMemCnrTitle = NULL;

PFNCBMEMDLOG    G_pMemdLogFunc = NULL;

ULONG           G_ulItemsReleased = 0,
                G_ulBytesReleased = 0;

/* ******************************************************************
 *                                                                  *
 *   Debug heap management                                          *
 *                                                                  *
 ********************************************************************/

/*
 *@@ memdLock:
 *      enables the global memory lock to protect
 *      the global variables here. Use memdUnlock
 *      to unlock again, and lock only for the shortest
 *      possible time. This is only used by the memdebug.c
 *      functions.
 *
 *@@added V0.9.3 (2000-04-10) [umoeller]
 */

BOOL memdLock(VOID)
{
    APIRET arc = NO_ERROR;
    if (G_hmtxMallocList == NULLHANDLE)
        // first call:
        arc = DosCreateMutexSem(NULL,
                                &G_hmtxMallocList,
                                0,
                                FALSE);

    arc = DosRequestMutexSem(G_hmtxMallocList,
                             SEM_INDEFINITE_WAIT);

    return (arc == NO_ERROR);
}

/*
 *@@ memdUnlock:
 *      the reverse to memdLock.
 *
 *@@added V0.9.3 (2000-04-10) [umoeller]
 */

VOID memdUnlock(VOID)
{
    DosReleaseMutexSem(G_hmtxMallocList);
}

/*
 *@@ memdMalloc:
 *      wrapper function for malloc() to trace malloc()
 *      calls more precisely.
 *
 *      If XWorkplace is compiled with debug code, setup.h
 *      automatically #includes memdebug.h, which maps
 *      malloc to this function so that the source file
 *      etc. parameters automatically get passed.
 *
 *      For each call, we call the default malloc(), whose
 *      return value is returned, and create a HEAPITEM
 *      for remembering the call, which is stored in a global
 *      linked list.
 *
 *@@added V0.9.3 (2000-04-11) [umoeller]
 */

void* memdMalloc(size_t stSize,
                 const char *pcszSourceFile, // in: source file name
                 unsigned long ulLine,       // in: source line
                 const char *pcszFunction)   // in: function name
{
    void *prc = NULL;

    if (stSize == 0)
        // malloc(0) called: report error
        if (G_pMemdLogFunc)
        {
            CHAR szMsg[1000];
            sprintf(szMsg,
                    "Function %s (%s, line %d) called malloc(0).",
                    pcszFunction,
                        pcszSourceFile,
                        ulLine);
            G_pMemdLogFunc(szMsg);
        }

    if (memdLock())
    {
        // call default malloc(), but with the additional
        // size of our MEMBLOCKMAGIC strings; we'll return
        // the first byte after the "front" string so we can
        // check for string overwrites
        void *pObj = malloc(stSize
                            + sizeof(MEMBLOCKMAGIC_HEAD)
                            + sizeof(MEMBLOCKMAGIC_TAIL));
        if (pObj)
        {
            PHEAPITEM   pHeapItem = (PHEAPITEM)malloc(sizeof(HEAPITEM));

            // store "front" magic string
            memcpy(pObj,
                   MEMBLOCKMAGIC_HEAD,
                   sizeof(MEMBLOCKMAGIC_HEAD));
            // return address: first byte after "front" magic string
            prc = ((PBYTE)pObj) + sizeof(MEMBLOCKMAGIC_HEAD);
            // store "tail" magic string to block which
            // will be returned plus the size which was requested
            memcpy(((PBYTE)prc) + stSize,
                   MEMBLOCKMAGIC_TAIL,
                   sizeof(MEMBLOCKMAGIC_TAIL));

            if (pHeapItem)
            {
                PTIB        ptib;
                PPIB        ppib;

                pHeapItem->pNext = 0;

                pHeapItem->pAfterMagic = prc;
                pHeapItem->ulSize = stSize;
                pHeapItem->pcszSourceFile = pcszSourceFile;
                pHeapItem->ulLine = ulLine;
                pHeapItem->pcszFunction = pcszFunction;

                DosGetDateTime(&pHeapItem->dtAllocated);

                pHeapItem->ulTID = 0;

                if (DosGetInfoBlocks(&ptib, &ppib) == NO_ERROR)
                    if (ptib)
                        if (ptib->tib_ptib2)
                            pHeapItem->ulTID = ptib->tib_ptib2->tib2_ultid;

                pHeapItem->fFreed = FALSE;

                // append heap item to linked list
                if (G_pHeapItemsRoot == NULL)
                    // first item:
                    G_pHeapItemsRoot = pHeapItem;
                else
                    // we have items already:
                    if (G_pHeapItemsLast)
                    {
                        // last item cached:
                        G_pHeapItemsLast->pNext = pHeapItem;
                        G_pHeapItemsLast = pHeapItem;
                    }
                    else
                    {
                        // not cached: find end of list
                        PHEAPITEM phi = G_pHeapItemsRoot;
                        while (phi->pNext)
                            phi = phi->pNext;

                        phi->pNext = pHeapItem;
                        G_pHeapItemsLast = pHeapItem;
                    }
            }
        }

        memdUnlock();
    }

    return (prc);
}

/*
 *@@ memdCalloc:
 *      similar to memdMalloc; this is the wrapper for
 *      the calloc() call. This is automatically
 *      remapped also.
 *
 *@@added V0.9.3 (2000-04-11) [umoeller]
 */

void* memdCalloc(size_t num,
                 size_t stSize,
                 const char *pcszSourceFile,
                 unsigned long ulLine,
                 const char *pcszFunction)
{
    void *p = memdMalloc(num * stSize,
                         pcszSourceFile,
                         ulLine,
                         pcszFunction);
    memset(p, 0, num * stSize);
    return (p);
}

/*
 *@@ memdFree:
 *      wrapper for the free() call, which is remapped
 *      by setup.h and memdebug.h like memdMalloc
 *      and memdCalloc. This searches the memory object
 *      (p) which was previously allocated on the linked
 *      list of HEAPITEM's and frees it then by calling
 *      the default free().
 *
 *      The HEAPITEM itself is not freed, but only marked
 *      as freed. As a result, the linked list can grow
 *      REALLY large. While memdMalloc does not become
 *      slower with large HEAPITEM lists because it only
 *      appends to the end of the list, which is remembered,
 *      memdFree can become extremely slow because the entire
 *      list needs to be searched with each call.
 *      So call memdReleaseFreed from time to time.
 *
 *@@added V0.9.3 (2000-04-10) [umoeller]
 */

void memdFree(void *p,
              const char *pcszSourceFile,
              unsigned long ulLine,
              const char *pcszFunction)
{
    BOOL fFound = FALSE;
    if (memdLock())
    {
        // PLISTNODE   pNode = lstQueryFirstNode(&G_llHeapItems);
        PHEAPITEM pHeapItem = G_pHeapItemsRoot;

        // search the list with the pointer which was
        // really returned by the original malloc(),
        // that is, the byte before the magic string
        void *pBeforeMagic = ((PBYTE)p) - sizeof(MEMBLOCKMAGIC_HEAD);

        while (pHeapItem)
        {
            if (pHeapItem->pAfterMagic == p)
                // the same address may be allocated and freed
                // several times, so if this address has been
                // freed, search on
                if (!pHeapItem->fFreed)
                {
                    // found:
                    ULONG   ulError = 0;
                    // check magic string
                    if (memcmp(pBeforeMagic,
                               MEMBLOCKMAGIC_HEAD,
                               sizeof(MEMBLOCKMAGIC_HEAD))
                            != 0)
                        ulError = 1;
                    else if (memcmp(((PBYTE)pHeapItem->pAfterMagic) + pHeapItem->ulSize,
                                    MEMBLOCKMAGIC_TAIL,
                                    sizeof(MEMBLOCKMAGIC_TAIL))
                            != 0)
                        ulError = 2;

                    if (ulError)
                    {
                        // magic block has been overwritten:
                        if (G_pMemdLogFunc)
                        {
                            CHAR szMsg[1000];
                            sprintf(szMsg,
                                    "Magic string %s memory block at 0x%lX has been overwritten.\n"
                                    "This was detected by the free() call at %s (%s, line %d).\n"
                                    "The block was allocated by %s (%s, line %d).",
                                    (ulError == 1) ? "before" : "after",
                                    p,
                                    pcszFunction,
                                        pcszSourceFile,
                                        ulLine, // free
                                    pHeapItem->pcszFunction,
                                        pHeapItem->pcszSourceFile,
                                        pHeapItem->ulLine);
                            G_pMemdLogFunc(szMsg);
                        }
                    }

                    free(pBeforeMagic);
                    pHeapItem->fFreed = TRUE;

                    /* lstRemoveNode(&G_llHeapItems,
                                  pNode); */
                    fFound = TRUE;
                    break;
                }

            pHeapItem = pHeapItem->pNext;
        }

        memdUnlock();
    }

    if (!fFound)
        if (G_pMemdLogFunc)
        {
            CHAR szMsg[1000];
            sprintf(szMsg,
                    "free() failed. Called from %s (%s, line %d) for object 0x%lX.",
                    pcszFunction,
                        pcszSourceFile,
                        ulLine,
                    p);
            G_pMemdLogFunc(szMsg);
        }
}

/*
 *@@ memdReleaseFreed:
 *      goes thru the entire global HEAPITEM's list
 *      and throws out all items which have been freed.
 *      Call this from time to time in order to keep
 *      the system usable. See memdFree() for details.
 *
 *      Returns the no. of HEAPITEM's that have been
 *      released.
 *
 *@@added V0.9.3 (2000-04-11) [umoeller]
 */

unsigned long memdReleaseFreed(void)
{
    BOOL    ulItemsReleased = 0,
            ulBytesReleased = 0;
    if (memdLock())
    {
        PHEAPITEM pHeapItem = G_pHeapItemsRoot,
                  pPrevious = NULL;

        while (pHeapItem)
        {
            PHEAPITEM   pNext = pHeapItem->pNext;       // can be NULL
            if (pHeapItem->fFreed)
            {
                // item freed:
                if (pPrevious == NULL)
                    // head of list:
                    G_pHeapItemsRoot = pNext;           // can be NULL
                else
                    // somewhere later:
                    // link next to previous to skip current
                    pPrevious->pNext = pNext;           // can be NULL

                ulItemsReleased++;
                ulBytesReleased += pHeapItem->ulSize;

                free(pHeapItem);
            }
            else
                // item still valid:
                pPrevious = pHeapItem;

            pHeapItem = pNext;
        }

        G_ulItemsReleased += ulItemsReleased;
        G_ulBytesReleased += ulBytesReleased;

        memdUnlock();
    }

    return (ulItemsReleased);
}

/* ******************************************************************
 *                                                                  *
 *   XFolder debugging helpers                                      *
 *                                                                  *
 ********************************************************************/

#ifdef _PMPRINTF_
    /*
     *@@ memdDumpMemoryBlock:
     *      if _PMPRINTF_ has been #define'd before including
     *      memdebug.h,
     *      this will dump a block of memory to the PMPRINTF
     *      output window. Useful for debugging internal
     *      structures.
     *      If _PMPRINTF_ has been NOT #define'd,
     *      no code will be produced at all. :-)
     */

    void memdDumpMemoryBlock(PBYTE pb,       // in: start address
                             ULONG ulSize,   // in: size of block
                             ULONG ulIndent) // in: how many spaces to put
                                             //     before each output line
    {
        PSZ psz = strhCreateDump(pb, ulSize, ulIndent);
        if (psz)
        {
            _Pmpf(("\n%s", psz));
            free(psz);
        }
    }
#endif

/* ******************************************************************
 *                                                                  *
 *   Heap debugging window                                          *
 *                                                                  *
 ********************************************************************/

/*
 *@@ MEMRECORD:
 *
 *@@added V0.9.1 (99-12-04) [umoeller]
 */

typedef struct _MEMRECORD
{
    RECORDCORE  recc;

    ULONG       ulIndex;

    CDATE       cdateAllocated;
    CTIME       ctimeAllocated;

    PSZ         pszFreed;

    ULONG       ulTID;

    PSZ         pszFunction;    // points to szFunction
    CHAR        szFunction[400];

    PSZ         pszSource;      // points to szSource
    CHAR        szSource[CCHMAXPATH];

    ULONG       ulLine;

    PSZ         pszAddress;     // points to szAddress
    CHAR        szAddress[20];

    ULONG       ulSize;

} MEMRECORD, *PMEMRECORD;

/* ULONG       ulHeapItemsCount1,
            ulHeapItemsCount2;
ULONG       ulTotalAllocated,
            ulTotalFreed;
PMEMRECORD  pMemRecordThis = NULL;
PSZ         pszMemCnrTitle = NULL; */

#if 0
    /*
     *@@ fncbMemHeapWalkCount:
     *      callback func for _heap_walk function used for
     *      fnwpMemDebug.
     *
     *@@added V0.9.1 (99-12-04) [umoeller]
     */

    int fncbMemHeapWalkCount(const void *pObject,
                             size_t Size,
                             int useflag,
                             int status,
                             const char *filename,
                             size_t line)
    {
        // skip all the items which seem to be
        // internal to the runtime
        if ((filename) || (useflag == _FREEENTRY))
        {
            ulHeapItemsCount1++;
            if (useflag == _FREEENTRY)
                ulTotalFreed += Size;
            else
                ulTotalAllocated += Size;
        }
        return (0);
    }

    /*
     *@@ fncbMemHeapWalkFill:
     *      callback func for _heap_walk function used for
     *      fnwpMemDebug.
     *
     *@@added V0.9.1 (99-12-04) [umoeller]
     */

    int fncbMemHeapWalkFill(const void *pObject,
                            size_t Size,
                            int useflag,
                            int status,
                            const char *filename,
                            size_t line)
    {
        // skip all the items which seem to be
        // internal to the runtime
        if ((filename) || (useflag == _FREEENTRY))
        {
            ulHeapItemsCount2++;
            if ((pMemRecordThis) && (ulHeapItemsCount2 < ulHeapItemsCount1))
            {
                pMemRecordThis->ulIndex = ulHeapItemsCount2 - 1;

                pMemRecordThis->pObject = pObject;
                pMemRecordThis->useflag = useflag;
                pMemRecordThis->status = status;
                pMemRecordThis->filename = filename;

                pMemRecordThis->pszAddress = pMemRecordThis->szAddress;

                pMemRecordThis->ulSize = Size;

                pMemRecordThis->pszSource = pMemRecordThis->szSource;

                pMemRecordThis->ulLine = line;

                pMemRecordThis = (PMEMRECORD)pMemRecordThis->recc.preccNextRecord;
            }
            else
                return (1);     // stop
        }

        return (0);
    }

    /*
     *@@ memdCreateRecordsVAC:
     *
     *@@added V0.9.3 (2000-04-10) [umoeller]
     */

    VOID memdCreateRecordsVAC(HWND hwndCnr)
    {
        // count heap items
        ulHeapItemsCount1 = 0;
        ulTotalFreed = 0;
        ulTotalAllocated = 0;
        _heap_walk(fncbMemHeapWalkCount);

        pMemRecordFirst = (PMEMRECORD)cnrhAllocRecords(hwndCnr,
                                                       sizeof(MEMRECORD),
                                                       ulHeapItemsCount1);
        if (pMemRecordFirst)
        {
            ulHeapItemsCount2 = 0;
            pMemRecordThis = pMemRecordFirst;
            _heap_walk(fncbMemHeapWalkFill);

            // the following doesn't work while _heap_walk is running
            pMemRecordThis = pMemRecordFirst;
            while (pMemRecordThis)
            {
                switch (pMemRecordThis->useflag)
                {
                    case _USEDENTRY: pMemRecordThis->pszUseFlag = "Used"; break;
                    case _FREEENTRY: pMemRecordThis->pszUseFlag = "Freed"; break;
                }

                switch (pMemRecordThis->status)
                {
                    case _HEAPBADBEGIN: pMemRecordThis->pszStatus = "heap bad begin"; break;
                    case _HEAPBADNODE: pMemRecordThis->pszStatus = "heap bad node"; break;
                    case _HEAPEMPTY: pMemRecordThis->pszStatus = "heap empty"; break;
                    case _HEAPOK: pMemRecordThis->pszStatus = "OK"; break;
                }

                sprintf(pMemRecordThis->szAddress, "0x%lX", pMemRecordThis->pObject);
                strcpy(pMemRecordThis->szSource,
                        (pMemRecordThis->filename)
                            ? pMemRecordThis->filename
                            : "?");

                pMemRecordThis = (PMEMRECORD)pMemRecordThis->recc.preccNextRecord;
            }

            cnrhInsertRecords(hwndCnr,
                              NULL,         // parent
                              (PRECORDCORE)pMemRecordFirst,
                              TRUE,
                              NULL,
                              CRA_RECORDREADONLY,
                              ulHeapItemsCount2);
        }
    }

#endif

/*
 *@@ memdCreateRecords:
 *
 *@@added V0.9.3 (2000-04-10) [umoeller]
 */

VOID memdCreateRecords(HWND hwndCnr,
                       PULONG pulTotalItems,
                       PULONG pulAllocatedItems,
                       PULONG pulFreedItems,
                       PULONG pulTotalBytes,
                       PULONG pulAllocatedBytes,
                       PULONG pulFreedBytes)
{
    // count heap items
    ULONG       ulHeapItemsCount1 = 0;
    PMEMRECORD  pMemRecordFirst;

    if (memdLock())
    {
        // PLISTNODE   pNode = lstQueryFirstNode(&G_llHeapItems);
        PHEAPITEM pHeapItem = G_pHeapItemsRoot;

        *pulTotalItems = 0;
        *pulAllocatedItems = 0;
        *pulFreedItems = 0;

        *pulTotalBytes = 0;
        *pulAllocatedBytes = 0;
        *pulFreedBytes = 0;

        while (pHeapItem)
        {
            ulHeapItemsCount1++;
            if (pHeapItem->fFreed)
            {
                (*pulFreedItems)++;
                (*pulFreedBytes) += pHeapItem->ulSize;
            }
            else
            {
                (*pulAllocatedItems)++;
                (*pulAllocatedBytes) += pHeapItem->ulSize;
            }

            (*pulTotalBytes) += pHeapItem->ulSize;

            pHeapItem = pHeapItem->pNext;
        }

        *pulTotalItems = ulHeapItemsCount1;

        pMemRecordFirst = (PMEMRECORD)cnrhAllocRecords(hwndCnr,
                                                       sizeof(MEMRECORD),
                                                       ulHeapItemsCount1);
        if (pMemRecordFirst)
        {
            ULONG       ulHeapItemsCount2 = 0;
            PMEMRECORD  pMemRecordThis = pMemRecordFirst;
            pHeapItem = G_pHeapItemsRoot;
            // PLISTNODE   pMemNode = lstQueryFirstNode(&G_llHeapItems);

            while ((pMemRecordThis) && (pHeapItem))
            {
                // PHEAPITEM pHeapItem = (PHEAPITEM)pMemNode->pItemData;

                pMemRecordThis->ulIndex = ulHeapItemsCount2++;

                cnrhDateTimeDos2Win(&pHeapItem->dtAllocated,
                                    &pMemRecordThis->cdateAllocated,
                                    &pMemRecordThis->ctimeAllocated);

                if (pHeapItem->fFreed)
                    pMemRecordThis->pszFreed = "yes";

                pMemRecordThis->ulTID = pHeapItem->ulTID;

                strcpy(pMemRecordThis->szSource, pHeapItem->pcszSourceFile);
                pMemRecordThis->pszSource = pMemRecordThis->szSource;

                pMemRecordThis->ulLine = pHeapItem->ulLine;

                strcpy(pMemRecordThis->szFunction, pHeapItem->pcszFunction);
                pMemRecordThis->pszFunction = pMemRecordThis->szFunction;

                sprintf(pMemRecordThis->szAddress, "0x%lX", pHeapItem->pAfterMagic);
                pMemRecordThis->pszAddress = pMemRecordThis->szAddress;

                pMemRecordThis->ulSize = pHeapItem->ulSize;


                /* switch (pMemRecordThis->useflag)
                {
                    case _USEDENTRY: pMemRecordThis->pszUseFlag = "Used"; break;
                    case _FREEENTRY: pMemRecordThis->pszUseFlag = "Freed"; break;
                }

                switch (pMemRecordThis->status)
                {
                    case _HEAPBADBEGIN: pMemRecordThis->pszStatus = "heap bad begin"; break;
                    case _HEAPBADNODE: pMemRecordThis->pszStatus = "heap bad node"; break;
                    case _HEAPEMPTY: pMemRecordThis->pszStatus = "heap empty"; break;
                    case _HEAPOK: pMemRecordThis->pszStatus = "OK"; break;
                }

                sprintf(pMemRecordThis->szAddress, "0x%lX", pMemRecordThis->pObject);
                strcpy(pMemRecordThis->szSource,
                        (pMemRecordThis->filename)
                            ? pMemRecordThis->filename
                            : "?"); */

                pMemRecordThis = (PMEMRECORD)pMemRecordThis->recc.preccNextRecord;
                pHeapItem = pHeapItem->pNext;
            }

            cnrhInsertRecords(hwndCnr,
                              NULL,         // parent
                              (PRECORDCORE)pMemRecordFirst,
                              TRUE,
                              NULL,
                              CRA_RECORDREADONLY,
                              ulHeapItemsCount2);
        }

        memdUnlock();
    }
}

/*
 *@@ mnu_fnCompareIndex:
 *
 *@@added V0.9.1 (99-12-03) [umoeller]
 */

SHORT EXPENTRY mnu_fnCompareIndex(PMEMRECORD pmrc1, PMEMRECORD  pmrc2, PVOID pStorage)
{
    // HAB habDesktop = WinQueryAnchorBlock(HWND_DESKTOP);
    pStorage = pStorage; // to keep the compiler happy
    if ((pmrc1) && (pmrc2))
        if (pmrc1->ulIndex < pmrc2->ulIndex)
            return (-1);
        else if (pmrc1->ulIndex > pmrc2->ulIndex)
            return (1);

    return (0);
}

/*
 *@@ mnu_fnCompareSourceFile:
 *
 *@@added V0.9.1 (99-12-03) [umoeller]
 */

SHORT EXPENTRY mnu_fnCompareSourceFile(PMEMRECORD pmrc1, PMEMRECORD  pmrc2, PVOID pStorage)
{
    HAB habDesktop = WinQueryAnchorBlock(HWND_DESKTOP);
    pStorage = pStorage; // to keep the compiler happy
    if ((pmrc1) && (pmrc2))
            switch (WinCompareStrings(habDesktop, 0, 0,
                                      pmrc1->szSource,
                                      pmrc2->szSource,
                                      0))
            {
                case WCS_LT: return (-1);
                case WCS_GT: return (1);
                default:    // equal
                    if (pmrc1->ulLine < pmrc2->ulLine)
                        return (-1);
                    else if (pmrc1->ulLine > pmrc2->ulLine)
                        return (1);

            }

    return (0);
}

#define ID_MEMCNR   1000

/*
 *@@ memd_fnwpMemDebug:
 *      client window proc for the heap debugger window
 *      accessible from the Desktop context menu if
 *      __XWPMEMDEBUG__ is defined. Otherwise, this is not
 *      compiled.
 *
 *      Usage: this is a regular PM client window procedure
 *      to be used with WinRegisterClass and WinCreateStdWindow.
 *      See dtpMenuItemSelected, which uses this.
 *
 *      This creates a container with all the memory objects
 *      with the size of the client area in turn.
 *
 *@@added V0.9.1 (99-12-04) [umoeller]
 */


MRESULT EXPENTRY memd_fnwpMemDebug(HWND hwndClient, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    MRESULT mrc = 0;

    switch (msg)
    {
        case WM_CREATE:
        {
            TRY_LOUD(excpt1, NULL)
            {
                // PCREATESTRUCT pcs = (PCREATESTRUCT)mp2;
                HWND hwndCnr;
                hwndCnr = WinCreateWindow(hwndClient,        // parent
                                          WC_CONTAINER,
                                          "",
                                          WS_VISIBLE | CCS_MINIICONS | CCS_READONLY | CCS_SINGLESEL,
                                          0, 0, 0, 0,
                                          hwndClient,        // owner
                                          HWND_TOP,
                                          ID_MEMCNR,
                                          NULL, NULL);
                if (hwndCnr)
                {
                    XFIELDINFO      xfi[11];
                    PFIELDINFO      pfi = NULL;
                    PMEMRECORD      pMemRecordFirst;
                    int             i = 0;

                    ULONG           ulTotalItems = 0,
                                    ulAllocatedItems = 0,
                                    ulFreedItems = 0;
                    ULONG           ulTotalBytes = 0,
                                    ulAllocatedBytes = 0,
                                    ulFreedBytes = 0;

                    // set up cnr details view
                    xfi[i].ulFieldOffset = FIELDOFFSET(MEMRECORD, ulIndex);
                    xfi[i].pszColumnTitle = "No.";
                    xfi[i].ulDataType = CFA_ULONG;
                    xfi[i++].ulOrientation = CFA_RIGHT;

                    xfi[i].ulFieldOffset = FIELDOFFSET(MEMRECORD, cdateAllocated);
                    xfi[i].pszColumnTitle = "Date";
                    xfi[i].ulDataType = CFA_DATE;
                    xfi[i++].ulOrientation = CFA_LEFT;

                    xfi[i].ulFieldOffset = FIELDOFFSET(MEMRECORD, ctimeAllocated);
                    xfi[i].pszColumnTitle = "Time";
                    xfi[i].ulDataType = CFA_TIME;
                    xfi[i++].ulOrientation = CFA_LEFT;

                    xfi[i].ulFieldOffset = FIELDOFFSET(MEMRECORD, pszFreed);
                    xfi[i].pszColumnTitle = "Freed";
                    xfi[i].ulDataType = CFA_STRING;
                    xfi[i++].ulOrientation = CFA_CENTER;

                    xfi[i].ulFieldOffset = FIELDOFFSET(MEMRECORD, ulTID);
                    xfi[i].pszColumnTitle = "TID";
                    xfi[i].ulDataType = CFA_ULONG;
                    xfi[i++].ulOrientation = CFA_RIGHT;

                    xfi[i].ulFieldOffset = FIELDOFFSET(MEMRECORD, pszFunction);
                    xfi[i].pszColumnTitle = "Function";
                    xfi[i].ulDataType = CFA_STRING;
                    xfi[i++].ulOrientation = CFA_LEFT;

                    xfi[i].ulFieldOffset = FIELDOFFSET(MEMRECORD, pszSource);
                    xfi[i].pszColumnTitle = "Source";
                    xfi[i].ulDataType = CFA_STRING;
                    xfi[i++].ulOrientation = CFA_LEFT;

                    xfi[i].ulFieldOffset = FIELDOFFSET(MEMRECORD, ulLine);
                    xfi[i].pszColumnTitle = "Line";
                    xfi[i].ulDataType = CFA_ULONG;
                    xfi[i++].ulOrientation = CFA_RIGHT;

                    xfi[i].ulFieldOffset = FIELDOFFSET(MEMRECORD, ulSize);
                    xfi[i].pszColumnTitle = "Size";
                    xfi[i].ulDataType = CFA_ULONG;
                    xfi[i++].ulOrientation = CFA_RIGHT;

                    xfi[i].ulFieldOffset = FIELDOFFSET(MEMRECORD, pszAddress);
                    xfi[i].pszColumnTitle = "Address";
                    xfi[i].ulDataType = CFA_STRING;
                    xfi[i++].ulOrientation = CFA_LEFT;

                    pfi = cnrhSetFieldInfos(hwndCnr,
                                            &xfi[0],
                                            i,             // array item count
                                            TRUE,          // no draw lines
                                            3);            // return column

                    {
                        PSZ pszFont = "9.WarpSans";
                        WinSetPresParam(hwndCnr,
                                        PP_FONTNAMESIZE,
                                        strlen(pszFont),
                                        pszFont);
                    }

                    memdCreateRecords(hwndCnr,
                                      &ulTotalItems,
                                      &ulAllocatedItems,
                                      &ulFreedItems,
                                      &ulTotalBytes,
                                      &ulAllocatedBytes,
                                      &ulFreedBytes);

                    BEGIN_CNRINFO()
                    {
                        CHAR    szCnrTitle[1000];
                        CHAR    szTotalItems[100],
                                szAllocatedItems[100],
                                szFreedItems[100],
                                szReleasedItems[100];
                        CHAR    szTotalBytes[100],
                                szAllocatedBytes[100],
                                szFreedBytes[100],
                                szReleasedBytes[100];
                        sprintf(szCnrTitle,
                                "Total logs in use: %s items = %s bytes\n"
                                "    Total in use: %s items = %s bytes\n"
                                "    Total freed: %s items = %s bytes\n"
                                "Total logs released: %s items = %s bytes",
                                strhThousandsDouble(szTotalItems,
                                                    ulTotalItems,
                                                    '.'),
                                strhThousandsDouble(szTotalBytes,
                                                    ulTotalBytes,
                                                    '.'),
                                strhThousandsDouble(szAllocatedItems,
                                                    ulAllocatedItems,
                                                    '.'),
                                strhThousandsDouble(szAllocatedBytes,
                                                    ulAllocatedBytes,
                                                    '.'),
                                strhThousandsDouble(szFreedItems,
                                                    ulFreedItems,
                                                    '.'),
                                strhThousandsDouble(szFreedBytes,
                                                    ulFreedBytes,
                                                    '.'),
                                strhThousandsDouble(szReleasedItems,
                                                    G_ulItemsReleased,
                                                    '.'),
                                strhThousandsDouble(szReleasedBytes,
                                                    G_ulBytesReleased,
                                                    '.'));
                        G_pszMemCnrTitle = strdup(szCnrTitle);
                        cnrhSetTitle(G_pszMemCnrTitle);
                        cnrhSetView(CV_DETAIL | CV_MINI | CA_DETAILSVIEWTITLES
                                        | CA_DRAWICON
                                    | CA_CONTAINERTITLE | CA_TITLEREADONLY
                                        | CA_TITLESEPARATOR | CA_TITLELEFT);
                        cnrhSetSplitBarAfter(pfi);
                        cnrhSetSplitBarPos(250);
                    } END_CNRINFO(hwndCnr);

                    WinSetFocus(HWND_DESKTOP, hwndCnr);
                }
            }
            CATCH(excpt1) {} END_CATCH();

            mrc = WinDefWindowProc(hwndClient, msg, mp1, mp2);
        break; }

        case WM_WINDOWPOSCHANGED:
        {
            PSWP pswp = (PSWP)mp1;
            mrc = WinDefWindowProc(hwndClient, msg, mp1, mp2);
            if (pswp->fl & SWP_SIZE)
            {
                WinSetWindowPos(WinWindowFromID(hwndClient, ID_MEMCNR), // cnr
                                HWND_TOP,
                                0, 0, pswp->cx, pswp->cy,
                                SWP_SIZE | SWP_MOVE | SWP_SHOW);
            }
        break; }

        case WM_CONTROL:
        {
            USHORT usItemID = SHORT1FROMMP(mp1),
                   usNotifyCode = SHORT2FROMMP(mp1);
            if (usItemID == ID_MEMCNR)       // cnr
            {
                switch (usNotifyCode)
                {
                    case CN_CONTEXTMENU:
                    {
                        PMEMRECORD precc = (PMEMRECORD)mp2;
                        if (precc == NULL)
                        {
                            // whitespace
                            HWND hwndMenu = WinCreateMenu(HWND_DESKTOP,
                                                          NULL); // no menu template
                            winhInsertMenuItem(hwndMenu,
                                               MIT_END,
                                               1001,
                                               "Sort by index",
                                               MIS_TEXT, 0);
                            winhInsertMenuItem(hwndMenu,
                                               MIT_END,
                                               1002,
                                               "Sort by source file",
                                               MIS_TEXT, 0);
                            cnrhShowContextMenu(WinWindowFromID(hwndClient, ID_MEMCNR),
                                                NULL,       // record
                                                hwndMenu,
                                                hwndClient);
                        }
                    }
                }
            }
        break; }

        case WM_COMMAND:
            switch (SHORT1FROMMP(mp1))
            {
                case 1001:  // sort by index
                    WinSendMsg(WinWindowFromID(hwndClient, ID_MEMCNR),
                               CM_SORTRECORD,
                               (MPARAM)mnu_fnCompareIndex,
                               0);
                break;

                case 1002:  // sort by source file
                    WinSendMsg(WinWindowFromID(hwndClient, ID_MEMCNR),
                               CM_SORTRECORD,
                               (MPARAM)mnu_fnCompareSourceFile,
                               0);
                break;
            }
        break;

        case WM_CLOSE:
            WinDestroyWindow(WinWindowFromID(hwndClient, ID_MEMCNR));
            WinDestroyWindow(WinQueryWindow(hwndClient, QW_PARENT));
            free(G_pszMemCnrTitle);
            G_pszMemCnrTitle = NULL;
        break;

        default:
            mrc = WinDefWindowProc(hwndClient, msg, mp1, mp2);
    }

    return (mrc);
}

/*
 *@@ memdCreateMemDebugWindow:
 *      creates a heap debugging window which
 *      is a standard frame with a container,
 *      listing all heap objects ever allocated.
 *
 *      The client of this standard frame is in
 *      memd_fnwpMemDebug.
 *
 *      This thing lists all calls to malloc()
 *      which were ever made, including the
 *      source file and source line number which
 *      made the call. Extreeeemely useful for
 *      detecting memory leaks.
 *
 *      This only works if the memory functions
 *      have been replaced with the debug versions
 *      in this file.
 */

VOID memdCreateMemDebugWindow(VOID)
{
    ULONG flStyle = FCF_TITLEBAR | FCF_SYSMENU | FCF_HIDEMAX
                    | FCF_SIZEBORDER | FCF_SHELLPOSITION
                    | FCF_NOBYTEALIGN | FCF_TASKLIST;
    if (WinRegisterClass(WinQueryAnchorBlock(HWND_DESKTOP),
                         "XWPMemDebug",
                         memd_fnwpMemDebug, 0L, 0))
    {
        HWND hwndClient;
        HWND hwndMemFrame = WinCreateStdWindow(HWND_DESKTOP,
                                               0L,
                                               &flStyle,
                                               "XWPMemDebug",
                                               "Allocated XWorkplace Memory Objects",
                                               0L,
                                               NULLHANDLE,     // resource
                                               0,
                                               &hwndClient);
        if (hwndMemFrame)
        {
            WinSetWindowPos(hwndMemFrame,
                            HWND_TOP,
                            0, 0, 0, 0,
                            SWP_ZORDER | SWP_SHOW | SWP_ACTIVATE);
        }
    }
}

#else
void memdDummy(void)
{
    int i = 0;
}
#endif

