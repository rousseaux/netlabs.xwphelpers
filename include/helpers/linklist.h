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

#ifndef LINKLIST_HEADER_INCLUDED
    #define LINKLIST_HEADER_INCLUDED

    #ifndef XWPENTRY
        #error You must define XWPENTRY to contain the standard linkage for the XWPHelpers.
    #endif

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
     *
     *   List base functions
     *
     ********************************************************************/

    void* XWPENTRY lstMalloc(size_t size);
    typedef void* XWPENTRY LSTMALLOC(size_t size);
    typedef LSTMALLOC *PLSTMALLOC;

    void* XWPENTRY lstStrDup(const char *pcsz);
    typedef void* XWPENTRY LSTSTRDUP(const char *pcsz);
    typedef LSTSTRDUP *PLSTSTRDUP;

    void XWPENTRY lstInit(PLINKLIST pList, BOOL fItemsFreeable);
    typedef void XWPENTRY LSTINIT(PLINKLIST pList, BOOL fItemsFreeable);
    typedef LSTINIT *PLSTINIT;

    #ifdef __XWPMEMDEBUG__ // setup.h, helpers\memdebug.c
        PLINKLIST XWPENTRY lstCreateDebug(BOOL fItemsFreeable,
                                          const char *file,
                                          unsigned long line,
                                          const char *function);
        typedef PLINKLIST XWPENTRY LSTCREATEDEBUG(BOOL fItemsFreeable,
                                          const char *file,
                                          unsigned long line,
                                          const char *function);
        typedef LSTCREATEDEBUG *PLSTCREATEDEBUG;

        #define lstCreate(b) lstCreateDebug((b), __FILE__, __LINE__, __FUNCTION__)
    #else
        PLINKLIST XWPENTRY lstCreate(BOOL fItemsFreeable);
        typedef PLINKLIST XWPENTRY LSTCREATE(BOOL fItemsFreeable);
        typedef LSTCREATE *PLSTCREATE;
    #endif

    BOOL XWPENTRY lstFree(PLINKLIST pList);
    typedef BOOL XWPENTRY LSTFREE(PLINKLIST pList);
    typedef LSTFREE *PLSTFREE;

    BOOL XWPENTRY lstClear(PLINKLIST pList);
    typedef BOOL XWPENTRY LSTCLEAR(PLINKLIST pList);
    typedef LSTCLEAR *PLSTCLEAR;

    long XWPENTRY lstCountItems(PLINKLIST pList);
    typedef long XWPENTRY LSTCOUNTITEMS(PLINKLIST pList);
    typedef LSTCOUNTITEMS *PLSTCOUNTITEMS;

    PLISTNODE XWPENTRY lstQueryFirstNode(PLINKLIST pList);
    typedef PLISTNODE XWPENTRY LSTQUERYFIRSTNODE(PLINKLIST pList);
    typedef LSTQUERYFIRSTNODE *PLSTQUERYFIRSTNODE;

    PLISTNODE XWPENTRY lstNodeFromIndex(PLINKLIST pList, unsigned long ulIndex);
    typedef PLISTNODE XWPENTRY LSTNODEFROMINDEX(PLINKLIST pList, unsigned long ulIndex);
    typedef LSTNODEFROMINDEX *PLSTNODEFROMINDEX;

    PLISTNODE XWPENTRY lstNodeFromItem(PLINKLIST pList, void* pItemData);
    typedef PLISTNODE XWPENTRY LSTNODEFROMITEM(PLINKLIST pList, void* pItemData);
    typedef LSTNODEFROMITEM *PLSTNODEFROMITEM;

    void* XWPENTRY lstItemFromIndex(PLINKLIST pList, unsigned long ulIndex);
    typedef void* XWPENTRY LSTITEMFROMINDEX(PLINKLIST pList, unsigned long ulIndex);
    typedef LSTITEMFROMINDEX *PLSTITEMFROMINDEX;

    #ifdef __XWPMEMDEBUG__ // setup.h, helpers\memdebug.c
        PLISTNODE XWPENTRY lstAppendItemDebug(PLINKLIST pList,
                                              void* pNewItemData,
                                              const char *file,
                                              unsigned long line,
                                              const char *function);
        #define lstAppendItem(pl, pd) lstAppendItemDebug((pl), (pd), __FILE__, __LINE__, __FUNCTION__)
    #else
        PLISTNODE XWPENTRY lstAppendItem(PLINKLIST pList, void* pNewItemData);
        typedef PLISTNODE XWPENTRY LSTAPPENDITEM(PLINKLIST pList, void* pNewItemData);
        typedef LSTAPPENDITEM *PLSTAPPENDITEM;
    #endif

    PLISTNODE XWPENTRY lstInsertItemBefore(PLINKLIST pList,
                                           void* pNewItemData,
                                           unsigned long ulIndex);
    typedef PLISTNODE XWPENTRY LSTINSERTITEMBEFORE(PLINKLIST pList,
                                           void* pNewItemData,
                                           unsigned long ulIndex);
    typedef LSTINSERTITEMBEFORE *PLSTINSERTITEMBEFORE;

    BOOL XWPENTRY lstRemoveNode(PLINKLIST pList, PLISTNODE pRemoveNode);
    typedef BOOL XWPENTRY LSTREMOVENODE(PLINKLIST pList, PLISTNODE pRemoveNode);
    typedef LSTREMOVENODE *PLSTREMOVENODE;

    BOOL XWPENTRY lstRemoveItem(PLINKLIST pList, void* pRemoveItem);
    typedef BOOL XWPENTRY LSTREMOVEITEM(PLINKLIST pList, void* pRemoveItem);
    typedef LSTREMOVEITEM *PLSTREMOVEITEM;

    BOOL XWPENTRY lstSwapNodes(PLISTNODE pNode1, PLISTNODE pNode2);
    typedef BOOL XWPENTRY LSTSWAPNODES(PLISTNODE pNode1, PLISTNODE pNode2);
    typedef LSTSWAPNODES *PLSTSWAPNODES;

    /* ******************************************************************
     *
     *   List sorting
     *
     ********************************************************************/

    BOOL XWPENTRY lstQuickSort(PLINKLIST pList,
                               PFNSORTLIST pfnSort,
                               void* pStorage);

    BOOL XWPENTRY lstBubbleSort(PLINKLIST pList,
                                PFNSORTLIST pfnSort,
                                void* pStorage);

#endif

#if __cplusplus
}
#endif

