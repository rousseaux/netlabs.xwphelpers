
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

    #define DLGERR_FIRST                        10000
    #define DLGERR_ROW_BEFORE_TABLE             (DLGERR_FIRST)
    #define DLGERR_CONTROL_BEFORE_ROW           (DLGERR_FIRST + 1)
    #define DLGERR_NULL_CTL_DEF                 (DLGERR_FIRST + 2)
    #define DLGERR_CANNOT_CREATE_FRAME          (DLGERR_FIRST + 3)
    #define DLGERR_INVALID_CODE                 (DLGERR_FIRST + 4)
    #define DLGERR_TABLE_NOT_CLOSED             (DLGERR_FIRST + 5)
    #define DLGERR_TOO_MANY_TABLES_CLOSED       (DLGERR_FIRST + 6)
    #define DLGERR_CANNOT_CREATE_CONTROL        (DLGERR_FIRST + 7)

    /* ******************************************************************
     *
     *   Structures
     *
     ********************************************************************/

    #define SZL_AUTOSIZE                (-1)

    #define CTL_COMMON_FONT             ((const char *)-1)

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

        SIZEL       szlControlProposed;
                // proposed size; a number of special flags are
                // available (per cx, cy field):
                // -- SZL_AUTOSIZE: determine size automatically.
                //    Works only for statics with SS_TEXT and
                //    SS_BITMAP.
                // This field is IGNORED if the CONTROLDEF appears
                // with a START_NEW_TABLE type in _DLGHITEM.

        ULONG       ulSpacing;          // spacing around control

        PVOID       pvCtlData;          // for WinCreateWindow

    } CONTROLDEF, *PCONTROLDEF;

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

    /* ******************************************************************
     *
     *   Macros
     *
     ********************************************************************/

    #define CONTROLDEF_GROUP(pcsz, id) { WC_STATIC, pcsz, \
            WS_VISIBLE | SS_GROUPBOX | DT_MNEMONIC, \
            id, CTL_COMMON_FONT, 0, { -1, -1 }, 0 }

    #define CONTROLDEF_TEXT(pcsz, id, cx, cy) { WC_STATIC, pcsz, \
            WS_VISIBLE | SS_TEXT | DT_LEFT | DT_VCENTER | DT_MNEMONIC, \
            id, CTL_COMMON_FONT,  0, {cx, cy}, 5 }

    #define CONTROLDEF_DEFPUSHBUTTON(pcsz, id, cx, cy) { WC_BUTTON, pcsz, \
            WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_DEFAULT, \
            id, CTL_COMMON_FONT, 0, {cx, cy}, 5 }

    #define CONTROLDEF_PUSHBUTTON(pcsz, id, cx, cy) { WC_BUTTON, pcsz, \
            WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, \
            id, CTL_COMMON_FONT, 0, {cx, cy}, 5 }

    #define CONTROLDEF_AUTOCHECKBOX(pcsz, id, cx, cy) { WC_BUTTON, pcsz, \
            WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, \
            id, CTL_COMMON_FONT, 0, { cx, cy }, 5 }

    #define CONTROLDEF_FIRST_AUTORADIO(pcsz, id, cx, cy) { WC_BUTTON, pcsz, \
            WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON | WS_GROUP, \
            id, CTL_COMMON_FONT, 0, { cx, cy }, 5 }

    #define CONTROLDEF_NEXT_AUTORADIO(pcsz, id, cx, cy) { WC_BUTTON, pcsz, \
            WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON, \
            id, CTL_COMMON_FONT, 0, { cx, cy }, 5 }

    #define CONTROLDEF_ENTRYFIELD(pcsz, id, cx, cy) { WC_ENTRYFIELD, pcsz, \
            WS_VISIBLE | WS_TABSTOP | ES_MARGIN, \
            id, CTL_COMMON_FONT, 0, { cx, cy }, 5 }

    #define CONTROLDEF_SPINBUTTON(id, cx, cy) { WC_SPINBUTTON, NULL, \
            WS_VISIBLE | WS_TABSTOP | SPBS_MASTER | SPBS_NUMERICONLY | SPBS_JUSTCENTER | SPBS_FASTSPIN, \
            id, CTL_COMMON_FONT, 0, {cx, cy}, 5 }

    /* ******************************************************************
     *
     *   APIs
     *
     ********************************************************************/

    APIRET XWPENTRY dlghCreateDlg(HWND *phwndDlg,
                                  HWND hwndOwner,
                                  ULONG flCreateFlags,
                                  PFNWP pfnwpDialogProc,
                                  const char *pcszDlgTitle,
                                  PDLGHITEM paDlgItems,
                                  ULONG cDlgItems,
                                  PVOID pCreateParams,
                                  const char *pcszControlsFont);
    typedef APIRET XWPENTRY DLGHCREATEDLG(HWND *phwndDlg,
                                          HWND hwndOwner,
                                          ULONG flCreateFlags,
                                          PFNWP pfnwpDialogProc,
                                          const char *pcszDlgTitle,
                                          PDLGHITEM paDlgItems,
                                          ULONG cDlgItems,
                                          PVOID pCreateParams,
                                          const char *pcszControlsFont);
    typedef DLGHCREATEDLG *PDLGHCREATEDLG;

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
                                const char *pcszTitle,
                                const char *pcszMessage,
                                ULONG flFlags,
                                const char *pcszFont,
                                const MSGBOXSTRINGS *pStrings,
                                PULONG pulAlarmFlag);

    ULONG dlghMessageBox(HWND hwndOwner,
                         HPOINTER hptrIcon,
                         const char *pcszTitle,
                         const char *pcszMessage,
                         ULONG flFlags,
                         const char *pcszFont,
                         const MSGBOXSTRINGS *pStrings);

    VOID dlghSetPrevFocus(PVOID pvllWindows);

    VOID dlghSetNextFocus(PVOID pvllWindows);

    HWND dlghProcessMnemonic(PVOID pvllWindows,
                             USHORT usch);

    BOOL dlghEnter(PVOID pvllWindows);

#endif

#if __cplusplus
}
#endif

