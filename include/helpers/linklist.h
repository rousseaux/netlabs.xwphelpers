/* $Id$ */


/*
 *@@sourcefile linklist.h:
 *      header file for linklist.c. See remarks there.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@include #include "linklist.h"
 */

/*      Copyright (C) 1997-2000 Ulrich M”ller.
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

#ifndef LINKLIST_HEADER_INCLUDED
    #define LINKLIST_HEADER_INCLUDED

    // define some basic things to make this work even with standard C
    #if (!defined OS2_INCLUDED) && (!defined _OS2_H) && (!defined __SIMPLES_DEFINED)   // changed V0.9.0 (99-10-22) [umoeller]
        typedef unsigned long BOOL;
        typedef unsigned long ULONG;
        typedef unsigned char *PSZ;
        #define TRUE (BOOL)1
        #define FALSE (BOOL)0

        #ifdef __IBMCPP__               // added V0.9.0 (99-10-22) [umoeller]
            #define APIENTRY _System
        #endif

        #define __SIMPLES_DEFINED
    #endif

    /* LISTITEM was used before V0.9.0 */
    /* typedef struct _LISTITEM {
        struct LIST_STRUCT   *pNext, *pPrevious;
        unsigned long        ulSize;
    } LISTITEM, *PLISTITEM; */

    /*
     *@@ LISTNODE:
     *      new with V0.9.0. This now defines
     *      one item in the list.
     *      pItemData points to the actual data.
     *
     *@@added V0.9.0
     */

    typedef struct _LISTNODE
    {
        void                *pItemData;
        struct _LISTNODE    *pNext,
                            *pPrevious;
    } LISTNODE, *PLISTNODE;

    /*
     *@@ LINKLIST:
     *      new with V0.9.0. This now defines
     *      the "root" of a linked list.
     *
     *@@added V0.9.0
     */

    typedef struct _LINKLIST
    {
        unsigned long   ulMagic;         // integrity check
        unsigned long   ulCount;         // no. of items on list
        PLISTNODE       pFirst,          // first node
                        pLast;           // last node
        BOOL            fItemsFreeable; // as in lstCreate()
    } LINKLIST, *PLINKLIST;

    #define LINKLISTMAGIC 0xf124        // could be anything

    typedef signed short _System FNSORTLIST(void*, void*, void*);
                            // changed V0.9.0 (99-10-22) [umoeller]
    typedef FNSORTLIST *PFNSORTLIST;

    /* ******************************************************************
     *                                                                  *
     *   List base functions                                            *
     *                                                                  *
     ********************************************************************/

    void lstInit(PLINKLIST pList, BOOL fItemsFreeable);

    #ifdef __XWPMEMDEBUG__ // setup.h, helpers\memdebug.c
        PLINKLIST lstCreateDebug(BOOL fItemsFreeable,
                                 const char *file,
                                 unsigned long line,
                                 const char *function);

        #define lstCreate(b) lstCreateDebug((b), __FILE__, __LINE__, __FUNCTION__)
    #else
        PLINKLIST lstCreate(BOOL fItemsFreeable);
    #endif

    BOOL lstFree(PLINKLIST pList);

    BOOL lstClear(PLINKLIST pList);

    long lstCountItems(PLINKLIST pList);

    PLISTNODE lstQueryFirstNode(PLINKLIST pList);

    PLISTNODE lstNodeFromIndex(PLINKLIST pList, unsigned long ulIndex);

    PLISTNODE lstNodeFromItem(PLINKLIST pList, void* pItemData);

    void* lstItemFromIndex(PLINKLIST pList, unsigned long ulIndex);

    #ifdef __XWPMEMDEBUG__ // setup.h, helpers\memdebug.c
        PLISTNODE lstAppendItemDebug(PLINKLIST pList,
                                     void* pNewItemData,
                                     const char *file,
                                     unsigned long line,
                                     const char *function);
        #define lstAppendItem(pl, pd) lstAppendItemDebug((pl), (pd), __FILE__, __LINE__, __FUNCTION__)
    #else
        PLISTNODE lstAppendItem(PLINKLIST pList, void* pNewItemData);
    #endif

    PLISTNODE lstInsertItemBefore(PLINKLIST pList,
                                  void* pNewItemData,
                                  unsigned long ulIndex);

    BOOL lstRemoveNode(PLINKLIST pList, PLISTNODE pRemoveNode);

    BOOL lstRemoveItem(PLINKLIST pList, void* pRemoveItem);

    BOOL lstSwapNodes(PLISTNODE pNode1,
                      PLISTNODE pNode2);

    /* ******************************************************************
     *                                                                  *
     *   List sorting                                                   *
     *                                                                  *
     ********************************************************************/

    BOOL lstQuickSort(PLINKLIST pList,
                      PFNSORTLIST pfnSort,
                      void* pStorage);

    BOOL lstBubbleSort(PLINKLIST pList,
                       PFNSORTLIST pfnSort,
                       void* pStorage);

#endif

#if __cplusplus
}
#endif

