
/*
 *@@sourcefile except.c:
 *      this file contains powerful exception handlers.
 *
 *      Usage: All OS/2 programs, PM or text mode.
 *
 *      <B>except.h macros</B>
 *
 *      except.h declares a few handy macros to easily install
 *      exception handling for a section of code.
 *
 *      These macros automatically insert code for properly
 *      registering and deregistering the handlers. You should
 *      ALWAYS use these macros instead of directly registering
 *      the handlers to avoid accidentally forgetting to deregister
 *      them. If you forget to deregister an exception handler, this
 *      can lead to really strange errors (crashes, hangs) which are
 *      nearly impossible to debug because the thread's stack space
 *      might get completely messed up.
 *
 *      The general idea of these macros is to define
 *      TRY / CATCH blocks similar to C++. If an exception
 *      occurs in the TRY block, execution is transferred
 *      to the CATCH block. (This works in both C and C++,
 *      by the way.)
 *
 *      In addition, with the TRY statement, you may specify
 *      an optional "OnKill" function which gets called if
 *      the thread gets killed while code in the TRY block is
 *      executed (new with V0.9.0). If you need no such
 *      function, specify NULL. Details follow.
 *
 *      The general usage is like this:
 *
 +          int your_protected_func(int ...)
 +          {
 +              TRY_LOUD(excptid, OnKillFunc)
 +              // or: TRY_QUIET(excptid, OnKillFunc)
 +              // OnKillFunc can be NULL
 +              {
 +                  ....        // the stuff in here is protected by
 +                              // the excHandlerLoud or excHandlerQuiet
 +                              // exception handler
 +              }
 +              CATCH(excptid)
 +              {
 +                  ....        // exception occured: react here
 +              } END_CATCH();  // always needed!
 +          } // end of your_func
 *
 *      TRY_LOUD  is for excHandlerLoud (see below).
 *      TRY_QUIET is for excHandlerQuiet (see below).
 *      CATCH / END_CATCH are the same for the two. This
 *      is where the exception handler jumps to if an
 *      exception occurs.
 *      The CATCH block is _required_ even if you do nothing
 *      in there.
 *
 *      "excptid" can be any identifier which is not used in
 *      your current variable scope, e.g. "excpt1". This
 *      is used for creating an EXCEPTSTRUCT variable of
 *      that name on the stack. The "excptid"'s in TRY_* and
 *      CATCH must match, since this is where the macros
 *      store the exception handler data.
 *
 *      It is possible to nest these handlers though. You
 *      must use different "excptid"'s only if more than one
 *      TRY_* block exists in the same variable scope. Avoid
 *      using gotos to jump between the different CATCH blocks,
 *      because this might not properly deregister the handlers.
 *
 *      Keep in mind that all the code in the TRY_* block is
 *      protected by the handler, including all functions that
 *      get called. So if you enclose your main() code in a
 *      TRY_* block, your entire application is protected.
 *
 *      Your "OnKillFunc" must be declared as
 +          VOID APIENTRY YourOnKillFunc(VOID) {...}.
 *      This gets called when the thread gets terminated
 *      while being executed in the TRY block (that is,
 *      if either the thread gets killed explicitly or
 *      the entire process ends for some reason). After
 *      that function got executed, the thread will _always_
 *      be terminated; the CATCH block will not be executed
 *      then. You can specify NULL for the "OnKillFunc", if
 *      you don't need one.
 *
 *      To summarize: If an exception occurs in the TRY
 *      block,
 *
 *      a) "OnKillFunc" gets called if the thread got
 *         killed, and the thread then terminates;
 *
 *      b) the CATCH block is executed for other exceptions
 *         and execution continues.
 *
 *      <B>Mutex semaphores</B>
 *
 *      Here's how to deal with mutex semaphores: WHENEVER
 *      you request a mutex semaphore, enclose the block
 *      with TRY/CATCH in case an error occurs, like this:
 *
 +          HMTX hmtx = ...
 +
 +          VOID APIENTRY OnKillYourFunc(VOID)
 +          {
 +              DosReleaseMutexSem(hmtx);
 +          }
 +
 +          int your_func(int)
 +          {
 +              BOOL fSemOwned = FALSE;
 +              TRY_QUIET(excpt1, OnKillYourFunc) // or TRY_LOUD
 +              {
 +                  fSemOwned = (WinRequestMutexSem(hmtx, ...) == NO_ERROR);
 +                  if (fSemOwned)
 +                  {       ... // work on your protected data
 +                  }
 +              }
 +              CATCH(excpt1) { } END_CATCH();    // always needed!
 +
 +              if (fSemOwned) {
 +                  // this gets executed always, even if an exception occured
 +                  DosReleaseMutexSem(hmtx);
 +                  fSemOwned = FALSE;
 +              }
 +          } // end of your_func
 *
 *      This way your mutex semaphore gets released even if
 *      exceptions occur in your code. If you don't do this,
 *      threads waiting for that semaphore will be blocked
 *      forever when exceptions occur. As a result, depending
 *      on the thread, PM will hang, or the application will
 *      never terminate.
 *
 *      <B>Customizing</B>
 *
 *      Note: As opposed to versions < 0.9.0, this is now
 *      completely independent of XWorkplace. This file now
 *      contains "pure" exception handlers only.
 *
 *      However, you can customize these exception handlers by
 *      calling excRegisterHooks. This is what XWorkplace does now.
 *      This should be done upon initialization of your application.
 *      If excRegisterHooks is not called, the following safe
 *      defaults are used:
 *          --  the trap log file is TRAP.LOG in the root
 *              directory of your boot drive.
 *
 *      For details on the provided exception handlers, refer
 *      to excHandlerLoud and excHandlerQuiet.
 *
 *      More useful debug information can be found in the "OS/2 Debugging
 *      Handbook", which is now available in INF format on the IBM
 *      DevCon site ("http://service2.boulder.ibm.com/devcon/").
 *      This book shows worked examples of how to unwind a stack dump.
 *
 *      This file incorporates code from the following:
 *      -- Monte Copeland, IBM Boca Ration, Florida, USA (1993)
 *      -- Roman Stangl, from the Program Commander/2 sources
 *         (1997-98)
 *      -- Marc Fiammante, John Currier, Kim Rasmussen,
 *         Anthony Cruise (EXCEPT3.ZIP package for a generic
 *         exception handling DLL, available at Hobbes).
 *
 *      If not explicitly stated otherwise, the code has been written
 *      by me, Ulrich M�ller.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@header "helpers\except.h"
 */

/*
 *      This file Copyright (C) 1992-99 Ulrich M�ller,
 *                                      Monte Copeland,
 *                                      Roman Stangl,
 *                                      Kim Rasmussen,
 *                                      Marc Fiammante,
 *                                      John Currier,
 *                                      Anthony Cruise.
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

#define INCL_DOSMODULEMGR
#define INCL_DOSEXCEPTIONS
#define INCL_DOSPROCESS
#define INCL_DOSERRORS
#include <os2.h>

// C library headers
#include <stdio.h>              // needed for except.h
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <setjmp.h>             // needed for except.h
#include <assert.h>             // needed for except.h

#define DONT_REPLACE_MALLOC
#include "setup.h"                      // code generation and debugging options

// headers in /helpers
#include "helpers\dosh.h"               // Control Program helper routines
#include "helpers\except.h"             // exception handling
#include "helpers\debug.h"              // symbol/debug code analysis

#pragma hdrstop

/* ******************************************************************
 *                                                                  *
 *   Global variables                                               *
 *                                                                  *
 ********************************************************************/

// hooks to be registered using excRegisterHooks
PFNEXCOPENFILE  G_pfnExcOpenFile = 0;
PFNEXCHOOK      G_pfnExcHook = 0;
PFNEXCHOOKERROR G_pfnExcHookError = 0;
// beep flag for excHandlerLoud
BOOL            G_fBeepOnException = TRUE;

/*
 *@@category: Helpers\Control program helpers\Exceptions/debugging
 */

/* ******************************************************************
 *                                                                  *
 *   Exception helper routines                                      *
 *                                                                  *
 ********************************************************************/

/*
 *@@ excDescribePage:
 *
 */

VOID excDescribePage(FILE *file, ULONG ulCheck)
{
    APIRET arc;
    ULONG ulCountPages = 1;
    ULONG ulFlagsPage = 0;
    arc = DosQueryMem((PVOID)ulCheck, &ulCountPages, &ulFlagsPage);

    if (arc == NO_ERROR)
    {
        fprintf(file, "valid, flags: ");
        if (ulFlagsPage & PAG_READ)
            fprintf(file, "read ");
        if (ulFlagsPage & PAG_WRITE)
            fprintf(file, "write ");
        if (ulFlagsPage & PAG_EXECUTE)
            fprintf(file, "execute ");
        if (ulFlagsPage & PAG_GUARD)
            fprintf(file, "guard ");
        if (ulFlagsPage & PAG_COMMIT)
            fprintf(file, "committed ");
        if (ulFlagsPage & PAG_SHARED)
            fprintf(file, "shared ");
        if (ulFlagsPage & PAG_FREE)
            fprintf(file, "free ");
        if (ulFlagsPage & PAG_BASE)
            fprintf(file, "base ");
    }
    else if (arc == ERROR_INVALID_ADDRESS)
        fprintf(file, "invalid");
}

/*
 *@@ excPrintStackFrame:
 *      wrapper for dbgPrintStackFrame to format
 *      output stuff right.
 *
 *@@added V0.9.2 (2000-03-10) [umoeller]
 */

VOID excPrintStackFrame(FILE *file,         // in: output log file
                        PSZ pszDescription, // in: description for stack frame (should be eight chars)
                        ULONG ulAddress)    // in: address to debug
{
    APIRET  arc = NO_ERROR;
    HMODULE hmod1 = NULLHANDLE;
    CHAR    szMod1[2*CCHMAXPATH] = "unknown";
    ULONG   ulObject = 0,
            ulOffset = 0;
    fprintf(file,
            "    %-8s: %08X ",
            pszDescription,
            ulAddress);
    arc = DosQueryModFromEIP(&hmod1,
                             &ulObject,
                             sizeof(szMod1), szMod1,
                             &ulOffset,
                             ulAddress);

    if (arc != NO_ERROR)
    {
        // error:
        fprintf(file,
                " %-8s:%d   Error: DosQueryModFromEIP returned %d\n",
                szMod1,
                ulObject,
                arc);
    }
    else
    {
        CHAR szFullName[2*CCHMAXPATH];

        fprintf(file,
                " %-8s:%d   ",
                szMod1,
                ulObject);

        DosQueryModuleName(hmod1, sizeof(szFullName), szFullName);

        dbgPrintStackFrame(file,
                           szFullName,
                           ulObject,
                           ulOffset);

        fprintf(file, "\n");

        // make a 'tick' sound to let the user know we're still alive
        DosBeep(2000, 10);
    }
}

/*
 *@@ excDumpStackFrames:
 *      called from excExplainException to dump the
 *      thread's stack frames. This calls excPrintStackFrame
 *      for each stack frame found.
 *
 *@@added V0.9.4 (2000-06-15) [umoeller]
 */

VOID excDumpStackFrames(FILE *file,                   // in: logfile from fopen()
                        PTIB ptib,
                        PCONTEXTRECORD pContextRec)   // in: excpt info
{
    PULONG pulStackWord = 0;

    fprintf(file, "\n\nStack frames:\n              Address   Module:Object\n");

    // first the trapping address itself
    excPrintStackFrame(file,
                       "CS:EIP  ",
                       pContextRec->ctx_RegEip);


    pulStackWord = (PULONG)pContextRec->ctx_RegEbp;
    /* if (pContextRec->ctx_RegEbp < pContextRec->ctx_RegEsp)
        pulStackWord = (PULONG)(pContextRec->ctx_RegEbp & 0xFFFFFFF0);
    else
        pulStackWord = (PULONG)(pContextRec->ctx_RegEsp & 0xFFFFFFF0); */

    while (    (pulStackWord != 0)
            && (pulStackWord < (PULONG)ptib->tib_pstacklimit)
          )
    {
        CHAR szAddress[20];

        if (((ULONG)pulStackWord & 0x00000FFF) == 0x00000000)
        {
            // we're on a page boundary: check access
            ULONG ulCountPages = 0x1000;
            ULONG ulFlagsPage = 0;
            APIRET arc = DosQueryMem((void *)pulStackWord,
                                     &ulCountPages,
                                     &ulFlagsPage);
            if (    (arc != NO_ERROR)
                 || (   (arc == NO_ERROR)
                      && ( !( ((ulFlagsPage & (PAG_COMMIT|PAG_READ))
                               == (PAG_COMMIT|PAG_READ)
                              )
                            )
                         )
                    )
               )
            {
                fprintf(file, "\n    %08X: ", pulStackWord);
                fprintf(file, "Page inaccessible");
                pulStackWord += 0x1000;
                continue; // for
            }
        }

        sprintf(szAddress, "%08X",
                pulStackWord);
        excPrintStackFrame(file,
                           szAddress,
                           *(pulStackWord+1));
        pulStackWord = (PULONG)*(pulStackWord);
    } // end while
}

/*
 *@@ excExplainException:
 *      used by the exception handlers below to write
 *      LOTS of information about the exception into a logfile.
 *
 *      This calls excPrintStackFrame for each stack frame.
 *
 *      If DEBUG_EXCPT_STACKDUMP is #define'd, this also dumps
 *      the stack completely, which isn't that useful.
 *
 *@@changed V0.9.0 [umoeller]: added support for application hook
 *@@changed V0.9.0 (99-11-02) [umoeller]: added TID to dump
 *@@changed V0.9.2 (2000-03-10) [umoeller]: now using excPrintStackFrame
 *@@changed V0.9.3 (2000-05-03) [umoeller]: fixed crashes
 */

VOID excExplainException(FILE *file,                   // in: logfile from fopen()
                         PSZ pszHandlerName,           // in: descriptive string
                         PEXCEPTIONREPORTRECORD pReportRec, // in: excpt info
                         PCONTEXTRECORD pContextRec)   // in: excpt info
{
    PTIB        ptib = NULL;
    PPIB        ppib = NULL;
    HMODULE     hMod1, hMod2;
    CHAR        szMod1[CCHMAXPATH] = "unknown",
                szMod2[CCHMAXPATH] = "unknown";
    ULONG       ulObjNum,
                ulOffset;
                /* ulCountPages,
                ulFlagsPage; */
    // APIRET      arc;
    // PULONG      pulStackWord;
                // pulStackBegin;
    ULONG       ul;

    ULONG       ulOldPriority = 0x0100; // regular, delta 0

    // raise this thread's priority, because this
    // might take some time
    if (DosGetInfoBlocks(&ptib, &ppib) == NO_ERROR)
        if (ptib)
            if (ptib->tib_ptib2)
            {
                ulOldPriority = ptib->tib_ptib2->tib2_ulpri;
                DosSetPriority(PRTYS_THREAD,
                               PRTYC_REGULAR,
                               PRTYD_MAXIMUM,
                               0);     // current thread
            }

    // make some noise
    if (G_fBeepOnException)
    {
        DosBeep( 250, 30);
        DosBeep( 500, 30);
        DosBeep(1000, 30);
        DosBeep(2000, 30);
        DosBeep(4000, 30);
        DosBeep(2000, 30);
        DosBeep(1000, 30);
        DosBeep( 500, 30);
        DosBeep( 250, 30);
    }

    // generic exception info
    fprintf(file,
            "\n%s:\n    Exception type: %08X\n    Address:        %08X\n    Params:         ",
            pszHandlerName,
            pReportRec->ExceptionNum,
            pReportRec->ExceptionAddress);
    for (ul = 0;  ul < pReportRec->cParameters;  ul++)
    {
        fprintf(file, "%08X  ",
                pReportRec->ExceptionInfo[ul]);
    }

    // now explain the exception in a bit more detail;
    // depending on the exception, pReportRec->ExceptionInfo
    // contains some useful data
    switch (pReportRec->ExceptionNum)
    {
        case XCPT_ACCESS_VIOLATION:
        {
            fprintf(file, "\nAccess violation: ");
            if (pReportRec->ExceptionInfo[0] & XCPT_READ_ACCESS)
                fprintf(file, "Invalid read access from 0x%04X:%08X.\n",
                        pContextRec->ctx_SegDs, pReportRec->ExceptionInfo[1]);
            else if (pReportRec->ExceptionInfo[0] & XCPT_WRITE_ACCESS)
                fprintf(file, "Invalid write access to 0x%04X:%08X.\n",
                        pContextRec->ctx_SegDs, pReportRec->ExceptionInfo[1]);
            else if (pReportRec->ExceptionInfo[0] & XCPT_SPACE_ACCESS)
                fprintf(file, "Invalid space access at 0x%04X.\n",
                        pReportRec->ExceptionInfo[1]);
            else if (pReportRec->ExceptionInfo[0] & XCPT_LIMIT_ACCESS)
                fprintf(file, "Invalid limit access occurred.\n");
            else if (pReportRec->ExceptionInfo[0] == XCPT_UNKNOWN_ACCESS)
                fprintf(file, "unknown at 0x%04X:%08X\n",
                            pContextRec->ctx_SegDs, pReportRec->ExceptionInfo[1]);
            fprintf(file,
                    "Explanation: An attempt was made to access a memory object which does\n"
                    "             not belong to the current process. Most probable causes\n"
                    "             for this are that an invalid pointer was used, there was\n"
                    "             confusion with administering memory or error conditions \n"
                    "             were not properly checked for.\n");
            break;
        }

        case XCPT_INTEGER_DIVIDE_BY_ZERO:
        {
            fprintf(file, "\nInteger division by zero.\n");
            fprintf(file,
                    "Explanation: An attempt was made to divide an integer value by zero,\n"
                    "             which is not defined.\n");
            break;
        }

        case XCPT_ILLEGAL_INSTRUCTION:
        {
            fprintf(file, "\nIllegal instruction found.\n");
            fprintf(file,
                    "Explanation: An attempt was made to execute an instruction that\n"
                    "             is not defined on this machine's architecture.\n");
            break;
        }

        case XCPT_PRIVILEGED_INSTRUCTION:
        {
            fprintf(file, "\nPrivileged instruction found.\n");
            fprintf(file,
                    "Explanation: An attempt was made to execute an instruction that\n"
                    "             is not permitted in the current machine mode or that\n"
                    "             XFolder had no permission to execute.\n");
            break;
        }

        case XCPT_INTEGER_OVERFLOW:
            fprintf(file, "\nInteger overflow.\n");
            fprintf(file,
                    "Explanation: An integer operation generated a carry-out of the most\n"
                    "             significant bit. This is a sign of an attempt to store\n"
                    "             a value which does not fit into an integer variable.\n");
    }

    if (DosGetInfoBlocks(&ptib, &ppib) == NO_ERROR)
    {
        /*
         * process info:
         *
         */

        if ((ptib) && (ppib))       // (99-11-01) [umoeller]
        {
            if (pContextRec->ContextFlags & CONTEXT_CONTROL)
            {
                // get the main module
                hMod1 = ppib->pib_hmte;
                DosQueryModuleName(hMod1,
                                   sizeof(szMod1),
                                   szMod1);

                // get the trapping module
                DosQueryModFromEIP(&hMod2,
                                   &ulObjNum,
                                   sizeof(szMod2),
                                   szMod2,
                                   &ulOffset,
                                   pContextRec->ctx_RegEip);
                DosQueryModuleName(hMod2,
                                   sizeof(szMod2),
                                   szMod2);
            }

            fprintf(file,
                    "\nProcess information:"
                    "\n    Process ID:      0x%lX"
                    "\n    Process module:  0x%lX (%s)"
                    "\n    Trapping module: 0x%lX (%s)"
                    "\n    Object: %d\n",
                    ppib->pib_ulpid,
                    hMod1, szMod1,
                    hMod2, szMod2,
                    ulObjNum);

            fprintf(file,
                    "\nTrapping thread information:"
                    "\n    Thread ID:       0x%lX (%d)"
                    "\n    Priority:        0x%lX\n",
                    ptib->tib_ptib2->tib2_ultid, ptib->tib_ptib2->tib2_ultid,
                    ulOldPriority);
        }
        else
            fprintf(file, "\nProcess information was not available.");

        /*
         *  now call the hook, if one has been defined,
         *  so that the application can write additional
         *  information to the traplog (V0.9.0)
         */

        if (G_pfnExcHook)
        {
            (*G_pfnExcHook)(file, ptib);
        }

        // *** registers

        fprintf(file, "\nRegisters:");
        if (pContextRec->ContextFlags & CONTEXT_INTEGER)
        {
            // EAX
            fprintf(file, "\n    EAX = %08X  ", pContextRec->ctx_RegEax);
            excDescribePage(file, pContextRec->ctx_RegEax);
            // EBX
            fprintf(file, "\n    EBX = %08X  ", pContextRec->ctx_RegEbx);
            excDescribePage(file, pContextRec->ctx_RegEbx);
            // ECX
            fprintf(file, "\n    ECX = %08X  ", pContextRec->ctx_RegEcx);
            excDescribePage(file, pContextRec->ctx_RegEcx);
            // EDX
            fprintf(file, "\n    EDX = %08X  ", pContextRec->ctx_RegEdx);
            excDescribePage(file, pContextRec->ctx_RegEdx);
            // ESI
            fprintf(file, "\n    ESI = %08X  ", pContextRec->ctx_RegEsi);
            excDescribePage(file, pContextRec->ctx_RegEsi);
            // EDI
            fprintf(file, "\n    EDI = %08X  ", pContextRec->ctx_RegEdi);
            excDescribePage(file, pContextRec->ctx_RegEdi);
            fprintf(file, "\n");
        }
        else
            fprintf(file, " not available\n");

        if (pContextRec->ContextFlags & CONTEXT_CONTROL)
        {

            // *** instruction

            fprintf(file, "Instruction pointer (where exception occured):\n    CS:EIP = %04X:%08X  ",
                    pContextRec->ctx_SegCs,
                    pContextRec->ctx_RegEip);
            excDescribePage(file, pContextRec->ctx_RegEip);

            // *** CPU flags

            fprintf(file, "\n    FLG    = %08X", pContextRec->ctx_EFlags);

            /*
             * stack:
             *
             */

            fprintf(file, "\nStack:\n    Base:         %08X\n    Limit:        %08X",
                   (ptib ? ptib->tib_pstack : 0),
                   (ptib ? ptib->tib_pstacklimit :0));
            fprintf(file, "\n    SS:ESP = %04X:%08X  ",
                    pContextRec->ctx_SegSs, pContextRec->ctx_RegEsp);
            excDescribePage(file, pContextRec->ctx_RegEsp);

            fprintf(file, "\n    EBP    =      %08X  ", pContextRec->ctx_RegEbp);
            excDescribePage(file, pContextRec->ctx_RegEbp);

            /*
             * stack dump:
             */

            if (ptib != 0)
                excDumpStackFrames(file, ptib, pContextRec);
        }
    }
    fprintf(file, "\n");

    // reset old priority
    DosSetPriority(PRTYS_THREAD,
                   (ulOldPriority & 0x0F00) >> 8,
                   (UCHAR)ulOldPriority,
                   0);     // current thread
}

/* ******************************************************************
 *                                                                  *
 *   Exported routines                                              *
 *                                                                  *
 ********************************************************************/

/*
 *@@ excRegisterHooks:
 *      this registers hooks which get called for
 *      exception handlers. You can set any of the
 *      hooks to NULL for safe defaults (see top of
 *      except.c for details). You can set none,
 *      one, or both of the hooks, and you can call
 *      this function several times.
 *
 *      Both hooks get called whenever an exception
 *      occurs, so there better be no bugs in these
 *      routines. ;-) They only get called from
 *      within excHandlerLoud (because excHandlerQuiet
 *      writes no trap logs).
 *
 *      The hooks are as follows:
 *
 *      pfnExcOpenFileNew gets called to open
 *          the trap log file. This must return a FILE*
 *          pointer from fopen(). If this is not defined,
 *          ?:\TRAP.LOG is used. Use this to specify a
 *          different file and have some notes written
 *          into it before the actual exception info.
 *
 *      pfnExcHookNew gets called while the trap log
 *          is being written. At this point,
 *          the following info has been written into
 *          the trap log already:
 *          -- exception type/address block
 *          -- exception explanation
 *          -- process information
 *
 *          _After_ the hook, the exception handler
 *          continues with the "Registers" information
 *          and stack dump/analysis.
 *
 *          Use this hook to write additional application
 *          info into the trap log, such as the state
 *          of your threads.
 *
 *      pfnExcHookError gets called when the TRY_* macros
 *          fail to install an exception handler (when
 *          DosSetExceptionHandler fails).
 *
 *@@added V0.9.0 [umoeller]
 *@@changed V0.9.2 (2000-03-10) [umoeller]: pfnExcHookError added
 */

VOID excRegisterHooks(PFNEXCOPENFILE pfnExcOpenFileNew,
                      PFNEXCHOOK pfnExcHookNew,
                      PFNEXCHOOKERROR pfnExcHookError,
                      BOOL fBeepOnExceptionNew)
{
    // adjust the global variables
    G_pfnExcOpenFile = pfnExcOpenFileNew;
    G_pfnExcHook = pfnExcHookNew;
    G_pfnExcHookError = pfnExcHookError;
    G_fBeepOnException = fBeepOnExceptionNew;
}

/*
 *@@ excHandlerLoud:
 *      this is the "sophisticated" exception handler;
 *      which gives forth a loud sequence of beeps thru the
 *      speaker, writes a trap log and then returns back
 *      to the thread to continue execution, i.e. the
 *      default OS/2 exception handler will never get
 *      called. This requires a setjmp() call on
 *      EXCEPTIONREGISTRATIONRECORD2.jmpThread before
 *      being installed.
 *
 *      This is best registered thru the TRY_LOUD macro
 *      (new with V0.84, described in except.c), which
 *      does the necessary setup.
 *
 *      Depending on the type of exception, the following
 *      happens:
 *
 *      <B>a) "real" exceptions</B>
 *
 *      --  XCPT_ACCESS_VIOLATION         (traps 0x0d, 0x0e)
 *      --  XCPT_INTEGER_DIVIDE_BY_ZERO   (trap 0)
 *      --  XCPT_ILLEGAL_INSTRUCTION      (trap 6)
 *      --  XCPT_PRIVILEGED_INSTRUCTION
 *      --  XCPT_INTEGER_OVERFLOW         (trap 4)
 *
 *      For these exceptions, we call the functions in debug.c
 *      to try to find debug code or SYM file information about
 *      what source code corresponds to the error.
 *
 *      Note that to get meaningful debugging information
 *      in this handler's traplog, you need the following:
 *      a)  have a MAP file created at link time (/MAP)
 *      b)  convert the MAP to a SYM file using MAPSYM
 *      c)  put the SYM file in the same directory of
 *          the executable. This must have the same
 *          filestem as the executable.
 *
 *      See the "Control Programming Guide and Reference"
 *      for details.
 *      All other exceptions are passed to the next handler
 *      in the exception handler chain. This might be the
 *      C/C++ compiler handler or the default OS/2 handler,
 *      which will probably terminate the process.
 *
 *      <B>b) thread kills</B>
 *
 *      -- XCPT_PROCESS_TERMINATE
 *      -- XCPT_ASYNC_PROCESS_TERMINATE:
 *
 *      If EXCEPTIONREGISTRATIONRECORD2.pfnOnKill is != NULL,
 *      that function gets called to allow for thread cleanup
 *      before the thread really terminates. This should be
 *      used for releasing mutex semaphores.
 *
 *@@changed V0.9.0 [umoeller]: added support for thread termination
 *@@changed V0.9.2 (2000-03-10) [umoeller]: switched date format to ISO
 */

ULONG _System excHandlerLoud(PEXCEPTIONREPORTRECORD pReportRec,
                             PEXCEPTIONREGISTRATIONRECORD2 pRegRec2,
                             PCONTEXTRECORD pContextRec,
                             PVOID pv)
{
    /* From the VAC++3 docs:
     *     "The first thing an exception handler should do is check the
     *     exception flags. If EH_EXIT_UNWIND is set, meaning
     *     the thread is ending, the handler tells the operating system
     *     to pass the exception to the next exception handler. It does the
     *     same if the EH_UNWINDING flag is set, the flag that indicates
     *     this exception handler is being removed.
     *     The EH_NESTED_CALL flag indicates whether the exception
     *     occurred within an exception handler. If the handler does
     *     not check this flag, recursive exceptions could occur until
     *     there is no stack remaining."
     * So for all these conditions, we exit immediately.
     */

    if (pReportRec->fHandlerFlags & EH_EXIT_UNWIND)
       return (XCPT_CONTINUE_SEARCH);
    if (pReportRec->fHandlerFlags & EH_UNWINDING)
       return (XCPT_CONTINUE_SEARCH);
    if (pReportRec->fHandlerFlags & EH_NESTED_CALL)
       return (XCPT_CONTINUE_SEARCH);

    switch (pReportRec->ExceptionNum)
    {
        case XCPT_PROCESS_TERMINATE:
        case XCPT_ASYNC_PROCESS_TERMINATE:
            // thread terminated:
            // if the handler has been registered to catch
            // these exceptions, continue;
            if (pRegRec2->pfnOnKill)
                // call the "OnKill" function
                pRegRec2->pfnOnKill();
            // get outta here, which will kill the thread
        break;

        case XCPT_ACCESS_VIOLATION:
        case XCPT_INTEGER_DIVIDE_BY_ZERO:
        case XCPT_ILLEGAL_INSTRUCTION:
        case XCPT_PRIVILEGED_INSTRUCTION:
        case XCPT_INTEGER_OVERFLOW:
        {
            // "real" exceptions:
            FILE *file;

            // open traplog file;
            if (G_pfnExcOpenFile)
                // hook defined for this: call it
                file = (*G_pfnExcOpenFile)();
            else
            {
                CHAR szFileName[100];
                // no hook defined: open some
                // default traplog file in root directory of
                // boot drive
                sprintf(szFileName, "%c:\\trap.log", doshQueryBootDrive());
                file = fopen(szFileName, "a");

                if (file)
                {
                    DATETIME DT;
                    DosGetDateTime(&DT);
                    fprintf(file,
                            "\nTrap message -- Date: %04d-%02d-%02d, Time: %02d:%02d:%02d\n",
                            DT.year, DT.month, DT.day,
                            DT.hours, DT.minutes, DT.seconds);
                    fprintf(file, "------------------------------------------------\n");

                }
            }

            // write error log
            excExplainException(file,
                                "excHandlerLoud",
                                pReportRec,
                                pContextRec);
            fclose(file);

            // jump back to failing routine
            /* DosSetPriority(PRTYS_THREAD,
                           PRTYC_REGULAR,
                           0,       // delta
                           0);      // current thread
                           */
            longjmp(pRegRec2->jmpThread, pReportRec->ExceptionNum);
        break; }
    }

    // not handled
    return (XCPT_CONTINUE_SEARCH);
}

/*
 *@@ excHandlerQuiet:
 *      "quiet" xcpt handler, which simply suppresses exceptions;
 *      this is useful for certain error-prone functions, where
 *      exceptions are likely to appear, for example used by
 *      cmnCheckObject to implement a fail-safe SOM object check.
 *
 *      This does _not_ write an error log and makes _no_ sound.
 *      This simply jumps back to the trapping thread or
 *      calls EXCEPTIONREGISTRATIONRECORD2.pfnOnKill.
 *
 *      Other than that, this behaves like excHandlerLoud.
 *
 *      This is best registered thru the TRY_QUIET macro
 *      (new with V0.84, described in except.c), which
 *      does the necessary setup.
 *
 *@@changed V0.9.0 [umoeller]: added support for thread termination
 */

ULONG _System excHandlerQuiet(PEXCEPTIONREPORTRECORD pReportRec,
                              PEXCEPTIONREGISTRATIONRECORD2 pRegRec2,
                              PCONTEXTRECORD pContextRec,
                              PVOID pv)
{
    if (pReportRec->fHandlerFlags & EH_EXIT_UNWIND)
       return (XCPT_CONTINUE_SEARCH);
    if (pReportRec->fHandlerFlags & EH_UNWINDING)
       return (XCPT_CONTINUE_SEARCH);
    if (pReportRec->fHandlerFlags & EH_NESTED_CALL)
       return (XCPT_CONTINUE_SEARCH);

    switch (pReportRec->ExceptionNum)
    {
        case XCPT_PROCESS_TERMINATE:
        case XCPT_ASYNC_PROCESS_TERMINATE:
            // thread terminated:
            // if the handler has been registered to catch
            // these exceptions, continue;
            if (pRegRec2->pfnOnKill)
                // call the "OnKill" function
                pRegRec2->pfnOnKill();
            // get outta here, which will kill the thread
        break;

        case XCPT_ACCESS_VIOLATION:
        case XCPT_INTEGER_DIVIDE_BY_ZERO:
        case XCPT_ILLEGAL_INSTRUCTION:
        case XCPT_PRIVILEGED_INSTRUCTION:
        case XCPT_INTEGER_OVERFLOW:
            // write excpt explanation only if the
            // resp. debugging #define is set (common.h)
            #ifdef DEBUG_WRITEQUIETEXCPT
            {
                FILE *file = excOpenTraplogFile();
                excExplainException(file,
                                    "excHandlerQuiet",
                                    pReportRec,
                                    pContextRec);
                fclose(file);
            }
            #endif

            // jump back to failing routine
            longjmp(pRegRec2->jmpThread, pReportRec->ExceptionNum);
        break;

        default:
             break;
    }

    return (XCPT_CONTINUE_SEARCH);
}

