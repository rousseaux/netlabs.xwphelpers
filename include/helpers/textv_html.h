
/*
 *@@sourcefile textv_html.h:
 *      header file for textv_html.c. See remarks there.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 */

/*      Copyright (C) 2000 Ulrich M”ller.
 *      This file is part of the "XWorkplace helpers" source package.
 *      This is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published
 *      by the Free Software Foundation, in version 2 as it comes in the
 *      "COPYING" file of the XWorkplace main distribution.
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *@@include #include <os2.h>
 *@@include #include "linklist.h"
 *@@include #include "textv_html.h"
 */

#if __cplusplus
extern "C" {
#endif

#ifndef TXV_HTML_HEADER_INCLUDED
    #define TXV_HTML_HEADER_INCLUDED

    /* ******************************************************************
     *
     *   HTML converter
     *
     ********************************************************************/

    /*
     *@@ XHTMLLINK:
     *      describes a link.
     *
     *@@added V0.9.3 (2000-05-19) [umoeller]
     */

    typedef struct _XHTMLLINK
    {
        USHORT      usLinkIndex;        // >= 1; this is stored in the XTextView control
        PSZ         pszTargetFile;      // target file (HREF="...") without anchors;
                                        // this is NULL if the target is an anchor only
                                        // (HREF="#anchor")
        PSZ         pszTargetAnchor;    // anchor in target file; this is NULL if the
                                        // target has no anchor
    } XHTMLLINK, *PXHTMLLINK;

    #ifdef LINKLIST_HEADER_INCLUDED
        /*
         *@@ XHTMLDATA:
         *      for storing output from txvConvertFromHTML.
         *
         *@@added V0.9.3 (2000-05-19) [umoeller]
         */

        typedef struct _XHTMLDATA
        {
            PSZ         pszTitle;           // contents of TITLE tag (must be freed)
            LINKLIST    llLinks;            // list of XHTMLLINK structures; empty if none
                                            // (auto-free mode; use lstClear)
        } XHTMLDATA, *PXHTMLDATA;
    #endif

    BOOL txvConvertFromHTML(char **ppszText,
                            PVOID pxhtml,
                            PULONG pulProgress,
                            PBOOL pfCancel);

#endif

#if __cplusplus
}
#endif

