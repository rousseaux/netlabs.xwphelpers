
/*
 *@@sourcefile xml.h:
 *      header file for xml.c (XML parsing).
 *
 *      See remarks there.
 *
 *@@added V0.9.6 (2000-10-29) [umoeller]
 *@@include #include <os2.h>
 *@@include #include "linklist.h"
 *@@include #include "xml.h"
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

    #define DOMERR_INDEX_SIZE                  1;
    #define DOMERR_DOMSTRING_SIZE              2;
    #define DOMERR_HIERARCHY_REQUEST           3;
    #define DOMERR_WRONG_DOCUMENT              4;
    #define DOMERR_INVALID_CHARACTER           5;
    #define DOMERR_NO_DATA_ALLOWED             6;
    #define DOMERR_NO_MODIFICATION_ALLOWED     7;
    #define DOMERR_NOT_FOUND                   8;
    #define DOMERR_NOT_SUPPORTED               9;
    #define DOMERR_INUSE_ATTRIBUTE            10;

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

        PSZ         pszNodeName;
        PSZ         pszNodeValue;

        struct _DOMNODE *pParentNode;
                        // the parent node;
                        // NULL for DOCUMENT, DOCUMENT_FRAGMENT and ATTR
        LINKLIST    listChildNodes;     // of DOMNODE* pointers

        LINKLIST    listAttributeNodes; // of DOMNODE* pointers

    } DOMNODE, *PDOMNODE;

    #define XMLACTION_BREAKUP               1
    #define XMLACTION_COPYASTEXT            2

    typedef ULONG _Optlink FNVALIDATE(const char *pcszTag);
    typedef FNVALIDATE *PFNVALIDATE;

    PDOMNODE xmlCreateNode(PDOMNODE pParentNode,
                           ULONG ulNodeType);

    ULONG xmlDeleteNode(PDOMNODE pNode);

    ULONG xmlParse(PDOMNODE pParentNode,
                   const char *pcszBuf,
                   PFNVALIDATE pfnValidateTag);

    PDOMNODE xmlCreateDocumentFromString(const char *pcszXML,
                                         PFNVALIDATE pfnValidateTag);

#endif

#if __cplusplus
}
#endif

