
/*
 *@@sourcefile tree.h:
 *      header file for tree.c (red-black balanced binary trees).
 *      See remarks there.
 */

#if __cplusplus
extern "C" {
#endif

#ifndef XWPTREE_INCLUDED               //  Allow multiple inclusions
    #define XWPTREE_INCLUDED

    #if (!defined OS2_INCLUDED) && (!defined _OS2_H) && (!defined __SIMPLES_DEFINED)   // changed V0.9.0 (99-10-22) [umoeller]
        typedef unsigned long BOOL;
        #define TRUE (BOOL)1
        #define FALSE (BOOL)0

        #ifdef __IBMCPP__               // added V0.9.0 (99-10-22) [umoeller]
            #define APIENTRY _System
        #endif

        #define __SIMPLES_DEFINED
    #endif

    typedef enum { BLACK, RED } nodeColor;

    /*
     *@@ TREE:
     *      tree node.
     *
     *      To use the tree functions, all your structures
     *      must have a TREE structure as their first member.
     *
     *      Example:
     *
     +      typedef struct _MYTREENODE
     +      {
     +          TREE        tree;
     +          char        acMyData[1000];
     +      } MYTREENODE, *PMYTREENODE;
     *
     *      See tree.c for an introduction to the tree functions.
     */

    typedef struct _TREE
    {
        struct _TREE    *left,
                        *right,
                        *parent;
        nodeColor       color;          // the node's color (BLACK, RED)

        unsigned long   ulKey;          // the node's key (data)

    } TREE, *PTREE;

    #if defined(__IBMC__) || defined(__IBMCPP__)
        #define TREEENTRY _Optlink
    #else
        // EMX or whatever: doesn't know calling conventions
        #define TREEENTRY
    #endif

    #define STATUS_OK                   0
    #define STATUS_DUPLICATE_KEY        -1
    #define STATUS_INVALID_NODE         -2

    typedef int TREEENTRY FNTREE_COMPARE(unsigned long ul1, unsigned long ul2);

    //  Function prototypes
    void treeInit(TREE **root);

    int TREEENTRY treeCompareKeys(unsigned long  ul1, unsigned long ul2);

    int TREEENTRY treeCompareStrings(unsigned long  ul1, unsigned long ul2);

    int treeInsert(TREE **root,
                   TREE *x,
                   FNTREE_COMPARE *pfnCompare);

    int treeDelete(TREE **root,
                   TREE *z);

    TREE* treeFind(TREE *root,
                   unsigned long key,
                   FNTREE_COMPARE *pfnCompare);

    TREE* treeFirst(TREE *r);

    TREE* treeLast(TREE *r);

    TREE* treeNext(TREE *r);

    TREE* treePrev(TREE *r);

    TREE** treeBuildArray(TREE* pRoot,
                          unsigned long *pulCount);

#endif

#if __cplusplus
}
#endif

