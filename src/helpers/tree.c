
/*
 *@@sourcefile tree.c:
 *      contains helper functions for maintaining 'Red-Black' balanced
 *      binary trees.
 *      See explanations below.
 *
 *      This file is all new with V0.9.5 (2000-09-29) [umoeller].
 *
 *      Usage: All C programs; not OS/2-specific.
 *
 *      Function prefixes (new with V0.81):
 *      --  tree*    tree helper functions
 *
 *      This has been taken from the Standard Function Library (SFL)
 *      by iMatix Corporation and changed to user the "id" member for
 *      tree sorting/comparison. This implementation is released
 *      under the GPL.
 *
 *      <B>Introduction</B>
 *
 *      Binary trees are different from linked lists in that items
 *      are not simply linked sequentially, but instead put into
 *      a tree-like structure.
 *
 *      For this, the functions here use the TREE structure. You can
 *      easily see that this has the "left" and "right" members,
 *      which make up the tree.
 *
 *      In addition, each tree has a "tree root" item, from which all
 *      other tree nodes can be reached by following the "left" and
 *      "right" pointers.
 *
 *      Per definition, in our trees, if you follow the "left" pointer,
 *      you will reach an item which is "greater than" the current node.
 *      Reversely, following the "right" pointer will lead you to a
 *      node which is "less than" the current node.
 *
 *      The implementation here has the following characteristics:
 *
 *      -- We have "binary" trees. That is, there are only "left" and
 *         "right" pointers.
 *
 *      -- The tree is always "balanced". The tree gets completely
 *         reordered when items are added/removed to ensure that
 *         all paths through the tree are approximately the same
 *         length. This avoids the "worst case" scenario that some
 *         paths grow terribly long while others remain short, which
 *         can make searching very inefficient.
 *
 *      -- The tree nodes are marked as either "red" or "black", which
 *         is an algorithm to allow the implementation of 2-3-4 trees
 *         using a binary tree only. I don't fully understand how this
 *         works, but essentially, "red" nodes represent a 3 or 4 node,
 *         while "black" nodes are plain binary nodes.
 *
 *      As much as I understand about all this, red-black balanced
 *      binary trees are the most efficient tree algorithm known to
 *      mankind. As long as you are sure that trees are more efficient
 *      in your situation than a linked list in the first place (see
 *      below for comparisons), use the functions in here.
 *
 *      <B>Using binary trees</B>
 *
 *      You can use any structure as elements in a tree, provided
 *      that the first member in the structure is a TREE structure
 *      (i.e. it has the left, right, parent, id, and colour members).
 *      The tree functions don't care what follows.
 *
 *      So the implementation here is slightly different from the
 *      linked lists in linklist.c, because the LISTNODE structs
 *      only have pointers to the data. By contrast, the TREE structs
 *      are expected to contain the data themselves. See treeInsertID()
 *      for a sample.
 *
 *      Initialize the root of the tree with treeInit(). Then
 *      add nodes to the tree with treeInsertID() and remove nodes
 *      with treeDelete().
 *
 *      You can test whether a tree is empty by comparing its
 *      root with TREE_NULL.
 *
 *      Most functions in here come in two flavors.
 *
 *      -- You can provide a comparison function and use the "Node"
 *         flavors of these functions. This is useful, for example,
 *         if you are storing strings. You can then write a short
 *         comparison function which does a strcmp() on the data
 *         of tree nodes.
 *
 *         The order of nodes in the tree is determined by calling a
 *         node comparison function provided by the caller
 *         (which you must write). This must be declared as:
 *
 +              int TREEENTRY fnMyCompareNodes(TREE *t1, TREE *t2);
 *
 *         It obviously receives two TREE pointers, which it must
 *         compare and return:
 *
 +               0: tree1 == tree2
 +              -1: tree1 < tree2
 +              +1: tree1 > tree2
 *
 *      -- The "ID" functions (e.g. treeInsertID) do not require
 *         a comparison function, but will use the "id" member of
 *         the TREE structure instead. If this flavor is used, an
 *         internal comparison function is used for comparing the
 *         "id" fields, which are plain ULONGs.
 *
 *      <B>Trees vs. linked lists</B>
 *
 *      Compared to linked lists (as implemented by linklist.c),
 *      trees allow for much faster searching.
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
 *      Example: You need to build a list of files, and you
 *      will search the list frequently according to the file
 *      handles. This would make the handle an ideal "id" field.
 *
 *      Differences compared to linklist.c:
 *
 *      -- Trees are considerably slower when inserting and removing
 *         nodes because the tree has to be rebalanced every time
 *         a node changes. By contrast, trees are much faster finding
 *         nodes because the tree is always sorted.
 *
 *      -- If you are not using the "ID" flavors, you must supply a
 *         comparison function to allow the tree functions to sort the tree.
 *
 *      -- As opposed to a LISTNODE, the TREE structure (which
 *         represents a tree node) does not contain a data pointer,
 *         as said above.
 *
 *@@added V0.9.5 (2000-09-29) [umoeller]
 *@@header "helpers\tree.h"
 */

/*
 *      Written:    97/11/18  Jonathan Schultz <jonathan@imatix.com>
 *      Revised:    98/12/08  Jonathan Schultz <jonathan@imatix.com>
 *
 *      Copyright (C) 1991-99 iMatix Corporation.
 *      Copyright (C) 2000 Ulrich M”ller.
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

//  Constants
TREE    TREE_EMPTY = {TREE_NULL, TREE_NULL, NULL, BLACK};

//  Internal function prototypes
static void insert_fixup(TREE **root, TREE *tree);
static void rotate_left(TREE **root, TREE *tree);
static void rotate_right(TREE **root, TREE *tree);
static void delete_fixup(TREE **root, TREE *tree);

/*
 *@@ treeInit:
 *      initializes an empty tree. The data on the
 *      tree will be invalid, and no memory will be
 *      freed.
 *
 *      Usage:
 +          TREE *TreeRoot;
 +          treeInit(&TreeRoot);
 */

void treeInit(TREE **root)
{
    *root = TREE_NULL;
}

/*
 * fnCompareIDs:
 *
 *added V0.9.9 (2001-02-06) [umoeller]
 */

int TREEENTRY fnCompareIDs(unsigned long id1, unsigned long id2)
{
    if (id1 < id2)
        return -1;
    if (id1 > id2)
        return +1;
    return (0);
}

/*
 *@@ treeInsertID:
 *      inserts a node into an existing tree.
 *
 *      Note: A tree node MUST contain a TREE structure
 *      at the beginning for the tree functions to work.
 *      So to create a tree node with usable data, do this:
 *
 +          typedef _MYTREENODE
 +          {
 +              // TREE must be at beginning
 +              TREE        Tree;
 +              // now use whatever you want
 +              CHAR        szMyExtraData[100];
 +          } MYTREENODE, *PMYTREENODE;
 *
 *      When calling the tree functions, manually cast your
 *      MYTREENODE pointers to (TREE*).
 *
 *      This function initialises the node pointers and colour
 *      in the TREE structure to correct values, so the caller
 *      does not have to worry about those.
 *
 *      However you must initialize the TREE.id member correctly
 *      so that your comparison function can compare on that
 *      to find the correct place in the tree to insert the node.
 *
 *      Usage:
 +          TREE *TreeRoot;
 +          treeInit(&TreeRoot);
 +
 +          PMYTREENODE pTreeItem = malloc(sizeof(MYTREENODE));
 +          pTreeItem->Tree.id = 1;
 +
 +          treeInsertID(&TreeRoot,
 +                     (TREE*)pTreeItem,
 +                     FALSE);
 *
 *      Returns:
 *
 *      -- TREE_OK: OK, item inserted.
 *
 *      -- TREE_DUPLICATE: if (fAllowDuplicates == FALSE), this is
 *          returned if a tree item with the specified ID already
 *          exists.
 *
 *@@changed V0.9.9 (2001-02-06) [umoeller]: removed comparison func
 */

int treeInsertID(TREE **root,             // in: root of tree
                 TREE *tree,              // in: new tree node
                 BOOL fAllowDuplicates)   // in: whether duplicates with the same ID are allowed
{
    TREE    *current,
            *parent;
    int     last_comp = 0;

    // find where node belongs
    current = *root;
    parent  = NULL;
    while (current != TREE_NULL)
    {
        parent  = current;
        last_comp = fnCompareIDs(tree->id, current->id);
        switch (last_comp)
        {
            case -1: current = current->left;  break;
            case  1: current = current->right; break;
            default:
                if (fAllowDuplicates)
                    current = current->left;
                else
                    return TREE_DUPLICATE;
        }
    }

    // set up new node
    ((TREE*)tree)->parent = parent;
    ((TREE*)tree)->left   = TREE_NULL;
    ((TREE*)tree)->right  = TREE_NULL;
    ((TREE*)tree)->colour = RED;

    // insert node in tree
    if (parent)
        switch (last_comp)
        {
            case  1: parent->right = tree;  break;
            default: parent->left  = tree;
        }
    else
        *root = tree;

    insert_fixup(root, tree);
    return(TREE_OK);
}

/*
 *@@ treeInsertNode:
 *      similar to treeInsertID, but this uses
 *      a comparision function which compares
 *      nodes.
 */

int treeInsertNode(TREE **root,             // in: root of tree
                   TREE *tree,              // in: new tree node
                   FNTREE_COMPARE_NODES *comp,  // in: comparison function
                   BOOL fAllowDuplicates)   // in: whether duplicates with the same ID are allowed
{
    TREE
       *current,
       *parent;
    int
        last_comp = 0;

    // find where node belongs
    current = *root;
    parent  = NULL;
    while (current != TREE_NULL)
    {
        parent  = current;
        last_comp = comp(tree, current);
        switch (last_comp)
        {
            case -1: current = current->left;  break;
            case  1: current = current->right; break;
            default: if (fAllowDuplicates)
                         current = current->left;
                     else
                         return TREE_DUPLICATE;

        }
    }

    // set up new node
    ((TREE*)tree)->parent = parent;
    ((TREE*)tree)->left   = TREE_NULL;
    ((TREE*)tree)->right  = TREE_NULL;
    ((TREE*)tree)->colour = RED;

    // insert node in tree
    if (parent)
        switch (last_comp)
        {
            case  1: parent->right = tree;  break;
            default: parent->left  = tree;
        }
    else
        *root = tree;

    insert_fixup(root, tree);
    return(TREE_OK);
}

/*
 * insert_fixup:
 *      maintains the Red-Black tree balance after a node has been inserted.
 *
 *      Private function.
 */

static void insert_fixup(TREE **root,
                         TREE *tree)
{
    TREE *uncle;

    // check red-black properties
    while ((tree != *root)
       &&  (tree->parent->colour == RED))
    {
        // we have a violation
        if (tree->parent == tree->parent->parent->left)
        {
            uncle = tree->parent->parent->right;
            if (uncle->colour == RED)
            {
                // uncle is RED
                tree ->parent->colour = BLACK;
                uncle->colour = BLACK;
                tree ->parent->parent->colour = RED;

                tree = tree->parent->parent;
            }
            else
            {
                // uncle is BLACK
                if (tree == tree->parent->right)
                {
                    // make tree a left child
                    tree = tree->parent;
                    rotate_left (root, tree);
                }

                // recolor and rotate
                tree->parent->colour = BLACK;
                tree->parent->parent->colour = RED;
                rotate_right (root, tree->parent->parent);
            }
        }
        else
        {
            // mirror image of above code
            uncle = tree->parent->parent->left;
            if (uncle->colour == RED)
            {
                // uncle is RED
                tree ->parent->colour = BLACK;
                uncle->colour = BLACK;
                tree ->parent->parent->colour = RED;

                tree = tree->parent->parent;
            }
            else
            {
                // uncle is BLACK
                if (tree == tree->parent->left)
                {
                    tree = tree->parent;
                    rotate_right (root, tree);
                }
                tree->parent->colour = BLACK;
                tree->parent->parent->colour = RED;
                rotate_left (root, tree->parent->parent);
            }
        }
    }
    (*root)->colour = BLACK;
}

/*
 * rotate_left:
 *      rotates tree to left.
 *
 *      Private function.
 */

static void rotate_left(TREE **root,
                        TREE *tree)
{
    TREE *other = tree->right;

    // establish tree->right link
    tree->right = other->left;
    if (other->left != TREE_NULL)
        other->left->parent = tree;

    // establish other->parent link
    if (other != TREE_NULL)
        other->parent = tree->parent;

    if (tree->parent)
    {
        if (tree == tree->parent->left)
            tree->parent->left  = other;
        else
            tree->parent->right = other;
    }
    else
        *root = other;

    // link tree and other
    other->left = tree;
    if (tree != TREE_NULL)
        tree->parent = other;
}

/*
 * rotate_right:
 *      rotates tree to right.
 *
 *      Private function.
 */

static void rotate_right(TREE **root,
                         TREE *tree)
{
    TREE *other;

    other = tree->left;

    // establish tree->left link
    tree->left = other->right;
    if (other->right != TREE_NULL)
        other->right->parent = tree;

    // establish other->parent link
    if (other != TREE_NULL)
        other->parent = tree->parent;

    if (tree->parent)
    {
        if (tree == tree->parent->right)
            tree->parent->right = other;
        else
            tree->parent->left  = other;
    }
    else
        *root = other;

    // link tree and other
    other->right = tree;
    if (tree != TREE_NULL)
        tree->parent = other;
}

/*
 *@@ treeDelete:
 *      deletes a node from a tree. Does not deallocate any memory.
 *
 *      Returns:
 *
 *      -- TREE_OK: node deleted.
 *      -- TREE_INVALID_NODE: tree node not found.
 */

int treeDelete(TREE **root,     // in: root of tree
               TREE *tree)      // in: tree node to delete
{
    int irc = TREE_OK;

    TREE
       *youngest, *descendent;
    TREE_COLOUR
        colour;

    if (    (!tree)
         || (tree == TREE_NULL)
       )
        return TREE_INVALID_NODE;

    if (    (((TREE*)tree)->left  == TREE_NULL)
         || (((TREE*)tree)->right == TREE_NULL)
       )
        // descendent has a TREE_NULL node as a child
        descendent = tree;
    else
  {
        // find tree successor with a TREE_NULL node as a child
        descendent = ((TREE*)tree)->right;
        while (descendent->left != TREE_NULL)
            descendent = descendent->left;
  }

    // youngest is descendent's only child, if there is one, else TREE_NULL
    if (descendent->left != TREE_NULL)
        youngest = descendent->left;
    else
        youngest = descendent->right;

    // remove descendent from the parent chain
    if (youngest != TREE_NULL)
        youngest->parent = descendent->parent;
    if (descendent->parent)
    {
        if (descendent == descendent->parent->left)
            descendent->parent->left  = youngest;
        else
            descendent->parent->right = youngest;
    }
    else
        *root = youngest;

    colour = descendent->colour;

    if (descendent != (TREE *) tree)
    {
        // Conceptually what we are doing here is moving the data from
        // descendent to tree.  In fact we do this by linking descendent
        // into the structure in the place of tree.
        descendent->left   = ((TREE*)tree)->left;
        descendent->right  = ((TREE*)tree)->right;
        descendent->parent = ((TREE*)tree)->parent;
        descendent->colour = ((TREE*)tree)->colour;

        if (descendent->parent)
        {
            if (tree == descendent->parent->left)
                descendent->parent->left  = descendent;
            else
                descendent->parent->right = descendent;
        }
        else
            *root = descendent;

        if (descendent->left != TREE_NULL)
            descendent->left->parent = descendent;

        if (descendent->right != TREE_NULL)
            descendent->right->parent = descendent;
    }

    if (    (youngest != TREE_NULL)
        &&  (colour   == BLACK))
        delete_fixup (root, youngest);

    return (irc);
}

/*
 *@@ delete_fixup:
 *      maintains Red-Black tree balance after deleting a node.
 *
 *      Private function.
 */

static void delete_fixup(TREE **root,
                         TREE *tree)
{
    TREE
       *sibling;

    while (tree != *root && tree->colour == BLACK)
    {
        if (tree == tree->parent->left)
        {
            sibling = tree->parent->right;
            if (sibling->colour == RED)
            {
                sibling->colour = BLACK;
                tree->parent->colour = RED;
                rotate_left (root, tree->parent);
                sibling = tree->parent->right;
            }
            if ((sibling->left->colour == BLACK)
            &&  (sibling->right->colour == BLACK))
            {
                sibling->colour = RED;
                tree = tree->parent;
            }
            else
            {
                if (sibling->right->colour == BLACK)
                {
                    sibling->left->colour = BLACK;
                    sibling->colour = RED;
                    rotate_right (root, sibling);
                    sibling = tree->parent->right;
                }
                sibling->colour = tree->parent->colour;
                tree->parent->colour = BLACK;
                sibling->right->colour = BLACK;
                rotate_left (root, tree->parent);
                tree = *root;
            }
        }
        else
        {
            sibling = tree->parent->left;
            if (sibling->colour == RED)
            {
                sibling->colour = BLACK;
                tree->parent->colour = RED;
                rotate_right (root, tree->parent);
                sibling = tree->parent->left;
            }
            if ((sibling->right->colour == BLACK)
            &&  (sibling->left->colour == BLACK))
            {
                sibling->colour = RED;
                tree = tree->parent;
            }
            else
            {
                if (sibling->left->colour == BLACK)
                {
                    sibling->right->colour = BLACK;
                    sibling->colour = RED;
                    rotate_left (root, sibling);
                    sibling = tree->parent->left;
                }
                sibling->colour = tree->parent->colour;
                tree->parent->colour = BLACK;
                sibling->left->colour = BLACK;
                rotate_right (root, tree->parent);
                tree = *root;
            }
        }
    }
    tree->colour = BLACK;
}

/*
 *@@ treeFindEQID:
 *      finds a node with ID exactly matching that provided.
 *      Returns NULL if not found.
 */

void* treeFindEQID(TREE **root,
                   unsigned long id)
{
    TREE
       *current = *root,
       *found = NULL;

    while (current != TREE_NULL)
        switch (fnCompareIDs(current->id, id))
        {
            case -1: current = current->right; break;
            case  1: current = current->left;  break;
            default: found = current;           //  In case of duplicates,
                     current = current->left;  //  get the first one.
        }

    return found;
}

/*
 *@@ treeFindGEID:
 *      finds a node with ID greater than or equal to provided.
 *      To find a tree node, your comparison function must
 *      compare the tree node IDs.
 */

void* treeFindGEID(TREE **root,
                   unsigned long idFind)
{
    TREE
       *current = *root,
       *found;

    found = NULL;
    while (current != TREE_NULL)
        switch (fnCompareIDs(current->id, idFind))
        {
            case -1: current = current->right; break;
            default: found = current;
                     current = current->left;
        }

    return found;
}

/*
 *@@ treeFindEQNode:
 *      finds a node with ID exactly matching that provided.
 *      To find a tree node, your comparison function must
 *      compare the tree nodes.
 */

void* treeFindEQNode(TREE **root,
                     TREE *nodeFind,
                     FNTREE_COMPARE_NODES *comp)
{
    TREE
       *current = *root,
       *found;

    found = NULL;
    while (current != TREE_NULL)
        switch (comp(current, nodeFind))
        {
            case -1: current = current->right; break;
            case  1: current = current->left;  break;
            default: found = current;           //  In case of duplicates,
                     current = current->left;  //  get the first one.
        }

    return found;
}

/*
 *@@ treeFindGENode:
 *      finds a node with ID greater than or equal to provided.
 *      To find a tree node, your comparison function must
 *      compare the tree nodes.
 */

void* treeFindGENode(TREE **root,
                     TREE *nodeFind,
                     FNTREE_COMPARE_NODES *comp)
{
    TREE
       *current = *root,
       *found;

    found = NULL;
    while (current != TREE_NULL)
        switch (comp(current, nodeFind))
        {
            case -1: current = current->right; break;
            default: found = current;
                     current = current->left;
        }

    return found;
}

/*
 *@@ treeFindLTNode:
 *      finds a node with Node less than provided.
 *      To find a tree node, your comparison function must
 *      compare the tree nodes.
 */

void* treeFindLTNode(TREE **root,
                     TREE *nodeFind,
                     FNTREE_COMPARE_NODES *comp)
{
    TREE
       *current = *root,
       *found;

    found = NULL;
    while (current != TREE_NULL)
        switch (comp(current, nodeFind))
        {
            case -1: found = current;
                     current = current->right; break;
            default: current = current->left;
        }

    return found;
}

/*
 *@@ treeFindLENode:
 *      finds a node with Node less than or equal to provided.
 *      To find a tree node, your comparison function must
 *      compare the tree nodes.
 */

void* treeFindLENode(TREE **root,
                     TREE *nodeFind,
                     FNTREE_COMPARE_NODES *comp)
{
    TREE
       *current = *root,
       *found;

    found = NULL;
    while (current != TREE_NULL)
        switch (comp(current, nodeFind))
        {
            case 1 : current = current->left;  break;
            default: found = current;
                     current = current->right;
        }

    return found;
}

/*
 *@@ treeFindGTNode:
 *      finds a node with Node greater than provided.
 *      To find a tree node, your comparison function must
 *      compare the tree nodes.
 */

void* treeFindGTNode(TREE **root,
                     TREE *nodeFind,
                     FNTREE_COMPARE_NODES *comp)
{
    TREE
       *current = *root,
       *found;

    found = NULL;
    while (current != TREE_NULL)
        switch (comp(current, nodeFind))
        {
            case 1 : found = current;
                     current = current->left; break;
            default: current = current->right;
        }

    return found;
}

/*
 *@@ treeFindEQID:
 *      finds a node with data exactly matching that provided.
 *      To find a tree node, your comparison function must
 *      compare a tree member with external data.
 *
 *      This is useful for finding a tree item from a string ID.
 *
 *      Make sure to use treeInsertNode and compare according
 *      to a string member, and then write a second compare
 *      function for this function which compares the string
 *      member to an external string.
 */

void* treeFindEQData(TREE **root,
                     void *pData,
                     FNTREE_COMPARE_DATA *comp)
{
    TREE    *current = *root,
            *found = NULL;

    while (current != TREE_NULL)
        switch (comp(current, pData))
        {
            case -1: current = current->right; break;
            case  1: current = current->left;  break;
            default: found = current;           //  In case of duplicates,
                     current = current->left;  //  get the first one.
        }

    return found;
}

/*
 *@@ treeTraverse:
 *      traverses the specified tree, calling a processing function
 *      for each tree node.
 *
 *      The processing function ("process") must be declared as
 *      follows:
 *
 +          void fnProcess(TREE *t,         // current tree node
 +                         void *pUser);    // user data
 *
 *      and will receive the "pUser" parameter, which you can use
 *      as a data pointer to some structure for whatever you like.
 *
 *      WARNING: This function recurses and can use up a lot of
 *      stack. For very deep trees, traverse the tree using
 *      treeFirst and treeNext instead. See treeNext for a sample.
 *
 *      "method" specifies in which order the nodes are traversed.
 *      This can be:
 *
 *      -- 1: current node first, then left node, then right node.
 *      -- 2: left node first, then right node, then current node.
 *      -- 0 or other: left node first, then current node, then right node.
 *           This is the sorted order.
 */

void treeTraverse(TREE *tree,               // in: root of tree
                  TREE_PROCESS *process,    // in: callback for each node
                  void *pUser,              // in: user param for callback
                  int method)               // in: traversal mode
{
    if (    (!tree)
         || (tree == TREE_NULL))
        return;

    if (method == 1)
    {
        process(tree, pUser);
        treeTraverse (((TREE*)tree)->left,  process, pUser, method);
        treeTraverse (((TREE*)tree)->right, process, pUser, method);
    }
    else if (method == 2)
    {
        treeTraverse (((TREE*)tree)->left,  process, pUser, method);
        treeTraverse (((TREE*)tree)->right, process, pUser, method);
        process(tree, pUser);
    }
    else
    {
        treeTraverse (((TREE*)tree)->left,  process, pUser, method);
        process(tree, pUser);
        treeTraverse (((TREE*)tree)->right, process, pUser, method);
    }
}

/*
 *@@ treeFirst:
 *      finds and returns the first node in a (sub-)tree.
 *
 *      See treeNext for a sample usage for traversing a tree.
 */

void* treeFirst(TREE *tree)
{
    TREE
       *current;

    if (    (!tree)
         || (tree == TREE_NULL)
       )
        return NULL;

    current = tree;
    while (current->left != TREE_NULL)
        current = current->left;

    return current;
}

/*
 *@@ treeLast:
 *      finds and returns the last node in a (sub-)tree.
 */

void* treeLast(TREE *tree)
{
    TREE
       *current;

    if (    (!tree)
         || (tree == TREE_NULL))
        return NULL;

    current = tree;
    while (current->right != TREE_NULL)
        current = current->right;

    return current;
}

/*
 *@@ treeNext:
 *      finds and returns the next node in a tree.
 *
 *      Example for traversing a whole tree if you don't
 *      want to use treeTraverse:
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

void* treeNext(TREE *tree)
{
    TREE
       *current,
       *child;

    if (    (!tree)
         || (tree == TREE_NULL)
       )
        return NULL;

    current = tree;
    if (current->right != TREE_NULL)
        return treeFirst (current->right);
    else
    {
        current = tree;
        child   = TREE_NULL;
        while (    (current->parent)
                && (current->right == child)
              )
        {
            child = current;
            current = current->parent;
        }
        if (current->right != child)
            return current;
        else
            return NULL;
    }
}

/*
 *@@ treePrev:
 *      finds and returns the previous node in a tree.
 */

void* treePrev(TREE *tree)
{
    TREE
       *current,
       *child;

    if (    (!tree)
         || (tree == TREE_NULL))
        return NULL;

    current = tree;
    if (current->left != TREE_NULL)
        return treeLast (current->left);
    else
    {
        current = tree;
        child   = TREE_NULL;
        while ((current->parent)
           &&  (current->left == child))
        {
            child = current;
            current = current->parent;
        }
        if (current->left != child)
            return current;
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
 +          treeInsertID(&G_TreeRoot, pNewNode, FALSE)
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


