
/*
 *@@sourcefile dialog.h:
 *      header file for dialog.c. See remarks there.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@added V0.9.9 (2001-04-01) [umoeller]
 *@@include <os2.h>
 *@@include #include "helpers\linklist.h"           // for mnemonic helpers
 *@@include #include "helpers\dialog.h"
 */

/*      Copyright (C) 2001 Ulrich M”ller.
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

#ifndef DIALOG_HEADER_INCLUDED
    #define DIALOG_HEADER_INCLUDED

    #ifndef NULL_POINT
        #define NULL_POINT {0, 0}
    #endif

    /* ******************************************************************
     *
     *   Error codes
     *
     ********************************************************************/

    #define ERROR_DLG_FIRST                     43000

    #define DLGERR_ROW_BEFORE_TABLE             (ERROR_DLG_FIRST)
    #define DLGERR_CONTROL_BEFORE_ROW           (ERROR_DLG_FIRST + 1)
    #define DLGERR_NULL_CTL_DEF                 (ERROR_DLG_FIRST + 2)
    #define DLGERR_CANNOT_CREATE_FRAME          (ERROR_DLG_FIRST + 3)
    #define DLGERR_INVALID_CODE                 (ERROR_DLG_FIRST + 4)
    #define DLGERR_TABLE_NOT_CLOSED             (ERROR_DLG_FIRST + 5)
    #define DLGERR_TOO_MANY_TABLES_CLOSED       (ERROR_DLG_FIRST + 6)
    #define DLGERR_CANNOT_CREATE_CONTROL        (ERROR_DLG_FIRST + 7)
    #define DLGERR_ARRAY_TOO_SMALL              (ERROR_DLG_FIRST + 8)
    #define DLGERR_INVALID_CONTROL_TITLE        (ERROR_DLG_FIRST + 9)
    #define DLGERR_INVALID_STATIC_BITMAP        (ERROR_DLG_FIRST + 10)

    #define ERROR_DLG_LAST                      (ERROR_DLG_FIRST + 10)

    /* ******************************************************************
     *
     *   Structures
     *
     ********************************************************************/

    #define SZL_AUTOSIZE                (-1)

    #define CTL_COMMON_FONT             ((PCSZ)-1)

    #define ROW_VALIGN_MASK             0x0003
    #define ROW_VALIGN_BOTTOM           0x0000
    #define ROW_VALIGN_CENTER           0x0001
    #define ROW_VALIGN_TOP              0x0002

    /*
     *@@ CONTROLDEF:
     *      defines a single control. Used
     *      with the TYPE_CONTROL_DEF type in
     *      DLGHITEM.
     *
     *@@changed V0.9.12 (2001-05-31) [umoeller]: added control data
     */

    typedef struct _CONTROLDEF
    {
        const char  *pcszClass;         // registered PM window class
        const char  *pcszText;
                // window text (class-specific)
                // special hacks:
                // -- For WS_STATIC with SS_BITMAP or SS_ICON set,
                //    you may specify the exact HPOINTER here.
                //    The dlg routine will then subclass the static
                //

        ULONG       flStyle;            // standard window styles

        USHORT      usID;               // dlg item ID

        const char  *pcszFont;          // font presparam, or NULL for no presparam,
                                        // or CTL_COMMON_FONT for standard dialog
                                        // font specified on input to dlghCreateDlg

        USHORT      usAdjustPosition;
                // flags for winhAdjustControls; any combination of
                // XAC_MOVEX, XAC_MOVEY, XAC_SIZEX, XAC_SIZEY
                // @@todo not implemented yet

        SIZEL       szlControlProposed;
                // proposed size; a number of special flags are
                // available (per cx, cy field):
                // -- SZL_AUTOSIZE (-1): determine size automatically.
                //    Works only for statics with SS_TEXT and
                //    SS_BITMAP.
                // -- Any other _negative_ value is considered a
                //    percentage of the largest row width in the
                //    table. For example, -50 would mean 50% of
                //    the largest row in the table. This is valid
                //    for the CX field only.
                // If the CONTROLDEF appears with a START_NEW_TABLE
                // type in _DLGHITEM (to specify a group table)
                // and they are not SZL_AUTOSIZE, they specify the
                // size of the inner table of the group (to override
                // the automatic formatting). Note that the dialog
                // formatter adds COMMON_SPACING to both the left and
                // the right of the table to center the table in the
                // PM group control, so the actual group size will
                // be the specified size + (2 * COMMON_SPACING).

        ULONG       ulSpacing;          // spacing around control

        PVOID       pvCtlData;          // for WinCreateWindow

    } CONTROLDEF, *PCONTROLDEF;

    typedef const struct _CONTROLDEF *PCCONTROLDEF;

    /*
     *@@ DLGHITEMTYPE:
     *
     */

    typedef enum _DLGHITEMTYPE
    {
        TYPE_START_NEW_TABLE,       // beginning of a new table; may nest
        TYPE_START_NEW_ROW,         // beginning of a new row in a table
        TYPE_CONTROL_DEF,           // control definition
        TYPE_END_TABLE              // end of a table
    } DLGHITEMTYPE;

    // a few handy macros for defining templates

    #define START_TABLE         { TYPE_START_NEW_TABLE, 0 }

    // this macro is slightly insane, but performs type checking
    // in case the user gives a pointer which is not of CONTROLDEF
    #define START_GROUP_TABLE(pDef)   { TYPE_START_NEW_TABLE, \
                (   (ULONG)(&(pDef)->pcszClass)   ) }

    #define END_TABLE           { TYPE_END_TABLE, 0 }

    #define START_ROW(fl)       { TYPE_START_NEW_ROW, fl }

    #define CONTROL_DEF(pDef)   { TYPE_CONTROL_DEF, \
                (   (ULONG)(&(pDef)->pcszClass)   ) }

    /*
     *@@ DLGHITEM:
     *      dialog format array item.
     *
     *      An array of these must be passed to dlghCreateDlg
     *      to tell it what controls the dialog contains.
     *      See dlghCreateDlg for details.
     *
     */

    typedef struct _DLGHITEM
    {
        DLGHITEMTYPE    Type;
                // one of:
                // TYPE_START_NEW_TABLE,        // beginning of a new table
                // TYPE_START_NEW_ROW,          // beginning of a new row in a table
                // TYPE_CONTROL_DEF             // control definition
                // TYPE_END_TABLE               // end of table

        ULONG           ulData;
                // -- with TYPE_START_NEW_TABLE: if NULL, this starts
                //          an invisible table (for formatting only).
                //          Otherwise a _CONTROLDEF pointer to specify
                //          a control to be produced around the table.
                //          For example, you can specify a WC_STATIC
                //          with SS_GROUPBOX to create a group around
                //          the table.
                // -- with TYPE_START_NEW_ROW: ROW_* formatting flags.
                // -- with TYPE_CONTROL_DEF: _CONTROLDEF pointer to a control definition
    } DLGHITEM, *PDLGHITEM;

    typedef const struct _DLGHITEM *PCDLGHITEM;

    /* ******************************************************************
     *
     *   Macros
     *
     ********************************************************************/

    #define LOAD_STRING     ((PCSZ)-1)

    #define COMMON_SPACING              3

    #define PM_GROUP_SPACING_X          16
    #define PM_GROUP_SPACING_TOP        16

    // the following require INCL_WINSTATICS

    #define CONTROLDEF_GROUP(pcsz, id, cx, cy) { WC_STATIC, pcsz, \
            WS_VISIBLE | SS_GROUPBOX | DT_MNEMONIC, \
            id, CTL_COMMON_FONT, 0, { cx, cy }, 0 }

    #define CDEF_GROUP_AUTO(id) CONTROLDEF_GROUP(LOAD_STRING, id, -1, -1)

    #define CONTROLDEF_TEXT(pcsz, id, cx, cy) { WC_STATIC, pcsz, \
            WS_VISIBLE | SS_TEXT | DT_LEFT | DT_VCENTER | DT_MNEMONIC, \
            id, CTL_COMMON_FONT,  0, {cx, cy}, COMMON_SPACING }

    #define CONTROLDEF_TEXT_CENTER(pcsz, id, cx, cy) { WC_STATIC, pcsz, \
            WS_VISIBLE | SS_TEXT | DT_CENTER | DT_VCENTER | DT_MNEMONIC, \
            id, CTL_COMMON_FONT,  0, {cx, cy}, COMMON_SPACING }

    #define CONTROLDEF_TEXT_WORDBREAK(pcsz, id, cx) { WC_STATIC, pcsz, \
            WS_VISIBLE | SS_TEXT | DT_LEFT | DT_TOP | DT_WORDBREAK, \
            id, CTL_COMMON_FONT,  0, {cx, -1}, COMMON_SPACING }

    #define CONTROLDEF_ICON(hptr, id) { WC_STATIC, (PCSZ)(hptr), \
            WS_VISIBLE | SS_ICON | DT_LEFT | DT_VCENTER, \
            id, CTL_COMMON_FONT, 0, {-1, -1}, COMMON_SPACING }

    #define CONTROLDEF_BITMAP(hbm, id) { WC_STATIC, (PCSZ)(hbm), \
            WS_VISIBLE | SS_BITMAP | DT_LEFT | DT_VCENTER, \
            id, CTL_COMMON_FONT, 0, {-1, -1}, COMMON_SPACING }

    // the following require INCL_WINBUTTONS

    #define CONTROLDEF_DEFPUSHBUTTON(pcsz, id, cx, cy) { WC_BUTTON, pcsz, \
            WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_DEFAULT, \
            id, CTL_COMMON_FONT, 0, {cx, cy}, COMMON_SPACING }

    #define CONTROLDEF_PUSHBUTTON(pcsz, id, cx, cy) { WC_BUTTON, pcsz, \
            WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, \
            id, CTL_COMMON_FONT, 0, {cx, cy}, COMMON_SPACING }

    #define CONTROLDEF_DEFNOFOCUSBUTTON(pcsz, id, cx, cy) { WC_BUTTON, pcsz, \
            WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_DEFAULT | BS_NOPOINTERFOCUS, \
            id, CTL_COMMON_FONT, 0, {cx, cy}, COMMON_SPACING }

    #define CONTROLDEF_NOFOCUSBUTTON(pcsz, id, cx, cy) { WC_BUTTON, pcsz, \
            WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_NOPOINTERFOCUS, \
            id, CTL_COMMON_FONT, 0, {cx, cy}, COMMON_SPACING }

    #define CONTROLDEF_HELPPUSHBUTTON(pcsz, id, cx, cy) { WC_BUTTON, pcsz, \
            WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_HELP, \
            id, CTL_COMMON_FONT, 0, {cx, cy}, COMMON_SPACING }

    #define CONTROLDEF_AUTOCHECKBOX(pcsz, id, cx, cy) { WC_BUTTON, pcsz, \
            WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, \
            id, CTL_COMMON_FONT, 0, { cx, cy }, COMMON_SPACING }

    #define CDEF_AUTOCB_AUTO(id) CONTROLDEF_AUTOCHECKBOX(LOAD_STRING, id, -1, -1)

    #define CONTROLDEF_FIRST_AUTORADIO(pcsz, id, cx, cy) { WC_BUTTON, pcsz, \
            WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON | WS_GROUP, \
            id, CTL_COMMON_FONT, 0, { cx, cy }, COMMON_SPACING }

    #define CONTROLDEF_NEXT_AUTORADIO(pcsz, id, cx, cy) { WC_BUTTON, pcsz, \
            WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON, \
            id, CTL_COMMON_FONT, 0, { cx, cy }, COMMON_SPACING }

    // the following require INCL_WINENTRYFIELDS

    #define CONTROLDEF_ENTRYFIELD(pcsz, id, cx, cy) { WC_ENTRYFIELD, pcsz, \
            WS_VISIBLE | WS_TABSTOP | ES_MARGIN | ES_AUTOSCROLL, \
            id, CTL_COMMON_FONT, 0, { cx, cy }, COMMON_SPACING }

    #define CONTROLDEF_ENTRYFIELD_RO(pcsz, id, cx, cy) { WC_ENTRYFIELD, pcsz, \
            WS_VISIBLE | WS_TABSTOP | ES_MARGIN | ES_READONLY | ES_AUTOSCROLL, \
            id, CTL_COMMON_FONT, 0, { cx, cy }, COMMON_SPACING }

    // the following require INCL_WINMLE

    #define CONTROLDEF_MLE(pcsz, id, cx, cy) { WC_MLE, pcsz, \
            WS_VISIBLE | WS_TABSTOP | MLS_BORDER | MLS_IGNORETAB | MLS_WORDWRAP, \
            id, CTL_COMMON_FONT, 0, { cx, cy }, COMMON_SPACING }

    // the following require INCL_WINLISTBOXES

    #define CONTROLDEF_LISTBOX(id, cx, cy) { WC_LISTBOX, NULL, \
            WS_VISIBLE | WS_TABSTOP | LS_HORZSCROLL | LS_NOADJUSTPOS, \
            id, CTL_COMMON_FONT, 0, { cx, cy }, COMMON_SPACING }

    // the following require INCL_WINLISTBOXES and INCL_WINENTRYFIELDS

    #define CONTROLDEF_DROPDOWN(id, cx, cy) { WC_COMBOBOX, NULL, \
            WS_VISIBLE | WS_TABSTOP | LS_HORZSCROLL | CBS_DROPDOWN, \
            id, CTL_COMMON_FONT, 0, { cx, cy }, COMMON_SPACING }

    #define CONTROLDEF_DROPDOWNLIST(id, cx, cy) { WC_COMBOBOX, NULL, \
            WS_VISIBLE | WS_TABSTOP | LS_HORZSCROLL | CBS_DROPDOWNLIST, \
            id, CTL_COMMON_FONT, 0, { cx, cy }, COMMON_SPACING }

    // the following require INCL_WINSTDSPIN

    #define CONTROLDEF_SPINBUTTON(id, cx, cy) { WC_SPINBUTTON, NULL, \
            WS_VISIBLE | WS_TABSTOP | SPBS_MASTER | SPBS_NUMERICONLY | SPBS_JUSTCENTER | SPBS_FASTSPIN, \
            id, CTL_COMMON_FONT, 0, {cx, cy}, COMMON_SPACING }

    // the following require INCL_WINSTDCNR

    #define CONTROLDEF_CONTAINER(id, cx, cy) { WC_CONTAINER, NULL, \
            WS_VISIBLE | WS_TABSTOP | 0, \
            id, CTL_COMMON_FONT, 0, {cx, cy}, COMMON_SPACING }

    // the following require INCL_WINSTDSLIDER

    #define CONTROLDEF_SLIDER(id, cx, cy, pctldata) { WC_SLIDER, NULL, \
            WS_VISIBLE | WS_TABSTOP | WS_GROUP | SLS_HORIZONTAL | SLS_PRIMARYSCALE1 \
            | SLS_BUTTONSRIGHT | SLS_SNAPTOINCREMENT, \
            id, CTL_COMMON_FONT, 0, {cx, cy}, COMMON_SPACING, pctldata }

    /* ******************************************************************
     *
     *   Dialog formatter entry points
     *
     ********************************************************************/

    #ifndef FCF_CLOSEBUTTON
        #define FCF_CLOSEBUTTON            0x04000000L // toolkit 4 only
    #endif

    #define FCF_FIXED_DLG       FCF_TITLEBAR | FCF_SYSMENU | FCF_DLGBORDER | FCF_NOBYTEALIGN | FCF_CLOSEBUTTON
    #define FCF_SIZEABLE_DLG    FCF_TITLEBAR | FCF_SYSMENU | FCF_SIZEBORDER | FCF_NOBYTEALIGN | FCF_CLOSEBUTTON

    APIRET XWPENTRY dlghCreateDlg(HWND *phwndDlg,
                                  HWND hwndOwner,
                                  ULONG flCreateFlags,
                                  PFNWP pfnwpDialogProc,
                                  PCSZ pcszDlgTitle,
                                  PCDLGHITEM paDlgItems,
                                  ULONG cDlgItems,
                                  PVOID pCreateParams,
                                  PCSZ pcszControlsFont);
    typedef APIRET XWPENTRY DLGHCREATEDLG(HWND *phwndDlg,
                                          HWND hwndOwner,
                                          ULONG flCreateFlags,
                                          PFNWP pfnwpDialogProc,
                                          PCSZ pcszDlgTitle,
                                          PCDLGHITEM paDlgItems,
                                          ULONG cDlgItems,
                                          PVOID pCreateParams,
                                          PCSZ pcszControlsFont);
    typedef DLGHCREATEDLG *PDLGHCREATEDLG;

    #define DFFL_CREATECONTROLS     0x0002

    APIRET dlghFormatDlg(HWND hwndDlg,
                         PCDLGHITEM paDlgItems,
                         ULONG cDlgItems,
                         PCSZ pcszControlsFont,
                         ULONG flFlags,
                         PSIZEL pszlClient,
                         PVOID *ppllControls);

    VOID dlghResizeFrame(HWND hwndDlg,
                         PSIZEL pszlClient);

    /* ******************************************************************
     *
     *   Dialog arrays
     *
     ********************************************************************/

    /*
     *@@ DLGARRAY:
     *      dialog array structure used with dlghCreateArray.
     *      See remarks there.
     *
     *@@added V0.9.16 (2001-10-15) [umoeller]
     */

    typedef struct _DLGARRAY
    {
        DLGHITEM    *paDlgItems;        // array of DLGHITEM's, allocated once
                                        // by dlghCreateArray
        ULONG       cDlgItemsMax,       // copied from dlghCreateArray
                    cDlgItemsNow;       // initially 0, raised after each
                                        // dlghAppendToArray; pass this to the
                                        // dialog formatter
    } DLGARRAY, *PDLGARRAY;

    APIRET dlghCreateArray(ULONG cMaxItems,
                           PDLGARRAY *ppArray);

    APIRET dlghFreeArray(PDLGARRAY *ppArray);

    APIRET dlghAppendToArray(PDLGARRAY pArray,
                             PCDLGHITEM paItems,
                             ULONG cItems);

    /* ******************************************************************
     *
     *   Standard dialogs
     *
     ********************************************************************/

    /*
     *@@ MSGBOXSTRINGS:
     *
     *@@added V0.9.13 (2001-06-21) [umoeller]
     */

    typedef struct _MSGBOXSTRINGS
    {
        const char      *pcszYes,           // "~Yes"
                        *pcszNo,            // "~No"
                        *pcszOK,            // "~OK"
                        *pcszCancel,        // "~Cancel"
                        *pcszAbort,         // "~Abort"
                        *pcszRetry,         // "~Retry"
                        *pcszIgnore,        // "~Ignore"
                        *pcszEnter,
                        *pcszYesToAll;      // "Yes to ~all"
    } MSGBOXSTRINGS, *PMSGBOXSTRINGS;

    /* the following are in os2.h somewhere:
    #define MB_OK                      0x0000
    #define MB_OKCANCEL                0x0001
    #define MB_RETRYCANCEL             0x0002
    #define MB_ABORTRETRYIGNORE        0x0003
    #define MB_YESNO                   0x0004
    #define MB_YESNOCANCEL             0x0005
    #define MB_CANCEL                  0x0006
    #define MB_ENTER                   0x0007
    #define MB_ENTERCANCEL             0x0008 */
    // we add:
    #define MB_YES_YES2ALL_NO          0x0009

    /* the following are in os2.h somewhere:
    #define MBID_OK                    1
    #define MBID_CANCEL                2
    #define MBID_ABORT                 3
    #define MBID_RETRY                 4
    #define MBID_IGNORE                5
    #define MBID_YES                   6
    #define MBID_NO                    7
    #define MBID_HELP                  8
    #define MBID_ENTER                 9 */
    // we add:
    #define MBID_YES2ALL               10

    APIRET dlghCreateMessageBox(HWND *phwndDlg,
                                HWND hwndOwner,
                                HPOINTER hptrIcon,
                                PCSZ pcszTitle,
                                PCSZ pcszMessage,
                                ULONG flFlags,
                                PCSZ pcszFont,
                                const MSGBOXSTRINGS *pStrings,
                                PULONG pulAlarmFlag);

    ULONG dlghMessageBox(HWND hwndOwner,
                         HPOINTER hptrIcon,
                         PCSZ pcszTitle,
                         PCSZ pcszMessage,
                         ULONG flFlags,
                         PCSZ pcszFont,
                         const MSGBOXSTRINGS *pStrings);

    #define TEBF_REMOVETILDE            0x0001
    #define TEBF_REMOVEELLIPSE          0x0002
    #define TEBF_SELECTALL              0x0004

    PSZ dlghTextEntryBox(HWND hwndOwner,
                         PCSZ pcszTitle,
                         PCSZ pcszDescription,
                         PCSZ pcszDefault,
                         PCSZ pcszOK,
                         PCSZ pcszCancel,
                         ULONG ulMaxLen,
                         ULONG fl,
                         PCSZ pcszFont);

    /* ******************************************************************
     *
     *   Dialog input handlers
     *
     ********************************************************************/

    VOID dlghSetPrevFocus(PVOID pvllWindows);

    VOID dlghSetNextFocus(PVOID pvllWindows);

    HWND dlghProcessMnemonic(PVOID pvllWindows,
                             USHORT usch);

    BOOL dlghEnter(PVOID pvllWindows);

#endif

#if __cplusplus
}
#endif

