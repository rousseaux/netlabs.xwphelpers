
/*
 *@@sourcefile xml.h:
 *      header file for xml.c (XML parsing).
 *
 *      See remarks there.
 *
 *@@added V0.9.6 (2000-10-29) [umoeller]
 *@@include #include <os2.h>
 *@@include #include "expat\expat.h"
 *@@include #include "helpers\linklist.h"
 *@@include #include "helpers\xstring.h"
 *@@include #include "helpers\xml.h"
 */

#if __cplusplus
extern "C" {
#endif

#ifndef XML_HEADER_INCLUDED
    #define XML_HEADER_INCLUDED

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

    #define ERROR_DOM_FIRST                14000

    #define ERROR_DOM_INDEX_SIZE                   (ERROR_DOM_FIRST + 1)
    #define ERROR_DOM_DOMSTRING_SIZE               (ERROR_DOM_FIRST + 2)
    #define ERROR_DOM_HIERARCHY_REQUEST            (ERROR_DOM_FIRST + 3)
    #define ERROR_DOM_WRONG_DOCUMENT               (ERROR_DOM_FIRST + 4)
    #define ERROR_DOM_INVALID_CHARACTER            (ERROR_DOM_FIRST + 5)
    #define ERROR_DOM_NO_DATA_ALLOWED              (ERROR_DOM_FIRST + 6)
    #define ERROR_DOM_NO_MODIFICATION_ALLOWED      (ERROR_DOM_FIRST + 7)
    #define ERROR_DOM_NOT_FOUND                    (ERROR_DOM_FIRST + 8)
    #define ERROR_DOM_NOT_SUPPORTED                (ERROR_DOM_FIRST + 9)
    #define ERROR_DOM_INUSE_ATTRIBUTE              (ERROR_DOM_FIRST + 10)

    #define ERROR_DOM_PARSING                      (ERROR_DOM_FIRST + 11)

    #define DOMNODE_ELEMENT         1          // node is an ELEMENT
    #define DOMNODE_ATTRIBUTE       2          // node is an ATTRIBUTE
    #define DOMNODE_TEXT            3          // node is a TEXT node
    // #define DOMNODE_CDATA_SECTION   4
    // #define DOMNODE_ENTITY_REFERENCE   5
    // #define DOMNODE_ENTITY          6
    // #define DOMNODE_PROCESSING_INSTRUCTION   7
    #define DOMNODE_COMMENT         8          // node is a COMMENT
    #define DOMNODE_DOCUMENT        9          // node is a document
    #define DOMNODE_DOCUMENT_TYPE   10         // node is a DOCUMENTTYPE
    // #define DOMNODE_DOCUMENT_FRAGMENT   11
    // #define DOMNODE_NOTATION        12

    /*
     *@@ DOMNODE:
     *      this represents one node in an XML document.
     *
     *      Per definition, each XML document is broken down into
     *      DOM nodes. The document itself is represented by a
     *      node with the DOMNODE_DOCUMENT type, which in turn
     *      contains all the other nodes. This thus forms a tree
     *      as shown with xmlParse.
     *
     *      The contents of the members vary according
     *      to ulNodeType:
     *
     +      ulNodeType      pszNodeName     pszNodeValue        listAttributeNodes
     +
     +      ELEMENT         tag name        0                   named node map
     +
     +      ATTRIBUTE       attribute name  attribute value     0
     +
     +      TEXT            0               text contents       0
     +
     +      COMMENT         0               comment contents    0
     +
     +      DOCUMENT        0               0                   0
     */

    typedef struct _DOMNODE
    {
        ULONG       ulNodeType;
                    // one of:
                    // -- DOMNODE_ELEMENT
                    // -- DOMNODE_ATTRIBUTE
                    // -- DOMNODE_TEXT
                    // -- DOMNODE_CDATA_SECTION
                    // -- DOMNODE_ENTITY_REFERENCE
                    // -- DOMNODE_ENTITY
                    // -- DOMNODE_PROCESSING_INSTRUCTION
                    // -- DOMNODE_COMMENT
                    // -- DOMNODE_DOCUMENT: the root document. See @documents.
                    // -- DOMNODE_DOCUMENT_TYPE
                    // -- DOMNODE_DOCUMENT_FRAGMENT
                    // -- DOMNODE_NOTATION

        XSTRING     strNodeName;            // malloc()'d
        XSTRING     strNodeValue;           // malloc()'d

        struct _DOMNODE *pParentNode;
                        // the parent node;
                        // NULL for DOCUMENT, DOCUMENT_FRAGMENT and ATTR

        LINKLIST    llChildNodes;     // of DOMNODE* pointers, no auto-free

        LINKLIST    llAttributeNodes; // of DOMNODE* pointers, no auto-free

    } DOMNODE, *PDOMNODE;

    #define XMLACTION_BREAKUP               1
    #define XMLACTION_COPYASTEXT            2

    typedef ULONG _Optlink FNVALIDATE(const char *pcszTag);
    typedef FNVALIDATE *PFNVALIDATE;

    PDOMNODE xmlCreateNode(PDOMNODE pParentNode,
                           ULONG ulNodeType);

    ULONG xmlDeleteNode(PDOMNODE pNode);

    /* ******************************************************************
     *
     *   DOM APIs
     *
     ********************************************************************/

    /*
     *@@ XMLDOM:
     *      DOM instance returned by xmlCreateDOM.
     *
     *@@added V0.9.9 (2000-02-14) [umoeller]
     */

    typedef struct _XMLDOM
    {
        /*
         *  Public fields (should be read only)
         */

        PDOMNODE        pDocumentNode;

        enum XML_Error  Error;
        const char      *pcszErrorDescription;
        ULONG           ulErrorLine;
        ULONG           ulErrorColumn;

        /*
         *  Private fields (for xml* functions)
         */

        XML_Parser      pParser;
                            // expat parser instance

        LINKLIST        llStack;
                            // stack for maintaining the current items;
                            // these point to the NODERECORDs (no auto-free)

        PDOMNODE        pLastWasTextNode;
    } XMLDOM, *PXMLDOM;

    APIRET xmlCreateDOM(ULONG flParserFlags,
                        PXMLDOM *ppDom);

    APIRET xmlParse(PXMLDOM pDom,
                    const char *pcszBuf,
                    ULONG cb,
                    BOOL fIsLast);

    APIRET xmlFreeDOM(PXMLDOM pDom);

#endif

#if __cplusplus
}
#endif

