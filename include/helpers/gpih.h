/* $Id$ */


/*
 *@@sourcefile gpih.h:
 *      header file for gpih.c (GPI helper functions). See remarks there.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@include #define INCL_GPILOGCOLORTABLE       // for some funcs
 *@@include #include <os2.h>
 *@@include #include "gpih.h"
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

#if __cplusplus
extern "C" {
#endif

#ifndef GPIH_HEADER_INCLUDED
    #define GPIH_HEADER_INCLUDED

    /* ******************************************************************
     *
     *   Device helpers
     *
     ********************************************************************/

    ULONG gpihQueryDisplayCaps(ULONG ulIndex);

    /* ******************************************************************
     *
     *   Color helpers
     *
     ********************************************************************/

    // common RGB colors
    #define RGBCOL_BLACK            0x00000000
    #define RGBCOL_WHITE            0x00FFFFFF

    #define RGBCOL_RED              0x00FF0000
    #define RGBCOL_PINK             0x00FF00FF
    #define RGBCOL_BLUE             0x000000FF
    #define RGBCOL_CYAN             0x0000FFFF
    #define RGBCOL_GREEN            0x0000FF00
    #define RGBCOL_YELLOW           0x00FFFF00
    #define RGBCOL_GRAY             0x00CCCCCC

    #define RGBCOL_DARKRED          0x00800000
    #define RGBCOL_DARKPINK         0x00800080
    #define RGBCOL_DARKBLUE         0x00000080
    #define RGBCOL_DARKCYAN         0x00008080
    #define RGBCOL_DARKGREEN        0x00008000
    #define RGBCOL_DARKYELLOW       0x00808000
    #define RGBCOL_DARKGRAY         0x00808080

    VOID gpihManipulateRGB(PLONG plColor,
                           BYTE bMultiplier,
                           BYTE bDivisor);

    #ifdef INCL_GPILOGCOLORTABLE

        /*
         *@@ gpihSwitchToRGB:
         *      this switches the given HPS into RGB mode.
         *      Requires INCL_GPILOGCOLORTABLE.
         */

        #define gpihSwitchToRGB(hps)                \
            GpiCreateLogColorTable(hps, 0, LCOLF_RGB, 0, 0, NULL);

    #endif

    /* ******************************************************************
     *
     *   Drawing primitives helpers
     *
     ********************************************************************/

    VOID gpihDrawRect(HPS hps, PRECTL prcl);

    VOID gpihBox(HPS hps,
                 LONG lControl,
                 PRECTL prcl,
                 LONG lColor);

    VOID gpihMarker(HPS hps,
                    LONG x,
                    LONG y,
                    ULONG ulWidth,
                    LONG lColor);

    VOID gpihDrawThickFrame(HPS hps,
                            PRECTL prcl,
                            ULONG ulWidth);

    VOID APIENTRY gpihDraw3DFrame(HPS hps,
                                  PRECTL prcl,
                                  USHORT usWidth,
                                  LONG lColorLeft,
                                  LONG lColorRight);
    typedef VOID APIENTRY GPIHDRAW3DFRAME(HPS hps,
                                          PRECTL prcl,
                                          USHORT usWidth,
                                          LONG lColorLeft,
                                          LONG lColorRight);
    typedef GPIHDRAW3DFRAME *PGPIHDRAW3DFRAME;

    LONG gpihCharStringPosAt(HPS hps,
                             PPOINTL pptlStart,
                             PRECTL prclRect,
                             ULONG flOptions,
                             LONG lCount,
                             PCH pchString);

    /* ******************************************************************
     *
     *   Font helpers
     *
     ********************************************************************/

    BOOL gpihSplitPresFont(PSZ pszFontNameSize,
                           PULONG pulSize,
                           PSZ *ppszFaceName);

    LONG gpihFindFont(HPS hps,
                      LONG lSize,
                      BOOL fFamily,
                      PSZ pszName,
                      USHORT usFormat,
                      PFONTMETRICS pFontMetrics);

    LONG gpihFindPresFont(HWND hwnd,
                          BOOL fInherit,
                          HPS hps,
                          PSZ pszDefaultFont,
                          PFONTMETRICS pFontMetrics,
                          PLONG plSize);

    BOOL gpihSetPointSize(HPS hps,
                          LONG lPointSize);

    LONG gpihQueryLineSpacing(HPS hps,
                              PSZ pszText);

    /* ******************************************************************
     *
     *   Bitmap helpers
     *
     ********************************************************************/

    BOOL gpihCreateMemPS(HAB hab,
                         PSIZEL psizlPage,
                         HDC *hdcMem,
                         HPS *hpsMem);

    HBITMAP gpihCreateBitmap(HPS hpsMem,
                             ULONG  cx,
                             ULONG  cy);

    HBITMAP gpihCreateBmpFromPS(HAB hab,
                                HPS hpsScreen,
                                PRECTL prcl);

    HBITMAP gpihCreateHalftonedBitmap(HAB       hab,
                                      HBITMAP   hbmSource,
                                      LONG      lColorGray);

    HBITMAP gpihLoadBitmapFile(HPS hps,
                               PSZ pszBmpFile,
                               PULONG pulError);

    LONG gpihStretchBitmap(HPS hpsTarget,
                           HBITMAP hbmSource,
                           PRECTL prclSource,
                           PRECTL prclTarget,
                           BOOL fProportional);

    BOOL gpihIcon2Bitmap(HPS hpsMem,
                         HPOINTER hptr,
                         LONG lBkgndColor,
                         ULONG ulIconSize);

#endif

#if __cplusplus
}
#endif

