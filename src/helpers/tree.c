
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
 *      tree sorting/comparison.
 *
 *      <B>Using binary trees</B>
 *
 *      Each TREE node does not only contain data, but also an
 *      "id" field. The order of nodes in the tree is determined
 *      by calling a node comparison function provided by the caller
 *      (which you must write). This takes two "id" values and must
 *      return:
 *
 +           0: id1 == id2
 +          -1: id1 < id2
 +          +1: id1 > id2
 *
 *      Initialize the root of the tree with treeInit(). Then
 *      add nodes to the tree with treeInsertID() and remove nodes
 *      with treeDelete().
 *
 *      You can test whether a tree is empty by comparing its
 *      root with TREE_NULL.
 *
 *      Compared to linked lists (as implemented by linklist.c),
 *      trees allow for much faster searching.
 *
 *      Assuming a linked list contains N items, then searching the
 *      list for an item will take an average of N/2 comparisons
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
 *      -- As opposed to a LISTNODE, the TREE structure (which
 *         represents a tree node) does not contain a data pointer.
 *         Instead, all tree nodes are assumed to contain the
 *         data themselves. As a result, you must define your
 *         own structures which start with a TREE structure.
 *
 *         See treeInsertID() for samples.
 *
 *      -- You must supply a comparison function to allow the
 *         tree functions to sort the tree.
 *
 *@@added V0.9.5 (2000-09-29) [umoeller]
 *@@header "helpers\tree.h"
 */

/*
 *      Written:    97/11/18  Jonathan Schultz <jonathan@imatix.com>
 *      Revised:    98/12/08  Jonathan Schultz <jonathan@imatix.com>
 *
 *      Copyright (C) 1991-99 iMatix Corporation.
 *      Copyright (C) 2000 Ulrich M�ller.
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

/*
 *@@category: Helpers\C helpers\Red-black balanced binary trees
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
 +                     fnCompare,       // your comparison function
 +                     FALSE);
 *
 *      Returns:
 *
 *      -- TREE_OK: OK, item inserted.
 *
 *      -- TREE_DUPLICATE: if (fAllowDuplicates == FALSE), this is
 *          returned if a tree item with the specified ID already
 *          exists.
 */

int treeInsertID(TREE **root,             // in: root of tree
                 TREE *tree,              // in: new tree node
                 FNTREE_COMPARE_IDS *comp,      // in: comparison function
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
        last_comp = comp(tree->id, current->id);
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

    // setup new node
    ((TREE *) tree)->parent = parent;
    ((TREE *) tree)->left   = TREE_NULL;
    ((TREE *) tree)->right  = TREE_NULL;
    ((TREE *) tree)->colour = RED;

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

    // setup new node
    ((TREE *) tree)->parent = parent;
    ((TREE *) tree)->left   = TREE_NULL;
    ((TREE *) tree)->right  = TREE_NULL;
    ((TREE *) tree)->colour = RED;

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

    if (    (((TREE *) tree)->left  == TREE_NULL)
         || (((TREE *) tree)->right == TREE_NULL)
       )
        // descendent has a TREE_NULL node as a child
        descendent = tree;
    else
  {
        // find tree successor with a TREE_NULL node as a child
        descendent = ((TREE *) tree)->right;
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
        if (descendent == descendent->parent->left)
            descendent->parent->left  = youngest;
        else
            descendent->parent->right = youngest;
    else
        *root = youngest;

    colour = descendent->colour;

    if (descendent != (TREE *) tree)
  {
        // Conceptually what we are doing here is moving the data from
        // descendent to tree.  In fact we do this by linking descendent
        // into the structure in the place of tree.
        descendent->left   = ((TREE *) tree)->left;
        descendent->right  = ((TREE *) tree)->right;
        descendent->parent = ((TREE *) tree)->parent;
        descendent->colour = ((TREE *) tree)->colour;

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
 *      To find a tree node, your comparison function must
 *      compare the tree node IDs.
 */

void* treeFindEQID(TREE **root,
                   unsigned long id,
                   FNTREE_COMPARE_IDS *comp)
{
    TREE
       *current = *root,
       *found;

    found = NULL;
    while (current != TREE_NULL)
        switch (comp(current->id, id))
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
                   unsigned long idFind,
                   FNTREE_COMPARE_IDS *comp)
{
    TREE
       *current = *root,
       *found;

    found = NULL;
    while (current != TREE_NULL)
        switch (comp(current->id, idFind))
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
 */

void treeTraverse(TREE *tree,
                  TREE_PROCESS *process,
                  void *pUser,
                  int method)
{
    if ((!tree)
    ||  (tree == TREE_NULL))
        return;

    if (method == 1)
    {
        process(tree, pUser);
        treeTraverse (((TREE *) tree)->left,  process, pUser, method);
        treeTraverse (((TREE *) tree)->right, process, pUser, method);
    }
    else if (method == 2)
    {
        treeTraverse (((TREE *) tree)->left,  process, pUser, method);
        treeTraverse (((TREE *) tree)->right, process, pUser, method);
        process(tree, pUser);
    }
    else
    {
        treeTraverse (((TREE *) tree)->left,  process, pUser, method);
        process(tree, pUser);
        treeTraverse (((TREE *) tree)->right, process, pUser, method);
    }
}

/*
 *@@ treeFirst:
 *      finds and returns the first node in a (sub-)tree.
 */

void* treeFirst(TREE *tree)
{
    TREE
       *current;

    if ((!tree)
    ||  (tree == TREE_NULL))
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

    if ((!tree)
    ||  (tree == TREE_NULL))
        return NULL;

    current = tree;
    while (current->right != TREE_NULL)
        current = current->right;

    return current;
}

/*
 *@@ treeNext:
 *      finds and returns the next node in a tree.
 */

void* treeNext(TREE *tree)
{
    TREE
       *current,
       *child;

    if ((!tree)
    ||  (tree == TREE_NULL))
        return NULL;

    current = tree;
    if (current->right != TREE_NULL)
        return treeFirst (current->right);
    else
    {
        current = tree;
        child   = TREE_NULL;
        while ((current->parent)
           &&  (current->right == child))
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

    if ((!tree)
    ||  (tree == TREE_NULL))
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

