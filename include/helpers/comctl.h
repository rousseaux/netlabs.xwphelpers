/* $Id$ */


/*
 *@@sourcefile comctl.h:
 *      header file for comctl.c. See remarks there.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 */

/*      Copyright (C) 1997-2000 Ulrich M”ller.
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
 *@@include #define INCL_WINWINDOWMGR
 *@@include #define INCL_WINMESSAGEMGR
 *@@include #define INCL_WINDIALOGS
 *@@include #define INCL_WINSTDCNR          // for checkbox containers
 *@@include #define INCL_WININPUT
 *@@include #define INCL_WINSYS
 *@@include #define INCL_WINSHELLDATA
 *@@include #include <os2.h>
 *@@include #include "comctl.h"
 */

#if __cplusplus
extern "C" {
#endif

#ifndef COMCTL_HEADER_INCLUDED
    #define COMCTL_HEADER_INCLUDED

    /* ******************************************************************
     *
     *   "Menu button" control
     *
     ********************************************************************/

    VOID ctlDisplayButtonMenu(HWND hwndButton,
                              HWND hwndMenu);

    BOOL ctlMakeMenuButton(HWND hwndButton,
                           HMODULE hmodMenu,
                           ULONG idMenu);

    /* ******************************************************************
     *
     *   Progress bars
     *
     ********************************************************************/

    /*
     *@@ PROGRESSBARDATA:
     *      structure for progress bar data,
     *      saved at QWL_USER window ulong.
     */

    typedef struct _PROGRESSBARDATA
    {
        ULONG      ulNow,
                   ulMax,
                   ulPaintX,
                   ulOldPaintX;
        ULONG      ulAttr;
        PFNWP      OldStaticProc;
    } PROGRESSBARDATA, *PPROGRESSBARDATA;

    #define WM_UPDATEPROGRESSBAR    WM_USER+1000

    // status bar style attributes
    #define PBA_NOPERCENTAGE        0x0000
    #define PBA_ALIGNLEFT           0x0001
    #define PBA_ALIGNRIGHT          0x0002
    #define PBA_ALIGNCENTER         0x0003
    #define PBA_PERCENTFLAGS        0x0003
    #define PBA_BUTTONSTYLE         0x0010

    BOOL ctlProgressBarFromStatic(HWND hwndStatic, ULONG ulAttr);

    /* ******************************************************************
     *
     *   Chart Control
     *
     ********************************************************************/

    /*
     *@@ CHARTDATA:
     *      chart data. This represents the
     *      data to be displayed.
     *
     *      Used with the CHTM_SETCHARTDATA message
     *      and stored within CHARTCDATA (below).
     */

    typedef struct _CHARTDATA
    {
        USHORT          usStartAngle,
                            // angle to start with (0%).
                            // This must be in the range of 0 to 360 degrees,
                            // with 0 degrees being the rightmost point
                            // of the arc.
                            // All degree values are counter-clockwise from that point.
                            // Example: 90 will start the arc at the top.
                        usSweepAngle;
                            // the maximum angle to use for 100%.
                            // This must be in the range of 0 to 360 degrees,
                            // with 0 degrees being usStartAngle.
                            // All degree values are counter-clockwise from that point.
                            // Example 1: usStartAngle = 0 and usSweepAngle = 360
                            // will draw a full circle from the right.
                            // Example 2: usStartAngle = 180 and usSweepAngle = 270
                            // will draw a three-quarter angle from the left.
        ULONG           cValues;
                            // data item count; the arrays in *padValues and *palColors
                            // (and also *papszDescriptions, if CHS_DESCRIPTIONS is
                            // enabled in CHARTSTYLE) must have this many items.
        double*         padValues;
                            // pointer to an array of double values;
                            // the sum of all these will make up 100%
                            // in the chart (i.e. the usSweepAngle angle).
                            // If this is NULL, the chart displays nothing.
                            // Otherwise, this array must have cValues items.
        LONG*           palColors;
                            // pointer to an array of LONG RGB colors;
                            // each item in this array must correspond
                            // to an item in padValues.
                            // This _must_ be specified if padValues is != NULL.
                            // This array must have cValues items.
        PSZ*            papszDescriptions;
                            // pointer to an array of PSZs containing
                            // data descriptions. If this pointer is
                            // NULL, or CHARTSTYLE.ulStyle does not have
                            // CHS_DESCRIPTIONS set, no texts will be displayed.
                            // Otherwise, this array must have cValues items.
    } CHARTDATA, *PCHARTDATA;

    // chart display mode: currently only PCF_PIECHART is supported.
    #define CHS_PIECHART            0x0000

    // chart display flags (CHARTSTYLE.ulStyle):
    #define CHS_SHADOW              0x0100  // draw shadow
    #define CHS_3D_BRIGHT           0x0200  // draw 3D block in same color as surface;
                                            // CHARTSTYLE.ulThickness defines thickness
    #define CHS_3D_DARKEN           0x0600  // draw 3D block too, but darker
                                            // compared to surface;
                                            // CHARTSTYLE.ulThickness defines thickness

    #define CHS_DESCRIPTIONS        0x1000  // show descriptions
    #define CHS_DESCRIPTIONS_3D     0x3000  // same as CHS_DESCRIPTIONS, but shaded

    #define CHS_SELECTIONS          0x4000  // allow data items to be selected

    /*
     *@@ CHARTSTYLE:
     *
     */

    typedef struct _CHARTSTYLE
    {
        ULONG           ulStyle;        // CHS_* flags
        ULONG           ulThickness;    // pie thickness (with CHS_3D_xxx) in pixels
        double          dPieSize;       // size of the pie chart relative to the control
                                        // size. A value of 1 would make the pie chart
                                        // consume all available space. A value of .5
                                        // would make the pie chart consume half of the
                                        // control's space. The pie chart is always
                                        // centered within the control.
        double          dDescriptions;  // position of the slice descriptions on the pie
                                        // relative to the window size. To calculate the
                                        // description positions, the control calculates
                                        // an invisible pie slice again, for which this
                                        // value is used. So a value of 1 would make the
                                        // descriptions appear on the outer parts of the
                                        // window (not recommended). A value of .5 would
                                        // make the descriptions appear in the center of
                                        // an imaginary line between the pie's center
                                        // and the pie slice's outer border.
                                        // This should be chosen in conjunction with
                                        // dPieSize as well. If this is equal to dPieSize,
                                        // the descriptions appear on the border of the
                                        // slice. If this is half dPieSize, the descriptions
                                        // appear in the center of the pie slice. If this
                                        // is larger than dPieSize, the descriptions appear
                                        // outside the slice.
    } CHARTSTYLE, *PCHARTSTYLE;

    /*
     *@@ CHARTCDATA:
     *      pie chart control data. Composed from the various
     *      chart initialization data.
     *      Stored in QWL_USER of the subclassed static control.
     *      Not available to the application.
     */

    typedef struct _CHARTCDATA
    {
        // data which is initialized upon creation:
        PFNWP           OldStaticProc;  // old static window procedure (from WinSubclassWindow)

        // data which is initialized upon CHTM_SETCHARTDATA/CHTM_SETCHARTSTYLE:
        HDC             hdcMem;         // memory device context for bitmap
        HPS             hpsMem;         // memory presentation space for bitmap
        CHARTDATA       cd;             // chart data: initialized to null values
        CHARTSTYLE      cs;             // chart style: initialized to null values

        HBITMAP         hbmChart;       // chart bitmap (for quick painting)
        HRGN*           paRegions;      // pointer to array of GPI regions for each data item

        // user interaction data:
        LONG            lSelected;      // zero-based index of selected chart item, or -1 if none
        BOOL            fHasFocus;
    } CHARTCDATA, *PCHARTCDATA;

    HBITMAP ctlCreateChartBitmap(HPS hpsMem,
                                 LONG lcx,
                                 LONG lcy,
                                 PCHARTDATA pChartData,
                                 PCHARTSTYLE pChartStyle,
                                 LONG lBackgroundColor,
                                 LONG lTextColor,
                                 HRGN* paRegions);

    BOOL ctlChartFromStatic(HWND hwndStatic);

    #define CHTM_SETCHARTDATA      WM_USER + 2

    #define CHTM_SETCHARTSTYLE     WM_USER + 3

    #define CHTM_ITEMFROMPOINT     WM_USER + 4

    /* ******************************************************************
     *
     *   Split bars
     *
     ********************************************************************/

    #define WC_SPLITWINDOW          "SplitWindowClass"

    #define SBCF_VERTICAL            0x0000
    #define SBCF_HORIZONTAL          0x0001

    #define SBCF_PERCENTAGE          0x0002
    #define SBCF_3DSUNK              0x0100
    #define SBCF_MOVEABLE            0x1000

    /*
     *@@ SPLITBARCDATA:
     *      split bar creation data
     */

    typedef struct _SPLITBARCDATA
    {
        ULONG   ulSplitWindowID;
                    // window ID of the split window
        ULONG   ulCreateFlags;
                    // split window style flags.
                    // One of the following:
                    // -- SBCF_VERTICAL: the split bar will be vertical.
                    // -- SBCF_HORIZONTAL: the split bar will be horizontal.
                    // plus any or none of the following:
                    // -- SBCF_PERCENTAGE: lPos does not specify absolute
                    //      coordinates, but a percentage of the window
                    //      width or height. In that case, the split
                    //      bar position is not fixed, but always recalculated
                    //      as a percentage.
                    //      Otherwise, the split bar will be fixed.
                    // -- SBCF_3DSUNK: draw a "sunk" 3D frame around the
                    //      split windows.
                    // -- SBCF_MOVEABLE: the split bar may be moved with
                    //      the mouse.
        LONG    lPos;
                    // position of the split bar within hwndParentAndOwner.
                    // If SBCF_PERCENTAGE, this has the percentage;
                    // otherwise:
                    //      if this value is positive, it's considered
                    //      an offset from the left/bottom of the frame;
                    //      if it's negative, it's from the right
        ULONG   ulLeftOrBottomLimit,
                ulRightOrTopLimit;
                    // when moving the split bar (SBCF_MOVEABLE), these
                    // are the minimum and maximum values.
                    // ulLeftOrBottomLimit is the left (or bottom) limit,
                    // ulRightOrTopLimit is the right (or top) limit.
                    // Both are offsets in window coordinates from the
                    // left (or bottom) and right (or top) boundaries of
                    // the split window. If both are 0, the whole split
                    // window is allowed (not recommended).
        HWND    hwndParentAndOwner;
                    // the owner and parent of the split bar
                    // and other windows; this must be the FID_CLIENT
                    // of a frame or another split window (when nesting)
    } SPLITBARCDATA, *PSPLITBARCDATA;

    /*
     *@@ SPLITBARDATA:
     *      internal split bar data,
     *      stored in QWL_USER window ulong
     */

    typedef struct _SPLITBARDATA
    {
        SPLITBARCDATA   sbcd;
        PFNWP           OldStaticProc;
        // RECTL           rclBar;
        HPOINTER        hptrOld,        // old pointer stored upon WM_MOUSEMOVE
                        hptrMove;       // PM move pointer, either vertical or horizontal
        BOOL            fCaptured;
        POINTS          ptsMousePos;
        POINTL          ptlDrawLineFrom,
                        ptlDrawLineTo;
        HPS             hpsTrackBar;
        HWND            hwndLinked1,
                            // the left/bottom window to link
                        hwndLinked2;
                            // the right/top window to link
    } SPLITBARDATA, *PSPLITBARDATA;

    #define ID_SPLITBAR  5000           // fixed ID of the split bar
                                        // (child of split window)

    /*
     *@@ SPLM_SETLINKS:
     *      this specifies the windows which the
     *      split window will link. This updates
     *      the internal SPLITBARDATA and changes
     *      the parents of the two subwindows to
     *      the split window.
     *
     *      Parameters:
     *      HWND mp1:   left or bottom subwindow
     *      HWND mp2:   right or top  subwindow
     */

    #define SPLM_SETLINKS       (WM_USER + 500)

    HWND ctlCreateSplitWindow(HAB hab,
                              PSPLITBARCDATA psbcd);

    BOOL ctlUpdateSplitWindow(HWND hwndSplit);

    BOOL ctlSetSplitFrameWindowPos(HWND hwndFrame,
                                   HWND hwndInsertBehind,
                                   LONG x,
                                   LONG y,
                                   LONG cx,
                                   LONG cy,
                                   ULONG fl,
                                   HWND *pahwnd,
                                   ULONG cbhwnd);

    LONG ctlQuerySplitPos(HWND hwndSplit);

    /* ******************************************************************
     *
     *   Subclassed Static Bitmap Control
     *
     ********************************************************************/

    // flags for ANIMATIONDATA.ulFlags
    #define ANF_ICON             0x0001
    #define ANF_BITMAP           0x0002
    #define ANF_PROPORTIONAL     0x0004

    /*
     *@@ ANIMATIONDATA:
     *      this structure gets stored in QWL_USER
     *      before subclassing the static control
     *
     *@@changed V0.9.0: added fields for bitmap support
     */

    typedef struct _ANIMATIONDATA
    {
        // the following need to be initialized before
        // subclassing
        HAB         hab;                // (added V0.9.0)
        ULONG       ulFlags;
                // one of the following:
                // -- ANF_ICON:   display icons
                // -- ANF_BITMAP: display bitmaps
                // -- ANF_BITMAP | ANF_PROPORTIONAL: display bitmaps, but preserve proportions
        RECTL       rclIcon;            // size of static control
        PFNWP       OldStaticProc;      // original WC_STATIC wnd proc

        // the following are set by fnwpSubclassedStatic upon
        // receiving SM_SETHANDLE (in all modes) or later
        HBITMAP     hbm,                // bitmap to be painted upon WM_PAINT
                    hbmHalftoned;       // bitmap in case of WS_DISABLED (added V0.9.0)
        HPOINTER    hptr;               // icon handle passed to SM_SETHANDLE
        HBITMAP     hbmSource;          // bitmap handle passed to SM_SETHANDLE
                                        // (this can be deleted later) (added V0.9.0)

        // the following need to be initialized
        // for icon mode only (ANF_ICON)
        ULONG       ulDelay;            // delay per animation step in ms
        USHORT      usAniCurrent;       // current animation step (>= 0)

        USHORT      usAniCount;         // no. of animation steps
        HPOINTER    ahptrAniIcons[1];   // variable-size array of animation steps;
                                        // there must be usAniCount items
    } ANIMATIONDATA, *PANIMATIONDATA;

    PANIMATIONDATA ctlPrepareStaticIcon(HWND hwndStatic, USHORT usAnimCount);

    BOOL ctlPrepareAnimation(HWND hwndStatic,
                             USHORT usAnimCount,
                             HPOINTER *pahptr,
                             ULONG ulDelay,
                             BOOL fStartAnimation);

    BOOL ctlStartAnimation(HWND hwndStatic);

    BOOL ctlStopAnimation(HWND hwndStatic);

    PANIMATIONDATA ctlPrepareStretchedBitmap(HWND hwndStatic,
                                             BOOL fPreserveProportions);


    /* ******************************************************************
     *
     *   "Tooltip" control
     *
     ********************************************************************/

    // addt'l tooltip window styles: use lower 16 bits
    #define TTS_ALWAYSTIP           0x0001
    #define TTS_NOPREFIX            0x0002
    // non-Win95 flags
    #define TTS_ROUNDED             0x0004
    #define TTS_SHADOW              0x0008

    // TOOLINFO.uFlags flags (ORed)
    #define TTF_IDISHWND            0x0001
    #define TTF_CENTERTIP           0x0002
    #define TTF_RTLREADING          0x0004
    #define TTF_SUBCLASS            0x0008
    // non-Win95 flags
    #define TTF_SHYMOUSE            0x0010

    #define LPSTR_TEXTCALLBACK      (PSZ)-1

    #define TT_SHADOWOFS            10
    #define TT_ROUNDING             8

    /*
     *@@ TOOLINFO:
     *      info structure to register a tool with a
     *      tooltip control. Used with TTM_ADDTOOL
     *      and many other TTM_* messages.
     */

    typedef struct _TOOLINFO
    {
        ULONG /* UINT */ cbSize;       // in: sizeof(TOOLINFO)
        ULONG /* UINT */ uFlags;
                    // in: flags for the tool, any combination of:
                    // -- TTF_IDISHWND: Indicates that the uId member is the window
                    //      handle to the tool. If this flag is not set, uId is
                    //      the identifier of the tool.
                    // -- TTF_CENTERTIP:  Centers the tooltip window below the
                    //      tool specified by the uId member.
                    // -- TTF_RTLREADING: Windows 95 only: Displays text using
                    //      right-to-left reading order on Hebrew or Arabic systems.
                    //      (Ignored on OS/2).
                    // -- TTF_SUBCLASS: Indicates that the tooltip control should
                    //      subclass the tool's window to intercept messages,
                    //      such as WM_MOUSEMOVE. See TTM_RELAYEVENT.
                    // -- TTF_SHYMOUSE (OS/2 only): shy away from mouse pointer;
                    //      always position the tool tip such that it is never
                    //      covered by the mouse pointer (for readability);
                    //      added V0.9.1 (2000-02-04) [umoeller]
        HWND      hwnd;
                    // in: handle to the window that contains the tool. If
                    // lpszText includes the LPSTR_TEXTCALLBACK value, this
                    // member identifies the window that receives TTN_NEEDTEXT
                    // notification messages.
        ULONG /* UINT */ uId;
                    // in: application-defined identifier of the tool. If uFlags
                    // includes the TTF_IDISHWND value, uId must specify the
                    // window handle to the tool.
        RECTL /* RECT */ rect;
                    // in: coordinates of the bounding rectangle of the tool.
                    // The coordinates are relative to the upper-left corner of
                    // the client area of the window identified by hwnd. If
                    // uFlags includes the TTF_IDISHWND value, this member is
                    // ignored.
        HMODULE /* HINSTANCE */ hinst;
                    // in: handle to the instance that contains the string
                    // resource for the tool. If lpszText specifies the identifier
                    // of a string resource, this member is used.
                    // Note: On Win32, this is an HINSTANCE field. On OS/2,
                    // this is a HMODULE field. The field name is retained for
                    // compatibility.
        PSZ /* LPTSTR */ lpszText;
                    // in: pointer to the buffer that contains the text for the
                    // tool (if the hiword is != NULL), or identifier of the string
                    // resource that contains the text (if the hiword == NULL).
                    // If this member is set to the LPSTR_TEXTCALLBACK value,
                    // the control sends the TTN_NEEDTEXT notification message to
                    // the owner window to retrieve the text.
    } TOOLINFO, *PTOOLINFO;

    #define TTM_FIRST                   (WM_USER + 1000)

    #define TTM_ACTIVATE                (TTM_FIRST + 1)

    #define TTM_ADDTOOL                 (TTM_FIRST + 2)

    #define TTM_DELTOOL                 (TTM_FIRST + 3)

    #define TTM_NEWTOOLRECT             (TTM_FIRST + 4)

    #define TTM_RELAYEVENT              (TTM_FIRST + 5)

    // flags for TTM_SETDELAYTIME
    #define TTDT_AUTOMATIC              1
    #define TTDT_AUTOPOP                2
    #define TTDT_INITIAL                3
    #define TTDT_RESHOW                 4

    #define TTM_SETDELAYTIME            (TTM_FIRST + 6)

    /*
     *@@ NMHDR:
     *      structure used with TOOLTIPTEXT;
     *      under Win32, this is a generic structure which
     *      comes with all WM_NOTIFY messages.
     */

    typedef struct _NMHDR
    {
        HWND    hwndFrom;       // control sending the message
        ULONG   idFrom;         // ID of that control
        USHORT  code;           // notification code
    } NMHDR, *PNMHDR;

    /*
     *@@ TOOLTIPTEXT:
     *      identifies a tool for which text is to be displayed and
     *      receives the text for the tool.
     *
     *      This structure is used with the TTN_NEEDTEXT notification.
     */

    typedef struct _TOOLTIPTEXT
    {
        NMHDR     hdr;          // on Win95, this is required for all WM_NOTIFY msgs
        PSZ /* LPTSTR */ lpszText;
                    // out: pointer to a string that contains or receives the text
                    // for a tool. If hinst specifies an instance handle, this
                    // member must be the identifier of a string resource.
        char      szText[80];
                    // out: buffer that receives the tooltip text. An application can
                    // copy the text to this buffer as an alternative to specifying a
                    // string address or string resource.
        HMODULE /* HINSTANCE */ hinst;
                    // out: handle to the instance (OS/2: module) that contains a string
                    // resource to be used as the tooltip text. If lpszText is the
                    // pointer to the tooltip text, this member is NULL.
                    // Note: On Win32, this is an HINSTANCE field. On OS/2,
                    // this is a HMODULE field. The field name is retained for
                    // compatibility.
        ULONG /* UINT */ uFlags;
                    // in: flag that indicates how to interpret the idFrom member of
                    // the NMHDR structure that is included in the structure. If this
                    // member is the TTF_IDISHWND value, idFrom is the handle of the
                    // tool. Otherwise, idFrom is the identifier of the tool.
                    // Windows 95 only: If this member is the TTF_RTLREADING value,
                    // then text on Hebrew or Arabic systems is displayed using
                    // right-to-left reading order. (Ignored on OS/2.)
    } TOOLTIPTEXT, *PTOOLTIPTEXT;

    #define TTM_GETTEXT                 (TTM_FIRST + 7)

    #define TTM_UPDATETIPTEXT           (TTM_FIRST + 8)

    /*
     *@@ TT_HITTESTINFO:
     *      contains information that a tooltip control uses to determine whether
     *      a point is in the bounding rectangle of the specified tool. If the point
     *      is in the rectangle, the structure receives information about the tool.
     *
     *      This structure is used with the TTM_HITTEST message.
     */

    typedef struct _TT_HITTESTINFO
    {
        HWND hwnd;      // in: handle to the tool or window with the specified tool.
        POINTL /* POINT */ pt;
                        // in: client coordinates of the point to test (Win95: POINT)
        TOOLINFO ti;    // out:  receives information about the specified tool.
    } TTHITTESTINFO, *PHITTESTINFO;

    #define TTM_HITTEST                 (TTM_FIRST + 9)

    #define TTM_WINDOWFROMPOINT         (TTM_FIRST + 10)

    #define TTM_ENUMTOOLS               (TTM_FIRST + 11)

    #define TTM_GETCURRENTTOOL          (TTM_FIRST + 12)

    #define TTM_GETTOOLCOUNT            (TTM_FIRST + 13)

    #define TTM_GETTOOLINFO             (TTM_FIRST + 14)

    #define TTM_SETTOOLINFO             (TTM_FIRST + 15)

    // non-Win95 messages

    #define TTM_SHOWTOOLTIPNOW          (TTM_FIRST + 16)

    /*
     *@@ TTN_NEEDTEXT:
     *      control notification sent with the WM_NOTIFY (Win95)
     *      and WM_CONTROL (OS/2) messages.
     *
     *      Parameters (OS/2, incompatible with Win95):
     *      -- mp1 USHORT usID;
     *             USHORT usNotifyCode == TTN_NEEDTEXT
     *      -- PTOOLTIPTEXT mp2: pointer to a TOOLTIPTEXT structure.
     *              The hdr member identifies the tool for which text is needed. The
     *              receiving window can specify the string by taking one of the
     *              following actions:
     *              -- Copying the text to the buffer specified by the szText member.
     *              -- Copying the address of the buffer that contains the text to the
     *                 lpszText member.
     *              -- Copying the identifier of a string resource to the lpszText
     *                 member and copying the handle of the instance that contains
     *                 the resource to the hinst member.
     *
     *      This notification message is sent to the window specified
     *      in the hwnd member of the TOOLINFO structure for the tool.
     *      This notification is sent only if the LPSTR_TEXTCALLBACK
     *      value is specified when the tool is added to a tooltip control.
     *
     *      Windows 95 only: When a TTN_NEEDTEXT notification is received,
     *      the application can set or clear the TTF_RTLREADING value
     *      in the uFlags member of the TOOLTIPTEXT structure pointed to
     *      by lpttt as required. This is the only flag that can be changed
     *      during the notification callback.
     *
     *      OS/2 only: Specifying LPSTR_TEXTCALLBACK in TOOLINFO.lpszText
     *      with TTM_ADDTOOL is the only way under OS/2 to have strings
     *      displayed which are longer than 256 characters, since string
     *      resources are limited to 256 characters with OS/2. It is the
     *      responsibility of the application to set the lpszText member
     *      to a static string buffer which holds the string for the tool.
     *      A common error would be to have that member point to some
     *      variable which has only been allocated on the stack.
     */

    #define TTN_NEEDTEXT        1000

    /*
     *@@ TTN_SHOW:
     *      control notification sent with the WM_NOTIFY (Win95)
     *      and WM_CONTROL (OS/2) messages.
     *
     *      Parameters (OS/2, incompatible with Win95):
     *      -- mp1 USHORT usID;
     *             USHORT usNotifyCode == TTN_NEEDTEXT
     *      -- ULONG mp2: identifier of the tool, as in TOOLINFO.uId.
     *
     *      Return value: always 0.
     *
     *      The TTN_SHOW notification message notifies the owner window
     *      that a tooltip is about to be displayed.
     */

    #define TTN_SHOW            1001

    /*
     *@@ TTN_POP:
     *      control notification sent with the WM_NOTIFY (Win95)
     *      and WM_CONTROL (OS/2) messages.
     *
     *      Parameters (OS/2, incompatible with Win95):
     *      -- mp1 USHORT usID;
     *             USHORT usNotifyCode == TTN_NEEDTEXT
     *      -- ULONG mp2: identifier of the tool, as in TOOLINFO.uId.
     *
     *      Return value: always 0.
     *
     *      The TTN_SHOW notification message notifies the owner window
     *      that a tooltip is about to be hidden.
     */

    #define TTN_POP             1002

    #define COMCTL_TOOLTIP_CLASS    "ComctlTooltipClass"

    BOOL ctlRegisterTooltip(HAB hab);

    MRESULT EXPENTRY ctl_fnwpTooltip(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);

    /* ******************************************************************
     *
     *   Checkbox container record cores
     *
     ********************************************************************/

    BOOL ctlDrawCheckbox(HPS hps,
                         LONG x,
                         LONG y,
                         USHORT usRow,
                         USHORT usColumn,
                         BOOL fHalftoned);

    #ifdef INCL_WINSTDCNR

        /*
         *@@ CN_RECORDCHECKED:
         *      extra WM_CONTROL notification code.
         *      See ctlMakeCheckboxContainer for
         *      details.
         */

        #define CN_RECORDCHECKED            999

        /*
         *@@ CHECKBOXRECORDCORE:
         *      extended record core structure used
         *      with checkbox containers. See
         *      ctlMakeCheckboxContainer for details.
         */

        typedef struct _CHECKBOXRECORDCORE
        {
            RECORDCORE      recc;
                        // standard record core structure
            ULONG           ulStyle;
                        // any combination of the following:
                        // -- WS_VISIBLE
                        // -- none or one of the following:
                        //      BS_AUTOCHECKBOX, BS_AUTO3STATE, BS_3STATE
                        // Internally, we use BS_BITMAP to identify
                        // the depressed checkbox button.
            USHORT          usItemID;
                        // this identifies the record. Must be
                        // unique within the container.
            USHORT          usCheckState;
                        // current check state as with checkboxes
                        // (0, 1, or 2 for tri-state).
            HPOINTER        hptrIcon;
                        // if this is != NULLHANDLE, this icon
                        // will always be used for painting,
                        // instead of the default check box
                        // bitmaps. Useful for non-auto check
                        // box records to implement something
                        // other than checkboxes.
        } CHECKBOXRECORDCORE, *PCHECKBOXRECORDCORE;

        BOOL ctlMakeCheckboxContainer(HWND hwndCnrOwner,
                                      USHORT usCnrID);

        PCHECKBOXRECORDCORE ctlFindCheckRecord(HWND hwndCnr,
                                               ULONG ulItemID);

        BOOL ctlSetRecordChecked(HWND hwndCnr,
                                 ULONG ulItemID,
                                 USHORT usCheckState);

        ULONG ctlQueryRecordChecked(HWND hwndCnr,
                                    ULONG ulItemID,
                                    USHORT usCheckState);

        BOOL ctlEnableRecord(HWND hwndCnr,
                             ULONG ulItemID,
                             BOOL fEnable);
    #endif

    /* ******************************************************************
     *
     *   Hotkey entry field
     *
     ********************************************************************/

    /*
     *@@ EN_HOTKEY:
     *      extra notification code with WM_CONTROL
     *      and subclassed hotkey entry fields.
     *      This is SENT to the entry field's owner
     *      every time a key is pressed. Note that
     *      this is only sent for key-down events
     *      and if all the KC_DEADKEY | KC_COMPOSITE | KC_INVALIDCOMP
     *      flags are not set.
     *
     *      WM_CONTROL parameters in this case:
     *      -- mp1: USHORT id,
     *              USHORT usNotifyCode == EN_HOTKEY
     *      -- mp2: PHOTKEYNOTIFY struct pointer
     *
     *      The receiving owner must check if the key
     *      combo described in HOTKEYNOTIFY makes up
     *      a valid hotkey and return a ULONG composed
     *      of the following flags:
     *
     *      -- HEFL_SETTEXT: if this is set, the text
     *              of the entry field is set to the
     *              text in HOTKEYNOTIFY.szDescription.
     *
     *      -- HEFL_FORWARD2OWNER: if this is set, the
     *              WM_CHAR message is instead passed
     *              to the owner. Use this for the tab
     *              key and such.
     *
     *@@added V0.9.1 (99-12-19) [umoeller]
     *@@changed V0.9.4 (2000-08-03) [umoeller]: added HEFL_* flags
     */

    #define EN_HOTKEY           0x1000

    #define HEFL_SETTEXT        0x0001
    #define HEFL_FORWARD2OWNER  0x0002

    typedef struct _HOTKEYNOTIFY
    {
        USHORT      usFlags,        // in: as in WM_CHAR
                    usvk,           // in: as in WM_CHAR
                    usch;           // in: as in WM_CHAR
        UCHAR       ucScanCode;     // in: as in WM_CHAR
        USHORT      usKeyCode;      // in: if KC_VIRTUAL is set, this has usKeyCode;
                                    //     otherwise usCharCode
        CHAR        szDescription[100]; // out: key description
    } HOTKEYNOTIFY, *PHOTKEYNOTIFY;

    BOOL ctlMakeHotkeyEntryField(HWND hwndHotkeyEntryField);

#endif

#if __cplusplus
}
#endif

