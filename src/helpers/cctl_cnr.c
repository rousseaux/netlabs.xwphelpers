
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
#define INCL_WINSCROLLBARS
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
 *   Global variables
 *
 ********************************************************************/

extern const SYSCOLORSET G_scsCnr =
    {
        TRUE,       // inherit presparams
        SYSCLR_WINDOW,
        SYSCLR_WINDOWTEXT
    };

/* ******************************************************************
 *
 *   Helper funcs
 *
 ********************************************************************/

VOID cnrDrawString(HPS hps,
                   PCSZ pcsz,              // in: string to test
                   PRECTL prcl,            // in: clipping rectangle (inclusive!)
                   ULONG fl,               // in: alignment flags
                   PFONTMETRICS pfm)
{
    ULONG fl2 = 0;
/*
   #define CFA_LEFT            0x00000001L
   #define CFA_RIGHT           0x00000002L
   #define CFA_CENTER          0x00000004L
   #define CFA_TOP             0x00000008L
   #define CFA_VCENTER         0x00000010L
   #define CFA_BOTTOM          0x00000020L

 *      --  DT_LEFT                    0x00000000
 *      --  DT_CENTER                  0x00000100
 *      --  DT_RIGHT                   0x00000200
 *      --  DT_TOP                     0x00000000
 *      --  DT_VCENTER                 0x00000400
 *      --  DT_BOTTOM                  0x00000800
*/
    if (fl & CFA_RIGHT)
        fl2 = DT_RIGHT;
    else if (fl & CFA_CENTER)
        fl2 = DT_CENTER;

    if (fl & CFA_BOTTOM)
        fl2 = DT_BOTTOM;
    else if (fl & CFA_VCENTER)
        fl2 = DT_VCENTER;

    gpihDrawString(hps,
                   pcsz,
                   prcl,
                   fl2,
                   pfm);
}

/*
 *@@ CreateChild:
 *      creates a container child window.
 */

HWND CreateChild(PCNRDATA pData,
                 PCSZ pcszClass,
                 ULONG id)
{
    return WinCreateWindow(pData->dwdMain.hwnd,
                           (PSZ)pcszClass,
                           NULL,
                           WS_VISIBLE,
                           0,
                           0,
                           0,
                           0,
                           pData->dwdMain.hwnd,
                           HWND_TOP,
                           id,
                           pData,
                           NULL);
}

/*
 *@@ FindColumnFromFI:
 *
 */

PDETAILCOLUMN FindColumnFromFI(PCNRDATA pData,
                               const FIELDINFO *pfi,
                               PLISTNODE *ppNode)       // out: listnode of column
{
    PLISTNODE pNode;
    FOR_ALL_NODES(&pData->llColumns, pNode)
    {
        PDETAILCOLUMN pCol = pNode->pItemData;
        if (pCol->pfi == pfi)
        {
            if (ppNode)
                *ppNode = pNode;

            return pCol;
        }
    }

    return NULL;
}

/*
 *@@ FindListNodeForRecc:
 *
 */

PLISTNODE FindListNodeForRecc(PCNRDATA pData,
                              const RECORDCORE *precc)
{
    RECORDTREEITEM  *pti;
    if (pti = (PRECORDTREEITEM)treeFind(pData->RecordsTree,
                                        (ULONG)precc,
                                        treeCompareKeys))
        return pti->pListNode;

    return NULL;
}

/* ******************************************************************
 *
 *   Standard window messages
 *
 ********************************************************************/

/*
 *@@ CnrCreate:
 *      implementation for WM_CREATE in fnwpCnr.
 */

MRESULT CnrCreate(HWND hwnd, MPARAM mp1, MPARAM mp2)
{
    PCNRDATA    pData;

    if (!(pData = NEW(CNRDATA)))
        return (MRESULT)TRUE;       // stop window creation

    WinSetWindowPtr(hwnd, QWL_USER + 1, pData);
    ZERO(pData);

    // initialize DEFWINDOWDATA
    ctlInitDWD(hwnd,
               mp2,
               &pData->dwdMain,
               WinDefWindowProc,
               &G_scsCnr);

    if (winhQueryWindowStyle(hwnd) & CCS_MINIRECORDCORE)
        pData->fMiniRecords = TRUE;

    // set up non-zero default values in cnrinfo
    pData->CnrInfo.cb = sizeof(CNRINFO);
    pData->CnrInfo.xVertSplitbar = -1;

    lstInit(&pData->llAllocatedFIs,
            TRUE);
    lstInit(&pData->llColumns,
            TRUE);
    lstInit(&pData->llAllocatedRecs,
            TRUE);
    lstInit(&pData->llRootRecords,
            TRUE);

    treeInit(&pData->RecordsTree,
             (PLONG)&pData->CnrInfo.cRecords);

    nlsQueryCountrySettings(&pData->cs);

    winhCreateScrollBars(hwnd,
                         &pData->hwndVScroll,
                         &pData->hwndHScroll);

    pData->cxScrollBar = WinQuerySysValue(HWND_DESKTOP, SV_CXVSCROLL);
    pData->cyScrollBar = WinQuerySysValue(HWND_DESKTOP, SV_CYHSCROLL);

    return (MRESULT)FALSE;
}

/*
 *@@ CnrSem2:
 *      implementation for WM_SEM2 in fnwpCnr.
 *
 *      The DDFL_* semaphore bits get set whenever the
 *      view needs to recompute something. In the worst
 *      case, we resize and/or invalidate the window.
 */

VOID CnrSem2(HWND hwnd, ULONG fl)
{
    PCNRDATA    pData;
    if (pData = (PCNRDATA)WinQueryWindowPtr(hwnd, QWL_USER + 1))
    {
        if (pData->CnrInfo.flWindowAttr & CV_DETAIL)
        {
            if (fl & (DDFL_INVALIDATECOLUMNS | DDFL_INCALIDATERECORDS))
            {
                HPS hps = WinGetPS(hwnd);
                cdtlRecalcDetails(pData, hps, &fl);
                WinReleasePS(hps);

                if (fl & DDFL_WINDOWSIZECHANGED)
                    WinInvalidateRect(pData->dwdMain.hwnd, NULL, TRUE);
            }

            if (fl & (DDFL_WINDOWSIZECHANGED | DDFL_WORKAREACHANGED))
            {
                LONG    y = 0,
                        cx = pData->dwdMain.szlWin.cx,
                        cy = pData->dwdMain.szlWin.cy - pData->cyColTitlesBox;

                if (pData->hwndVScroll)
                    cx -= pData->cxScrollBar;
                if (pData->hwndHScroll)
                {
                    y += pData->cyScrollBar;
                    cy -= pData->cyScrollBar;
                }

                _PmpfF(("cyColTitlesBox %d, new cy: %d", pData->cyColTitlesBox, cy));

                if (fl & DDFL_WINDOWSIZECHANGED)
                    WinSetWindowPos(pData->hwndDetails,
                                    HWND_TOP,
                                    0,
                                    y,
                                    cx,
                                    cy,
                                    SWP_MOVE | SWP_SIZE);
                                            // SWP_MOVE is required or PM will move our
                                            // subwindow to some adjustment position

                if (pData->hwndVScroll)
                {
                    WinSetWindowPos(pData->hwndVScroll,
                                    HWND_TOP,
                                    cx,
                                    y,
                                    pData->cxScrollBar,
                                    cy,
                                    SWP_MOVE | SWP_SIZE);

                    _PmpfF(("updating VScroll, cy: %d, szlWorkarea.cy: %d",
                            cy,
                            pData->szlWorkarea.cy));

                    winhUpdateScrollBar(pData->hwndVScroll,
                                        cy,
                                        pData->szlWorkarea.cy,
                                        pData->ptlScrollOfs.y,
                                        FALSE);
                }

                if (pData->hwndHScroll)
                {
                    WinSetWindowPos(pData->hwndHScroll,
                                    HWND_TOP,
                                    0,
                                    0,
                                    cx,
                                    pData->cyScrollBar,
                                    SWP_MOVE | SWP_SIZE);

                    _PmpfF(("updating HScroll, cx: %d, szlWorkarea.cx: %d",
                            cx,
                            pData->szlWorkarea.cx));

                    winhUpdateScrollBar(pData->hwndHScroll,
                                        cx,
                                        pData->szlWorkarea.cx,
                                        pData->ptlScrollOfs.x,
                                        FALSE);
                }
            }
        }
    }
}

/*
 *@@ CnrPaint:
 *      implementation for WM_PAINT in fnwpCnr.
 */

VOID CnrPaint(HWND hwnd)
{
    HPS         hps;
    PCNRDATA    pData;
    RECTL       rclPaint;

    if (    (pData = (PCNRDATA)WinQueryWindowPtr(hwnd, QWL_USER + 1))
         && (hps = WinBeginPaint(hwnd, NULLHANDLE, &rclPaint))
       )
    {
        gpihSwitchToRGB(hps);

        if (    (pData->CnrInfo.flWindowAttr & (CV_DETAIL | CA_DETAILSVIEWTITLES))
             == (CV_DETAIL | CA_DETAILSVIEWTITLES)
           )
        {
            PLISTNODE pColNode;

            POINTL  ptl;
            RECTL   rcl;
            LONG    yPadding = pData->CnrInfo.cyLineSpacing / 2;

            // lowest y that we are allowed to paint at
            LONG    yLowest = pData->dwdMain.szlWin.cy - pData->cyColTitlesBox;

            if (rclPaint.yTop > yLowest)
            {
                // clip paint rect so we don't paint into details wnd
                if (rclPaint.yBottom < yLowest)
                    rclPaint.yBottom = yLowest;

                WinFillRect(hps,
                            &rclPaint,
                            pData->dwdMain.lcolBackground);

                FOR_ALL_NODES(&pData->llColumns, pColNode)
                {
                    RECTL       rcl2;
                    PDETAILCOLUMN pCol = (PDETAILCOLUMN)pColNode->pItemData;
                    const FIELDINFO *pfi = pCol->pfi;
                    PLISTNODE   pRecNode;
                    ULONG       cRow;

                    rcl.xLeft = pCol->xLeft + COLUMN_PADDING_X - pData->ptlScrollOfs.x;
                    rcl.xRight = rcl.xLeft + pCol->cxTotal;

                    // we start out at the top and work our way down,
                    // decrementing rcl.yTop with every item we painted
                    rcl.yTop = pData->dwdMain.szlWin.cy;

                    rcl.yTop -= COLUMN_PADDING_Y + yPadding;
                    rcl.yBottom =   rcl.yTop
                                  - pData->cyColTitlesContent;

                    if (pfi->flTitle & CFA_BITMAPORICON)
                        // @@todo
                        ;
                    else
                    {
                        cnrDrawString(hps,
                                      (PCSZ)pfi->pTitleData,
                                      &rcl,
                                      pfi->flTitle,
                                      &pData->fm);
                    }

                    rcl.yBottom -= COLUMN_PADDING_Y + yPadding;

                    if (pfi->flData & CFA_HORZSEPARATOR)
                    {
                        ptl.x = rcl.xLeft;
                        ptl.y = rcl.yBottom;

                        GpiMove(hps,
                                &ptl);
                        ptl.x = rcl.xRight;
                        GpiLine(hps,
                                &ptl);
                    }

                    rcl.yTop = rcl.yBottom;

                } // FOR_ALL_NODES(&pData->llColumns, pColNode)
            }
        }

        WinEndPaint(hps);
    }
}

/*
 *@@ CnrWindowPosChanged:
 *      implementation for WM_WINDOWPOSCHANGED in fnwpCnr.
 */

MRESULT CnrWindowPosChanged(HWND hwnd,
                            MPARAM mp1,
                            MPARAM mp2)
{
    MRESULT mrc = 0;
    PCNRDATA pData;
    if (pData = (PCNRDATA)WinQueryWindowPtr(hwnd, QWL_USER + 1))
    {
        mrc = ctlDefWindowProc(&pData->dwdMain, WM_WINDOWPOSCHANGED, mp1, mp2);

        if (((PSWP)mp1)->fl & SWP_SIZE)
        {
            WinPostMsg(hwnd,
                       WM_SEM2,
                       (MPARAM)DDFL_WINDOWSIZECHANGED,
                       0);
        }
    }

    return mrc;
}

/*
 *@@ cnrPresParamChanged:
 *      implementation for WM_PRESPARAMCHANGED for both
 *      fnwpCnr and fnwpCnrDetails.
 */

VOID cnrPresParamChanged(HWND hwnd,
                         ULONG ulpp)
{
    PCNRDATA pData;
    if (pData = (PCNRDATA)WinQueryWindowPtr(hwnd, QWL_USER + 1))
    {
        // note, we use the dwdMain buffer here cos
        // we share presparams with the cnr
        ctlRefreshColors(&pData->dwdMain);

        switch (ulpp)
        {
            case 0:     // layout palette thing dropped
            case PP_FONTNAMESIZE:
                if (pData->CnrInfo.flWindowAttr & CV_DETAIL)
                {
                    // if we got this on the details window,
                    // set it on the main cnr as well, and
                    // vice versa
                    PSZ pszFont;
                    if (pszFont = winhQueryWindowFont(hwnd))
                    {
                        HWND hwndOther;
                        DosBeep(1000, 10);
                        if (hwnd == pData->dwdMain.hwnd)
                            hwndOther = pData->dwdContent.hwnd;
                        else
                            hwndOther = pData->dwdMain.hwnd;

                        winhSetWindowFont(hwndOther, pszFont);
                        free(pszFont);
                    }

                    WinPostMsg(pData->dwdMain.hwnd,
                               WM_SEM2,
                               (MPARAM)(DDFL_INVALIDATECOLUMNS | DDFL_INCALIDATERECORDS),
                               0);
                }
            break;

            default:
                // just repaint everything
                WinInvalidateRect(pData->dwdMain.hwnd, NULL, TRUE);
        }
    }
}

/*
 *@@ CnrScroll:
 *      implementation for WM_HSCROLL and WM_VSCROLL in fnwpCnr.
 */

VOID CnrScroll(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    PCNRDATA pData;
    if (pData = (PCNRDATA)WinQueryWindowPtr(hwnd, QWL_USER + 1))
    {
        if (pData->CnrInfo.flWindowAttr & CV_DETAIL)
        {
            POINTL  ptlScroll;
            if (msg == WM_HSCROLL)
            {
                ptlScroll.y = 0;
                if (ptlScroll.x = winhHandleScrollMsg2(pData->hwndHScroll,
                                                       &pData->ptlScrollOfs.x,
                                                       pData->dwdContent.szlWin.cx,
                                                       pData->szlWorkarea.cx,
                                                       5,
                                                       msg,
                                                       mp2))
                {
                    if (pData->CnrInfo.flWindowAttr & CA_DETAILSVIEWTITLES)
                    {
                        RECTL rclClip;
                        rclClip.xLeft = 0;
                        rclClip.xRight = pData->dwdMain.szlWin.cx;
                        rclClip.yBottom = pData->dwdMain.szlWin.cy - pData->cyColTitlesBox;
                        rclClip.yTop = pData->dwdMain.szlWin.cy;
                        winhScrollWindow(hwnd,
                                         &rclClip,
                                         &ptlScroll);
                    }
                    winhScrollWindow(pData->hwndDetails,
                                     NULL,
                                     &ptlScroll);
                }
            }
            else
            {
                ptlScroll.x = 0;
                if (ptlScroll.y = winhHandleScrollMsg2(pData->hwndVScroll,
                                                       &pData->ptlScrollOfs.y,
                                                       pData->dwdContent.szlWin.cy,
                                                       pData->szlWorkarea.cy,
                                                       5,
                                                       msg,
                                                       mp2))
                    winhScrollWindow(pData->hwndDetails,
                                     NULL,
                                     &ptlScroll);
            }
        }
    }
}

/*
 *@@ CnrDestroy:
 *      implementation for WM_DESTROY in fnwpCnr.
 */

VOID CnrDestroy(HWND hwnd)
{
    PCNRDATA pData;
    if (pData = (PCNRDATA)WinQueryWindowPtr(hwnd, QWL_USER + 1))
    {
        // free all data that we ever allocated;
        // all these lists are auto-free
        lstClear(&pData->llColumns);
        lstClear(&pData->llAllocatedFIs);
        lstClear(&pData->llAllocatedRecs);
        lstClear(&pData->llRootRecords);

        free(pData);
    }
}

/* ******************************************************************
 *
 *   General container messages
 *
 ********************************************************************/

/*
 *@@ CnrQueryCnrInfo:
 *      implementation for CM_QUERYCNRINFO in fnwpCnr.
 *
 *      Returns no. of bytes copied.
 */

USHORT CnrQueryCnrInfo(HWND hwnd,
                       PCNRINFO pci,        // in: mp1 of CM_QUERYCNRINFO
                       USHORT cb)           // in: mp2 of CM_QUERYCNRINFO
{
    PCNRDATA pData;
    if (pData = (PCNRDATA)WinQueryWindowPtr(hwnd, QWL_USER + 1))
    {
        USHORT cbCopied = max(cb, sizeof(CNRINFO));
        memcpy(pci,
               &pData->CnrInfo,
               cbCopied);
        return cbCopied;
    }

    return 0;
}

/*
 *@@ CnrSetCnrInfo:
 *      implementation for CM_SETCNRINFO in fnwpCnr.
 *
 *      Returns no. of bytes copied.
 */

BOOL CnrSetCnrInfo(HWND hwnd,
                   PCNRINFO pci,        // in: mp1 of CM_SETCNRINFO
                   ULONG flCI)          // in: CMA_* flags in mp2 of CM_SETCNRINFO
{
    PCNRDATA pData;
    if (pData = (PCNRDATA)WinQueryWindowPtr(hwnd, QWL_USER + 1))
    {
        ULONG   flDirty = 0;

        if (flCI & CMA_PSORTRECORD)
            /* Pointer to the comparison function for sorting container records. If NULL,
            which is the default condition, no sorting is performed. Sorting only occurs
            during record insertion and when changing the value of this field. The third
            parameter of the comparison function, pStorage, must be NULL. See
            CM_SORTRECORD for a further description of the comparison function. */
            pData->CnrInfo.pSortRecord = pci->pSortRecord;

        if (flCI & CMA_PFIELDINFOLAST)
            /* Pointer to the last column in the left window of the split details view. The
            default is NULL, causing all columns to be positioned in the left window. */
            pData->CnrInfo.pFieldInfoLast = pci->pFieldInfoLast;

        if (flCI & CMA_PFIELDINFOOBJECT)
            /* Pointer to a column that represents an object in the details view. This
            FIELDINFO structure must contain icons or bit maps. In-use emphasis is applied
            to this column of icons or bit maps only. The default is the leftmost column
            in the unsplit details view, or the leftmost column in the left window of the
            split details view. */
            pData->CnrInfo.pFieldInfoObject = pci->pFieldInfoObject;

        if (flCI & CMA_CNRTITLE)
        {
            // Text for the container title. The default is NULL.
            pData->CnrInfo.pszCnrTitle = pci->pszCnrTitle;

            // @@todo recalc window components, repaint
        }

        if (flCI & CMA_FLWINDOWATTR)
        {
            // Container window attributes.
            if (pData->CnrInfo.flWindowAttr != pci->flWindowAttr)
            {
                HWND    hwndSwitchOwner = NULLHANDLE;

                pData->CnrInfo.flWindowAttr = pci->flWindowAttr;

                // if switching to details view, then create
                // details subwindow
                if (pData->CnrInfo.flWindowAttr & CV_DETAIL)
                {
                    if (!pData->hwndDetails)
                    {
                        if (pData->hwndDetails = CreateChild(pData,
                                                             WC_CCTL_CNR_DETAILS,
                                                             CID_LEFTDVWND))
                        {
                            flDirty = DDFL_ALL;
                        }
                    }
                }
                else
                {
                    winhDestroyWindow(&pData->hwndDetails);
                }
            }
        }

        if (flCI & CMA_PTLORIGIN)
        {
            // Lower-left origin of the container window in virtual workspace coordinates,
            // used in the icon view. The default origin is (0,0).
            memcpy(&pData->CnrInfo.ptlOrigin,
                   &pci->ptlOrigin,
                   sizeof(POINTL));

            // @@todo recalc window components, repaint
        }

        if (flCI & CMA_DELTA)
        {
            // An application-defined threshold, or number of records, from either end of
            // the list of available records. Used when a container needs to handle large
            // amounts of data. The default is 0. Refer to the description of the container
            // control in the OS/2 Programming Guide for more information about specifying
            // deltas.
            pData->CnrInfo.cDelta = pci->cDelta;
        }

        if (flCI & CMA_SLBITMAPORICON)
        {
            // The size (in pels) of icons or bit maps. The default is the system size.
            memcpy(&pData->CnrInfo.slBitmapOrIcon,
                   &pci->slBitmapOrIcon,
                   sizeof(SIZEL));

            // @@todo recalc window components, repaint
        }

        if (flCI & CMA_SLTREEBITMAPORICON)
        {
            // The size (in pels) of the expanded and collapsed icons or bit maps in the
            // tree icon and tree text views.
            memcpy(&pData->CnrInfo.slTreeBitmapOrIcon,
                   &pci->slTreeBitmapOrIcon,
                   sizeof(SIZEL));

            // @@todo recalc window components, repaint
        }

        if (flCI & CMA_TREEBITMAP)
        {
            // Expanded and collapsed bit maps in the tree icon and tree text views.
            pData->CnrInfo.hbmExpanded = pci->hbmExpanded;
            pData->CnrInfo.hbmCollapsed = pci->hbmCollapsed;

            // @@todo recalc window components, repaint
        }

        if (flCI & CMA_TREEICON)
        {
            // Expanded and collapsed icons in the tree icon and tree text views.
            pData->CnrInfo.hptrExpanded = pci->hptrExpanded;
            pData->CnrInfo.hptrCollapsed = pci->hptrCollapsed;

            // @@todo recalc window components, repaint
        }

        if (flCI & CMA_LINESPACING)
        {
            // The amount of vertical space (in pels) between the records. If this value is
            // less than 0, a default value is used.
            pData->CnrInfo.cyLineSpacing = pci->cyLineSpacing;

            // @@todo recalc window components, repaint
        }

        if (flCI & CMA_CXTREEINDENT)
        {
            // Horizontal distance (in pels) between levels in the tree view. If this value is
            // less than 0, a default value is used.
            pData->CnrInfo.cxTreeIndent = pci->cxTreeIndent;

            // @@todo recalc window components, repaint
        }

        if (flCI & CMA_CXTREELINE)
        {
            // Width of the lines (in pels) that show the relationship between items in the
            // tree view. If this value is less than 0, a default value is used. Also, if the
            // CA_TREELINE container attribute of the CNRINFO data structure's
            // flWindowAttr field is not specified, these lines are not drawn.
            pData->CnrInfo.cxTreeLine = pci->cxTreeLine;
        }

        if (flCI & CMA_XVERTSPLITBAR)
        {
            // The initial position of the split bar relative to the container, used in the
            // details view. If this value is less than 0, the split bar is not used. The default
            // value is negative one (-1).
            pData->CnrInfo.xVertSplitbar = pci->xVertSplitbar;
        }

        if (flDirty)
            // post semaphore to force resize of details wnd
            WinPostMsg(hwnd,
                       WM_SEM2,
                       (MPARAM)flDirty,
                       0);

        return TRUE;
    }

    return FALSE;
}

/* ******************************************************************
 *
 *   FIELDINFO-related messages
 *
 ********************************************************************/

/*
 *@@ CnrAllocDetailFieldInfo:
 *      implementation for CM_ALLOCDETAILFIELDINFO in fnwpCnr.
 *
 *      Returns: PFIELDINFO linked list or NULL.
 */

PFIELDINFO CnrAllocDetailFieldInfo(HWND hwnd,
                                   USHORT cFieldInfos)      // in: no. of fieldinfos to allocate (> 0)
{
    PFIELDINFO  pfiFirst = NULL,
                pfiPrev = NULL;

    PCNRDATA    pData;
    if (pData = (PCNRDATA)WinQueryWindowPtr(hwnd, QWL_USER + 1))
    {
        ULONG       ul;
        for (ul = 0;
             ul < cFieldInfos;
             ++ul)
        {
            // we must allocate each one separately or we cannot
            // free them separately with CM_FREEDETAILFIELDINFO
            PFIELDINFO  pfiThis;
            if (pfiThis = NEW(FIELDINFO))
            {
                ZERO(pfiThis);

                // link into list
                if (!pfiPrev)
                    // first round:
                    pfiFirst = pfiThis;
                else
                    pfiPrev->pNextFieldInfo = pfiThis;

                // put into private linklist
                lstAppendItem(&pData->llAllocatedFIs,
                              pfiThis);

                pfiPrev = pfiThis;
            }
        }
    }

    return pfiFirst;
}

/*
 *@@ CnrInsertDetailFieldInfo:
 *      implementation for CM_INSERTDETAILFIELDINFO in fnwpCnr.
 */

USHORT CnrInsertDetailFieldInfo(HWND hwnd,
                                PFIELDINFO pfiFirst,
                                PFIELDINFOINSERT pfii)
{
    USHORT      usrc = 0;
    PCNRDATA    pData;
    if (    (pData = (PCNRDATA)WinQueryWindowPtr(hwnd, QWL_USER + 1))
         && (pfiFirst)
         && (pfii)
         && (pfii->cb = sizeof(FIELDINFOINSERT))
       )
    {
        ULONG       ul;
        PFIELDINFO  pfiThis = pfiFirst;
        PLISTNODE   pNodeInsertAfter;

        usrc = lstCountItems(&pData->llColumns);

        switch ((ULONG)pfii->pFieldInfoOrder)
        {
            case CMA_FIRST:
                pNodeInsertAfter = NULL;        // first
            break;

            case CMA_END:
                pNodeInsertAfter = lstQueryLastNode(&pData->llColumns);
                                            // can return NULL also
            break;

            default:
                if (!FindColumnFromFI(pData,
                                      pfii->pFieldInfoOrder,
                                      &pNodeInsertAfter))
                    pNodeInsertAfter = NULL;
        }

        for (ul = 0;
             ul < pfii->cFieldInfoInsert;
             ++ul)
        {
            PDETAILCOLUMN pdc;
            if (pdc = NEW(DETAILCOLUMN))
            {
                ZERO(pdc);

                pdc->pfi = pfiThis;

                if (pNodeInsertAfter = lstInsertItemAfterNode(&pData->llColumns,
                                                              pdc,
                                                              pNodeInsertAfter))
                {
                    ++usrc;
                    ++pData->CnrInfo.cFields;
                    pfiThis = pfiThis->pNextFieldInfo;
                    continue;
                }
            }

            free(pdc);

            usrc = 0;
            break;

        } // for (ul = 0; ul < pfii->cFieldInfoInsert; ...

        if (    (usrc)
             && (pfii->fInvalidateFieldInfo)
           )
        {
            // post semaphore to force resize of details wnd
            WinPostMsg(hwnd,
                       WM_SEM2,
                       (MPARAM)DDFL_ALL,
                       0);
        }
    }

    return usrc;
}

/*
 *@@ CnrInvalidateDetailFieldInfo:
 *      implementation for CM_INVALIDATEDETAILFIELDINFO in fnwpCnr.
 */

BOOL CnrInvalidateDetailFieldInfo(HWND hwnd)
{
    PCNRDATA    pData;
    if (pData = (PCNRDATA)WinQueryWindowPtr(hwnd, QWL_USER + 1))
    {
        WinPostMsg(hwnd,
                   WM_SEM2,
                   (MPARAM)DDFL_INVALIDATECOLUMNS,
                   0);
    }

    return FALSE;
}

/*
 *@@ CnrRemoveDetailFieldInfo:
 *      implementation for CM_REMOVEDETAILFIELDINFO in fnwpCnr.
 */

SHORT CnrRemoveDetailFieldInfo(HWND hwnd,
                               PFIELDINFO* ppafi,
                               USHORT cfi,
                               USHORT fl)
{
    PCNRDATA    pData;
    if (pData = (PCNRDATA)WinQueryWindowPtr(hwnd, QWL_USER + 1))
    {
        SHORT   rc = lstCountItems(&pData->llColumns);
        ULONG   fAnythingFound = FALSE,
                ul;

        for (ul = 0;
             ul < cfi;
             ++ul)
        {
            PDETAILCOLUMN pCol;
            PLISTNODE pNodeCol;
            if (!(pCol = FindColumnFromFI(pData,
                                          ppafi[ul],
                                          &pNodeCol)))
            {
                rc = -1;
                break;
            }

            // found:
            lstRemoveNode(&pData->llColumns,
                          pNodeCol);            // auto-free, so this frees the DETAILCOLUMN

            if (fl & CMA_FREE)
                lstRemoveItem(&pData->llAllocatedFIs,
                              ppafi[ul]);       // auto-free, so this frees the FIELDINFO

            fAnythingFound = TRUE;

            --rc;
            --pData->CnrInfo.cFields;
        }

        if (    (fAnythingFound)
             && (fl & CMA_INVALIDATE)
           )
        {
            WinPostMsg(hwnd,
                       WM_SEM2,
                       (MPARAM)DDFL_INVALIDATECOLUMNS,
                       0);
        }

        return rc;
    }

    return -1;
}

/*
 *@@ CnrFreeDetailFieldInfo:
 *      implementation for CM_FREEDETAILFIELDINFO in fnwpCnr.
 */

BOOL CnrFreeDetailFieldInfo(HWND hwnd,
                            PFIELDINFO *ppafi,      // in: mp1 of CM_FREEDETAILFIELDINFO
                            USHORT cFieldInfos)     // in: no. of items in array
{
    BOOL        brc = FALSE;

    PCNRDATA    pData;
    if (pData = (PCNRDATA)WinQueryWindowPtr(hwnd, QWL_USER + 1))
    {
        ULONG   ul;

        // @@todo return FALSE if the FI is currently inserted

        if (1)
        {
            for (ul = 0;
                 ul < cFieldInfos;
                 ++ul)
            {
                PFIELDINFO pfiThis = ppafi[ul];
                lstRemoveItem(&pData->llAllocatedFIs,
                              pfiThis);
            }

            brc = TRUE;
        }
    }

    return brc;
}

/* ******************************************************************
 *
 *   Record insertion/removal
 *
 ********************************************************************/

/*
 *@@ CnrAllocRecord:
 *      implementation for CM_ALLOCRECORD in fnwpCnr.
 */

PRECORDCORE CnrAllocRecord(HWND hwnd,
                           ULONG cbExtra,
                           USHORT cRecords)
{
    PRECORDCORE preccFirst = NULL;
    PCNRDATA    pData;
    if (pData = (PCNRDATA)WinQueryWindowPtr(hwnd, QWL_USER + 1))
    {
        ULONG   ul;
        ULONG   cbAlloc = (   (pData->fMiniRecords)
                            ? sizeof(MINIRECORDCORE)
                            : sizeof(RECORDCORE)
                          ) + cbExtra;

        PRECORDCORE preccPrev = NULL;

        for (ul = 0;
             ul < cRecords;
             ++ul)
        {
            PRECORDCORE preccThis;
            if (!(preccThis = (PRECORDCORE)malloc(cbAlloc)))
            {
                preccFirst = NULL;
                break;
            }

            memset(preccThis, 0, cbAlloc);

            preccThis->cb = cbAlloc;

            // link into list
            if (!preccPrev)
                // first round:
                preccFirst = preccThis;
            else
                preccPrev->preccNextRecord = preccThis;

            // put into private linklist
            lstAppendItem(&pData->llAllocatedRecs,
                          preccThis);

            preccPrev = preccThis;
        }
    }

    return preccFirst;
}

/*
 *@@ CnrInsertRecord:
 *      implementation for CM_INSERTRECORD in fnwpCnr.
 */

ULONG CnrInsertRecord(HWND hwnd,
                      PRECORDCORE preccFirst,
                      PRECORDINSERT pri)
{
    ULONG       cReturn = 0;
    PCNRDATA    pData;
    if (    (pData = (PCNRDATA)WinQueryWindowPtr(hwnd, QWL_USER + 1))
         && (preccFirst)
         && (pri)
         && (pri->cb = sizeof(RECORDINSERT))
       )
    {
        PRECORDCORE preccThis = preccFirst;
        ULONG ul;
        PLINKLIST   pll;
        PLISTNODE   pNodeInsertAfter;

        cReturn = lstCountItems(&pData->llRootRecords);

        if (pri->pRecordParent)
        {
            // @@todo
        }
        else
            // insert at root:
            pll = &pData->llRootRecords;

        switch ((ULONG)pri->pRecordOrder)
        {
            case CMA_FIRST:
                pNodeInsertAfter = NULL;        // first
            break;

            case CMA_END:
                pNodeInsertAfter = lstQueryLastNode(pll);
                                            // can return NULL also
            break;

            default:
                pNodeInsertAfter = FindListNodeForRecc(pData,
                                                       pri->pRecordOrder);
        }

        for (ul = 0;
             ul < pri->cRecordsInsert;
             ++ul)
        {
            PRECORDLISTITEM pListItem;

            if (pListItem = NEW(RECORDLISTITEM))
            {
                ZERO(pListItem);

                pListItem->precc = preccThis;
                pListItem->preccParent = pri->pRecordParent;

                if (pNodeInsertAfter = lstInsertItemAfterNode(pll,
                                                              pListItem,
                                                              pNodeInsertAfter))
                {
                    PRECORDTREEITEM pTreeItem;

                    if (pTreeItem = NEW(RECORDTREEITEM))
                    {
                        ZERO(pTreeItem);

                        pTreeItem->Tree.ulKey = (ULONG)preccThis;
                        pTreeItem->pListNode = pNodeInsertAfter;    // newly created list node

                        // the following will fail if the record
                        // is already inserted!
                        if (!treeInsert(&pData->RecordsTree,
                                        (PLONG)&pData->CnrInfo.cRecords,
                                        (TREE*)pTreeItem,
                                        treeCompareKeys))
                        {
                            ++cReturn;
                            preccThis = preccThis->preccNextRecord;
                            continue;
                        }

                        free(pTreeItem);
                    }

                    lstRemoveNode(pll,
                                  pNodeInsertAfter);
                }

                free(pListItem);
            }

            free(pListItem);

            cReturn = 0;
            break;      // for
        } // for (ul = 0; ul < pri->cRecordsInsert; ...

        if (    (cReturn)
             && (pri->fInvalidateRecord)
           )
        {
            WinPostMsg(hwnd,
                       WM_SEM2,
                       (MPARAM)DDFL_INCALIDATERECORDS,
                       0);
        }
    }

    return cReturn;
}

/* ******************************************************************
 *
 *   Container window proc
 *
 ********************************************************************/

/*
 *@@ fnwpCnr:
 *
 */

MRESULT EXPENTRY fnwpCnr(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    MRESULT     mrc = 0;
    PCNRDATA    pData;

    switch (msg)
    {
        /* ******************************************************************
         *
         *   Standard window messages
         *
         ********************************************************************/

        case WM_CREATE:
            mrc = CnrCreate(hwnd, mp1, mp2);
        break;

        case WM_SEM2:
            CnrSem2(hwnd, (ULONG)mp1);
        break;

        case WM_PAINT:
            CnrPaint(hwnd);
        break;

        case WM_WINDOWPOSCHANGED:
            mrc = CnrWindowPosChanged(hwnd, mp1, mp2);
        break;

        case WM_HSCROLL:
        case WM_VSCROLL:
            CnrScroll(hwnd, msg, mp1, mp2);
        break;

        case WM_DESTROY:
            CnrDestroy(hwnd);
        break;

        /* ******************************************************************
         *
         *   General container messages
         *
         ********************************************************************/

        /*
         * CM_QUERYCNRINFO:
         *
         *      Parameters:
         *
         *      --  PCNRINFO mp1: buffer to copy to.
         *      --  USHORT mp2: size of buffer.
         *
         *      Returns no. of bytes copied or 0 on errors.
         */

        case CM_QUERYCNRINFO:
            mrc = (MRESULT)CnrQueryCnrInfo(hwnd,
                                           (PCNRINFO)mp1,
                                           (USHORT)mp2);
        break;

        /*
         * CM_SETCNRINFO:
         *
         *      Parameters:
         *
         *      --  PCNRINFO mp1: buffer to copy fields from.
         *
         *      --  ULONG fl: CMA_* flags for fields that changed.
         *
         *      Returns BOOL.
         */

        case CM_SETCNRINFO:
            mrc = (MRESULT)CnrSetCnrInfo(hwnd,
                                         (PCNRINFO)mp1,
                                         (ULONG)mp2);
        break;

        /* ******************************************************************
         *
         *   FIELDINFO-related messages
         *
         ********************************************************************/

        /*
         * CM_ALLOCDETAILFIELDINFO:
         *
         *      Parameters:
         *
         *      --  USHORT mp1: no. of fieldinfos to allocate
         *      --  mp2: reserved
         *
         *      Returns PFIELDINFO linked list of fieldinfos,
         *      or NULL on errors.
         */

        case CM_ALLOCDETAILFIELDINFO:
            mrc = (MRESULT)CnrAllocDetailFieldInfo(hwnd,
                                                   (USHORT)mp1);
        break;

        /*
         * CM_INSERTDETAILFIELDINFO:
         *
         *      Parameters:
         *
         *      --  PFIELDINFO mp1
         *
         *      --  PFIELDINFOINSERT mp2
         *
         *      Returns the no. of FI's in the cnr or 0 on errors.
         */

        case CM_INSERTDETAILFIELDINFO:
            mrc = (MRESULT)CnrInsertDetailFieldInfo(hwnd,
                                                    (PFIELDINFO)mp1,
                                                    (PFIELDINFOINSERT)mp2);
        break;

        /*
         * CM_INVALIDATEDETAILFIELDINFO:
         *      No parameters.
         *
         *      Returns BOOL.
         */

        case CM_INVALIDATEDETAILFIELDINFO:
            mrc = (MRESULT)CnrInvalidateDetailFieldInfo(hwnd);
        break;

        /*
         * CM_REMOVEDETAILFIELDINFO:
         *
         *      Parameters:
         *
         *      --  PFIELDINFO* mp1: ptr to array of PFIELDINFO's
         *
         *      --  SHORT1FROMMP(mp1): no. of fieldinfos in array
         *
         *      --  SHORT2FROMMP(mp2): flRemove (CMA_FREE, CMA_INVALIDATE)
         *
         *      Returns the no. of FI's in the cnr or -1 on errors.
         */

        case CM_REMOVEDETAILFIELDINFO:
            mrc = (MRESULT)CnrRemoveDetailFieldInfo(hwnd,
                                                    (PFIELDINFO*)mp1,
                                                    SHORT1FROMMP(mp2),
                                                    SHORT2FROMMP(mp2));
        break;

        /*
         * CM_FREEDETAILFIELDINFO:
         *
         *      Paramters:
         *
         *      --  PFIELDINFO* mp1: ptr to array of PFIELDINFO's
         *      --  USHORT mp2: no. of ptrs in array
         *
         *      Returns BOOL.
         */

        case CM_FREEDETAILFIELDINFO:
            mrc = (MRESULT)CnrFreeDetailFieldInfo(hwnd,
                                                  (PFIELDINFO*)mp1,
                                                  (USHORT)mp2);
        break;

        /* ******************************************************************
         *
         *   Record allocation/insertion/removal
         *
         ********************************************************************/

        /*
         * CM_ALLOCRECORD:
         *
         *      Parameters:
         *
         *      --  ULONG mp1: record size in addition to (MINI)RECORDCORE size.
         *
         *      --  USHORT mp2: no. of records to allocate.
         *
         *      Returns linked list of RECORDCORE's or NULL on errors.
         */

        case CM_ALLOCRECORD:
            mrc = (MRESULT)CnrAllocRecord(hwnd,
                                          (ULONG)mp1,
                                          (USHORT)mp2);
        break;

        /*
         * CM_INSERTRECORD:
         *
         *      Parameters:
         *
         *      --  PRECORDCORE mp1: first record
         *
         *      --  PRECORDINSERT pri
         *
         *      Returns the no. of records in the container or 0 on errors.
         */

        case CM_INSERTRECORD:
            mrc = (MRESULT)CnrInsertRecord(hwnd,
                                           (PRECORDCORE)mp1,
                                           (PRECORDINSERT)mp2);
        break;

        default:
            if (pData = (PCNRDATA)WinQueryWindowPtr(hwnd, QWL_USER + 1))
                mrc = ctlDefWindowProc(&pData->dwdMain, msg, mp1, mp2);
        break;

    }

    return mrc;
}

/*
 *@@ ctlRegisterXCnr:
 *
 */

BOOL ctlRegisterXCnr(HAB hab)
{
    return (    WinRegisterClass(hab,
                                 WC_CCTL_CNR,
                                 fnwpCnr,
                                 0, // CS_SYNCPAINT, // CS_CLIPSIBLINGS CS_CLIPCHILDREN
                                 sizeof(PVOID) * 2)
             && WinRegisterClass(hab,
                                 WC_CCTL_CNR_DETAILS,
                                 fnwpCnrDetails,
                                 0, // CS_SYNCPAINT, //  | CS_PARENTCLIP, // CS_CLIPSIBLINGS CS_CLIPCHILDREN
                                 sizeof(PVOID) * 2)
           );
}

