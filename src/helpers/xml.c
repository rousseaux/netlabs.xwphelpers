
/*
 *@@sourcefile xml.c:
 *      XML parsing.
 *
 *      This is vaguely modelled after the Document Object Model
 *      (DOM) standardized by the W3C.
 *
 *      In short, DOM specifies that an XML document is broken
 *      up into a tree of nodes, representing the various parts
 *      of an XML document. Most importantly, we have:
 *
 *      -- ELEMENT: some XML tag or a pair of tags (e.g. <LI>...<LI>.
 *
 *      -- ATTRIBUTE: an attribute to an element.
 *
 *      -- TEXT: a piece of, well, text.
 *
 *      -- COMMENT: a comment.
 *
 *      See xmlParse() for a more detailed explanation.
 *
 *      However, since this implementation was supposed to be a
 *      C-only interface, we do not implement inheritance. Instead,
 *      each XML document is broken up into a tree of DOMNODE's only,
 *      each of which has a special type.
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
 *      Copyright (C) 2000 Ulrich M”ller.
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

#define OS2EMX_PLAIN_CHAR
    // this is needed for "os2emx.h"; if this is defined,
    // emx will define PSZ as _signed_ char, otherwise
    // as unsigned char

#define INCL_DOSERRORS
#include <os2.h>

#include <stdlib.h>
#include <string.h>

#include "setup.h"                      // code generation and debugging options

#include "helpers\linklist.h"
#include "helpers\stringh.h"
#include "helpers\xml.h"

#pragma hdrstop

/*
 *@@category: Helpers\C helpers\XML\Node management
 */

/* ******************************************************************
 *
 *   Node Management
 *
 ********************************************************************/

/*
 *@@ xmlCreateNode:
 *      creates a new DOMNODE with the specified
 *      type and parent.
 */

PDOMNODE xmlCreateNode(PDOMNODE pParentNode,
                       ULONG ulNodeType)
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
            lstAppendItem(&pParentNode->listChildNodes,
                          pNewNode);
        }

        lstInit(&pNewNode->listChildNodes, FALSE);
        lstInit(&pNewNode->listAttributeNodes, FALSE);
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
        ulrc = DOMERR_NOT_FOUND;
    }
    else
    {
        // recurse into child nodes
        PLISTNODE   pNodeThis = lstQueryFirstNode(&pNode->listChildNodes);
        while (pNodeThis)
        {
            // recurse!!
            xmlDeleteNode((PDOMNODE)(pNodeThis->pItemData));

            pNodeThis = pNodeThis->pNext;
        }

        // delete attribute nodes
        pNodeThis = lstQueryFirstNode(&pNode->listAttributeNodes);
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
            lstRemoveItem(&pNode->pParentNode->listChildNodes,
                          pNode);
            pNode->pParentNode = NULL;
        }

        if (pNode->pszNodeName)
        {
            free(pNode->pszNodeName);
            pNode->pszNodeName = NULL;
        }
        if (pNode->pszNodeValue)
        {
            free(pNode->pszNodeValue);
            pNode->pszNodeValue = NULL;
        }

        free(pNode);
    }

    return (ulrc);
}

/*
 *@@category: Helpers\C helpers\XML\Parsing
 */

/* ******************************************************************
 *
 *   Tokenizing (Compiling)
 *
 ********************************************************************/

/*
 *@@ xmlTokenize:
 *      this takes any block of XML text and "tokenizes"
 *      it.
 *
 *      Tokenizing (or compiling, or "scanning" in bison/flex
 *      terms) means preparing the XML code for parsing later.
 *      This finds all tags and tag attributes and creates
 *      special codes for them in the output buffer.
 *
 *      For example:
 +
 +      <TAG ATTR="text"> block </TAG>
 +
 *      becomes
 *
 +      0xFF            escape code
 +      0x01            tag start code
 +      "TAG"           tag name
 +      0xFF            end of tag name code
 +
 +      0xFF            escape code
 +      0x03            attribute name code
 +      "ATTR"          attribute name
 +      0xFF
 +      "text"          attribute value (without quotes)
 +      0xFF            end of attribute code
 +
 +      " block "       regular text
 +
 +      0xFF            escape code
 +      0x01            tag start code
 +      "/TAG"          tag name
 +      0xFF            end of tag name code
 *
 *@@added V0.9.6 (2000-11-01) [umoeller]
 */

PSZ xmlTokenize(const char *pcszXML)
{
    return (0);
}

/* ******************************************************************
 *
 *   Parsing
 *
 ********************************************************************/

/*
 * TAGFOUND:
 *      structure created for each tag by BuildTagsList.
 */

typedef struct _TAGFOUND
{
    BOOL        fIsComment;
    const char  *pOpenBrck;
    const char  *pStartOfTagName;
    const char  *pFirstAfterTagName;
    const char  *pCloseBrck;             // ptr to '>' char; this plus one should
                                         // point to after the tag
} TAGFOUND, *PTAGFOUND;

/*
 * BuildTagsList:
 *      builds a LINKLIST containing TAGFOUND structs for
 *      each tag found in the specified buffer.
 *
 *      This is a flat list without any tree structure. This
 *      only searches for the tags and doesn't create any
 *      hierarchy.
 *
 *      The tags are simply added to the list in the order
 *      in which they are found in pcszBuffer.
 *
 *      The list is auto-free, you can simply do a lstFree
 *      to clean up.
 */

PLINKLIST BuildTagsList(const char *pcszBuffer)
{
    PLINKLIST    pllTags = lstCreate(TRUE);

    const char *pSearchPos = pcszBuffer;

    while ((pSearchPos) && (*pSearchPos))
    {
        // find first '<'
        PSZ     pOpenBrck = strchr(pSearchPos, '<');
        if (!pOpenBrck)
            // no open bracket found: stop search
            pSearchPos = 0;
        else
        {
            if (strncmp(pOpenBrck + 1, "!--", 3) == 0)
            {
                // it's a comment:
                // treat that differently
                const char *pEndOfComment = strstr(pOpenBrck + 4, "-->");
                const char *pCloseBrck = 0;
                const char *pFirstAfterTagName = 0;
                PTAGFOUND pTagFound;
                if (!pEndOfComment)
                {
                    // no end of comment found:
                    // skip entire rest of string
                    pCloseBrck = pOpenBrck + strlen(pOpenBrck);
                    pFirstAfterTagName = pCloseBrck;
                    pSearchPos = 0;
                }
                else
                {
                    pCloseBrck = pEndOfComment + 2; // point directly to '>'
                    pFirstAfterTagName = pCloseBrck + 1;
                }

                // append it to the list
                pTagFound = (PTAGFOUND)malloc(sizeof(TAGFOUND));
                if (!pTagFound)
                    // error:
                    pSearchPos = 0;
                else
                {
                    pTagFound->fIsComment = TRUE;
                    pTagFound->pOpenBrck = pOpenBrck;
                    pTagFound->pStartOfTagName = pOpenBrck + 1;
                    pTagFound->pFirstAfterTagName = pFirstAfterTagName;
                    pTagFound->pCloseBrck = pCloseBrck;

                    lstAppendItem(pllTags, pTagFound);
                }

                pSearchPos = pFirstAfterTagName;
            }
            else
            {
                // no comment:
                // find matching closing bracket
                const char *pCloseBrck = strchr(pOpenBrck + 1, '>');
                if (!pCloseBrck)
                    pSearchPos = 0;
                else
                {
                    const char *pNextOpenBrck = strchr(pOpenBrck + 1, '<');
                    // if we have another opening bracket before the closing bracket,
                    if ((pNextOpenBrck) && (pNextOpenBrck < pCloseBrck))
                        // ignore this one
                        pSearchPos = pNextOpenBrck;
                    else
                    {
                        // OK, apparently we have a tag.
                        // Skip all spaces after the tag.
                        const char *pTagName = pOpenBrck + 1;
                        while (    (*pTagName)
                                && (    (*pTagName == ' ')
                                     || (*pTagName == '\r')
                                     || (*pTagName == '\n')
                                   )
                              )
                            pTagName++;
                        if (!*pTagName)
                            // no tag name: stop
                            pSearchPos = 0;
                        else
                        {
                            // ookaaayyy, we got a tag now.
                            // Find first space or ">" after tag name:
                            const char *pFirstAfterTagName = pTagName + 1;
                            while (    (*pFirstAfterTagName)
                                    && (*pFirstAfterTagName != ' ')
                                    && (*pFirstAfterTagName != '\n')
                                    && (*pFirstAfterTagName != '\r')
                                    && (*pFirstAfterTagName != '\t')        // tab
                                    && (*pFirstAfterTagName != '>')
                                  )
                                pFirstAfterTagName++;
                            if (!*pFirstAfterTagName)
                                // no closing bracket found:
                                pSearchPos = 0;
                            else
                            {
                                // got a tag name:
                                // append it to the list
                                PTAGFOUND pTagFound = (PTAGFOUND)malloc(sizeof(TAGFOUND));
                                if (!pTagFound)
                                    // error:
                                    pSearchPos = 0;
                                else
                                {
                                    pTagFound->fIsComment = FALSE;
                                    pTagFound->pOpenBrck = pOpenBrck;
                                    pTagFound->pStartOfTagName = pTagName;
                                    pTagFound->pFirstAfterTagName = pFirstAfterTagName;
                                    pTagFound->pCloseBrck = pCloseBrck;

                                    lstAppendItem(pllTags, pTagFound);

                                    // search on after closing bracket
                                    pSearchPos = pCloseBrck + 1;
                                }
                            }
                        }
                    }
                } // end else if (!pCloseBrck)
            } // end else if (strncmp(pOpenBrck + 1, "!--"))
        } // end if (pOpenBrck)
    } // end while

    return (pllTags);
}

/*
 *@@ CreateTextNode:
 *      shortcut for creating a TEXT node. Calls
 *      xmlCreateNode in turn.
 *
 *      The text is extracted from in between the
 *      two pointers using strhSubstr.
 */

PDOMNODE CreateTextNode(PDOMNODE pParentNode,
                        const char *pStart,
                        const char *pEnd)
{
    PDOMNODE pNewTextNode = xmlCreateNode(pParentNode,
                                          DOMNODE_TEXT);
    if (pNewTextNode)
        pNewTextNode->pszNodeValue = strhSubstr(pStart,
                                                pEnd);

    return (pNewTextNode);
}

/*
 *@@ CreateElementNode:
 *      shortcut for creating a new ELEMENT node and
 *      parsing attributes at the same time.
 *
 *      pszTagName is assumed to be static (no copy
 *      is made).
 *
 *      pAttribs is assumed to point to an attributes
 *      string. This function creates ATTRIBUTE nodes
 *      from that string until either a null character
 *      or '>' is found.
 */

PDOMNODE CreateElementNode(PDOMNODE pParentNode,
                           PSZ pszTagName,
                           const char *pAttribs) // in: ptr to attribs; can be NULL
{
    PDOMNODE pNewNode = xmlCreateNode(pParentNode,
                                      DOMNODE_ELEMENT);
    if (pNewNode)
    {
        const char *p = pAttribs;

        pNewNode->pszNodeName = pszTagName;

        // find-start-of-attribute loop
        while (p)
        {
            switch (*p)
            {
                case 0:
                case '>':
                    p = 0;
                break;

                case ' ':
                case '\t':  // tab
                case '\n':
                case '\r':
                    p++;
                break;

                default:
                {
                    // first (or next) non-space:
                    // that's the start of an attrib, probably
                    // go until we find a space or '>'

                    const char *pNameStart = p,
                               *p2 = p;

                    const char *pEquals = 0,
                               *pFirstQuote = 0,
                               *pEnd = 0;       // last char... non-inclusive!

                    // copy-rest-of-attribute loop
                    while (p2)
                    {
                        switch (*p2)
                        {
                            case '"':
                                if (!pEquals)
                                {
                                    // '"' cannot appear before '='
                                    p2 = 0;
                                    p = 0;
                                }
                                else
                                {
                                    if (pFirstQuote)
                                    {
                                        // second quote:
                                        // get value between quotes
                                        pEnd = p2;
                                        // we're done with this one
                                        p = p2 + 1;
                                        p2 = 0;
                                    }
                                    else
                                    {
                                        // first quote:
                                        pFirstQuote = p2;
                                        p2++;
                                    }
                                }
                            break;

                            case '=':
                                if (!pEquals)
                                {
                                    // first equals sign:
                                    pEquals = p2;
                                    // extract name
                                    p2++;
                                }
                                else
                                    if (pFirstQuote)
                                        p2++;
                                    else
                                    {
                                        // error
                                        p2 = 0;
                                        p = 0;
                                    }
                            break;

                            case ' ':
                            case '\t':  // tab
                            case '\n':
                            case '\r':
                                // spaces can appear in quotes
                                if (pFirstQuote)
                                    // just continue
                                    p2++;
                                else
                                {
                                    // end of it!
                                    pEnd = p2;
                                    p = p2 + 1;
                                    p2 = 0;
                                }
                            break;

                            case 0:
                            case '>':
                            {
                                pEnd = p2;
                                // quit inner AND outer loop
                                p2 = 0;
                                p = 0;
                            break; }

                            default:
                                p2++;
                        }
                    } // end while (p2)

                    if (pEnd)
                    {
                        PDOMNODE pAttribNode = xmlCreateNode(pNewNode,
                                                             DOMNODE_ATTRIBUTE);
                        if (pAttribNode)
                        {
                            if (pEquals)
                            {
                                pAttribNode->pszNodeName
                                    = strhSubstr(pNameStart, pEquals);

                                // did we have quotes?
                                if (pFirstQuote)
                                    pAttribNode->pszNodeValue
                                        = strhSubstr(pFirstQuote + 1, pEnd);
                                else
                                    pAttribNode->pszNodeValue
                                        = strhSubstr(pEquals + 1, pEnd);
                            }
                            else
                                // no "equals":
                                pAttribNode->pszNodeName
                                    = strhSubstr(pNameStart, pEnd);
                        }
                    }
                break; }
            }
        }
    }

    return (pNewNode);
}

/*
 *@@ CreateNodesForBuf:
 *      this gets called (recursively) for a piece of text
 *      for which we need to create TEXT and ELEMENT DOMNODE's.
 *
 *      This does the heavy work for xmlParse.
 *
 *      If an error (!= 0) is returned, *ppError points to
 *      the code part that failed.
 */

ULONG CreateNodesForBuf(const char *pcszBufStart,
                        const char *pcszBufEnd,     // in: can be NULL
                        PLINKLIST pllTagsList,
                        PDOMNODE pParentNode,
                        PFNVALIDATE pfnValidateTag,
                        const char **ppError)
{
    ULONG ulrc = 0;
    PLISTNODE pCurrentTagListNode = lstQueryFirstNode(pllTagsList);
    const char *pBufCurrent = pcszBufStart;
    BOOL        fContinue = TRUE;

    if (pcszBufEnd == NULL)
        pcszBufEnd = pcszBufStart + strlen(pcszBufStart);

    while (fContinue)
    {
        if (    (!*pBufCurrent)
             || (pBufCurrent == pcszBufEnd)
           )
            // end of buf reached:
            fContinue = FALSE;

        else if (!pCurrentTagListNode)
        {
            // no (more) tags for this buffer:
            CreateTextNode(pParentNode,
                           pBufCurrent,
                           pcszBufEnd);
            fContinue = FALSE;
        }
        else
        {
            // another tag found:
            PTAGFOUND pFoundTag = (PTAGFOUND)pCurrentTagListNode->pItemData;
            const char *pStartOfTag = pFoundTag->pOpenBrck;
            if (pStartOfTag > pBufCurrent + 1)
            {
                // we have text before the opening tag:
                // make a DOMTEXT out of this
                CreateTextNode(pParentNode,
                               pBufCurrent,
                               pStartOfTag);
                pBufCurrent = pStartOfTag;
            }
            else
            {
                // OK, go for this tag...

                if (*(pFoundTag->pStartOfTagName) == '/')
                {
                    // this is a closing tag: that's an error
                    ulrc = 1;
                    *ppError = pFoundTag->pStartOfTagName;
                    fContinue = FALSE;
                }
                else if (pFoundTag->fIsComment)
                {
                    // it's a comment: that's simple
                    PDOMNODE pCommentNode = xmlCreateNode(pParentNode,
                                                          DOMNODE_COMMENT);
                    if (!pCommentNode)
                        ulrc = ERROR_NOT_ENOUGH_MEMORY;
                    else
                    {
                        pCommentNode->pszNodeValue = strhSubstr(pFoundTag->pOpenBrck + 4,
                                                                pFoundTag->pCloseBrck - 2);
                    }
                    pBufCurrent = pFoundTag->pCloseBrck + 1;
                }
                else
                {
                    BOOL fKeepTagName = FALSE;       // free pszTagName below
                    PSZ pszTagName = strhSubstr(pFoundTag->pStartOfTagName,
                                                pFoundTag->pFirstAfterTagName);
                    if (!pszTagName)
                        // zero-length string:
                        // go ahead after that
                        pBufCurrent = pFoundTag->pCloseBrck + 1;
                    else
                    {
                        // XML knows two types of elements:

                        // a) Element pairs, which have opening and closing tags
                        //    (<TAG> and </TAG>
                        // b) Single elements, which must have "/" as their last
                        //    character; these have no closing tag
                        //    (<TAG/>)

                        // However, HTML doesn't usually tag single elements
                        // with a trailing '/'. To maintain compatibility,
                        // if we don't find a matching closing tag, we extract
                        // everything up to the end of the buffer.

                        ULONG   ulTagNameLen = strlen(pszTagName);

                        // search for closing tag first...
                        // create string with closing tag to search for;
                        // that's '/' plus opening tag name
                        ULONG   ulClosingTagLen2Find = ulTagNameLen + 1;
                        PSZ     pszClosingTag2Find = (PSZ)malloc(ulClosingTagLen2Find + 1); // plus null byte
                        PLISTNODE pTagListNode2 = pCurrentTagListNode->pNext;
                        PLISTNODE pTagListNodeForChildren = pTagListNode2;

                        BOOL    fClosingTagFound = FALSE;

                        *pszClosingTag2Find = '/';
                        strcpy(pszClosingTag2Find + 1, pszTagName);

                        // now find matching closing tag
                        while (pTagListNode2)
                        {
                            PTAGFOUND pFoundTag2 = (PTAGFOUND)pTagListNode2->pItemData;
                            ULONG ulFoundTag2Len = (pFoundTag2->pFirstAfterTagName - pFoundTag2->pStartOfTagName);
                            // compare tag name lengths
                            if (ulFoundTag2Len == ulClosingTagLen2Find)
                            {
                                // same length:
                                // compare
                                if (memcmp(pFoundTag2->pStartOfTagName,
                                           pszClosingTag2Find,
                                           ulClosingTagLen2Find)
                                     == 0)
                                {
                                    // found matching closing tag:

                                    // we now have
                                    // -- pCurrentTagListNode pointing to the opening tag
                                    //    (pFoundTag has its PTAGFOUND item data)
                                    // -- pTagListNode2 pointing to the closing tag
                                    //    (pFoundTag2 has its PTAGFOUND item data)

                                    // create DOM node
                                    PDOMNODE pNewNode = CreateElementNode(pParentNode,
                                                                          pszTagName,
                                                                          pFoundTag->pFirstAfterTagName);
                                    if (pNewNode)
                                    {
                                        ULONG       ulAction = XMLACTION_BREAKUP;

                                        fKeepTagName = TRUE;    // do not free below

                                        // validate tag
                                        if (pfnValidateTag)
                                        {
                                            // validator specified:
                                            ulAction = pfnValidateTag(pszTagName);
                                        }

                                        if (ulAction == XMLACTION_COPYASTEXT)
                                        {
                                            CreateTextNode(pNewNode,
                                                           pFoundTag->pCloseBrck + 1,
                                                           pFoundTag2->pOpenBrck - 1);
                                        }
                                        else if (ulAction == XMLACTION_BREAKUP)
                                        {
                                            PLINKLIST   pllSubList = lstCreate(FALSE);
                                            PLISTNODE   pSubNode = 0;
                                            ULONG       cSubNodes = 0;

                                            // text buffer to search
                                            const char *pSubBufStart = pFoundTag->pCloseBrck + 1;
                                            const char *pSubBufEnd = pFoundTag2->pOpenBrck;

                                            // create a child list containing
                                            // all tags from the first tag after
                                            // the current opening tag to the closing tag
                                            for (pSubNode = pTagListNodeForChildren;
                                                 pSubNode != pTagListNode2;
                                                 pSubNode = pSubNode->pNext)
                                            {
                                                lstAppendItem(pllSubList,
                                                              pSubNode->pItemData);
                                                cSubNodes++;
                                            }

                                            // now recurse to build child nodes
                                            // (text and elements), even if the
                                            // list is empty, we can have text!
                                            CreateNodesForBuf(pSubBufStart,
                                                              pSubBufEnd,
                                                              pllSubList,
                                                              pNewNode,
                                                              pfnValidateTag,
                                                              ppError);

                                            lstFree(pllSubList);
                                        } // end if (ulAction == XMLACTION_BREAKUP)

                                        // now search on after the closing tag
                                        // we've found; the next tag will be set below
                                        pCurrentTagListNode = pTagListNode2;
                                        pBufCurrent = pFoundTag2->pCloseBrck + 1;

                                        fClosingTagFound = TRUE;

                                        break; // // while (pTagListNode2)
                                    } // end if (pNewNode)
                                } // end if (memcmp(pFoundTag2->pStartOfTagName,
                            } // if (ulFoundTag2Len == ulClosingTagLen2Find)

                            pTagListNode2 = pTagListNode2->pNext;

                        } // while (pTagListNode2)

                        if (!fClosingTagFound)
                        {
                            // no matching closing tag found:
                            // that's maybe a block of not well-formed XML

                            // e.g. with WarpIN:
                            // <README> <-- we start after this
                            //      block of plain HTML with <P> tags and such
                            // </README>

                            // just create an element
                            PDOMNODE pNewNode = CreateElementNode(pParentNode,
                                                                  pszTagName,
                                                                  pFoundTag->pFirstAfterTagName);
                            if (pNewNode)
                                fKeepTagName = TRUE;

                            // now search on after the closing tag
                            // we've found; the next tag will be set below
                            // pCurrentTagListNode = pTagListNodeForChildren;
                            pBufCurrent = pFoundTag->pCloseBrck + 1;
                        }

                        free(pszClosingTag2Find);

                        if (!fKeepTagName)
                            free(pszTagName);
                    } // end if (pszTagName)
                }

                pCurrentTagListNode = pCurrentTagListNode->pNext;
            }
        }
    }

    return (ulrc);
}

/*
 * xmlParse:
 *      generic XML parser.
 *
 *      This takes the specified zero-terminated string
 *      in pcszBuf and parses it, adding DOMNODE's as
 *      children to pNode.
 *
 *      This recurses, if necessary, to build a node tree.
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
 +                    ÚÄÄÄÄÄÄÄÄÄÄÄÄ¿
 +                    ³   TABLE    ³        (only ELEMENT node in root DOCUMENT node)
 +                    ÀÄÄÄÄÄÂÄÄÄÄÄÄÙ
 +                          ³
 +                    ÚÄÄÄÄÄÁÄÄÄÄÄÄ¿
 +                    ³   TBODY    ³        (only ELEMENT node in root "TABLE" node)
 +                    ÀÄÄÄÄÄÂÄÄÄÄÄÄÙ
 +              ÚÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄ¿
 +        ÚÄÄÄÄÄÁÄÄÄÄÄÄ¿          ÚÄÄÄÄÄÁÄÄÄÄÄÄ¿
 +        ³   TR       ³          ³   TR       ³
 +        ÀÄÄÄÄÄÂÄÄÄÄÄÄÙ          ÀÄÄÄÄÄÂÄÄÄÄÄÄÙ
 +          ÚÄÄÄÁÄÄÄÄÄÄ¿            ÚÄÄÄÁÄÄÄÄÄÄ¿
 +      ÚÄÄÄÁÄ¿     ÚÄÄÁÄÄ¿     ÚÄÄÄÁÄ¿     ÚÄÄÁÄÄ¿
 +      ³ TD  ³     ³ TD  ³     ³ TD  ³     ³ TD  ³
 +      ÀÄÄÂÄÄÙ     ÀÄÄÂÄÄÙ     ÀÄÄÄÂÄÙ     ÀÄÄÂÄÄÙ
 +   ÉÍÍÍÍÍÊÍÍÍÍ» ÉÍÍÍÍÊÍÍÍÍÍ» ÉÍÍÍÍÊÍÍÍÍÍ» ÉÍÍÊÍÍÍÍÍÍÍ»
 +   ºColumn 1-1º ºColumn 1-2º ºColumn 2-1º ºColumn 2-2º    (one TEXT node in each parent node)
 +   ÈÍÍÍÍÍÍÍÍÍÍ¼ ÈÍÍÍÍÍÍÍÍÍÍ¼ ÈÍÍÍÍÍÍÍÍÍÍ¼ ÈÍÍÍÍÍÍÍÍÍÍ¼
 */

ULONG xmlParse(PDOMNODE pParentNode,        // in: node to append children to; must not be NULL
               const char *pcszBuf,         // in: buffer to search
               PFNVALIDATE pfnValidateTag)
{
    ULONG   ulrc = 0;

    PLINKLIST pllTags = BuildTagsList(pcszBuf);

    // now create DOMNODE's according to that list...
    const char *pcszError = 0;
    CreateNodesForBuf(pcszBuf,
                      NULL,     // enitre buffer
                      pllTags,
                      pParentNode,
                      pfnValidateTag,
                      &pcszError);

    lstFree(pllTags);

    return (ulrc);
}

/*
 *@@ xmlCreateDocumentFromString:
 *      creates a DOCUMENT DOMNODE and calls xmlParse
 *      to break down the specified buffer into that
 *      node.
 */

PDOMNODE xmlCreateDocumentFromString(const char *pcszXML,
                                     PFNVALIDATE pfnValidateTag)
{
    PDOMNODE pDocument = xmlCreateNode(NULL,       // no parent
                                       DOMNODE_DOCUMENT);
    xmlParse(pDocument,
             pcszXML,
             pfnValidateTag);

    return (pDocument);
}


