
/*
 *@@sourcefile cctl_cnr.c:
 *      implementation for the replacement container control.
 *
 *@@header "helpers\comctl.h"
 *@@added V1.0.1 (2003-01-17) [umoeller]
 */

/*
 *      Copyright (C) 2003 Ulrich M”ller.
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

#define INCL_WINWINDOWMGR
#define INCL_WINFRAMEMGR
#define INCL_WINSYS
#define INCL_WINSTDCNR
#define INCL_GPIPRIMITIVES
#define INCL_GPILCIDS
#include <os2.h>

#include "setup.h"                      // code generation and debugging options

#include "helpers\comctl.h"
#include "helpers\linklist.h"
#include "helpers\gpih.h"
#include "helpers\nls.h"
#include "helpers\standards.h"
#include "helpers\stringh.h"
#include "helpers\tree.h"
#include "helpers\winh.h"

#include "private\cnr.h"

#pragma hdrstop

/*
 *@@category: Helpers\PM helpers\Window classes\Container control replacement
 *      See cctl_cnr.c.
 */

/* ******************************************************************
 *
 *   Helper funcs
 *
 ********************************************************************/

/*
 *@@ cdtlRecalcDetails:
 *      worker routine for refreshing all internal DETAILCOLUMN
 *      data when important stuff has changed.
 *
 *      This only gets called from CnrSem2 when WM_SEM2 comes
 *      in with sufficient update flags.
 *
 *      Returns the following ORed flags:
 *
 *      --  DDFL_WINDOWSIZECHANGED: cnr titles area changed,
 *          so details window might need adjustment.
 *
 *      --  DDFL_WORKAREACHANGED: cnr workarea changed, so
 *          scroll bars need adjustment.
 */

VOID cdtlRecalcDetails(PCNRDATA pData,
                       HPS hps,
                       PULONG pfl)
{
    LONG        xCurrent = 0;
    PLISTNODE   pColNode;
    ULONG       cRow = 0;

    // compute total padding for one row:
    LONG        cxPaddingRow = pData->CnrInfo.cFields * 2 * COLUMN_PADDING_X,
                yOfsNow = 0;

    LONG        cyColTitlesContent = 0,
                cyColTitlesBox = 0;

    GpiQueryFontMetrics(hps, sizeof(FONTMETRICS), &pData->fm);

    // outer loop: columns (go RIGHT)
    FOR_ALL_NODES(&pData->llColumns, pColNode)
    {
        PDETAILCOLUMN pCol = (PDETAILCOLUMN)pColNode->pItemData;
        const FIELDINFO *pfi = pCol->pfi;
        PLISTNODE   pRecNode;

        pCol->xLeft = xCurrent;

        pCol->cxTotal
        = pCol->cxWidestRecord
        = 0;

        // inner loop: records (go DOWN)
        FOR_ALL_NODES(&pData->llRootRecords, pRecNode)
        {
            PRECORDLISTITEM prliThis = (PRECORDLISTITEM)pRecNode->pItemData;
            const   RECORDCORE *preccThis = prliThis->precc;
            PVOID   pColumnData = (PVOID)((PBYTE)preccThis + pfi->offStruct);

            SIZEL   szlThis = {10, 10};
            ULONG   cLines;
            PCSZ    pcsz = NULL;
            CHAR    szTemp[30];

            if (!cRow)
                prliThis->szlContent.cy = 0;

            switch (pfi->flData & (   CFA_BITMAPORICON
                                    | CFA_DATE
                                    | CFA_STRING
                                    | CFA_TIME
                                    | CFA_ULONG))
            {
                case CFA_BITMAPORICON:
                    // @@todo
                break;

                case CFA_STRING:
                    pcsz = *(PSZ*)pColumnData;
                break;

                case CFA_DATE:
                    nlsDate(&pData->cs,
                            szTemp,
                            ((PCDATE)pColumnData)->year,
                            ((PCDATE)pColumnData)->month,
                            ((PCDATE)pColumnData)->day);
                    pcsz = szTemp;
                break;

                case CFA_TIME:
                    nlsTime(&pData->cs,
                            szTemp,
                            ((PCTIME)pColumnData)->hours,
                            ((PCTIME)pColumnData)->minutes,
                            ((PCTIME)pColumnData)->seconds);
                    pcsz = szTemp;
                break;

                case CFA_ULONG:
                    nlsThousandsULong(szTemp,
                                      *((PULONG)pColumnData),
                                      pData->cs.cs.cThousands);
                    pcsz = szTemp;
                break;
            }

            if (pcsz)
            {
                gpihCalcTextExtent(hps,
                                   pcsz,
                                   &szlThis.cx,
                                   &cLines);
                szlThis.cy = cLines * (pData->fm.lMaxBaselineExt + pData->fm.lExternalLeading);
            }

            // remember max width of this record's column
            // for the entire column
            if (szlThis.cx > pCol->cxWidestRecord)
                pCol->cxWidestRecord = szlThis.cx;

            // record's content height = height of tallest column of that record
            if (szlThis.cy > prliThis->szlContent.cy)
                prliThis->szlContent.cy = szlThis.cy;

            if (!pColNode->pNext)
            {
                // last column: compute box from content
                prliThis->szlBox.cx =   prliThis->szlContent.cx
                                      + cxPaddingRow;       // computed at top
                prliThis->szlBox.cy =   prliThis->szlContent.cy
                                      + pData->CnrInfo.cyLineSpacing;

                prliThis->yOfs = yOfsNow;

                yOfsNow += prliThis->szlBox.cy;
            }

            ++cRow;
        }

        if (!(pCol->cxTotal = pfi->cxWidth))
        {
            // this is an auto-size column:

            if (pData->CnrInfo.flWindowAttr & CA_DETAILSVIEWTITLES)
            {
                // compute space needed for title

                pCol->szlTitleData.cx
                = pCol->szlTitleData.cy
                = 0;

                if (pfi->flTitle & CFA_BITMAPORICON)
                    // @@todo
                    ;
                else
                {
                    ULONG cLines;
                    gpihCalcTextExtent(hps,
                                       (PCSZ)pfi->pTitleData,
                                       &pCol->szlTitleData.cx,
                                       &cLines);
                    pCol->szlTitleData.cy = cLines * (pData->fm.lMaxBaselineExt + pData->fm.lExternalLeading);
                }

                pCol->cxTotal =   max(pCol->cxWidestRecord, pCol->szlTitleData.cx);

                if (pCol->szlTitleData.cy > cyColTitlesContent)
                    cyColTitlesContent = pCol->szlTitleData.cy;
            }
            else
            {
                pCol->szlTitleData.cx
                = 0;
            }
        }

        // go one column to the right
        xCurrent += pCol->cxTotal + 2 * COLUMN_PADDING_X;

        if (    (pfi->flData & CFA_SEPARATOR)
             && (pColNode->pNext)
           )
            xCurrent += DEFAULT_BORDER_WIDTH;
    }

    if (    (pData->szlWorkarea.cx != xCurrent)
         || (pData->szlWorkarea.cy != yOfsNow)
       )
    {
        pData->szlWorkarea.cx = xCurrent;
        pData->szlWorkarea.cy = yOfsNow;
        *pfl |= DDFL_WORKAREACHANGED;
    }

    if (pData->CnrInfo.flWindowAttr & CA_DETAILSVIEWTITLES)
        cyColTitlesBox =   cyColTitlesContent
                         + 2 * COLUMN_PADDING_Y
                         + pData->CnrInfo.cyLineSpacing;

    if (    (cyColTitlesContent != pData->cyColTitlesContent)
         || (cyColTitlesBox != pData->cyColTitlesBox)
       )
    {
        pData->cyColTitlesContent = cyColTitlesContent;
        pData->cyColTitlesBox = cyColTitlesBox;

        *pfl |= DDFL_WINDOWSIZECHANGED | DDFL_WORKAREACHANGED;
    }
}

/*
 *@@ DetailsCreate:
 *      implementation for WM_CREATE in fnwpCnrDetails.
 *
 *      We receive the CNRDATA as the create param.
 */

MRESULT DetailsCreate(HWND hwnd, MPARAM mp1, MPARAM mp2)
{
    if (!mp1)
        return (MRESULT)TRUE;

    WinSetWindowPtr(hwnd, QWL_USER + 1, mp1);

    // initialize DEFWINDOWDATA
    ctlInitDWD(hwnd,
               mp2,
               &((PCNRDATA)mp1)->dwdContent,
               WinDefWindowProc,
               &G_scsCnr);

    return (MRESULT)FALSE;
}

/*
 *@@ DetailsPaint:
 *      implementation for WM_PAINT in fnwpCnrDetails.
 *
 */

VOID DetailsPaint(HWND hwnd)
{
    HPS         hps;
    PCNRDATA    pData;
    RECTL       rclPaint;

    if (    (pData = (PCNRDATA)WinQueryWindowPtr(hwnd, QWL_USER + 1))
         && (hps = WinBeginPaint(hwnd, NULLHANDLE, &rclPaint))
       )
    {
        PLISTNODE pColNode;

        POINTL  ptl;
        RECTL   rcl;
        LONG    yPadding = pData->CnrInfo.cyLineSpacing / 2;
        ULONG   cColumn = 0;

        gpihSwitchToRGB(hps);
        WinFillRect(hps,
                    &rclPaint,
                    pData->dwdMain.lcolBackground);

        FOR_ALL_NODES(&pData->llColumns, pColNode)
        {
            PDETAILCOLUMN pCol = (PDETAILCOLUMN)pColNode->pItemData;
            const FIELDINFO *pfi = pCol->pfi;
            PLISTNODE   pRecNode;
            ULONG       cRow;

            rcl.xLeft = pCol->xLeft + COLUMN_PADDING_X - pData->ptlScrollOfs.x;
            rcl.xRight = rcl.xLeft + pCol->cxTotal;

            // we start out at the top and work our way down,
            // decrementing rcl.yTop with every item we painted
            rcl.yTop = pData->dwdContent.szlWin.cy + pData->ptlScrollOfs.y;

            PMPF_RECT("rcl: ", &rcl);

            // now paint this column's data for all records!
            cRow = 0;
            FOR_ALL_NODES(&pData->llRootRecords, pRecNode)
            {
                PRECORDLISTITEM prliThis = (PRECORDLISTITEM)pRecNode->pItemData;
                const   RECORDCORE *preccThis = prliThis->precc;
                PVOID   pColumnData = (PVOID)((PBYTE)preccThis + pfi->offStruct);
                CHAR    szTemp[30];
                PCSZ    pcsz = NULL;

                rcl.yTop -= yPadding;
                rcl.yBottom =   rcl.yTop
                              - prliThis->szlContent.cy;

                switch (pfi->flData & (   CFA_BITMAPORICON
                                        | CFA_DATE
                                        | CFA_STRING
                                        | CFA_TIME
                                        | CFA_ULONG))
                {
                    case CFA_BITMAPORICON:
                        // @@todo
                    break;

                    case CFA_STRING:
                        pcsz = *(PSZ*)pColumnData;
                    break;

                    case CFA_DATE:
                        nlsDate(&pData->cs,
                                szTemp,
                                ((PCDATE)pColumnData)->year,
                                ((PCDATE)pColumnData)->month,
                                ((PCDATE)pColumnData)->day);
                        pcsz = szTemp;
                    break;

                    case CFA_TIME:
                        nlsTime(&pData->cs,
                                szTemp,
                                ((PCTIME)pColumnData)->hours,
                                ((PCTIME)pColumnData)->minutes,
                                ((PCTIME)pColumnData)->seconds);
                        pcsz = szTemp;
                    break;

                    case CFA_ULONG:
                        nlsThousandsULong(szTemp,
                                          *((PULONG)pColumnData),
                                          pData->cs.cs.cThousands);
                        pcsz = szTemp;
                    break;
                }

                if (pcsz)
                    cnrDrawString(hps,
                                  pcsz,
                                  &rcl,
                                  pfi->flData,
                                  &pData->fm);

                ++cRow;

                rcl.yTop = rcl.yBottom - yPadding;
            }

            // paint vertical separators after this column?
            if (pfi->flData & CFA_SEPARATOR)
            {
                ptl.x = rcl.xRight + COLUMN_PADDING_X;
                ptl.y = pData->dwdContent.szlWin.cy;
                GpiMove(hps,
                        &ptl);
                ptl.y = 0;
                GpiLine(hps,
                        &ptl);
            }

            ++cColumn;

        } // FOR_ALL_NODES(&pData->llColumns, pColNode)

        WinEndPaint(hps);
    }
}

/* ******************************************************************
 *
 *   Container details window proc
 *
 ********************************************************************/

/*
 *@@ fnwpCnrDetails:
 *
 */

MRESULT EXPENTRY fnwpCnrDetails(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    MRESULT mrc = 0;
    PCNRDATA    pData;

    switch (msg)
    {
        case WM_CREATE:
            mrc = DetailsCreate(hwnd, mp1, mp2);
        break;

        case WM_PAINT:
            DetailsPaint(hwnd);
        break;

        case WM_PRESPARAMCHANGED:
            cnrPresParamChanged(hwnd, (ULONG)mp1);
        break;

        default:
            if (pData = (PCNRDATA)WinQueryWindowPtr(hwnd, QWL_USER + 1))
                mrc = ctlDefWindowProc(&pData->dwdContent, msg, mp1, mp2);
        break;

    }

    return mrc;
}

