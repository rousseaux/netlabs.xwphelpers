
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
     *   Rectangle helpers
     *
     ********************************************************************/

    BOOL gpihIsPointInRect(PRECTL prcl,
                           LONG x,
                           LONG y);

    VOID gpihInflateRect(PRECTL prcl,
                         LONG l);

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

    VOID XWPENTRY gpihManipulateRGB(PLONG plColor, BYTE bMultiplier, BYTE bDivisor);
    typedef VOID XWPENTRY GPIHMANIPULATERGB(PLONG plColor, BYTE bMultiplier, BYTE bDivisor);
    typedef GPIHMANIPULATERGB *PGPIHMANIPULATERGB;

    BOOL XWPENTRY gpihSwitchToRGB(HPS hps);
    typedef BOOL XWPENTRY GPIHSWITCHTORGB(HPS hps);
    typedef GPIHSWITCHTORGB *PGPIHSWITCHTORGB;

    /* ******************************************************************
     *
     *   Drawing primitives helpers
     *
     ********************************************************************/

    VOID XWPENTRY gpihDrawRect(HPS hps, PRECTL prcl);
    typedef VOID XWPENTRY GPIHDRAWRECT(HPS hps, PRECTL prcl);
    typedef GPIHDRAWRECT *PGPIHDRAWRECT;

    VOID XWPENTRY gpihBox(HPS hps, LONG lControl, PRECTL prcl);
    typedef VOID XWPENTRY GPIHBOX(HPS hps, LONG lControl, PRECTL prcl);
    typedef GPIHBOX *PGPIHBOX;

    VOID XWPENTRY gpihMarker(HPS hps, LONG x, LONG y, ULONG ulWidth);
    typedef VOID XWPENTRY GPIHMARKER(HPS hps, LONG x, LONG y, ULONG ulWidth);
    typedef GPIHMARKER *PGPIHMARKER;

    VOID XWPENTRY gpihDrawThickFrame(HPS hps, PRECTL prcl, ULONG ulWidth);
    typedef VOID XWPENTRY GPIHDRAWTHICKFRAME(HPS hps, PRECTL prcl, ULONG ulWidth);
    typedef GPIHDRAWTHICKFRAME *PGPIHDRAWTHICKFRAME;

    VOID XWPENTRY gpihDraw3DFrame(HPS hps,
                                  PRECTL prcl,
                                  USHORT usWidth,
                                  LONG lColorLeft,
                                  LONG lColorRight);
    typedef VOID XWPENTRY GPIHDRAW3DFRAME(HPS hps,
                                          PRECTL prcl,
                                          USHORT usWidth,
                                          LONG lColorLeft,
                                          LONG lColorRight);
    typedef GPIHDRAW3DFRAME *PGPIHDRAW3DFRAME;

    LONG XWPENTRY gpihCharStringPosAt(HPS hps,
                                      PPOINTL pptlStart,
                                      PRECTL prclRect,
                                      ULONG flOptions,
                                      LONG lCount,
                                      PCH pchString);
    typedef LONG XWPENTRY GPIHCHARSTRINGPOSAT(HPS hps,
                                              PPOINTL pptlStart,
                                              PRECTL prclRect,
                                              ULONG flOptions,
                                              LONG lCount,
                                              PCH pchString);
    typedef GPIHCHARSTRINGPOSAT *PGPIHCHARSTRINGPOSAT;

    /* ******************************************************************
     *
     *   Font helpers
     *
     ********************************************************************/

    BOOL XWPENTRY gpihSplitPresFont(PSZ pszFontNameSize,
                                    PULONG pulSize,
                                    PSZ *ppszFaceName);
    typedef BOOL XWPENTRY GPIHSPLITPRESFONT(PSZ pszFontNameSize,
                                            PULONG pulSize,
                                            PSZ *ppszFaceName);
    typedef GPIHSPLITPRESFONT *PGPIHSPLITPRESFONT;

    BOOL XWPENTRY gpihLockLCIDs(VOID);
    typedef BOOL XWPENTRY GPIHLOCKLCIDS(VOID);
    typedef GPIHLOCKLCIDS *PGPIHLOCKLCIDS;

    VOID XWPENTRY gpihUnlockLCIDs(VOID);
    typedef VOID XWPENTRY GPIHUNLOCKLCIDS(VOID);
    typedef GPIHUNLOCKLCIDS *PGPIHUNLOCKLCIDS;

    LONG XWPENTRY gpihFindFont(HPS hps,
                               LONG lSize,
                               BOOL fFamily,
                               PSZ pszName,
                               USHORT usFormat,
                               PFONTMETRICS pFontMetrics);
    typedef LONG XWPENTRY GPIHFINDFONT(HPS hps,
                                       LONG lSize,
                                       BOOL fFamily,
                                       PSZ pszName,
                                       USHORT usFormat,
                                       PFONTMETRICS pFontMetrics);
    typedef GPIHFINDFONT *PGPIHFINDFONT;

    LONG XWPENTRY gpihFindPresFont(HWND hwnd,
                                   BOOL fInherit,
                                   HPS hps,
                                   const char *pcszDefaultFont,
                                   PFONTMETRICS pFontMetrics,
                                   PLONG plSize);
    typedef LONG XWPENTRY GPIHFINDPRESFONT(HWND hwnd,
                                           BOOL fInherit,
                                           HPS hps,
                                           const char *pcszDefaultFont,
                                           PFONTMETRICS pFontMetrics,
                                           PLONG plSize);
    typedef GPIHFINDPRESFONT *PGPIHFINDPRESFONT;

    BOOL XWPENTRY gpihSetPointSize(HPS hps, LONG lPointSize);
    typedef BOOL XWPENTRY GPIHSETPOINTSIZE(HPS hps, LONG lPointSize);
    typedef GPIHSETPOINTSIZE *PGPIHSETPOINTSIZE;

    LONG XWPENTRY gpihQueryLineSpacing(HPS hps);
    typedef LONG XWPENTRY GPIHQUERYLINESPACING(HPS hps);
    typedef GPIHQUERYLINESPACING *PGPIHQUERYLINESPACING;

    /* ******************************************************************
     *
     *   Bitmap helpers
     *
     ********************************************************************/

    BOOL XWPENTRY gpihCreateMemPS(HAB hab, PSIZEL psizlPage, HDC *hdcMem, HPS *hpsMem);
    typedef BOOL XWPENTRY GPIHCREATEMEMPS(HAB hab, PSIZEL psizlPage, HDC *hdcMem, HPS *hpsMem);
    typedef GPIHCREATEMEMPS *PGPIHCREATEMEMPS;

    HBITMAP XWPENTRY gpihCreateBitmap(HPS hpsMem, ULONG  cx, ULONG  cy);
    typedef HBITMAP XWPENTRY GPIHCREATEBITMAP(HPS hpsMem, ULONG  cx, ULONG  cy);
    typedef GPIHCREATEBITMAP *PGPIHCREATEBITMAP;

    HBITMAP XWPENTRY gpihCreateBmpFromPS(HAB hab, HPS hpsScreen, PRECTL prcl);
    typedef HBITMAP XWPENTRY GPIHCREATEBMPFROMPS(HAB hab, HPS hpsScreen, PRECTL prcl);
    typedef GPIHCREATEBMPFROMPS *PGPIHCREATEBMPFROMPS;

    HBITMAP XWPENTRY gpihCreateHalftonedBitmap(HAB hab, HBITMAP hbmSource, LONG lColorGray);
    typedef HBITMAP XWPENTRY GPIHCREATEHALFTONEDBITMAP(HAB hab, HBITMAP hbmSource, LONG lColorGray);
    typedef GPIHCREATEHALFTONEDBITMAP *PGPIHCREATEHALFTONEDBITMAP;

    HBITMAP XWPENTRY gpihLoadBitmapFile(HPS hps, PSZ pszBmpFile, PULONG pulError);
    typedef HBITMAP XWPENTRY GPIHLOADBITMAPFILE(HPS hps, PSZ pszBmpFile, PULONG pulError);
    typedef GPIHLOADBITMAPFILE *PGPIHLOADBITMAPFILE;

    LONG XWPENTRY gpihStretchBitmap(HPS hpsTarget,
                                    HBITMAP hbmSource,
                                    PRECTL prclSource,
                                    PRECTL prclTarget,
                                    BOOL fProportional);
    typedef LONG XWPENTRY GPIHSTRETCHBITMAP(HPS hpsTarget,
                                            HBITMAP hbmSource,
                                            PRECTL prclSource,
                                            PRECTL prclTarget,
                                            BOOL fProportional);
    typedef GPIHSTRETCHBITMAP *PGPIHSTRETCHBITMAP;

    BOOL XWPENTRY gpihIcon2Bitmap(HPS hpsMem, HPOINTER hptr, LONG lBkgndColor, ULONG ulIconSize);
    typedef BOOL XWPENTRY GPIHICON2BITMAP(HPS hpsMem, HPOINTER hptr, LONG lBkgndColor, ULONG ulIconSize);
    typedef GPIHICON2BITMAP *PGPIHICON2BITMAP;

#endif

#if __cplusplus
}
#endif

