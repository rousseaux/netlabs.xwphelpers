
/*
 *@@sourcefile xml.c:
 *      XML document handling.
 *
 *      XML support in the XWorkplace Helpers is broken into two
 *      layers:
 *
 *      --  The bottom layer is implemented by expat, which I have
 *          ported to this library. See xmlparse.c for an introduction.
 *
 *      --  Because expat requires so many callbacks and is non-validating,
 *          I have added a top layer above the expat library
 *          which is vaguely modelled after the Document Object Model
 *          (DOM) standardized by the W3C. That's this file.
 *
 *      To understand and use this code, you should be familiar with
 *      the following:
 *
 *      -- XML parsers operate on XML @documents.
 *
 *      -- Each XML document has both a logical and a physical
 *         structure.
 *
 *         Physically, the document is composed of units called
 *         @entities.
 *
 *         Logically, the document is composed of @markup and
 *         @content. Among other things, markup separates the content
 *         into @elements.
 *
 *      -- The logical and physical structures must nest properly (be
 *         @well-formed) for each entity, which results in the entire
 *         XML document being well-formed as well.
 *
 *      <B>Document Object Model (DOM)</B>
 *
 *      In short, DOM specifies that an XML document is broken
 *      up into a tree of nodes, representing the various parts
 *      of an XML document. The W3C calls this "a platform- and
 *      language-neutral interface that allows programs and scripts
 *      to dynamically access and update the content, structure
 *      and style of documents. The Document Object Model provides
 *      a standard set of objects for representing HTML and XML
 *      documents, a standard model of how these objects can
 *      be combined, and a standard interface for accessing and
 *      manipulating them. Vendors can support the DOM as an
 *      interface to their proprietary data structures and APIs,
 *      and content authors can write to the standard DOM
 *      interfaces rather than product-specific APIs, thus
 *      increasing interoperability on the Web."
 *
 *      Example: Take this HTML table definition:
 +
 +          <TABLE>
 +          <TBODY>
 +          <TR>
 +          <TD>Column 1-1</TD>
 +          <TD>Column 1-2</TD>
 +          </TR>
 +          <TR>
 +          <TD>Column 2-1</TD>
 +          <TD>Column 2-2</TD>
 +          </TR>
 +          </TBODY>
 +          </TABLE>
 *
 *      This function will create a tree as follows:
 +
 +                          ÚÄÄÄÄÄÄÄÄÄÄÄÄ¿
 +                          ³   TABLE    ³        (only ELEMENT node in root DOCUMENT node)
 +                          ÀÄÄÄÄÄÂÄÄÄÄÄÄÙ
 +                                ³
 +                          ÚÄÄÄÄÄÁÄÄÄÄÄÄ¿
 +                          ³   TBODY    ³        (only ELEMENT node in root "TABLE" node)
 +                          ÀÄÄÄÄÄÂÄÄÄÄÄÄÙ
 +                    ÚÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄ¿
 +              ÚÄÄÄÄÄÁÄÄÄÄÄÄ¿          ÚÄÄÄÄÄÁÄÄÄÄÄÄ¿
 +              ³   TR       ³          ³   TR       ³
 +              ÀÄÄÄÄÄÂÄÄÄÄÄÄÙ          ÀÄÄÄÄÄÂÄÄÄÄÄÄÙ
 +                ÚÄÄÄÁÄÄÄÄÄÄ¿            ÚÄÄÄÁÄÄÄÄÄÄ¿
 +            ÚÄÄÄÁÄ¿     ÚÄÄÁÄÄ¿     ÚÄÄÄÁÄ¿     ÚÄÄÁÄÄ¿
 +            ³ TD  ³     ³ TD  ³     ³ TD  ³     ³ TD  ³
 +            ÀÄÄÂÄÄÙ     ÀÄÄÂÄÄÙ     ÀÄÄÄÂÄÙ     ÀÄÄÂÄÄÙ
 +         ÉÍÍÍÍÍÊÍÍÍÍ» ÉÍÍÍÍÊÍÍÍÍÍ» ÉÍÍÍÍÊÍÍÍÍÍ» ÉÍÍÊÍÍÍÍÍÍÍ»
 +         ºColumn 1-1º ºColumn 1-2º ºColumn 2-1º ºColumn 2-2º    (one TEXT node in each parent node)
 +         ÈÍÍÍÍÍÍÍÍÍÍ¼ ÈÍÍÍÍÍÍÍÍÍÍ¼ ÈÍÍÍÍÍÍÍÍÍÍ¼ ÈÍÍÍÍÍÍÍÍÍÍ¼
 *
 *      DOM really calls for object oriented programming so the various
 *      structs can inherit from each other. Since this implementation
 *      was supposed to be a C-only interface, we do not implement
 *      inheritance. Instead, each XML document is broken up into a tree
 *      of DOMNODE's only, each of which has a special type.
 *
 *      It shouldn't be too difficult to write a C++ encapsulation
 *      of this which implements all the methods required by the DOM
 *      standard.
 *
 *      The main entry point into this is xmlParse or
 *      xmlCreateDocumentFromString. See remarks there for details.
 *
 *      Limitations:
 *
 *      1)  This presently only parses ELEMENT, ATTRIBUTE, TEXT,
 *          and COMMENT nodes.
 *
 *      2)  This doesn't use 16-bit characters, but 8-bit characters.
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
 *   Node management
 *
 ********************************************************************/

/*
 *@@ xmlCreateNode:
 *      creates a new DOMNODE with the specified
 *      type and parent. Other than that, the
 *      node is zeroed.
 */

PDOMNODE xmlCreateNode(PDOMNODE pParentNode,        // in: parent node or NULL if root
                       ULONG ulNodeType)            // in: DOMNODE_* type
{
    PDOMNODE pNewNode = (PDOMNODE)malloc(sizeof(DOMNODE));
    if (pNewNode)
    {
        memset(pNewNode, 0, sizeof(DOMNODE));
        pNewNode->ulNodeType = ulNodeType;
        pNewNode->pParentNode = pParentNode;
        if (pParentNode)
        {
            // parent specified:
            // append this new node to the parent's
            // list of child nodes
            lstAppendItem(&pParentNode->llChildNodes,
                          pNewNode);
        }

        lstInit(&pNewNode->llChildNodes, FALSE);
        lstInit(&pNewNode->llAttributeNodes, FALSE);
    }

    return (pNewNode);
}

/*
 *@@ xmlDeleteNode:
 *      deletes the specified node.
 *
 *      If the node has child nodes, all of them are deleted
 *      as well. This recurses, if necessary.
 *
 *      As a result, if the node is a document node, this
 *      deletes an entire document, including all of its
 *      child nodes.
 *
 *      Returns:
 *
 *      -- 0: NO_ERROR.
 */

ULONG xmlDeleteNode(PDOMNODE pNode)
{
    ULONG ulrc = 0;

    if (!pNode)
    {
        ulrc = ERROR_DOM_NOT_FOUND;
    }
    else
    {
        // recurse into child nodes
        PLISTNODE   pNodeThis = lstQueryFirstNode(&pNode->llChildNodes);
        while (pNodeThis)
        {
            // recurse!!
            xmlDeleteNode((PDOMNODE)(pNodeThis->pItemData));

            pNodeThis = pNodeThis->pNext;
        }

        // delete attribute nodes
        pNodeThis = lstQueryFirstNode(&pNode->llAttributeNodes);
        while (pNodeThis)
        {
            // recurse!!
            xmlDeleteNode((PDOMNODE)(pNodeThis->pItemData));

            pNodeThis = pNodeThis->pNext;
        }

        if (pNode->pParentNode)
        {
            // node has a parent:
            // remove this node from the parent's list
            // of child nodes before deleting this node
            lstRemoveItem(&pNode->pParentNode->llChildNodes,
                          pNode);
            pNode->pParentNode = NULL;
        }

        xstrClear(&pNode->strNodeName);
        xstrClear(&pNode->strNodeValue);

        lstClear(&pNode->llChildNodes);
        lstClear(&pNode->llAttributeNodes);

        free(pNode);
    }

    return (ulrc);
}

/* ******************************************************************
 *
 *   Expat handlers
 *
 ********************************************************************/

/*
 *@@ StartElementHandler:
 *      expat handler called when a new element is
 *      found.
 *
 *      We create a new record in the container and
 *      push it onto our stack so we can insert
 *      children into it. We first start with the
 *      attributes.
 */

void EXPATENTRY StartElementHandler(void *data,           // in: our PXMLFILE really
                                    const char *pcszElement,
                                    const char **papcszAttribs)
{
    PXMLDOM     pDom = (PXMLDOM)data;

    ULONG       i;

    PDOMNODE    pParent = NULL,
                pNew = NULL;

    PLISTNODE   pParentNode = lstPop(&pDom->llStack);

    if (pParentNode)
    {
        // non-root level:
        pParent = (PDOMNODE)pParentNode->pItemData;

        pNew = xmlCreateNode(pParent,
                             DOMNODE_ELEMENT);

        if (pNew)
            xstrcpy(&pNew->strNodeName, pcszElement, 0);

        // push this on the stack so we can add child elements
        lstPush(&pDom->llStack, pNew);

        // now for the attribs
        for (i = 0;
             papcszAttribs[i];
             i += 2)
        {
            PDOMNODE    pAttrNode = xmlCreateNode(pNew,     // element
                                                  DOMNODE_ATTRIBUTE);
            if (pAttrNode)
            {
                xstrcpy(&pAttrNode->strNodeName, papcszAttribs[i], 0);
                xstrcpy(&pAttrNode->strNodeValue, papcszAttribs[i + 1], 0);
            }
        }
    }

    pDom->pLastWasTextNode = NULL;
}

/*
 *@@ EndElementHandler:
 *
 */

void EXPATENTRY EndElementHandler(void *data,             // in: our PXMLFILE really
                                  const XML_Char *name)
{
    PXMLDOM     pDom = (PXMLDOM)data;
    PLISTNODE   pNode = lstPop(&pDom->llStack);
    if (pNode)
        lstRemoveNode(&pDom->llStack, pNode);

    pDom->pLastWasTextNode = NULL;
}

/*
 *@@ CharacterDataHandler:
 *
 */

void EXPATENTRY CharacterDataHandler(void *userData,      // in: our PXMLFILE really
                                     const XML_Char *s,
                                     int len)
{
    PXMLDOM     pDom = (PXMLDOM)userData;

    ULONG       i;

    if (len)
    {
        if (pDom->pLastWasTextNode)
        {
            // we had a text node, and no elements or other
            // stuff in between:
            xstrcat(&pDom->pLastWasTextNode->strNodeValue,
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

            pNew = xmlCreateNode(pParent,
                                 DOMNODE_TEXT);
            if (pNew)
            {
                PSZ pszNodeValue = (PSZ)malloc(len + 1);
                memcpy(pszNodeValue, s, len);
                pszNodeValue[len] = '\0';
                xstrInitSet(&pNew->strNodeValue, pszNodeValue);
            }

            pDom->pLastWasTextNode = pNew;
        }
    }
}

/* ******************************************************************
 *
 *   DOM APIs
 *
 ********************************************************************/

/*
 *@@ xmlCreateDOM:
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
 *@@added V0.9.9 (2000-02-14) [umoeller]
 */

APIRET xmlCreateDOM(ULONG flParserFlags,
                    PXMLDOM *ppDom)
{
    APIRET  arc = NO_ERROR;

    PXMLDOM pDom = (PXMLDOM)malloc(sizeof(XMLDOM));
    if (!pDom)
        arc = ERROR_NOT_ENOUGH_MEMORY;
    else
    {
        memset(pDom, 0, sizeof(XMLDOM));

        lstInit(&pDom->llStack,
                FALSE);                 // no auto-free

        // create the document node
        pDom->pDocumentNode = xmlCreateNode(NULL, // no parent
                                            DOMNODE_DOCUMENT);

        if (!pDom->pDocumentNode)
            arc = ERROR_NOT_ENOUGH_MEMORY;
        else
        {
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
 *      If (fIsLast == TRUE), the internal expat parser
 *      will be freed, but not the DOM itself.
 *
 *      You can pass an XML document to this function
 *      in one flush. Set fIsLast = TRUE on the first
 *      and only call then.
 *
 *      This returns NO_ERROR if the chunk was successfully
 *      parsed. Otherwise ERROR_DOM_PARSING is returned,
 *      and you will find error information in the XMLDOM
 *      fields.
 *
 *@@added V0.9.9 (2000-02-14) [umoeller]
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
            // error:
            pDom->Error = XML_GetErrorCode(pDom->pParser);
            pDom->pcszErrorDescription = XML_ErrorString(pDom->Error);
            pDom->ulErrorLine = XML_GetCurrentLineNumber(pDom->pParser);
            pDom->ulErrorColumn = XML_GetCurrentColumnNumber(pDom->pParser);

            if (pDom->pDocumentNode)
            {
                xmlDeleteNode(pDom->pDocumentNode);
                pDom->pDocumentNode = NULL;
            }

            arc = ERROR_DOM_PARSING;
        }


        if (!fSuccess && fIsLast)
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
 *@@added V0.9.9 (2000-02-14) [umoeller]
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
