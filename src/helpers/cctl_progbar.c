
/*
 *@@sourcefile cctl_progbar.c:
 *      implementation for the progress bar common control.
 *      See comctl.c for an overview.
 *
 *      This has been extracted from comctl.c with V0.9.3 (2000-05-21) [umoeller].
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@header "helpers\comctl.h"
 *@@added V0.9.3 (2000-05-21) [umoeller].
 */

/*
 *      Copyright (C) 1997-2000 Ulrich M”ller.
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

#define INCL_DOSEXCEPTIONS
#define INCL_DOSPROCESS
#define INCL_DOSSEMAPHORES
#define INCL_DOSERRORS

#define INCL_WINWINDOWMGR
#define INCL_WINFRAMEMGR
#define INCL_WINMESSAGEMGR
#define INCL_WININPUT
#define INCL_WINPOINTERS
#define INCL_WINTRACKRECT
#define INCL_WINTIMER
#define INCL_WINSYS

#define INCL_WINRECTANGLES      /// xxx temporary

#define INCL_WINMENUS
#define INCL_WINSTATICS
#define INCL_WINBUTTONS
#define INCL_WINSTDCNR

#define INCL_GPIPRIMITIVES
#define INCL_GPILOGCOLORTABLE
#define INCL_GPILCIDS
#define INCL_GPIPATHS
#define INCL_GPIREGIONS
#define INCL_GPIBITMAPS             // added V0.9.1 (2000-01-04) [umoeller]: needed for EMX headers
#include <os2.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>             // needed for except.h
#include <assert.h>             // needed for except.h

#include "setup.h"                      // code generation and debugging options

#include "helpers\cnrh.h"
#include "helpers\except.h"             // exception handling
#include "helpers\gpih.h"
#include "helpers\linklist.h"
#include "helpers\winh.h"

#include "helpers\comctl.h"

#pragma hdrstop

/*
 *@@category: Helpers\PM helpers\Window classes\Progress bars
 */

/* ******************************************************************
 *
 *   Progress bars
 *
 ********************************************************************/

/*
 *@@ PaintProgress:
 *      this does the actual painting of the progress bar, called
 *      from ctl_fnwpProgressBar.
 *      It is called both upon WM_PAINT and WM_UPDATEPROGRESSBAR
 *      with different HPS's then.
 *
 *@@added V0.9.0 [umoeller]
 *@@changed V0.9.5 (2000-09-22) [umoeller]: fixed ypos of text
 */

VOID PaintProgress(PPROGRESSBARDATA pData, HWND hwndBar, HPS hps)
{
    POINTL  ptl1, ptlText, aptlText[TXTBOX_COUNT];
    RECTL   rcl, rcl2;

    CHAR    szPercent[10] = "";

    // switch to RGB mode
    GpiCreateLogColorTable(hps, 0, LCOLF_RGB, 0, 0, NULL);

    if (pData->ulPaintX <= pData->ulOldPaintX)
    {
        // draw frame and background only if this is either
        //    a "real" WM_PAINT (i.e., the window was overlapped
        //   and needs repainting; then ulPaintX == ulOldPaintX)
        //   or if ulNow has _de_creased
        GpiSetColor(hps, WinQuerySysColor(HWND_DESKTOP, SYSCLR_BUTTONDARK, 0));
        ptl1.x = 0;
        ptl1.y = 0;
        GpiMove(hps, &ptl1);
        ptl1.y = (pData->rclBar.yTop);
        GpiLine(hps, &ptl1);
        ptl1.x = (pData->rclBar.xRight);
        GpiLine(hps, &ptl1);
        GpiSetColor(hps, WinQuerySysColor(HWND_DESKTOP, SYSCLR_BUTTONLIGHT, 0));
        ptl1.y = 0;
        GpiLine(hps, &ptl1);
        ptl1.x = 0;
        GpiLine(hps, &ptl1);

        pData->rclBar.xLeft = 1;
        pData->rclBar.yBottom = 1;
        WinFillRect(hps, &(pData->rclBar),
            WinQuerySysColor(HWND_DESKTOP, SYSCLR_SCROLLBAR, 0));
    }

    // draw percentage?
    if (pData->ulAttr & PBA_PERCENTFLAGS)
    {
        // make string
        sprintf(szPercent, "%lu %%", ((100 * pData->ulNow) / pData->ulMax) );

        // calculate string space
        GpiQueryTextBox(hps, strlen(szPercent), szPercent,
                TXTBOX_COUNT, (PPOINTL)&aptlText);

        // calculate coordinates
        ptlText.x = pData->rclBar.xLeft +
                        (   (   (pData->rclBar.xRight-pData->rclBar.xLeft)
                              - (aptlText[TXTBOX_BOTTOMRIGHT].x-aptlText[TXTBOX_BOTTOMLEFT].x)
                            )
                        / 2);
        ptlText.y = 2 + pData->rclBar.yBottom +     // fixed V0.9.5 (2000-09-22) [umoeller]
                        (   (   (pData->rclBar.yTop-pData->rclBar.yBottom)
                              - (aptlText[TXTBOX_TOPLEFT].y-aptlText[TXTBOX_BOTTOMLEFT].y)
                            )
                        / 2);

        // do we need to repaint the background under the percentage?
        if (    (   (ptlText.x
                        + (  aptlText[TXTBOX_BOTTOMRIGHT].x
                           - aptlText[TXTBOX_BOTTOMLEFT].x)
                    )
                  > pData->ulPaintX)
            &&  (pData->ulPaintX > pData->ulOldPaintX)
           )
        {
            // if we haven't drawn the background already,
            // we'll need to do it now for the percentage area
            rcl.xLeft      = ptlText.x;
            rcl.xRight     = ptlText.x + (aptlText[TXTBOX_BOTTOMRIGHT].x-aptlText[TXTBOX_BOTTOMLEFT].x);
            rcl.yBottom    = ptlText.y;
            rcl.yTop       = ptlText.y + (aptlText[TXTBOX_TOPLEFT].y-aptlText[TXTBOX_BOTTOMLEFT].y);
            WinFillRect(hps, &rcl,
                WinQuerySysColor(HWND_DESKTOP, SYSCLR_SCROLLBAR, 0));
        }
    }

    // now draw the actual progress
    rcl2.xLeft = pData->rclBar.xLeft;
    rcl2.xRight = (pData->ulPaintX > (rcl2.xLeft + 3))
            ? pData->ulPaintX
            : rcl2.xLeft + ((pData->ulAttr & PBA_BUTTONSTYLE)
                           ? 3 : 1);
    rcl2.yBottom = pData->rclBar.yBottom;
    rcl2.yTop = pData->rclBar.yTop-1;

    if (pData->ulAttr & PBA_BUTTONSTYLE)
    {
        RECTL rcl3 = rcl2;
        // draw "raised" inner rect
        rcl3.xLeft = rcl2.xLeft;
        rcl3.yBottom = rcl2.yBottom;
        rcl3.yTop = rcl2.yTop+1;
        rcl3.xRight = rcl2.xRight+1;
        gpihDraw3DFrame(hps, &rcl3, 2,
                        WinQuerySysColor(HWND_DESKTOP, SYSCLR_BUTTONLIGHT, 0),
                        WinQuerySysColor(HWND_DESKTOP, SYSCLR_BUTTONDARK, 0));
        // WinInflateRect(WinQueryAnchorBlock(hwndBar), &rcl2, -2, -2);
        rcl2.xLeft += 2;
        rcl2.yBottom += 2;
        rcl2.yTop -= 2;
        rcl2.xRight -= 2;
    }

    if (rcl2.xRight > rcl2.xLeft)
    {
        GpiSetColor(hps, WinQuerySysColor(HWND_DESKTOP,
                    // SYSCLR_HILITEBACKGROUND,
                    SYSCLR_BUTTONMIDDLE,
                0));
        ptl1.x = rcl2.xLeft;
        ptl1.y = rcl2.yBottom;
        GpiMove(hps, &ptl1);
        ptl1.x = rcl2.xRight;
        ptl1.y = rcl2.yTop;
        GpiBox(hps, DRO_FILL | DRO_OUTLINE,
            &ptl1,
            0,
            0);
    }

    // now print the percentage
    if (pData->ulAttr & PBA_PERCENTFLAGS)
    {
        GpiMove(hps, &ptlText);
        GpiSetColor(hps, WinQuerySysColor(HWND_DESKTOP,
                // SYSCLR_HILITEFOREGROUND,
                SYSCLR_BUTTONDEFAULT,
            0));
        GpiCharString(hps, strlen(szPercent), szPercent);
    }

    // store current X position for next time
    pData->ulOldPaintX = pData->ulPaintX;
}

/*
 *@@ ctl_fnwpProgressBar:
 *      this is the window procedure for the progress bar control.
 *
 *      This is not a stand-alone window procedure, but must only
 *      be used with static rectangle controls subclassed by
 *      ctlProgressBarFromStatic.
 *
 *      We need to capture WM_PAINT to draw the progress bar according
 *      to the current progress, and we also update the static text field
 *      (percentage) next to it.
 *
 *      This also evaluates the WM_UPDATEPROGRESSBAR message.
 *
 *@@changed V0.9.0 [umoeller]: moved this code here
 *@@changed V0.9.1 (99-12-06): fixed memory leak
 */

MRESULT EXPENTRY ctl_fnwpProgressBar(HWND hwndBar, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    HPS                 hps;
    PPROGRESSBARDATA    ppd = (PPROGRESSBARDATA)WinQueryWindowULong(hwndBar, QWL_USER);

    PFNWP   OldStaticProc = NULL;

    MRESULT mrc = NULL;

    if (ppd)
    {
        OldStaticProc = ppd->OldStaticProc;

        switch(msg)
        {

            /*
             *@@ WM_UPDATEPROGRESSBAR:
             *      post or send this message to a progress bar
             *      to have a new progress displayed.
             *      Parameters:
             *      --  ULONG mp1   current value
             *      --  ULONG mp2   max value
             *      Example: mp1 = 100, mp2 = 300 will result
             *      in a progress of 33%.
             */

            case WM_UPDATEPROGRESSBAR:
            {
                if (    (ppd->ulNow != (ULONG)mp1)
                     || (ppd->ulMax != (ULONG)mp2)
                   )
                {
                    ppd->ulNow = (ULONG)mp1;
                    ppd->ulMax = (ULONG)mp2;
                }
                else
                    // value not changed: do nothing
                    break;

                // check validity
                if (ppd->ulNow > ppd->ulMax)
                    ppd->ulNow = ppd->ulMax;
                // avoid division by zero
                if (ppd->ulMax == 0)
                {
                    ppd->ulMax = 1;
                    ppd->ulNow = 0;
                    // paint 0% then
                }

                // calculate new X position of the progress
                ppd->ulPaintX =
                    (ULONG)(
                              (    (ULONG)(ppd->rclBar.xRight - ppd->rclBar.xLeft)
                                 * (ULONG)(ppd->ulNow)
                              )
                              /  (ULONG)ppd->ulMax
                           );
                if (ppd->ulPaintX != ppd->ulOldPaintX)
                {
                    // X position changed: redraw
                    // WinInvalidateRect(hwndBar, NULL, FALSE);
                    hps = WinGetPS(hwndBar);
                    PaintProgress(ppd, hwndBar, hps);
                    WinReleasePS(hps);
                }
            break; }

            case WM_PAINT:
            {
                RECTL rcl;
                hps = WinBeginPaint(hwndBar, NULLHANDLE, &rcl);
                PaintProgress(ppd, hwndBar, hps);
                WinEndPaint(hps);
            break; }

            /*
             * WM_DESTROY:
             *      free PROGRESSBARDATA
             *      (added V0.9.1 (99-12-06))
             */

            case WM_DESTROY:
            {
                free(ppd);
                mrc = OldStaticProc(hwndBar, msg, mp1, mp2);
            break; }

            default:
                mrc = OldStaticProc(hwndBar, msg, mp1, mp2);
       }
    }
    return (mrc);
}

/*
 *@@ ctlProgressBarFromStatic:
 *      this function turns an existing static rectangle control
 *      into a progress bar by subclassing its window procedure
 *      with ctl_fnwpProgressBar.
 *
 *      This way you can easily create a progress bar as a static
 *      control in any Dialog Editor; after loading the dlg template,
 *      simply call this function with the hwnd of the static control
 *      to make it a status bar.
 *
 *      This is used for all the progress bars in XWorkplace and
 *      WarpIN.
 *
 *      In order to _update_ the progress bar, simply post or send
 *      WM_UPDATEPROGRESSBAR to the static (= progress bar) window;
 *      this message is equal to WM_USER and needs the following
 *      parameters:
 *      --  mp1     ULONG ulNow: the current progress
 *      --  mp2     ULONG ulMax: the maximally possible progress
 *                               (= 100%)
 *
 *      The progress bar automatically calculates the current progress
 *      display. For example, if ulNow = 4096 and ulMax = 8192,
 *      a progress of 50% will be shown. It is possible to change
 *      ulMax after the progress bar has started display. If ulMax
 *      is 0, a progress of 0% will be shown (to avoid division
 *      by zero traps).
 *
 *      ulAttr accepts of the following:
 *      --  PBA_NOPERCENTAGE:    do not display percentage
 *      --  PBA_ALIGNLEFT:       left-align percentage
 *      --  PBA_ALIGNRIGHT:      right-align percentage
 *      --  PBA_ALIGNCENTER:     center percentage
 *      --  PBA_BUTTONSTYLE:     no "flat", but button-like look
 *
 *@@changed V0.9.0 [umoeller]: moved this code here
 */

BOOL ctlProgressBarFromStatic(HWND hwndChart, ULONG ulAttr)
{
    PFNWP OldStaticProc = WinSubclassWindow(hwndChart, ctl_fnwpProgressBar);
    if (OldStaticProc)
    {
        PPROGRESSBARDATA pData = (PPROGRESSBARDATA)malloc(sizeof(PROGRESSBARDATA));
        pData->ulMax = 1;
        pData->ulNow = 0;
        pData->ulPaintX = 0;
        pData->ulAttr = ulAttr;
        pData->OldStaticProc = OldStaticProc;
        WinQueryWindowRect(hwndChart, &(pData->rclBar));
        (pData->rclBar.xRight)--;
        (pData->rclBar.yTop)--;

        WinSetWindowULong(hwndChart, QWL_USER, (ULONG)pData);
        return (TRUE);
    }
    else return (FALSE);
}


