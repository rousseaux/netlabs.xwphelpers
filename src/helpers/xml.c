
/*
 *@@sourcefile xml.c:
 *      XML document handling.
 *
 *      XML support in the XWorkplace Helpers is broken into two
 *      layers:
 *
 *      --  The bottom layer is implemented by @expat, which I have
 *          ported and hacked to the xwphelpers. See xmlparse.c for
 *          an introduction.
 *
 *      --  Because expat requires so many callbacks and is non-validating,
 *          I have added a top layer above the expat library
 *          which is vaguely modelled after the Document Object Model
 *          (DOM) standardized by the W3C. That's this file.
 *
 *      <B>XML</B>
 *
 *      In order to understand XML myself, I have written a couple of
 *      glossary entries for the complex XML terminology. See @XML
 *      for a start.
 *
 *      <B>Document Object Model (DOM)</B>
 *
 *      See @DOM for a general introduction.
 *
 *      DOM really calls for object oriented programming so the various
 *      structs can inherit from each other. Since this implementation
 *      was supposed to be a C-only interface, we cannot implement
 *      inheritance at the language level. Instead, each XML document
 *      is broken up into a tree of node structures only (see _DOMNODE),
 *      each of which has a special type. The W3C DOM allows this
 *      (and calls this the "flattened" view, as opposed to the
 *      "inheritance view").
 *
 *      The W3C DOM specification prescribes tons of methods, which I
 *      really had no use for, so I didn't implement them. This implementation
 *      is only a DOM insofar as it uses nodes which represent @documents,
 *      @elements, @attributes, @comments, and @processing_instructions.
 *
 *      Most notably, there are the following differences:
 *
 *      --  Not all node types are implemented. See _DOMNODE for
 *          the supported types.
 *
 *      --  Only a small subset of the standardized methods is implemented,
 *          and they are called differently to adhere to the xwphelpers
 *          conventions.
 *
 *      --  DOM uses UTF-16 for its DOMString type. @expat gives UTF-8
 *          strings to all the handlers though, so all data in the DOM nodes
 *          is UTF-8 encoded. This still needs to be fixed.
 *
 *      --  DOM defines the DOMException class. This isn't supported in C.
 *          Instead, we use special error codes which add to the standard
 *          OS/2 error codes (APIRET). All our error codes are >= 40000
 *          to avoid conflicts.
 *
 *      It shouldn't be too difficult to write a C++ encapsulation
 *      of this though which fully implements all the DOM methods.
 *
 *      However, we do implement node management as in the standard.
 *      See xmlCreateNode and xmlDeleteNode.
 *
 *      The main entry point into this is xmlCreateDOM. See remarks
 *      there for details.
 *
 *      <B>Validation</B>
 *
 *      @expat doesn't check XML documents for whether they are @valid.
 *      In other words, expat is a non-validating XML processor.
 *
 *      By contrast, this pseudo-DOM implementation can validate. To
 *      do this, you must pass DF_PARSEDTD to xmlCreateDOM (otherwise
 *      the @DTD entries will not be stored in the DOM nodes). This
 *      will not validate yet; to do this, explicitly call xmlValidate.
 *
 *@@header "helpers\xml.h"
 *@@added V0.9.6 (2000-10-29) [umoeller]
 */

/*
 *      Copyright (C) 2000-2001 Ulrich M”ller.
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

#define OS2EMX_PLAIN_CHAR
    // this is needed for "os2emx.h"; if this is defined,
    // emx will define PSZ as _signed_ char, otherwise
    // as unsigned char

#define INCL_DOSERRORS
#include <os2.h>

#include <stdlib.h>
#include <string.h>

#include "setup.h"                      // code generation and debugging options

#include "expat\expat.h"

#include "helpers\linklist.h"
#include "helpers\stringh.h"
#include "helpers\tree.h"
#include "helpers\xstring.h"
#include "helpers\xml.h"

#pragma hdrstop

/*
 *@@category: Helpers\C helpers\XML
 *      see xml.c.
 */

/*
 *@@category: Helpers\C helpers\XML\Document Object Model (DOM)
 *      see xml.c.
 */

/* ******************************************************************
 *
 *   Generic methods
 *
 ********************************************************************/

/*
 *@@ CompareCMNodeNodes:
 *      tree comparison func for CMNodes.
 *      This works for all trees which contain structures
 *      whose first item is a _NODEBASE because NODEBASE's first
 *      member is a TREE.
 *
 *      Used in two places:
 *
 *      --  to insert _CMELEMENTDECLNODE nodes into
 *          _DOMDOCTYPENODE.ElementDeclsTree;
 *
 *      --  to insert _CMELEMENTPARTICLE nodes into
 *          _CMELEMENTDECLNODE.ElementNamesTree.
 *
 *@@added V0.9.9 (2001-02-16) [umoeller]
 */

int CompareCMNodeNodes(TREE *t1,
                       TREE *t2)
{
    PNODEBASE   p1 = (PNODEBASE)t1,
                p2 = (PNODEBASE)t2;
    return (strhcmp(p1->strNodeName.psz, p2->strNodeName.psz));
}

/*
 *@@ CompareCMNodeNodes:
 *      tree comparison func for element declarations.
 *      Used to find nodes in _DOMDOCTYPENODE.ElementDeclsTree.
 *
 *@@added V0.9.9 (2001-02-16) [umoeller]
 */

int CompareCMNodeData(TREE *t1,
                           void *pData)
{
    PNODEBASE     p1 = (PNODEBASE)t1;
    return (strhcmp(p1->strNodeName.psz, (const char*)pData));
}

/*
 *@@ xmlCreateNode:
 *      creates a new DOMNODE with the specified
 *      type and parent. Other than that, the
 *      node fields are zeroed.
 *
 *      If pParentNode is specified (which is required,
 *      unless you are creating a document node),
 *      its children list is automatically updated
 *      (unless this is an attribute node, which updates
 *      the attributes map).
 *
 *      This returns the following errors:
 *
 *      --  ERROR_NOT_ENOUGH_MEMORY
 *
 *      --  ERROR_DOM_NOT_SUPPORTED: invalid ulNodeType
 *          specified.
 *
 *      --  ERROR_DOM_WRONG_DOCUMENT: cannot find the
 *          document for this node. This happens if you do
 *          not have a document node at the root of your tree.
 */

APIRET xmlCreateNode(PDOMNODE pParentNode,        // in: parent node or NULL if root
                     ULONG ulNodeType,            // in: DOMNODE_* type
                     PDOMNODE *ppNew)             // out: new node
{
    PDOMNODE pNewNode = NULL;
    APIRET  arc = NO_ERROR;

    ULONG   cb = 0;

    switch (ulNodeType)
    {
        case DOMNODE_DOCUMENT:
            cb = sizeof(DOMDOCUMENTNODE);
        break;

        case DOMNODE_DOCUMENT_TYPE:
            cb = sizeof(DOMDOCTYPENODE);
        break;

        default:
            cb = sizeof(DOMNODE);
        break;
    }

    pNewNode = (PDOMNODE)malloc(cb);

    if (!pNewNode)
        arc = ERROR_NOT_ENOUGH_MEMORY;
    else
    {
        memset(pNewNode, 0, cb);
        pNewNode->NodeBase.ulNodeType = ulNodeType;
        xstrInit(&pNewNode->NodeBase.strNodeName, 0);
        pNewNode->pParentNode = pParentNode;

        if (pParentNode)
        {
            // parent specified:
            // check if this is an attribute
            if (ulNodeType == DOMNODE_ATTRIBUTE)
            {
                // attribute:
                // add to parent's attributes list
                if (treeInsertNode(&pParentNode->AttributesMap,
                                   (TREE*)pNewNode,
                                   CompareCMNodeNodes,
                                   FALSE)      // no duplicates
                        == TREE_DUPLICATE)
                    arc = ERROR_DOM_DUPLICATE_ATTRIBUTE;
                                // shouldn't happen, because expat takes care of this
            }
            else
                // append this new node to the parent's
                // list of child nodes
                lstAppendItem(&pParentNode->llChildren,
                              pNewNode);

            if (!arc)
            {
                // set document pointer...
                // if the parent node has a document pointer,
                // we can copy that
                if (pParentNode->pDocumentNode)
                    pNewNode->pDocumentNode = pParentNode->pDocumentNode;
                else
                    // parent has no document pointer: then it is probably
                    // the document itself... check
                    if (pParentNode->NodeBase.ulNodeType == DOMNODE_DOCUMENT)
                        pNewNode->pDocumentNode = pParentNode;
                    else
                        arc = ERROR_DOM_NO_DOCUMENT;
            }
        }

        lstInit(&pNewNode->llChildren, FALSE);
        treeInit(&pNewNode->AttributesMap);
    }

    if (!arc)
        *ppNew = pNewNode;
    else
        if (pNewNode)
            free(pNewNode);

    return (arc);
}

/*
 *@@ xmlDeleteNode:
 *      deletes the specified node and updates the
 *      parent node's children list.
 *
 *      If the node has child nodes, all of them are deleted
 *      as well. This recurses, if necessary.
 *
 *      As a result, if the node is a document node, this
 *      deletes an entire document, including all of its
 *      child nodes.
 *
 *      This returns the following errors:
 *
 *      -- ERROR_DOM_NOT_FOUND
 */

APIRET xmlDeleteNode(PDOMNODE pNode)
{
    ULONG ulrc = 0;

    if (!pNode)
    {
        ulrc = ERROR_INVALID_PARAMETER;
    }
    else
    {
        PLISTNODE   pNodeThis;

        // recurse into child nodes
        while (pNodeThis  = lstQueryFirstNode(&pNode->llChildren))
            // recurse!!
            xmlDeleteNode((PDOMNODE)(pNodeThis->pItemData));
                        // this updates llChildren

        // recurse into attribute nodes
        // while (pNodeThis = lstQueryFirstNode(&pNode->llAttributes))
            // recurse!! ###
            // xmlDeleteNode((PDOMNODE)(pNodeThis->pItemData));
                        // this updates llAttributes

        if (pNode->pParentNode)
        {
            // node has a parent:
            if (pNode->NodeBase.ulNodeType == DOMNODE_ATTRIBUTE)
                // this is an attribute:
                // remove from parent's attributes map
                treeDelete(&pNode->pParentNode->AttributesMap,
                           (TREE*)pNode);
            else
                // remove this node from the parent's list
                // of child nodes before deleting this node
                lstRemoveItem(&pNode->pParentNode->llChildren,
                              pNode);

            pNode->pParentNode = NULL;
        }

        xstrClear(&pNode->NodeBase.strNodeName);
        xstrFree(pNode->pstrNodeValue);

        lstClear(&pNode->llChildren);
        // lstClear(&pNode->llAttributes); ###

        free(pNode);
    }

    return (ulrc);
}

/*
 *@@ xmlGetFirstChild:
 *      returns the first child node of pDomNode.
 *      See _DOMNODE for what a "child" can be for the
 *      various node types.
 *
 *@@added V0.9.9 (2001-02-14) [umoeller]
 */

PDOMNODE xmlGetFirstChild(PDOMNODE pDomNode)
{
    PLISTNODE pListNode = lstQueryFirstNode(&pDomNode->llChildren);
    if (pListNode)
        return ((PDOMNODE)pListNode->pItemData);

    return (0);
}

/*
 *@@ xmlGetLastChild:
 *      returns the last child node of pDomNode.
 *      See _DOMNODE for what a "child" can be for the
 *      various node types.
 *
 *@@added V0.9.9 (2001-02-14) [umoeller]
 */

PDOMNODE xmlGetLastChild(PDOMNODE pDomNode)
{
    PLISTNODE pListNode = lstQueryLastNode(&pDomNode->llChildren);
    if (pListNode)
        return ((PDOMNODE)pListNode->pItemData);

    return (0);
}

/*
 *@@ xmlDescribeError:
 *      returns a string describing the error corresponding to code.
 *      The code should be one of the enums that can be returned from
 *      XML_GetErrorCode.
 *
 *@@changed V0.9.9 (2001-02-14) [umoeller]: adjusted for new error codes
 *@@changed V0.9.9 (2001-02-16) [umoeller]: moved this here from xmlparse.c
 */

const char* xmlDescribeError(int code)
{
    static const char *message[] =
    {
        // start of expat (parser) errors
        "Out of memory",
        "Syntax error",
        "No element found",
        "Not well-formed (invalid token)",
        "Unclosed token",
        "Unclosed token",
        "Mismatched tag",
        "Duplicate attribute",
        "Junk after root element",
        "Illegal parameter entity reference",
        "Undefined entity",
        "Recursive entity reference",
        "Asynchronous entity",
        "Reference to invalid character number",
        "Reference to binary entity",
        "Reference to external entity in attribute",
        "XML processing instruction not at start of external entity",
        "Unknown encoding",
        "Encoding specified in XML declaration is incorrect",
        "Unclosed CDATA section",
        "Error in processing external entity reference",
        "Document is not standalone",
        "Unexpected parser state - please send a bug report",
        // end of expat (parser) errors

        // start of validation errors
        "Element has not been declared",
        "Root element name does not match DOCTYPE name",
        "Invalid or duplicate root element",
        "Invalid sub-element in parent element",
        "Duplicate element declaration",
        "Duplicate attribute declaration",
        "Undeclared attribute in element"
    };

    int code2 = code - ERROR_XML_FIRST;

    if (    code2 >= 0
         && code2 < sizeof(message) / sizeof(message[0])
       )
        return message[code2];

    return 0;
}

/*
 *@@ xmlSetError:
 *      sets the DOM's error state and stores error information
 *      and parser position.
 *
 *@@added V0.9.9 (2001-02-16) [umoeller]
 */

VOID xmlSetError(PXMLDOM pDom,
                 APIRET arc,
                 const char *pcszFailing,
                 BOOL fValidityError)   // in: if TRUE, this is a validation error;
                                        // if FALSE, this is a parser error
{
    pDom->arcDOM = arc;
    pDom->pcszErrorDescription = xmlDescribeError(pDom->arcDOM);
    pDom->ulErrorLine = XML_GetCurrentLineNumber(pDom->pParser);
    pDom->ulErrorColumn = XML_GetCurrentColumnNumber(pDom->pParser);

    if (pcszFailing)
    {
        if (!pDom->pxstrFailingNode)
            pDom->pxstrFailingNode = xstrCreate(0);

        xstrcpy(pDom->pxstrFailingNode, pcszFailing, 0);
    }

    if (fValidityError)
        pDom->fInvalid = TRUE;
}

/* ******************************************************************
 *
 *   Specific DOM node methods
 *
 ********************************************************************/

/*
 *@@ xmlCreateElementNode:
 *      creates a new element node with the specified name.
 *
 *@@added V0.9.9 (2001-02-14) [umoeller]
 */

APIRET xmlCreateElementNode(PDOMNODE pParent,         // in: parent node (either document or element)
                            const char *pcszElement,  // in: element name (null-terminated)
                            PDOMNODE *ppNew)
{
    PDOMNODE pNew = NULL;
    APIRET arc = xmlCreateNode(pParent,
                               DOMNODE_ELEMENT,
                               &pNew);

    if (arc == NO_ERROR)
    {
        xstrcpy(&pNew->NodeBase.strNodeName, pcszElement, 0);

        *ppNew = pNew;
    }

    return (arc);
}

/*
 *@@ xmlCreateAttributeNode:
 *      creates a new attribute node with the specified data.
 *
 *      NOTE: Attributes have no "parent" node, technically.
 *      They are added to a special, separate list in @DOM_ELEMENT
 *      nodes.
 *
 *      This returns the following errors:
 *
 *      --  Error codes from xmlCreateNode.
 *
 *      --  ERROR_DOM_NO_ELEMENT: pElement is invalid or does
 *          not point to an @DOM_ELEMENT node.
 *
 *@@added V0.9.9 (2001-02-14) [umoeller]
 */

APIRET xmlCreateAttributeNode(PDOMNODE pElement,        // in: element node
                              const char *pcszName,     // in: attribute name (null-terminated)
                              const char *pcszValue,    // in: attribute value (null-terminated)
                              PDOMNODE *ppNew)
{
    APIRET arc = NO_ERROR;

    if (    !pElement
         || pElement->NodeBase.ulNodeType != DOMNODE_ELEMENT
       )
        arc = ERROR_DOM_NO_ELEMENT;
    else
    {
        PDOMNODE pNew = NULL;
        arc = xmlCreateNode(pElement,          // this takes care of adding to the list
                            DOMNODE_ATTRIBUTE,
                            &pNew);
        if (arc == NO_ERROR)
        {
            xstrcpy(&pNew->NodeBase.strNodeName, pcszName, 0);
            pNew->pstrNodeValue = xstrCreate(0);
            xstrcpy(pNew->pstrNodeValue, pcszValue, 0);

            *ppNew = pNew;
        }
    }

    return (arc);
}

/*
 *@@ xmlCreateTextNode:
 *      creates a new text node with the specified content.
 *
 *      Note: This differs from the createText method
 *      as specified by DOM, which has no ulLength parameter.
 *      We need this for speed with @expat though.
 *
 *@@added V0.9.9 (2001-02-14) [umoeller]
 */

APIRET xmlCreateTextNode(PDOMNODE pParent,         // in: parent element node
                         const char *pcszText,     // in: ptr to start of text
                         ULONG ulLength,           // in: length of *pcszText
                         PDOMNODE *ppNew)
{
    PDOMNODE pNew = NULL;
    APIRET arc = xmlCreateNode(pParent,
                               DOMNODE_TEXT,
                               &pNew);
    if (arc == NO_ERROR)
    {
        PSZ pszNodeValue = (PSZ)malloc(ulLength + 1);
        if (!pszNodeValue)
        {
            arc = ERROR_NOT_ENOUGH_MEMORY;
            xmlDeleteNode(pNew);
        }
        else
        {
            memcpy(pszNodeValue, pcszText, ulLength);
            pszNodeValue[ulLength] = '\0';
            pNew->pstrNodeValue = xstrCreate(0);
            xstrset(pNew->pstrNodeValue, pszNodeValue);

            *ppNew = pNew;
        }
    }

    return (arc);
}

/*
 *@@ xmlCreateCommentNode:
 *      creates a new comment node with the specified
 *      content.
 *
 *@@added V0.9.9 (2001-02-14) [umoeller]
 */

APIRET xmlCreateCommentNode(PDOMNODE pParent,         // in: parent element node
                            const char *pcszText,     // in: comment (null-terminated)
                            PDOMNODE *ppNew)
{
    PDOMNODE pNew = NULL;
    APIRET arc = xmlCreateNode(pParent,
                               DOMNODE_COMMENT,
                               &pNew);
    if (arc == NO_ERROR)
    {
        pNew->pstrNodeValue = xstrCreate(0);
        xstrcpy(pNew->pstrNodeValue, pcszText, 0);
        *ppNew = pNew;
    }

    return (arc);
}

/*
 *@@ xmlCreatePINode:
 *      creates a new processing instruction node with the
 *      specified data.
 *
 *@@added V0.9.9 (2001-02-14) [umoeller]
 */

APIRET xmlCreatePINode(PDOMNODE pParent,         // in: parent element node
                       const char *pcszTarget,   // in: PI target (null-terminated)
                       const char *pcszData,     // in: PI data (null-terminated)
                       PDOMNODE *ppNew)
{
    PDOMNODE pNew = NULL;
    APIRET arc = xmlCreateNode(pParent,
                               DOMNODE_PROCESSING_INSTRUCTION,
                               &pNew);
    if (arc == NO_ERROR)
    {
        xstrcpy(&pNew->NodeBase.strNodeName, pcszTarget, 0);
        pNew->pstrNodeValue = xstrCreate(0);
        xstrcpy(pNew->pstrNodeValue, pcszData, 0);

        *ppNew = pNew;
    }

    return (arc);
}

/*
 *@@ xmlCreateDocumentTypeNode:
 *      creates a new document type node with the
 *      specified data.
 *
 *@@added V0.9.9 (2001-02-14) [umoeller]
 */

APIRET xmlCreateDocumentTypeNode(PDOMDOCUMENTNODE pDocumentNode,            // in: document node
                                 const char *pcszDoctypeName,
                                 const char *pcszSysid,
                                 const char *pcszPubid,
                                 int fHasInternalSubset,
                                 PDOMDOCTYPENODE *ppNew)
{
    APIRET arc = NO_ERROR;

    if (pDocumentNode->pDocType)
        // we already have a doctype:
        arc = ERROR_DOM_DUPLICATE_DOCTYPE;
    else
    {
        // create doctype node
        PDOMDOCTYPENODE pNew = NULL;
        arc = xmlCreateNode((PDOMNODE)pDocumentNode,
                            DOMNODE_DOCUMENT_TYPE,
                            (PDOMNODE*)&pNew);

        if (!arc)
        {
            // the node has already been added to the children
            // list of the document node... in addition, set
            // the doctype field in the document
            pDocumentNode->pDocType = pNew;

            // initialize the extra fields
            xstrcpy(&pNew->strPublicID, pcszPubid, 0);
            xstrcpy(&pNew->strSystemID, pcszSysid, 0);
            pNew->fHasInternalSubset = fHasInternalSubset;

            if (pcszDoctypeName)
            {
                ULONG ul = strlen(pcszDoctypeName);
                if (ul)
                {
                    xstrcpy(&pDocumentNode->DomNode.NodeBase.strNodeName,
                            pcszDoctypeName,
                            ul);
                }
            }

            treeInit(&pNew->ElementDeclsTree);
            treeInit(&pNew->AttribDeclBasesTree);

            *ppNew = pNew;
        }
    }
    return (arc);
}

/*
 *@@ xmlGetElementsByTagName:
 *      returns a linked list of @DOM_ELEMENT nodes which
 *      match the specified element name. The special name
 *      "*" matches all elements.
 *
 *      The caller must free the list by calling lstFree.
 *      Returns NULL if no such elements could be found.
 *
 *@@added V0.9.9 (2001-02-14) [umoeller]
 */

PLINKLIST xmlGetElementsByTagName(const char *pcszName)
{
    APIRET arc = NO_ERROR;

    return (0);
}

/* ******************************************************************
 *
 *   Content model methods
 *
 ********************************************************************/

/*
 *@@ SetupParticleAndSubs:
 *
 *      This creates sub-particles and recurses to set them up,
 *      if necessary.
 *
 *@@added V0.9.9 (2001-02-16) [umoeller]
 */

APIRET SetupParticleAndSubs(PCMELEMENTPARTICLE pParticle,
                            PXMLCONTENT pModel,
                            TREE **ppElementNamesTree) // in: ptr to _CMELEMENTDECLNODE.ElementNamesTree
                                                       // (passed to all recursions)
{
    APIRET arc = NO_ERROR;

    // set up member NODEBASE
    switch (pModel->type)
    {
        case XML_CTYPE_EMPTY: // that's easy
            pParticle->CMNode.ulNodeType = ELEMENTPARTICLE_EMPTY;
        break;

        case XML_CTYPE_ANY:   // that's easy
            pParticle->CMNode.ulNodeType = ELEMENTPARTICLE_ANY;
        break;

        case XML_CTYPE_NAME:   // that's easy
            pParticle->CMNode.ulNodeType = ELEMENTPARTICLE_NAME;
            xstrInitCopy(&pParticle->CMNode.strNodeName, pModel->name, 0);
            treeInsertNode(ppElementNamesTree,
                           &pParticle->CMNode.Tree,
                           CompareCMNodeNodes,
                           TRUE);       // allow duplicates here
        break;

        case XML_CTYPE_MIXED:
            pParticle->CMNode.ulNodeType = ELEMENTPARTICLE_MIXED;
        break;

        case XML_CTYPE_CHOICE:
            pParticle->CMNode.ulNodeType = ELEMENTPARTICLE_CHOICE;
        break;

        case XML_CTYPE_SEQ:
            pParticle->CMNode.ulNodeType = ELEMENTPARTICLE_SEQ;
        break;
    }

    pParticle->ulRepeater = pModel->quant;

    if (pModel->numchildren)
    {
        // these are the three cases where we have subnodes
        // in the XMLCONTENT... go for these and recurse
        ULONG ul;
        pParticle->pllSubNodes = lstCreate(FALSE);
        for (ul = 0;
             ul < pModel->numchildren;
             ul++)
        {
            PXMLCONTENT pSubModel = &pModel->children[ul];
            PCMELEMENTPARTICLE pSubNew
                = (PCMELEMENTPARTICLE)malloc(sizeof(*pSubNew));
            if (!pSubNew)
                arc = ERROR_NOT_ENOUGH_MEMORY;
            else
            {
                memset(pSubNew, 0, sizeof(*pSubNew));

                arc = SetupParticleAndSubs(pSubNew,
                                           pSubModel,
                                           ppElementNamesTree);

                if (!arc)
                    // no error: append sub-particle to this particle's
                    // children list
                    lstAppendItem(pParticle->pllSubNodes,
                                  pSubNew);
            }

            if (arc)
                break;
        }
    }

    return (arc);
}

/*
 *@@ xmlCreateElementDecl:
 *      creates a new _CMELEMENTDECLNODE for the specified
 *      _XMLCONTENT content model (which is the @expat structure).
 *      This recurses, if necessary.
 *
 *@@added V0.9.9 (2001-02-16) [umoeller]
 */

APIRET xmlCreateElementDecl(const char *pcszName,
                            PXMLCONTENT pModel,
                            PCMELEMENTDECLNODE *ppNew)
{
    APIRET arc = NO_ERROR;
    PCMELEMENTDECLNODE pNew = (PCMELEMENTDECLNODE)malloc(sizeof(*pNew));
    if (!pNew)
        arc = ERROR_NOT_ENOUGH_MEMORY;
    else
    {
        memset(pNew, 0, sizeof(CMELEMENTDECLNODE));

        // pNew->Particle.CMNode.ulNodeType = ELEMENT_DECLARATION;

        xstrcpy(&pNew->Particle.CMNode.strNodeName, pcszName, 0);

        treeInit(&pNew->ParticleNamesTree);

        // set up the "particle" member and recurse into sub-particles
        arc = SetupParticleAndSubs(&pNew->Particle,
                                   pModel,
                                   &pNew->ParticleNamesTree);

        if (!arc)
            *ppNew = pNew;
        else
            free(pNew);
    }

    return (arc);
}

/*
 *@@ xmlFindElementDecl:
 *      returns the _CMELEMENTDECLNODE for the element
 *      with the specified name or NULL if there's none.
 *
 *@@added V0.9.9 (2001-02-16) [umoeller]
 */

PCMELEMENTDECLNODE xmlFindElementDecl(PXMLDOM pDom,
                                      const XSTRING *pstrElementName)
{
    PCMELEMENTDECLNODE pElementDecl = NULL;

    PDOMDOCTYPENODE pDocTypeNode = pDom->pDocTypeNode;
    if (    (pDocTypeNode)
         && (pstrElementName)
         && (pstrElementName->ulLength)
       )
    {
        pElementDecl = treeFindEQData(&pDocTypeNode->ElementDeclsTree,
                                      (void*)pstrElementName->psz,
                                      CompareCMNodeData);
    }

    return (pElementDecl);
}

/*
 *@@ xmlFindAttribDeclBase:
 *      returns the _CMATTRIBUTEDEDECLBASE for the specified
 *      element name, or NULL if none exists.
 *
 *      To find a specific attribute declaration from both
 *      an element and an attribute name, use xmlFindAttribDecl
 *      instead.
 *
 *@@added V0.9.9 (2001-02-16) [umoeller]
 */

PCMATTRIBUTEDEDECLBASE xmlFindAttribDeclBase(PXMLDOM pDom,
                                             const XSTRING *pstrElementName)
{
    PCMATTRIBUTEDEDECLBASE pAttribDeclBase = NULL;

    PDOMDOCTYPENODE pDocTypeNode = pDom->pDocTypeNode;
    if (    (pDocTypeNode)
         && (pstrElementName)
         && (pstrElementName->ulLength)
       )
    {
        pAttribDeclBase = treeFindEQData(&pDocTypeNode->AttribDeclBasesTree,
                                         (void*)pstrElementName->psz,
                                         CompareCMNodeData);
    }

    return (pAttribDeclBase);
}

/*
 *@@ xmlFindAttribDecl:
 *      returns the _CMATTRIBUTEDEDECL for the specified
 *      element and attribute name, or NULL if none exists.
 *
 *@@added V0.9.9 (2001-02-16) [umoeller]
 */

PCMATTRIBUTEDECL xmlFindAttribDecl(PXMLDOM pDom,
                                   const XSTRING *pstrElementName,
                                   const XSTRING *pstrAttribName)
{
    PCMATTRIBUTEDECL pAttribDecl = NULL;
    if (pstrElementName && pstrAttribName)
    {
        PCMATTRIBUTEDEDECLBASE pAttribDeclBase = xmlFindAttribDeclBase(pDom,
                                                                       pstrElementName);
        if (pAttribDeclBase)
        {
            pAttribDecl = treeFindEQData(&pAttribDeclBase->AttribDeclsTree,
                                         (void*)pstrAttribName->psz,
                                         CompareCMNodeData);
        }
    }

    return (pAttribDecl);
}

/*
 *@@ ValidateElement:
 *      validates the specified element against the document's
 *      @DTD.
 *
 *      This sets arcDOM in XMLDOM on errors.
 *
 *      According to the XML spec, an element is valid if there
 *      is a declaration matching the element declaration where the
 *      element's name matches the element type, and _one_ of the
 *      following holds: ###
 *
 *      (1) The declaration matches EMPTY and the element has no @content.
 *
 *      (2) The declaration matches (children) (see @element_declaration)
 *          and the sequence of child elements belongs to the language
 *          generated by the regular expression in the content model, with
 *          optional @white_space between the start-tag and the first child
 *          element, between child elements, or between the last
 *          child element and the end-tag. Note that a CDATA section
 *          is never considered "whitespace", even if it contains
 *          white space only.
 *
 *      (3) The declaration matches (mixed) (see @element_declaration)
 *          and the content consists of @content and child elements
 *          whose types match names in the content model.
 *
 *      (4) The declaration matches ANY, and the types of any child
 *          elements have been declared. (done)
 *
 *@@added V0.9.9 (2001-02-16) [umoeller]
 */

VOID ValidateElement(PXMLDOM pDom,
                     PDOMNODE pElement)
{
    // yes: get the element decl from the tree
    PCMELEMENTDECLNODE pElementDecl = xmlFindElementDecl(pDom,
                                                         &pElement->NodeBase.strNodeName);
    if (!pElementDecl)
    {
        xmlSetError(pDom,
                    ERROR_DOM_UNDECLARED_ELEMENT,
                    pElement->NodeBase.strNodeName.psz,
                    TRUE);
    }
    else
    {
        // element has been declared:
        // check if it may appear in this element's parent...
        PDOMNODE pParentElement = pElement->pParentNode;

        if (!pParentElement)
            pDom->arcDOM = ERROR_DOM_INTEGRITY;
        else switch (pParentElement->NodeBase.ulNodeType)
        {
            case DOMNODE_DOCUMENT:
            {
                // if this is the root element, compare its name
                // to the DOCTYPE name
                if (pParentElement != (PDOMNODE)pDom->pDocumentNode)
                    xmlSetError(pDom,
                                ERROR_DOM_INVALID_ROOT_ELEMENT,
                                pElement->NodeBase.strNodeName.psz,
                                TRUE);
                else if (strcmp(pDom->pDocumentNode->DomNode.NodeBase.strNodeName.psz,
                                pElement->NodeBase.strNodeName.psz))
                    // no match:
                    xmlSetError(pDom,
                                ERROR_DOM_ROOT_ELEMENT_MISNAMED,
                                pElement->NodeBase.strNodeName.psz,
                                TRUE);
            break; }

            case DOMNODE_ELEMENT:
            {
                // parent of element is another element:
                // check the parent in the DTD and find out if
                // this element may appear in the parent element
                PCMELEMENTDECLNODE pParentElementDecl
                        = xmlFindElementDecl(pDom,
                                             &pParentElement->NodeBase.strNodeName);
                if (!pParentElementDecl)
                    pDom->arcDOM = ERROR_DOM_INTEGRITY;
                else
                {
                    // now check the element names tree of the parent element decl
                    // for whether this element is allowed as a sub-element at all
                    PCMELEMENTPARTICLE pParticle
                        = treeFindEQData(&pParentElementDecl->ParticleNamesTree,
                                         (void*)pElement->NodeBase.strNodeName.psz,
                                         CompareCMNodeData);
                    if (!pParticle)
                        // not found: then this element is not allowed within this
                        // parent
                        xmlSetError(pDom,
                                    ERROR_DOM_INVALID_SUBELEMENT,
                                    pElement->NodeBase.strNodeName.psz,
                                    TRUE);
                }
            break; }
        }
    }
}

/*
 *@@ ValidateAttribute:
 *      validates the specified element against the document's
 *      @DTD.
 *
 *      This sets arcDOM in XMLDOM on errors.
 *
 *@@added V0.9.9 (2001-02-16) [umoeller]
 */

VOID ValidateAttribute(PXMLDOM pDom,
                       PDOMNODE pAttrib)
{
    PDOMNODE pElement = pAttrib->pParentNode;

    PCMATTRIBUTEDECL pAttribDecl = xmlFindAttribDecl(pDom,
                                                     &pElement->NodeBase.strNodeName,
                                                     &pAttrib->NodeBase.strNodeName);
    if (!pAttribDecl)
        xmlSetError(pDom,
                    ERROR_DOM_UNDECLARED_ATTRIBUTE,
                    pAttrib->NodeBase.strNodeName.psz,
                    TRUE);
}

/* ******************************************************************
 *
 *   Expat handlers
 *
 ********************************************************************/

/*
 *@@ StartElementHandler:
 *      @expat handler called when a new element is
 *      found.
 *
 *      We create a new record in the container and
 *      push it onto our stack so we can insert
 *      children into it. We first start with the
 *      attributes.
 */

void EXPATENTRY StartElementHandler(void *pUserData,      // in: our PXMLDOM really
                                    const char *pcszElement,
                                    const char **papcszAttribs)
{
    PXMLDOM     pDom = (PXMLDOM)pUserData;

    // continue parsing only if we had no errors so far
    if (!pDom->arcDOM)
    {
        ULONG       i;

        PDOMNODE    pParent = NULL,
                    pNew = NULL;

        PLISTNODE   pParentNode = lstPop(&pDom->llStack);

        if (!pParentNode)
            pDom->arcDOM = ERROR_DOM_NO_DOCUMENT;
        else
        {
            // we have at least one node:
            pParent = (PDOMNODE)pParentNode->pItemData;

            pDom->arcDOM = xmlCreateElementNode(pParent,
                                                pcszElement,
                                                &pNew);

            if (!pDom->arcDOM)
            {
                // shall we validate?
                if (pDom->pDocTypeNode)
                {
                    // yes:
                    ValidateElement(pDom,
                                    pNew);
                }

                if (!pDom->arcDOM)
                {
                    // OK, node is valid:
                    // push this on the stack so we can add child elements
                    lstPush(&pDom->llStack, pNew);

                    // now for the attribs
                    for (i = 0;
                         (papcszAttribs[i]) && (!pDom->arcDOM);
                         i += 2)
                    {
                        PDOMNODE pAttrib;

                        pDom->arcDOM = xmlCreateAttributeNode(pNew,                    // element,
                                                              papcszAttribs[i],        // attr name
                                                              papcszAttribs[i + 1],    // attr value
                                                              &pAttrib);

                        // shall we validate?
                        if (pDom->pDocTypeNode)
                        {
                            ValidateAttribute(pDom,
                                              pAttrib);
                        }
                    }
                }
            }
        }

        pDom->pLastWasTextNode = NULL;
    }
}

/*
 *@@ EndElementHandler:
 *      @expat handler for when parsing an element is done.
 *      We pop the element off of our stack then.
 */

void EXPATENTRY EndElementHandler(void *pUserData,      // in: our PXMLDOM really
                                  const XML_Char *name)
{
    PXMLDOM     pDom = (PXMLDOM)pUserData;
    // continue parsing only if we had no errors so far
    if (!pDom->arcDOM)
    {
        PLISTNODE   pNode = lstPop(&pDom->llStack);
        if (pNode)
            lstRemoveNode(&pDom->llStack, pNode);

        pDom->pLastWasTextNode = NULL;
    }
}

/*
 *@@ CharacterDataHandler:
 *      @expat handler for character data (@content).
 *
 *      Note: expat passes chunks of content without zero-terminating
 *      them. We must concatenate the chunks to a full text node.
 */

void EXPATENTRY CharacterDataHandler(void *pUserData,      // in: our PXMLDOM really
                                     const XML_Char *s,
                                     int len)
{
    PXMLDOM     pDom = (PXMLDOM)pUserData;

    // continue parsing only if we had no errors so far
    if (!pDom->arcDOM)
    {
        ULONG       i;

        if (len)
        {
            if (pDom->pLastWasTextNode)
            {
                // we had a text node, and no elements or other
                // stuff in between:
                xstrcat(pDom->pLastWasTextNode->pstrNodeValue,
                        s,
                        len);
            }
            else
            {
                // we need a new text node:
                PDOMNODE pNew,
                         pParent;
                // non-root level:
                PLISTNODE pParentNode = lstPop(&pDom->llStack);
                pParent = (PDOMNODE)pParentNode->pItemData;

                pDom->arcDOM = xmlCreateTextNode(pParent,
                                                 s,
                                                 len,
                                                 &pDom->pLastWasTextNode);
            }
        }
    }
}

/*
 *@@ CommentHandler:
 *      @expat handler for @comments.
 *
 *      Note: This is only set if DF_PARSECOMMENTS is
 *      flagged with xmlCreateDOM.
 *
 *@@added V0.9.9 (2001-02-14) [umoeller]
 */

void EXPATENTRY CommentHandler(void *pUserData,      // in: our PXMLDOM really
                               const XML_Char *data)
{
    PXMLDOM     pDom = (PXMLDOM)pUserData;

    // continue parsing only if we had no errors so far
    if (!pDom->arcDOM)
    {
        PLISTNODE   pParentNode = lstPop(&pDom->llStack);

        if (pParentNode)
        {
            // non-root level:
            PDOMNODE pParent = (PDOMNODE)pParentNode->pItemData;
            PDOMNODE pComment;

            pDom->arcDOM = xmlCreateCommentNode(pParent,
                                                data,
                                                &pComment);
        }
    }
}

/*
 *@@ StartDoctypeDeclHandler:
 *      @expat handler that is called at the start of a DOCTYPE
 *      declaration, before any external or internal subset is
 *      parsed.
 *
 *      Both pcszSysid and pcszPubid may be NULL. "fHasInternalSubset"
 *      will be non-zero if the DOCTYPE declaration has an internal subset.
 *
 *@@added V0.9.9 (2001-02-14) [umoeller]
 */

void EXPATENTRY StartDoctypeDeclHandler(void *pUserData,
                                        const XML_Char *pcszDoctypeName,
                                        const XML_Char *pcszSysid,
                                        const XML_Char *pcszPubid,
                                        int fHasInternalSubset)
{
    PXMLDOM     pDom = (PXMLDOM)pUserData;

    // continue parsing only if we had no errors so far
    if (!pDom->arcDOM)
    {
        // get the document node
        PDOMDOCUMENTNODE pDocumentNode = pDom->pDocumentNode;
        if (!pDocumentNode)
            pDom->arcDOM = ERROR_DOM_NO_DOCUMENT;
        else
        {
            pDom->arcDOM = xmlCreateDocumentTypeNode(pDocumentNode,
                                                     pcszDoctypeName,
                                                     pcszSysid,
                                                     pcszPubid,
                                                     fHasInternalSubset,
                                                     &pDom->pDocTypeNode);

            // push this on the stack so we can add child elements
            lstPush(&pDom->llStack, pDom->pDocTypeNode);
        }
    }
}

/*
 *@@ EndDoctypeDeclHandler:
 *      @expat handler that is called at the end of a DOCTYPE
 *      declaration, after parsing any external subset.
 *
 *@@added V0.9.9 (2001-02-14) [umoeller]
 */

void EXPATENTRY EndDoctypeDeclHandler(void *pUserData)      // in: our PXMLDOM really
{
    PXMLDOM     pDom = (PXMLDOM)pUserData;

    PLISTNODE pListNode = lstPop(&pDom->llStack);
    if (!pListNode)
        pDom->arcDOM = ERROR_DOM_DOCTYPE_STRUCTURE;
    else
    {
        PDOMNODE pDomNode = (PDOMNODE)pListNode->pItemData;
        if (pDomNode->NodeBase.ulNodeType != DOMNODE_DOCUMENT_TYPE)
            pDom->arcDOM = ERROR_DOM_DOCTYPE_STRUCTURE;

        lstRemoveNode(&pDom->llStack, pListNode);
    }

    // continue parsing only if we had no errors so far
    if (!pDom->arcDOM)
    {

    }
}

/*
 *@@ NotationDeclHandler:
 *      @expat handler for @notation_declarations.
 *
 *@@added V0.9.9 (2001-02-14) [umoeller]
 */

void EXPATENTRY NotationDeclHandler(void *pUserData,      // in: our PXMLDOM really
                                    const XML_Char *pcszNotationName,
                                    const XML_Char *pcszBase,
                                    const XML_Char *pcszSystemId,
                                    const XML_Char *pcszPublicId)
{
    PXMLDOM     pDom = (PXMLDOM)pUserData;

    // continue parsing only if we had no errors so far
    if (!pDom->arcDOM)
    {
    }
}

/*
 *@@ ExternalEntityRefHandler:
 *      @expat handler for references to @external_entities.
 *
 *      This handler is also called for processing an external DTD
 *      subset if parameter entity parsing is in effect.
 *      (See XML_SetParamEntityParsing.)
 *
 *      The pcszContext argument specifies the parsing context in the
 *      format expected by the context argument to
 *      XML_ExternalEntityParserCreate; pcszContext is valid only until
 *      the handler returns, so if the referenced entity is to be
 *      parsed later, it must be copied.
 *
 *      The pcszBase parameter is the base to use for relative system
 *      identifiers. It is set by XML_SetBase and may be null.
 *
 *      The pcszPublicId parameter is the public id given in the entity
 *      declaration and may be null.
 *
 *      The pcszSystemId is the system identifier specified in the
 *      entity declaration and is never null.
 *
 *      There are a couple of ways in which this handler differs
 *      from others. First, this handler returns an integer. A
 *      non-zero value should be returned for successful handling
 *      of the external entity reference. Returning a zero indicates
 *      failure, and causes the calling parser to return an
 *      ERROR_EXPAT_EXTERNAL_ENTITY_HANDLING error.
 *
 *      Second, instead of having pUserData as its first argument,
 *      it receives the parser that encountered the entity reference.
 *      This, along with the context parameter, may be used as
 *      arguments to a call to XML_ExternalEntityParserCreate.
 *      Using the returned parser, the body of the external entity
 *      can be recursively parsed.
 *
 *      Since this handler may be called recursively, it should not
 *      be saving information into global or static variables.
 *
 *      Your handler isn't actually responsible for parsing the entity,
 *      but it is responsible for creating a subsidiary parser with
 *      XML_ExternalEntityParserCreate that will do the job. That returns
 *      an instance of XML_Parser that has handlers and other data
 *      structures initialized from the parent parser. You may then use
 *      XML_Parse or XML_ParseBuffer calls against that parser. Since
 *      external entities may refer to other external entities, your
 *      handler should be prepared to be called recursively.
 *
 *@@added V0.9.9 (2001-02-14) [umoeller]
 */

int EXPATENTRY ExternalEntityRefHandler(XML_Parser parser,
                                        const XML_Char *pcszContext,
                                        const XML_Char *pcszBase,
                                        const XML_Char *pcszSystemId,
                                        const XML_Char *pcszPublicId)
{
    int i = 1;

    /* PXMLDOM     pDom = (PXMLDOM)pUserData;

    // continue parsing only if we had no errors so far
    if (!pDom->arcDOM)
    {
    } */

    return (i);
}

/*
 *@@ ElementDeclHandler:
 *      @expat handler for element declarations in a DTD. The
 *      handler gets called with the name of the element in
 *      the declaration and a pointer to a structure that contains
 *      the element model.
 *
 *      It is the application's responsibility to free this data
 *      structure. ###
 *
 *      The XML spec defines that no element may be declared more
 *      than once.
 *
 *@@added V0.9.9 (2001-02-14) [umoeller]
 */

void EXPATENTRY ElementDeclHandler(void *pUserData,      // in: our PXMLDOM really
                                   const XML_Char *pcszName,
                                   XMLCONTENT *pModel)
{
    PXMLDOM     pDom = (PXMLDOM)pUserData;

    // continue parsing only if we had no errors so far
    if (!pDom->arcDOM)
    {
        // pop the last DOMNODE off the stack and check if it's a DOCTYPE
        PLISTNODE pListNode = lstPop(&pDom->llStack);
        if (!pListNode)
            pDom->arcDOM = ERROR_DOM_DOCTYPE_STRUCTURE;
        else
        {
            PDOMNODE pDomNode = (PDOMNODE)pListNode->pItemData;
            if (pDomNode->NodeBase.ulNodeType != DOMNODE_DOCUMENT_TYPE)
                pDom->arcDOM = ERROR_DOM_DOCTYPE_STRUCTURE;
            else
            {
                // OK, we're in a DOCTYPE node:
                PDOMDOCTYPENODE pDocType = (PDOMDOCTYPENODE)pDomNode;

                // create an element declaration and push it unto the
                // declarations tree
                PCMELEMENTDECLNODE pNew = NULL;
                pDom->arcDOM = xmlCreateElementDecl(pcszName,
                                                    pModel,
                                                    &pNew);
                                        // this recurses!!

                if (pDom->arcDOM == NO_ERROR)
                {
                    // add this to the doctype's declarations tree
                    if (treeInsertNode(&pDocType->ElementDeclsTree,
                                       (TREE*)pNew,
                                       CompareCMNodeNodes,
                                       FALSE)
                            == TREE_DUPLICATE)
                        // element already declared:
                        // according to the XML specs, this is a validity
                        // constraint, so we report a validation error
                        xmlSetError(pDom,
                                    ERROR_DOM_DUPLICATE_ELEMENT_DECL,
                                    pNew->Particle.CMNode.strNodeName.psz,
                                    TRUE);
                }
            }
        }
    }
}

/*
 *@@ AddEnum:
 *
 *@@added V0.9.9 (2001-02-16) [umoeller]
 */

VOID AddEnum(PCMATTRIBUTEDECL pNew,
             const char *p,
             const char *pNext)
{
    PSZ pszType = strhSubstr(p, pNext);
    PNODEBASE pCMNode = (PNODEBASE)malloc(sizeof(*pCMNode));
    memset(pCMNode, 0, sizeof(*pCMNode));
    pCMNode->ulNodeType = ATTRIBUTE_DECLARATION_ENUM;
    xstrInitSet(&pCMNode->strNodeName, pszType);

    treeInsertNode(&pNew->ValuesTree,
                   (TREE*)pCMNode,
                   CompareCMNodeNodes,
                   FALSE);
}

/*
 *@@ AttlistDeclHandler:
 *      @expat handler for attlist declarations in the DTD.
 *
 *      This handler is called for each attribute. So a single attlist
 *      declaration with multiple attributes declared will generate
 *      multiple calls to this handler.
 *
 *      --  pcszElementName is the name of the  element for which the
 *          attribute is being declared.
 *
 *      --  pcszAttribName has the attribute name being declared.
 *
 *      --  pcszAttribType is the attribute type.
 *          It is the string representing the type in the declaration
 *          with whitespace removed.
 *
 *      --  pcszDefault holds the default value. It will be
 *          NULL in the case of "#IMPLIED" or "#REQUIRED" attributes.
 *          You can distinguish these two cases by checking the
 *          fIsRequired parameter, which will be true in the case of
 *          "#REQUIRED" attributes. Attributes which are "#FIXED"
 *          will have also have a TRUE fIsRequired, but they will have
 *          the non-NULL fixed value in the pcszDefault parameter.
 *
 *@@added V0.9.9 (2001-02-14) [umoeller]
 */

void EXPATENTRY AttlistDeclHandler(void *pUserData,      // in: our PXMLDOM really
                                   const XML_Char *pcszElementName,
                                   const XML_Char *pcszAttribName,
                                   const XML_Char *pcszAttribType,
                                   const XML_Char *pcszDefault,
                                   int fIsRequired)
{
    PXMLDOM     pDom = (PXMLDOM)pUserData;

    // continue parsing only if we had no errors so far
    if (!pDom->arcDOM)
    {
        // pop the last DOMNODE off the stack and check if it's a DOCTYPE
        PLISTNODE pListNode = lstPop(&pDom->llStack);
        if (!pListNode)
            pDom->arcDOM = ERROR_DOM_DOCTYPE_STRUCTURE;
        else
        {
            PDOMNODE pDomNode = (PDOMNODE)pListNode->pItemData;
            if (pDomNode->NodeBase.ulNodeType != DOMNODE_DOCUMENT_TYPE)
                pDom->arcDOM = ERROR_DOM_DOCTYPE_STRUCTURE;
            else
            {
                // OK, we're in a DOCTYPE node:
                PDOMDOCTYPENODE pDocType = (PDOMDOCTYPENODE)pDomNode;
                PCMATTRIBUTEDEDECLBASE    pThis = NULL,
                                        pCache = pDom->pAttListDeclCache;

                // check if this is for the same attlist as the previous
                // call (we cache the pointer for speed)
                if (    (pCache)
                     && (!strhcmp(pCache->CMNode.strNodeName.psz,
                                  pcszElementName))
                   )
                    // this attdecl is for the same element:
                    // use that (we won't have to search the tree)
                    pThis = pDom->pAttListDeclCache;

                if (!pThis)
                {
                    // cache didn't match: look up attributes tree then
                    pThis = treeFindEQData(&pDocType->AttribDeclBasesTree,
                                           (void*)pcszElementName,
                                           CompareCMNodeData);

                    if (!pThis)
                    {
                        // still not found:
                        // we need a new node then
                        pThis = (PCMATTRIBUTEDEDECLBASE)malloc(sizeof(*pThis));
                        if (!pThis)
                            pDom->arcDOM = ERROR_NOT_ENOUGH_MEMORY;
                        else
                        {
                            pThis->CMNode.ulNodeType = ATTRIBUTE_DECLARATION_BASE;
                            xstrInitCopy(&pThis->CMNode.strNodeName, pcszElementName, 0);

                            // initialize the subtree
                            treeInit(&pThis->AttribDeclsTree);

                            treeInsertNode(&pDocType->AttribDeclBasesTree,
                                           (TREE*)pThis,
                                           CompareCMNodeNodes,
                                           FALSE);
                        }
                    }

                    pDom->pAttListDeclCache = pThis;
                }

                if (pThis)
                {
                    // pThis now has either an existing or a new CMATTRIBUTEDEDECLBASE;
                    // add a new attribute def (CMATTRIBUTEDEDECL) to that
                    PCMATTRIBUTEDECL  pNew = (PCMATTRIBUTEDECL)malloc(sizeof(*pNew));
                    if (!pNew)
                        pDom->arcDOM = ERROR_NOT_ENOUGH_MEMORY;
                    else
                    {
                        memset(pNew, 0, sizeof(*pNew));
                        pNew->CMNode.ulNodeType = ATTRIBUTE_DECLARATION;

                        xstrInitCopy(&pNew->CMNode.strNodeName,
                                     pcszAttribName,
                                     0);

                        // fill the other fields
                        /* xstrInitCopy(&pNew->strType,
                                     pcszAttribType,
                                     0); */

                        treeInit(&pNew->ValuesTree);

                        // check the type... expat is too lazy to parse this for
                        // us, so we must check manually. Expat only normalizes
                        // the "type" string to kick out whitespace, so we get:
                        // (TYPE1|TYPE2|TYPE3)
                        if (*pcszAttribType == '(')
                        {
                            // enumeration:
                            const char *p = pcszAttribType + 1,
                                       *pNext;
                            while (pNext = strchr(p, '|'))
                            {
                                AddEnum(pNew, p, pNext);
                                p = pNext + 1;
                            }

                            pNext = strchr(p, ')');
                            AddEnum(pNew, p, pNext);

                            pNew->ulAttrType = CMAT_ENUM;
                        }
                        else if (!strcmp(pcszAttribType, "CDATA"))
                            pNew->ulAttrType = CMAT_CDATA;
                        else if (!strcmp(pcszAttribType, "ID"))
                            pNew->ulAttrType = CMAT_ID;
                        else if (!strcmp(pcszAttribType, "IDREF"))
                            pNew->ulAttrType = CMAT_IDREF;
                        else if (!strcmp(pcszAttribType, "IDREFS"))
                            pNew->ulAttrType = CMAT_IDREFS;
                        else if (!strcmp(pcszAttribType, "ENTITY"))
                            pNew->ulAttrType = CMAT_ENTITY;
                        else if (!strcmp(pcszAttribType, "ENTITIES"))
                            pNew->ulAttrType = CMAT_ENTITIES;
                        else if (!strcmp(pcszAttribType, "NMTOKEN"))
                            pNew->ulAttrType = CMAT_NMTOKEN;
                        else if (!strcmp(pcszAttribType, "NMTOKENS"))
                            pNew->ulAttrType = CMAT_NMTOKENS;

                        if (pcszDefault)
                        {
                            // fixed or default:
                            if (fIsRequired)
                                // fixed:
                                pNew->ulConstraint = CMAT_FIXED_VALUE;
                            else
                                pNew->ulConstraint = CMAT_DEFAULT_VALUE;

                            pNew->pstrDefaultValue = xstrCreate(0);
                            xstrcpy(pNew->pstrDefaultValue, pcszDefault, 0);
                        }
                        else
                            // implied or required:
                            if (fIsRequired)
                                pNew->ulConstraint = CMAT_REQUIRED;
                            else
                                pNew->ulConstraint = CMAT_IMPLIED;

                        if (treeInsertNode(&pThis->AttribDeclsTree,
                                           (TREE*)pNew,
                                           CompareCMNodeNodes,
                                           FALSE)
                                == TREE_DUPLICATE)
                            xmlSetError(pDom,
                                        ERROR_DOM_DUPLICATE_ATTRIBUTE_DECL,
                                        pcszAttribName,
                                        TRUE);
                    }
                }
            }
        }
    }
}

/*
 *@@ EntityDeclHandler:
 *      @expat handler that will be called for all entity declarations.
 *
 *      The fIsParameterEntity argument will be non-zero in the case
 *      of parameter entities and zero otherwise.
 *
 *      For internal entities (<!ENTITY foo "bar">), pcszValue will be
 *      non-NULL and pcszSystemId, pcszPublicId, and pcszNotationName
 *      will all be NULL. The value string is not NULL terminated; the
 *      length is provided in the iValueLength parameter. Do not use
 *      iValueLength to test for internal entities, since it is legal
 *      to have zero-length values. Instead check for whether or not
 *      pcszValue is NULL.
 *
 *      The pcszNotationName argument will have a non-NULL value only
 *      for unparsed entity declarations.
 *
 *@@added V0.9.9 (2001-02-14) [umoeller]
 */

void EXPATENTRY EntityDeclHandler(void *pUserData,      // in: our PXMLDOM really
                                  const XML_Char *pcszEntityName,
                                  int fIsParameterEntity,
                                  const XML_Char *pcszValue,
                                  int iValueLength,
                                  const XML_Char *pcszBase,
                                  const XML_Char *pcszSystemId,
                                  const XML_Char *pcszPublicId,
                                  const XML_Char *pcszNotationName)
{
    PXMLDOM     pDom = (PXMLDOM)pUserData;

    // continue parsing only if we had no errors so far
    if (!pDom->arcDOM)
    {
    }
}

/* ******************************************************************
 *
 *   DOM APIs
 *
 ********************************************************************/

/*
 *@@ xmlCreateDOM:
 *      creates an XMLDOM instance, which can be used for parsing
 *      an XML document and building a @DOM tree from it at the
 *      same time.
 *
 *      Pass the XMLDOM returned here to xmlParse afterwards.
 *
 *      ulFlags is any combination of the following:
 *
 *      --  DF_PARSECOMMENTS: XML @comments are to be returned in
 *          the DOM tree. Otherwise they are silently ignored.
 *
 *      --  DF_PARSEDTD: add the @DTD of the document into the DOM tree
 *          as well and validate the document.
 *
 *      Usage:
 *
 *      1) Create a DOM instance.
 *
 +          PXMLDOM pDom = NULL;
 +          APIRET arc = xmlCreateDom(flags, &pDom);
 +
 *      2) Give chunks of data (or an entire buffer)
 *         to the DOM instance for parsing.
 *
 +          arc = xmlParse(pDom,
 +                         pBuf,
 +                         TRUE); // if last, this will clean up the parser
 *
 *      3) Process the data in the DOM tree. When done,
 *         call xmlFreeDOM, which will free all memory.
 *
 *@@added V0.9.9 (2001-02-14) [umoeller]
 */

APIRET xmlCreateDOM(ULONG flParserFlags,
                    PXMLDOM *ppDom)
{
    APIRET  arc = NO_ERROR;

    PXMLDOM pDom = (PXMLDOM)malloc(sizeof(*pDom));
    if (!pDom)
        arc = ERROR_NOT_ENOUGH_MEMORY;
    else
    {
        PDOMNODE pDocument = NULL;

        memset(pDom, 0, sizeof(XMLDOM));

        lstInit(&pDom->llStack,
                FALSE);                 // no auto-free

        // create the document node
        arc = xmlCreateNode(NULL, // no parent
                            DOMNODE_DOCUMENT,
                            &pDocument);

        if (arc == NO_ERROR)
        {
            // store the document in the DOM
            pDom->pDocumentNode = (PDOMDOCUMENTNODE)pDocument;

            // push the document on the stack so the handlers
            // will append to that
            lstPush(&pDom->llStack,
                    pDom->pDocumentNode);

            pDom->pParser = XML_ParserCreate(NULL);

            if (!pDom->pParser)
                arc = ERROR_NOT_ENOUGH_MEMORY;
            else
            {
                XML_SetElementHandler(pDom->pParser,
                                      StartElementHandler,
                                      EndElementHandler);

                XML_SetCharacterDataHandler(pDom->pParser,
                                            CharacterDataHandler);

                // XML_SetProcessingInstructionHandler(XML_Parser parser,
                //                          XML_ProcessingInstructionHandler handler);


                if (flParserFlags & DF_PARSECOMMENTS)
                    XML_SetCommentHandler(pDom->pParser,
                                          CommentHandler);

                if (flParserFlags & DF_PARSEDTD)
                {
                    XML_SetDoctypeDeclHandler(pDom->pParser,
                                              StartDoctypeDeclHandler,
                                              EndDoctypeDeclHandler);

                    XML_SetNotationDeclHandler(pDom->pParser,
                                               NotationDeclHandler);

                    XML_SetExternalEntityRefHandler(pDom->pParser,
                                                    ExternalEntityRefHandler);

                    XML_SetElementDeclHandler(pDom->pParser,
                                              ElementDeclHandler);

                    XML_SetAttlistDeclHandler(pDom->pParser,
                                              AttlistDeclHandler);

                    XML_SetEntityDeclHandler(pDom->pParser,
                                             EntityDeclHandler);

                    XML_SetParamEntityParsing(pDom->pParser,
                                              XML_PARAM_ENTITY_PARSING_ALWAYS);
                }

                // XML_SetXmlDeclHandler ... do we care for this? I guess not

                // pass the XMLDOM as user data to the handlers
                XML_SetUserData(pDom->pParser,
                                pDom);
            }
        }
    }

    if (arc == NO_ERROR)
        *ppDom = pDom;
    else
        xmlFreeDOM(pDom);

    return (arc);
}

/*
 *@@ xmlParse:
 *      parses another piece of XML data.
 *
 *      If (fIsLast == TRUE), the internal @expat parser
 *      will be freed, but not the DOM itself.
 *
 *      You can pass an XML document to this function
 *      in one flush. Set fIsLast = TRUE on the first
 *      and only call then.
 *
 *      This returns NO_ERROR if the chunk was successfully
 *      parsed. Otherwise one of the following errors is
 *      returned:
 *
 *      -- ERROR_INVALID_PARAMETER
 *
 *      -- ERROR_DOM_PARSING: an @expat parsing error occured.
 *         This might also be memory problems.
 *         With this error code, you will find specific
 *         error information in the XMLDOM fields.
 *
 *      -- ERROR_DOM_PARSING: the document is not @valid.
 *         This can only happen if @DTD parsing was enabled
 *         with xmlCreateDOM.
 *         With this error code, you will find specific
 *         error information in the XMLDOM fields.
 *
 *@@added V0.9.9 (2001-02-14) [umoeller]
 */

APIRET xmlParse(PXMLDOM pDom,
                const char *pcszBuf,
                ULONG cb,
                BOOL fIsLast)
{
    APIRET arc = NO_ERROR;

    if (!pDom)
        arc = ERROR_INVALID_PARAMETER;
    else
    {
        BOOL fSuccess = XML_Parse(pDom->pParser,
                                  pcszBuf,
                                  cb,
                                  fIsLast);

        if (!fSuccess)
        {
            // expat parsing error:
            xmlSetError(pDom,
                        XML_GetErrorCode(pDom->pParser),
                        NULL,
                        FALSE);

            if (pDom->pDocumentNode)
            {
                xmlDeleteNode((PDOMNODE)pDom->pDocumentNode);
                pDom->pDocumentNode = NULL;
            }

            arc = ERROR_DOM_PARSING;
        }
        else if (pDom->fInvalid)
        {
            // expat was doing OK, but the handlers' validation failed:
            arc = ERROR_DOM_VALIDITY;
                        // error info has already been set
        }
        else
            // expat was doing OK, but maybe we have integrity errors
            // from our DOM callbacks:
            if (pDom->arcDOM)
                arc = pDom->arcDOM;

        if (arc != NO_ERROR || fIsLast)
        {
            // last call or error: clean up
            XML_ParserFree(pDom->pParser);
            pDom->pParser = NULL;

            // clean up the stack (but not the DOM itself)
            lstClear(&pDom->llStack);
        }
    }

    return (arc);
}

/*
 *@@ xmlFreeDOM:
 *      cleans up all resources allocated by
 *      xmlCreateDOM and xmlParse, including
 *      the entire DOM tree.
 *
 *      If you wish to keep any data, make
 *      a copy of the respective pointers in pDom
 *      or subitems and set them to NULL before
 *      calling this function.
 *
 *@@added V0.9.9 (2001-02-14) [umoeller]
 */

APIRET xmlFreeDOM(PXMLDOM pDom)
{
    APIRET arc = NO_ERROR;
    if (pDom)
    {
        // if the parser is still alive for some reason, close it.
        if (pDom->pParser)
        {
            XML_ParserFree(pDom->pParser);
            pDom->pParser = NULL;
        }

        free(pDom);
    }

    return (arc);
}
