/*
 *@@sourcefile cnr.h.h:
 *      private header file for cctl_cnr.c. See remarks there.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 */

/*      Copyright (C) 2003 Ulrich M”ller.
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
 */

#if __cplusplus
extern "C" {
#endif

#ifndef PRIVATE_CNR_HEADER_INCLUDED
    #define PRIVATE_CNR_HEADER_INCLUDED

    /* ******************************************************************
     *
     *   Global stuff
     *
     ********************************************************************/

    #define WC_CCTL_CNR_DETAILS     "ComctlCnrDtls"

    extern const SYSCOLORSET G_scsCnr;

    #define COLUMN_PADDING_X        7
    #define COLUMN_PADDING_Y        3
                // padding to apply between column border and column data

    #define DEFAULT_BORDER_WIDTH    1
                // default width of lines used for details and tree view lines

    /*
     *@@ DETAILCOLUMN:
     *      private wrapper data around FIELDINFO.
     */

    typedef struct _DETAILCOLUMN
    {
        const FIELDINFO *pfi;           // ptr to fieldinfo for this column

        LONG        cxTotal;            // current width of the column (excluding padding);
                                        // -- if pfi->cxWidth == 0, this is the computed auto-size;
                                        // -- otherwise this is a copy of the pfi->cxWidth value

        LONG        cxWidestRecord;     // width of widest record data in this column (excluding padding)

        SIZEL       szlTitleData;       // dimensions of title data (excluding padding)

        LONG        xLeft;              // position of column in cnr workspace coordinates

    } DETAILCOLUMN, *PDETAILCOLUMN;

    /*
     *@@ RECORDLISTITEM:
     *      private wrapper data around an app's RECORDCORE
     *      that is currently inserted somewhere.
     *
     *      We create both a list node and a tree node for
     *      each record that is currently inserted.
     */

    typedef struct _RECORDLISTITEM
    {
        const RECORDCORE
                    *precc,             // ptr to app's RECORDCORE buffer
                    *preccParent;       // parent of record or NULL if root

        SIZEL       szlContent;         // space that records needs in current cnr view
                                        // (excluding padding)
        SIZEL       szlBox;             // space that records needs in current cnr view
                                        // (including padding)
        LONG        yOfs;               // y offset of the top of this record's rectangle
                                        // from the top of the cnr workspace. Positive
                                        // values mean the record is further down.
                                        // In Details view, the topmost record has an
                                        // offset of 0.

    } RECORDLISTITEM, *PRECORDLISTITEM;

    /*
     *@@ RECORDTREEITEM:
     *      second private wrapper data around a
     *      RECORDCORE that is currently inserted somewhere.
     *
     *      We create both a list node and a tree node for
     *      each record that is currently inserted.
     */

    typedef struct _RECORDTREEITEM
    {
        TREE        Tree;               // ulKey is app's PRECORDCORE
        PLISTNODE   pListNode;          // points to the LISTNODE corresponding to record
    } RECORDTREEITEM, *PRECORDTREEITEM;

    /*
     *@@ CNRDATA:
     *
     */

    typedef struct _CNRDATA
    {
        DEFWINDATA  dwdMain,
                    dwdContent;

        BOOL        fMiniRecords;

        CNRINFO     CnrInfo;

        LINKLIST    llAllocatedFIs;     // contains PFIELDINFO's that were allocated, auto-free
        LINKLIST    llColumns;          // contains PDETAILCOLUMN's with FIELDINFO's that are
                                        // currently inserted, auto-free

        LINKLIST    llAllocatedRecs;    // contains PRECORDCORE's that were allocated, auto-free
        LINKLIST    llRootRecords;      // contains PRECORDLISTITEM's with RECORDCORE's that are
                                        // currently inserted at root level (i.e. have no parent
                                        // record), auto-free
        TREE        *RecordsTree;       // tree of _all_ currently inserted RECORDLISTITEM's;
                                        // we use CnrInfo.cRecords for the tree count

        // paint data
        FONTMETRICS fm;
        COUNTRYSETTINGS2 cs;

        HWND        hwndDetails;        // child of main cnr window

        HWND        hwndVScroll,        // child of hwndDetails
                    hwndHScroll;

        LONG        cxScrollBar,
                    cyScrollBar;

        LONG        cyColTitlesContent, // if CnrInfo.flWindowAttr & CA_DETAILSVIEWTITLES, height of
                                        // column titles area (excluding padding and horz. separator)
                    cyColTitlesBox;     // ... and including those

        SIZEL       szlWorkarea;        // dimensions of cnr workarea (area that window scrolls over)

        POINTL      ptlScrollOfs;       // scroll ofs: x = 0 means topmost, y = 0 means leftmost

    } CNRDATA, *PCNRDATA;

    // bits for WM_SEM2
    #define DDFL_INVALIDATECOLUMNS     0x0001
                // no. of columns changed

    #define DDFL_INCALIDATERECORDS      0x0002

    #define DDFL_WINDOWSIZECHANGED      0x0004

    #define DDFL_WORKAREACHANGED        0x0008

    #define DDFL_ALL                    0xFFFF

    /* ******************************************************************
     *
     *   Cnr details view stuff
     *
     ********************************************************************/

    VOID cnrDrawString(HPS hps,
                       PCSZ pcsz,
                       PRECTL prcl,
                       ULONG fl,
                       PFONTMETRICS pfm);

    VOID cnrPresParamChanged(HWND hwnd,
                             ULONG ulpp);

    VOID cdtlRecalcDetails(PCNRDATA pData,
                           HPS hps,
                           PULONG pfl);

    MRESULT EXPENTRY fnwpCnrDetails(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);

#endif

#if __cplusplus
}
#endif

