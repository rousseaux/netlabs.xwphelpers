
/*
 *@@sourcefile dialog.c:
 *      contains PM helper functions to create and
 *      auto-format dialogs from control arrays in memory.
 *
 *      See dlghCreateDlg for details.
 *
 *      Usage: All PM programs.
 *
 *      Function prefixes (new with V0.81):
 *      --  dlg*   Dialog functions
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@added V0.9.9 (2001-04-01) [umoeller]
 *@@header "helpers\dialog.h"
 */

/*
 *      Copyright (C) 2001 Ulrich M”ller.
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

#define INCL_WINWINDOWMGR
#define INCL_WINFRAMEMGR
#define INCL_WINDIALOGS
#define INCL_WINBUTTONS
#define INCL_WINSTATICS
#define INCL_WINSYS

#define INCL_GPIPRIMITIVES
#define INCL_GPIBITMAPS
#define INCL_GPILCIDS
#include <os2.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "setup.h"                      // code generation and debugging options

#include "helpers\comctl.h"
#include "helpers\dialog.h"
#include "helpers\gpih.h"
#include "helpers\linklist.h"
#include "helpers\standards.h"
#include "helpers\stringh.h"
#include "helpers\winh.h"

/*
 *@@category: Helpers\PM helpers\Dialog templates
 */

/* ******************************************************************
 *
 *   Private declarations
 *
 ********************************************************************/

/*
 *@@ DLGPRIVATE:
 *      private data to the dlg manager, allocated
 *      by dlghCreateDlg. This is what is really
 *      used, even though the prototype only
 *      declares DIALOGDATA.
 */

typedef struct _DLGPRIVATE
{
    // public data
    HWND        hwndDlg;

    // definition data (private)
    LINKLIST    llTables;

    HWND        hwndFirstFocus;

    POINTL      ptlTotalOfs;

    LINKLIST    llControls;     // linked list of all PCOLUMNDEF structs,
                                 // in the order in which windows were
                                 // created

    const char  *pcszControlsFont;  // from dlghCreateDlg

    HPS         hps;
    const char  *pcszFontLast;
    LONG        lcidLast;
} DLGPRIVATE, *PDLGPRIVATE;

typedef struct _COLUMNDEF *PCOLUMNDEF;
typedef struct _ROWDEF *PROWDEF;
typedef struct _TABLEDEF *PTABLEDEF;

/*
 *@@ CONTROLPOS:
 *      control position. We don't want to use SWP.
 */

typedef struct _CONTROLPOS
{
    LONG        x,
                y,
                cx,
                cy;
} CONTROLPOS, *PCONTROLPOS;

/*
 *@@ COLUMNDEF:
 *      representation of a table column.
 *      This is stored in a linked list in ROWDEF.
 *
 *      A table column represents either a PM control
 *      window or another table, which may therefore
 *      be nested.
 */

typedef struct _COLUMNDEF
{
    PROWDEF     pOwningRow;         // row whose linked list this column belongs to

    BOOL        fIsNestedTable;     // if TRUE, pvDefinition points to a nested TABLEDEF;
                                    // if FALSE, pvDefinition points to a CONTROLDEF as
                                    // specified by the user

    PVOID       pvDefinition;       // either a PTABLEDEF or a PCONTROLDEF

    CONTROLPOS  cpControl,          // real pos and size of control
                cpColumn;           // pos and size of column; can be wider, spacings applied

    HWND        hwndControl;        // created control; NULLHANDLE for tables always

} COLUMNDEF;

/*
 *@@ ROWDEF:
 *
 */

typedef struct _ROWDEF
{
    PTABLEDEF   pOwningTable;       // table whose linked list this row belongs to

    LINKLIST    llColumns;          // contains COLUMNDEF structs, no auto-free

    ULONG       flRowFormat;        // one of:
                                    // -- ROW_VALIGN_BOTTOM           0x0000
                                    // -- ROW_VALIGN_CENTER           0x0001
                                    // -- ROW_VALIGN_TOP              0x0002

    CONTROLPOS  cpRow;

} ROWDEF;

/*
 *@@ TABLEDEF:
 *
 */

typedef struct _TABLEDEF
{
    LINKLIST    llRows;             // contains ROWDEF structs, no auto-free

    PCONTROLDEF pCtlDef;            // if != NULL, we create a PM control around the table

    CONTROLPOS  cpTable;

} TABLEDEF;

/*
 *@@ PROCESSMODE:
 *
 */

typedef enum _PROCESSMODE
{
    PROCESS_CALC_SIZES,             // step 1
    PROCESS_CALC_POSITIONS,         // step 3
    PROCESS_CREATE_CONTROLS         // step 4
} PROCESSMODE;

/* ******************************************************************
 *
 *   Worker routines
 *
 ********************************************************************/

#define PM_GROUP_SPACING_X          20
#define PM_GROUP_SPACING_TOP        30

VOID ProcessTable(PTABLEDEF pTableDef,
                  const CONTROLPOS *pcpTable,
                  PROCESSMODE ProcessMode,
                  PDLGPRIVATE pDlgData);

/*
 *@@ CalcAutoSizeText:
 *
 */

VOID CalcAutoSizeText(PCONTROLDEF pControlDef,
                      BOOL fMultiLine,          // in: if TRUE, multiple lines
                      PSIZEL pszlAuto,          // out: computed size
                      PDLGPRIVATE pDlgData)
{
    BOOL        fFind = TRUE;
    RECTL       rclText;

    const char *pcszFontThis = pControlDef->pcszFont;
                    // can be NULL,
                    // or CTL_COMMON_FONT

    if (pcszFontThis == CTL_COMMON_FONT)
        pcszFontThis = pDlgData->pcszControlsFont;

    if (!pDlgData->hps)
    {
        // first call: get a PS
        pDlgData->hps = WinGetPS(pDlgData->hwndDlg);
        fFind = TRUE;
    }
    else
        if (strhcmp(pcszFontThis, pDlgData->pcszFontLast))
            fFind = TRUE;

    if (fFind)
    {
        FONTMETRICS fm;
        LONG lcid;
        LONG lPointSize = 0;
        if (pDlgData->lcidLast)
        {
            // delete old font
            GpiSetCharSet(pDlgData->hps, LCID_DEFAULT);
            GpiDeleteSetId(pDlgData->hps, pDlgData->lcidLast);
        }

        // create new font
        pDlgData->lcidLast = gpihFindPresFont(NULLHANDLE,        // no window yet
                                              FALSE,
                                              pDlgData->hps,
                                              pcszFontThis,
                                              &fm,
                                              &lPointSize);
        GpiSetCharSet(pDlgData->hps, pDlgData->lcidLast);
        if (fm.fsDefn & FM_DEFN_OUTLINE)
            gpihSetPointSize(pDlgData->hps, lPointSize);

        pszlAuto->cy = fm.lMaxBaselineExt + fm.lExternalLeading;
    }

    // ok, we FINALLY have a font now...
    // get the control string and see how much space it needs
    if (pControlDef->pcszText)
    {
        // do we have multiple lines?
        if (fMultiLine)
        {
            RECTL rcl = {0, 0, 1000, 1000};
            winhDrawFormattedText(pDlgData->hps,
                                  &rcl,
                                  pControlDef->pcszText,
                                  DT_LEFT | DT_TOP | DT_WORDBREAK | DT_QUERYEXTENT);
            pszlAuto->cx = rcl.xRight - rcl.xLeft;
            pszlAuto->cy = rcl.yTop - rcl.yBottom;
        }
        else
        {
            POINTL aptl[TXTBOX_COUNT];
            GpiQueryTextBox(pDlgData->hps,
                            strlen(pControlDef->pcszText),
                            (PCH)pControlDef->pcszText,
                            TXTBOX_COUNT,
                            aptl);
            pszlAuto->cx = aptl[TXTBOX_TOPRIGHT].x - aptl[TXTBOX_BOTTOMLEFT].x;
        }
    }
}

/*
 *@@ CalcAutoSize:
 *
 */

VOID CalcAutoSize(PCONTROLDEF pControlDef,
                  PSIZEL pszlAuto,          // out: computed size
                  PDLGPRIVATE pDlgData)
{
    // dumb defaults
    pszlAuto->cx = 100;
    pszlAuto->cy = 30;

    switch ((ULONG)pControlDef->pcszClass)
    {
        case 0xffff0003L: // WC_BUTTON:
            CalcAutoSizeText(pControlDef,
                             FALSE,         // no multiline
                             pszlAuto,
                             pDlgData);
            if (pControlDef->flStyle & (  BS_AUTOCHECKBOX
                                        | BS_AUTORADIOBUTTON
                                        | BS_AUTO3STATE
                                        | BS_3STATE
                                        | BS_CHECKBOX
                                        | BS_RADIOBUTTON))
                pszlAuto->cx += 20;     // @@@
            else if (pControlDef->flStyle & BS_BITMAP)
                ;
            else if (pControlDef->flStyle & (BS_ICON | BS_MINIICON))
                ;
            // we can't test for BS_PUSHBUTTON because that's 0x0000
            else if (!(pControlDef->flStyle & BS_USERBUTTON))
            {
                pszlAuto->cx += (2 * WinQuerySysValue(HWND_DESKTOP, SV_CXBORDER) + 15);
                pszlAuto->cy += (2 * WinQuerySysValue(HWND_DESKTOP, SV_CYBORDER) + 15);
            }
        break;

        case 0xffff0005L: // WC_STATIC:
            if (pControlDef->flStyle & SS_TEXT)
                CalcAutoSizeText(pControlDef,
                                 ((pControlDef->flStyle & DT_WORDBREAK) != 0),
                                 pszlAuto,
                                 pDlgData);
            else if (pControlDef->flStyle & SS_BITMAP)
            {
                HBITMAP hbm = (HBITMAP)pControlDef->pcszText;
                if (hbm)
                {
                    BITMAPINFOHEADER2 bmih2;
                    ZERO(&bmih2);
                    bmih2.cbFix = sizeof(bmih2);
                    if (GpiQueryBitmapInfoHeader(hbm,
                                                 &bmih2))
                    {
                        pszlAuto->cx = bmih2.cx;
                        pszlAuto->cy = bmih2.cy;
                    }
                }
            }
        break;
    }
}

/*
 *@@ ProcessColumn:
 *      processes a column, which per definition is either
 *      a control or a nested subtable.
 *
 *      A column is part of a row, which in turn is part
 *      of a table. There can be several columns in a row,
 *      and several rows in a table.
 *
 *      Since tables may be specified as columns, it is
 *      possible to produce complex dialog layouts by
 *      nesting tables.
 *
 *      This does the following:
 *
 *      -- PROCESS_CALC_SIZES: size is taken from control def,
 *         or for tables, this produces a recursive ProcessTable
 *         call.
 *         Preconditions: none.
 *
 *      -- PROCESS_CALC_POSITIONS: position of each column
 *         is taken from *plX, which is increased by the
 *         column width by this call.
 *
 *         Preconditions: Owning row must already have its
 *         y position properly set, or we can't compute
 *         ours. Besides, plX must point to the current X
 *         in the row and will be incremented by the columns
 *         size here.
 *
 *      -- PROCESS_CREATE_CONTROLS: well, creates the controls.
 *
 *         For tables, this recurses again. If the table has
 *         a string assigned, this also produces a group box
 *         after the recursion.
 *
 */

VOID ProcessColumn(PCOLUMNDEF pColumnDef,
                   PROWDEF pOwningRow,
                   PROCESSMODE ProcessMode,
                   PLONG plX,                   // in/out: PROCESS_CALC_POSITIONS only
                   PDLGPRIVATE pDlgData)
{
    pColumnDef->pOwningRow = pOwningRow;

    switch (ProcessMode)
    {
        /*
         * PROCESS_CALC_SIZES:
         *      step 1.
         */

        case PROCESS_CALC_SIZES:
        {
            ULONG       ulXSpacing = 0,
                        ulYSpacing = 0;
            if (pColumnDef->fIsNestedTable)
            {
                // nested table: recurse!!
                PTABLEDEF pTableDef = (PTABLEDEF)pColumnDef->pvDefinition;
                ProcessTable(pTableDef,
                             NULL,
                             ProcessMode,
                             pDlgData);

                // store the size of the sub-table
                pColumnDef->cpControl.cx = pTableDef->cpTable.cx;
                pColumnDef->cpControl.cy = pTableDef->cpTable.cy;

                // should we create a PM control around the table?
                if (pTableDef->pCtlDef)
                {
                    // yes: make this wider
                    ulXSpacing = (2 * PM_GROUP_SPACING_X);
                    ulYSpacing = (PM_GROUP_SPACING_X + PM_GROUP_SPACING_TOP);
                }
            }
            else
            {
                // no nested table, but control:
                PCONTROLDEF pControlDef = (PCONTROLDEF)pColumnDef->pvDefinition;
                PSIZEL      pszl = &pControlDef->szlControlProposed;
                SIZEL       szlAuto;

                if (    (pszl->cx == -1)
                     || (pszl->cy == -1)
                   )
                {
                    CalcAutoSize(pControlDef,
                                 &szlAuto,
                                 pDlgData);
                }

                if (pszl->cx == -1)
                    pColumnDef->cpControl.cx = szlAuto.cx;
                else
                    pColumnDef->cpControl.cx = pszl->cx;

                if (pszl->cy == -1)
                    pColumnDef->cpControl.cy = szlAuto.cy;
                else
                    pColumnDef->cpControl.cy = pszl->cy;

                // @@@hack sizes

                ulXSpacing = ulYSpacing = (2 * pControlDef->ulSpacing);
            }

            pColumnDef->cpColumn.cx =   pColumnDef->cpControl.cx
                                       + ulXSpacing;
            pColumnDef->cpColumn.cy =   pColumnDef->cpControl.cy
                                       + ulYSpacing;
        break; }

        /*
         * PROCESS_CALC_POSITIONS:
         *      step 2.
         */

        case PROCESS_CALC_POSITIONS:
        {
            // calculate column position: this includes spacing
            ULONG ulSpacing = 0;

            // column position = *plX on ProcessRow stack
            pColumnDef->cpColumn.x = *plX;
            pColumnDef->cpColumn.y = pOwningRow->cpRow.y;

            // check vertical alignment of row;
            // we might need to increase column y
            switch (pOwningRow->flRowFormat & ROW_VALIGN_MASK)
            {
                // case ROW_VALIGN_BOTTOM:      // do nothing

                case ROW_VALIGN_CENTER:
                    if (pColumnDef->cpColumn.cy < pOwningRow->cpRow.cy)
                        pColumnDef->cpColumn.y
                            += (   (pOwningRow->cpRow.cy - pColumnDef->cpColumn.cy)
                                 / 2);
                break;

                case ROW_VALIGN_TOP:
                    if (pColumnDef->cpColumn.cy < pOwningRow->cpRow.cy)
                        pColumnDef->cpColumn.y
                            += (pOwningRow->cpRow.cy - pColumnDef->cpColumn.cy);
                break;
            }

            if (pColumnDef->fIsNestedTable)
            {
                PTABLEDEF pTableDef = (PTABLEDEF)pColumnDef->pvDefinition;
                // should we create a PM control around the table?
                if (pTableDef->pCtlDef)
                    // yes:
                    ulSpacing = PM_GROUP_SPACING_X;
            }
            else
            {
                // no nested table, but control:
                PCONTROLDEF pControlDef = (PCONTROLDEF)pColumnDef->pvDefinition;
                ulSpacing = pControlDef->ulSpacing;
            }

            // increase plX by column width
            *plX += pColumnDef->cpColumn.cx;

            // calculate CONTROL pos from COLUMN pos by applying spacing
            pColumnDef->cpControl.x =   pColumnDef->cpColumn.x
                                      + ulSpacing;
            pColumnDef->cpControl.y =   pColumnDef->cpColumn.y
                                      + ulSpacing;

            if (pColumnDef->fIsNestedTable)
            {
                // nested table:
                PTABLEDEF pTableDef = (PTABLEDEF)pColumnDef->pvDefinition;

                // recurse!! to create windows for the sub-table
                ProcessTable(pTableDef,
                             &pColumnDef->cpControl,   // start pos for new table
                             ProcessMode,
                             pDlgData);
            }
        break; }

        /*
         * PROCESS_CREATE_CONTROLS:
         *      step 3.
         */

        case PROCESS_CREATE_CONTROLS:
        {
            PCONTROLPOS pcp = NULL;
            PCONTROLDEF pControlDef = NULL;
            const char  *pcszTitle = NULL;
            ULONG       flStyle = 0;
            LHANDLE     lHandleSet = NULLHANDLE;

            if (pColumnDef->fIsNestedTable)
            {
                // nested table:
                PTABLEDEF pTableDef = (PTABLEDEF)pColumnDef->pvDefinition;

                // recurse!!
                ProcessTable(pTableDef,
                             NULL,
                             ProcessMode,
                             pDlgData);

                // should we create a PM control around the table?
                // (do this AFTER the other controls from recursing,
                // otherwise the stupid container doesn't show up)
                if (pTableDef->pCtlDef)
                {
                    // yes:
                    pcp  = &pColumnDef->cpColumn;  // !! not control
                    pControlDef = pTableDef->pCtlDef;
                    pcszTitle = pControlDef->pcszText;
                    flStyle = pControlDef->flStyle;
                }
            }
            else
            {
                // no nested table, but control:
                pControlDef = (PCONTROLDEF)pColumnDef->pvDefinition;
                pcp = &pColumnDef->cpControl;
                pcszTitle = pControlDef->pcszText;
                flStyle = pControlDef->flStyle;

                // change the title if this is a static with SS_BITMAP;
                // we have used a HBITMAP in there!
                if (    ((ULONG)pControlDef->pcszClass == 0xffff0005L) // WC_STATIC:
                     && (flStyle & SS_BITMAP)
                   )
                {
                    // change style flag to not use SS_BITMAP;
                    // control creation fails otherwise (stupid, stupid PM)
                    flStyle = ((flStyle & ~SS_BITMAP) | SS_FGNDFRAME);
                    pcszTitle = "";
                    lHandleSet = (LHANDLE)pControlDef->pcszText;
                }
            }

            if (pcp && pControlDef)
            {
                // create something:
                PPRESPARAMS ppp = NULL;

                const char  *pcszFont = pControlDef->pcszFont;
                                // can be NULL, or CTL_COMMON_FONT
                if (pcszFont == CTL_COMMON_FONT)
                    pcszFont = pDlgData->pcszControlsFont;

                if (pcszFont)
                    winhStorePresParam(&ppp,
                                       PP_FONTNAMESIZE,
                                       strlen(pcszFont),
                                       (PVOID)pcszFont);

                pColumnDef->hwndControl
                    = WinCreateWindow(pDlgData->hwndDlg,   // parent
                                      (PSZ)pControlDef->pcszClass,
                                      (pcszTitle)   // hacked
                                            ? (PSZ)pcszTitle
                                            : "",
                                      flStyle,      // hacked
                                      pcp->x + pDlgData->ptlTotalOfs.x,
                                      pcp->y + pDlgData->ptlTotalOfs.y,
                                      pcp->cx,
                                      pcp->cy,
                                      pDlgData->hwndDlg,   // owner
                                      HWND_BOTTOM,
                                      pControlDef->usID,
                                      NULL,             // control data @@@
                                      ppp);

                if (pColumnDef->hwndControl && lHandleSet)
                {
                    // subclass the damn static
                    ctlPrepareStretchedBitmap(pColumnDef->hwndControl,
                                              TRUE);

                    WinSendMsg(pColumnDef->hwndControl,
                               SM_SETHANDLE,
                               (MPARAM)lHandleSet,
                               0);
                }

                if (ppp)
                    free(ppp);

                if (pColumnDef->hwndControl)
                {
                    lstAppendItem(&pDlgData->llControls,
                                  pColumnDef);

                    // if this is the first control with WS_TABSTOP,
                    // we give it the focus later
                    if (    (flStyle & WS_TABSTOP)
                         && (!pDlgData->hwndFirstFocus)
                       )
                        pDlgData->hwndFirstFocus = pColumnDef->hwndControl;
                }
            }
        break; }
    }

}

/*
 *@@ ProcessRow:
 *
 */

VOID ProcessRow(PROWDEF pRowDef,
                PTABLEDEF pOwningTable,
                PROCESSMODE ProcessMode,
                PLONG plY,
                PDLGPRIVATE pDlgData)
{
    ULONG   ul;
    LONG    lX;
    PLISTNODE pNode;

    pRowDef->pOwningTable = pOwningTable;

    if (ProcessMode == PROCESS_CALC_SIZES)
    {
        pRowDef->cpRow.cx = 0;
        pRowDef->cpRow.cy = 0;
    }
    else if (ProcessMode == PROCESS_CALC_POSITIONS)
    {
        // set up x and y so that the columns can
        // base on that
        pRowDef->cpRow.x = pOwningTable->cpTable.x;
        // decrease y by row height
        *plY -= pRowDef->cpRow.cy;
        // and use that for our bottom position
        pRowDef->cpRow.y = *plY;

        // set lX to left of row; used by column calls below
        lX = pRowDef->cpRow.x;
    }

    FOR_ALL_NODES(&pRowDef->llColumns, pNode)
    {
        PCOLUMNDEF  pColumnDefThis = (PCOLUMNDEF)pNode->pItemData;

        ProcessColumn(pColumnDefThis, pRowDef, ProcessMode, &lX, pDlgData);

        if (ProcessMode == PROCESS_CALC_SIZES)
        {
            // row width = sum of all columns
            pRowDef->cpRow.cx += pColumnDefThis->cpColumn.cx;

            // row height = maximum height of a column
            if (pRowDef->cpRow.cy < pColumnDefThis->cpColumn.cy)
                pRowDef->cpRow.cy = pColumnDefThis->cpColumn.cy;
        }
    }
}

/*
 *@@ ProcessTable:
 *
 *      This routine is a bit sick because it can be
 *      called recursively if a nested table is found
 *      in a COLUMNDEF.
 *
 *      With PROCESS_CALC_POSITIONS, pptl must specify
 *      the lower left corner of the table. For the
 *      root call, this will be {0, 0}; for nested calls,
 *      this must be the lower left corner of the column
 *      to which the nested table belongs.
 *
 */

VOID ProcessTable(PTABLEDEF pTableDef,
                  const CONTROLPOS *pcpTable,       // in: table position with PROCESS_CALC_POSITIONS
                  PROCESSMODE ProcessMode,
                  PDLGPRIVATE pDlgData)
{
    ULONG   ul;
    LONG    lY;
    PLISTNODE pNode;

    if (ProcessMode == PROCESS_CALC_SIZES)
    {
        pTableDef->cpTable.cx = 0;
        pTableDef->cpTable.cy = 0;
    }
    else if (ProcessMode == PROCESS_CALC_POSITIONS)
    {
        pTableDef->cpTable.x = pcpTable->x;
        pTableDef->cpTable.y = pcpTable->y;

        // start the rows on top
        lY = pcpTable->y + pTableDef->cpTable.cy;
    }

    FOR_ALL_NODES(&pTableDef->llRows, pNode)
    {
        PROWDEF pRowDefThis = (PROWDEF)pNode->pItemData;

        ProcessRow(pRowDefThis, pTableDef, ProcessMode, &lY, pDlgData);

        if (ProcessMode == PROCESS_CALC_SIZES)
        {
            // table width = maximum width of a row
            if (pTableDef->cpTable.cx < pRowDefThis->cpRow.cx)
                pTableDef->cpTable.cx = pRowDefThis->cpRow.cx;

            // table height = sum of all rows
            pTableDef->cpTable.cy += pRowDefThis->cpRow.cy;
        }
    }
}

/*
 *@@ ProcessAll:
 *
 *      -- PROCESS_CALC_SIZES: calculates the sizes
 *         of all tables, rows, columns, and controls.
 *
 *      -- PROCESS_CALC_POSITIONS: calculates the positions
 *         based on the sizes calculated before.
 *
 *      -- PROCESS_CREATE_CONTROLS: creates the controls with the
 *         positions and sizes calculated before.
 *
 */

VOID ProcessAll(PDLGPRIVATE pDlgData,
                PSIZEL pszlClient,
                PROCESSMODE ProcessMode)
{
    ULONG ul;
    PLISTNODE pNode;
    CONTROLPOS cpTable;
    ZERO(&cpTable);

    switch (ProcessMode)
    {
        case PROCESS_CALC_SIZES:
            pszlClient->cx = 0;
            pszlClient->cy = 0;
        break;

        case PROCESS_CALC_POSITIONS:
            // start with the table on top
            cpTable.y = pszlClient->cy;
        break;
    }

    FOR_ALL_NODES(&pDlgData->llTables, pNode)
    {
        PTABLEDEF pTableDefThis = (PTABLEDEF)pNode->pItemData;

        if (ProcessMode == PROCESS_CALC_POSITIONS)
        {
            cpTable.x = 0;
            cpTable.y -= pTableDefThis->cpTable.cy;
        }

        ProcessTable(pTableDefThis,
                     &cpTable,      // start pos
                     ProcessMode,
                     pDlgData);

        if (ProcessMode == PROCESS_CALC_SIZES)
        {
            pszlClient->cx += pTableDefThis->cpTable.cx;
            pszlClient->cy += pTableDefThis->cpTable.cy;
        }
    }
}

/*
 *@@ CreateColumn:
 *
 */

APIRET CreateColumn(PROWDEF pCurrentRow,
                    BOOL fIsNestedTable,
                    PVOID pvDefinition,       // in: either PTABLEDEF or PCONTROLDEF
                    PCOLUMNDEF *ppColumnDef)    // out: new COLUMNDEF
{
    APIRET arc = NO_ERROR;

    if (!pCurrentRow)
        arc = DLGERR_CONTROL_BEFORE_ROW;
    else
    {
        // append the control def
        if (!pvDefinition)
            arc = DLGERR_NULL_CTL_DEF;
        else
        {
            // create column and store ctl def
            PCOLUMNDEF pColumnDef = NEW(COLUMNDEF);
            if (!pColumnDef)
                arc = ERROR_NOT_ENOUGH_MEMORY;
            else
            {
                memset(pColumnDef, 0, sizeof(COLUMNDEF));
                pColumnDef->pOwningRow = pCurrentRow;
                pColumnDef->fIsNestedTable = fIsNestedTable;
                pColumnDef->pvDefinition = pvDefinition;

                *ppColumnDef = pColumnDef;
            }
        }
    }

    return (arc);
}

/* ******************************************************************
 *
 *   Public APIs
 *
 ********************************************************************/

/*
 *@@ STACKITEM:
 *
 */

typedef struct _STACKITEM
{
    PTABLEDEF       pLastTable;
    PROWDEF         pLastRow;

} STACKITEM, *PSTACKITEM;

/*
 *@@ dlghCreateDlg:
 *      replacement for WinCreateDlg for creating a dialog
 *      from a settings array in memory, which is formatted
 *      automatically.
 *
 *      This does NOT use regular dialog templates from
 *      module resources. Instead, you pass in an array
 *      of _DLGHITEM structures, which define the controls
 *      and how they are to be formatted.
 *
 *      A regular standard dialog would use something like
 *
 +          FCF_TITLEBAR | FCF_SYSMENU | FCF_DLGBORDER | FCF_NOBYTEALIGN
 *
 *      for flCreateFlags. To make the dlg sizeable, specify
 *      FCF_SIZEBORDER instead of FCF_DLGBORDER.
 *
 *      <B>Usage:</B>
 *
 *      Like WinLoadDlg, this creates a standard WC_FRAME and
 *      subclasses it with fnwpMyDlgProc. It then sends WM_INITDLG
 *      to the dialog with pCreateParams in mp2. You can use
 *      WinProcessDlg as usual. In your dlg proc, use WinDefDlgProc
 *      as usual.
 *
 *      There is NO run-time overhead after creation; after this
 *      function returns, the dialog is a standard dialog as if
 *      loaded from WinLoadDlg.
 *
 *      The array of _DLGHITEM structures defines how the
 *      dialog is set up. All this is ONLY used by this function
 *      and NOT needed after the dialog has been created.
 *
 *      In _DLGHITEM, the "Type" field determines what this
 *      structure defines. A number of handy macros have been
 *      defined to make this easier and to provide type-checking
 *      at compile time.
 *
 *      The macros are:
 *
 *      --  START_TABLE starts a new table. The tables may nest,
 *          but must each be properly terminated with END_TABLE.
 *
 *      --  START_ROW(fl) starts a new row in a table.
 *
 *          fl specifies formatting flags for the row. This
 *          can be one of ROW_VALIGN_BOTTOM, ROW_VALIGN_CENTER,
 *          ROW_VALIGN_TOP and affects all items in the control.
 *
 *      --  START_GROUP_TABLE(pDef) starts a group. This
 *          behaves exacly like START_TABLE, but in addition,
 *          it produces a control around the table. Useful
 *          for group boxes. pDef must point to a _CONTROLDEF,
 *          whose size parameter is ignored.
 *
 *          This must also be terminated with END_TABLE.
 *
 *      --  CONTROL_DEF(pDef) defines a control in a table row.
 *          pDef must point to a _CONTROLDEF structure.
 *
 *          The main difference (and advantage) of this
 *          function is that there is NO information in
 *          _CONTROLDEF about a control's _position_.
 *          Instead, the structure only contains the _size_
 *          of the control. All positions are computed by
 *          this function, depending on the position of the
 *          control and its nesting within the various tables.
 *
 *          If you specify SZL_AUTOSIZE, the size of the
 *          control is even computed automatically. Presently,
 *          this only works for statics with SS_TEXT and
 *          SS_BITMAP.
 *
 *          Unless separated with TYPE_START_NEW_ROW items,
 *          subsequent control items will be considered
 *          to be in the same row (== positioned next to
 *          each other).
 *
 *      There are a few rules, whose violation will produce
 *      an error:
 *
 *      -- The entire array must be enclosed in a table
 *         (the "root" table).
 *
 *      -- After START_TABLE or START_GROUP_TABLE, there must
 *         always be a START_ROW first.
 *
 *      To create a dialog, set up arrays like the following:
 *
 +          // control definitions referenced by DlgTemplate:
 +          CONTROLDEF
 +      (1)             GroupDef = {
 +                                  WC_STATIC, "", ....,
 +                                  { 0, 0 },       // size, ignored for groups
 +                                  5               // spacing
 +                               },
 +      (2)             CnrDef = {
 +                                  WC_CONTAINER, "", ....,
 +                                  { 50, 50 },     // size
 +                                  5               // spacing
 +                               },
 +      (3)             Static = {
 +                                  WC_STATIC, "Static below cnr", ...,
 +                                  { SZL_AUTOSIZE, SZL_AUTOSIZE },     // size
 +                                  5               // spacing
 +                               },
 +      (4)             OKButton = {
 +                                  WC_STATIC, "~OK", ...,
 +                                  { 100, 30 },    // size
 +                                  5               // spacing
 +                               },
 +      (5)             CancelButton = {
 +                                  WC_STATIC, "~Cancel", ...,
 +                                  { 100, 30 },    // size
 +                                  5               // spacing
 +                               };
 +
 +          DLGHITEM DlgTemplate[] =
 +              {
 +                  START_TABLE,            // root table, must exist
 +                      START_ROW(0),       // row 1 in the root table
 +                          // create group on top
 +      (1)                 START_GROUP_TABLE(&Group),
 +                              START_ROW(0),
 +      (2)                         CONTROL_DEF(&CnrDef),
 +                              START_ROW(0),
 +      (3)                         CONTROL_DEF(&Static),
 +                          END_TABLE,      // end of group
 +                      START_ROW(0),       // row 2 in the root table
 +                          // two buttons next to each other
 +      (4)                 CONTROL_DEF(&OKButton),
 +      (5)                 CONTROL_DEF(&CancelButton),
 +                  END_TABLE
 +              }
 *
 *      This will produce a dlg like this:
 *
 +          ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»
 +          º                                   º
 +          º ÚÄ Group (1) ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿  º
 +          º ³                              ³  º
 +          º ³  ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿  ³  º
 +          º ³  ³                        ³  ³  º
 +          º ³  ³  Cnr inside group (2)  ³  ³  º
 +          º ³  ³                        ³  ³  º
 +          º ³  ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ  ³  º
 +          º ³  Static below cnr (3)        ³  º
 +          º ³                              ³  º
 +          º ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ  º
 +          º ÚÄÄÄÄÄÄÄÄÄÄÄ¿ ÚÄÄÄÄÄÄÄÄÄÄÄÄÄ¿     º
 +          º ³   OK (4)  ³ ³  Cancel (5) ³     º
 +          º ÀÄÄÄÄÄÄÄÄÄÄÄÙ ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÙ     º
 +          º                                   º
 +          ÈÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ¼
 *
 *      <B>Errors:</B>
 *
 *      This does not return a HWND, but an APIRET. This will be
 *      one of the following:
 *
 *      --  NO_ERROR: only in that case, the phwndDlg ptr
 *          receives the HWND of the new dialog, which can
 *          then be given to WinProcessDlg. Don't forget
 *          WinDestroyWindow.
 *
 *      --  ERROR_NOT_ENOUGH_MEMORY
 *
 *      --  DLGERR_ROW_BEFORE_TABLE: a row definition appeared
 *          outside a table definition.
 *
 *      --  DLGERR_CONTROL_BEFORE_ROW: a control definition
 *          appeared right after a table definition. You must
 *          specify a row first.
 *
 *      --  DLGERR_NULL_CTL_DEF: TYPE_END_TABLE was specified,
 *          but the CONTROLDEF ptr was NULL.
 *
 *      --  DLGERR_CANNOT_CREATE_FRAME: unable to create the
 *          WC_FRAME window. Maybe an invalid owner was specified.
 *
 *      --  DLGERR_INVALID_CODE: invalid "Type" field in
 *          DLGHITEM.
 *
 *      --  DLGERR_TABLE_NOT_CLOSED, DLGERR_TOO_MANY_TABLES_CLOSED:
 *          improper nesting of TYPE_START_NEW_TABLE and
 *          TYPE_END_TABLE fields.
 *
 *      <B>Example:</B>
 *
 *      The typical calling sequence would be:
 *
 +          HWND hwndDlg = NULLHANDLE;
 +          if (NO_ERROR == dlghCreateDlg(&hwndDlg,
 +                                        hwndOwner,
 +                                        FCF_DLGBORDER | FCF_NOBYTEALIGN,
 +                                        fnwpMyDlgProc,
 +                                        "My Dlg Title",
 +                                        G_aMyDialogTemplate,
 +                                        ARRAYITEMSIZE(G_aMyDialogTemplate),
 +                                        NULL))
 +          {
 +              ULONG idReturn = WinProcessDlg(pDlgData->hwndDlg);
 +              WinDestroyWindow(hwndDlg);
 +          }
 *
 *
 */

APIRET dlghCreateDlg(HWND *phwndDlg,            // out: new dialog
                     HWND hwndOwner,
                     ULONG flCreateFlags,       // in: standard FCF_* frame flags
                     PFNWP pfnwpDialogProc,
                     const char *pcszDlgTitle,
                     PDLGHITEM paDlgItems,      // in: definition array
                     ULONG cDlgItems,           // in: array item count (NOT array size)
                     PVOID pCreateParams,       // in: for mp2 of WM_INITDLG
                     const char *pcszControlsFont) // in: font for ctls with CTL_COMMON_FONT
{
    APIRET      arc = NO_ERROR;

    #define SPACING     10

    PTABLEDEF   pCurrentTable = NULL;
    PROWDEF     pCurrentRow = NULL;
    ULONG       ul;
    LINKLIST    llStack;

    PDLGPRIVATE  pDlgData = NEW(DLGPRIVATE);

    if (!pDlgData)
        return (ERROR_NOT_ENOUGH_MEMORY);

    ZERO(pDlgData);
    lstInit(&pDlgData->llTables, FALSE);
    lstInit(&pDlgData->llControls, FALSE);

    pDlgData->pcszControlsFont = pcszControlsFont;

    /*
     *  1) parse the table and create structures from it
     *
     */

    lstInit(&llStack, TRUE);      // this is our stack for nested table definitions

    for (ul = 0;
         ul < cDlgItems;
         ul++)
    {
        PDLGHITEM   pItemThis = &paDlgItems[ul];

        switch (pItemThis->Type)
        {
            /*
             * TYPE_START_NEW_TABLE:
             *
             */

            case TYPE_START_NEW_TABLE:
            {
                // root table or nested?
                BOOL fIsRoot = (pCurrentTable == NULL);

                // push the current table on the stack
                PSTACKITEM pStackItem = NEW(STACKITEM);
                if (pStackItem)
                {
                    pStackItem->pLastTable = pCurrentTable;
                    pStackItem->pLastRow = pCurrentRow;
                    lstPush(&llStack, pStackItem);
                }

                // create new table
                pCurrentTable = NEW(TABLEDEF);
                if (!pCurrentTable)
                    arc = ERROR_NOT_ENOUGH_MEMORY;
                else
                {
                    ZERO(pCurrentTable);

                    lstInit(&pCurrentTable->llRows, FALSE);

                    if (pItemThis->ulData)
                        // control specified: store it (this will become a PM group)
                        pCurrentTable->pCtlDef = (PCONTROLDEF)pItemThis->ulData;

                    if (fIsRoot)
                        // root table:
                        // append to dialog data list
                        lstAppendItem(&pDlgData->llTables, pCurrentTable);
                    else
                    {
                        // nested table:
                        // create "table" column for this
                        PCOLUMNDEF pColumnDef;
                        arc = CreateColumn(pCurrentRow,
                                           TRUE,        // nested table
                                           pCurrentTable,
                                           &pColumnDef);
                        if (!arc)
                            lstAppendItem(&pCurrentRow->llColumns, pColumnDef);
                    }
                }

                pCurrentRow = NULL;
            break; }

            /*
             * TYPE_START_NEW_ROW:
             *
             */

            case TYPE_START_NEW_ROW:
            {
                if (!pCurrentTable)
                    arc = DLGERR_ROW_BEFORE_TABLE;
                else
                {
                    // create new row
                    pCurrentRow = NEW(ROWDEF);
                    if (!pCurrentRow)
                        arc = ERROR_NOT_ENOUGH_MEMORY;
                    else
                    {
                        ZERO(pCurrentRow);

                        pCurrentRow->pOwningTable = pCurrentTable;
                        lstInit(&pCurrentRow->llColumns, FALSE);

                        pCurrentRow->flRowFormat = pItemThis->ulData;

                        lstAppendItem(&pCurrentTable->llRows, pCurrentRow);
                    }
                }
            break; }

            /*
             * TYPE_CONTROL_DEF:
             *
             */

            case TYPE_CONTROL_DEF:
            {
                PCOLUMNDEF pColumnDef;
                arc = CreateColumn(pCurrentRow,
                                   FALSE,        // no nested table
                                   (PVOID)pItemThis->ulData,
                                   &pColumnDef);
                if (!arc)
                    lstAppendItem(&pCurrentRow->llColumns, pColumnDef);
            break; }

            /*
             * TYPE_END_TABLE:
             *
             */

            case TYPE_END_TABLE:
            {
                PLISTNODE pNode = lstPop(&llStack);
                if (!pNode)
                    // nothing on the stack:
                    arc = DLGERR_TOO_MANY_TABLES_CLOSED;
                else
                {
                    PSTACKITEM pStackItem = (PSTACKITEM)pNode->pItemData;
                    pCurrentTable = pStackItem->pLastTable;
                    pCurrentRow = pStackItem->pLastRow;

                    lstRemoveNode(&llStack, pNode);
                }
            break; }

            default:
                arc = DLGERR_INVALID_CODE;
        }

        if (arc)
            break;
    }

    if (arc == NO_ERROR)
        if (lstCountItems(&llStack))
            arc = DLGERR_TABLE_NOT_CLOSED;

    lstClear(&llStack);

    if (arc == NO_ERROR)
    {
        /*
         *  2) create empty dialog frame
         *
         */

        FRAMECDATA      fcData = {0};

        #define FCF_DIALOGBOX   0x40000000L

        fcData.cb = sizeof(FRAMECDATA);
        fcData.flCreateFlags = flCreateFlags | FCF_DIALOGBOX;

        pDlgData->hwndDlg = WinCreateWindow(HWND_DESKTOP,
                                            WC_FRAME,
                                            (PSZ)pcszDlgTitle,
                                            0,               // style; invisible for now
                                            0, 0, 0, 0,
                                            hwndOwner,
                                            HWND_TOP,
                                            0,               // ID
                                            &fcData,
                                            NULL);           // presparams

        if (!pDlgData->hwndDlg)
            arc = DLGERR_CANNOT_CREATE_FRAME;
        else
        {
            SIZEL   szlClient = {0};
            RECTL   rclClient;
            HWND    hwndFocusItem = NULLHANDLE;

            /*
             *  3) compute size of client after computing all control sizes
             *
             */

            ProcessAll(pDlgData,
                       &szlClient,
                       PROCESS_CALC_SIZES);

            // this might have created a HPS and a font in dlgdata
            // for auto-sizing controls
            if (pDlgData->hps)
            {
                if (pDlgData->lcidLast)
                {
                    // delete old font
                    GpiSetCharSet(pDlgData->hps, LCID_DEFAULT);
                    GpiDeleteSetId(pDlgData->hps, pDlgData->lcidLast);
                    pDlgData->lcidLast = 0;
                }

                WinReleasePS(pDlgData->hps);
                pDlgData->hps = NULLHANDLE;
            }

            WinSubclassWindow(pDlgData->hwndDlg, pfnwpDialogProc);

            // calculate the frame size from the client size
            rclClient.xLeft = 10;
            rclClient.yBottom = 10;
            rclClient.xRight = szlClient.cx + 2 * SPACING;
            rclClient.yTop = szlClient.cy + 2 * SPACING;
            WinCalcFrameRect(pDlgData->hwndDlg,
                             &rclClient,
                             FALSE);            // frame from client

            WinSetWindowPos(pDlgData->hwndDlg,
                            0,
                            10,
                            10,
                            rclClient.xRight,
                            rclClient.yTop,
                            SWP_MOVE | SWP_SIZE | SWP_NOADJUST);

            /*
             *  4) compute positions of all controls
             *
             */

            ProcessAll(pDlgData,
                       &szlClient,
                       PROCESS_CALC_POSITIONS);

            /*
             *  5) create control windows, finally
             *
             */

            pDlgData->ptlTotalOfs.x = SPACING;
            pDlgData->ptlTotalOfs.y = SPACING;

            ProcessAll(pDlgData,
                       &szlClient,
                       PROCESS_CREATE_CONTROLS);

            /*
             *  6) WM_INITDLG, set focus
             *
             */

            hwndFocusItem = (pDlgData->hwndFirstFocus)
                                    ? pDlgData->hwndFirstFocus
                                    : pDlgData->hwndDlg;
            if (!WinSendMsg(pDlgData->hwndDlg,
                            WM_INITDLG,
                            (MPARAM)hwndFocusItem,
                            (MPARAM)pCreateParams))
            {
                // if WM_INITDLG returns FALSE, this means
                // the dlg proc has not changed the focus;
                // we must then set the focus here
                WinSetFocus(HWND_DESKTOP, hwndFocusItem);
            }
        }
    }

    if (pDlgData)
    {
        PLISTNODE pTableNode;

        if (arc)
        {
            // error: clean up
            if (pDlgData->hwndDlg)
                WinDestroyWindow(pDlgData->hwndDlg);
        }
        else
            // no error: output dialog
            *phwndDlg = pDlgData->hwndDlg;

        // in any case, clean up our mess:

        // clean up the tables
        FOR_ALL_NODES(&pDlgData->llTables, pTableNode)
        {
            PTABLEDEF pTable = (PTABLEDEF)pTableNode->pItemData;

            // for each table, clean up the rows
            PLISTNODE pRowNode;
            FOR_ALL_NODES(&pTable->llRows, pRowNode)
            {
                PROWDEF pRow = (PROWDEF)pRowNode->pItemData;

                // for each row, clean up the columns
                PLISTNODE pColumnNode;
                FOR_ALL_NODES(&pRow->llColumns, pColumnNode)
                {
                    PCOLUMNDEF pColumn = (PCOLUMNDEF)pColumnNode->pItemData;
                    free(pColumn);
                }
                lstClear(&pRow->llColumns);

                free(pRow);
            }
            lstClear(&pTable->llRows);

            free(pTable);
        }

        lstClear(&pDlgData->llTables);
        lstClear(&pDlgData->llControls);

        free(pDlgData);
    }

    return (arc);
}


