
/*
 *@@sourcefile tree.c:
 *      contains helper functions for maintaining 'Red-Black' balanced
 *      binary trees.
 *
 *      Usage: All C programs; not OS/2-specific.
 *
 *      Function prefixes (new with V0.81):
 *      --  tree*    tree helper functions
 *
 *      <B>Introduction</B>
 *
 *      While linked lists have "next" and "previous" pointers (which
 *      makes them one-dimensional), trees have a two-dimensional layout:
 *      each tree node has one "parent" and two "children" which are
 *      called "left" and "right". The "left" pointer will always lead
 *      to a tree node that is "less than" its parent node, while the
 *      "right" pointer will lead to a node that is "greater than"
 *      its parent. What is considered "less" or "greater" for sorting
 *      is determined by a comparison callback to be supplied by the
 *      tree functions' caller. The "leafs" of the tree will have
 *      null left and right pointers.
 *
 *      For this, the functions here use the TREE structure. The most
 *      important member here is the "ulKey" field which is used for
 *      sorting (passed to the compare callbacks). Since the tree
 *      functions do no memory allocation, the caller can easily
 *      use an extended TREE structure with additional fields as
 *      long as the first member is the TREE structure. See below.
 *
 *      Each tree must have a "root" item, from which all other tree
 *      nodes can eventually be reached by following the "left" and
 *      "right" pointers. The root node is the only node whose
 *      parent is null.
 *
 *      <B>Trees vs. linked lists</B>
 *
 *      Compared to linked lists (as implemented by linklist.c),
 *      trees allow for much faster searching, since they are
 *      always sorted.
 *
 *      Take an array of numbers, and assume you'd need to find
 *      the array node with the specified number.
 *
 *      With a (sorted) linked list, this would look like:
 *
 +          4  -->  7  -->  16  -->  20  -->  37  -->  38  -->  43
 *
 *      Searching for "43" would need 6 iterations.
 *
 *      With a binary tree, this would instead look like:
 *
 +                       20
 +                     /    \
 +                    7      38
 +                   / \    /  \
 +                  4  16  37   43
 +                 / \ / \ / \ / \
 *
 *      Searching for "43" would need 2 iterations only.
 *
 *      Assuming a linked list contains N items, then searching a
 *      linked list for an item will take an average of N/2 comparisons
 *      and even N comparisons if the item cannot be found (unless
 *      you keep the list sorted, but linklist.c doesn't do this).
 *
 *      According to "Algorithms in C", a search in a balanced
 *      "red-black" binary tree takes about lg N comparisons on
 *      average, and insertions take less than one rotation on
 *      average.
 *
 *      Differences compared to linklist.c:
 *
 *      -- A tree is always sorted.
 *
 *      -- Trees are considerably slower when inserting and removing
 *         nodes because the tree has to be rebalanced every time
 *         a node changes. By contrast, trees are much faster finding
 *         nodes because the tree is always sorted.
 *
 *      -- As opposed to a LISTNODE, the TREE structure (which
 *         represents a tree node) does not contain a data pointer,
 *         as said above. The caller must do all memory management.
 *
 *      <B>Background</B>
 *
 *      Now, a "red-black balanced binary tree" means the following:
 *
 *      -- We have "binary" trees. That is, there are only "left" and
 *         "right" pointers. (Other algorithms allow tree nodes to
 *         have more than two children, but binary trees are usually
 *         more efficient.)
 *
 *      -- The tree is always "balanced". The tree gets reordered
 *         when items are added/removed to ensure that all paths
 *         through the tree are approximately the same length.
 *         This avoids the "worst case" scenario that some paths
 *         grow terribly long while others remain short ("degenerated"
 *         trees), which can make searching very inefficient:
 *
 +                  4
 +                 / \
 +                    7
 +                   / \
 +                      16
 +                     / \
 +                        20
 +                       / \
 +                          37
 +                         / \
 +                            43
 +                           /  \
 *
 *      -- Fully balanced trees can be quite expensive because on
 *         every insertion or deletion, the tree nodes must be rotated.
 *         By contrast, "Red-black" binary balanced trees contain
 *         an additional bit in each node which marks the node as
 *         either red or black. This bit is used only for efficient
 *         rebalancing when inserting or deleting nodes.
 *
 *         I don't fully understand why this works, but if you really
 *         care, this is explained at
 *         "http://www.eli.sdsu.edu/courses/fall96/cs660/notes/redBlack/redBlack.html".
 *
 *      In other words, as opposed to regular binary trees, RB trees
 *      are not _fully_ balanced, but they are _mostly_ balanced. With
 *      respect to efficiency, RB trees are thus a good compromise:
 *
 *      -- Completely unbalanced trees are efficient when inserting,
 *         but can have a terrible worst case when searching.
 *
 *      -- RB trees are still acceptably efficient when inserting
 *         and quite efficient when searching.
 *         A RB tree with n internal nodes has a height of at most
 *         2lg(n+1). Both average and worst-case search time is O(lg n).
 *
 *      -- Fully balanced binary trees are inefficient when inserting
 *         but most efficient when searching.
 *
 *      So as long as you are sure that trees are more efficient
 *      in your situation than a linked list in the first place, use
 *      these RB trees instead of linked lists.
 *
 *      <B>Using binary trees</B>
 *
 *      You can use any structure as elements in a tree, provided
 *      that the first member in the structure is a TREE structure
 *      (i.e. it has the left, right, parent, and color members).
 *      Each TREE node has a ulKey field which is used for
 *      comparing tree nodes and thus determines the location of
 *      the node in the tree.
 *
 *      The tree functions don't care what follows in each TREE
 *      node since they do not manage any memory themselves.
 *      So the implementation here is slightly different from the
 *      linked lists in linklist.c, because the LISTNODE structs
 *      only have pointers to the data. By contrast, the TREE structs
 *      are expected to contain the data themselves.
 *
 *      Initialize the root of the tree with treeInit(). Then
 *      add nodes to the tree with treeInsert() and remove nodes
 *      with treeDelete(). See below for a sample.
 *
 *      You can test whether a tree is empty by comparing its
 *      root with LEAF.
 *
 *      For most tree* functions, you must specify a comparison
 *      function which will always receive two "key" parameters
 *      to compare. This must be declared as
 +
 +          int TREEENTRY fnCompare(ULONG ul1, ULONG ul2);
 *
 *      This will receive two TREE.ulKey members (whose meaning
 *      is defined by your implementation) and must return
 *
 *      -- something < 0: ul1 < ul2
 *      -- 0: ul1 == ul2
 *      -- something > 1: ul1 > ul2
 *
 *      <B>Example</B>
 *
 *      A good example where trees are efficient would be the
 *      case where you have "keyword=value" string pairs and
 *      you frequently need to search for "keyword" to find
 *      a "value". So "keyword" would be an ideal candidate for
 *      the TREE.key field.
 *
 *      You'd then define your own tree nodes like this:
 *
 +          typedef struct _MYTREENODE
 +          {
 +              TREE        Tree;       // regular tree node, which has
 +                                      // the ULONG "key" field; we'd
 +                                      // use this as a const char*
 +                                      // pointer to the keyword string
 +              // here come the additional fields
 +              // (whatever you need for your data)
 +              const char  *pcszValue;
 +
 +          } MYTREENODE, *PMYTREENODE;
 *
 *      Initialize the tree root:
 *
 +          TREE *root;
 +          treeInit(&root);
 *
 *      To add a new "keyword=value" pair, do this:
 *
 +          PMYTREENODE AddNode(TREE **root,
 +                              const char *pcszKeyword,
 +                              const char *pcszValue)
 +          {
 +              PMYTREENODE p = (PMYTREENODE)malloc(sizeof(MYTREENODE));
 +              p.Tree.ulKey = (ULONG)pcszKeyword;
 +              p.pcszValue = pcszValue;
 +              treeInsert(root,                // tree's root
 +                         p,                   // new tree node
 +                         fnCompare);          // comparison func
 +              return (p);
 +          }
 *
 *      Your comparison func receives two ulKey values to compare,
 *      which in this case would be the typecast string pointers:
 *
 +          int TREEENTRY fnCompare(ULONG ul1, ULONG ul2)
 +          {
 +              return (strcmp((const char*)ul1,
 +                             (const char*)ul2));
 +          }
 *
 *      You can then use treeFind to very quickly find a node
 *      with a specified ulKey member.
 *
 *      This file was new with V0.9.5 (2000-09-29) [umoeller].
 *      With V0.9.13, all the code has been replaced with the public
 *      domain code found at http://epaperpress.com/sortsearch/index.html
 *      ("A compact guide to searching and sorting") by Thomas Niemann.
 *      The old implementation from the Standard Function Library (SFL)
 *      turned out to be buggy for large trees (more than 100 nodes).
 *
 *@@added V0.9.5 (2000-09-29) [umoeller]
 *@@header "helpers\tree.h"
 */

/*
 *      Original coding by Thomas Niemann, placed in the public domain
 *      (see http://epaperpress.com/sortsearch/index.html).
 *
 *      This implementation Copyright (C) 2001 Ulrich M�ller.
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

/*
 *@@category: Helpers\C helpers\Red-black balanced binary trees
 *      See tree.c.
 */

#include "setup.h"
#include "helpers\tree.h"

#define LEAF &sentinel           // all leafs are sentinels
static TREE sentinel = { LEAF, LEAF, 0, BLACK};

/*
A binary search tree is a red-black tree if:

1. Every node is either red or black.
2. Every leaf (nil) is black.
3. If a node is red, then both its children are black.
4. Every simple path from a node to a descendant leaf contains the same
   number of black nodes.
*/

/*
 *@@ treeInit:
 *      initializes the root of a tree.
 *
 */

void treeInit(TREE **root)
{
    *root = LEAF;
}

/*
 *@@ treeCompareKeys:
 *      standard comparison func if the TREE.ulKey
 *      field really is a ULONG.
 */

int TREEENTRY treeCompareKeys(unsigned long  ul1, unsigned long ul2)
{
    if (ul1 < ul2)
        return -1;
    if (ul1 > ul2)
        return +1;
    return (0);
}

/*
 *@@ rotateLeft:
 *      private function during rebalancing.
 */

static void rotateLeft(TREE **root,
                       TREE *x)
{
   /**************************
    *  rotate node x to left *
    **************************/

    TREE *y = x->right;

    // establish x->right link
    x->right = y->left;
    if (y->left != LEAF)
        y->left->parent = x;

    // establish y->parent link
    if (y != LEAF)
        y->parent = x->parent;
    if (x->parent)
    {
        if (x == x->parent->left)
            x->parent->left = y;
        else
            x->parent->right = y;
    }
    else
        *root = y;

    // link x and y
    y->left = x;
    if (x != LEAF)
        x->parent = y;
}

/*
 *@@ rotateRight:
 *      private function during rebalancing.
 */

static void rotateRight(TREE **root,
                        TREE *x)
{

   /****************************
    *  rotate node x to right  *
    ****************************/

    TREE *y = x->left;

    // establish x->left link
    x->left = y->right;
    if (y->right != LEAF)
        y->right->parent = x;

    // establish y->parent link
    if (y != LEAF)
        y->parent = x->parent;
    if (x->parent)
    {
        if (x == x->parent->right)
            x->parent->right = y;
        else
            x->parent->left = y;
    }
    else
        *root = y;

    // link x and y
    y->right = x;
    if (x != LEAF)
        x->parent = y;
}

/*
 *@@ insertFixup:
 *      private function during rebalancing.
 */

static void insertFixup(TREE **root,
                        TREE *x)
{
   /*************************************
    *  maintain Red-Black tree balance  *
    *  after inserting node x           *
    *************************************/

    // check Red-Black properties
    while (    x != *root
            && x->parent->color == RED
          )
    {
        // we have a violation
        if (x->parent == x->parent->parent->left)
        {
            TREE *y = x->parent->parent->right;
            if (y->color == RED)
            {
                // uncle is RED
                x->parent->color = BLACK;
                y->color = BLACK;
                x->parent->parent->color = RED;
                x = x->parent->parent;
            }
            else
            {
                // uncle is BLACK
                if (x == x->parent->right)
                {
                    // make x a left child
                    x = x->parent;
                    rotateLeft(root,
                               x);
                }

                // recolor and rotate
                x->parent->color = BLACK;
                x->parent->parent->color = RED;
                rotateRight(root,
                            x->parent->parent);
            }
        }
        else
        {
            // mirror image of above code
            TREE *y = x->parent->parent->left;
            if (y->color == RED)
            {
                // uncle is RED
                x->parent->color = BLACK;
                y->color = BLACK;
                x->parent->parent->color = RED;
                x = x->parent->parent;
            }
            else
            {
                // uncle is BLACK
                if (x == x->parent->left)
                {
                    x = x->parent;
                    rotateRight(root,
                                x);
                }
                x->parent->color = BLACK;
                x->parent->parent->color = RED;
                rotateLeft(root,
                           x->parent->parent);
            }
        }
    }
    (*root)->color = BLACK;
}

/*
 *@@ treeInsert:
 *      inserts a new tree node into the specified
 *      tree, using the specified comparison function
 *      for sorting.
 *
 *      "x" specifies the new tree node which must
 *      have been allocated by the caller. x->ulKey
 *      must already contain the node's key (data).
 *      This function will then set the parent,
 *      left, right, and color members.
 *
 *      Returns 0 if no error. Might return
 *      STATUS_DUPLICATE_KEY if a node with the
 *      same ulKey already exists.
 */

int treeInsert(TREE **root,                     // in: root of the tree
               TREE *x,                         // in: new node to insert
               FNTREE_COMPARE *pfnCompare)      // in: comparison func
{
    TREE    *current,
            *parent;

    unsigned long key = x->ulKey;

    // find future parent
    current = *root;
    parent = 0;

    while (current != LEAF)
    {
        int iResult;
        if (0 == (iResult = pfnCompare(key, current->ulKey))) // if (compEQ(key, current->key))
            return STATUS_DUPLICATE_KEY;
        parent = current;
        current = (iResult < 0)    // compLT(key, current->key)
                    ? current->left
                    : current->right;
    }

    // set up new node
    /* if ((x = malloc (sizeof(*x))) == 0)
        return STATUS_MEM_EXHAUSTED; */
    x->parent = parent;
    x->left = LEAF;
    x->right = LEAF;
    x->color = RED;
    // x->key = key;
    // x->rec = *rec;

    // insert node in tree
    if (parent)
    {
        if (pfnCompare(key, parent->ulKey) < 0) // (compLT(key, parent->key))
            parent->left = x;
        else
            parent->right = x;
    }
    else
        *root = x;

    insertFixup(root,
                x);
    // lastFind = NULL;

    return STATUS_OK;
}

/*
 *@@ deleteFixup:
 *
 */

static void deleteFixup(TREE **root,
                        TREE *tree)
{
    TREE    *s;

    while (    tree != *root
            && tree->color == BLACK
          )
    {
        if (tree == tree->parent->left)
        {
            s = tree->parent->right;
            if (s->color == RED)
            {
                s->color = BLACK;
                tree->parent->color = RED;
                rotateLeft(root, tree->parent);
                s = tree->parent->right;
            }
            if (    (s->left->color == BLACK)
                 && (s->right->color == BLACK)
               )
            {
                s->color = RED;
                tree = tree->parent;
            }
            else
            {
                if (s->right->color == BLACK)
                {
                    s->left->color = BLACK;
                    s->color = RED;
                    rotateRight(root, s);
                    s = tree->parent->right;
                }
                s->color = tree->parent->color;
                tree->parent->color = BLACK;
                s->right->color = BLACK;
                rotateLeft(root, tree->parent);
                tree = *root;
            }
        }
        else
        {
            s = tree->parent->left;
            if (s->color == RED)
            {
                s->color = BLACK;
                tree->parent->color = RED;
                rotateRight(root, tree->parent);
                s = tree->parent->left;
            }
            if (    (s->right->color == BLACK)
                 && (s->left->color == BLACK)
               )
            {
                s->color = RED;
                tree = tree->parent;
            }
            else
            {
                if (s->left->color == BLACK)
                {
                    s->right->color = BLACK;
                    s->color = RED;
                    rotateLeft(root, s);
                    s = tree->parent->left;
                }
                s->color = tree->parent->color;
                tree->parent->color = BLACK;
                s->left->color = BLACK;
                rotateRight (root, tree->parent);
                tree = *root;
            }
        }
    }
    tree->color = BLACK;

   /*************************************
    *  maintain Red-Black tree balance  *
    *  after deleting node x            *
    *************************************/

    /* while (    x != *root
            && x->color == BLACK
          )
    {
        if (x == x->parent->left)
        {
            TREE *w = x->parent->right;
            if (w->color == RED)
            {
                w->color = BLACK;
                x->parent->color = RED;
                rotateLeft(root,
                           x->parent);
                w = x->parent->right;
            }
            if (    w->left->color == BLACK
                 && w->right->color == BLACK
               )
            {
                w->color = RED;
                x = x->parent;
            }
            else
            {
                if (w->right->color == BLACK)
                {
                    w->left->color = BLACK;
                    w->color = RED;
                    rotateRight(root,
                                w);
                    w = x->parent->right;
                }
                w->color = x->parent->color;
                x->parent->color = BLACK;
                w->right->color = BLACK;
                rotateLeft(root,
                           x->parent);
                x = *root;
            }
        }
        else
        {
            TREE *w = x->parent->left;
            if (w->color == RED)
            {
                w->color = BLACK;
                x->parent->color = RED;
                rotateRight(root,
                            x->parent);
                w = x->parent->left;
            }
            if (    w->right->color == BLACK
                 && w->left->color == BLACK
               )
            {
                w->color = RED;
                x = x->parent;
            }
            else
            {
                if (w->left->color == BLACK)
                {
                    w->right->color = BLACK;
                    w->color = RED;
                    rotateLeft(root,
                               w);
                    w = x->parent->left;
                }
                w->color = x->parent->color;
                x->parent->color = BLACK;
                w->left->color = BLACK;
                rotateRight(root,
                            x->parent);
                x = *root;
            }
        }
    }
    x->color = BLACK; */
}

/*
 *@@ treeDelete:
 *      removes the specified node from the tree.
 *      Does not free() the node though.
 *
 *      Returns 0 if the node was deleted or
 *      STATUS_INVALID_NODE if not.
 */

int treeDelete(TREE **root,         // in: root of the tree
               TREE *tree)          // in: tree node to delete
{
    TREE        *y,
                *d;
    nodeColor   color;

    if (    (!tree)
         || (tree == LEAF)
       )
        return STATUS_INVALID_NODE;

    if (    (tree->left  == LEAF)
         || (tree->right == LEAF)
       )
        // d has a TREE_NULL node as a child
        d = tree;
    else
    {
        // find tree successor with a TREE_NULL node as a child
        d = tree->right;
        while (d->left != LEAF)
            d = d->left;
    }

    // y is d's only child, if there is one, else TREE_NULL
    if (d->left != LEAF)
        y = d->left;
    else
        y = d->right;

    // remove d from the parent chain
    if (y != LEAF)
        y->parent = d->parent;
    if (d->parent)
    {
        if (d == d->parent->left)
            d->parent->left  = y;
        else
            d->parent->right = y;
    }
    else
        *root = y;

    color = d->color;

    if (d != tree)
    {
        // move the data from d to tree; we do this by
        // linking d into the structure in the place of tree
        d->left   = tree->left;
        d->right  = tree->right;
        d->parent = tree->parent;
        d->color  = tree->color;

        if (d->parent)
        {
            if (tree == d->parent->left)
                d->parent->left  = d;
            else
                d->parent->right = d;
        }
        else
            *root = d;

        if (d->left != LEAF)
            d->left->parent = d;

        if (d->right != LEAF)
            d->right->parent = d;
    }

    if (    (y != LEAF)
         && (color == BLACK)
       )
        deleteFixup(root,
                    y);

    return (STATUS_OK);

    /* TREE    *x,
            *y; */
            // *z;

   /*****************************
    *  delete node z from tree  *
    *****************************/

    // find node in tree
    /* if (lastFind && compEQ(lastFind->key, key))
        // if we just found node, use pointer
        z = lastFind;
    else {
        z = *root;
        while(z != LEAF)
        {
            int iResult = pfnCompare(key, z->key);
            if (iResult == 0)
            // if(compEQ(key, z->key))
                break;
            else
                z = (iResult < 0) // compLT(key, z->key)
                    ? z->left
                    : z->right;
        }
        if (z == LEAF)
            return STATUS_KEY_NOT_FOUND;
    }

    if (    z->left == LEAF
         || z->right == LEAF
       )
    {
        // y has a LEAF node as a child
        y = z;
    }
    else
    {
        // find tree successor with a LEAF node as a child
        y = z->right;
        while (y->left != LEAF)
            y = y->left;
    }

    // x is y's only child
    if (y->left != LEAF)
        x = y->left;
    else
        x = y->right;

    // remove y from the parent chain
    x->parent = y->parent;
    if (y->parent)
        if (y == y->parent->left)
            y->parent->left = x;
        else
            y->parent->right = x;
    else
        *root = x;

    // y is about to be deleted...

    if (y != z)
    {
        // now, the original code simply copied the data
        // from y to z... we can't safely do that since
        // we don't know about the real data in the
        // caller's TREE structure
        z->ulKey = y->ulKey;
        // z->rec = y->rec;         // hope this works...
                                    // the original implementation used rec
                                    // for the node's data

        if (cbStruct > sizeof(TREE))
        {
            memcpy(((char*)&z) + sizeof(TREE),
                   ((char*)&y) + sizeof(TREE),
                   cbStruct - sizeof(TREE));
        }
    }

    if (y->color == BLACK)
        deleteFixup(root,
                    x);

    // free(y);
    // lastFind = NULL;

    return STATUS_OK; */
}

/*
 *@@ treeFind:
 *      finds the tree node with the specified key.
 *      Returns NULL if none exists.
 */

TREE* treeFind(TREE *root,                    // in: root of the tree
               unsigned long key,             // in: key to find
               FNTREE_COMPARE *pfnCompare)    // in: comparison func
{
   /*******************************
    *  find node containing data  *
    *******************************/

    TREE *current = root;
    while (current != LEAF)
    {
        int iResult;
        if (0 == (iResult = pfnCompare(key, current->ulKey)))
            return (current);
        else
        {
            current = (iResult < 0) // compLT (key, current->key)
                ? current->left
                : current->right;
        }
    }

    return 0;
}

/*
 *@@ treeFirst:
 *      finds and returns the first node in a (sub-)tree.
 *
 *      See treeNext for a sample usage for traversing a tree.
 */

TREE* treeFirst(TREE *r)
{
    TREE    *p;

    if (    (!r)
         || (r == LEAF)
       )
        return NULL;

    p = r;
    while (p->left != LEAF)
        p = p->left;

    return p;
}

/*
 *@@ treeLast:
 *      finds and returns the last node in a (sub-)tree.
 */

TREE* treeLast(TREE *r)
{
    TREE    *p;

    if (    (!r)
         || (r == LEAF))
        return NULL;

    p = r;
    while (p->right != LEAF)
        p = p->right;

    return p;
}

/*
 *@@ treeNext:
 *      finds and returns the next node in a tree.
 *
 *      Example for traversing a whole tree:
 *
 +          TREE    *TreeRoot;
 +          ...
 +          TREE* pNode = treeFirst(TreeRoot);
 +          while (pNode)
 +          {
 +              ...
 +              pNode = treeNext(pNode);
 +          }
 *
 *      This runs through the tree items in sorted order.
 */

TREE* treeNext(TREE *r)
{
    TREE    *p,
            *child;

    if (    (!r)
         || (r == LEAF)
       )
        return NULL;

    p = r;
    if (p->right != LEAF)
        return treeFirst (p->right);
    else
    {
        p = r;
        child   = LEAF;
        while (    (p->parent)
                && (p->right == child)
              )
        {
            child = p;
            p = p->parent;
        }
        if (p->right != child)
            return p;
        else
            return NULL;
    }
}

/*
 *@@ treePrev:
 *      finds and returns the previous node in a tree.
 */

TREE* treePrev(TREE *r)
{
    TREE    *p,
            *child;

    if (    (!r)
         || (r == LEAF))
        return NULL;

    p = r;
    if (p->left != LEAF)
        return treeLast (p->left);
    else
    {
        p = r;
        child   = LEAF;
        while ((p->parent)
           &&  (p->left == child))
        {
            child = p;
            p = p->parent;
        }
        if (p->left != child)
            return p;
        else
            return NULL;
    }
}

/*
 *@@ treeBuildArray:
 *      builds an array of TREE* pointers containing
 *      all tree items in sorted order.
 *
 *      This returns a TREE** pointer to the array.
 *      Each item in the array is a TREE* pointer to
 *      the respective tree item.
 *
 *      The array has been allocated using malloc()
 *      and must be free()'d by the caller.
 *
 *      NOTE: This will only work if you maintain a
 *      tree node count yourself, which you must pass
 *      in *pulCount on input.
 *
 *      This is most useful if you want to delete an
 *      entire tree without having to traverse it
 *      and rebalance the tree on every delete.
 *
 *      Example usage for deletion:
 *
 +          TREE    *G_TreeRoot;
 +          treeInit(&G_TreeRoot);
 +
 +          // add stuff to the tree
 +          TREE    *pNewNode = malloc(...);
 +          treeInsert(&G_TreeRoot, pNewNode, fnCompare)
 +
 +          // now delete all nodes
 +          ULONG   cItems = ... // insert item count here
 +          TREE**  papNodes = treeBuildArray(G_TreeRoot,
 +                                            &cItems);
 +          if (papNodes)
 +          {
 +              ULONG ul;
 +              for (ul = 0; ul < cItems; ul++)
 +              {
 +                  TREE *pNodeThis = papNodes[ul];
 +                  free(pNodeThis);
 +              }
 +
 +              free(papNodes);
 +          }
 +
 *
 *@@added V0.9.9 (2001-04-05) [umoeller]
 */

TREE** treeBuildArray(TREE* pRoot,
                      unsigned long *pulCount)  // in: item count, out: array item count
{
    TREE            **papNodes = NULL,
                    **papThis = NULL;
    unsigned long   cb = (sizeof(TREE*) * (*pulCount)),
                    cNodes = 0;

    if (cb)
    {
        papNodes = (TREE**)malloc(cb);
        papThis = papNodes;

        if (papNodes)
        {
            TREE    *pNode = (TREE*)treeFirst(pRoot);

            memset(papNodes, 0, cb);

            // copy nodes to array
            while (    pNode
                    && cNodes < (*pulCount)     // just to make sure
                  )
            {
                *papThis = pNode;
                cNodes++;
                papThis++;

                pNode = (TREE*)treeNext(pNode);
            }

            // output count
            *pulCount = cNodes;
        }
    }

    return (papNodes);
}

/* void main(int argc, char **argv) {
    int maxnum, ct;
    recType rec;
    keyType key;
    statusEnum status;

    maxnum = atoi(argv[1]);

    printf("maxnum = %d\n", maxnum);
    for (ct = maxnum; ct; ct--) {
        key = rand() % 9 + 1;
        if ((status = find(key, &rec)) == STATUS_OK) {
            status = delete(key);
            if (status) printf("fail: status = %d\n", status);
        } else {
            status = insert(key, &rec);
            if (status) printf("fail: status = %d\n", status);
        }
    }
} */
