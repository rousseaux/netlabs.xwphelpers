/*  ----------------------------------------------------------------<Prolog>-
    Name:       sfltree.h
    Title:      Linked-list functions
    Package:    Standard Function Library (SFL)

    Written:    97/11/18  Jonathan Schultz <jonathan@imatix.com>
    Revised:    98/01/03  Jonathan Schultz <jonathan@imatix.com>

    Synopsis:   Provides functions to maintain 'Red-Black' balanced binary
                trees.  You can use these functions to work with trees of any
                structure.  To make this work, all structures must start with
                the following: "void *left, *right, *parent; TREE_COLOUR
                colour;".  All trees need a pointer to the root of type TREE
                which should be initialised with treeInit - you can test
                whether a tree is empty by comparing its root with TREE_NULL.
                The order of nodes in the tree is determined by calling a
                node comparison function provided by the caller - this
                accepts two node pointers  and returns zero if the two nodes
                are equal, -1 if the first is smaller and 1 if the first is
                larger.

    Copyright:  Copyright (c) 1991-99 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#if __cplusplus
extern "C" {
#endif

#ifndef SFLTREE_INCLUDED               //  Allow multiple inclusions
    #define SFLTREE_INCLUDED

    #if (!defined OS2_INCLUDED) && (!defined _OS2_H) && (!defined __SIMPLES_DEFINED)   // changed V0.9.0 (99-10-22) [umoeller]
        typedef unsigned long BOOL;
        #define TRUE (BOOL)1
        #define FALSE (BOOL)0

        #ifdef __IBMCPP__               // added V0.9.0 (99-10-22) [umoeller]
            #define APIENTRY _System
        #endif

        #define __SIMPLES_DEFINED
    #endif

    // Red-Black tree description
    typedef enum {BLACK, RED} TREE_COLOUR;

    /*
     *@@ TREE:
     *      tree node.
     */

    typedef struct _TREE
    {
        struct _TREE    *left,
                        *right,
                        *parent;
        unsigned long   id;
        TREE_COLOUR     colour;
    } TREE;

    // The tree algorithm needs to know how to sort the data.
    // It does this using a functions provided by the calling program.
    // This must return:
    // --   0: t1 == t2
    // --  -1: t1 < t2
    // --  +1: t1 > t2
    typedef int (FNTREE_COMPARE_IDS) (unsigned long id1, unsigned long id2);
    typedef int (FNTREE_COMPARE_NODES) (TREE *t1, TREE *t2);
    typedef int (FNTREE_COMPARE_DATA) (TREE *t1, void *pData);

    //  Define a function type for use with the tree traversal function
    typedef void (TREE_PROCESS) (TREE *t, void *pUser);

    //  Global variables
    extern TREE   TREE_EMPTY;

    //  Function prototypes
    void treeInit(TREE **root);

    int treeInsertID(TREE **root,
                     TREE *tree,
                     FNTREE_COMPARE_IDS *comp,
                     BOOL fAllowDuplicates);

    int treeInsertNode(TREE **root,
                       TREE *tree,
                       FNTREE_COMPARE_NODES *comp,
                       BOOL fAllowDuplicates);

    int treeDelete(TREE **root,
                   TREE *tree);

    /* void *treeFindEQID  (TREE **root,
                       TREE *tree,
                       TREE_COMPARE *comp); */
    /* void *treeFindGEID  (TREE **root,
                       TREE *tree,
                       FNTREE_COMPARE_IDS *comp); */

    void* treeFindEQNode(TREE **root,
                         TREE *nodeFind,
                         FNTREE_COMPARE_NODES *comp);
    void* treeFindLTNode(TREE **root,
                         TREE *nodeFind,
                         FNTREE_COMPARE_NODES *comp);
    void* treeFindLENode(TREE **root,
                         TREE *nodeFind,
                         FNTREE_COMPARE_NODES *comp);
    void* treeFindGTNode(TREE **root,
                         TREE *nodeFind,
                         FNTREE_COMPARE_NODES *comp);
    void* treeFindGENode(TREE **root,
                         TREE *nodeFind,
                         FNTREE_COMPARE_NODES *comp);

    void* treeFindEQID(TREE **root,
                       unsigned long idFind,
                       FNTREE_COMPARE_IDS *comp);
    void* treeFindLTID(TREE **root,
                       unsigned long idFind,
                       FNTREE_COMPARE_IDS *comp);
    void* treeFindLEID(TREE **root,
                       unsigned long idFind,
                       FNTREE_COMPARE_IDS *comp);
    void* treeFindGTID(TREE **root,
                       unsigned long idFind,
                       FNTREE_COMPARE_IDS *comp);
    void* treeFindGEID(TREE **root,
                       unsigned long idFind,
                       FNTREE_COMPARE_IDS *comp);

    void* treeFindEQData(TREE **root,
                         void *pData,
                         FNTREE_COMPARE_DATA *comp);

    void  treeTraverse (TREE *tree,
                        TREE_PROCESS *process,
                        void *pUser,
                        int method);
    void *treeNext     (TREE *tree);
    void *treePrev     (TREE *tree);
    void *treeFirst    (TREE *tree);
    void *treeLast     (TREE *tree);

    //  Return codes
    #define TREE_OK                 0
    #define TREE_DUPLICATE          1
    #define TREE_INVALID_NODE       2

    //  Macros
    #define TREE_NULL &TREE_EMPTY

#endif

#if __cplusplus
}
#endif

