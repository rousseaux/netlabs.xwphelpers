
/*
 *@@sourcefile dialog.c:
 *      contains PM helper functions to create and
 *      auto-format dialogs from control arrays in memory.
 *
 *      See dlghCreateDlg for details.
 *
 *      In addition, this has dlghMessageBox (a WinMessageBox
 *      replacement) and some helper functions for simulating
 *      dialog behavior in regular window procs (see
 *      dlghSetPrevFocus and others).
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
#define INCL_WININPUT
#define INCL_WINSTATICS
#define INCL_WINBUTTONS
#define INCL_WINENTRYFIELDS
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
#include "helpers\xstring.h"

#pragma hdrstop

// #define DEBUG_DIALOG_WINDOWS 1

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
 *      by dlghCreateDlg.
 *
 *      This only exists while the dialog is being
 *      created and is not stored with the new dialog.
 */

typedef struct _DLGPRIVATE
{
    // public data
    HWND        hwndDlg;

    // definition data (private)
    LINKLIST    llTables;

    HWND        hwndFirstFocus,
                hwndDefPushbutton;      // V0.9.14 (2001-08-21) [umoeller]

    POINTL      ptlTotalOfs;

    PLINKLIST   pllControls;            // linked list of HWNDs in the order
                                        // in which controls were created;
                                        // ptr can be NULL

    PCSZ        pcszControlsFont;  // from dlghCreateDlg

    // size of the client to be created
    SIZEL       szlClient;

    // various cached data V0.9.14 (2001-08-01) [umoeller]
    HPS         hps;
    PCSZ        pcszFontLast;
    LONG        lcidLast;
    FONTMETRICS fmLast;

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
                                    // specified by the caller

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
    PCOLUMNDEF  pOwningColumn;      // != NULL if this is a nested table

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
    PROCESS_1_CALC_SIZES,             // step 1
    PROCESS_2_CALC_SIZES_FROM_TABLES, // step 2
    PROCESS_3_CALC_FINAL_TABLE_SIZES, // step 3
    PROCESS_4_CALC_POSITIONS,         // step 4
    PROCESS_5_CREATE_CONTROLS         // step 5
} PROCESSMODE;

/* ******************************************************************
 *
 *   Worker routines
 *
 ********************************************************************/

static APIRET ProcessTable(PTABLEDEF pTableDef,
                           const CONTROLPOS *pcpTable,
                           PROCESSMODE ProcessMode,
                           PDLGPRIVATE pDlgData);

/*
 *@@ SetDlgFont:
 *
 *@@added V0.9.16 (2001-10-11) [umoeller]
 */

static VOID SetDlgFont(PCONTROLDEF pControlDef,
                       PDLGPRIVATE pDlgData)
{
    LONG lPointSize = 0;
    PCSZ pcszFontThis = pControlDef->pcszFont;
                    // can be NULL,
                    // or CTL_COMMON_FONT

    if (pcszFontThis == CTL_COMMON_FONT)
        pcszFontThis = pDlgData->pcszControlsFont;

    if (!pDlgData->hps)
        pDlgData->hps = WinGetPS(pDlgData->hwndDlg);

    // check if we can reuse font data from last time
    // V0.9.14 (2001-08-01) [umoeller]
    if (strhcmp(pcszFontThis,               // can be NULL!
                pDlgData->pcszFontLast))
    {
        // different font than last time:

        // delete old font?
        if (pDlgData->lcidLast)
        {
            GpiSetCharSet(pDlgData->hps, LCID_DEFAULT);     // LCID_DEFAULT == 0
            GpiDeleteSetId(pDlgData->hps, pDlgData->lcidLast);
        }

        if (pcszFontThis)
        {
            // create new font
            pDlgData->lcidLast = gpihFindPresFont(NULLHANDLE,        // no window yet
                                                  FALSE,
                                                  pDlgData->hps,
                                                  pcszFontThis,
                                                  &pDlgData->fmLast,
                                                  &lPointSize);

            GpiSetCharSet(pDlgData->hps, pDlgData->lcidLast);
            if (pDlgData->fmLast.fsDefn & FM_DEFN_OUTLINE)
                gpihSetPointSize(pDlgData->hps, lPointSize);
        }
        else
        {
            // use default font:
            // @@todo handle presparams, maybe inherited?
            GpiSetCharSet(pDlgData->hps, LCID_DEFAULT);
            GpiQueryFontMetrics(pDlgData->hps,
                                sizeof(pDlgData->fmLast),
                                &pDlgData->fmLast);
        }

        pDlgData->pcszFontLast = pcszFontThis;      // can be NULL
    }
}

/*
 *@@ CalcAutoSizeText:
 *
 *@@changed V0.9.12 (2001-05-31) [umoeller]: fixed various things with statics
 *@@changed V0.9.12 (2001-05-31) [umoeller]: fixed broken fonts
 *@@changed V0.9.14 (2001-08-01) [umoeller]: now caching fonts, which is significantly faster
 *@@changed V0.9.16 (2001-10-15) [umoeller]: added APIRET
 *@@changed V0.9.16 (2002-02-02) [umoeller]: added ulWidth
 */

static APIRET CalcAutoSizeText(PCONTROLDEF pControlDef,
                               BOOL fMultiLine,          // in: if TRUE, multiple lines
                               ULONG ulWidth,            // in: proposed width of control
                               PSIZEL pszlAuto,          // out: computed size
                               PDLGPRIVATE pDlgData)
{
    APIRET arc = NO_ERROR;

    SetDlgFont(pControlDef, pDlgData);

    pszlAuto->cy =   pDlgData->fmLast.lMaxBaselineExt
                   + pDlgData->fmLast.lExternalLeading;

    // ok, we FINALLY have a font now...
    // get the control string and see how much space it needs
    if (    (pControlDef->pcszText)
         && (pControlDef->pcszText != (PCSZ)-1)
       )
    {
        // do we have multiple lines?
        if (fMultiLine)
        {
            RECTL rcl = {0, 0, 0, 0};
            /*
            if (pControlDef->szlControlProposed.cx > 0)
                rcl.xRight = pControlDef->szlControlProposed.cx;   // V0.9.12 (2001-05-31) [umoeller]
            else
                rcl.xRight = winhQueryScreenCX() * 2 / 3;
            */
            rcl.xRight = ulWidth;
            if (pControlDef->szlControlProposed.cy > 0)
                rcl.yTop = pControlDef->szlControlProposed.cy;   // V0.9.12 (2001-05-31) [umoeller]
            else
                rcl.yTop = winhQueryScreenCY() * 2 / 3;

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
    else
        arc = DLGERR_INVALID_CONTROL_TITLE;

    return (arc);
}

/*
 *@@ CalcAutoSize:
 *
 *@@changed V0.9.12 (2001-05-31) [umoeller]: fixed various things with statics
 *@@changed V0.9.16 (2001-10-15) [umoeller]: added APIRET
 */

static APIRET CalcAutoSize(PCONTROLDEF pControlDef,
                           ULONG ulWidth,            // in: proposed width of control
                           PSIZEL pszlAuto,          // out: computed size
                           PDLGPRIVATE pDlgData)
{
    APIRET arc = NO_ERROR;

    // dumb defaults
    pszlAuto->cx = 100;
    pszlAuto->cy = 30;

    switch ((ULONG)pControlDef->pcszClass)
    {
        case 0xffff0003L: // WC_BUTTON:
            if (!(arc = CalcAutoSizeText(pControlDef,
                                         FALSE,         // no multiline
                                         ulWidth,
                                         pszlAuto,
                                         pDlgData)))
            {
                if (pControlDef->flStyle & (  BS_AUTOCHECKBOX
                                            | BS_AUTORADIOBUTTON
                                            | BS_AUTO3STATE
                                            | BS_3STATE
                                            | BS_CHECKBOX
                                            | BS_RADIOBUTTON))
                {
                    // give a little extra width for the box bitmap
                    pszlAuto->cx += 20;     // @@todo
                    // and height
                    pszlAuto->cy += 2;
                }
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
            }
        break;

        case 0xffff0005L: // WC_STATIC:
            if ((pControlDef->flStyle & 0x0F) == SS_TEXT)
                arc = CalcAutoSizeText(pControlDef,
                                       ((pControlDef->flStyle & DT_WORDBREAK) != 0),
                                       ulWidth,
                                       pszlAuto,
                                       pDlgData);
            else if ((pControlDef->flStyle & 0x0F) == SS_BITMAP)
            {
                HBITMAP hbm;
                if (hbm = (HBITMAP)pControlDef->pcszText)
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
                    else
                        arc = DLGERR_INVALID_STATIC_BITMAP;
                }
            }
            else if ((pControlDef->flStyle & 0x0F) == SS_ICON)
            {
                pszlAuto->cx = WinQuerySysValue(HWND_DESKTOP, SV_CXICON);
                pszlAuto->cy = WinQuerySysValue(HWND_DESKTOP, SV_CYICON);
            }
        break;

        default:
            // any other control (just to be safe):
            SetDlgFont(pControlDef, pDlgData);
            pszlAuto->cx = 50;
            pszlAuto->cy =   pDlgData->fmLast.lMaxBaselineExt
                           + pDlgData->fmLast.lExternalLeading
                           + 7;         // some space
    }

    return (arc);
}

/*
 *@@ ColumnCalcSizes:
 *      implementation for PROCESS_1_CALC_SIZES in
 *      ProcessColumn.
 *
 *      This gets called a second time for
 *      PROCESS_3_CALC_FINAL_TABLE_SIZES (V0.9.16).
 *
 *@@added V0.9.15 (2001-08-26) [umoeller]
 *@@changed V0.9.16 (2001-10-15) [umoeller]: fixed ugly group table spacings
 *@@changed V0.9.16 (2001-10-15) [umoeller]: added APIRET
 *@@changed V0.9.16 (2002-02-02) [umoeller]: added support for explicit group size
 */

static APIRET ColumnCalcSizes(PCOLUMNDEF pColumnDef,
                              PROCESSMODE ProcessMode,     // in: PROCESS_1_CALC_SIZES or PROCESS_3_CALC_FINAL_TABLE_SIZES
                              PDLGPRIVATE pDlgData)
{
    APIRET arc = NO_ERROR;

    ULONG       ulExtraCX = 0,
                ulExtraCY = 0;
    if (pColumnDef->fIsNestedTable)
    {
        // nested table: recurse!!
        PTABLEDEF pTableDef = (PTABLEDEF)pColumnDef->pvDefinition;
        if (!(arc = ProcessTable(pTableDef,
                                 NULL,
                                 ProcessMode,
                                 pDlgData)))
        {
            // store the size of the sub-table
            pColumnDef->cpControl.cx = pTableDef->cpTable.cx;
            pColumnDef->cpControl.cy = pTableDef->cpTable.cy;

            // should we create a PM control around the table?
            if (pTableDef->pCtlDef)
            {
                // yes:

                // check if maybe an explicit size was specified
                // for the group; if that is larger than what
                // we've calculated above, use it instead
                if (pTableDef->pCtlDef->szlControlProposed.cx > pColumnDef->cpControl.cx)
                        // should be -1 for auto-size
                    pColumnDef->cpControl.cx = pTableDef->pCtlDef->szlControlProposed.cx;

                if (pTableDef->pCtlDef->szlControlProposed.cy > pColumnDef->cpControl.cy)
                        // should be -1 for auto-size
                    pColumnDef->cpControl.cy = pTableDef->pCtlDef->szlControlProposed.cy;

                // in any case, make this wider
                ulExtraCX =    2 * PM_GROUP_SPACING_X;
                ulExtraCY =    (PM_GROUP_SPACING_X + PM_GROUP_SPACING_TOP);
            }
        }
    }
    else
    {
        // no nested table, but control:
        PCONTROLDEF pControlDef = (PCONTROLDEF)pColumnDef->pvDefinition;
        SIZEL       szlAuto;

        // do auto-size calculations only on the first loop
        // V0.9.16 (2002-02-02) [umoeller]
        if (ProcessMode == PROCESS_1_CALC_SIZES)
        {
            if (    (pControlDef->szlControlProposed.cx == -1)
                 || (pControlDef->szlControlProposed.cy == -1)
               )
            {
                ULONG ulWidth;
                if (pControlDef->szlControlProposed.cx == -1)
                    ulWidth = 1000;
                else
                    ulWidth = pControlDef->szlControlProposed.cx;
                arc = CalcAutoSize(pControlDef,
                                   ulWidth,
                                   &szlAuto,
                                   pDlgData);
            }

            if (    (pControlDef->szlControlProposed.cx < -1)
                 && (pControlDef->szlControlProposed.cx >= -100)
               )
            {
                // other negative CX value:
                // this is then a percentage of the table width... ignore for now
                // V0.9.16 (2002-02-02) [umoeller]
                szlAuto.cx = 0;
            }

            if (    (pControlDef->szlControlProposed.cy < -1)
                 && (pControlDef->szlControlProposed.cy >= -100)
               )
            {
                // other negative CY value:
                // this is then a percentage of the row height... ignore for now
                // V0.9.16 (2002-02-02) [umoeller]
                szlAuto.cy = 0;
            }

            if (!arc)
            {
                if (pControlDef->szlControlProposed.cx < 0)
                    pColumnDef->cpControl.cx = szlAuto.cx;
                else
                    pColumnDef->cpControl.cx = pControlDef->szlControlProposed.cx;

                if (pControlDef->szlControlProposed.cy < 0)
                    pColumnDef->cpControl.cy = szlAuto.cy;
                else
                    pColumnDef->cpControl.cy = pControlDef->szlControlProposed.cy;
            }

        } // end if (ProcessMode == PROCESS_1_CALC_SIZES)


        ulExtraCX
        = ulExtraCY
        = (2 * pControlDef->ulSpacing);
    }

    pColumnDef->cpColumn.cx =   pColumnDef->cpControl.cx
                               + ulExtraCX;
    pColumnDef->cpColumn.cy =   pColumnDef->cpControl.cy
                               + ulExtraCY;

    return (arc);
}

/*
 *@@ ColumnCalcPositions:
 *      implementation for PROCESS_4_CALC_POSITIONS in
 *      ProcessColumn.
 *
 *@@added V0.9.15 (2001-08-26) [umoeller]
 *@@changed V0.9.16 (2001-10-15) [umoeller]: added APIRET
 */

static APIRET ColumnCalcPositions(PCOLUMNDEF pColumnDef,
                                  PROWDEF pOwningRow,          // in: current row from ProcessRow
                                  PLONG plX,                   // in/out: PROCESS_4_CALC_POSITIONS only
                                  PDLGPRIVATE pDlgData)
{
    APIRET arc = NO_ERROR;

    // calculate column position: this includes spacing
    LONG   lSpacingX = 0,
           lSpacingY = 0;

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
        {
            // yes:
            lSpacingX = PM_GROUP_SPACING_X;     // V0.9.16 (2001-10-15) [umoeller]
            lSpacingY = PM_GROUP_SPACING_X;     // V0.9.16 (2001-10-15) [umoeller]
        }
    }
    else
    {
        // no nested table, but control:
        PCONTROLDEF pControlDef = (PCONTROLDEF)pColumnDef->pvDefinition;
        lSpacingX = lSpacingY = pControlDef->ulSpacing;
    }

    // increase plX by column width
    *plX += pColumnDef->cpColumn.cx;

    // calculate CONTROL pos from COLUMN pos by applying spacing
    pColumnDef->cpControl.x =   (LONG)pColumnDef->cpColumn.x
                              + lSpacingX;
    pColumnDef->cpControl.y =   (LONG)pColumnDef->cpColumn.y
                              + lSpacingY;

    if (pColumnDef->fIsNestedTable)
    {
        // nested table:
        PTABLEDEF pTableDef = (PTABLEDEF)pColumnDef->pvDefinition;

        // recurse!! to create windows for the sub-table
        arc = ProcessTable(pTableDef,
                           &pColumnDef->cpControl,   // start pos for new table
                           PROCESS_4_CALC_POSITIONS,
                           pDlgData);
    }

    return (arc);
}

/*
 *@@ ColumnCreateControls:
 *      implementation for PROCESS_5_CREATE_CONTROLS in
 *      ProcessColumn.
 *
 *@@added V0.9.15 (2001-08-26) [umoeller]
 *@@changed V0.9.16 (2001-10-15) [umoeller]: fixed ugly group table spacings
 *@@changed V0.9.16 (2001-12-08) [umoeller]: fixed entry field ES_MARGIN positioning
 */

static APIRET ColumnCreateControls(PCOLUMNDEF pColumnDef,
                                   PDLGPRIVATE pDlgData)
{
    APIRET      arc = NO_ERROR;

    PCONTROLDEF pControlDef = NULL;

    PCSZ        pcszClass = NULL;
    PCSZ        pcszTitle = NULL;
    ULONG       flStyle = 0;
    LHANDLE     lHandleSet = NULLHANDLE;
    ULONG       flOld = 0;

    LONG        x, cx, y, cy;              // for combo box hacks

    if (pColumnDef->fIsNestedTable)
    {
        // nested table:
        PTABLEDEF pTableDef = (PTABLEDEF)pColumnDef->pvDefinition;

        // recurse!!
        if (!(arc = ProcessTable(pTableDef,
                                 NULL,
                                 PROCESS_5_CREATE_CONTROLS,
                                 pDlgData)))
        {
            // should we create a PM control around the table?
            // (do this AFTER the other controls from recursing,
            // otherwise the stupid container doesn't show up)
            if (pTableDef->pCtlDef)
            {
                // yes:
                // pcp  = &pColumnDef->cpColumn;  // !! not control
                pControlDef = pTableDef->pCtlDef;
                pcszClass = pControlDef->pcszClass;
                pcszTitle = pControlDef->pcszText;
                flStyle = pControlDef->flStyle;

                x  =   pColumnDef->cpColumn.x
                     + pDlgData->ptlTotalOfs.x
                     + PM_GROUP_SPACING_X / 2;
                cx =   pColumnDef->cpColumn.cx
                     - PM_GROUP_SPACING_X;
                    // note, just one spacing: for the _column_ size,
                    // we have specified 2 X spacings
                y  =   pColumnDef->cpColumn.y
                     + pDlgData->ptlTotalOfs.y
                     + PM_GROUP_SPACING_X / 2;
                // cy = pcp->cy - PM_GROUP_SPACING_X;
                // cy = pcp->cy - /* PM_GROUP_SPACING_X - */ PM_GROUP_SPACING_TOP;
                cy =   pColumnDef->cpColumn.cy
                     - PM_GROUP_SPACING_X / 2; //  - PM_GROUP_SPACING_TOP / 2;
            }

#ifdef DEBUG_DIALOG_WINDOWS
            {
                HWND hwndDebug;
                // debug: create a frame with the exact size
                // of the _column_ (not the control), so this
                // includes spacing
                hwndDebug =
                   WinCreateWindow(pDlgData->hwndDlg,   // parent
                                WC_STATIC,
                                "",
                                WS_VISIBLE | SS_FGNDFRAME,
                                pTableDef->cpTable.x + pDlgData->ptlTotalOfs.x,
                                pTableDef->cpTable.y + pDlgData->ptlTotalOfs.y,
                                pTableDef->cpTable.cx,
                                pTableDef->cpTable.cy,
                                pDlgData->hwndDlg,   // owner
                                HWND_BOTTOM,
                                -1,
                                NULL,
                                NULL);
                winhSetPresColor(hwndDebug, PP_FOREGROUNDCOLOR, RGBCOL_BLUE);
            }
#endif
        }
    }
    else
    {
        // no nested table, but control:
        pControlDef = (PCONTROLDEF)pColumnDef->pvDefinition;
        // pcp = &pColumnDef->cpControl;
        pcszClass = pControlDef->pcszClass;
        pcszTitle = pControlDef->pcszText;
        flStyle = pControlDef->flStyle;

        x  =   pColumnDef->cpControl.x
             + pDlgData->ptlTotalOfs.x;
        cx =   pColumnDef->cpControl.cx;
        y  =   pColumnDef->cpControl.y
             + pDlgData->ptlTotalOfs.y;
        cy =   pColumnDef->cpControl.cy;

        // now implement hacks for certain controls
        switch ((ULONG)pControlDef->pcszClass)
        {
            case 0xffff0005L: // WC_STATIC:
                // change the title if this is a static with SS_BITMAP;
                // we have used a HBITMAP in there!
                if (    (    ((flStyle & 0x0F) == SS_BITMAP)
                          || ((flStyle & 0x0F) == SS_ICON)
                        )
                   )
                {
                    // change style flag to not use SS_BITMAP nor SS_ICON;
                    // control creation fails otherwise (stupid, stupid PM)
                    flOld = flStyle;
                    flStyle = ((flStyle & ~0x0F) | SS_FGNDFRAME);
                    pcszTitle = "";
                    lHandleSet = (LHANDLE)pControlDef->pcszText;
                }
            break;

            case 0xffff0002L:   // combobox
                // hack the stupid drop-down combobox which doesn't
                // expand otherwise (the size of the drop-down is
                // the full size of the control... duh)
                if (flStyle & (CBS_DROPDOWN | CBS_DROPDOWNLIST))
                {
                    y -= 100;
                    cy += 100;
                }
            break;

            case 0xffff0006L:   // entry field
            case 0xffff000AL:   // MLE:
                // the stupid entry field resizes itself if it has
                // the ES_MARGIN style, so correlate that too... dammit
                // V0.9.16 (2001-12-08) [umoeller]
                if (flStyle & ES_MARGIN)
                {
                    LONG cxMargin = 3 * WinQuerySysValue(HWND_DESKTOP, SV_CXBORDER);
                    LONG cyMargin = 3 * WinQuerySysValue(HWND_DESKTOP, SV_CYBORDER);

                    x += cxMargin;
                    y += cyMargin;
                    cx -= 2 * cxMargin;
                    cy -= 2 * cyMargin;
                    // cy -= cxMargin;
                }
            break;
        } // end switch ((ULONG)pControlDef->pcszClass)
    }

    if (pControlDef)
    {
        // create something:
        PCSZ pcszFont = pControlDef->pcszFont;
                        // can be NULL, or CTL_COMMON_FONT
        if (pcszFont == CTL_COMMON_FONT)
            pcszFont = pDlgData->pcszControlsFont;

        if (pColumnDef->hwndControl
            = WinCreateWindow(pDlgData->hwndDlg,   // parent
                              (PSZ)pcszClass,   // hacked
                              (pcszTitle)   // hacked
                                    ? (PSZ)pcszTitle
                                    : "",
                              flStyle,      // hacked
                              x,
                              y,
                              cx,
                              cy,
                              pDlgData->hwndDlg,   // owner
                              HWND_BOTTOM,
                              pControlDef->usID,
                              pControlDef->pvCtlData,
                              NULL))
        {
#ifdef DEBUG_DIALOG_WINDOWS
            {
                HWND hwndDebug;
                // debug: create a frame with the exact size
                // of the _column_ (not the control), so this
                // includes spacing
                hwndDebug =
                   WinCreateWindow(pDlgData->hwndDlg,   // parent
                                WC_STATIC,
                                "",
                                WS_VISIBLE | SS_FGNDFRAME,
                                pColumnDef->cpColumn.x + pDlgData->ptlTotalOfs.x,
                                pColumnDef->cpColumn.y + pDlgData->ptlTotalOfs.y,
                                pColumnDef->cpColumn.cx,
                                pColumnDef->cpColumn.cy,
                                pDlgData->hwndDlg,   // owner
                                HWND_BOTTOM,
                                -1,
                                NULL,
                                NULL);
                winhSetPresColor(hwndDebug, PP_FOREGROUNDCOLOR, RGBCOL_RED);
            }
#endif

            if (lHandleSet)
            {
                // subclass the damn static
                if ((flOld & 0x0F) == SS_ICON)
                    // this was a static:
                    ctlPrepareStaticIcon(pColumnDef->hwndControl,
                                         1);
                else
                    // this was a bitmap:
                    ctlPrepareStretchedBitmap(pColumnDef->hwndControl,
                                              TRUE);

                WinSendMsg(pColumnDef->hwndControl,
                           SM_SETHANDLE,
                           (MPARAM)lHandleSet,
                           0);
            }
            else
                if (pcszFont)
                    // we must set the font explicitly here...
                    // doesn't always work with WinCreateWindow
                    // presparams parameter, for some reason
                    // V0.9.12 (2001-05-31) [umoeller]
                    winhSetWindowFont(pColumnDef->hwndControl,
                                      pcszFont);

            // append window that was created
            // V0.9.18 (2002-03-03) [umoeller]
            if (pDlgData->pllControls)
                lstAppendItem(pDlgData->pllControls,
                              (PVOID)pColumnDef->hwndControl);

            // if this is the first control with WS_TABSTOP,
            // we give it the focus later
            if (    (flStyle & WS_TABSTOP)
                 && (!pDlgData->hwndFirstFocus)
               )
                pDlgData->hwndFirstFocus = pColumnDef->hwndControl;

            // if this is the first default push button,
            // go store it too
            // V0.9.14 (2001-08-21) [umoeller]
            if (    (!pDlgData->hwndDefPushbutton)
                 && ((ULONG)pControlDef->pcszClass == 0xffff0003L)
                 && (pControlDef->flStyle & BS_DEFAULT)
               )
                pDlgData->hwndDefPushbutton = pColumnDef->hwndControl;
        }
        else
            // V0.9.14 (2001-08-03) [umoeller]
            arc = DLGERR_CANNOT_CREATE_CONTROL;
    }

    return (arc);
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
 *      -- PROCESS_1_CALC_SIZES: size is taken from control def,
 *         or for tables, this produces a recursive ProcessTable
 *         call.
 *         Preconditions: none.
 *
 *      -- PROCESS_4_CALC_POSITIONS: position of each column
 *         is taken from *plX, which is increased by the
 *         column width by this call.
 *
 *         Preconditions: Owning row must already have its
 *         y position properly set, or we can't compute
 *         ours. Besides, plX must point to the current X
 *         in the row and will be incremented by the columns
 *         size here.
 *
 *      -- PROCESS_5_CREATE_CONTROLS: well, creates the controls.
 *
 *         For tables, this recurses again. If the table has
 *         a string assigned, this also produces a group box
 *         after the recursion.
 *
 *@@changed V0.9.12 (2001-05-31) [umoeller]: added control data
 *@@changed V0.9.12 (2001-05-31) [umoeller]: fixed font problems
 */

static APIRET ProcessColumn(PCOLUMNDEF pColumnDef,
                            PROWDEF pOwningRow,          // in: current row from ProcessRow
                            PROCESSMODE ProcessMode,     // in: processing mode (see ProcessAll)
                            PLONG plX,                   // in/out: PROCESS_4_CALC_POSITIONS only
                            PDLGPRIVATE pDlgData)
{
    APIRET arc = NO_ERROR;

    pColumnDef->pOwningRow = pOwningRow;

    switch (ProcessMode)
    {
        /*
         * PROCESS_1_CALC_SIZES:
         *      step 1.
         */

        case PROCESS_1_CALC_SIZES:
            arc = ColumnCalcSizes(pColumnDef,
                                  ProcessMode,
                                  pDlgData);
        break;

        /*
         * PROCESS_2_CALC_SIZES_FROM_TABLES:
         *
         */

        case PROCESS_2_CALC_SIZES_FROM_TABLES:
            if (pColumnDef->fIsNestedTable)
            {
                PTABLEDEF pTableDef = (PTABLEDEF)pColumnDef->pvDefinition;
                if (!(arc = ProcessTable(pTableDef,
                                         NULL,
                                         PROCESS_2_CALC_SIZES_FROM_TABLES,
                                         pDlgData)))
                    ;
            }
            else
            {
                // no nested table, but control:
                PCONTROLDEF pControlDef = (PCONTROLDEF)pColumnDef->pvDefinition;

                if (    (pControlDef->szlControlProposed.cx < -1)
                     && (pControlDef->szlControlProposed.cx >= -100)
                   )
                {
                    // other negative CX value:
                    // this we ignored during PROCESS_1_CALC_SIZES
                    // (see ColumnCalcSizes); now set it to the
                    // table width!
                    ULONG cxThis = pOwningRow->pOwningTable->cpTable.cx
                                    * -pControlDef->szlControlProposed.cx / 100;

                    // but the table already has spacing applied,
                    // so reduce that
                    pColumnDef->cpControl.cx = cxThis
                                            - (2 * pControlDef->ulSpacing);

                    pColumnDef->cpColumn.cx = cxThis;

                    // now we might have to re-compute auto-size
                    if (pControlDef->szlControlProposed.cy == -1)
                    {
                        SIZEL   szlAuto;
                        if (!(arc = CalcAutoSize(pControlDef,
                                                 // now that we now the width,
                                                 // use that!
                                                 pColumnDef->cpControl.cx,
                                                 &szlAuto,
                                                 pDlgData)))
                        {
                            LONG cyColumnOld = pColumnDef->cpColumn.cy;
                            LONG lDelta;
                            PROWDEF pRowThis;

                            pColumnDef->cpControl.cy = szlAuto.cy;
                            pColumnDef->cpColumn.cy = szlAuto.cy
                                        + (2 * pControlDef->ulSpacing);
                        }
                    }
                }

                if (    (pControlDef->szlControlProposed.cy < -1)
                     && (pControlDef->szlControlProposed.cy >= -100)
                   )
                {
                    // same thing for CY, but this time we
                    // take the row height
                    ULONG cyThis = pOwningRow->cpRow.cy
                                    * -pControlDef->szlControlProposed.cy / 100;

                    // but the table already has spacing applied,
                    // so reduce that
                    pColumnDef->cpControl.cy = cyThis
                                            - (2 * pControlDef->ulSpacing);

                    pColumnDef->cpColumn.cy = cyThis;
                }
            }
        break;

        /*
         * PROCESS_3_CALC_FINAL_TABLE_SIZES:
         *
         */

        case PROCESS_3_CALC_FINAL_TABLE_SIZES:
            // re-run calc sizes since we now know all
            // the auto-size items
            arc = ColumnCalcSizes(pColumnDef,
                                  ProcessMode,
                                  pDlgData);
        break;

        /*
         * PROCESS_4_CALC_POSITIONS:
         *      step 4.
         */

        case PROCESS_4_CALC_POSITIONS:
            arc = ColumnCalcPositions(pColumnDef,
                                      pOwningRow,
                                      plX,
                                      pDlgData);
        break;

        /*
         * PROCESS_5_CREATE_CONTROLS:
         *      step 5.
         */

        case PROCESS_5_CREATE_CONTROLS:
            arc = ColumnCreateControls(pColumnDef,
                                       pDlgData);
        break;
    }

    return (arc);
}

/*
 *@@ ProcessRow:
 *      level-3 procedure (called from ProcessTable),
 *      which in turn calls ProcessColumn for each column
 *      in the row.
 *
 *      See ProcessAll for the meaning of ProcessMode.
 */

static APIRET ProcessRow(PROWDEF pRowDef,
                         PTABLEDEF pOwningTable,     // in: current table from ProcessTable
                         PROCESSMODE ProcessMode,    // in: processing mode (see ProcessAll)
                         PLONG plY,                  // in/out: current y position (decremented)
                         PDLGPRIVATE pDlgData)
{
    APIRET  arc = NO_ERROR;
    LONG    lX;
    PLISTNODE pNode;

    pRowDef->pOwningTable = pOwningTable;

    if (    (ProcessMode == PROCESS_1_CALC_SIZES)
         || (ProcessMode == PROCESS_3_CALC_FINAL_TABLE_SIZES)
       )
    {
        pRowDef->cpRow.cx = 0;
        pRowDef->cpRow.cy = 0;
    }
    else if (ProcessMode == PROCESS_4_CALC_POSITIONS)
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

        if (!(arc = ProcessColumn(pColumnDefThis, pRowDef, ProcessMode, &lX, pDlgData)))
        {
            if (    (ProcessMode == PROCESS_1_CALC_SIZES)
                 || (ProcessMode == PROCESS_3_CALC_FINAL_TABLE_SIZES)
               )
            {
                // row width = sum of all columns
                pRowDef->cpRow.cx += pColumnDefThis->cpColumn.cx;

                // row height = maximum height of a column
                if (pRowDef->cpRow.cy < pColumnDefThis->cpColumn.cy)
                    pRowDef->cpRow.cy = pColumnDefThis->cpColumn.cy;
            }
        }
    }

    return (arc);
}

/*
 *@@ ProcessTable:
 *      level-2 procedure (called from ProcessAll),
 *      which in turn calls ProcessRow for each row
 *      in the table (which in turn calls ProcessColumn
 *      for each column in the row).
 *
 *      See ProcessAll for the meaning of ProcessMode.
 *
 *      This routine is a bit sick because it can even be
 *      called recursively from ProcessColumn (!) if a
 *      nested table is found in a COLUMNDEF.
 *
 *      With PROCESS_4_CALC_POSITIONS, pptl must specify
 *      the lower left corner of the table. For the
 *      root call, this will be {0, 0}; for nested calls,
 *      this must be the lower left corner of the column
 *      to which the nested table belongs.
 *
 */

static APIRET ProcessTable(PTABLEDEF pTableDef,
                           const CONTROLPOS *pcpTable,       // in: table position with PROCESS_4_CALC_POSITIONS
                           PROCESSMODE ProcessMode,          // in: processing mode (see ProcessAll)
                           PDLGPRIVATE pDlgData)
{
    APIRET  arc = NO_ERROR;
    LONG    lY;
    PLISTNODE pNode;

    switch (ProcessMode)
    {
        case PROCESS_1_CALC_SIZES:
        case PROCESS_3_CALC_FINAL_TABLE_SIZES:
            pTableDef->cpTable.cx = 0;
            pTableDef->cpTable.cy = 0;
        break;

        case PROCESS_4_CALC_POSITIONS:
            pTableDef->cpTable.x = pcpTable->x;
            pTableDef->cpTable.y = pcpTable->y;

            // start the rows on top
            lY = pcpTable->y + pTableDef->cpTable.cy;
        break;
    }

    FOR_ALL_NODES(&pTableDef->llRows, pNode)
    {
        PROWDEF pRowDefThis = (PROWDEF)pNode->pItemData;

        if (!(arc = ProcessRow(pRowDefThis, pTableDef, ProcessMode, &lY, pDlgData)))
        {
            if (    (ProcessMode == PROCESS_1_CALC_SIZES)
                 || (ProcessMode == PROCESS_3_CALC_FINAL_TABLE_SIZES)
               )
            {
                // table width = maximum width of a row
                if (pTableDef->cpTable.cx < pRowDefThis->cpRow.cx)
                    pTableDef->cpTable.cx = pRowDefThis->cpRow.cx;

                // table height = sum of all rows
                pTableDef->cpTable.cy += pRowDefThis->cpRow.cy;
            }
        }
        else
            break;
    }

    return (arc);
}

/*
 *@@ ProcessAll:
 *      level-1 procedure, which in turn calls ProcessTable
 *      for each root-level table found (which in turn
 *      calls ProcessRow for each row in the table, which
 *      in turn calls ProcessColumn for each column in
 *      the row).
 *
 *      The first trick to formatting is that ProcessAll will
 *      get three times, thus going down the entire tree three
 *      times, with ProcessMode being set to one of the
 *      following for each call (in this order):
 *
 *      --  PROCESS_1_CALC_SIZES: calculates the preliminary
 *          sizes of all tables, rows, columns, and controls
 *          except those controls that have specified that
 *          their size should depend on others.
 *
 *      --  PROCESS_2_CALC_SIZES_FROM_TABLES: calculates the
 *          sizes of those controls that want to depend on
 *          others.
 *
 *      --  PROCESS_3_CALC_FINAL_TABLE_SIZES: since the table
 *          and row sizes might have changed during
 *          PROCESS_2_CALC_SIZES_FROM_TABLES, we need to re-run
 *          to re-calculate the size of all rows and tables.
 *          After this first call, we know _all_ the sizes
 *          and can then calculate the positions.
 *
 *      --  PROCESS_4_CALC_POSITIONS: calculates the positions
 *          based on the sizes calculated before.
 *
 *      --  PROCESS_5_CREATE_CONTROLS: creates the controls with the
 *          positions and sizes calculated before.
 *
 *      The second trick is the precondition that tables may
 *      nest by allowing another table definition in a column.
 *      This way we can recurse from ProcessColumn back into
 *      ProcessTable and thus know the size and position of a
 *      nested table column just as if it were a regular control.
 */

static APIRET ProcessAll(PDLGPRIVATE pDlgData,
                         PROCESSMODE ProcessMode)
{
    APIRET arc = NO_ERROR;
    PLISTNODE pNode;
    CONTROLPOS cpTable;
    ZERO(&cpTable);

    switch (ProcessMode)
    {
        case PROCESS_1_CALC_SIZES:
        case PROCESS_3_CALC_FINAL_TABLE_SIZES:
            pDlgData->szlClient.cx = 0;
            pDlgData->szlClient.cy = 0;
        break;

        case PROCESS_4_CALC_POSITIONS:
            // start with the table on top
            cpTable.y = pDlgData->szlClient.cy;
        break;
    }

    FOR_ALL_NODES(&pDlgData->llTables, pNode)
    {
        PTABLEDEF pTableDefThis = (PTABLEDEF)pNode->pItemData;

        if (ProcessMode == PROCESS_4_CALC_POSITIONS)
        {
            cpTable.x = 0;
            cpTable.y -= pTableDefThis->cpTable.cy;
        }

        if (!(arc = ProcessTable(pTableDefThis,
                                 &cpTable,      // start pos
                                 ProcessMode,
                                 pDlgData)))
        {
            if (    (ProcessMode == PROCESS_2_CALC_SIZES_FROM_TABLES)
                 || (ProcessMode == PROCESS_3_CALC_FINAL_TABLE_SIZES)
               )
            {
                // all sizes have now been computed:
                pDlgData->szlClient.cx += pTableDefThis->cpTable.cx;
                pDlgData->szlClient.cy += pTableDefThis->cpTable.cy;
            }
        }
    }

    return (arc);
}

/*
 *@@ CreateColumn:
 *
 */

static APIRET CreateColumn(PROWDEF pCurrentRow,
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

/*
 *@@ FreeTable:
 *      frees the specified table and recurses
 *      into nested tables, if necessary.
 *
 *      This was added with V0.9.14 to fix the
 *      bad memory leaks with nested tables.
 *
 *@@added V0.9.14 (2001-08-01) [umoeller]
 */

static VOID FreeTable(PTABLEDEF pTable)
{
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

            if (pColumn->fIsNestedTable)
            {
                // nested table: recurse!
                PTABLEDEF pNestedTable = (PTABLEDEF)pColumn->pvDefinition;
                FreeTable(pNestedTable);
            }

            free(pColumn);
        }
        lstClear(&pRow->llColumns);

        free(pRow);
    }
    lstClear(&pTable->llRows);

    free(pTable);
}

/* ******************************************************************
 *
 *   Dialog formatter engine
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

#define SPACING     10

/*
 *@@ Dlg0_Init:
 *
 *@@added V0.9.15 (2001-08-26) [umoeller]
 *@@changed V0.9.18 (2002-03-03) [umoeller]: aded pllWindows
 */

static APIRET Dlg0_Init(PDLGPRIVATE *ppDlgData,
                        PCSZ pcszControlsFont,
                        PLINKLIST pllControls)
{
    PDLGPRIVATE pDlgData;
    if (!(pDlgData = NEW(DLGPRIVATE)))
        return (ERROR_NOT_ENOUGH_MEMORY);
    ZERO(pDlgData);
    lstInit(&pDlgData->llTables, FALSE);

    if (pllControls)
        pDlgData->pllControls = pllControls;

    pDlgData->pcszControlsFont = pcszControlsFont;

    *ppDlgData = pDlgData;

    return NO_ERROR;
}

/*
 *@@ Dlg1_ParseTables:
 *
 *@@added V0.9.15 (2001-08-26) [umoeller]
 */

static APIRET Dlg1_ParseTables(PDLGPRIVATE pDlgData,
                               PCDLGHITEM paDlgItems,      // in: definition array
                               ULONG cDlgItems)           // in: array item count (NOT array size)
{
    APIRET      arc = NO_ERROR;

    LINKLIST    llStack;
    ULONG       ul;
    PTABLEDEF   pCurrentTable = NULL;
    PROWDEF     pCurrentRow = NULL;

    lstInit(&llStack, TRUE);      // this is our stack for nested table definitions

    for (ul = 0;
         ul < cDlgItems;
         ul++)
    {
        PCDLGHITEM   pItemThis = &paDlgItems[ul];

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
                PSTACKITEM pStackItem;
                if (!(pStackItem = NEW(STACKITEM)))
                {
                    arc = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
                else
                {
                    pStackItem->pLastTable = pCurrentTable;
                    pStackItem->pLastRow = pCurrentRow;
                    lstPush(&llStack, pStackItem);
                }

                // create new table
                if (!(pCurrentTable = NEW(TABLEDEF)))
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
                        if (!(arc = CreateColumn(pCurrentRow,
                                                 TRUE,        // nested table
                                                 pCurrentTable,
                                                 &pColumnDef)))
                        {
                            pCurrentTable->pOwningColumn = pColumnDef;
                            lstAppendItem(&pCurrentRow->llColumns,
                                          pColumnDef);
                        }
                    }
                }

                pCurrentRow = NULL;
            }
            break;

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
                    if (!(pCurrentRow = NEW(ROWDEF)))
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
            }
            break;

            /*
             * TYPE_CONTROL_DEF:
             *
             */

            case TYPE_CONTROL_DEF:
            {
                PCOLUMNDEF pColumnDef;
                if (!(arc = CreateColumn(pCurrentRow,
                                         FALSE,        // no nested table
                                         (PVOID)pItemThis->ulData,
                                         &pColumnDef)))
                    lstAppendItem(&pCurrentRow->llColumns,
                                  pColumnDef);
            }
            break;

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
            }
            break;

            default:
                arc = DLGERR_INVALID_CODE;
        }

        if (arc)
            break;
    }

    if ((!arc) && (lstCountItems(&llStack)))
        arc = DLGERR_TABLE_NOT_CLOSED;

    lstClear(&llStack);

    return (arc);
}

/*
 *@@ Dlg2_CalcSizes:
 *
 *      After this, DLGPRIVATE.szlClient is valid.
 *
 *@@added V0.9.15 (2001-08-26) [umoeller]
 */

static APIRET Dlg2_CalcSizes(PDLGPRIVATE pDlgData)
{
    APIRET arc;

    if (!(arc = ProcessAll(pDlgData,
                           PROCESS_1_CALC_SIZES)))
                     // this goes into major recursions...
        // run again to compute sizes that depend on tables
        if (!(arc = ProcessAll(pDlgData,
                               PROCESS_2_CALC_SIZES_FROM_TABLES)))
            arc = ProcessAll(pDlgData,
                             PROCESS_3_CALC_FINAL_TABLE_SIZES);

    // free the cached font resources that
    // might have been created here
    if (pDlgData->hps)
    {
        if (pDlgData->lcidLast)
        {
            GpiSetCharSet(pDlgData->hps, LCID_DEFAULT);
            GpiDeleteSetId(pDlgData->hps, pDlgData->lcidLast);
        }
        WinReleasePS(pDlgData->hps);
    }

    return (arc);
}

/*
 *@@ Dlg3_PositionAndCreate:
 *
 *@@added V0.9.15 (2001-08-26) [umoeller]
 *@@changed V0.9.15 (2001-08-26) [umoeller]: BS_DEFAULT for other than first button was ignored, fixed
 */

static APIRET Dlg3_PositionAndCreate(PDLGPRIVATE pDlgData,
                                     HWND *phwndFocusItem)       // out: item to give focus to
{
    APIRET arc = NO_ERROR;

    /*
     *  5) compute _positions_ of all controls
     *
     */

    ProcessAll(pDlgData,
               PROCESS_4_CALC_POSITIONS);

    /*
     *  6) create control windows, finally
     *
     */

    pDlgData->ptlTotalOfs.x
    = pDlgData->ptlTotalOfs.y
    = SPACING;

    ProcessAll(pDlgData,
               PROCESS_5_CREATE_CONTROLS);

    if (pDlgData->hwndDefPushbutton)
    {
        // we had a default pushbutton:
        // go set it V0.9.14 (2001-08-21) [umoeller]
        WinSetWindowULong(pDlgData->hwndDlg,
                          QWL_DEFBUTTON,
                          pDlgData->hwndDefPushbutton);
        *phwndFocusItem = pDlgData->hwndDefPushbutton;
                // V0.9.15 (2001-08-26) [umoeller]
    }
    else
        *phwndFocusItem = (pDlgData->hwndFirstFocus)
                            ? pDlgData->hwndFirstFocus
                            : pDlgData->hwndDlg;

    return (arc);
}

/*
 *@@ Dlg9_Cleanup:
 *
 *@@added V0.9.15 (2001-08-26) [umoeller]
 */

static VOID Dlg9_Cleanup(PDLGPRIVATE *ppDlgData)
{
    PDLGPRIVATE pDlgData;
    if (    (ppDlgData)
         && (pDlgData = *ppDlgData)
       )
    {
        PLISTNODE pTableNode;

        // in any case, clean up our mess:

        // clean up the tables
        FOR_ALL_NODES(&pDlgData->llTables, pTableNode)
        {
            PTABLEDEF pTable = (PTABLEDEF)pTableNode->pItemData;

            FreeTable(pTable);
                    // this may recurse for nested tables
        }

        lstClear(&pDlgData->llTables);

        free(pDlgData);

        *ppDlgData = NULL;
    }
}

/* ******************************************************************
 *
 *   Dialog formatter entry points
 *
 ********************************************************************/

/*
 *@@ dlghCreateDlg:
 *      replacement for WinCreateDlg/WinLoadDlg for creating a
 *      dialog from a settings array in memory, which is
 *      formatted automatically.
 *
 *      This does NOT use regular dialog templates from
 *      module resources. Instead, you pass in an array
 *      of DLGHITEM structures, which define the controls
 *      and how they are to be formatted.
 *
 *      The main advantage compared to dialog resources is
 *      that with this function, you will never have to
 *      define control _positions_. Instead, you only specify
 *      the control _sizes_, and all positions are computed
 *      automatically here. Even better, for many controls,
 *      auto-sizing is supported according to the control's
 *      text (e.g. for statics and checkboxes). In a way,
 *      this is a bit similar to HTML tables.
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
 *      to the dialog with pCreateParams in mp2.
 *
 *      If this func returns no error, you can then use
 *      WinProcessDlg with the newly created dialog as usual. In
 *      your dlg proc, use WinDefDlgProc as usual.
 *
 *      There is NO run-time overhead for either code or memory
 *      after dialog creation; after this function returns, the
 *      dialog is a standard dialog as if loaded from WinLoadDlg.
 *      The array of DLGHITEM structures defines how the
 *      dialog is set up. All this is ONLY used by this function
 *      and NOT needed after the dialog has been created.
 *
 *      In DLGHITEM, the "Type" field determines what this
 *      structure defines. A number of handy macros have been
 *      defined to make this easier and to provide type-checking
 *      at compile time. See dialog.h for more.
 *
 *      Essentially, such a dialog item operates similarly to
 *      HTML tables. There are rows and columns in the table,
 *      and each control which is specified must be a column
 *      in some table. Tables may also nest (see below).
 *
 *      The DLGHITEM macros are:
 *
 *      --  START_TABLE starts a new table. The tables may nest,
 *          but must each be properly terminated with END_TABLE.
 *
 *      --  START_GROUP_TABLE(pDef) starts a group. This
 *          behaves exacly like START_TABLE, but in addition,
 *          it produces a static group control around the table.
 *          Useful for group boxes. pDef must point to a
 *          CONTROLDEF describing the control to be used for
 *          the group (usually a WC_STATIC with SS_GROUP style),
 *          whose size parameter is ignored.
 *
 *          As with START_TABLE, START_GROUP_TABLE must be
 *          terminated with END_TABLE.
 *
 *      --  START_ROW(fl) starts a new row in a table (regular
 *          or group). This must also be the first item after
 *          the (group) table tag.
 *
 *          fl specifies formatting flags for the row. This
 *          can be one of ROW_VALIGN_BOTTOM, ROW_VALIGN_CENTER,
 *          ROW_VALIGN_TOP and affects all items in the row.
 *
 *      --  CONTROL_DEF(pDef) defines a control in a table row.
 *          pDef must point to a CONTROLDEF structure.
 *
 *          Again, there is is NO information in CONTROLDEF
 *          about a control's _position_. Instead, the structure
 *          only contains the _size_ of the control. All
 *          positions are computed by this function, depending
 *          on the sizes of the controls and their nesting within
 *          the various tables.
 *
 *          If you specify SZL_AUTOSIZE with either cx or cy
 *          or both, the size of the control is even computed
 *          automatically. Presently, this only works for statics
 *          with SS_TEXT, SS_ICON, and SS_BITMAP, push buttons,
 *          and radio and check boxes.
 *
 *          Unless separated with START_ROW items, subsequent
 *          control items will be considered to be in the same
 *          row (== positioned next to each other).
 *
 *      There are a few rules, whose violation will produce
 *      an error:
 *
 *      --  The entire array must be enclosed in a table
 *          (the "root" table).
 *
 *      --  After START_TABLE or START_GROUP_TABLE, there must
 *          always be a START_ROW first.
 *
 *      While it is possible to set up the CONTROLDEFs manually
 *      as static structures, I recommend using the bunch of
 *      other macros that were defined in dialog.h for this.
 *      For example, you can use CONTROLDEF_PUSHBUTTON to create
 *      a push button, and many more.
 *
 *      To create a dialog, set up arrays like the following:
 *
 +          // control definitions referenced by DlgTemplate:
 +          CONTROLDEF
 +      (1)             GroupDef = CONTROLDEF_GROUP("Group",
 +                                                  -1,     // ID
 +                                                  SZL_AUTOSIZE,
 +                                                  SZL_AUTOSIZE),
 +      (2)             CnrDef = CONTROLDEF_CONTAINER(-1,   // ID,
 +                                                  50,
 +                                                  50),
 +      (3)             Static = CONTROLDEF_TEXT("Static below cnr",
 +                                                  -1,     // ID
 +                                                  SZL_AUTOSIZE,
 +                                                  SZL_AUTOSIZE),
 +      (4)             OKButton = CONTROLDEF_DEFPUSHBUTTON("~OK",
 +                                                  DID_OK,
 +                                                  SZL_AUTOSIZE,
 +                                                  SZL_AUTOSIZE),
 +      (5)             CancelButton = CONTROLDEF_PUSHBUTTON("~Cancel",
 +                                                  DID_CANCEL,
 +                                                  SZL_AUTOSIZE,
 +                                                  SZL_AUTOSIZE);
 +
 +          DLGHITEM DlgTemplate[] =
 +              {
 +                  START_TABLE,            // root table, required
 +                      START_ROW(0),       // row 1 in the root table, required
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
 +          º ³                              ³  º
 +          º ³  Static below cnr (3)        ³  º
 +          º ³                              ³  º
 +          º ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ  º
 +          º                                   º
 +          º ÚÄÄÄÄÄÄÄÄÄÄÄ¿ ÚÄÄÄÄÄÄÄÄÄÄÄÄÄ¿     º
 +          º ³   OK (4)  ³ ³  Cancel (5) ³     º
 +          º ÀÄÄÄÄÄÄÄÄÄÄÄÙ ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÙ     º
 +          º                                   º
 +          ÈÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ¼
 *
 *      <B>Example:</B>
 *
 *      The typical calling sequence would be:
 *
 +          HWND hwndDlg = NULLHANDLE;
 +          if (NO_ERROR == dlghCreateDlg(&hwndDlg,
 +                                        hwndOwner,
 +                                        FCF_TITLEBAR | FCF_SYSMENU
 +                                           | FCF_DLGBORDER | FCF_NOBYTEALIGN,
 +                                        fnwpMyDlgProc,
 +                                        "My Dlg Title",
 +                                        DlgTemplate,      // DLGHITEM array
 +                                        ARRAYITEMCOUNT(DlgTemplate),
 +                                        NULL,             // mp2 for WM_INITDLG
 +                                        "9.WarpSans"))    // default font
 +          {
 +              ULONG idReturn = WinProcessDlg(hwndDlg);
 +              WinDestroyWindow(hwndDlg);
 +          }
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
 *      --  DLGERR_CANNOT_CREATE_CONTROL: creation of some
 *          sub-control failed. Maybe an invalid window class
 *          was specified.
 *
 *      --  DLGERR_INVALID_CONTROL_TITLE: bad window title in
 *          control.
 *
 *      --  DLGERR_INVALID_STATIC_BITMAP: static bitmap contains
 *          an invalid bitmap handle.
 *
 *@@changed V0.9.14 (2001-07-07) [umoeller]: fixed disabled mouse with hwndOwner == HWND_DESKTOP
 *@@changed V0.9.14 (2001-08-01) [umoeller]: fixed major memory leaks with nested tables
 *@@changed V0.9.14 (2001-08-21) [umoeller]: fixed default push button problems
 *@@changed V0.9.16 (2001-12-06) [umoeller]: fixed bad owner if not direct desktop child
 */

APIRET dlghCreateDlg(HWND *phwndDlg,            // out: new dialog
                     HWND hwndOwner,
                     ULONG flCreateFlags,       // in: standard FCF_* frame flags
                     PFNWP pfnwpDialogProc,
                     PCSZ pcszDlgTitle,
                     PCDLGHITEM paDlgItems,      // in: definition array
                     ULONG cDlgItems,           // in: array item count (NOT array size)
                     PVOID pCreateParams,       // in: for mp2 of WM_INITDLG
                     PCSZ pcszControlsFont) // in: font for ctls with CTL_COMMON_FONT
{
    APIRET      arc = NO_ERROR;

    ULONG       ul;

    PDLGPRIVATE  pDlgData = NULL;

    HWND        hwndDesktop = WinQueryDesktopWindow(NULLHANDLE, NULLHANDLE);
                                        // works with a null HAB

    /*
     *  1) parse the table and create structures from it
     *
     */

    if (!(arc = Dlg0_Init(&pDlgData,
                          pcszControlsFont,
                          NULL)))
    {
        if (!(arc = Dlg1_ParseTables(pDlgData,
                                     paDlgItems,
                                     cDlgItems)))
        {
            /*
             *  2) create empty dialog frame
             *
             */

            FRAMECDATA      fcData = {0};
            ULONG           flStyle = 0;
            HWND            hwndOwnersParent;

            fcData.cb = sizeof(FRAMECDATA);
            fcData.flCreateFlags = flCreateFlags | 0x40000000L;

            if (flCreateFlags & FCF_SIZEBORDER)
                // dialog has size border:
                // add "clip siblings" style
                flStyle |= WS_CLIPSIBLINGS;

            if (hwndOwner == HWND_DESKTOP)
                // there's some dumb XWorkplace code left
                // which uses this, and this disables the
                // mouse for some reason
                // V0.9.14 (2001-07-07) [umoeller]
                hwndOwner = NULLHANDLE;

            // now, make sure the owner window is child of
            // HWND_DESKTOP... if it is not, we'll only disable
            // some dumb child window, which is not sufficient
            // V0.9.16 (2001-12-06) [umoeller]
            while (    (hwndOwner)
                    && (hwndOwnersParent = WinQueryWindow(hwndOwner, QW_PARENT))
                    && (hwndOwnersParent != hwndDesktop)
                  )
                hwndOwner = hwndOwnersParent;

            if (!(pDlgData->hwndDlg = WinCreateWindow(HWND_DESKTOP,
                                                      WC_FRAME,
                                                      (PSZ)pcszDlgTitle,
                                                      flStyle,        // style; invisible for now
                                                      0, 0, 0, 0,
                                                      hwndOwner,
                                                      HWND_TOP,
                                                      0,              // ID
                                                      &fcData,
                                                      NULL)))          // presparams
                arc = DLGERR_CANNOT_CREATE_FRAME;
            else
            {
                HWND    hwndDlg = pDlgData->hwndDlg;
                HWND    hwndFocusItem = NULLHANDLE;
                RECTL   rclClient;

                /*
                 *  3) compute size of all controls
                 *
                 */

                if (!(arc = Dlg2_CalcSizes(pDlgData)))
                {
                    WinSubclassWindow(hwndDlg, pfnwpDialogProc);

                    /*
                     *  4) compute size of dialog client from total
                     *     size of all controls
                     */

                    // calculate the frame size from the client size
                    rclClient.xLeft = 10;
                    rclClient.yBottom = 10;
                    rclClient.xRight = pDlgData->szlClient.cx + 2 * SPACING;
                    rclClient.yTop = pDlgData->szlClient.cy + 2 * SPACING;
                    WinCalcFrameRect(hwndDlg,
                                     &rclClient,
                                     FALSE);            // frame from client

                    WinSetWindowPos(hwndDlg,
                                    0,
                                    10,
                                    10,
                                    rclClient.xRight,
                                    rclClient.yTop,
                                    SWP_MOVE | SWP_SIZE | SWP_NOADJUST);

                    arc = Dlg3_PositionAndCreate(pDlgData,
                                                 &hwndFocusItem);

                    /*
                     *  7) WM_INITDLG, set focus
                     *
                     */

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
        }

        if (arc)
        {
            // error: clean up
            if (pDlgData->hwndDlg)
            {
                WinDestroyWindow(pDlgData->hwndDlg);
                pDlgData->hwndDlg = NULLHANDLE;
            }
        }
        else
            // no error: output dialog
            *phwndDlg = pDlgData->hwndDlg;

        Dlg9_Cleanup(&pDlgData);
    }

    if (arc)
    {
        CHAR szErr[300];
        sprintf(szErr, "Error %d occured in " __FUNCTION__ ".", arc);
        winhDebugBox(hwndOwner,
                     "Error in Dialog Manager",
                     szErr);
    }

    return (arc);
}

/*
 *@@ dlghFormatDlg:
 *      similar to dlghCreateDlg in that this can
 *      dynamically format dialog items.
 *
 *      The differences however are the following:
 *
 *      --  This assumes that hwndDlg already points
 *          to a valid dialog frame and that this
 *          dialog should be modified according to
 *          flFlags.
 *
 *      This is what's used in XWorkplace for notebook
 *      settings pages since these always have to be
 *      based on a resource dialog (which is loaded
 *      empty).
 *
 *      flFlags can be any combination of the following:
 *
 *      --  DFFL_CREATECONTROLS: paDlgItems points to
 *          an array of cDlgItems DLGHITEM structures
 *          (see dlghCreateDlg) which is used for creating
 *          subwindows in hwndDlg. By using this flag, the
 *          function will essentially work like dlghCreateDlg,
 *          except that the frame is already created.
 *
 *      If pszlClient is specified, it receives the required
 *      size of the client to surround all controls properly.
 *      You can then use dlghResizeFrame to resize the frame
 *      with a bit of spacing, if desired.
 *
 *@@added V0.9.16 (2001-09-29) [umoeller]
 *@@changed V0.9.18 (2002-03-03) [umoeller]: added pszlClient, fixed output
 */

APIRET dlghFormatDlg(HWND hwndDlg,              // in: dialog frame to work on
                     PCDLGHITEM paDlgItems,      // in: definition array
                     ULONG cDlgItems,           // in: array item count (NOT array size)
                     PCSZ pcszControlsFont, // in: font for ctls with CTL_COMMON_FONT
                     ULONG flFlags,             // in: DFFL_* flags
                     PSIZEL pszlClient,         // out: size of all controls (ptr can be NULL)
                     PVOID *ppllControls)   // out: new LINKLIST receiving HWNDs of created controls (ptr can be NULL)
{
    APIRET      arc = NO_ERROR;

    ULONG       ul;

    PDLGPRIVATE  pDlgData = NULL;
    PLINKLIST   pllControls = NULL;

    /*
     *  1) parse the table and create structures from it
     *
     */

    if (ppllControls)
        pllControls = *(PLINKLIST*)ppllControls = lstCreate(FALSE);

    if (!(arc = Dlg0_Init(&pDlgData,
                          pcszControlsFont,
                          pllControls)))
    {
        if (!(arc = Dlg1_ParseTables(pDlgData,
                                     paDlgItems,
                                     cDlgItems)))
        {
            HWND hwndFocusItem;

            /*
             *  2) create empty dialog frame
             *
             */

            pDlgData->hwndDlg = hwndDlg;

            /*
             *  3) compute size of all controls
             *
             */

            Dlg2_CalcSizes(pDlgData);

            if (pszlClient)
            {
                pszlClient->cx = pDlgData->szlClient.cx + 2 * SPACING;
                pszlClient->cy = pDlgData->szlClient.cy + 2 * SPACING;
            }

            if (flFlags & DFFL_CREATECONTROLS)
            {
                if (!(arc = Dlg3_PositionAndCreate(pDlgData,
                                                   &hwndFocusItem)))
                    WinSetFocus(HWND_DESKTOP, hwndFocusItem);
            }
        }

        Dlg9_Cleanup(&pDlgData);
    }

    if (arc)
    {
        CHAR szErr[300];
        sprintf(szErr, "Error %d occured in " __FUNCTION__ ".", arc);
        winhDebugBox(NULLHANDLE,
                     "Error in Dialog Manager",
                     szErr);
    }

    return (arc);
}

/*
 *@@ dlghResizeFrame:
 *
 *@@added V0.9.18 (2002-03-03) [umoeller]
 */

VOID dlghResizeFrame(HWND hwndDlg,
                     PSIZEL pszlClient)
{
    // calculate the frame size from the client size
    RECTL   rclClient;
    rclClient.xLeft = 10;
    rclClient.yBottom = 10;
    rclClient.xRight = pszlClient->cx;
    rclClient.yTop = pszlClient->cy;
    WinCalcFrameRect(hwndDlg,
                     &rclClient,
                     FALSE);            // frame from client

    WinSetWindowPos(hwndDlg,
                    0,
                    10,
                    10,
                    rclClient.xRight,
                    rclClient.yTop,
                    SWP_MOVE | SWP_SIZE | SWP_NOADJUST);
}

/* ******************************************************************
 *
 *   Dialog arrays
 *
 ********************************************************************/

/*
 *@@ dlghCreateArray:
 *      creates a "dialog array" for dynamically
 *      building a dialog template in memory.
 *
 *      A dialog array is simply an array of
 *      DLGHITEM structures, as you would normally
 *      define them statically in the source.
 *      However, there are situations where you
 *      might want to leave out certain controls
 *      depending on certain conditions, which
 *      can be difficult with static arrays.
 *
 *      As a result, these "array" functions have
 *      been added to allow for adding static
 *      DLGHITEM subarrays to a dynamic array in
 *      memory, which can then be passed to the
 *      formatter.
 *
 *      Usage:
 *
 *      1)  Call this function with the maximum
 *          amount of DLGHITEM's that will need
 *          to be allocated in cMaxItems. Set this
 *          to the total sum of all DLGHITEM's
 *          in all the subarrays.
 *
 *      2)  For each of the subarrays, call
 *          dlghAppendToArray to have the subarray
 *          appended to the dialog array.
 *          After each call, DLGARRAY.cDlgItemsNow
 *          will contain the actual total count of
 *          DLGHITEM's that were added.
 *
 *      3)  Call dlghCreateDialog with the dialog
 *          array.
 *
 *      4)  Call dlghFreeArray.
 *
 *      Sort of like this (error checking omitted):
 *
 +      DLGHITEM    dlgSampleFront =  ...   // always included
 +      DLGHITEM    dlgSampleSometimes =  ...   // not always included
 +      DLGHITEM    dlgSampleTail =  ...   // always included
 +
 +      PDLGARRAY pArraySample = NULL;
 +      dlghCreateArray(   ARRAYITEMCOUNT(dlgSampleFront)
 +                       + ARRAYITEMCOUNT(dlgSampleSometimes)
 +                       + ARRAYITEMCOUNT(dlgSampleTail),
 +                      &pArraySample);
 +
 +      // always include front
 +      dlghAppendToArray(pArraySample,
 +                        dlgSampleFront,
 +                        ARRAYITEMCOUNT(dlgSampleFront));
 +      // include "sometimes" conditionally
 +      if (...)
 +          dlghAppendToArray(pArraySample,
 +                            dlgSampleSometimes,
 +                            ARRAYITEMCOUNT(dlgSampleSometimes));
 +      // include tail always
 +      dlghAppendToArray(pArraySample,
 +                        dlgSampleTail,
 +                        ARRAYITEMCOUNT(dlgSampleTail));
 +
 +      // now create the dialog from the array
 +      dlghCreateDialog(&hwndDlg,
 +                       hwndOwner,
 +                       FCF_ ...
 +                       fnwpMyDialogProc,
 +                       "Title",
 +                       pArray->paDialogItems,     // dialog array!
 +                       pArray->cDlgItemsNow,      // real count of items!
 +                       NULL,
 +                       NULL);
 +
 +      dlghFreeArray(&pArraySample);
 *
 *@@added V0.9.16 (2001-10-15) [umoeller]
 */

APIRET dlghCreateArray(ULONG cMaxItems,
                       PDLGARRAY *ppArray)       // out: DLGARRAY
{
    APIRET arc = NO_ERROR;
    PDLGARRAY pArray;

    if (pArray = NEW(DLGARRAY))
    {
        ULONG cb;

        ZERO(pArray);
        if (    (cb = cMaxItems * sizeof(DLGHITEM))
             && (pArray->paDlgItems = (DLGHITEM*)malloc(cb))
           )
        {
            memset(pArray->paDlgItems, 0, cb);
            pArray->cDlgItemsMax = cMaxItems;
            *ppArray = pArray;
        }
        else
            arc = ERROR_NOT_ENOUGH_MEMORY;

        if (arc)
            dlghFreeArray(&pArray);
    }
    else
        arc = ERROR_NOT_ENOUGH_MEMORY;

    return arc;
}

/*
 *@@ dlghFreeArray:
 *      frees a dialog array created by dlghCreateArray.
 *
 *@@added V0.9.16 (2001-10-15) [umoeller]
 */

APIRET dlghFreeArray(PDLGARRAY *ppArray)
{
    PDLGARRAY pArray;
    if (    (ppArray)
         && (pArray = *ppArray)
       )
    {
        if (pArray->paDlgItems)
            free(pArray->paDlgItems);
        free(pArray);
    }
    else
        return ERROR_INVALID_PARAMETER;

    return NO_ERROR;
}

/*
 *@@ dlghAppendToArray:
 *      appends a subarray of DLGHITEM's to the
 *      given DLGARRAY. See dlghCreateArray for
 *      usage.
 *
 *      Returns:
 *
 *      --  NO_ERROR
 *
 *      --  ERROR_INVALID_PARAMETER
 *
 *      --  DLGERR_ARRAY_TOO_SMALL: pArray does not
 *          have enough memory to hold the new items.
 *          The cMaxItems parameter given to dlghCreateArray
 *          wasn't large enough.
 *
 *@@added V0.9.16 (2001-10-15) [umoeller]
 */

APIRET dlghAppendToArray(PDLGARRAY pArray,      // in: dialog array created by dlghCreateArray
                         PCDLGHITEM paItems,     // in: subarray to be appended
                         ULONG cItems)          // in: subarray item count (NOT array size)
{
    APIRET arc = NO_ERROR;
    if (pArray)
    {
        if (    (pArray->cDlgItemsMax >= cItems)
             && (pArray->cDlgItemsMax - pArray->cDlgItemsNow >= cItems)
           )
        {
            // enough space left in the array:
            memcpy(&pArray->paDlgItems[pArray->cDlgItemsNow],
                   paItems,     // source
                   cItems * sizeof(DLGHITEM));
            pArray->cDlgItemsNow += cItems;
        }
        else
            arc = DLGERR_ARRAY_TOO_SMALL;
    }
    else
        arc = ERROR_INVALID_PARAMETER;

    return (arc);
}

/* ******************************************************************
 *
 *   Standard dialogs
 *
 ********************************************************************/

/*
 *@@ dlghCreateMessageBox:
 *
 *@@added V0.9.13 (2001-06-21) [umoeller]
 *@@changed V0.9.14 (2001-07-26) [umoeller]: fixed missing focus on buttons
 */

APIRET dlghCreateMessageBox(HWND *phwndDlg,
                            HWND hwndOwner,
                            HPOINTER hptrIcon,
                            PCSZ pcszTitle,
                            PCSZ pcszMessage,
                            ULONG flFlags,
                            PCSZ pcszFont,
                            const MSGBOXSTRINGS *pStrings,
                            PULONG pulAlarmFlag)      // out: alarm sound to be played
{
    CONTROLDEF
        Icon = {
                        WC_STATIC,
                        NULL,           // text, set below
                        WS_VISIBLE | SS_ICON,
                        0,          // ID
                        NULL,       // no font
                        0,
                        { SZL_AUTOSIZE, SZL_AUTOSIZE },
                        5
                    },
        InfoText =
                    {
                        WC_STATIC,
                        NULL,       // text, set below
                        WS_VISIBLE | SS_TEXT | DT_WORDBREAK | DT_LEFT | DT_TOP,
                        10,          // ID
                        CTL_COMMON_FONT,
                        0,
                        { 400, SZL_AUTOSIZE },
                        5
                    },
        Buttons[] = {
                        {
                            WC_BUTTON,
                            NULL,       // text, set below
                            WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                            1,          // ID
                            CTL_COMMON_FONT,  // no font
                            0,
                            { 100, 30 },
                            5
                        },
                        {
                            WC_BUTTON,
                            NULL,       // text, set below
                            WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                            2,          // ID
                            CTL_COMMON_FONT,  // no font
                            0,
                            { 100, 30 },
                            5
                        },
                        {
                            WC_BUTTON,
                            NULL,       // text, set below
                            WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                            3,          // ID
                            CTL_COMMON_FONT,  // no font
                            0,
                            { 100, 30 },
                            5
                        }
                    };

    DLGHITEM MessageBox[] =
                 {
                    START_TABLE,
                        START_ROW(ROW_VALIGN_CENTER),
                            CONTROL_DEF(&Icon),
                        START_TABLE,
                            START_ROW(ROW_VALIGN_CENTER),
                                CONTROL_DEF(&InfoText),
                            START_ROW(ROW_VALIGN_CENTER),
                                CONTROL_DEF(&Buttons[0]),
                                CONTROL_DEF(&Buttons[1]),
                                CONTROL_DEF(&Buttons[2]),
                        END_TABLE,
                    END_TABLE
                 };

    ULONG flButtons = flFlags & 0xF;        // low nibble contains MB_YESNO etc.

    PCSZ        p0 = "Error",
                p1 = NULL,
                p2 = NULL;

    Icon.pcszText = (PCSZ)hptrIcon;
    InfoText.pcszText = pcszMessage;

    // now work on the three buttons of the dlg template:
    // give them proper titles or hide them
    if (flButtons == MB_OK)
    {
        p0 = pStrings->pcszOK;
    }
    else if (flButtons == MB_OKCANCEL)
    {
        p0 = pStrings->pcszOK;
        p1 = pStrings->pcszCancel;
    }
    else if (flButtons == MB_RETRYCANCEL)
    {
        p0 = pStrings->pcszRetry;
        p1 = pStrings->pcszCancel;
    }
    else if (flButtons == MB_ABORTRETRYIGNORE)
    {
        p0 = pStrings->pcszAbort;
        p1 = pStrings->pcszRetry;
        p2 = pStrings->pcszIgnore;
    }
    else if (flButtons == MB_YESNO)
    {
        p0 = pStrings->pcszYes;
        p1 = pStrings->pcszNo;
    }
    else if (flButtons == MB_YESNOCANCEL)
    {
        p0 = pStrings->pcszYes;
        p1 = pStrings->pcszNo;
        p2 = pStrings->pcszCancel;
    }
    else if (flButtons == MB_CANCEL)
    {
        p0 = pStrings->pcszCancel;
    }
    else if (flButtons == MB_ENTER)
    {
        p0 = pStrings->pcszEnter;
    }
    else if (flButtons == MB_ENTERCANCEL)
    {
        p0 = pStrings->pcszEnter;
        p1 = pStrings->pcszCancel;
    }
    else if (flButtons == MB_YES_YES2ALL_NO)
    {
        p0 = pStrings->pcszYes;
        p1 = pStrings->pcszYesToAll;
        p2 = pStrings->pcszNo;
    }

    // now set strings and hide empty buttons
    Buttons[0].pcszText = p0;

    if (p1)
        Buttons[1].pcszText = p1;
    else
        Buttons[1].flStyle &= ~WS_VISIBLE;

    if (p2)
        Buttons[2].pcszText = p2;
    else
        Buttons[2].flStyle &= ~WS_VISIBLE;

    // query default button IDs
    if (flFlags & MB_DEFBUTTON2)
        Buttons[1].flStyle |= BS_DEFAULT;
    else if (flFlags & MB_DEFBUTTON3)
        Buttons[2].flStyle |= BS_DEFAULT;
    else
        Buttons[0].flStyle |= BS_DEFAULT;

    *pulAlarmFlag = WA_NOTE;
    if (flFlags & (MB_ICONHAND | MB_ERROR))
        *pulAlarmFlag = WA_ERROR;
    else if (flFlags & (MB_ICONEXCLAMATION | MB_WARNING))
        *pulAlarmFlag = WA_WARNING;

    return (dlghCreateDlg(phwndDlg,
                          hwndOwner,
                          FCF_TITLEBAR | FCF_SYSMENU | FCF_DLGBORDER | FCF_NOBYTEALIGN,
                          WinDefDlgProc,
                          pcszTitle,
                          MessageBox,
                          ARRAYITEMCOUNT(MessageBox),
                          NULL,
                          pcszFont));
}

/*
 *@@ dlghProcessMessageBox:
 *
 *@@added V0.9.13 (2001-06-21) [umoeller]
 */

ULONG dlghProcessMessageBox(HWND hwndDlg,
                            ULONG ulAlarmFlag,
                            ULONG flFlags)
{
    ULONG ulrcDlg;
    ULONG flButtons = flFlags & 0xF;        // low nibble contains MB_YESNO etc.

    winhCenterWindow(hwndDlg);

    if (flFlags & MB_SYSTEMMODAL)
        WinSetSysModalWindow(HWND_DESKTOP, hwndDlg);

    if (ulAlarmFlag)
        WinAlarm(HWND_DESKTOP, ulAlarmFlag);

    ulrcDlg = WinProcessDlg(hwndDlg);

    WinDestroyWindow(hwndDlg);

    if (flButtons == MB_OK)
        return MBID_OK;
    else if (flButtons == MB_OKCANCEL)
        switch (ulrcDlg)
        {
            case 1:     return MBID_OK;
            default:    return MBID_CANCEL;
        }
    else if (flButtons == MB_RETRYCANCEL)
        switch (ulrcDlg)
        {
            case 1:     return MBID_RETRY;
            default:    return MBID_CANCEL;
        }
    else if (flButtons == MB_ABORTRETRYIGNORE)
        switch (ulrcDlg)
        {
            case 2:     return MBID_RETRY;
            case 3:     return MBID_IGNORE;
            default:    return MBID_ABORT;
        }
    else if (flButtons == MB_YESNO)
        switch (ulrcDlg)
        {
            case 1:     return MBID_YES;
            default:    return MBID_NO;
        }
    else if (flButtons == MB_YESNOCANCEL)
        switch (ulrcDlg)
        {
            case 1:     return MBID_YES;
            case 2:     return MBID_NO;
            default:    return MBID_CANCEL;
        }
    else if (flButtons == MB_CANCEL)
        return MBID_CANCEL;
    else if (flButtons == MB_ENTER)
        return MBID_ENTER;
    else if (flButtons == MB_ENTERCANCEL)
        switch (ulrcDlg)
        {
            case 1:     return MBID_ENTER;
            default:    return MBID_CANCEL;
        }
    else if (flButtons == MB_YES_YES2ALL_NO)
        switch (ulrcDlg)
        {
            case 1:     return MBID_YES;
            case 2:     return MBID_YES2ALL;
            default:    return MBID_NO;
        }

    return (MBID_CANCEL);
}

/*
 *@@ dlghMessageBox:
 *      WinMessageBox replacement.
 *
 *      This has all the flags of the standard call,
 *      but looks much prettier. Besides, it allows
 *      you to specify any icon to be displayed.
 *
 *      Currently the following flStyle's are supported:
 *
 *      -- MB_OK                      0x0000
 *      -- MB_OKCANCEL                0x0001
 *      -- MB_RETRYCANCEL             0x0002
 *      -- MB_ABORTRETRYIGNORE        0x0003
 *      -- MB_YESNO                   0x0004
 *      -- MB_YESNOCANCEL             0x0005
 *      -- MB_CANCEL                  0x0006
 *      -- MB_ENTER                   0x0007 (not implemented yet)
 *      -- MB_ENTERCANCEL             0x0008 (not implemented yet)
 *
 *      -- MB_YES_YES2ALL_NO          0x0009
 *          This is new: this has three buttons called "Yes"
 *          (MBID_YES), "Yes to all" (MBID_YES2ALL), "No" (MBID_NO).
 *
 *      -- MB_DEFBUTTON2            (for two-button styles)
 *      -- MB_DEFBUTTON3            (for three-button styles)
 *
 *      -- MB_ICONHAND
 *      -- MB_ICONEXCLAMATION
 *
 *      Returns MBID_* codes like WinMessageBox.
 *
 *@@added V0.9.13 (2001-06-21) [umoeller]
 */

ULONG dlghMessageBox(HWND hwndOwner,            // in: owner for msg box
                     HPOINTER hptrIcon,         // in: icon to display
                     PCSZ pcszTitle,     // in: title
                     PCSZ pcszMessage,   // in: message
                     ULONG flFlags,             // in: standard message box flags
                     PCSZ pcszFont,      // in: font (e.g. "9.WarpSans")
                     const MSGBOXSTRINGS *pStrings) // in: strings array
{
    HWND hwndDlg;
    ULONG ulAlarmFlag;
    APIRET arc = dlghCreateMessageBox(&hwndDlg,
                                      hwndOwner,
                                      hptrIcon,
                                      pcszTitle,
                                      pcszMessage,
                                      flFlags,
                                      pcszFont,
                                      pStrings,
                                      &ulAlarmFlag);

    if (!arc && hwndDlg)
    {
        // SHOW DIALOG
        return (dlghProcessMessageBox(hwndDlg,
                                      ulAlarmFlag,
                                      flFlags));
    }
    else
    {
        CHAR szMsg[100];
        sprintf(szMsg, "dlghCreateMessageBox reported error %u.", arc);
        WinMessageBox(HWND_DESKTOP,
                      NULLHANDLE,
                      "Error",
                      szMsg,
                      0,
                      MB_CANCEL | MB_MOVEABLE);
    }

    return (DID_CANCEL);
}

/*
 *@@ cmnTextEntryBox:
 *      common dialog for entering a text string.
 *      The dialog has a descriptive text on top
 *      with an entry field below and "OK" and "Cancel"
 *      buttons.
 *
 *      The string from the user is returned in a
 *      new buffer, which must be free'd by the caller.
 *      Returns NULL if the user pressed "Cancel".
 *
 *      fl can be any combination of the following
 *      flags:
 *
 *      --  TEBF_REMOVETILDE: tilde ("~") characters
 *          are removed from pcszTitle before setting
 *          the title. Useful for reusing menu item
 *          texts.
 *
 *      --  TEBF_REMOVEELLIPSE: ellipse ("...") strings
 *          are removed from pcszTitle before setting
 *          the title. Useful for reusing menu item
 *          texts.
 *
 *      --  TEBF_SELECTALL: the default text in the
 *          entry field is initially highlighted.
 *
 *@@added V0.9.15 (2001-09-14) [umoeller]
 */

PSZ dlghTextEntryBox(HWND hwndOwner,
                     PCSZ pcszTitle,          // in: dlg title
                     PCSZ pcszDescription,    // in: descriptive text above entry field
                     PCSZ pcszDefault,        // in: default text for entry field or NULL
                     PCSZ pcszOK,             // in: "OK" string
                     PCSZ pcszCancel,         // in: "Cancel" string
                     ULONG ulMaxLen,                 // in: maximum length for entry
                     ULONG fl,                       // in: TEBF_* flags
                     PCSZ pcszFont)           // in: font (e.g. "9.WarpSans")
{
    CONTROLDEF
                Static = {
                            WC_STATIC,
                            NULL,
                            WS_VISIBLE | SS_TEXT | DT_LEFT | DT_WORDBREAK,
                            -1,
                            CTL_COMMON_FONT,
                            0,
                            { 300, SZL_AUTOSIZE },     // size
                            5               // spacing
                         },
                Entry = {
                            WC_ENTRYFIELD,
                            NULL,
                            WS_VISIBLE | WS_TABSTOP | ES_LEFT | ES_MARGIN | ES_AUTOSCROLL,
                            999,
                            CTL_COMMON_FONT,
                            0,
                            { 300, SZL_AUTOSIZE },     // size
                            5               // spacing
                         },
                OKButton = {
                            WC_BUTTON,
                            NULL,
                            WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_DEFAULT,
                            DID_OK,
                            CTL_COMMON_FONT,
                            0,
                            { 100, 30 },    // size
                            5               // spacing
                         },
                CancelButton = {
                            WC_BUTTON,
                            NULL,
                            WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                            DID_CANCEL,
                            CTL_COMMON_FONT,
                            0,
                            { 100, 30 },    // size
                            5               // spacing
                         };
    DLGHITEM DlgTemplate[] =
        {
            START_TABLE,
                START_ROW(0),
                    CONTROL_DEF(&Static),
                START_ROW(0),
                    CONTROL_DEF(&Entry),
                START_ROW(0),
                    CONTROL_DEF(&OKButton),
                    CONTROL_DEF(&CancelButton),
            END_TABLE
        };

    HWND hwndDlg = NULLHANDLE;
    PSZ  pszReturn = NULL;
    XSTRING strTitle;

    xstrInitCopy(&strTitle, pcszTitle, 0);

    if (fl & (TEBF_REMOVEELLIPSE | TEBF_REMOVETILDE))
    {
        ULONG ulOfs;
        if (fl & TEBF_REMOVEELLIPSE)
        {
            ulOfs = 0;
            while (xstrFindReplaceC(&strTitle,
                                    &ulOfs,
                                    "...",
                                    ""))
                ;
        }

        if (fl & TEBF_REMOVETILDE)
        {
            ulOfs = 0;
            while (xstrFindReplaceC(&strTitle,
                                    &ulOfs,
                                    "~",
                                    ""))
                ;
        }
    }

    Static.pcszText = pcszDescription;

    OKButton.pcszText = pcszOK;
    CancelButton.pcszText = pcszCancel;

    if (NO_ERROR == dlghCreateDlg(&hwndDlg,
                                  hwndOwner,
                                  FCF_TITLEBAR | FCF_SYSMENU | FCF_DLGBORDER | FCF_NOBYTEALIGN,
                                  WinDefDlgProc,
                                  strTitle.psz,
                                  DlgTemplate,      // DLGHITEM array
                                  ARRAYITEMCOUNT(DlgTemplate),
                                  NULL,
                                  pcszFont))
    {
        HWND hwndEF = WinWindowFromID(hwndDlg, 999);
        winhCenterWindow(hwndDlg);
        winhSetEntryFieldLimit(hwndEF, ulMaxLen);
        if (pcszDefault)
        {
            WinSetWindowText(hwndEF, (PSZ)pcszDefault);
            if (fl & TEBF_SELECTALL)
                winhEntryFieldSelectAll(hwndEF);
        }
        WinSetFocus(HWND_DESKTOP, hwndEF);
        if (DID_OK == WinProcessDlg(hwndDlg))
            pszReturn = winhQueryWindowText(hwndEF);

        WinDestroyWindow(hwndDlg);
    }

    xstrClear(&strTitle);

    return (pszReturn);
}

/* ******************************************************************
 *
 *   Dialog input handlers
 *
 ********************************************************************/

/*
 *@@ dlghSetPrevFocus:
 *      "backward" function for rotating the focus
 *      in a dialog when the "shift+tab" keys get
 *      pressed.
 *
 *      pllWindows must be a linked list with the
 *      plain HWND window handles of the focussable
 *      controls in the dialog.
 */

VOID dlghSetPrevFocus(PVOID pvllWindows)
{
    PLINKLIST   pllWindows = (PLINKLIST)pvllWindows;

    // check current focus
    HWND        hwndFocus = WinQueryFocus(HWND_DESKTOP);

    PLISTNODE   pNode = lstNodeFromItem(pllWindows, (PVOID)hwndFocus);

    BOOL fRestart = FALSE;

    while (pNode)
    {
        CHAR    szClass[100];

        // previos node
        pNode = pNode->pPrevious;

        if (    (!pNode)        // no next, or not found:
             && (!fRestart)     // avoid infinite looping if bad list
           )
        {
            pNode = lstQueryLastNode(pllWindows);
            fRestart = TRUE;
        }

        if (pNode)
        {
            // check if this is a focusable control
            if (WinQueryClassName((HWND)pNode->pItemData,
                                  sizeof(szClass),
                                  szClass))
            {
                if (    (strcmp(szClass, "#5"))    // not static
                   )
                    break;
                // else: go for next then
            }
        }
    }

    if (pNode)
    {
        WinSetFocus(HWND_DESKTOP,
                    (HWND)pNode->pItemData);
    }
}

/*
 *@@ dlghSetNextFocus:
 *      "forward" function for rotating the focus
 *      in a dialog when the "ab" key gets pressed.
 *
 *      pllWindows must be a linked list with the
 *      plain HWND window handles of the focussable
 *      controls in the dialog.
 */

VOID dlghSetNextFocus(PVOID pvllWindows)
{
    PLINKLIST   pllWindows = (PLINKLIST)pvllWindows;

    // check current focus
    HWND        hwndFocus = WinQueryFocus(HWND_DESKTOP);

    PLISTNODE   pNode = lstNodeFromItem(pllWindows, (PVOID)hwndFocus);

    BOOL fRestart = FALSE;

    while (pNode)
    {
        CHAR    szClass[100];

        // next focus in node
        pNode = pNode->pNext;

        if (    (!pNode)        // no next, or not found:
             && (!fRestart)     // avoid infinite looping if bad list
           )
        {
            pNode = lstQueryFirstNode(pllWindows);
            fRestart = TRUE;
        }

        if (pNode)
        {
            // check if this is a focusable control
            if (WinQueryClassName((HWND)pNode->pItemData,
                                  sizeof(szClass),
                                  szClass))
            {
                if (    (strcmp(szClass, "#5"))    // not static
                   )
                    break;
                // else: go for next then
            }
        }
    }

    if (pNode)
    {
        WinSetFocus(HWND_DESKTOP,
                    (HWND)pNode->pItemData);
    }
}

/*
 *@@ dlghProcessMnemonic:
 *      finds the control which matches usch
 *      and gives it the focus. If this is a
 *      static control, the next control in the
 *      list is given focus instead. (Standard
 *      dialog behavior.)
 *
 *      Pass in usch from WM_CHAR. It is assumed
 *      that the caller has already tested for
 *      the "alt" key to be depressed.
 *
 *@@added V0.9.9 (2001-03-17) [umoeller]
 */

HWND dlghProcessMnemonic(PVOID pvllWindows,
                         USHORT usch)
{
    PLINKLIST   pllWindows = (PLINKLIST)pvllWindows;

    HWND hwndFound = NULLHANDLE;
    PLISTNODE pNode = lstQueryFirstNode(pllWindows);
    CHAR szClass[100];

    while (pNode)
    {
        HWND hwnd = (HWND)pNode->pItemData;

        if (WinSendMsg(hwnd,
                       WM_MATCHMNEMONIC,
                       (MPARAM)usch,
                       0))
        {
            // according to the docs, only buttons and static
            // return TRUE to that msg;
            // if this is a static, give focus to the next
            // control

            // _Pmpf((__FUNCTION__ ": hwnd 0x%lX", hwnd));

            // check if this is a focusable control
            if (WinQueryClassName(hwnd,
                                  sizeof(szClass),
                                  szClass))
            {
                if (!strcmp(szClass, "#3"))
                    // it's a button: click it
                    WinSendMsg(hwnd, BM_CLICK, (MPARAM)TRUE, 0);
                else if (!strcmp(szClass, "#5"))
                {
                    // it's a static: give focus to following control
                    pNode = pNode->pNext;
                    if (pNode)
                        WinSetFocus(HWND_DESKTOP, (HWND)pNode->pItemData);
                }
            }
            else
                // any other control (are there any?): give them focus
                WinSetFocus(HWND_DESKTOP, hwnd);

            // in any case, stop
            hwndFound = hwnd;
            break;
        }

        pNode = pNode->pNext;
    }

    return (hwndFound);
}

/*
 *@@ dlghEnter:
 *      presses the first button with BS_DEFAULT.
 */

BOOL dlghEnter(PVOID pvllWindows)
{
    PLINKLIST   pllWindows = (PLINKLIST)pvllWindows;

    PLISTNODE pNode = lstQueryFirstNode(pllWindows);
    CHAR szClass[100];
    while (pNode)
    {
        HWND hwnd = (HWND)pNode->pItemData;
        if (WinQueryClassName(hwnd,
                              sizeof(szClass),
                              szClass))
        {
            if (!strcmp(szClass, "#3"))    // button
            {
                // _Pmpf((__FUNCTION__ ": found button"));
                if (    (WinQueryWindowULong(hwnd, QWL_STYLE) & (BS_PUSHBUTTON | BS_DEFAULT))
                     == (BS_PUSHBUTTON | BS_DEFAULT)
                   )
                {
                    // _Pmpf(("   is default!"));
                    WinPostMsg(hwnd,
                               BM_CLICK,
                               (MPARAM)TRUE,        // upclick
                               0);
                    return (TRUE);
                }
            }
        }

        pNode = pNode->pNext;
    }

    return (FALSE);
}


