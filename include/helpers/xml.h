
/*
 *@@sourcefile xml.h:
 *      header file for xml.c (XML parsing).
 *
 *      See remarks there.
 *
 *@@added V0.9.6 (2000-10-29) [umoeller]
 *@@include #include <os2.h>
 *@@include #include "expat\expat.h"                // must come before xml.h
 *@@include #include "helpers\linklist.h"
 *@@include #include "helpers\tree.h"
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

    typedef enum _DOMERROR
    {
        // validity errors:
// START MATCHING ERROR MESSAGES (xmlDescribeError)
        ERROR_DOM_UNDECLARED_ELEMENT = ERROR_EXPAT_AFTER_LAST,
                // invalidity: element is undeclared
        ERROR_DOM_ROOT_ELEMENT_MISNAMED,
        ERROR_DOM_INVALID_ROOT_ELEMENT,
        ERROR_DOM_INVALID_SUBELEMENT,
                // subelement may not appear in its parent element
        ERROR_DOM_DUPLICATE_ELEMENT_DECL,
                // more than one declaration for an element type
        ERROR_DOM_DUPLICATE_ATTRIBUTE_DECL,
                // more than one declaration for an attribute type
        ERROR_DOM_UNDECLARED_ATTRIBUTE,
        ERROR_ELEMENT_CANNOT_HAVE_CONTENT,
                // element was declared "empty" and contains text anyway,
                // or was declared "children" and contains something other
                // than whitespace
        ERROR_DOM_INVALID_ATTRIB_VALUE,
        ERROR_DOM_REQUIRED_ATTRIBUTE_MISSING,
        ERROR_DOM_SUBELEMENT_IN_EMPTY_ELEMENT,
// END MATCHING ERROR MESSAGES (xmlDescribeError)

        // error categories:
        ERROR_DOM_PARSING,
        ERROR_DOM_VALIDITY,

        // additional DOM errors
        ERROR_DOM_NODETYPE_NOT_SUPPORTED,
                // invalid node type in xmlCreateDomNode
        ERROR_DOM_NO_DOCUMENT,
                // cannot find document node
        ERROR_DOM_NO_ELEMENT,
        ERROR_DOM_DUPLICATE_DOCTYPE,
        ERROR_DOM_DOCTYPE_STRUCTURE,
                // DOCTYPE is given and root element name does not match doctype name
        ERROR_DOM_INTEGRITY,
        ERROR_DOM_DUPLICATE_ATTRIBUTE
    } DOMERROR;

    /* ******************************************************************
     *
     *   Most basic node management
     *
     ********************************************************************/

    // content model node types:
    typedef enum _NODEBASETYPE
    {
        TYPE_UNKNOWN,

        DOMNODE_ELEMENT,                // node is a DOM ELEMENT
        DOMNODE_ATTRIBUTE,              // node is a DOM ATTRIBUTE
        DOMNODE_TEXT,                   // node is a DOM TEXT node
            // DOMNODE_CDATA_SECTION   4
            // DOMNODE_ENTITY_REFERENCE   5
            // DOMNODE_ENTITY          6
        DOMNODE_PROCESSING_INSTRUCTION, // node is a DOM PI
        DOMNODE_COMMENT,                // node is a DOM COMMENT
        DOMNODE_DOCUMENT,               // node is a DOM document
        DOMNODE_DOCUMENT_TYPE,          // node is a DOM DOCUMENTTYPE
            // #define DOMNODE_DOCUMENT_FRAGMENT   11
            // #define DOMNODE_NOTATION        12

        // the following are all CMELEMENTPARTICLE nodes
        ELEMENTPARTICLE_EMPTY,
        ELEMENTPARTICLE_ANY,
        ELEMENTPARTICLE_MIXED,
        ELEMENTPARTICLE_CHOICE,
        ELEMENTPARTICLE_SEQ,
        ELEMENTPARTICLE_NAME,

        ATTRIBUTE_DECLARATION_BASE,     // node is a CMATTRIBUTEDECLBASE
        ATTRIBUTE_DECLARATION,          // node is a CMATTRIBUTEDECL
        ATTRIBUTE_DECLARATION_ENUM      // node is a plain NODEBASE, part of an attr value enum
    } NODEBASETYPE;

    /*
     *@@ NODEBASE:
     *      "content model" node. With the DOM content models,
     *      this represents an entry in a DTD or XML schema.
     *
     *@@added V0.9.9 (2001-02-14) [umoeller]
     */

    typedef struct _NODEBASE
    {
        TREE            Tree;

        NODEBASETYPE    ulNodeType;

        XSTRING         strNodeName;
                    // node name;
                    // -- for the various DOMNODE_* items, see _DOMNODE;
                    // -- for CMELEMENTPARTICLE nodes, this is the particle's name
                    // -- for CMELEMENTDECLNODE nodes, element name being declared
                    // -- for CMATTRIBUTEDECLBASE nodes, name of element to which this
                    //          attrib decl belongs
                    // -- for CMATTRIBUTEDECL nodes, name of attribute;
                    // -- for ATTRIBUTE_DECLARATION_ENUM, attribute value in the
                    //          possible values list.

    } NODEBASE, *PNODEBASE;

    /* ******************************************************************
     *
     *   DOM level 1
     *
     ********************************************************************/

    /*
     *@@ DOMNODE:
     *      this represents one @DOM node in an @XML document.
     *
     *      The document itself is represented by a node with the
     *      DOMNODE_DOCUMENT type, which is the root of a tree as
     *      shown with xmlParse.
     *
     *      The contents of the members vary according
     *      to ulNodeType (0 specifies that the field does not
     *      apply to that type).
     *
     *      The first member of a DOMNODE is a NODEBASE to allow
     *      inserting these things in a tree. NODEBASE.ulNodeType
     *      _always_ specifies the various types that are using
     *      that structure to allow for type-safety (if we watch out).
     *      This is for faking inheritance.
     *
     *      Note that we also implement specialized sub-structures of
     *      DOMNODE, whose first member is the DOMNODE (and therefore
     *      a NODEBASE as well):
     *
     *      --  DOCUMENT nodes are given a _DOMDOCUMENTNODE structure.
     *
     *      --  DOCTYPE nodes are given a _DOMDOCTYPENODE structure.
     *
     *      Overview of member fields usage:
     +
     +      ulNodeType    | strNodeName | strNodeValue | llChildren | llAttributes
     +      =======================================================================
     +                    |             |              |            |
     +      DOCUMENT      | name from   | 0            | 1 root     | 0
     +                    | DOCTYPE or  |              | ELEMENT    |
     +                    | NULL        |              |            |
     +                    |             |              |            |
     +      --------------+-------------+--------------+------------+--------------
     +                    |             |              |            |
     +      ELEMENT       | tag name    | 0          8  | ELEMENT    | ATTRIBUTE
     +                    |             |              | nodes      | nodes
     +                    |             |              |            |
     +      --------------+-------------+--------------+------------+--------------
     +                    |             |              |            |
     +      ATTRIBUTE     | attribute   | attribute    | 0          | 0
     +                    | name        | value        |            |
     +                    |             |              |            |
     +      --------------+-------------+--------------+------------+--------------
     +                    |             |              |            |
     +      TEXT          | 0           | text         | 0          | 0
     +                    |             | contents     |            |
     +                    |             |              |            |
     +      --------------+-------------+--------------+------------+--------------
     +                    |             |              |            |
     +      COMMENT       | 0           | comment      | 0          | 0
     +                    |             | contents     |            |
     +                    |             |              |            |
     +      --------------+-------------+--------------+------------+--------------
     +                    |             |              |            |
     +      PI            | PI target   | PI data      | 0          | 0
     +                    |             |              |            |
     +                    |             |              |            |
     +      --------------+-------------+--------------+------------+--------------
     +                    |             |              |            |
     +      DOCTYPE       | doctype     |              | 0          | 0
     +                    | name        |              |            |
     +                    |             |              |            |
     +
     *      The xwphelpers implementation does not implement CDATA sections,
     *      for which we have no need because @expat properly converts these
     *      into plain @content.
     *
     *      In addition, W3C DOM specifies that the "node name" members contain
     *      "#document", "#text", and "#comment" strings for DOCUMENT,
     *      TEXT, and COMMENT nodes, respectively. I see no point in this other
     *      than consuming memory, so these fields are empty with this implementation.
     */

    typedef struct _DOMNODE
    {
        NODEBASE        NodeBase;

        PXSTRING        pstrNodeValue;           // ptr is NULL if none

        struct _DOMNODE *pParentNode;
                        // the parent node;
                        // NULL for DOCUMENT, DOCUMENT_FRAGMENT.
                        // The DOM spec says that attribs have no parent,
                        // but even though the attribute is not added to
                        // the "children" list of an element (but to the
                        // attributes map instead), we specify the element
                        // as the attribute's parent here.

        struct _DOMNODE *pDocumentNode;
                        // the document node, unless this is a DOCUMENT in itself.

        LINKLIST        llChildren;     // of DOMNODE* pointers, no auto-free

        TREE            *AttributesMap; // of DOMNODE* pointers

    } DOMNODE, *PDOMNODE;

    /*
     *@@ DOMDOCTYPENODE:
     *      specific _DOMNODE replacement structure which
     *      is used for DOCTYPE nodes.
     *
     *      The DOMDOCTYPENODE is special (other than having
     *      extra fields) in that it is stored both in
     *      the document node's children list and in its
     *      pDocType field.
     *
     *      DOMNODE.pstrNodeName is set to the name in the
     *      DOCTYPE statement by xmlCreateDocumentTypeNode,
     *      or is NULL if there's no DOCTYPE.
     *
     *@@added V0.9.9 (2001-02-14) [umoeller]
     */

    typedef struct _DOMDOCTYPENODE
    {
        DOMNODE     DomNode;

        XSTRING     strPublicID;
        XSTRING     strSystemID;

        BOOL        fHasInternalSubset;

        TREE        *ElementDeclsTree;
                    // tree with pointers to _CMELEMENTDECLNODE nodes

        TREE        *AttribDeclBasesTree;
                    // tree with pointers to _CMATTRIBUTEDECLBASE nodes

    } DOMDOCTYPENODE, *PDOMDOCTYPENODE;

    /*
     *@@ DOMDOCUMENTNODE:
     *      specific _DOMNODE replacement structure which
     *      is used for DOCUMENT nodes.
     *
     *@@added V0.9.9 (2001-02-14) [umoeller]
     */

    typedef struct _DOMDOCUMENTNODE
    {
        DOMNODE             DomNode;

        PDOMDOCTYPENODE     pDocType;
                        // != NULL if DOCTYPE was found

    } DOMDOCUMENTNODE, *PDOMDOCUMENTNODE;

    APIRET xmlCreateDomNode(PDOMNODE pParentNode,
                            NODEBASETYPE ulNodeType,
                            const char *pcszNodeName,
                            ULONG ulNodeNameLength,
                            PDOMNODE *ppNew);

    VOID xmlDeleteNode(PNODEBASE pNode);

    /* ******************************************************************
     *
     *   DOM level 3 content models
     *
     ********************************************************************/

    // data types (XML schemes):
    #define STRING_DATATYPE                1
    #define BOOLEAN_DATATYPE               2
    #define FLOAT_DATATYPE                 3
    #define DOUBLE_DATATYPE                4
    #define LONG_DATATYPE                  5
    #define INT_DATATYPE                   6
    #define SHORT_DATATYPE                 7
    #define BYTE_DATATYPE                  8

    /*
     *@@ CMELEMENTPARTICLE:
     *      element declaration particle in a
     *      _CMELEMENTDECLNODE.
     *
     *      One of these structures is a full
     *      (non-pointer) member in _CMELEMENTDECLNODE.
     *      This struct in turn has a linked list with
     *      possible subnodes. See _CMELEMENTDECLNODE.
     *
     *@@added V0.9.9 (2001-02-16) [umoeller]
     */

    typedef struct _CMELEMENTPARTICLE
    {
        NODEBASE          NodeBase;       // has TREE* as first item in turn
                    // NODEBASE.ulNodeType may be one of these:
                    // -- ELEMENTPARTICLE_EMPTY:
                    //          ulRepeater will be XML_CQUANT_NONE, rest is NULL
                    // -- ELEMENTPARTICLE_ANY:
                    //          ulRepeater will be XML_CQUANT_NONE, rest is NULL
                    // -- ELEMENTPARTICLE_MIXED:
                    //          mixed content (with PCDATA); if the list contains
                    //          something, the element may have PCDATA and sub-elements
                    //          mixed
                    // -- ELEMENTPARTICLE_CHOICE:
                    //          list is a choicelist
                    // -- ELEMENTPARTICLE_SEQ:
                    //          list is a seqlist
                    // -- ELEMENTPARTICLE_NAME:
                    //          used for terminal particles in a parent particle's
                    //          list, which finally specifies the name of a sub-particle.
                    //          This can never appear in a root particle.

        ULONG           ulRepeater;
                    // one of:
                    // -- XML_CQUANT_NONE   --> all fields below are NULL
                    // -- XML_CQUANT_OPT,
                    // -- XML_CQUANT_REP,
                    // -- XML_CQUANT_PLUS

        struct _CMELEMENTPARTICLE *pParentParticle;     // or NULL if this is in the
                                                        // CMELEMENTDECLNODE

        PLINKLIST       pllSubNodes;
                    // linked list of sub-CMELEMENTPARTICLE structs
                    // (for mixed, choice, seq types);
                    // if NULL, there's no sub-CMELEMENTPARTICLE

    } CMELEMENTPARTICLE, *PCMELEMENTPARTICLE;

    /*
     *@@ CMELEMENTDECLNODE:
     *      representation of an @element_declaration within a
     *      _DOMDOCTYPENODE (a document @DTD).
     *
     *      This is complicated because element declarations
     *      are complicated with nested lists and content
     *      particles. For this, we introduce the representation
     *      of a _CMELEMENTPARTICLE, which is contained in the
     *      "Particle" member.
     *
     *      For minimal memory consumption, the _CMELEMENTDECLNODE
     *      is an _CMELEMENTPARTICLE with extra fields, while the
     *      list in _CMELEMENTPARTICLE points to plain
     *      _CMELEMENTPARTICLE structs only.
     *
     *      For the "root" element declaration in the DTD,
     *      Particle.NODEBASE.ulNodeType will always be one of the following:
     *
     *      -- ELEMENTPARTICLE_EMPTY: element must be empty.
     *
     *      -- ELEMENTPARTICLE_ANY: element can have any content.
     *
     *      -- ELEMENTPARTICLE_CHOICE: _CMELEMENTPARTICLE has a choicelist with
     *                           more _CMELEMENTPARTICLE structs.
     *
     *      -- ELEMENTPARTICLE_SEQ: _CMELEMENTPARTICLE has a seqlist with
     *                           more _CMELEMENTPARTICLE structs.
     *
     *      -- ELEMENTPARTICLE_MIXED: element can have mixed content including #PCDATA.
     *            If there is no content particle list, then the element may
     *            ONLY have PCDATA. If there's a content particle list, then the
     *            element may have both sub-elements and PCDATA. Oh my.
     *
     *@@added V0.9.9 (2001-02-14) [umoeller]
     */

    typedef struct _CMELEMENTDECLNODE
    {
        CMELEMENTPARTICLE   Particle;
                    // root particle for this element decl; this may contain
                    // sub-particles...
                    // this has a NODEBASE as first member, which has TREE* as
                    // first item in turn

        TREE            *ParticleNamesTree;
                    // tree sorted by element names with all sub-particles,
                    // no matter how deeply nested; this is just for quickly
                    // checking if an element name is allowed as a sub-element
                    // at all. Tree items are _CMELEMENTPARTICLE nodes.

    } CMELEMENTDECLNODE, *PCMELEMENTDECLNODE;

    typedef enum _ATTRIBCONSTRAINT
    {
        CMAT_IMPLIED,
        CMAT_REQUIRED,
        CMAT_DEFAULT_VALUE,
        CMAT_FIXED_VALUE
    } ATTRIBCONSTRAINT;

    typedef enum _ATTRIBTYPE
    {
        CMAT_CDATA,
        CMAT_ID,
        CMAT_IDREF,
        CMAT_IDREFS,
        CMAT_ENTITY,
        CMAT_ENTITIES,
        CMAT_NMTOKEN,
        CMAT_NMTOKENS,
        CMAT_ENUM
    } ATTRIBTYPE;

    /*
     *@@ CMATTRIBUTEDECL:
     *      single attribute declaration within the attribute
     *      declarations tree in _CMATTRIBUTEDECLBASE.
     *
     *@@added V0.9.9 (2001-02-16) [umoeller]
     */

    typedef struct _CMATTRIBUTEDECL
    {
        NODEBASE          NodeBase;       // has TREE* as first item in turn
                    // NodeBase.strName is attribute name

        ATTRIBTYPE      ulAttrType;
                    // one of:
                    // -- CMAT_CDATA
                    // -- CMAT_ID
                    // -- CMAT_IDREF
                    // -- CMAT_IDREFS
                    // -- CMAT_ENTITY
                    // -- CMAT_ENTITIES
                    // -- CMAT_NMTOKEN
                    // -- CMAT_NMTOKENS
                    // -- CMAT_ENUM: pllEnum lists the allowed values.
        TREE            *ValuesTree;
                    // enumeration of allowed values, if CMAT_ENUM;
                    // tree entries are plain NODEBASEs with
                    // ATTRIBUTE_DECLARATION_ENUM type

        ATTRIBCONSTRAINT    ulConstraint;
                    // one of:
                    // -- CMAT_IMPLIED: attrib can have any value.
                    // -- CMAT_REQUIRED: attrib must be specified.
                    // -- CMAT_DEFAULT_VALUE: attrib is optional and has default
                    //                        value as in pstrDefaultValue.
                    // -- CMAT_FIXED_VALUE: attrib is optional, but must have
                    //                        fixed value as in pstrDefaultValue.
        PXSTRING        pstrDefaultValue;
                    // default value of this attribute; NULL with implied or required

    } CMATTRIBUTEDECL, *PCMATTRIBUTEDECL;

    /*
     *@@ CMATTRIBUTEDECLBASE:
     *      representation of an @attribute_declaration.
     *
     *      I'd love to have stored the attribute declarations with
     *      the element specifications, but the XML spec says that
     *      attribute declarations are allowed even if no element
     *      declaration exists for the element to which the attribute
     *      belongs. Now, whatever this is good for... anyway, this
     *      forces us to do a second tree in the _DOMDOCTYPENODE node
     *      according to attribute's element names.
     *
     *@@added V0.9.9 (2001-02-14) [umoeller]
     */

    typedef struct _CMATTRIBUTEDECLBASE
    {
        NODEBASE        NodeBase;         // has TREE* as first item in turn
                    // NodeBase.strName is element name

        TREE            *AttribDeclsTree;
                            // root of tree with CMATTRIBUTEDECL;

    } CMATTRIBUTEDECLBASE, *PCMATTRIBUTEDECLBASE;

    /*
     *@@ CMENTITYDECLNODE:
     *
     *      See @entity_declaration.
     *
     *@@added V0.9.9 (2001-02-14) [umoeller]
     */

    typedef struct _CMENTITYDECLNODE
    {
        NODEBASE          NodeBase;
    } CMENTITYDECLNODE, *PCMENTITYDECLNODE;

    /*
     *@@ CMNOTATIONDECLNODE:
     *
     *      See @notation_declaration.
     *
     *@@added V0.9.9 (2001-02-14) [umoeller]
     */

    typedef struct _CMNOTATIONDECLNODE
    {
        NODEBASE          NodeBase;
    } CMNOTATIONDECLNODE, *PCMNOTATIONDECLNODE;

    /* ******************************************************************
     *
     *   DOM APIs
     *
     ********************************************************************/

    /*
     *@@ XMLDOM:
     *      DOM instance returned by xmlCreateDOM.
     *
     *@@added V0.9.9 (2001-02-14) [umoeller]
     */

    typedef struct _XMLDOM
    {
        /*
         *  Public fields (should be read only)
         */

        PDOMDOCUMENTNODE pDocumentNode;

        PDOMDOCTYPENODE pDocTypeNode;
                        // != NULL only if the document has a DOCTYPE

        APIRET          arcDOM;         // validation errors etc.
        BOOL            fInvalid;       // TRUE after validation failed

        const char      *pcszErrorDescription;
        ULONG           ulErrorLine;
        ULONG           ulErrorColumn;
        PXSTRING        pxstrFailingNode; // element or attribute name

        /*
         *  Private fields (for xml* functions)
         */

        XML_Parser      pParser;
                            // expat parser instance

        LINKLIST        llElementStack;
                            // stack for maintaining the current items;
                            // these point to DOMSTACKITEMs (auto-free)

        PDOMNODE        pLastWasTextNode;

        PCMATTRIBUTEDECLBASE pAttListDeclCache;
                            // cache for attribute declarations according
                            // to attdecl element name
    } XMLDOM, *PXMLDOM;

    #define DF_PARSECOMMENTS        0x0001
    #define DF_PARSEDTD             0x0002

    APIRET xmlCreateDOM(ULONG flParserFlags,
                        PXMLDOM *ppDom);

    APIRET xmlParse(PXMLDOM pDom,
                    const char *pcszBuf,
                    ULONG cb,
                    BOOL fIsLast);

    APIRET xmlFreeDOM(PXMLDOM pDom);

    PCMELEMENTDECLNODE xmlFindElementDecl(PXMLDOM pDom,
                                          const XSTRING *pstrElementName);

    PCMATTRIBUTEDECLBASE xmlFindAttribDeclBase(PXMLDOM pDom,
                                                 const XSTRING *pstrElementName);

    PCMATTRIBUTEDECL xmlFindAttribDecl(PXMLDOM pDom,
                                       const XSTRING *pstrElementName,
                                       const XSTRING *pstrAttribName,
                                       PCMATTRIBUTEDECLBASE *ppAttribDeclBase);

#endif

#if __cplusplus
}
#endif

