
/*
 *@@sourcefile memdebug.h:
 *      header file for memdebug.c.
 *      See remarks there.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@include #include <os2.h>
 *@@include #include "memdebug.h"
 */

/*      Copyright (C) 2000 Ulrich M”ller.
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

#if __cplusplus
extern "C" {
#endif

#ifndef MEMDEBUG_HEADER_INCLUDED
    #define MEMDEBUG_HEADER_INCLUDED

    #ifndef __stdlib_h           // <stdlib.h>
        #error stdlib.h must be included before memdebug.h.
    #endif

    typedef void (FNCBMEMDLOG)(const char*);        // message
    typedef FNCBMEMDLOG *PFNCBMEMDLOG;

    // global variable for memory error logger func
    extern PFNCBMEMDLOG    G_pMemdLogFunc;

    void* memdMalloc(size_t stSize,
                     const char *pcszSourceFile,
                     unsigned long ulLine,
                     const char *pcszFunction);

    void* memdCalloc(size_t num,
                     size_t stSize,
                     const char *pcszSourceFile,
                     unsigned long ulLine,
                     const char *pcszFunction);

    void memdFree(void *p,
                  const char *pcszSourceFile,
                  unsigned long ulLine,
                  const char *pcszFunction);

    unsigned long memdReleaseFreed(void);

    #ifdef __XWPMEMDEBUG__

        #ifndef DONT_REPLACE_MALLOC
            #define malloc(ul) memdMalloc(ul, __FILE__, __LINE__, __FUNCTION__)
            #define calloc(n, size) memdCalloc(n, size, __FILE__, __LINE__, __FUNCTION__)
            #define free(p) memdFree(p, __FILE__, __LINE__, __FUNCTION__)

            #ifdef __string_h
                // string.h included and debugging is on:
                // redefine strdup to use memory debugging
                #define strdup(psz)                                 \
                    strcpy( (char*)memdMalloc(strlen(psz) + 1, __FILE__, __LINE__, __FUNCTION__), psz)
                    // the original crashes also if psz is NULL
            #endif

        #endif
    #endif

    #ifdef PM_INCLUDED
        /********************************************************************
         *                                                                  *
         *   XFolder debugging helpers                                      *
         *                                                                  *
         ********************************************************************/

        #ifdef _PMPRINTF_
            void memdDumpMemoryBlock(PBYTE pb,
                                     ULONG ulSize,
                                     ULONG ulIndent);
        #else
            // _PMPRINTF not #define'd: do nothing
            #define memdDumpMemoryBlock(pb, ulSize, ulIndent)
        #endif

        /* ******************************************************************
         *                                                                  *
         *   Heap debugging window                                          *
         *                                                                  *
         ********************************************************************/

        MRESULT EXPENTRY memd_fnwpMemDebug(HWND hwndClient, ULONG msg, MPARAM mp1, MPARAM mp2);

        VOID memdCreateMemDebugWindow(VOID);
    #endif

#endif

#if __cplusplus
}
#endif

