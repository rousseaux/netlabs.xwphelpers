/* $Id$ */


/*
 *@@sourcefile winh.h:
 *      header file for winh.c (PM helper funcs). See remarks there.
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
 *@@include #define INCL_WINSYS             // for winhStorePresParam
 *@@include #define INCL_WINDIALOGS
 *@@include #define INCL_WINMENUS           // for menu helpers
 *@@include #define INCL_WINENTRYFIELDS     // for entry field helpers
 *@@include #define INCL_WINBUTTONS         // for button/check box helpers
 *@@include #define INCL_WINLISTBOXES       // for list box helpers
 *@@include #define INCL_WINSTDSPIN         // for spin button helpers
 *@@include #define INCL_WINSTDSLIDER       // for slider helpers
 *@@include #define INCL_WININPUT
 *@@include #define INCL_WINSYS
 *@@include #define INCL_WINSHELLDATA
 *@@include #define INCL_WINSWITCHLIST      // for winhQuerySwitchList
 *@@include #define INCL_WINPROGRAMLIST     // for winhStartApp
 *@@include #define INCL_WINHELP            // for help manager helpers
 *@@include #include <os2.h>
 *@@include #include "winh.h"
 */

#if __cplusplus
extern "C" {
#endif

#ifndef WINH_HEADER_INCLUDED
    #define WINH_HEADER_INCLUDED

    #ifndef XWPENTRY
        #error You must define XWPENTRY to contain the standard linkage for the XWPHelpers.
    #endif

    /* ******************************************************************
     *
     *   Declarations
     *
     ********************************************************************/

    #define MPNULL                 (MPFROMP(NULL))
    #define MPZERO                 (MPFROMSHORT(0))
    #define MRTRUE                 (MRFROMSHORT((SHORT) TRUE))
    #define MRFALSE                (MRFROMSHORT((SHORT) FALSE))
    #define BM_UNCHECKED           0   // for checkboxes: disabled
    #define BM_CHECKED             1   // for checkboses: enabled
    #define BM_INDETERMINATE       2   // for tri-state checkboxes: indeterminate

    /* ******************************************************************
     *
     *   Macros
     *
     ********************************************************************/

    /*
     *  Here come some monster macros for
     *  frequently needed functions.
     */

    #define winhDebugBox(hwndOwner, title, text) \
    WinMessageBox(HWND_DESKTOP, hwndOwner, ((PSZ)text), ((PSZ)title), 0, MB_OK | MB_ICONEXCLAMATION | MB_MOVEABLE)

    #define winhYesNoBox(title, text) \
    WinMessageBox(HWND_DESKTOP, HWND_DESKTOP, ((PSZ)text), ((PSZ)title), 0, MB_YESNO | MB_ICONQUESTION | MB_MOVEABLE)

    #define winhSetDlgItemChecked(hwnd, id, bCheck) \
            WinSendDlgItemMsg((hwnd), (id), BM_SETCHECK, MPFROMSHORT(bCheck), MPNULL)

    #define winhIsDlgItemChecked(hwnd, id) \
            (SHORT1FROMMR(WinSendDlgItemMsg((hwnd), (id), BM_QUERYCHECK, MPNULL, MPNULL)))

    #define winhSetMenuItemChecked(hwndMenu, usId, bCheck) \
            WinSendMsg(hwndMenu, MM_SETITEMATTR, MPFROM2SHORT(usId, TRUE), \
                    MPFROM2SHORT(MIA_CHECKED, (((bCheck)) ? MIA_CHECKED : FALSE)))

    #define winhShowDlgItem(hwnd, id, show) \
            WinShowWindow(WinWindowFromID(hwnd, id), show)

    #define winhEnableDlgItem(hwndDlg, ulId, Enable) \
            WinEnableWindow(WinWindowFromID(hwndDlg, ulId), Enable)

    #define winhIsDlgItemEnabled(hwndDlg, ulId) \
            WinIsWindowEnabled(WinWindowFromID(hwndDlg, ulId))

    #define winhSetDlgItemFocus(hwndDlg, ulId) \
            WinSetFocus(HWND_DESKTOP, WinWindowFromID(hwndDlg, ulId))

    /* ******************************************************************
     *
     *   Menu helpers
     *
     ********************************************************************/

    /*
     * winhCreateEmptyMenu:
     *      this macro creates an empty menu, which can
     *      be used with winhInsertMenuItem etc. later.
     *      Useful for creating popup menus on the fly.
     *      Note that even though HWND_DESKTOP is specified
     *      here as both the parent and the owner, the
     *      actual owner and parent are specified later
     *      with WinPopupMenu.
     */

    #define winhCreateEmptyMenu()                                   \
    WinCreateWindow(HWND_DESKTOP, WC_MENU, "", 0, 0, 0, 0, 0,       \
                    HWND_DESKTOP, HWND_TOP, 0, 0, 0)

    SHORT XWPENTRY winhInsertMenuItem(HWND hwndMenu,
                                      SHORT iPosition,
                                      SHORT sItemId,
                                      const char *pcszItemTitle,
                                      SHORT afStyle,
                                      SHORT afAttr);

    HWND XWPENTRY winhInsertSubmenu(HWND hwndMenu,
                                    ULONG iPosition,
                                    SHORT sMenuId,
                                    const char *pcszSubmenuTitle,
                                    USHORT afMenuStyle,
                                    SHORT sItemId,
                                    const char *pcszItemTitle,
                                    USHORT afItemStyle,
                                    USHORT afAttribute);

    /*
     *@@ winhRemoveMenuItem:
     *      removes a menu item (SHORT) from the
     *      given menu (HWND). Returns the no. of
     *      remaining menu items (SHORT).
     *
     *      This works for whole submenus too.
     */

    #define winhRemoveMenuItem(hwndMenu, sItemId) \
            (SHORT)WinSendMsg(hwndMenu, MM_REMOVEITEM, MPFROM2SHORT(sItemId, FALSE), 0)

    /*
     *@@ winhDeleteMenuItem:
     *      deleted a menu item (SHORT) from the
     *      given menu (HWND). Returns the no. of
     *      remaining menu items (SHORT).
     *
     *      As opposed to MM_REMOVEITEM, MM_DELETEITEM
     *      frees submenus and bitmaps also.
     *
     *      This works for whole submenus too.
     */

    #define winhDeleteMenuItem(hwndMenu, sItemId) \
            (SHORT)WinSendMsg(hwndMenu, MM_DELETEITEM, MPFROM2SHORT(sItemId, FALSE), 0)

    SHORT XWPENTRY winhInsertMenuSeparator(HWND hMenu,
                                           SHORT iPosition,
                                           SHORT sId);

    PSZ XWPENTRY winhQueryMenuItemText(HWND hwndMenu,
                                       USHORT usItemID);

    BOOL XWPENTRY winhAppend2MenuItemText(HWND hwndMenu,
                                          USHORT usItemID,
                                          const char *pcszAppend,
                                          BOOL fTab);

    VOID XWPENTRY winhMenuRemoveEllipse(HWND hwndMenu,
                                        USHORT usItemId);

    SHORT XWPENTRY winhQueryItemUnderMouse(HWND hwndMenu, POINTL *pptlMouse, RECTL *prtlItem);

    /* ******************************************************************
     *
     *   Slider helpers
     *
     ********************************************************************/

    HWND XWPENTRY winhReplaceWithLinearSlider(HWND hwndParent,
                                              HWND hwndOwner,
                                              HWND hwndInsertAfter,
                                              ULONG ulID,
                                              ULONG ulSliderStyle,
                                              ULONG ulTickCount);

    BOOL winhSetSliderTicks(HWND hwndSlider,
                            MPARAM mpEveryOther1,
                            ULONG ulPixels1,
                            MPARAM mpEveryOther2,
                            ULONG ulPixels2);

    /*
     * winhSetSliderArmPosition:
     *      this moves the slider arm in a given
     *      linear slider.
     *
     *      usMode can be one of the following:
     *      --  SMA_RANGEVALUE: usOffset is in pixels
     *          from the slider's home position.
     *      --  SMA_INCREMENTVALUE: usOffset is in
     *          units of the slider's primary scale.
     */

    #define winhSetSliderArmPosition(hwndSlider, usMode, usOffset)  \
            WinSendMsg(hwndSlider, SLM_SETSLIDERINFO,               \
                   MPFROM2SHORT(SMA_SLIDERARMPOSITION,              \
                     usMode),                                       \
                   MPFROMSHORT(usOffset))

    /*
     * winhQuerySliderArmPosition:
     *      reverse to the previous, this returns a
     *      slider arm position (as a LONG value).
     */

    #define winhQuerySliderArmPosition(hwndSlider, usMode)          \
            (LONG)(WinSendMsg(hwndSlider,                           \
                              SLM_QUERYSLIDERINFO,                  \
                              MPFROM2SHORT(SMA_SLIDERARMPOSITION,   \
                                           usMode),                 \
                              0))

    HWND XWPENTRY winhReplaceWithCircularSlider(HWND hwndParent,
                                                HWND hwndOwner,
                                                HWND hwndInsertAfter,
                                                ULONG ulID,
                                                ULONG ulSliderStyle,
                                                SHORT sMin,
                                                SHORT sMax,
                                                USHORT usIncrement,
                                                USHORT usTicksEvery);

    /* ******************************************************************
     *
     *   Spin button helpers
     *
     ********************************************************************/

    VOID XWPENTRY winhSetDlgItemSpinData(HWND hwndDlg,
                                         ULONG idSpinButton,
                                         ULONG min,
                                         ULONG max,
                                         ULONG current);

    LONG XWPENTRY winhAdjustDlgItemSpinData(HWND hwndDlg,
                                            USHORT usItemID,
                                            LONG lGrid,
                                            USHORT usNotifyCode);

    /* ******************************************************************
     *
     *   Entry field helpers
     *
     ********************************************************************/

    /*
     * winhSetEntryFieldLimit:
     *      sets the maximum length for the entry field
     *      (EM_SETTEXTLIMIT message).
     *
     *      PMREF doesn't say this, but the limit does
     *      _not_ include the null-terminator. That is,
     *      if you specify "12" characters here, you can
     *      really enter 12 characters.
     *
     *      The default length is 30 characters, I think.
     */

    #define winhSetEntryFieldLimit(hwndEntryField, usLength)        \
            WinSendMsg(hwndEntryField, EM_SETTEXTLIMIT, (MPARAM)(usLength), (MPARAM)0)

    /*
     *@@ winhEntryFieldSelectAll:
     *      this selects the entire text in the entry field.
     *      Useful when the thing gets the focus.
     */

    #define winhEntryFieldSelectAll(hwndEntryField)                 \
            WinSendMsg(hwndEntryField, EM_SETSEL, MPFROM2SHORT(0, 10000), MPNULL)

    /*
     *@@ winhHasEntryFieldChanged:
     *      this queries whether the entry field's contents
     *      have changed (EM_QUERYCHANGED message).
     *
     *      This returns TRUE if changes have occured since
     *      the last time this message or WM_QUERYWINDOWPARAMS
     *      (WinQueryWindowText) was received.
     */

    #define winhHasEntryFieldChanged(hwndEntryField)                \
            (BOOL)WinSendMsg(hwndEntryField, EM_QUERYCHANGED, (MPARAM)0, (MPARAM)0)

    /* ******************************************************************
     *
     *   List box helpers
     *
     ********************************************************************/

    /*  The following macros are defined in the OS/2 headers for
        list boxes:

        LONG WinQueryLboxCount(HWND hwndLbox);
                    // wrapper around LM_QUERYITEMCOUNT;
                    // list box item count

        SHORT WinQueryLboxItemTextLength(HWND hwndLbox,
                                         SHORT index); // item index, starting from 0
                    // wrapper around LM_QUERYITEMTEXTLENGTH;
                    // returns length of item text, excluding NULL character

        LONG WinQueryLboxItemText(HWND hwndLbox,
                                  SHORT index,      // item index, starting from 0
                                  PSZ psz,          // buffer
                                  PSZ cchMax);      // size of buffer, incl. null
                    // wrapper around LM_QUERYITEMTEXT;
                    // returns length of item text, excluding NULL character

        BOOL WinSetLboxItemText(HWND hwndLbox,
                                LONG index,
                                PSZ psz);
                    // wrapper around LM_SETITEMTEXT

        LONG WinInsertLboxItem(HWND hwndLbox,
                               LONG index,          // new item index, starting from 0
                                                    // or LIT_END
                                                    // or LIT_SORTASCENDING
                                                    // or LIT_SORTDESCENDING
                               PSZ psz)
                    // wrapper around LM_INSERTITEM;
                    // returns LIT_MEMERROR, LIT_ERROR, or zero-based index

        LONG WinDeleteLboxItem(HWND hwndLbox,
                               LONG index);         // item index, starting from 0

        LONG WinQueryLboxSelectedItem(HWND hwndLbox);
                    // wrapper around LM_QUERYSELECTION;
                    // works with single selection only,
                    // use winhQueryLboxSelectedItem instead
    */

    /*
     * winhQueryLboxItemCount:
     *      returns the no. of items in the listbox
     *      as a SHORT.
     *
     *added V0.9.1 (99-12-14) [umoeller]
     */

    // #define winhQueryLboxItemCount(hwndListBox)
    // (SHORT)WinSendMsg(hwndListBox, LM_QUERYITEMCOUNT, 0, 0)

    // removed, use WinQueryLboxCount

    /*
     *@@ winhDeleteAllItems:
     *      deletes all list box items. Returns
     *      a BOOL.
     *
     *@@added V0.9.1 (99-12-14) [umoeller]
     */

    #define winhDeleteAllItems(hwndListBox)                     \
        (BOOL)WinSendMsg(hwndListBox,                           \
                         LM_DELETEALL, 0, 0)
    /*
     *@@ winhQueryLboxSelectedItem:
     *      this queries the next selected list box item.
     *      For the first call, set sItemStart to LIT_FIRST;
     *      then repeat the call with sItemStart set to
     *      the previous return value until this returns
     *      LIT_NONE.
     *
     *      Example:
     +          SHORT sItemStart = LIT_FIRST;
     +          while (TRUE)
     +          {
     +              sItemStart = winhQueryLboxSelectedItem(hwndListBox,
     +                                                     sItemStart)
     +              if (sItemStart == LIT_NONE)
     +                  break;
     +              ...
     +          }
     *
     *      To have the cursored item returned, use LIT_CURSOR.
     *
     *      For single selection, you can also use
     *      WinQueryLboxSelectedItem from the OS/2 PM headers.
     */

    #define winhQueryLboxSelectedItem(hwndListBox, sItemStart) \
            (SHORT)(WinSendMsg(hwndListBox,                    \
                            LM_QUERYSELECTION,                 \
                            (MPARAM)(sItemStart),              \
                            MPNULL))

    /*
     *@@ winhSetLboxSelectedItem:
     *      selects a list box item.
     *      This works for both single-selection and
     *      multiple-selection listboxes.
     *      In single-selection listboxes, if an item
     *      is selected (fSelect == TRUE), the previous
     *      item is automatically deselected.
     *      Note that (BOOL)fSelect is ignored if
     *      sItemIndex == LIT_NONE.
     */

    #define winhSetLboxSelectedItem(hwndListBox, sItemIndex, fSelect)   \
            (BOOL)(WinSendMsg(hwndListBox,                              \
                            LM_SELECTITEM,                              \
                            (MPARAM)(sItemIndex),                       \
                            (MPARAM)(fSelect)))

    ULONG XWPENTRY winhLboxSelectAll(HWND hwndListBox, BOOL fSelect);

    /*
     * winhSetLboxItemHandle:
     *      sets the "item handle" for the specified sItemIndex.
     *      See LM_SETITEMHANDLE in PMREF for details.
     */

    #define winhSetLboxItemHandle(hwndListBox, sItemIndex, ulHandle)    \
            (BOOL)(WinSendMsg(hwndListBox, LM_SETITEMHANDLE,            \
                              (MPARAM)(sItemIndex),                     \
                              (MPARAM)ulHandle))

    /*
     * winhQueryLboxItemHandle:
     *      the reverse to the previous. Returns a ULONG.
     */

    #define winhQueryLboxItemHandle(hwndListBox, sItemIndex)            \
            (ULONG)WinSendMsg(hwndListBox, LM_QUERYITEMHANDLE,          \
                              MPFROMSHORT(sItemIndex), (MPARAM)NULL)

    PSZ XWPENTRY winhQueryLboxItemText(HWND hwndListbox, SHORT sIndex);

    BOOL XWPENTRY winhMoveLboxItem(HWND hwndSource,
                                   SHORT sSourceIndex,
                                   HWND hwndTarget,
                                   SHORT sTargetIndex,
                                   BOOL fSelectTarget);

    /* ******************************************************************
     *
     *   Scroll bar helpers
     *
     ********************************************************************/

    BOOL XWPENTRY winhUpdateScrollBar(HWND hwndScrollBar,
                                      ULONG ulWinPels,
                                      ULONG ulViewportPels,
                                      ULONG ulCurUnitOfs,
                                      BOOL fAutoHide);

    BOOL XWPENTRY winhHandleScrollMsg(HWND hwnd2Scroll,
                                      HWND hwndScrollBar,
                                      PULONG pulCurPelsOfs,
                                      PRECTL prcl2Scroll,
                                      LONG ulViewportPels,
                                      USHORT usLineStepUnits,
                                      ULONG msg,
                                      MPARAM mp2);

    BOOL XWPENTRY winhProcessScrollChars(HWND hwndClient,
                                         HWND hwndVScroll,
                                         HWND hwndHScroll,
                                         MPARAM mp1,
                                         MPARAM mp2,
                                         ULONG ulVertMax,
                                         ULONG ulHorzMax);

    /* ******************************************************************
     *
     *   Window positioning helpers
     *
     ********************************************************************/

    BOOL XWPENTRY winhSaveWindowPos(HWND hwnd, HINI hIni, const char *pcszApp, const char *pcszKey);

    BOOL XWPENTRY winhRestoreWindowPos(HWND hwnd, HINI hIni, const char *pcszApp, const char *pcszKey, ULONG fl);

    #define XAC_MOVEX       0x0001
    #define XAC_MOVEY       0x0002
    #define XAC_SIZEX       0x0004
    #define XAC_SIZEY       0x0008

    /*
     *@@ XADJUSTCTRLS:
     *
     */

    typedef struct _XADJUSTCTRLS
    {
        BOOL        fInitialized;
        SWP         swpMain;            // SWP for main window
        SWP         *paswp;             // pointer to array of control SWP structs
    } XADJUSTCTRLS, *PXADJUSTCTRLS;

    BOOL XWPENTRY winhAdjustControls(HWND hwndDlg,
                                     MPARAM *pmpFlags,
                                     ULONG ulCount,
                                     PSWP pswpNew,
                                     PXADJUSTCTRLS pxac);

    void XWPENTRY winhCenterWindow(HWND hwnd);
    typedef void XWPENTRY WINHCENTERWINDOW(HWND hwnd);
    typedef WINHCENTERWINDOW *PWINHCENTERWINDOW;

    HWND XWPENTRY winhFindWindowBelow(HWND hwndFind);

    /* ******************************************************************
     *
     *   Presparams helpers
     *
     ********************************************************************/

    PSZ XWPENTRY winhQueryWindowFont(HWND hwnd);
    typedef PSZ XWPENTRY WINHQUERYWINDOWFONT(HWND hwnd);
    typedef WINHQUERYWINDOWFONT *PWINHQUERYWINDOWFONT;

    BOOL XWPENTRY winhSetWindowFont(HWND hwnd, const char *pcszFont);
    typedef BOOL XWPENTRY WINHSETWINDOWFONT(HWND hwnd, const char *pcszFont);
    typedef WINHSETWINDOWFONT *PWINHSETWINDOWFONT;

    /*
     *@@ winhSetDlgItemFont:
     *      invokes winhSetWindowFont on a dialog
     *      item.
     *
     *      Returns TRUE if successful or FALSE otherwise.
     *
     *@@added V0.9.0
     */

    #define winhSetDlgItemFont(hwnd, usId, pszFont) \
            (winhSetWindowFont(WinWindowFromID(hwnd, usId), pszFont))

    ULONG XWPENTRY winhSetControlsFont(HWND hwndDlg, SHORT usIDMin, SHORT usIDMax, const char *pcszFont);

    #ifdef INCL_WINSYS
        BOOL XWPENTRY winhStorePresParam(PPRESPARAMS *pppp,
                                         ULONG ulAttrType,
                                         ULONG cbData,
                                         PVOID pData);
    #endif

    LONG XWPENTRY winhQueryPresColor(HWND hwnd, ULONG ulPP, BOOL fInherit, LONG lSysColor);
    typedef LONG XWPENTRY WINHQUERYPRESCOLOR(HWND hwnd, ULONG ulPP, BOOL fInherit, LONG lSysColor);
    typedef WINHQUERYPRESCOLOR *PWINHQUERYPRESCOLOR;

    /* ******************************************************************
     *
     *   Help instance helpers
     *
     ********************************************************************/

    #ifdef INCL_WINHELP
        HWND XWPENTRY winhCreateHelp(HWND hwndFrame,
                                     const char *pcszFileName,
                                     HMODULE hmod,
                                     PHELPTABLE pHelpTable,
                                     const char *pcszWindowTitle);

        ULONG winhDisplayHelpPanel(HWND hwndHelpInstance,
                                   ULONG ulHelpPanel);

        void XWPENTRY winhDestroyHelp(HWND hwndHelp, HWND hwndFrame);
    #endif

    /* ******************************************************************
     *
     *   Application control
     *
     ********************************************************************/

    #ifdef INCL_WINPROGRAMLIST
        HAPP XWPENTRY winhStartApp(HWND hwndNotify, const PROGDETAILS *pcProgDetails);
    #endif

    BOOL XWPENTRY winhAnotherInstance(const char *pcszSemName, BOOL fSwitch);

    HSWITCH XWPENTRY winhAddToTasklist(HWND hwnd, HPOINTER hIcon);

    /* ******************************************************************
     *
     *   Miscellaneous
     *
     ********************************************************************/

    VOID XWPENTRY winhFree(PVOID p);
    typedef VOID XWPENTRY WINHFREE(PVOID p);
    typedef WINHFREE *PWINHFREE;

    VOID XWPENTRY winhSleep(HAB hab, ULONG ulSleep);

    #define WINH_FOD_SAVEDLG        0x0001
    #define WINH_FOD_INILOADDIR     0x0010
    #define WINH_FOD_INISAVEDIR     0x0020

    BOOL XWPENTRY winhFileDlg(HWND hwndOwner,
                              PSZ pszFile,
                              ULONG flFlags,
                              HINI hini,
                              const char *pcszApplication,
                              const char *pcszKey);

    HPOINTER XWPENTRY winhSetWaitPointer(VOID);

    PSZ XWPENTRY winhQueryWindowText(HWND hwnd);

    /*
     *@@ winhQueryDlgItemText:
     *      like winhQueryWindowText, but for the dialog item
     *      in hwnd which has the ID usItemID.
     */

    #define winhQueryDlgItemText(hwnd, usItemID) winhQueryWindowText(WinWindowFromID(hwnd, usItemID))

    BOOL XWPENTRY winhReplaceWindowText(HWND hwnd,
                                        const char *pcszSearch,
                                        const char *pcszReplaceWith);

    ULONG XWPENTRY winhCenteredDlgBox(HWND hwndParent, HWND hwndOwner,
                  PFNWP pfnDlgProc, HMODULE hmod, ULONG idDlg, PVOID pCreateParams);
    typedef ULONG XWPENTRY WINHCENTEREDDLGBOX(HWND hwndParent, HWND hwndOwner,
                  PFNWP pfnDlgProc, HMODULE hmod, ULONG idDlg, PVOID pCreateParams);
    typedef WINHCENTEREDDLGBOX *PWINHCENTEREDDLGBOX;

    ULONG XWPENTRY winhEnableControls(HWND hwndDlg,
                                      USHORT usIDFirst,
                                      USHORT usIDLast,
                                      BOOL fEnable);

    HWND XWPENTRY winhCreateStdWindow(HWND hwndFrameParent,
                                      PSWP pswpFrame,
                                      ULONG flFrameCreateFlags,
                                      ULONG ulFrameStyle,
                                      const char *pcszFrameTitle,
                                      ULONG ulResourcesID,
                                      const char *pcszClassClient,
                                      ULONG flStyleClient,
                                      ULONG ulID,
                                      PVOID pClientCtlData,
                                      PHWND phwndClient);

    HWND winhCreateObjectWindow(const char *pcszWindowClass,
                                PVOID pvCreateParam);

    VOID XWPENTRY winhRepaintWindows(HWND hwndParent);

    HMQ XWPENTRY winhFindMsgQueue(PID pid, TID tid, HAB* phab);

    VOID XWPENTRY winhFindPMErrorWindows(HWND *phwndHardError, HWND *phwndSysError);

    HWND XWPENTRY winhCreateFakeDesktop(HWND hwndSibling);

    BOOL XWPENTRY winhAssertWarp4Notebook(HWND hwndDlg,
                                 USHORT usIdThreshold,
                                 ULONG ulDownUnits);

    ULONG XWPENTRY winhDrawFormattedText(HPS hps, PRECTL prcl, const char *pcszText, ULONG flCmd);

    #ifdef INCL_WINSWITCHLIST
        PSWBLOCK XWPENTRY winhQuerySwitchList(HAB hab);
        typedef PSWBLOCK XWPENTRY WINHQUERYSWITCHLIST(HAB hab);
        typedef WINHQUERYSWITCHLIST *PWINHQUERYSWITCHLIST;
    #endif

    HWND XWPENTRY winhQueryTasklistWindow(VOID);
    typedef HWND XWPENTRY WINHQUERYTASKLISTWINDOW(VOID);
    typedef WINHQUERYTASKLISTWINDOW *PWINHQUERYTASKLISTWINDOW;

    VOID XWPENTRY winhKillTasklist(VOID);

    ULONG XWPENTRY winhQueryPendingSpoolJobs(VOID);

    VOID XWPENTRY winhSetNumLock(BOOL fState);

    /*
     *@@ winhQueryScreenCX:
     *      helper macro for getting the screen width.
     */

    #define winhQueryScreenCX() (WinQuerySysValue(HWND_DESKTOP, SV_CXSCREEN))

    /*
     *@@ winhQueryScreenCY:
     *      helper macro for getting the screen height.
     */

    #define winhQueryScreenCY() (WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN))

    /* ******************************************************************
     *
     *   WPS Class List helpers
     *
     ********************************************************************/

    PBYTE XWPENTRY winhQueryWPSClassList(VOID);

    PBYTE XWPENTRY winhQueryWPSClass(PBYTE pObjClass, const char *pszClass);

    APIRET XWPENTRY winhRegisterClass(const char* pcszClassName,
                                      const char* pcszModule,
                                      PSZ pszBuf,
                                      ULONG cbBuf);

    BOOL XWPENTRY winhIsClassRegistered(const char *pcszClass);

    ULONG XWPENTRY winhResetWPS(HAB hab);
#endif

#if __cplusplus
}
#endif

