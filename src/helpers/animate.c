
/*
 *@@sourcefile animate.c:
 *      contains a bit of helper code for animations.
 *
 *      This is a new file with V0.81. Most of this code used to reside
 *      in common.c with previous versions.
 *      Note that with V0.9.0, the code for icon animations has been
 *      moved to the new comctl.c file.
 *
 *      Usage: All PM programs.
 *
 *      Function prefixes (new with V0.81):
 *      --  anm*   Animation helper functions
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@header "helpers\animate.h"
 */

/*
 *      Copyright (C) 1997-2000 Ulrich M”ller.
 *      This file is part of the XWorkplace source package.
 *      XWorkplace is free software; you can redistribute it and/or modify
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

#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_DOSDEVICES
#define INCL_DOSDEVIOCTL

#define INCL_WIN
#define INCL_WINSYS

#define INCL_GPILOGCOLORTABLE
#define INCL_GPIPRIMITIVES
#define INCL_GPIBITMAPS             // added UM 99-10-22; needed for EMX headers
#include <os2.h>

#include <stdlib.h>
#include <string.h>

#include "setup.h"                      // code generation and debugging options

#include "helpers\animate.h"

#include "helpers\datetime.h"
#include "helpers\winh.h"
#include "helpers\gpih.h"

/*
 *@@category: Helpers\PM helpers\Animation helpers
 */

/*
 *@@ anmBlowUpBitmap:
 *      this displays an animation based on a given bitmap.
 *      The bitmap is "blown up" in that it is continually
 *      increased in size until the original size is reached.
 *      The animation is calculated so that it lasts exactly
 *      ulAnimation milliseconds, no matter how fast the
 *      system is.
 *
 *      This function does not return until the animation
 *      has completed.
 *
 *      You should run this routine in a thread with higher-
 *      than-normal priority, because otherwise the kernel
 *      might interrupt the thread, causing a somewhat jerky
 *      display.
 *
 *      Returns the count of animation steps that were drawn.
 *      This is dependent on the speed of the system.
 */

BOOL anmBlowUpBitmap(HPS hps,               // in: from WinGetScreenPS(HWND_DESKTOP)
                     HBITMAP hbm,           // in: bitmap to be displayed
                     ULONG ulAnimationTime) // in: total animation time (ms)
{
    ULONG   ulrc = 0;
    ULONG   ulInitialTime,
            ulNowTime;
            // ulCurrentSize = 10;

    if (hps)
    {
        POINTL  ptl = {0, 0};
        RECTL   rtlStretch;
        ULONG   ul,
                ulSteps = 20;
        BITMAPINFOHEADER bih;
        GpiQueryBitmapParameters(hbm, &bih);
        /* ptl.y = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN)
                        - BMPSPACING
                        - bih.cy; */
        ptl.x = (WinQuerySysValue(HWND_DESKTOP, SV_CXSCREEN)
                        - bih.cx) / 2;
        ptl.y = (WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN)
                        - bih.cy) / 2;

        // we now use ul for the current animation step,
        // which is a pointer on a scale from 1 to ulAnimationTime;
        // ul will be recalculated after each animation
        // step according to how much time the animation
        // has cost on this display so far. This has the
        // following advantages:
        // 1) no matter how fast the system is, the animation
        //    will always last ulAnimationTime milliseconds
        // 2) since large bitmaps take more time to calculate,
        //    the animation won't appear to slow down then
        ulInitialTime = dtGetULongTime();
        ul = 1;
        ulSteps = 1000;
        do {
            LONG cx = (((bih.cx-20) * ul) / ulSteps) + 20;
            LONG cy = (((bih.cy-20) * ul) / ulSteps) + 20;
            rtlStretch.xLeft   = ptl.x + ((bih.cx - cx) / 2);
            rtlStretch.yBottom = ptl.y + ((bih.cy - cy) / 2);
            rtlStretch.xRight = rtlStretch.xLeft + cx;
            rtlStretch.yTop   = rtlStretch.yBottom + cy;

            WinDrawBitmap(hps, hbm, NULL, (PPOINTL)&rtlStretch,
                    0, 0,       // we don't need colors
                    DBM_STRETCH);

            ulNowTime = dtGetULongTime();

            // recalculate ul: rule of three based on the
            // time we've spent on animation so far
            ul = (ulSteps
                    * (ulNowTime - ulInitialTime)) // time spent so far
                    / ulAnimationTime;      // time to spend altogether

            ulrc++;                         // return count

        } while (ul < ulSteps);

        // finally, draw the 1:1 version
        WinDrawBitmap(hps, hbm, NULL, &ptl,
                0, 0,       // we don't need colors
                DBM_NORMAL);

    } // end if (hps)

    return (ulrc);
}

#define LAST_WIDTH 2
#define LAST_STEPS 50
#define WAIT_TIME  10

/* ******************************************************************
 *                                                                  *
 *   Other animations                                               *
 *                                                                  *
 ********************************************************************/

/*
 *@@ anmPowerOff:
 *      displays an animation that looks like a
 *      monitor being turned off; hps must have
 *      been acquired using WinGetScreenPS,
 *      ulSteps should be around 40-50.
 */

VOID anmPowerOff(HPS hps, ULONG ulSteps)
{
    RECTL       rclScreen, rclNow, rclLast, rclDraw;
    ULONG       ul = ulSteps;
    ULONG       ulPhase = 1;

    WinQueryWindowRect(HWND_DESKTOP, &rclScreen);

    WinShowPointer(HWND_DESKTOP, FALSE);

    memcpy(&rclLast, &rclScreen, sizeof(RECTL));

    // In order to draw the animation, we tell apart three
    // "phases", signified by the ulPhase variable. While
    // ulPhase != 99, we stay in a do-while loop.
    ul = 0;
    ulPhase = 1;

    do {
        ULONG ulFromTime = dtGetULongTime();

        if (ulPhase == 1)
        {
            // Phase 1: "shrink" the screen by drawing black
            // rectangles from the edges towards the center
            // of the screen. With every loop, we draw the
            // rectangles a bit closer to the center, until
            // the center is black too. Sort of like this:

            //          ***************************
            //          *       black             *
            //          *                         *
            //          *      .............      *
            //          *      . rclNow:   .      *
            //          *  ->  . untouched .  <-  *
            //          *      .............      *
            //          *            ^            *
            //          *            !            *
            //          ***************************

            // rclNow contains the rectangle _around_ which
            // the black rectangles are to be drawn. With
            // every iteration, rclNow is reduced in size.
            rclNow.xLeft = ((rclScreen.yTop / 2) * ul / ulSteps );
            rclNow.xRight = (rclScreen.xRight) - rclNow.xLeft;
            rclNow.yBottom = ((rclScreen.yTop / 2) * ul / ulSteps );
            rclNow.yTop = (rclScreen.yTop) - rclNow.yBottom;

            if (rclNow.yBottom > (rclNow.yTop - LAST_WIDTH) ) {
                rclNow.yBottom = (rclScreen.yTop / 2) - LAST_WIDTH;
                rclNow.yTop = (rclScreen.yTop / 2) + LAST_WIDTH;
            }

            // draw black rectangle on top of rclNow
            rclDraw.xLeft = rclLast.xLeft;
            rclDraw.xRight = rclLast.xRight;
            rclDraw.yBottom = rclNow.yTop;
            rclDraw.yTop = rclLast.yTop;
            WinFillRect(hps, &rclDraw, CLR_BLACK);

            // draw black rectangle left of rclNow
            rclDraw.xLeft = rclLast.xLeft;
            rclDraw.xRight = rclNow.xLeft;
            rclDraw.yBottom = rclLast.yBottom;
            rclDraw.yTop = rclLast.yTop;
            WinFillRect(hps, &rclDraw, CLR_BLACK);

            // draw black rectangle right of rclNow
            rclDraw.xLeft = rclNow.xRight;
            rclDraw.xRight = rclLast.xRight;
            rclDraw.yBottom = rclLast.yBottom;
            rclDraw.yTop = rclLast.yTop;
            WinFillRect(hps, &rclDraw, CLR_BLACK);

            // draw black rectangle at the bottom of rclNow
            rclDraw.xLeft = rclLast.xLeft;
            rclDraw.xRight = rclLast.xRight;
            rclDraw.yBottom = rclLast.yBottom;
            rclDraw.yTop = rclNow.yBottom;
            WinFillRect(hps, &rclDraw, CLR_BLACK);

            // remember rclNow for next iteration
            memcpy(&rclLast, &rclNow, sizeof(RECTL));

            // done with "shrinking"?
            if ( rclNow.xRight < ((rclScreen.xRight / 2) + LAST_WIDTH) ) {
                ulPhase = 2; // exit
            }

        } else if (ulPhase == 2) {
            // Phase 2: draw a horizontal white line about
            // where the last rclNow was. This is only
            // executed once.

            //          ***************************
            //          *       black             *
            //          *                         *
            //          *                         *
            //          *        -----------      *
            //          *                         *
            //          *                         *
            //          *                         *
            //          ***************************

            rclDraw.xLeft = (rclScreen.xRight / 2) - LAST_WIDTH;
            rclDraw.xRight = (rclScreen.xRight / 2) + LAST_WIDTH;
            rclDraw.yBottom = (rclScreen.yTop * 1 / 4);
            rclDraw.yTop = (rclScreen.yTop * 3 / 4);
            WinFillRect(hps, &rclDraw, CLR_WHITE);

            ulPhase = 3;
            ul = 0;

        } else if (ulPhase == 3) {
            // Phase 3: make the white line shorter with
            // every iteration by drawing black rectangles
            // above it. These are drawn closer to the
            // center with each iteration.

            //          ***************************
            //          *       black             *
            //          *                         *
            //          *                         *
            //          *       ->  ----  <-      *
            //          *                         *
            //          *                         *
            //          *                         *
            //          ***************************

            rclDraw.xLeft = (rclScreen.xRight / 2) - LAST_WIDTH;
            rclDraw.xRight = (rclScreen.xRight / 2) + LAST_WIDTH;
            rclDraw.yTop = (rclScreen.yTop * 3 / 4);
            rclDraw.yBottom = (rclScreen.yTop * 3 / 4) - ((rclScreen.yTop * 1 / 4) * ul / LAST_STEPS);
            if (rclDraw.yBottom < ((rclScreen.yTop / 2) + LAST_WIDTH))
                rclDraw.yBottom = ((rclScreen.yTop / 2) + LAST_WIDTH);
            WinFillRect(hps, &rclDraw, CLR_BLACK);

            rclDraw.xLeft = (rclScreen.xRight / 2) - LAST_WIDTH;
            rclDraw.xRight = (rclScreen.xRight / 2) + LAST_WIDTH;
            rclDraw.yBottom = (rclScreen.yTop * 1 / 4);
            rclDraw.yTop = (rclScreen.yTop * 1 / 4) + ((rclScreen.yTop * 1 / 4) * ul / LAST_STEPS);
            if (rclDraw.yTop > ((rclScreen.yTop / 2) - LAST_WIDTH))
                rclDraw.yBottom = ((rclScreen.yTop / 2) - LAST_WIDTH);

            WinFillRect(hps, &rclDraw, CLR_BLACK);

            ul++;
            if (ul > LAST_STEPS) {
                ulPhase = 99;
            }
        }

        ul++;

        while (dtGetULongTime() < ulFromTime + WAIT_TIME) {
            // PSZ p = NULL; // keep compiler happy
        }
    } while (ulPhase != 99);

    // sleep a while
    DosSleep(500);
    WinFillRect(hps, &rclScreen, CLR_BLACK);
    DosSleep(500);

    WinShowPointer(HWND_DESKTOP, TRUE);
}



