
/*
 *@@sourcefile linklist.c:
 *      contains helper functions for maintaining doubly-linked lists.
 *      See explanations below.
 *
 *      This file is new with V0.81 and contains all the functions
 *      that were in helpers.c previously.
 *
 *      Usage: All C programs; not OS/2-specific.
 *
 *      Function prefixes (new with V0.81):
 *      --  lst*    linked list helper functions
 *
 *      <B>Usage:</B>
 *
 *      Each list item is stored in a LISTNODE structure, which in
 *      turn has a pItemData field, which you can use to get your
 *      data. So a typical list would be coded like this:
 *
 +          // fill the list
 +          PLINKLIST pll = lstCreate(TRUE or FALSE);
 +          while ...
 +          {
 +              PYOURDATA pYourData = ...
 +              lstAppendItem(pll, pYourData);  // store the data in a new node
 +          }
 +
 +          ...
 +
 +          // now iterate over the list
 +          PLISTNODE pNode = lstQueryFirstNode(pll);
 +          while (pNode)
 +          {
 +              PYOURDATA pYourData = (PYOURDATA)pNode->pItemData;
 +              ...
 +              pNode = pNode->pNext;
 +          }
 +
 +          lstFree(pll);   // this is free the list items too, if TRUE had
 +                          // been specified with lstCreate
 *
 *      If __XWPMEMDEBUG__ is defined, the lstCreate and lstAppendItem funcs will
 *      automatically be replaced with lstCreateDebug and lstAppendItemDebug,
 *      and mapper macros are defined in linklist.h.
 *
 *      Note: If you frequently need to search your data structures,
 *      you might prefer the new balanced binary trees in tree.c which
 *      have been added with V0.9.5.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@header "helpers\linklist.h"
 */

/*
 *      Copyright (C) 1997-2000 Ulrich M”ller.
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

#include <stdlib.h>
#include <string.h>

#include "setup.h"                      // code generation and debugging options

#include "helpers\linklist.h"

#pragma hdrstop

/*
 *@@category: Helpers\C helpers\Linked lists
 */

/* ******************************************************************
 *                                                                  *
 *   List base functions                                            *
 *                                                                  *
 ********************************************************************/

/*
 *@@ lstInit:
 *      this initializes a given linked list.
 *      This is useful only if you have a static
 *      LINKLIST structure in your sources and don't
 *      use lstCreate/lstFree to have lists created
 *      dynamically. In that case, use this function
 *      as follows:
 +
 +          LINKLIST llWhatever;
 +          lstInit(&llWhatever);
 +          ...
 +          lstClear(&llWhatever);
 *
 *      The list which is initialized here must be
 *      cleaned up using lstClear.
 *
 *      If (fItemsFreeable == TRUE), free() will be
 *      invoked on list items automatically in lstClear,
 *      lstRemoveNode, and lstRemoveItem.
 *      Set this to TRUE if you have created the
 *      list items yourself. Set this to FALSE if
 *      you're storing other objects, such as numbers
 *      or other static items.
 *
 *      Note: You better call lstInit only once per list,
 *      because we don't check here if the list
 *      contains any data. This will lead to memory
 *      leaks if called upon existing lists, because
 *      we'll lose track of the allocated items.
 */

void lstInit(PLINKLIST pList,
             BOOL fItemsFreeable)   // in: invoke free() on the data
                                    // item pointers upon destruction?
{
    if (pList)
    {
        memset(pList, 0, sizeof(LINKLIST));
        pList->ulMagic = LINKLISTMAGIC;      // set integrity check word
        pList->fItemsFreeable = fItemsFreeable;
    }
}

/*
 *@@ lstClear:
 *      this will delete all list items. As opposed to
 *      lstFree, the "root" of the list will not be
 *      freed so that items can be added later again.
 *
 *      This can be used on static lists initialized with
 *      lstInit also.
 *
 *      If fItemsFreeable had been specified as TRUE
 *      when the list was initialized (lstInit), free()
 *      will be invoked on all data item pointers also.
 *      See the remarks there.
 *
 *      Returns FALSE only upon errors, e.g. because
 *      integrity checks failed.
 */

BOOL lstClear(PLINKLIST pList)
{
    BOOL brc = FALSE;

    if (pList)
        if (pList->ulMagic == LINKLISTMAGIC)
        {
            PLISTNODE  pNode = pList->pFirst,
                       pNode2;

            while (pNode)
            {
                if (pList->fItemsFreeable)
                    if (pNode->pItemData)
                        free(pNode->pItemData);
                pNode2 = pNode->pNext;
                free(pNode);
                pNode = pNode2;
            } // while (pNode);

            lstInit(pList, pList->fItemsFreeable);
            brc = TRUE;
        }

    return (brc);
}

#ifdef __XWPMEMDEBUG__

/*
 *@@ lstCreateDebug:
 *      debug version of lstCreate, using _debug_malloc.
 *
 *      Calls to lstCreate are automatically mapped to
 *      this function if __XWPMEMDEBUG__ is defined.
 *      This makes sure that the source file and line stored
 *      with the debug memory functions will be that of the
 *      caller, not the ones in this file.
 *
 *@@added V0.9.1 (99-12-18) [umoeller]
 */

PLINKLIST lstCreateDebug(BOOL fItemsFreeable,
                         const char *file,
                         unsigned long line,
                         const char *function)
{
    PLINKLIST pNewList = (PLINKLIST)memdMalloc(sizeof(LINKLIST),
                                               file,
                                               line,
                                               function);
    lstInit(pNewList, fItemsFreeable);
    return (pNewList);
}

#else

/*
 *@@ lstCreate:
 *      this creates a new linked list and initializes it
 *      (using lstInit).
 *      The LINKLIST structure which defines the "root" of
 *      the list is returned. This must be passed to all
 *      other list functions.
 *
 *      The list which is created here must be destroyed
 *      using lstFree.
 *
 *      If (fItemsFreeable == TRUE), free() will be
 *      invoked on list items automatically in lstFree,
 *      lstRemoveNode, and lstRemoveItem.
 *      Set this to TRUE if you have created the
 *      list items yourself. Set this to FALSE if
 *      you're storing other objects, such as numbers
 *      or other static items.
 *
 *      Returns NULL upon errors.
 *
 *      <B>Usage:</B>
 +          PLINKLIST pllWhatever = lstCreate();
 +          ...
 +          lstFree(pllWhatever);
 */

PLINKLIST lstCreate(BOOL fItemsFreeable)    // in: invoke free() on the data
                                            // item pointers upon destruction?
{
    PLINKLIST pNewList = (PLINKLIST)malloc(sizeof(LINKLIST));
    if (pNewList)
        lstInit(pNewList, fItemsFreeable);
    return (pNewList);
}

#endif      // __XWPMEMDEBUG__

/*
 *@@ lstFree:
 *      this will delete all list items (by calling
 *      lstClear) and the list "root" itself.
 *      Returns TRUE if anything was deleted at all
 *      (even if the list was empty)
 *      or FALSE upon errors, e.g. because integrity
 *      checks failed.
 *
 *      If fItemsFreeable had been specified as TRUE
 *      when the list was created (lstCreate), free()
 *      will be invoked on all data item pointers also.
 *      See the remarks there.
 *
 *      This must only be used with dynamic lists created
 *      with lstCreate.
 */

BOOL lstFree(PLINKLIST pList)
{
    BOOL brc = FALSE;

    if (lstClear(pList))        // this checks for list integrity
    {
        // unset magic word; the pList pointer
        // will point to invalid memory after
        // freeing the list, and subsequent
        // integrity checks must fail
        pList->ulMagic = 0;
        free(pList);

        brc = TRUE;
    }

    return (brc);
}

/*
 *@@ lstCountItems:
 *      this will return the number of items
 *      in the given linked list.
 *      This returns 0 if the list is empty
 *      or -1 if the list is invalid or corrupt.
 */

long lstCountItems(PLINKLIST pList)
{
    long lCount = -1;

    if (pList)
        if (pList->ulMagic == LINKLISTMAGIC)
        {
            lCount = pList->ulCount;
        }

    return (lCount);
}

/*
 *@@ lstQueryFirstNode:
 *      this returns the first node of the list,
 *      or NULL if the list is empty. This is
 *      preferred over getting the first directly
 *      from the LINKLIST structure, because this
 *      one checks for data integrity.
 *
 *      The item data can be queried from the node
 *      as follows:
 +          PLINKLIST pll = lstCreate();
 +          ...  // add items here
 +          PLISTNODE pNode = lstQueryFirstNode(pll);
 +          PYOURDATA pData = pNode->pItemData;
 *
 *      You can iterate over the whole list like this:
 +          PLISTNODE pNode = lstQueryFirstNode(pll);
 +          while (pNode)
 +          {
 +              PYOURDATA pYourData = (PYOURDATA)pNode->pItemData;
 +              ...
 +              pNode = pNode->pNext;
 +          }
 */

PLISTNODE lstQueryFirstNode(PLINKLIST pList)
{
    if (pList)
        if (pList->ulMagic == LINKLISTMAGIC)
            return (pList->pFirst);

    return (0);
}

/*
 *@@ lstNodeFromIndex:
 *      returns the data of list item with a given ulIndex
 *      (counting from 0), or NULL if ulIndex is too large.
 *
 *      This traverses the whole list, so it's not terribly
 *      fast. See lstQueryFirstNode for how to traverse
 *      the list yourself.
 *
 *      Note: As opposed to lstItemFromIndex, this one
 *      returns the LISTNODE structure.
 */

PLISTNODE lstNodeFromIndex(PLINKLIST pList,
                           unsigned long ulIndex)
{
    PLISTNODE pNode = NULL;

    if (pList)
        if (pList->ulMagic == LINKLISTMAGIC)
            if (pList->pFirst)
            {
                unsigned long ulCount = 0;
                pNode = pList->pFirst;
                for (ulCount = 0;
                     ((pNode) && (ulCount < ulIndex));
                     ulCount++)
                {
                    if (pNode->pNext)
                        pNode = pNode->pNext;
                    else
                        pNode = NULL; // exit
                }
            }

    return (pNode);
}

/*
 *@@ lstNodeFromItem:
 *      this finds the list node which has the given
 *      data pointer or NULL if not found.
 *
 *      This traverses the whole list, so it's not terribly
 *      fast. See lstQueryFirstNode for how to traverse
 *      the list yourself.
 */

PLISTNODE lstNodeFromItem(PLINKLIST pList, void* pItemData)
{
    PLISTNODE pNode = 0,
              pNodeFound = 0;

    if (pList)
        if (    (pList->ulMagic == LINKLISTMAGIC)
             && (pItemData)
           )
        {
            pNode = pList->pFirst;
            while (pNode)
            {
                if (pNode->pItemData == pItemData)
                {
                    pNodeFound = pNode;
                    break;
                }

                pNode = pNode->pNext;
            }
        }

    return (pNodeFound);
}

/*
 *@@ lstItemFromIndex:
 *      returns the data of the list item with a given ulIndex
 *      (counting from 0), or NULL if ulIndex is too large.
 *
 *      This traverses the whole list, so it's not terribly
 *      fast. See lstQueryFirstNode for how to traverse
 *      the list yourself.
 *
 *      Note: As opposed to lstNodeFromIndex, this one
 *      returns the item data directly, not the LISTNODE
 *      structure.
 */

void* lstItemFromIndex(PLINKLIST pList, unsigned long ulIndex)
{
    PLISTNODE pNode = lstNodeFromIndex(pList, ulIndex);
    if (pNode)
        return (pNode->pItemData);
    else
        return (0);
}

#ifdef __XWPMEMDEBUG__

/*
 *@@ lstAppendItemDebug:
 *      debug version of lstAppendItem, using _debug_malloc.
 *
 *      Calls to lstAppendItem are automatically mapped to
 *      this function if __XWPMEMDEBUG__ is defined.
 *      This makes sure that the source file and line stored
 *      with the debug memory functions will be that of the
 *      caller, not the ones in this file.
 *
 *@@added V0.9.1 (99-12-18) [umoeller]
 */

PLISTNODE lstAppendItemDebug(PLINKLIST pList,
                             void* pNewItemData,     // in: data to store in list node
                             const char *file,
                             unsigned long line,
                             const char *function)
{
    PLISTNODE pNewNode = 0;

    if (pList)
        if (pList->ulMagic == LINKLISTMAGIC)
        {
            pNewNode = (PLISTNODE)memdMalloc(sizeof(LISTNODE), file, line, function);
            if (pNewNode)
            {
                memset(pNewNode, 0, sizeof(LISTNODE));
                pNewNode->pItemData = pNewItemData;

                if (pList->pLast)
                {
                    // list is not empty: append to tail

                    // 1) make last item point to new node
                    pList->pLast->pNext = pNewNode;
                    // 2) make new node point to last item
                    pNewNode->pPrevious = pList->pLast;
                    // 3) store new node as new last item
                    pList->pLast = pNewNode;

                    pList->ulCount++;
                }
                else
                {
                    // list is empty: insert as first
                    pList->pFirst
                        = pList->pLast
                        = pNewNode;

                    pList->ulCount = 1;
                }
            }
         }

    return (pNewNode);
}

#else

/*
 *@@ lstAppendItem:
 *      this adds a new node to the tail of the list.
 *      As opposed to lstInsertItemBefore, this is very
 *      fast, since the list always keeps track of its last item.
 *
 *      This returns the LISTNODE of the new list item,
 *      or NULL upon errors.
 */

PLISTNODE lstAppendItem(PLINKLIST pList,
                        void* pNewItemData)     // in: data to store in list node
{
    PLISTNODE pNewNode = 0;

    if (pList)
        if (pList->ulMagic == LINKLISTMAGIC)
        {
            pNewNode = (PLISTNODE)malloc(sizeof(LISTNODE));
            if (pNewNode)
            {
                memset(pNewNode, 0, sizeof(LISTNODE));
                pNewNode->pItemData = pNewItemData;

                if (pList->pLast)
                {
                    // list is not empty: append to tail

                    // 1) make last item point to new node
                    pList->pLast->pNext = pNewNode;
                    // 2) make new node point to last item
                    pNewNode->pPrevious = pList->pLast;
                    // 3) store new node as new last item
                    pList->pLast = pNewNode;

                    pList->ulCount++;
                }
                else
                {
                    // list is empty: insert as first
                    pList->pFirst
                        = pList->pLast
                        = pNewNode;

                    pList->ulCount = 1;
                }
            }
         }

    return (pNewNode);
}

#endif // __XWPMEMDEBUG__

/*
 *@@ lstInsertItemBefore:
 *      this inserts a new node to the list. As opposed to
 *      lstAppendItem, the new node can be appended anywhere
 *      in the list, that is, it will be appended BEFORE
 *      the specified index lIndex.
 *
 *      So if ulIndex == 0, the new item will be made the first
 *      item.
 *
 *      If ulIndex is larger than the item count on the list,
 *      the new node will be appended at the tail of the list
 *      (as with lstAppendItem).
 *
 *      This needs to traverse the list to find the indexed
 *      item, so for larger ulIndex's this function is not
 *      terribly fast.
 *
 *      This returns the LISTNODE of the new list item,
 *      or NULL upon errors.
 */

PLISTNODE lstInsertItemBefore(PLINKLIST pList,
                              void* pNewItemData,     // data to store in list node
                              unsigned long ulIndex)
{
    PLISTNODE pNewNode = 0;

    if (pList)
        if (pList->ulMagic == LINKLISTMAGIC)
        {
            pNewNode = (PLISTNODE)malloc(sizeof(LISTNODE));
            if (pNewNode)
            {
                memset(pNewNode, 0, sizeof(LISTNODE));
                pNewNode->pItemData = pNewItemData;

                if (ulIndex == 0)
                {
                    // insert at beginning:
                    if (pList->pFirst)
                        pList->pFirst->pPrevious = pNewNode;

                    pNewNode->pNext = pList->pFirst;
                    pNewNode->pPrevious = NULL;

                    pList->pFirst = pNewNode;

                    pList->ulCount++;
                }
                else
                {
                    // insert at a later position:
                    PLISTNODE pNodeInsertAfter = lstNodeFromIndex(
                                            pList,
                                            (ulIndex-1));

                    if (pNodeInsertAfter)
                    {
                        // 1) set pointers for new node
                        pNewNode->pPrevious = pNodeInsertAfter;
                        pNewNode->pNext = pNodeInsertAfter->pNext;

                        // 2) adjust next item
                        // so that it points to the new node
                        if (pNodeInsertAfter->pNext)
                            pNodeInsertAfter->pNext->pPrevious = pNewNode;

                        // 3) adjust previous item
                        // so that it points to the new node
                        pNodeInsertAfter->pNext = pNewNode;

                        // 4) adjust last item, if necessary
                        if (pList->pLast == pNodeInsertAfter)
                            pList->pLast = pNewNode;

                        pList->ulCount++;
                    }
                    else
                    {
                        // item index too large: append instead
                        free(pNewNode);
                        pNewNode = lstAppendItem(pList, pNewItemData);
                    }
                }
            }
        }

    return (pNewNode);
}

/*
 *@@ lstRemoveNode:
 *      this will remove a given pRemoveNode from
 *      the given linked list.
 *
 *      To remove a node according to the item pointer,
 *      use lstRemoveItem.
 *      To get the node from a node index, use
 *      lstNodeFromIndex.
 *
 *      Since the LISTNODE is known, this function
 *      operates very fast.
 *
 *      If fItemsFreeable had been specified as TRUE
 *      when the list was created (lstInit or lstCreate),
 *      free() will be invoked on the data item pointer also.
 *      See the remarks there.
 *
 *      Returns TRUE if successful, FALSE upon errors.
 */

BOOL lstRemoveNode(PLINKLIST pList, PLISTNODE pRemoveNode)
{
    BOOL fFound = FALSE;

    if (pList)
        if (    (pList->ulMagic == LINKLISTMAGIC)
             && (pRemoveNode)
           )
        {
            if (pList->pFirst == pRemoveNode)
                // item to be removed is first: adjust first
                pList->pFirst = pRemoveNode->pNext;     // can be NULL
            if (pList->pLast == pRemoveNode)
                // item to be removed is last: adjust last
                pList->pLast = pRemoveNode->pPrevious;  // can be NULL

            if (pRemoveNode->pPrevious)
                // adjust previous item
                pRemoveNode->pPrevious->pNext = pRemoveNode->pNext;

            if (pRemoveNode->pNext)
                // adjust next item
                pRemoveNode->pNext->pPrevious = pRemoveNode->pPrevious;

            // decrease list count
            pList->ulCount--;

            // free node data
            if (pList->fItemsFreeable)
                if (pRemoveNode->pItemData)
                    free(pRemoveNode->pItemData);
            // free node
            free(pRemoveNode);

            fFound = TRUE;
        }

    return (fFound);
}

/*
 *@@ lstRemoveItem:
 *      as opposed to lstRemoveNode, this removes a
 *      node according to an item data pointer.
 *
 *      This needs to call lstNodeFromItem first and
 *      then invokes lstRemoveNode, so this is a lot
 *      slower.
 *
 *      If fItemsFreeable had been specified as TRUE
 *      when the list was created (lstInit or lstCreate),
 *      free() will be invoked on the data item pointer also.
 *      See the remarks there.
 *
 *      Returns TRUE if successful, FALSE upon errors.
 */

BOOL lstRemoveItem(PLINKLIST pList, void* pRemoveItem)
{
    BOOL brc = FALSE;

    PLISTNODE pNode = lstNodeFromItem(pList, pRemoveItem);

    if (pNode)
        brc = lstRemoveNode(pList, pNode);

    return (brc);
}

/*
 *@@ lstSwapItems:
 *      this will swap the items pNode1 and pNode2.
 *      This is used in the sort routines.
 *
 *      Note that it is not really the nodes that are swapped,
 *      but only the data item pointers. This is a lot quicker
 *      than what we did in versions < 0.9.0.
 */

BOOL lstSwapNodes(PLISTNODE pNode1,
                  PLISTNODE pNode2)
{
    BOOL brc = FALSE;

    if ( (pNode1) && (pNode2) )
    {
        void* pTemp = pNode1->pItemData;
        pNode1->pItemData = pNode2->pItemData;
        pNode2->pItemData = pTemp;

        brc = TRUE;
    }

    return (brc);
}

/* ******************************************************************
 *                                                                  *
 *   List sorting                                                   *
 *                                                                  *
 ********************************************************************/

/*
 * lstQuickSort2:
 *      helper routine for recursing during quicksort (lstQuickSort).
 */

void lstQuickSort2(PLINKLIST pList,
                   PFNSORTLIST pfnSort,
                   void* pStorage,
                   long lLeft,
                   long lRight)
{
    long ll = lLeft,
         lr = lRight-1,
         lPivot = lRight;

    if (lRight > lLeft)
    {
        PLISTNODE   pNodeLeft = lstNodeFromIndex(pList, ll),
                    pNodeRight = lstNodeFromIndex(pList, lr),
                    pNodePivot = lstNodeFromIndex(pList, lPivot);

        while (TRUE)
        {
            // compare left item data to pivot item data
            while ( (*pfnSort)(pNodeLeft->pItemData,
                               pNodePivot->pItemData,
                               pStorage)
                    < 0 )
            {
                ll++;
                // advance to next
                pNodeLeft = pNodeLeft->pNext;
            }

            // compare right item to pivot
            while (     ( (*pfnSort)(pNodeRight->pItemData,
                                     pNodePivot->pItemData,
                                     pStorage)
                           >= 0 )
                    && (lr > ll)
                  )
            {
                lr--;
                // step to previous
                pNodeRight = pNodeRight->pPrevious;
            }

            if (lr <= ll)
                break;

            // swap pNodeLeft and pNodeRight
            lstSwapNodes(pNodeLeft, pNodeRight);
        }

        // swap pNodeLeft and pNodePivot
        lstSwapNodes(pNodeLeft, pNodePivot);

        // recurse!
        lstQuickSort2(pList, pfnSort, pStorage, lLeft, ll-1);
        lstQuickSort2(pList, pfnSort, pStorage, ll+1, lRight);
    }
}

/*
 *@@ lstQuickSort:
 *      this will sort the given linked list using the
 *      sort function pfnSort, which works similar to those of the
 *      container sorts, i.e. it must be declared as a
 +          SHORT fnSort(void* pItem1, void* pItem1, void* pStorage)
 *
 *      These sort functions need to return the following:
 *      --      0:   pItem1 == pItem2
 *      --     -1:   pItem1 <  pItem2
 *      --     +1:   pItem1 >  pItem2
 *
 *      Note that the comparison function does not get the PLISTNODEs,
 *      but the item data pointers instead.
 *
 *      The pStorage parameter will simply be passed to the comparison
 *      function. This might be useful for additional data you might need.
 *
 *      This returns FALSE upon errors.
 *
 *      This function implements the "quick sort" algorithm, which is one
 *      of the fastest sort algorithms known to mankind. However, this is
 *      an "instable" sort algorithm, meaning that list items considered
 *      equal by pfnSort do _not_ retain their position, but might be
 *      changed. If you need these to remain where they are, use
 *      lstBubbleSort, which is a lot slower though.
 *
 *      This calls lstSwapNodes to swap the list items.
 */

BOOL lstQuickSort(PLINKLIST pList,
                  PFNSORTLIST pfnSort,
                  void* pStorage)
{
    BOOL brc = FALSE;

    if (pList)
        if (    (pList->ulMagic == LINKLISTMAGIC)
             && (pfnSort)
           )
        {
            long lRight = lstCountItems(pList)-1;

            lstQuickSort2(pList, pfnSort, pStorage,
                        0,          // lLeft
                        lRight);
            brc = TRUE;
        }

    return (brc);
}

/*
 *@@ lstBubbleSort:
 *      just like lstQuickSort, this will sort a given linked list.
 *      See lstQuickSort for the parameters.
 *
 *      However, this function uses the "bubble sort" algorithm, which
 *      will cause a lot of calls to pfnSort and which is really
 *      ineffective for lists with more than 100 items.
 *      Use only if you absolutely need a stable sort.
 *
 *      This calls lstSwapNodes to swap the list items.
 */

BOOL lstBubbleSort(PLINKLIST pList,
                   PFNSORTLIST pfnSort,
                   void* pStorage)
{
    BOOL brc = FALSE;

    if (pList)
        if (    (pList->ulMagic == LINKLISTMAGIC)
             && (pfnSort)
           )
        {
            long lRight = lstCountItems(pList),
                 lSorted,
                 x;

            do {
                lRight--;
                lSorted = 0;
                for (x = 0; x < lRight; x++)
                {
                    // ulComp++;
                    PLISTNODE pNode1 = lstNodeFromIndex(pList, x),
                              pNode2 = pNode1->pNext;
                    if ((pNode1) && (pNode2))
                        if ( (*pfnSort)(pNode1->pItemData,
                                        pNode2->pItemData,
                                        pStorage) > 0
                           )
                        {
                            lstSwapNodes(pNode1, pNode2);
                            lSorted++;
                        }
                }
            } while ( lSorted && (lRight > 1) );

            brc = TRUE;
        }

    return (brc);
}

/* ******************************************************************
 *                                                                  *
 *   List pseudo-stacks                                             *
 *                                                                  *
 ********************************************************************/

/*
 *@@ lstPush:
 *      pushes any data onto a "stack", which is a plain
 *      LINKLIST being abused as a stack.
 *      The "stack" is last-in, first-out (LIFO), meaning
 *      that the last item pushed onto the list will be
 *      the first returned by lstPop. See lstPop for example
 *      usage.
 *
 *@@added V0.9.3 (2000-05-18) [umoeller]
 */

PLISTNODE lstPush(PLINKLIST pList,
                  void* pNewItemData)     // in: data to store in list node
{
    return (lstAppendItem(pList, pNewItemData));
}

/*
 *@@ lstPop:
 *      returns the last item which has been pushed onto
 *      a pseudo-stack LIFO LINKLIST using lstPush.
 *      Note that to really "pop" the item, i.e. remove
 *      it from the list, you MUST call lstRemoveNode
 *      with the list node returned from this function,
 *      or otherwise you keep getting the same last item
 *      on subsequent calls.
 *
 *      Example:
 *
 +          LINKLIST ll;
 +          PLISTNODE pNode;
 +          lstInit(&ll, FALSE;
 +
 +          lstPush(&ll, (PVOID)1);
 +          lstPush(&ll, (PVOID)2);
 +
 +          pNode = lstPop(&ll);     // returns the node containing "2"
 +          lstRemoveNode(pNode);
 +          pNode = lstPop(&ll);     // returns the node containing "1"
 +          lstRemoveNode(pNode);
 *
 *@@added V0.9.3 (2000-05-18) [umoeller]
 */

PLISTNODE lstPop(PLINKLIST pList)
{
    PLISTNODE pNodeReturn = NULL;
    unsigned long cItems = lstCountItems(pList);
    if (cItems)
    {
        // list not empty:
        // get last item on the list
        pNodeReturn = lstNodeFromIndex(pList,
                                       cItems - 1);
    }
    return (pNodeReturn);
}

