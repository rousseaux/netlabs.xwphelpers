
/*
 *@@sourcefile winh.c:
 *      contains Presentation Manager helper functions that are
 *      independent of a single application, i.e. these can be
 *      used w/out the rest of the XWorkplace source in any PM
 *      program.
 *
 *      Usage: All PM programs.
 *
 *      Function prefixes (new with V0.81):
 *      --  winh*   Win (Presentation Manager) helper functions
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@header "helpers\winh.h"
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

#define INCL_DOSDEVIOCTL
#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_WIN
#define INCL_GPI

// spooler #include's
#define INCL_BASE
#define INCL_SPL
#define INCL_SPLDOSPRINT
#define INCL_SPLERRORS

#include <os2.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "setup.h"                      // code generation and debugging options

#include "helpers\dosh.h"
#include "helpers\winh.h"
#include "helpers\prfh.h"
#include "helpers\gpih.h"
#include "helpers\stringh.h"
#include "helpers\undoc.h"
#include "helpers\xstring.h"            // extended string helpers

/*
 *@@category: Helpers\PM helpers\Menu helpers
 */

/* ******************************************************************
 *                                                                  *
 *   Menu helpers                                                   *
 *                                                                  *
 ********************************************************************/

/*
 *@@ winhInsertMenuItem:
 *      this inserts one one menu item into a given menu.
 *
 *      Returns the return value of the MM_INSERTITEM msg:
 *      --  MIT_MEMERROR:    space allocation for menu item failed
 *      --  MIT_ERROR:       other error
 *      --  other:           zero-based index of new item in menu.
 */

SHORT winhInsertMenuItem(HWND hwndMenu,     // in:  menu to insert item into
                         SHORT iPosition,   // in:  zero-based index of where to
                                            //      insert or MIT_END
                         SHORT sItemId,     // in:  ID of new menu item
                         PSZ pszItemTitle,  // in:  title of new menu item
                         SHORT afStyle,
                            // in:  MIS_* style flags.
                            // Valid menu item styles are:
                            // --  MIS_SUBMENU
                            // --  MIS_SEPARATOR
                            // --  MIS_BITMAP: the display object is a bit map.
                            // --  MIS_TEXT: the display object is a text string.
                            // --  MIS_BUTTONSEPARATOR:
                            //          The item is a menu button. Any menu can have zero,
                            //          one, or two items of this type.  These are the last
                            //          items in a menu and are automatically displayed after
                            //          a separator bar. The user cannot move the cursor to
                            //          these items, but can select them with the pointing
                            //          device or with the appropriate key.
                            // --  MIS_BREAK: the item begins a new row or column.
                            // --  MIS_BREAKSEPARATOR:
                            //          Same as MIS_BREAK, except that it draws a separator
                            //          between rows or columns of a pull-down menu.
                            //          This style can only be used within a submenu.
                            // --  MIS_SYSCOMMAND:
                            //          menu posts a WM_SYSCOMMAND message rather than a
                            //          WM_COMMAND message.
                            // --  MIS_OWNERDRAW:
                            //          WM_DRAWITEM and WM_MEASUREITEM notification messages
                            //          are sent to the owner to draw the item or determine its size.
                            // --  MIS_HELP:
                            //          menu posts a WM_HELP message rather than a
                            //          WM_COMMAND message.
                            // --  MIS_STATIC
                            //          This type of item exists for information purposes only.
                            //          It cannot be selected with the pointing device or
                            //          keyboard.
                         SHORT afAttr)
                            // in:  MIA_* attribute flags
                            // Valid menu item attributes (afAttr) are:
                            // --  MIA_HILITED: if and only if, the item is selected.
                            // --  MIA_CHECKED: a check mark appears next to the item (submenu only).
                            // --  MIA_DISABLED: item is disabled and cannot be selected.
                            //         The item is drawn in a disabled state (gray).
                            // --  MIA_FRAMED: a frame is drawn around the item (top-level menu only).
                            // --  MIA_NODISMISS:
                            //          if the item is selected, the submenu remains down. A menu
                            //          with this attribute is not hidden until the  application
                            //          or user explicitly does so, for example by selecting either
                            //          another menu on the action bar or by pressing the escape key.
{
    MENUITEM mi;
    SHORT    src = MIT_ERROR;

    mi.iPosition = iPosition;
    mi.afStyle = afStyle;
    mi.afAttribute = afAttr;
    mi.id = sItemId;
    mi.hwndSubMenu = 0;
    mi.hItem = 0;
    src = SHORT1FROMMR(WinSendMsg(hwndMenu,
                                  MM_INSERTITEM,
                                  (MPARAM)&mi,
                                  (MPARAM)pszItemTitle));
    return (src);
}

/*
 *@@ winhInsertSubmenu:
 *      this inserts a submenu into a given menu and, if
 *      sItemId != 0, inserts one item into this new submenu also.
 *
 *      See winhInsertMenuItem for valid menu item styles and
 *      attributes.
 *
 *      Returns the HWND of the new submenu.
 */

HWND winhInsertSubmenu(HWND hwndMenu,       // in: menu to add submenu to
                       ULONG iPosition,     // in: index where to add submenu or MIT_END
                       SHORT sMenuId,       // in: menu ID of new submenu
                       PSZ pszSubmenuTitle, // in: title of new submenu
                       USHORT afMenuStyle,  // in: MIS* style flags for submenu;
                                            // MIS_SUBMENU will always be added
                       SHORT sItemId,       // in: ID of first item to add to submenu;
                                            // if 0, no first item is inserted
                       PSZ pszItemTitle,    // in: title of this item
                                            // (if sItemID != 0)
                       USHORT afItemStyle,  // in: style flags for this item, e.g. MIS_TEXT
                                            // (this is ignored if sItemID == 0)
                       USHORT afAttribute)  // in: attributes for this item, e.g. MIA_DISABLED
                                            // (this is ignored if sItemID == 0)
{
    MENUITEM mi;
    SHORT    src = MIT_ERROR;
    HWND     hwndNewMenu;

    // create new, empty menu
    hwndNewMenu = WinCreateMenu(hwndMenu,
                                NULL); // no menu template
    if (hwndNewMenu)
    {
        // add "submenu item" to this empty menu;
        // for some reason, PM always needs submenus
        // to be a menu item
        mi.iPosition = iPosition;
        mi.afStyle = afMenuStyle | MIS_SUBMENU;
        mi.afAttribute = 0;
        mi.id = sMenuId;
        mi.hwndSubMenu = hwndNewMenu;
        mi.hItem = 0;
        src = SHORT1FROMMR(WinSendMsg(hwndMenu, MM_INSERTITEM, (MPARAM)&mi, (MPARAM)pszSubmenuTitle));
        if (    (src != MIT_MEMERROR)
            &&  (src != MIT_ERROR)
           )
        {
            // set the new menu's ID to the same as the
            // submenu item
            WinSetWindowUShort(hwndNewMenu, QWS_ID, sMenuId);

            if (sItemId)
            {
                // item id given: insert first menu item also
                mi.iPosition = 0;
                mi.afStyle = afItemStyle;
                mi.afAttribute = afAttribute;
                mi.id = sItemId;
                mi.hwndSubMenu = 0;
                mi.hItem = 0;
                WinSendMsg(hwndNewMenu,
                           MM_INSERTITEM,
                           (MPARAM)&mi,
                           (MPARAM)pszItemTitle);
            }
        }
    }
    return (hwndNewMenu);
}

/*
 *@@ winhInsertMenuSeparator:
 *      this inserts a separator into a given menu at
 *      the given position (which may be MIT_END);
 *      returns the position at which the item was
 *      inserted.
 */

SHORT winhInsertMenuSeparator(HWND hMenu,       // in: menu to add separator to
                              SHORT iPosition,  // in: index where to add separator or MIT_END
                              SHORT sId)        // in: separator menu ID (doesn't really matter)
{
    MENUITEM mi;
    mi.iPosition = iPosition;
    mi.afStyle = MIS_SEPARATOR;             // append separator
    mi.afAttribute = 0;
    mi.id = sId;
    mi.hwndSubMenu = 0;
    mi.hItem = 0;

    return (SHORT1FROMMR(WinSendMsg(hMenu,
                                    MM_INSERTITEM,
                                    (MPARAM)&mi,
                                    (MPARAM)"")));
}

/*
 *@@ winhQueryMenuItemText:
 *      this returns a menu item text as a PSZ
 *      to a newly allocated buffer or NULL if
 *      not found.
 *
 *      If something != NULL is returned, you
 *      should free() the buffer afterwards.
 *
 *      Use the WinSetMenuItemText macro to
 *      set the menu item text.
 */

PSZ winhQueryMenuItemText(HWND hwndMenu,
                          USHORT usItemID)  // in: menu item ID (not index)
{
    PSZ     prc = NULL;

    SHORT sLength = SHORT1FROMMR(WinSendMsg(hwndMenu, MM_QUERYITEMTEXTLENGTH,
                                           (MPARAM)(ULONG)usItemID,
                                           (MPARAM)NULL));
    if (sLength)
    {
        prc = (PSZ)malloc(sLength + 1);
        WinSendMsg(hwndMenu,
                   MM_QUERYITEMTEXT,
                   MPFROM2SHORT(usItemID, sLength + 1),
                   (MPARAM)prc);
    }

    return (prc);
}

/*
 *@@ winhAppend2MenuItemText:
 *
 *@@added V0.9.2 (2000-03-08) [umoeller]
 */

BOOL winhAppend2MenuItemText(HWND hwndMenu,
                             USHORT usItemID,  // in: menu item ID (not index)
                             const char *pcszAppend, // in: text to append
                             BOOL fTab)    // in: if TRUE, add \t before pcszAppend
{
    BOOL brc = FALSE;
    CHAR szItemText[400];
    if (WinSendMsg(hwndMenu,
                   MM_QUERYITEMTEXT,
                   MPFROM2SHORT(usItemID,
                                sizeof(szItemText)),
                   (MPARAM)szItemText))
    {
        // text copied:
        if (fTab)
        {
            if (strchr(szItemText, '\t'))
                // we already have a tab:
                strcat(szItemText, " ");
            else
                strcat(szItemText, "\t");
        }
        strcat(szItemText, pcszAppend);

        brc = (BOOL)WinSendMsg(hwndMenu,
                               MM_SETITEMTEXT,
                               MPFROMSHORT(usItemID),
                               (MPARAM)szItemText);
    }

    return (brc);
}

/*
 *@@ winhMenuRemoveEllipse:
 *      removes a "..." substring from a menu item
 *      title, if found. This is useful if confirmations
 *      have been turned off for a certain menu item, which
 *      should be reflected in the menu.
 */

VOID winhMenuRemoveEllipse(HWND hwndMenu,
                           USHORT usItemId)    // in:  item to remove "..." from
{
    CHAR szBuf[255];
    CHAR *p;
    WinSendMsg(hwndMenu,
               MM_QUERYITEMTEXT,
               MPFROM2SHORT(usItemId, sizeof(szBuf)-1),
               (MPARAM)&szBuf);
    if ((p = strstr(szBuf, "...")))
        strcpy(p, p+3);
    WinSendMsg(hwndMenu,
               MM_SETITEMTEXT,
               MPFROMSHORT(usItemId),
               (MPARAM)&szBuf);
}

/*
 *@@ winhQueryItemUnderMouse:
 *      this queries the menu item which corresponds
 *      to the given mouse coordinates.
 *      Returns the ID of the menu item and stores its
 *      rectangle in *prtlItem; returns (-1) upon errors.
 */

SHORT winhQueryItemUnderMouse(HWND hwndMenu,      // in: menu handle
                              POINTL *pptlMouse,  // in: mouse coordinates
                              RECTL *prtlItem)    // out: rectangle of menu item
{
    SHORT   s, sItemId, sItemCount;
    HAB     habDesktop = WinQueryAnchorBlock(HWND_DESKTOP);

    sItemCount = SHORT1FROMMR(WinSendMsg(hwndMenu, MM_QUERYITEMCOUNT, MPNULL, MPNULL));

    for (s = 0;
         s <= sItemCount;
         s++)
    {
        sItemId = SHORT1FROMMR(WinSendMsg(hwndMenu,
                                          MM_ITEMIDFROMPOSITION,
                                          (MPARAM)(ULONG)s, MPNULL));
        WinSendMsg(hwndMenu,
                   MM_QUERYITEMRECT,
                   MPFROM2SHORT(sItemId, FALSE),
                   (MPARAM)prtlItem);
        if (WinPtInRect(habDesktop, prtlItem, pptlMouse))
            return (sItemId);
    }
    /* sItemId = (SHORT)WinSendMsg(hwndMenu, MM_ITEMIDFROMPOSITION, (MPARAM)(sItemCount-1), MPNULL);
    return (sItemId); */
    return (-1); // error: no valid menu item
}

/*
 *@@category: Helpers\PM helpers\Slider helpers
 */

/* ******************************************************************
 *                                                                  *
 *   Slider helpers                                                 *
 *                                                                  *
 ********************************************************************/

/*
 *@@ winhReplaceWithLinearSlider:
 *      this destroys the control with the ID ulID in hwndDlg
 *      and creates a linear slider at the same position with the
 *      same ID (effectively replacing it).
 *
 *      This is needed because the IBM dialog editor (DLGEDIT.EXE)
 *      keeps crashing when creating sliders. So the way to do
 *      this easily is to create some other control with DLGEDIT
 *      where the slider should be later and call this function
 *      on that control when the dialog is initialized.
 *
 *      You need to specify _one_ of the following with ulSliderStyle:
 *      -- SLS_HORIZONTAL: horizontal slider (default)
 *      -- SLS_VERTICAL: vertical slider
 *
 *      plus _one_ additional common slider style for positioning:
 *      -- for horizontal sliders: SLS_BOTTOM, SLS_CENTER, or SLS_TOP
 *      -- for vertical sliders: SLS_LEFT, SLS_CENTER, or SLS_RIGHT
 *
 *      Additional common slider styles are:
 *      -- SLS_PRIMARYSCALE1: determines the location of the scale
 *                  on the slider shaft by using increment
 *                  and spacing specified for scale 1 as
 *                  the incremental value for positioning
 *                  the slider arm. Scale 1 is displayed
 *                  above the slider shaft of a horizontal
 *                  slider and to the right of the slider
 *                  shaft of a vertical slider. This is
 *                  the default for a slider.
 *      -- SLS_PRIMARYSCALE2: not supported by this function
 *      -- SLS_READONLY: creates a read-only slider, which
 *                  presents information to the user but
 *                  allows no interaction with the user.
 *      -- SLS_RIBBONSTRIP: fills, as the slider arm moves, the
 *                  slider shaft between the home position
 *                  and the slider arm with a color value
 *                  different from slider shaft color,
 *                  similar to mercury in a thermometer.
 *      -- SLS_OWNERDRAW: notifies the application whenever the
 *                  slider shaft, the ribbon strip, the
 *                  slider arm, and the slider background
 *                  are to be drawn.
 *      -- SLS_SNAPTOINCREMENT: causes the slider arm, when positioned
 *                  between two values, to be positioned
 *                  to the nearest value and redrawn at
 *                  that position.
 *
 *      Additionally, for horizontal sliders:
 *      -- SLS_BUTTONSLEFT: specifies that the optional slider
 *                  buttons are to be used and places them
 *                  to the left of the slider shaft. The
 *                  buttons move the slider arm by one
 *                  position, left or right, in the
 *                  direction selected.
 *      -- SLS_BUTTONSRIGHT: specifies that the optional slider
 *                  buttons are to be used and places them
 *                  to the right of the slider shaft. The
 *                  buttons move the slider arm by one
 *                  position, left or right, in the
 *                  direction selected.
 *      -- SLS_HOMELEFT: specifies the slider arm's home
 *                  position. The left edge is used as the
 *                  base value for incrementing (default).
 *      -- SLS_HOMERIGHT: specifies the slider arm's home
 *                  position. The right edge is used as
 *                  the base value for incrementing.
 *
 *      Instead, for vertical sliders:
 *      -- SLS_BUTTONSBOTTOM: specifies that the optional slider
 *                  buttons are to be used and places them
 *                  at the bottom of the slider shaft. The
 *                  buttons move the slider arm by one
 *                  position, up or down, in the direction
 *                  selected.
 *      -- SLS_BUTTONSTOP: specifies that the optional slider
 *                  buttons are to be used and places them
 *                  at the top of the slider shaft. The
 *                  buttons move the slider arm by one
 *                  position, up or down, in the direction
 *                  selected.
 *      -- SLS_HOMEBOTTOM: specifies the slider arm's home
 *                  position. The bottom of the slider is
 *                  used as the base value for
 *                  incrementing.
 *      -- SLS_HOMETOP: specifies the slider arm's home
 *                  position. The top of the slider is
 *                  used as the base value for
 *                  incrementing.
 *
 *      Notes: This function automatically adds WS_PARENTCLIP,
 *      WS_TABSTOP, and WS_SYNCPAINT to the specified styles.
 *      For the WS_TABSTOP style, hwndInsertAfter is important.
 *      If you specify HWND_TOP, your window will be the first
 *      in the tab stop list.
 *
 *      It also shows the slider after having done all the
 *      processing in here by calling WinShowWindow.
 *
 *      Also, we only provide support for scale 1 here, so
 *      do not specify SLS_PRIMARYSCALE2 with ulSliderStyle,
 *      and we have the slider calculate all the spacings.
 *
 *      This returns the HWND of the slider or NULLHANDLE upon
 *      errors.
 *
 *@@added V0.9.0 [umoeller]
 */

HWND winhReplaceWithLinearSlider(HWND hwndParent,   // in: parent of old control and slider
                                 HWND hwndOwner,          // in: owner of old control and slider
                                 HWND hwndInsertAfter,    // in: the control after which the slider should
                                                          // come up, or HWND_TOP, or HWND_BOTTOM
                                 ULONG ulID,              // in: ID of old control and slider
                                 ULONG ulSliderStyle,     // in: SLS_* styles
                                 ULONG ulTickCount)       // in: number of ticks (scale 1)
{
    HWND    hwndSlider = NULLHANDLE;
    HWND    hwndKill = WinWindowFromID(hwndParent, ulID);
    if (hwndKill)
    {
        SWP swpControl;
        if (WinQueryWindowPos(hwndKill, &swpControl))
        {
            SLDCDATA slcd;

            // destroy the old control
            WinDestroyWindow(hwndKill);

            // initialize slider control data
            slcd.cbSize = sizeof(SLDCDATA);
            slcd.usScale1Increments = ulTickCount;
            slcd.usScale1Spacing = 0;           // have slider calculate it
            slcd.usScale2Increments = 0;
            slcd.usScale2Spacing = 0;

            // create a slider with the same ID at the same
            // position
            hwndSlider = WinCreateWindow(hwndParent,
                                         WC_SLIDER,
                                         NULL,           // no window text
                                         ulSliderStyle
                                            | WS_PARENTCLIP
                                            | WS_SYNCPAINT
                                            | WS_TABSTOP,
                                         swpControl.x,
                                         swpControl.y,
                                         swpControl.cx,
                                         swpControl.cy,
                                         hwndOwner,
                                         hwndInsertAfter,
                                         ulID,           // same ID as destroyed control
                                         &slcd,          // slider control data
                                         NULL);          // presparams

            WinSendMsg(hwndSlider,
                       SLM_SETTICKSIZE,
                       MPFROM2SHORT(SMA_SETALLTICKS,
                                    6),     // 15 pixels high
                       NULL);

            WinShowWindow(hwndSlider, TRUE);
        }
    }

    return (hwndSlider);
}

/*
 *@@ winhSetSliderTicks:
 *      this adds ticks to the given linear slider,
 *      which are ulPixels pixels high. A useful
 *      value for this is 4.
 *
 *      This queries the slider for the primary
 *      scale values. Only the primary scale is
 *      supported.
 *
 *      If mpEveryOther is 0, this sets all ticks
 *      on the primary slider scale.
 *
 *      If mpEveryOther is != 0, SHORT1FROMMP
 *      specifies the first tick to set, and
 *      SHORT2FROMMP specifies every other tick
 *      to set from there. For example:
 +          MPFROM2SHORT(9, 10)
 *      would set tick 9, 19, 29, and so forth.
 *
 *      Returns FALSE upon errors.
 *
 *@@added V0.9.1 (99-12-04) [umoeller]
 */

BOOL winhSetSliderTicks(HWND hwndSlider,
                        MPARAM mpEveryOther,
                        ULONG ulPixels)
{
    BOOL brc = FALSE;

    if (mpEveryOther == 0)
    {
        // set all ticks:
        brc = (BOOL)WinSendMsg(hwndSlider,
                               SLM_SETTICKSIZE,
                               MPFROM2SHORT(SMA_SETALLTICKS,
                                            ulPixels),
                               NULL);
    }
    else
    {
        SLDCDATA  slcd;
        WNDPARAMS   wp;
        memset(&wp, 0, sizeof(WNDPARAMS));
        wp.fsStatus = WPM_CTLDATA;
        wp.cbCtlData = sizeof(slcd);
        wp.pCtlData = &slcd;
        // get primary scale data from the slider
        if (WinSendMsg(hwndSlider,
                       WM_QUERYWINDOWPARAMS,
                       (MPARAM)&wp,
                       0))
        {
            USHORT usStart = SHORT1FROMMP(mpEveryOther),
                   usEveryOther = SHORT2FROMMP(mpEveryOther);

            USHORT usScale1Max = slcd.usScale1Increments,
                   us;

            brc = TRUE;

            for (us = usStart; us < usScale1Max; us += usEveryOther)
            {
                if (!(BOOL)WinSendMsg(hwndSlider,
                                      SLM_SETTICKSIZE,
                                      MPFROM2SHORT(us,
                                                   ulPixels),
                                      NULL))
                {
                    brc = FALSE;
                    break;
                }
            }
        }
    }

    return (brc);
}

/*
 *@@ winhReplaceWithCircularSlider:
 *      this destroys the control with the ID ulID in hwndDlg
 *      and creates a linear slider at the same position with the
 *      same ID (effectively replacing it).
 *
 *      This is needed because the IBM dialog editor (DLGEDIT.EXE)
 *      cannot create circular sliders. So the way to do this
 *      easily is to create some other control with DLGEDIT
 *      where the slider should be later and call this function
 *      on that control when the dialog is initialized.
 *
 *      You need to specify the following with ulSliderStyle:
 *      --  CSS_CIRCULARVALUE: draws a circular thumb, rather than a line,
 *                  for the value indicator.
 *      --  CSS_MIDPOINT: makes the mid-point tick mark larger.
 *      --  CSS_NOBUTTON: does not display value buttons. Per default, the
 *                  slider displays "-" and "+" buttons to the bottom left
 *                  and bottom right of the knob. (BTW, these bitmaps can be
 *                  changed using CSM_SETBITMAPDATA.)
 *      --  CSS_NONUMBER: does not display the value on the dial.
 *      --  CSS_NOTEXT: does not display title text under the dial.
 *                  Otherwise, the text in the pszTitle parameter
 *                  will be used.
 *      --  CSS_NOTICKS (only listed in pmstddlg.h, not in PMREF):
 *                  obviously, this prevents tick marks from being drawn.
 *      --  CSS_POINTSELECT: permits the values on the circular slider
 *                  to change immediately when dragged.
 *                  Direct manipulation is performed by using a mouse to
 *                  click on and drag the circular slider. There are two
 *                  modes of direct manipulation for the circular slider:
 *                  <BR><B>1)</B> The default direct manipulation mode is to scroll to
 *                  the value indicated by the position of the mouse.
 *                  This could be important if you used a circular slider
 *                  for a volume control, for example. Increasing the volume
 *                  from 0% to 100% too quickly could result in damage to
 *                  both the user's ears and the equipment.
 *                  <BR><B>2)</B>The other mode of direct manipulation permits
 *                  the value on the circular slider to change immediately when dragged.
 *                  This mode is enabled using the CSS_POINTSELECT style bit. When this
 *                  style is used, the value of the dial can be changed by tracking
 *                  the value with the mouse, which changes values quickly.
 *      --  CSS_PROPORTIONALTICKS: allow the length of the tick marks to be calculated
 *                  as a percentage of the radius (for small sliders).
 *      --  CSS_360: permits the scroll range to extend 360 degrees.
 *                  CSS_360 forces the CSS_NONUMBER style on. This is necessary
 *                  to keep the value indicator from corrupting the number value.
 *
 *      FYI: The most commonly known circular slider in OS/2, the one in the
 *      default "Sound" object, has a style of 0x9002018a, meaning
 *      CSS_NOTEXT | CSS_POINTSELECT | CSS_NOTICKS.
 *
 *      Notes: This function automatically adds WS_PARENTCLIP,
 *      WS_TABSTOP, and WS_SYNCPAINT to the specified styles.
 *      For the WS_TABSTOP style, hwndInsertAfter is important.
 *      If you specify HWND_TOP, your window will be the first
 *      in the tab stop list.
 *
 *      It also shows the slider after having done all the
 *      processing in here by calling WinShowWindow.
 *
 *      This returns the HWND of the slider or NULLHANDLE upon
 *      errors.
 *
 *@@added V0.9.0 [umoeller]
 */

HWND winhReplaceWithCircularSlider(HWND hwndParent,   // in: parent of old control and slider
                                   HWND hwndOwner,          // in: owner of old control and slider
                                   HWND hwndInsertAfter,    // in: the control after which the slider should
                                                            // come up, or HWND_TOP, or HWND_BOTTOM
                                   ULONG ulID,              // in: ID of old control and slider
                                   ULONG ulSliderStyle,     // in: SLS_* styles
                                   SHORT sMin,              // in: minimum value (e.g. 0)
                                   SHORT sMax,              // in: maximum value (e.g. 100)
                                   USHORT usIncrement,      // in: minimum increment (e.g. 1)
                                   USHORT usTicksEvery)     // in: ticks ever x values (e.g. 20)
{
    HWND    hwndSlider = NULLHANDLE;
    HWND    hwndKill = WinWindowFromID(hwndParent, ulID);
    if (hwndKill)
    {
        SWP swpControl;
        if (WinQueryWindowPos(hwndKill, &swpControl))
        {
            // destroy the old control
            WinDestroyWindow(hwndKill);

            // WinRegisterCircularSlider();

            // create a slider with the same ID at the same
            // position
            hwndSlider = WinCreateWindow(hwndParent,
                                         WC_CIRCULARSLIDER,
                                         "dummy",        // no window text
                                         ulSliderStyle
                                            // | WS_PARENTCLIP
                                            // | WS_SYNCPAINT
                                            | WS_TABSTOP,
                                         swpControl.x,
                                         swpControl.y,
                                         swpControl.cx,
                                         swpControl.cy,
                                         hwndOwner,
                                         hwndInsertAfter,
                                         ulID,           // same ID as destroyed control
                                         NULL,           // control data
                                         NULL);          // presparams

            if (hwndSlider)
            {
                // set slider range
                WinSendMsg(hwndSlider,
                           CSM_SETRANGE,
                           (MPARAM)(ULONG)sMin,
                           (MPARAM)(ULONG)sMax);

                // set slider increments
                WinSendMsg(hwndSlider,
                           CSM_SETINCREMENT,
                           (MPARAM)(ULONG)usIncrement,
                           (MPARAM)(ULONG)usTicksEvery);

                // set slider value
                WinSendMsg(hwndSlider,
                           CSM_SETVALUE,
                           (MPARAM)0,
                           (MPARAM)0);

                // for some reason, the slider always has
                // WS_CLIPSIBLINGS set, even though we don't
                // set this; we must unset this now, or
                // the slider won't draw itself (%&$&%"$&%!!!)
                WinSetWindowBits(hwndSlider,
                                 QWL_STYLE,
                                 0,         // unset bit
                                 WS_CLIPSIBLINGS);

                WinShowWindow(hwndSlider, TRUE);
            }
        }
    }

    return (hwndSlider);
}

/*
 *@@category: Helpers\PM helpers\Spin button helpers
 */

/* ******************************************************************
 *                                                                  *
 *   Spin button helpers                                            *
 *                                                                  *
 ********************************************************************/

/*
 *@@ winhSetDlgItemSpinData:
 *      sets a spin button's limits and data within a dialog window.
 *      This only works for decimal spin buttons.
 */

VOID winhSetDlgItemSpinData(HWND hwndDlg,       // in: dlg window
                            ULONG idSpinButton,  // in: item ID of spin button
                            ULONG min,           // in: minimum allowed value
                            ULONG max,           // in: maximum allowed value
                            ULONG current)       // in: new current value
{
    HWND hwndSpinButton = WinWindowFromID(hwndDlg, idSpinButton);
    if (hwndSpinButton)
    {
        WinSendMsg(hwndSpinButton,
                   SPBM_SETLIMITS,          // Set limits message
                   (MPARAM)max,             // Spin Button maximum setting
                   (MPARAM)min);             // Spin Button minimum setting

        WinSendMsg(hwndSpinButton,
                   SPBM_SETCURRENTVALUE,    // Set current value message
                   (MPARAM)current,
                   (MPARAM)NULL);
    }
}

/*
 *@@ winhAdjustDlgItemSpinData:
 *      this can be called on a spin button control to
 *      have its current data snap to a grid. This only
 *      works for LONG integer values.
 *
 *      For example, if you specify 100 for the grid and call
 *      this func after you have received SPBN_UP/DOWNARROW,
 *      the spin button's value will always in/decrease
 *      in steps of 100.
 *
 *      This returns the "snapped" value to which the spin
 *      button was set.
 *
 *      If you specify lGrid == 0, this returns the spin
 *      button's value only (V0.9.0).
 *
 *@@changed V0.9.0 [umoeller]: added check for lGrid == 0 (caused division by zero previously)
 */

LONG winhAdjustDlgItemSpinData(HWND hwndDlg,     // in: dlg window
                               USHORT usItemID,  // in: item ID of spin button
                               LONG lGrid,       // in: grid
                               USHORT usNotifyCode) // in: SPBN_UP* or *DOWNARROW of WM_CONTROL message
{
    HWND hwndSpin = WinWindowFromID(hwndDlg, usItemID);
    LONG lBottom, lTop, lValue;
    // get value, which has already increased /
    // decreased by 1
    WinSendMsg(hwndSpin,
               SPBM_QUERYVALUE,
               (MPARAM)&lValue,
               MPFROM2SHORT(0, SPBQ_ALWAYSUPDATE));

    if ((lGrid)
        && (    (usNotifyCode == SPBN_UPARROW)
             || (usNotifyCode == SPBN_DOWNARROW)
           )
       )
    {
        // only if the up/down buttons were pressed,
        // snap to the nearest grid; if the user
        // manually enters something (SPBN_CHANGE),
        // we'll accept that value
        lValue = (lValue / lGrid) * lGrid;
        // add /subtract grid
        if (usNotifyCode == SPBN_UPARROW)
            lValue += lGrid;
        // else we'll have -= lGrid already

        // balance with spin button limits
        WinSendMsg(hwndSpin,
                   SPBM_QUERYLIMITS,
                   (MPARAM)&lTop,
                   (MPARAM)&lBottom);
        if (lValue < lBottom)
            lValue = lTop;
        else if (lValue > lTop)
            lValue = lBottom;

        WinSendMsg(hwndSpin,
                   SPBM_SETCURRENTVALUE,
                   (MPARAM)(lValue),
                   MPNULL);
    }
    return (lValue);
}

/*
 *@@category: Helpers\PM helpers\List box helpers
 */

/* ******************************************************************
 *                                                                  *
 *   List box helpers                                               *
 *                                                                  *
 ********************************************************************/

/*
 *@@ winhQueryLboxItemText:
 *      returns the text of the specified
 *      list box item in a newly allocated
 *      buffer, which you should free() afterwards,
 *      or NULL upon errors.
 *
 *@@added V0.9.1 (99-12-14) [umoeller]
 */

PSZ winhQueryLboxItemText(HWND hwndListbox,
                          SHORT sIndex)
{
    PSZ   pszReturn = 0;
    SHORT cbText = SHORT1FROMMR(WinSendMsg(hwndListbox,
                                           LM_QUERYITEMTEXTLENGTH,
                                           (MPARAM)(ULONG)sIndex,
                                           0));
    if ((cbText) && (cbText != LIT_ERROR))
    {
        pszReturn = (PSZ)malloc(cbText + 1);        // add zero terminator
        WinSendMsg(hwndListbox,
                   LM_QUERYITEMTEXT,
                   MPFROM2SHORT(sIndex,
                                cbText + 1),
                   (MPARAM)pszReturn);
    }

    return (pszReturn);
}

/*
 *@@ winhMoveLboxItem:
 *      this moves one list box item from one
 *      list box to another, including the
 *      item text and the item "handle"
 *      (see LM_QUERYITEMHANDLE).
 *
 *      sTargetIndex can either be a regular
 *      item index or one of the following
 *      (as in LM_INSERTITEM):
 *      -- LIT_END
 *      -- LIT_SORTASCENDING
 *      -- LIT_SORTDESCENDING
 *
 *      If (fSelectTarget == TRUE), the new
 *      item is also selected in the target
 *      list box.
 *
 *      Returns FALSE if moving failed. In
 *      that case, the list boxes are unchanged.
 *
 *@@added V0.9.1 (99-12-14) [umoeller]
 */

BOOL winhMoveLboxItem(HWND hwndSource,
                      SHORT sSourceIndex,
                      HWND hwndTarget,
                      SHORT sTargetIndex,
                      BOOL fSelectTarget)
{
    BOOL brc = FALSE;

    PSZ pszItemText = winhQueryLboxItemText(hwndSource, sSourceIndex);
    if (pszItemText)
    {
        ULONG   ulItemHandle = winhQueryLboxItemHandle(hwndSource,
                                                       sSourceIndex);
                    // probably 0, if not used
        LONG lTargetIndex = WinInsertLboxItem(hwndTarget,
                                              sTargetIndex,
                                              pszItemText);
        if (    (lTargetIndex != LIT_ERROR)
             && (lTargetIndex != LIT_MEMERROR)
           )
        {
            // successfully inserted:
            winhSetLboxItemHandle(hwndTarget, lTargetIndex, ulItemHandle);
            if (fSelectTarget)
                winhSetLboxSelectedItem(hwndTarget, lTargetIndex, TRUE);

            // remove source
            WinDeleteLboxItem(hwndSource,
                              sSourceIndex);

            brc = TRUE;
        }

        free(pszItemText);
    }

    return (brc);
}

/*
 *@@ winhLboxSelectAll:
 *      this selects or deselects all items in the
 *      given list box, depending on fSelect.
 *
 *      Returns the number of items in the list box.
 */

ULONG winhLboxSelectAll(HWND hwndListBox,   // in: list box
                        BOOL fSelect)       // in: TRUE = select, FALSE = deselect
{
    LONG lItemCount = WinQueryLboxCount(hwndListBox);
    ULONG ul;

    for (ul = 0; ul < lItemCount; ul++)
    {
        WinSendMsg(hwndListBox,
                   LM_SELECTITEM,
                   (MPARAM)ul,      // index
                   (MPARAM)fSelect);
    }

    return (lItemCount);
}

/*
 *@@category: Helpers\PM helpers\Scroll bar helpers
 */

/* ******************************************************************
 *                                                                  *
 *   Scroll bar helpers                                             *
 *                                                                  *
 ********************************************************************/

/*
 *@@ winhUpdateScrollBar:
 *      updates the given scroll bar according to the given
 *      values. This updates the scroll bar's thumb size,
 *      extension, and position, all in one shot.
 *
 *      This simplifies the typical functionality of a scroll
 *      bar in a client window which is to be scrolled. I am
 *      wondering why IBM never included such a function, since
 *      it is so damn basic and still writing it cost me a whole
 *      day.
 *
 *      Terminology:
 *
 *      -- "window": the actual window with scroll bars which displays
 *         a subrectangle of the available data.
 *         The width or height of this must be passed in ulWinPels.
 *
 *      -- "viewport": the entire data to be displayed, of which the
 *         "window" can only display a subrectangle, if the viewport
 *         is larger than the window. The width or height of this must be
 *         passed in ulViewportPels. This can be smaller than ulWinPels (if the
 *         window is larger than the data), the same or larger than ulWinPels
 *         (if the window is too small to show all the data).
 *
 *      -- "window offset": the offset of the current window within
 *         the viewport. For horizontal scroll bars, this is
 *         the X coordinate, counting from the left of the window
 *         (0 means leftmost).
 *         For vertical scroll bars, this is counted from the _top_
 *         of the viewport (0 means topmost, as opposed to OS/2
 *         window coordinates!).
 *         This is because for vertical scroll bars controls, higher
 *         values move the thumb _down_.
 *
 *      The scroll bar is disabled if the entire viewport is visible,
 *      that is, if ulViewportPels <= ulWinPels. In that case
 *      FALSE is returned. If (fAutoHide == TRUE), the scroll
 *      bar is not only disabled, but also hidden from the display.
 *      In that case, you will need to reformat your output because
 *      your viewport becomes larger without the scroll bar.
 *
 *      This function will set the range of the scroll bar to 0 up
 *      to a value depending on the viewport size. For vertical scroll
 *      bars, 0 means topmost (which is kinda sick with the OS/2
 *      coordinate system), for horizontal scroll bars, 0 means leftmost.
 *      The maximum value of the scroll bar will be:
 +          (ulViewportPels - ulWinPels) / usScrollUnitPels
 *
 *      The thumb size of the scroll bar will also be adjusted
 *      based on the viewport and window size, as it should be.
 *
 *@@added V0.9.1 (2000-02-14) [umoeller]
 *@@changed V0.9.3 (2000-04-30) [umoeller]: fixed pels/unit confusion
 *@@changed V0.9.3 (2000-05-08) [umoeller]: now handling scroll units automatically
 */

BOOL winhUpdateScrollBar(HWND hwndScrollBar,    // in: scroll bar (vertical or horizontal)
                         ULONG ulWinPels,       // in: vertical or horizontal dimension of
                                                // visible window part (in pixels),
                                                // excluding the scroll bar!
                         ULONG ulViewportPels,  // in: dimension of total data part, of
                                                // which ulWinPels is a sub-dimension
                                                // (in pixels);
                                                // if <= ulWinPels, the scrollbar will be
                                                // disabled
                         ULONG ulCurPelsOfs,    // in: current offset of visible part
                                                // (in pixels)
                         BOOL fAutoHide)        // in: hide scroll bar if disabled
{
    BOOL brc = FALSE;

    // _Pmpf(("Entering winhUpdateScrollBar"));

    // for large viewports, adjust scroll bar units
    USHORT  usScrollUnitPels = 1;
    if (ulViewportPels > 10000)
        usScrollUnitPels = 100;

    if (ulViewportPels > ulWinPels)
    {
        // scrollbar needed:
        USHORT  usThumbDivisorUnits = usScrollUnitPels;
        USHORT  lMaxAllowedUnitOfs;
        // _Pmpf(("winhUpdateScrollBar: ulViewportPels > ulWinPels, enabling scroller"));
        // divisor for thumb size (below)
        if (ulViewportPels > 10000)
            // for very large viewports, we need to
            // raise the divisor, because we only
            // have a USHORT
            usThumbDivisorUnits = usScrollUnitPels * 100;

        // viewport is larger than window:
        WinEnableWindow(hwndScrollBar, TRUE);
        if (fAutoHide)
            WinShowWindow(hwndScrollBar, TRUE);

        // calculate limit
        lMaxAllowedUnitOfs = ((ulViewportPels - ulWinPels + usScrollUnitPels)
                               // scroll unit is 10
                               / usScrollUnitPels);

        // _Pmpf(("    usCurUnitOfs: %d", ulCurUnitOfs));
        // _Pmpf(("    usMaxUnits: %d", lMaxAllowedUnitOfs));

        // set thumb position and limit
        WinSendMsg(hwndScrollBar,
                   SBM_SETSCROLLBAR,
                   (MPARAM)(ulCurPelsOfs), //  / usThumbDivisorUnits),   // position: 0 means top
                   MPFROM2SHORT(0,  // minimum
                                lMaxAllowedUnitOfs));    // maximum

        // set thumb size based on ulWinPels and
        // ulViewportPels
        WinSendMsg(hwndScrollBar,
                   SBM_SETTHUMBSIZE,
                   MPFROM2SHORT(    ulWinPels / usThumbDivisorUnits,       // visible
                                    ulViewportPels / usThumbDivisorUnits), // total
                   0);
        brc = TRUE;
    }
    else
    {
        // _Pmpf(("winhUpdateScrollBar: ulViewportPels <= ulWinPels"));
        // entire viewport is visible:
        WinEnableWindow(hwndScrollBar, FALSE);
        if (fAutoHide)
            WinShowWindow(hwndScrollBar, FALSE);
    }

    // _Pmpf(("End of winhUpdateScrollBar"));

    return (brc);
}

/*
 *@@ winhHandleScrollMsg:
 *      this helper handles a WM_VSCROLL or WM_HSCROLL
 *      message posted to a client window when the user
 *      has worked on a client scroll bar. Calling this
 *      function is ALL you need to do to handle those
 *      two messages.
 *
 *      This is most useful in conjunction with winhUpdateScrollBar.
 *      See that function for the terminology also.
 *
 *      This function calculates the new scrollbar position
 *      (from the mp2 value, which can be line up/down,
 *      page up/down, or slider track) and calls WinScrollWindow
 *      accordingly. The window part which became invalid
 *      because of the scrolling is automatically invalidated
 *      (using WinInvalidateRect), so expect a WM_PAINT after
 *      calling this function.
 *
 *      This function assumes that the scrollbar operates
 *      on values starting from zero. The maximum value
 *      of the scroll bar is:
 +          ulViewportPels - (prcl2Scroll->yTop - prcl2Scroll->yBottom)
 *
 *      This function also automatically changes the scroll bar
 *      units, should you have a viewport size which doesn't fit
 *      into the SHORT's that the scroll bar uses internally. As
 *      a result, this function handles a the complete range of
 *      a ULONG for the viewport.
 *
 *      Replace "bottom" and "top" with "right" and "left" for
 *      horizontal scrollbars in the above formula.
 *
 *@@added V0.9.1 (2000-02-13) [umoeller]
 *@@changed V0.9.3 (2000-04-30) [umoeller]: changed prototype, fixed pels/unit confusion
 *@@changed V0.9.3 (2000-05-08) [umoeller]: now handling scroll units automatically
 */

BOOL winhHandleScrollMsg(HWND hwnd2Scroll,          // in: client window to scroll
                         HWND hwndScrollBar,        // in: vertical or horizontal scroll bar window
                         PLONG plCurPelsOfs,        // in/out: current viewport offset;
                                                    // this is updated with the proper scroll units
                         PRECTL prcl2Scroll,        // in: hwnd2Scroll rectangle to scroll
                                                    // (in window coordinates);
                                                    // this is passed to WinScrollWindow,
                                                    // which considers this inclusive!
                         LONG ulViewportPels,       // in: total viewport dimension,
                                                    // into which *plCurPelsOfs is an offset
                         USHORT usLineStepPels,     // in: pixels to scroll line-wise
                                                    // (scroll bar buttons pressed)
                         ULONG msg,                 // in: either WM_VSCROLL or WM_HSCROLL
                         MPARAM mp2)                // in: complete mp2 of WM_VSCROLL/WM_HSCROLL;
                                                    // this has two SHORT's (usPos and usCmd),
                                                    // see PMREF for details
{
    LONG    lOldPelsOfs = *plCurPelsOfs;
    USHORT  usPosUnits = SHORT1FROMMP(mp2), // in scroll units
            usCmd = SHORT2FROMMP(mp2);
    LONG    lMaxAllowedUnitOfs;
    ULONG   ulWinPels;

    // for large viewports, adjust scroll bar units
    USHORT  usScrollUnitPels = 1;
    if (ulViewportPels > 10000)
        usScrollUnitPels = 100;

    // calculate window size (vertical or horizontal)
    if (msg == WM_VSCROLL)
        ulWinPels = (prcl2Scroll->yTop - prcl2Scroll->yBottom);
    else
        ulWinPels = (prcl2Scroll->xRight - prcl2Scroll->xLeft);

    lMaxAllowedUnitOfs = ((LONG)ulViewportPels - ulWinPels) / usScrollUnitPels;

    // _Pmpf(("Entering winhHandleScrollMsg"));

    switch (usCmd)
    {
        case SB_LINEUP:
            // _Pmpf(("SB_LINEUP"));
            *plCurPelsOfs -= usLineStepPels;  //  * usScrollUnitPels);
        break;

        case SB_LINEDOWN:
            // _Pmpf(("SB_LINEDOWN"));
            *plCurPelsOfs += usLineStepPels;  //  * usScrollUnitPels);
        break;

        case SB_PAGEUP:
            *plCurPelsOfs -= ulWinPels; // convert to units
        break;

        case SB_PAGEDOWN:
            *plCurPelsOfs += ulWinPels; // convert to units
        break;

        case SB_SLIDERTRACK:
            *plCurPelsOfs = (usPosUnits * usScrollUnitPels);
            // _Pmpf(("    SB_SLIDERTRACK: usUnits = %d", usPosUnits));
        break;

        case SB_SLIDERPOSITION:
            *plCurPelsOfs = (usPosUnits * usScrollUnitPels);
        break;
    }

    // are we close to the lower limit?
    /* if (*plCurUnitOfs < usLineStepUnits) // usScrollUnit)
        *plCurUnitOfs = 0;
    // are we close to the upper limit?
    else if (*plCurUnitOfs + usLineStepUnits > lMaxUnitOfs)
    {
        _Pmpf(("        !!! limiting: %d to %d", *plCurUnitOfs, lMaxUnitOfs));
        *plCurUnitOfs = lMaxUnitOfs;
    } */

    if (*plCurPelsOfs < 0)
        *plCurPelsOfs = 0;
    if (*plCurPelsOfs > (lMaxAllowedUnitOfs * usScrollUnitPels))
    {
        // _Pmpf(("        !!! limiting 2: %d to %d", *plCurUnitOfs, lMaxAllowedUnitOfs));
        *plCurPelsOfs = (lMaxAllowedUnitOfs * usScrollUnitPels);
    }
    if (    (*plCurPelsOfs != lOldPelsOfs)
         || (*plCurPelsOfs == 0)
         || (*plCurPelsOfs == (lMaxAllowedUnitOfs * usScrollUnitPels))
       )
    {
        RECTL   rcl2Scroll,
                rcl2Update;

        // changed:
        WinSendMsg(hwndScrollBar,
                   SBM_SETPOS,
                   (MPARAM)(*plCurPelsOfs / usScrollUnitPels), //  / usScrollUnit),
                   0);
        // scroll window rectangle:
        rcl2Scroll.xLeft =  prcl2Scroll->xLeft;
        rcl2Scroll.xRight =  prcl2Scroll->xRight;
        rcl2Scroll.yBottom =  prcl2Scroll->yBottom;
        rcl2Scroll.yTop =  prcl2Scroll->yTop;

        if (msg == WM_VSCROLL)
            WinScrollWindow(hwnd2Scroll,
                            0,
                            (*plCurPelsOfs - lOldPelsOfs)  // scroll units changed
                            ,    // * usScrollUnitPels,     // convert to pels
                            &rcl2Scroll,  // rcl to scroll
                            prcl2Scroll, // clipping rect
                            NULLHANDLE, // no region
                            &rcl2Update,
                            0);
        else
            WinScrollWindow(hwnd2Scroll,
                            -(*plCurPelsOfs - lOldPelsOfs) // scroll units changed
                            ,    // * usScrollUnitPels,
                            0,
                            &rcl2Scroll,  // rcl to scroll
                            prcl2Scroll, // clipping rect
                            NULLHANDLE, // no region
                            &rcl2Update,
                            0);

        // WinScrollWindow has stored the invalid window
        // rectangle which needs to be repainted in rcl2Update:
        WinInvalidateRect(hwnd2Scroll, &rcl2Update, FALSE);
    }

    // _Pmpf(("End of winhHandleScrollMsg"));

    return (TRUE);
}

/*
 *@@ winhProcessScrollChars:
 *      helper for processing WM_CHAR messages for
 *      client windows with scroll bars.
 *
 *      If your window has scroll bars, you normally
 *      need to process a number of keystrokes to be
 *      able to scroll the window contents. This is
 *      tiresome to code, so here is a helper.
 *
 *      When receiving WM_CHAR, call this function.
 *      If this returns TRUE, the keystroke has been
 *      a scroll keystroke, and the window has been
 *      updated (by sending WM_VSCROLL or WM_HSCROLL
 *      to hwndClient). Otherwise, you should process
 *      the keystroke as usual because it's not a
 *      scroll keystroke.
 *
 *      The following keystrokes are processed here:
 *
 *      -- "cursor up, down, right, left": scroll one
 *         line in the proper direction.
 *      -- "page up, down": scroll one page up or down.
 *      -- "Home": scroll leftmost.
 *      -- "Ctrl+ Home": scroll topmost.
 *      -- "End": scroll rightmost.
 *      -- "Ctrl+ End": scroll bottommost.
 *      -- "Ctrl + page up, down": scroll topmost or bottommost.
 *
 *      This is roughly CUA behavior.
 *
 *      Returns TRUE if the message has been
 *      processed.
 *
 *@@added V0.9.3 (2000-04-29) [umoeller]
 */

BOOL winhProcessScrollChars(HWND hwndClient,    // in: client window
                            HWND hwndVScroll,   // in: vertical scroll bar
                            HWND hwndHScroll,   // in: horizontal scroll bar
                            MPARAM mp1,         // in: WM_CHAR mp1
                            MPARAM mp2,         // in: WM_CHAR mp2
                            ULONG ulVertMax,    // in: maximum viewport cy
                            ULONG ulHorzMax)    // in: maximum viewport cx
{
    BOOL    fProcessed = FALSE;
    USHORT usFlags    = SHORT1FROMMP(mp1);
    // USHORT usch       = SHORT1FROMMP(mp2);
    USHORT usvk       = SHORT2FROMMP(mp2);

    // _Pmpf(("Entering winhProcessScrollChars"));

    if (usFlags & KC_VIRTUALKEY)
    {
        ULONG   ulMsg = 0;
        SHORT   sPos = 0;
        SHORT   usCmd = 0;
        fProcessed = TRUE;

        switch (usvk)
        {
            case VK_UP:
                ulMsg = WM_VSCROLL;
                usCmd = SB_LINEUP;
            break;

            case VK_DOWN:
                ulMsg = WM_VSCROLL;
                usCmd = SB_LINEDOWN;
            break;

            case VK_RIGHT:
                ulMsg = WM_HSCROLL;
                usCmd = SB_LINERIGHT;
            break;

            case VK_LEFT:
                ulMsg = WM_HSCROLL;
                usCmd = SB_LINELEFT;
            break;

            case VK_PAGEUP:
                ulMsg = WM_VSCROLL;
                if (usFlags & KC_CTRL)
                {
                    sPos = 0;
                    usCmd = SB_SLIDERPOSITION;
                }
                else
                    usCmd = SB_PAGEUP;
            break;

            case VK_PAGEDOWN:
                ulMsg = WM_VSCROLL;
                if (usFlags & KC_CTRL)
                {
                    sPos = ulVertMax;
                    usCmd = SB_SLIDERPOSITION;
                }
                else
                    usCmd = SB_PAGEDOWN;
            break;

            case VK_HOME:
                if (usFlags & KC_CTRL)
                    // vertical:
                    ulMsg = WM_VSCROLL;
                else
                    ulMsg = WM_HSCROLL;

                sPos = 0;
                usCmd = SB_SLIDERPOSITION;
            break;

            case VK_END:
                if (usFlags & KC_CTRL)
                {
                    // vertical:
                    ulMsg = WM_VSCROLL;
                    sPos = ulVertMax;
                }
                else
                {
                    ulMsg = WM_HSCROLL;
                    sPos = ulHorzMax;
                }

                usCmd = SB_SLIDERPOSITION;
            break;

            default:
                // other:
                fProcessed = FALSE;
        }

        if (    ((usFlags & KC_KEYUP) == 0)
             && (ulMsg)
           )
        {
            HWND   hwndScrollBar = ((ulMsg == WM_VSCROLL)
                                        ? hwndVScroll
                                        : hwndHScroll);
            if (WinIsWindowEnabled(hwndScrollBar))
            {
                USHORT usID = WinQueryWindowUShort(hwndScrollBar,
                                                   QWS_ID);
                WinSendMsg(hwndClient,
                           ulMsg,
                           MPFROMSHORT(usID),
                           MPFROM2SHORT(sPos,
                                        usCmd));
            }
        }
    }

    // _Pmpf(("End of  winhProcessScrollChars"));

    return (fProcessed);
}

/*
 *@@category: Helpers\PM helpers\Window positioning
 */

/* ******************************************************************
 *                                                                  *
 *   Window positioning helpers                                     *
 *                                                                  *
 ********************************************************************/

/*
 *@@ winhSaveWindowPos:
 *      saves the position of a certain window. As opposed
 *      to the barely documented WinStoreWindowPos API, this
 *      one only saves one regular SWP structure for the given
 *      window, as returned by WinQueryWindowPos for hwnd.
 *
 *      If the window is currently maximized or minimized,
 *      we won't store the current window size and position
 *      (which wouldn't make much sense), but retrieve the
 *      "restored" window position from the window words
 *      instead.
 *
 *      The window should still be visible on the screen
 *      when calling this function. Do not call it in WM_DESTROY,
 *      because then the SWP data is no longer valid.
 *
 *      This returns TRUE if saving was successful.
 *
 *@@changed V0.9.1 (99-12-19) [umoeller]: added minimize/maximize support
 */

BOOL winhSaveWindowPos(HWND hwnd,   // in: window to save
                       HINI hIni,   // in: INI file (or HINI_USER/SYSTEM)
                       PSZ pszApp,  // in: INI application name
                       PSZ pszKey)  // in: INI key name
{
    BOOL brc = FALSE;
    SWP swp;
    if (WinQueryWindowPos(hwnd, &swp))
    {
        if (swp.fl & (SWP_MAXIMIZE | SWP_MINIMIZE))
        {
            // window currently maximized or minimized:
            // retrieve "restore" position from window words
            swp.x = WinQueryWindowUShort(hwnd, QWS_XRESTORE);
            swp.y = WinQueryWindowUShort(hwnd, QWS_YRESTORE);
            swp.cx = WinQueryWindowUShort(hwnd, QWS_CXRESTORE);
            swp.cy = WinQueryWindowUShort(hwnd, QWS_CYRESTORE);
        }

        brc = PrfWriteProfileData(hIni, pszApp, pszKey, &swp, sizeof(swp));
    }
    return (brc);
}

/*
 *@@ winhRestoreWindowPos:
 *      this will retrieve a window position which was
 *      previously stored using winhSaveWindowPos.
 *
 *      The window should not be visible to avoid flickering.
 *      "fl" must contain the SWP_flags as in WinSetWindowPos.
 *
 *      Note that only the following may be used:
 *      --  SWP_MOVE        reposition the window
 *      --  SWP_SIZE        also resize the window to
 *                          the stored position; this might
 *                          lead to problems with different
 *                          video resolutions, so be careful.
 *      --  SWP_SHOW        make window visible too
 *      --  SWP_NOREDRAW    changes are not redrawn
 *      --  SWP_NOADJUST    do not send a WM_ADJUSTWINDOWPOS message
 *                          before moving or sizing
 *      --  SWP_ACTIVATE    activate window (make topmost)
 *      --  SWP_DEACTIVATE  deactivate window (make bottommost)
 *
 *      Do not specify any other SWP_* flags.
 *
 *      If SWP_SIZE is not set, the window will be moved only.
 *
 *      This returns TRUE if INI data was found.
 *
 *      This function automatically checks for whether the
 *      window would be positioned outside the visible screen
 *      area and will adjust coordinates accordingly. This can
 *      happen when changing video resolutions.
 */

BOOL winhRestoreWindowPos(HWND hwnd,   // in: window to restore
                          HINI hIni,   // in: INI file (or HINI_USER/SYSTEM)
                          PSZ pszApp,  // in: INI application name
                          PSZ pszKey,  // in: INI key name
                          ULONG fl)    // in: "fl" parameter for WinSetWindowPos
{
    BOOL    brc = FALSE;
    SWP     swp, swpNow;
    ULONG   cbswp = sizeof(swp);
    ULONG   fl2 = fl;

    if (PrfQueryProfileData(hIni, pszApp, pszKey, &swp, &cbswp))
    {
        ULONG ulScreenCX = WinQuerySysValue(HWND_DESKTOP, SV_CXSCREEN);
        ULONG ulScreenCY = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN);

        brc = TRUE;

        if ((fl & SWP_SIZE) == 0)
        {
            // if no resize, we need to get the current position
            brc = WinQueryWindowPos(hwnd, &swpNow);
            swp.cx = swpNow.cx;
            swp.cy = swpNow.cy;
        }

        if (brc)
        {
            // check for full visibility
            if ( (swp.x + swp.cx) > ulScreenCX)
                swp.x = ulScreenCX - swp.cx;
            if ( (swp.y + swp.cy) > ulScreenCY)
                swp.y = ulScreenCY - swp.cy;
        }

        brc = TRUE;

    }
    else
        // window pos not found in INI: unset SWP_MOVE etc.
        fl2 &= ~(SWP_MOVE | SWP_SIZE);

    WinSetWindowPos(hwnd,
                    NULLHANDLE,       // insert-behind window
                    swp.x,
                    swp.y,
                    swp.cx,
                    swp.cy,
                    fl2);        // SWP_* flags

    return (brc);
}

/*
 *@@ winhAdjustControls:
 *      helper function for dynamically adjusting a window's
 *      controls when the window is resized.
 *
 *      This is most useful with dialogs loaded from resources
 *      which should be sizeable. Normally, when the dialog
 *      frame is resized, the controls stick to their positions,
 *      and for dialogs with many controls, programming the
 *      changes can be tiresome.
 *
 *      Enter this function. ;-) Basically, this takes a
 *      static array of MPARAM's as input (plus one dynamic
 *      storage area for the window positions).
 *
 *      This function must get called in three contexts:
 *      during WM_INITDLG, during WM_WINDOWPOSCHANGED, and
 *      during WM_DESTROY, with varying parameters.
 *
 *      In detail, there are four things you need to do to make
 *      this work:
 *
 *      1)  Set up a static array (as a global variable) of
 *          MPARAM's, one for each control in your array.
 *          Each MPARAM will have the control's ID and the
 *          XAF* flags (winh.h) how the control shall be moved.
 *          Use MPFROM2SHORT to easily create this. Example:
 *
 +          MPARAM ampControlFlags[] =
 +              {  MPFROM2SHORT(ID_CONTROL_1, XAC_MOVEX),
 +                 MPFROM2SHORT(ID_CONTROL_2, XAC_SIZEY),
 +                  ...
 +              }
 *
 *          This can safely be declared as a global variable
 *          because this data will only be read and never
 *          changed by this function.
 *
 *      2)  In WM_INITDLG of your dialog function, set up
 *          an XADJUSTCTRLS structure, preferrably in your
 *          window words (QWL_USER).
 *
 *          ZERO THAT STRUCTURE (memset(&xac, 0, sizeof(XADJUSTCTRLS),
 *          or this func will not work.
 *
 *          Call this function with pswpNew == NULL and
 *          pxac pointing to that new structure. This will
 *          query the positions of all the controls listed
 *          in the MPARAMs array and store them in the
 *          XADJUSTCTRLS area.
 *
 *      3)  Intercept WM_WINDOWPOSCHANGED:
 *
 +          case WM_WINDOWPOSCHANGED:
 +          {
 +              // this msg is passed two SWP structs:
 +              // one for the old, one for the new data
 +              // (from PM docs)
 +              PSWP pswpNew = PVOIDFROMMP(mp1);
 +              PSWP pswpOld = pswpNew + 1;
 +
 +              // resizing?
 +              if (pswpNew->fl & SWP_SIZE)
 +              {
 +                  PXADJUSTCTRLS pxac = ... // get it from your window words
 +
 +                  winhAdjustControls(hwndDlg,             // dialog
 +                                     ampControlFlags,     // MPARAMs array
 +                                     sizeof(ampControlFlags) / sizeof(MPARAM),
 +                                                          // items count
 +                                     pswpNew,             // mp1
 +                                     pxac);               // storage area
 +              }
 +              mrc = WinDefDlgProc(hwnd, msg, mp1, mp2); ...
 *
 *      4)  In WM_DESTROY, call this function again with pmpFlags,
 *          pswpNew, and pswpNew set to NULL. This will clean up the
 *          data which has been allocated internally (pointed to from
 *          the XADJUSTCTRLS structure).
 *          Don't forget to free your storage for XADJUSTCTLRS
 *          _itself_, that's the job of the caller.
 *
 *      This might sound complicated, but it's a lot easier than
 *      having to write dozens of WinSetWindowPos calls oneself.
 *
 *@@added V0.9.0 [umoeller]
 */

BOOL winhAdjustControls(HWND hwndDlg,           // in: dialog (req.)
                        MPARAM *pmpFlags,       // in: init flags or NULL for cleanup
                        ULONG ulCount,          // in: item count (req.)
                        PSWP pswpNew,           // in: pswpNew from WM_WINDOWPOSCHANGED or NULL for cleanup
                        PXADJUSTCTRLS pxac)     // in: adjust-controls storage area (req.)
{
    BOOL    brc = FALSE;
    ULONG   ul = 0;

    /* if (!WinIsWindowVisible(hwndDlg))
        return (FALSE); */

    if ((pmpFlags) && (pxac))
    {
        PSWP    pswpThis;
        MPARAM  *pmpThis;
        LONG    ldcx, ldcy;
        ULONG   cWindows = 0;

        // setup mode:
        if (pxac->fInitialized == FALSE)
        {
            // first call: get all the SWP's
            WinQueryWindowPos(hwndDlg, &pxac->swpMain);
            // _Pmpf(("winhAdjustControls: queried main cx = %d, cy = %d",
               //      pxac->swpMain.cx, pxac->swpMain.cy));

            pxac->paswp = (PSWP)malloc(sizeof(SWP) * ulCount);

            pswpThis = pxac->paswp;
            pmpThis = pmpFlags;

            for (ul = 0;
                 ul < ulCount;
                 ul++)
            {
                HWND hwndThis = WinWindowFromID(hwndDlg, SHORT1FROMMP(*pmpThis));
                if (hwndThis)
                {
                    WinQueryWindowPos(hwndThis, pswpThis);
                    cWindows++;
                }

                pswpThis++;
                pmpThis++;
            }

            pxac->fInitialized = TRUE;
            // _Pmpf(("winhAdjustControls: queried %d controls", cWindows));
        }

        if (pswpNew)
        {
            // compute width and height delta
            ldcx = (pswpNew->cx - pxac->swpMain.cx);
            ldcy = (pswpNew->cy - pxac->swpMain.cy);

            // _Pmpf(("winhAdjustControls: new cx = %d, cy = %d",
              //       pswpNew->cx, pswpNew->cy));

            // now adjust the controls
            cWindows = 0;
            pswpThis = pxac->paswp;
            pmpThis = pmpFlags;
            for (ul = 0;
                 ul < ulCount;
                 ul++)
            {
                HWND hwndThis = WinWindowFromID(hwndDlg, SHORT1FROMMP(*pmpThis));
                if (hwndThis)
                {
                    LONG    x = pswpThis->x,
                            y = pswpThis->y,
                            cx = pswpThis->cx,
                            cy = pswpThis->cy;

                    ULONG   ulSwpFlags = 0;
                    // get flags for this control
                    USHORT  usFlags = SHORT2FROMMP(*pmpThis);

                    if (usFlags & XAC_MOVEX)
                    {
                        x += ldcx;
                        ulSwpFlags |= SWP_MOVE;
                    }
                    if (usFlags & XAC_MOVEY)
                    {
                        y += ldcy;
                        ulSwpFlags |= SWP_MOVE;
                    }
                    if (usFlags & XAC_SIZEX)
                    {
                        cx += ldcx;
                        ulSwpFlags |= SWP_SIZE;
                    }
                    if (usFlags & XAC_SIZEY)
                    {
                        cy += ldcy;
                        ulSwpFlags |= SWP_SIZE;
                    }

                    if (ulSwpFlags)
                    {
                        WinSetWindowPos(hwndThis,
                                        NULLHANDLE, // hwndInsertBehind
                                        x, y, cx, cy,
                                        ulSwpFlags);
                        cWindows++;
                        brc = TRUE;
                    }
                }

                pswpThis++;
                pmpThis++;
            }

            // _Pmpf(("winhAdjustControls: set %d windows", cWindows));
        }
    }
    else
    {
        // pxac == NULL:
        // cleanup mode
        if (pxac->paswp)
            free(pxac->paswp);
    }

    return (brc);
}

/*
 *@@ winhCenterWindow:
 *      centers a window within its parent window. If that's
 *      the PM desktop, it will be centered according to the
 *      whole screen.
 *      For dialog boxes, use WinCenteredDlgBox as a one-shot
 *      function.
 *
 *      Note: When calling this function, the window should
 *      not be visible to avoid flickering.
 *      This func does not show the window either, so call
 *      WinShowWindow afterwards.
 */

void winhCenterWindow(HWND hwnd)
{
   RECTL rclParent;
   RECTL rclWindow;

   WinQueryWindowRect(hwnd, &rclWindow);
   WinQueryWindowRect(WinQueryWindow(hwnd, QW_PARENT), &rclParent);

   rclWindow.xLeft   = (rclParent.xRight - rclWindow.xRight) / 2;
   rclWindow.yBottom = (rclParent.yTop   - rclWindow.yTop  ) / 2;

   WinSetWindowPos(hwnd, NULLHANDLE, rclWindow.xLeft, rclWindow.yBottom,
                    0, 0, SWP_MOVE);
}

/*
 *@@ winhCenteredDlgBox:
 *      just like WinDlgBox, but the dlg box is centered on the screen;
 *      you should mark the dlg template as not visible in the dlg
 *      editor, or display will flicker.
 *      As opposed to winhCenterWindow, this _does_ show the window.
 */

ULONG winhCenteredDlgBox(HWND hwndParent,
                         HWND hwndOwner,
                         PFNWP pfnDlgProc,
                         HMODULE hmod,
                         ULONG idDlg,
                         PVOID pCreateParams)
{
    ULONG   ulReply;
    HWND    hwndDlg = WinLoadDlg(hwndParent,
                                 hwndOwner,
                                 pfnDlgProc,
                                 hmod,
                                 idDlg,
                                 pCreateParams);
    winhCenterWindow(hwndDlg);
    ulReply = WinProcessDlg(hwndDlg);
    WinDestroyWindow(hwndDlg);
    return (ulReply);
}

/*
 *@@category: Helpers\PM helpers\Presentation parameters
 */

/* ******************************************************************
 *                                                                  *
 *   Presparams helpers                                             *
 *                                                                  *
 ********************************************************************/

/*
 *@@ winhQueryWindowFont:
 *      returns the window font presentation parameter
 *      in a newly allocated buffer, which you must
 *      free() afterwards. Returns NULL if not found.
 *
 *@@added V0.9.1 (2000-02-14) [umoeller]
 */

PSZ winhQueryWindowFont(HWND hwnd)
{
    CHAR  szNewFont[100] = "";
    WinQueryPresParam(hwnd,
                      PP_FONTNAMESIZE,
                      0,
                      NULL,
                      (ULONG)sizeof(szNewFont),
                      (PVOID)&szNewFont,
                      QPF_NOINHERIT);
    if (szNewFont[0] != 0)
        return (strdup(szNewFont));

    return (NULL);
}

/*
 *@@ winhSetWindowFont:
 *      this sets a window's font by invoking
 *      WinSetPresParam on it.
 *
 *      If (pszFont == NULL), a default font will be set
 *      (on Warp 4, "9.WarpSans", on Warp 3, "8.Helv").
 *
 *      winh.h also defines the winhSetDlgItemFont macro.
 *
 *      Returns TRUE if successful or FALSE otherwise.
 *
 *@@added V0.9.0 [umoeller]
 */

BOOL winhSetWindowFont(HWND hwnd,
                       PSZ pszFont)
{
    CHAR    szFont[256];

    if (pszFont == NULL)
    {
        if (doshIsWarp4())
            strhncpy0(szFont, "9.WarpSans", sizeof(szFont));
        else
            strhncpy0(szFont, "8.Helv", sizeof(szFont));
    }
    else
        strhncpy0(szFont, pszFont, sizeof(szFont));

    return (WinSetPresParam(hwnd,
                            PP_FONTNAMESIZE,
                            strlen(szFont)+1,
                            szFont));
}

/*
 *@@ winhSetControlsFont:
 *      this sets the font for all the controls of hwndDlg
 *      which have a control ID in the range of usIDMin to
 *      usIDMax. "Unused" IDs (i.e. -1) will also be set.
 *
 *      If (pszFont == NULL), a default font will be set
 *      (on Warp 4, "9.WarpSans", on Warp 3, "8.Helv").
 *
 *      Returns the no. of controls set.
 *
 *@@added V0.9.0 [umoeller]
 */

ULONG winhSetControlsFont(HWND hwndDlg,      // in: dlg to set
                          SHORT usIDMin,     // in: minimum control ID to be set (inclusive)
                          SHORT usIDMax,     // in: maximum control ID to be set (inclusive)
                          const char *pcszFont)  // in: font to use (e.g. "9.WarpSans") or NULL
{
    ULONG   ulrc = 0;
    HENUM   henum;
    HWND    hwndItem;
    CHAR    szFont[256];
    ULONG   cbFont;

    if (pcszFont == NULL)
    {
        if (doshIsWarp4())
            strhncpy0(szFont, "9.WarpSans", sizeof(szFont));
        else
            strhncpy0(szFont, "8.Helv", sizeof(szFont));
    }
    else
        strhncpy0(szFont, pcszFont, sizeof(szFont));
    cbFont = strlen(szFont)+1;

    // set font for all the dialog controls
    henum = WinBeginEnumWindows(hwndDlg);
    while ((hwndItem = WinGetNextWindow(henum)))
    {
        SHORT sID = WinQueryWindowUShort(hwndItem, QWS_ID);
        if (    (sID == -1)
             || ((sID >= usIDMin) && (sID <= usIDMax))
           )
            if (WinSetPresParam(hwndItem,
                                PP_FONTNAMESIZE,
                                cbFont,
                                szFont))
                // successful:
                ulrc++;
    }
    WinEndEnumWindows(henum);
    return (ulrc);
}

/*
 *@@ winhStorePresParam:
 *      this appends a new presentation parameter to an
 *      array of presentation parameters which can be
 *      passed to WinCreateWindow. This is preferred
 *      over setting the presparams using WinSetPresParams,
 *      because that call will cause a lot of messages.
 *
 *      On the first call, pppp _must_ be NULL. This
 *      will allocate memory for storing the given
 *      data as necessary and modify *pppp to point
 *      to the new array.
 *
 *      On subsequent calls with the same pppp, memory
 *      will be reallocated, the old data will be copied,
 *      and the new given data will be appended.
 *
 *      Use free() on your PPRESPARAMS pointer (whose
 *      address was passed) after WinCreateWindow.
 *
 *      Example:
 +      PPRESPARAMS ppp = NULL;
 +      CHAR szFont[] = "9.WarpSans";
 +      LONG lColor = CLR_WHITE;
 +      winhStorePresParam(&ppp, PP_FONTNAMESIZE, sizeof(szFont), szFont);
 +      winhStorePresParam(&ppp, PP_BACKGROUNDCOLOR, sizeof(lColor), &lColor);
 +      WinCreateWindow(...., ppp);
 +      free(ppp);
 *
 *@@added V0.9.0 [umoeller]
 */

BOOL winhStorePresParam(PPRESPARAMS *pppp,      // in: data pointer (modified)
                        ULONG ulAttrType,       // in: PP_* index
                        ULONG cbData,           // in: sizeof(*pData), e.g. sizeof(LONG)
                        PVOID pData)            // in: presparam data (e.g. a PLONG to a color)
{
    BOOL        brc = FALSE;
    if (pppp)
    {
        ULONG       cbOld = 0,
                    cbNew;
        PBYTE       pbTemp = 0;
        PPRESPARAMS pppTemp = 0;
        PPARAM      pppCopyTo = 0;

        if (*pppp != NULL)
            // subsequent calls:
            cbOld = (**pppp).cb;

        cbNew = sizeof(ULONG)       // PRESPARAMS.cb
                + cbOld             // old count, which does not include PRESPARAMS.cb
                + sizeof(ULONG)     // PRESPARAMS.aparam[0].id
                + sizeof(ULONG)     // PRESPARAMS.aparam[0].cb
                + cbData;           // PRESPARAMS.aparam[0].ab[]

        pbTemp = (PBYTE)malloc(cbNew);
        if (pbTemp)
        {
            pppTemp = (PPRESPARAMS)pbTemp;

            if (*pppp != NULL)
            {
                // copy old data
                memcpy(pbTemp, *pppp, cbOld + sizeof(ULONG)); // including PRESPARAMS.cb
                pppCopyTo = (PPARAM)(pbTemp             // new buffer
                                     + sizeof(ULONG)    // skipping PRESPARAMS.cb
                                     + cbOld);          // old PARAM array
            }
            else
                // first call:
                pppCopyTo = pppTemp->aparam;

            pppTemp->cb = cbNew - sizeof(ULONG);     // excluding PRESPARAMS.cb
            pppCopyTo->id = ulAttrType;
            pppCopyTo->cb = cbData;       // byte count of PARAM.ab[]
            memcpy(pppCopyTo->ab, pData, cbData);

            free(*pppp);
            *pppp = pppTemp;

            brc = TRUE;
        }
    }
    return (brc);
}

/*
 *@@ winhQueryPresColor:
 *      returns the specified color. This is queried in the
 *      following order:
 *
 *      1)  hwnd's pres params are searched for ulPP
 *          (which should be a PP_* index);
 *      2)  if (fInherit == TRUE), the parent windows
 *          are searched also;
 *      3)  if this fails or (fInherit == FALSE), WinQuerySysColor
 *          is called to get lSysColor (which should be a SYSCLR_*
 *          index).
 *
 *      The return value is always an RGB LONG, _not_ a color index.
 *      This is even true for the returned system colors, which are
 *      converted to RGB.
 *
 *      If you do any painting with this value, you should switch
 *      the HPS you're using to RGB mode (use gpihSwitchToRGB for that).
 *
 *      Some useful ulPP / lSysColor pairs
 *      (default values as in PMREF):
 *
 +          --  PP_FOREGROUNDCOLOR          SYSCLR_WINDOWTEXT (for most controls also)
 +                                          SYSCLR_WINDOWSTATICTEXT (for static controls)
 +                 Foreground color (default: black)
 +          --  PP_BACKGROUNDCOLOR          SYSCLR_BACKGROUND
 +                                          SYSCLR_DIALOGBACKGROUND
 +                                          SYSCLR_FIELDBACKGROUND (for disabled scrollbars)
 +                                          SYSCLR_WINDOW (application surface -- empty clients)
 +                 Background color (default: light gray)
 +          --  PP_ACTIVETEXTFGNDCOLOR
 +          --  PP_HILITEFOREGROUNDCOLOR    SYSCLR_HILITEFOREGROUND
 +                 Highlighted foreground color, for example for selected menu
 +                 (def.: white)
 +          --  PP_ACTIVETEXTBGNDCOLOR
 +          --  PP_HILITEBACKGROUNDCOLOR    SYSCLR_HILITEBACKGROUND
 +                 Highlighted background color (def.: dark gray)
 +          --  PP_INACTIVETEXTFGNDCOLOR
 +          --  PP_DISABLEDFOREGROUNDCOLOR  SYSCLR_MENUDISABLEDTEXT
 +                 Disabled foreground color (dark gray)
 +          --  PP_INACTIVETEXTBGNDCOLOR
 +          --  PP_DISABLEDBACKGROUNDCOLOR
 +                 Disabled background color
 +          --  PP_BORDERCOLOR              SYSCLR_WINDOWFRAME
 +                                          SYSCLR_INACTIVEBORDER
 +                 Border color (around pushbuttons, in addition to
 +                 the 3D colors)
 +          --  PP_ACTIVECOLOR              SYSCLR_ACTIVETITLE
 +                 Active color
 +          --  PP_INACTIVECOLOR            SYSCLR_INACTIVETITLE
 +                 Inactive color
 *
 *      For menus:
 +          --  PP_MENUBACKGROUNDCOLOR      SYSCLR_MENU
 +          --  PP_MENUFOREGROUNDCOLOR      SYSCLR_MENUTEXT
 +          --  PP_MENUHILITEBGNDCOLOR      SYSCLR_MENUHILITEBGND
 +          --  PP_MENUHILITEFGNDCOLOR      SYSCLR_MENUHILITE
 +          --  ??                          SYSCLR_MENUDISABLEDTEXT
 +
 *      For containers (according to the API ref. at EDM/2):
 +          --  PP_FOREGROUNDCOLOR          SYSCLR_WINDOWTEXT
 +          --  PP_BACKGROUNDCOLOR          SYSCLR_WINDOW
 +          --  PP_HILITEFOREGROUNDCOLOR    SYSCLR_HILITEFOREGROUND
 +          --  PP_HILITEBACKGROUNDCOLOR    SYSCLR_HILITEBACKGROUND
 +          --  PP_BORDERCOLOR
 +                  (used for separator lines, eg. in Details view)
 +          --  PP_ICONTEXTBACKGROUNDCOLOR
 +                  (column titles in Details view?!?)
 +
 *      For listboxes / entryfields / MLE's:
 +          --  PP_BACKGROUNDCOLOR          SYSCLR_ENTRYFIELD
 *
 *  PMREF has more of these.
 *
 *@@changed V0.9.0 [umoeller]: removed INI key query, using SYSCLR_* instead; function prototype changed
 *@@changed V0.9.0 [umoeller]: added fInherit parameter
 */

LONG winhQueryPresColor(HWND    hwnd,       // in: window to query
                        ULONG   ulPP,       // in: PP_* index
                        BOOL    fInherit,   // in: search parent windows too?
                        LONG    lSysColor)  // in: SYSCLR_* index
{
    ULONG   ul,
            attrFound,
            abValue[32];

    if (ulPP != (ULONG)-1)
        if ((ul = WinQueryPresParam(hwnd,
                                    ulPP,
                                    0,
                                    &attrFound,
                                    (ULONG)sizeof(abValue),
                                    (PVOID)&abValue,
                                    (fInherit)
                                         ? 0
                                         : QPF_NOINHERIT)))
            return (abValue[0]);

    // not found: get system color
    return (WinQuerySysColor(HWND_DESKTOP, lSysColor, 0));
}

/*
 *@@category: Helpers\PM helpers\Help (IPF)
 */

/* ******************************************************************
 *                                                                  *
 *   Help instance helpers                                          *
 *                                                                  *
 ********************************************************************/

/*
 *@@ winhCreateHelp:
 *      creates a help instance and connects it with the
 *      given frame window.
 *
 *      If (pszFileName == NULL), we'll retrieve the
 *      executable's fully qualified file name and
 *      replace the extension with .HLP simply. This
 *      avoids the typical "Help not found" errors if
 *      the program isn't started in its own directory.
 *
 *      If you have created a help table in memory, specify it
 *      with pHelpTable. To load a help table from the resources,
 *      specify hmod (or NULLHANDLE) and set pHelpTable to the
 *      following:
 +
 +          (PHELPTABLE)MAKELONG(usTableID, 0xffff)
 *
 *      Returns the help window handle or NULLHANDLE on errors.
 *
 *      Based on an EDM/2 code snippet.
 *
 *@@added V0.9.4 (2000-07-03) [umoeller]
 */

HWND winhCreateHelp(HWND    hwndFrame,      // in: app's frame window handle; can be NULLHANDLE
                    PSZ     pszFileName,    // in: help file name or NULL
                    HMODULE hmod,           // in: module with help table or NULLHANDLE (current)
                    PHELPTABLE pHelpTable,  // in: help table or resource ID
                    PSZ     pszWindowTitle) // in: help window title or NULL
{
    PPIB     ppib;
    PTIB     ptib;
    HELPINIT hi;
    PSZ      pszExt;
    CHAR     szName[ CCHMAXPATH ];
    HWND     hwndHelp;

    if (     (pszFileName == NULL)
          // || (*pszFileName)
       )
    {
        DosGetInfoBlocks(&ptib, &ppib);

        DosQueryModuleName(ppib->pib_hmte, sizeof(szName), szName);

        pszExt = strrchr(szName, '.');
        if (pszExt)
            strcpy(pszExt, ".hlp");
        else
            strcat(szName, ".hlp");

        pszFileName = szName;
    }

    hi.cb                       = sizeof(HELPINIT);
    hi.ulReturnCode             = 0;
    hi.pszTutorialName          = NULL;
    hi.phtHelpTable             = pHelpTable;
    hi.hmodHelpTableModule      = hmod;
    hi.hmodAccelActionBarModule = NULLHANDLE;
    hi.idAccelTable             = 0;
    hi.idActionBar              = 0;
    hi.pszHelpWindowTitle       = pszWindowTitle;
    hi.fShowPanelId             = CMIC_HIDE_PANEL_ID;
    hi.pszHelpLibraryName       = pszFileName;

    hwndHelp = WinCreateHelpInstance(WinQueryAnchorBlock(hwndFrame),
                                     &hi);
    if ((hwndFrame) && (hwndHelp))
    {
        WinAssociateHelpInstance(hwndHelp, hwndFrame);
    }

    return (hwndHelp);
}

/*
 *@@ winhDestroyHelp:
 *      destroys the help instance created by winhCreateHelp.
 *
 *      Based on an EDM/2 code snippet.
 *
 *@@added V0.9.4 (2000-07-03) [umoeller]
 */

void winhDestroyHelp(HWND hwndHelp,
                     HWND hwndFrame)    // can be NULLHANDLE if not used with winhCreateHelp
{
    if (hwndHelp)
    {
        if (hwndFrame)
            WinAssociateHelpInstance(NULLHANDLE, hwndFrame);
        WinDestroyHelpInstance(hwndHelp);
    }
}

/*
 *@@category: Helpers\PM helpers\Application control
 */

/* ******************************************************************
 *
 *   Application control
 *
 ********************************************************************/

/*
 *@@ CallBatchCorrectly:
 *      fixes the specified PROGDETAILS for
 *      command files in the executable part
 *      by inserting /C XXX into the parameters
 *      and setting the executable according
 *      to an environment variable.
 *
 *@@added V0.9.6 (2000-10-16) [umoeller]
 */

VOID CallBatchCorrectly(PPROGDETAILS pProgDetails,
                        PSZ *ppszParams,            // in/out: modified parameters (reallocated)
                        const char *pcszEnvVar,     // in: env var spec'g command proc
                                                    // (e.g. "OS2_SHELL"); can be NULL
                        const char *pcszDefProc)    // in: def't command proc (e.g. "CMD.EXE")
{
    // XXX.CMD file as executable:
    // fix args to /C XXX.CMD
    XSTRING strNewParams;
    xstrInit(&strNewParams, 200);
    xstrcpy(&strNewParams, "/C ");
    xstrcat(&strNewParams, pProgDetails->pszExecutable);
    if (*ppszParams)
    {
        // append old params
        xstrcat(&strNewParams, " ");
        xstrcat(&strNewParams, *ppszParams);
        free(*ppszParams);
    }
    *ppszParams = strNewParams.psz;
            // freed by caller

    // set executable to $(OS2_SHELL)
    pProgDetails->pszExecutable = NULL;
    if (pcszEnvVar)
        pProgDetails->pszExecutable = getenv(pcszEnvVar);
    if (!pProgDetails->pszExecutable)
        pProgDetails->pszExecutable = (PSZ)pcszDefProc;
                // should be on PATH
}

/*
 *@@ winhStartApp:
 *      wrapper around WinStartApp which fixes the
 *      specified PROGDETAILS to (hopefully) work
 *      work with all executable types:
 *
 *      1.  This fixes the executable info to support:
 *
 *          -- starting "*" executables (command prompts
 *             for OS/2, DOS, Win-OS/2);
 *
 *          -- starting ".CMD" and ".BAT" files as
 *             PROGDETAILS.pszExecutable.
 *
 *      2.  Handles and merges special and default
 *          environments for the app to be started.
 *          If PROGDETAILS.pszEnvironment is empty
 *          and the application is a Win-OS/2 app,
 *          this uses the default Win-OS/2 settings
 *          as specified in the "Win-OS/2" WPS settings
 *          object.
 *
 *      Since this calls WinStartApp in turn, this
 *      requires a message queue on the calling thread.
 *
 *@@added V0.9.6 (2000-10-16) [umoeller]
 */

HAPP winhStartApp(HWND hwndNotify,                  // in: notify window (as with WinStartApp)
                  const PROGDETAILS *pcProgDetails) // in: program data
{
    HAPP            happ = NULLHANDLE;
    PSZ             pszParamsPatched = NULL;
    BOOL            fIsWindowsApp = FALSE,
                    fIsWindowsEnhApp = FALSE;
    PROGDETAILS     ProgDetails;
    PSZ             pszWinOS2Env = 0;

    memcpy(&ProgDetails, pcProgDetails, sizeof(PROGDETAILS));
            // pointers still point into old prog details buffer
    ProgDetails.Length = sizeof(PROGDETAILS);
    ProgDetails.progt.fbVisible = SHE_VISIBLE;
    ProgDetails.pszEnvironment = 0; // "WORKPLACE\0\0";
    memset(&ProgDetails.swpInitial, 0, sizeof(SWP));

    // duplicate parameters...
    // we need this for string manipulations below...
    if (ProgDetails.pszParameters)
        pszParamsPatched = strdup(ProgDetails.pszParameters);

    // _Pmpf((__FUNCTION__ ": old progc: 0x%lX", pcProgDetails->progt.progc));
    // _Pmpf(("  pszTitle: %s", (ProgDetails.pszTitle) ? ProgDetails.pszTitle : NULL));
    // _Pmpf(("  pszIcon: %s", (ProgDetails.pszIcon) ? ProgDetails.pszIcon : NULL));

    // program type fixups
    switch (ProgDetails.progt.progc)
    {
        case ((ULONG)-1):       // we get that sometimes...
        case PROG_DEFAULT:
            // ###
        break;
    }

    // now try again...
    switch (ProgDetails.progt.progc)
    {
        case PROG_31_ENH:
        case PROG_31_ENHSEAMLESSCOMMON:
        case PROG_31_ENHSEAMLESSVDM:
            fIsWindowsApp = TRUE;
            fIsWindowsEnhApp = TRUE;
        break;

        case PROG_WINDOW_AUTO:
#ifdef PROG_30_STD
        case PROG_30_STD:
#endif
        case PROG_31_STD:
        case PROG_WINDOW_REAL:
#ifdef PROG_30_STDSEAMLESSVDM
        case PROG_30_STDSEAMLESSVDM:
#endif
        case PROG_31_STDSEAMLESSVDM:
        case PROG_30_STDSEAMLESSCOMMON:
        case PROG_31_STDSEAMLESSCOMMON:
            fIsWindowsApp = TRUE;
        break;
    }

    /*
     * command lines fixups:
     *
     */

    if (strcmp(ProgDetails.pszExecutable, "*") == 0)
    {
        /*
         * "*" for command sessions:
         *
         */

        if (fIsWindowsEnhApp)
        {
            // enhanced Win-OS/2 session:
            XSTRING str2;
            xstrInit(&str2, 200);
            xstrcpy(&str2, "/3 ");
            if (pszParamsPatched)
            {
                // append existing params
                xstrcat(&str2, pszParamsPatched);
                free(pszParamsPatched);
            }

            pszParamsPatched = str2.psz;
        }

        if (fIsWindowsApp)
        {
            // cheat: WinStartApp doesn't support NULL
            // for Win-OS2 sessions, so manually start winos2.com
            ProgDetails.pszExecutable = "WINOS2.COM";
            // this is a DOS app, so fix this to DOS fullscreen
            ProgDetails.progt.progc = PROG_VDM;
        }
        else
            // for all other executable types
            // (including OS/2 and DOS sessions),
            // set pszExecutable to NULL; this will
            // have WinStartApp start a cmd shell
            ProgDetails.pszExecutable = NULL;

    } // end if (strcmp(pProgDetails->pszExecutable, "*") == 0)
    else
        switch (ProgDetails.progt.progc)
        {
            /*
             *  .CMD files fixups
             *
             */

            case PROG_FULLSCREEN:       // OS/2 fullscreen
            case PROG_WINDOWABLEVIO:    // OS/2 window
            {
                PSZ pszExtension = doshGetExtension(ProgDetails.pszExecutable);
                if (pszExtension)
                {
                    if (stricmp(pszExtension, "CMD") == 0)
                    {
                        CallBatchCorrectly(&ProgDetails,
                                           &pszParamsPatched,
                                           "OS2_SHELL",
                                           "CMD.EXE");
                    }
                }
            break; }

            case PROG_VDM:              // DOS fullscreen
            case PROG_WINDOWEDVDM:      // DOS window
            {
                PSZ pszExtension = doshGetExtension(ProgDetails.pszExecutable);
                if (pszExtension)
                {
                    if (stricmp(pszExtension, "BAT") == 0)
                    {
                        CallBatchCorrectly(&ProgDetails,
                                           &pszParamsPatched,
                                           NULL,
                                           "COMMAND.COM");
                    }
                }
            break; }
        } // end switch (ProgDetails.progt.progc)

    /*
     *  Fix environment for Win-OS/2
     *
     */

    if (    !(xstrIsString(ProgDetails.pszEnvironment)) // env empty
         && (fIsWindowsApp)                             // and win-os2 app
       )
    {
        ULONG ulSize = 0;
        // get default environment (from Win-OS/2 settings object)
        // from OS2.INI
        PSZ pszDefEnv = prfhQueryProfileData(HINI_USER,
                                             "WINOS2",
                                             "PM_GlobalWindows31Settings",
                                             &ulSize);
        if (pszDefEnv)
        {
            PSZ pszDefEnv2 = (PSZ)malloc(ulSize + 2);
            if (pszDefEnv2)
            {
                PSZ p = pszDefEnv2;
                memset(pszDefEnv2, 0, ulSize + 2);
                memcpy(pszDefEnv2, pszDefEnv, ulSize);

                for (p = pszDefEnv2;
                     p < pszDefEnv2 + ulSize;
                     p++)
                    if (*p == ';')
                        *p = 0;

                // okay.... now we got an OS/2-style environment
                // with 0, 0, 00 strings

                pszWinOS2Env = pszDefEnv2;  // freed below

                // use this
                ProgDetails.pszEnvironment = pszWinOS2Env;
            }

            free(pszDefEnv);
        }
    }

    // _Pmpf((__FUNCTION__ ": calling WinStartApp"));
    // _Pmpf(("    exec: %s",
    //             (ProgDetails.pszExecutable)
                    // ? ProgDetails.pszExecutable
                // : "NULL"));
    // _Pmpf(("    startupDir: %s",
       //      (ProgDetails.pszStartupDir)
          //       ? ProgDetails.pszStartupDir
             //    : "NULL"));
    // _Pmpf(("    params: %s",
       //      (pszParamsPatched)
          //       ? pszParamsPatched
             //    : "NULL"));
    // _Pmpf(("    new progc: 0x%lX", ProgDetails.progt.progc));

    ProgDetails.pszParameters = pszParamsPatched;

    happ = WinStartApp(hwndNotify,
                                // receives WM_APPTERMINATENOTIFY
                       &ProgDetails,
                       pszParamsPatched,
                       NULL,            // "reserved", PMREF says...
                       SAF_INSTALLEDCMDLINE);
                            // we MUST use SAF_INSTALLEDCMDLINE
                            // or no Win-OS/2 session will start...
                            // whatever is going on here... Warp 4 FP11

                            // do not use SAF_STARTCHILDAPP, or the
                            // app will be terminated automatically

    // _Pmpf((__FUNCTION__ ": got happ 0x%lX", happ));

    if (pszParamsPatched)
        free(pszParamsPatched);
    if (pszWinOS2Env)
        free(pszWinOS2Env);

    return (happ);
}

/*
 *@@ winhAnotherInstance:
 *      this tests whether another instance of the same
 *      application is already running.
 *
 *      To identify instances of the same application, the
 *      application must call this function during startup
 *      with the unique name of an OS/2 semaphore. As with
 *      all OS/2 semaphores, the semaphore name must begin
 *      with "\\SEM32\\". The semaphore isn't really used
 *      except for testing for its existence, since that
 *      name is unique among all processes.
 *
 *      If another instance is found, TRUE is returned. If
 *      (fSwitch == TRUE), that instance is switched to,
 *      using the tasklist.
 *
 *      If no other instance is found, FALSE is returned only.
 *
 *      Based on an EDM/2 code snippet.
 *
 *@@added V0.9.0 (99-10-22) [umoeller]
 */

BOOL winhAnotherInstance(PSZ pszSemName,    // in: semaphore ID
                         BOOL fSwitch)      // in: if TRUE, switch to first instance if running
{
    HMTX hmtx;

    if (DosCreateMutexSem(pszSemName,
                          &hmtx,
                          DC_SEM_SHARED,
                          TRUE)
              == NO_ERROR)
        // semapore created: this doesn't happen if the semaphore
        // exists already, so no other instance is running
        return (FALSE);

    // else: instance running
    hmtx = NULLHANDLE;

    // switch to other instance?
    if (fSwitch)
    {
        // yes: query mutex creator
        if (DosOpenMutexSem(pszSemName,
                            &hmtx)
                    == NO_ERROR)
        {
            PID     pid = 0;
            TID     tid = 0;        // unused
            ULONG   ulCount;        // unused

            if (DosQueryMutexSem(hmtx, &pid, &tid, &ulCount) == NO_ERROR)
            {
                HSWITCH hswitch = WinQuerySwitchHandle(NULLHANDLE, pid);
                if (hswitch != NULLHANDLE)
                    WinSwitchToProgram(hswitch);
            }

            DosCloseMutexSem(hmtx);
        }
    }

    return (TRUE);      // another instance exists
}

/*
 *@@ winhAddToTasklist:
 *      this adds the specified window to the tasklist
 *      with hIcon as its program icon (which is also
 *      set for the main window). This is useful for
 *      the old "dialog as main window" trick.
 *
 *      Returns the HSWITCH of the added entry.
 */

HSWITCH winhAddToTasklist(HWND hwnd,       // in: window to add
                          HPOINTER hIcon)  // in: icon for main window
{
    SWCNTRL     swctl;
    HSWITCH hswitch = 0;
    swctl.hwnd = hwnd;                     // window handle
    swctl.hwndIcon = hIcon;                // icon handle
    swctl.hprog = NULLHANDLE;              // program handle (use default)
    WinQueryWindowProcess(hwnd, &(swctl.idProcess), NULL);
                                           // process identifier
    swctl.idSession = 0;                   // session identifier ?
    swctl.uchVisibility = SWL_VISIBLE;     // visibility
    swctl.fbJump = SWL_JUMPABLE;           // jump indicator
    // get window title from window titlebar
    if (hwnd)
        WinQueryWindowText(hwnd, sizeof(swctl.szSwtitle), swctl.szSwtitle);
    swctl.bProgType = PROG_DEFAULT;        // program type
    hswitch = WinAddSwitchEntry(&swctl);

    // give the main window the icon
    if ((hwnd) && (hIcon))
        WinSendMsg(hwnd,
                   WM_SETICON,
                   (MPARAM)hIcon,
                   NULL);

    return (hswitch);
}

/*
 *@@category: Helpers\PM helpers\Miscellaneous
 */

/* ******************************************************************
 *                                                                  *
 *   Miscellaneous                                                  *
 *                                                                  *
 ********************************************************************/

/*
 *@@ winhSleep:
 *      sleeps at least the specified amount of time,
 *      without blocking the message queue.
 *
 *@@added V0.9.4 (2000-07-11) [umoeller]
 */

VOID winhSleep(HAB hab,
               ULONG ulSleep)    // in: sleep time in milliseconds
{
    ULONG ul = 0;
    QMSG qmsg;
    for (ul = 0;
         ul < (ulSleep / 50);
         ul++)
    {
        DosSleep(50);
        while (WinPeekMsg(hab,
                          &qmsg, 0, 0, 0,
                          PM_REMOVE))
            WinDispatchMsg(hab, &qmsg);

    }
}

/*
 *@@ winhFileDlg:
 *      one-short function for opening an "Open" file
 *      dialog.
 *
 *      On input, pszFile specifies the directory and
 *      file specification (e.g. "F:\*.txt").
 *
 *      Returns TRUE if the user pressed OK. In that
 *      case, the fully qualified filename is written
 *      into pszFile again.
 *
 *      Returns FALSE if the user pressed Cancel.
 *
 *      Notes about flFlags:
 *
 *      -- WINH_FOD_SAVEDLG: display a "Save As" dialog.
 *         Otherwise an "Open" dialog is displayed.
 *
 *      -- WINH_FOD_INILOADDIR: load a directory from the
 *         specified INI key and switch the dlg to it.
 *         In that case, on input, pszFile must only
 *         contain the file filter without any path
 *         specification, because that is loaded from
 *         the INI key. If the INI key does not exist,
 *         the current process directory will be used.
 *
 *      -- WINH_FOD_INISAVEDIR: if the user presses OK,
 *         the directory of the selected file is written
 *         to the specified INI key so that it can be
 *         reused later. This flag is independent of
 *         WINH_FOD_INISAVEDIR: you can specify none,
 *         one, or both of them.
 *
 *@@added V0.9.3 (2000-04-29) [umoeller]
 */

BOOL winhFileDlg(HWND hwndOwner,    // in: owner for file dlg
                 PSZ pszFile,       // in: file mask; out: fully q'd filename
                                    //    (should be CCHMAXPATH in size)
                 ULONG flFlags,     // in: any combination of the following:
                                    // -- WINH_FOD_SAVEDLG: save dlg; else open dlg
                                    // -- WINH_FOD_INILOADDIR: load FOD path from INI
                                    // -- WINH_FOD_INISAVEDIR: store FOD path to INI on OK
                 HINI hini,         // in: INI file to load/store last path from (can be HINI_USER)
                 PSZ pszApplication, // in: INI application to load/store last path from
                 PSZ pszKey)        // in: INI key to load/store last path from
{
    FILEDLG fd;
    memset(&fd, 0, sizeof(FILEDLG));
    fd.cbSize = sizeof(FILEDLG);
    fd.fl = FDS_CENTER;

    if (flFlags & WINH_FOD_SAVEDLG)
        fd.fl |= FDS_SAVEAS_DIALOG;
    else
        fd.fl |= FDS_OPEN_DIALOG;

    // default: copy pszFile
    strcpy(fd.szFullFile, pszFile);

    if ( (hini) && (flFlags & WINH_FOD_INILOADDIR) )
    {
        // overwrite with initial directory for FOD from OS2.INI
        if (PrfQueryProfileString(hini,
                                  pszApplication,
                                  pszKey,
                                  "",      // default string
                                  fd.szFullFile,
                                  sizeof(fd.szFullFile)-10)
                    >= 2)
        {
            // found: append "\*"
            strcat(fd.szFullFile, "\\");
            strcat(fd.szFullFile, pszFile);
        }
    }

    if (    WinFileDlg(HWND_DESKTOP,    // parent
                       hwndOwner, // owner
                       &fd)
        && (fd.lReturn == DID_OK)
       )
    {
        // save path back?
        if (    (hini)
             && (flFlags & WINH_FOD_INISAVEDIR)
           )
        {
            // get the directory that was used
            PSZ p = strrchr(fd.szFullFile, '\\');
            if (p)
            {
                // contains directory:
                // copy to OS2.INI
                PSZ pszDir = strhSubstr(fd.szFullFile, p);
                if (pszDir)
                {
                    PrfWriteProfileString(hini,
                                          pszApplication,
                                          pszKey, // "XWPSound:LastDir"
                                          pszDir);
                    free(pszDir);
                }
            }
        }

        strcpy(pszFile, fd.szFullFile);

        return (TRUE);
    }

    return (FALSE);
}

/*
 *@@ winhSetWaitPointer:
 *      this sets the mouse pointer to "Wait".
 *      Returns the previous pointer (HPOINTER),
 *      which should be stored somewhere to be
 *      restored later. Example:
 +          HPOINTER hptrOld = winhSetWaitPointer();
 +          ...
 +          WinSetPointer(HWND_DESKTOP, hptrOld);
 */

HPOINTER winhSetWaitPointer(VOID)
{
    HPOINTER hptr = WinQueryPointer(HWND_DESKTOP);
    WinSetPointer(HWND_DESKTOP,
                  WinQuerySysPointer(HWND_DESKTOP,
                                     SPTR_WAIT,
                                     FALSE));   // no copy
    return (hptr);
}

/*
 *@@ winhQueryWindowText:
 *      this returns the window text of the specified
 *      HWND in a newly allocated buffer, which has
 *      the exact size of the window text.
 *
 *      This buffer must be free()'d later.
 */

PSZ winhQueryWindowText(HWND hwnd)
{
    PSZ     pszText = NULL;
    ULONG   cbText = WinQueryWindowTextLength(hwnd);
                                    // additional null character
    if (cbText)
    {
        pszText = (PSZ)malloc(cbText + 1);
        if (pszText)
            WinQueryWindowText(hwnd,
                               cbText + 1,
                               pszText);
    }
    return (pszText);
}

/*
 *@@ winhReplaceWindowText:
 *      this is a combination of winhQueryWindowText
 *      and xstrrpl (stringh.c) to replace substrings
 *      in a window.
 *
 *      This is useful for filling in placeholders
 *      a la "%1" in control windows, e.g. static
 *      texts.
 *
 *      This replaces only the first occurence of
 *      pszSearch.
 *
 *      Returns TRUE only if the window exists and
 *      the search string was replaced.
 *
 *@@added V0.9.0 [umoeller]
 */

BOOL winhReplaceWindowText(HWND hwnd,           // in: window whose text is to be modified
                           PSZ pszSearch,       // in: search string (e.g. "%1")
                           PSZ pszReplaceWith)  // in: replacement string for pszSearch
{
    BOOL    brc = FALSE;
    PSZ     pszText = winhQueryWindowText(hwnd);
    if (pszText)
    {
        if (strhrpl(&pszText, 0, pszSearch, pszReplaceWith, 0) > 0)
        {
            WinSetWindowText(hwnd, pszText);
            brc = TRUE;
        }
        free(pszText);
    }
    return (brc);
}

/*
 *@@ winhEnableDlgItems:
 *      this enables/disables a whole range of controls
 *      in a window by enumerating the child windows
 *      until usIDFirst is found. If so, that subwindow
 *      is enabled/disabled and all the following windows
 *      in the enumeration also, until usIDLast is found.
 *
 *      Note that this affects _all_ controls following
 *      the usIDFirst window, no matter what ID they have
 *      (even if "-1"), until usIDLast is found.
 *
 *      Returns the no. of controls which were enabled/disabled
 *      (null if none).
 *
 *@@added V0.9.0 [umoeller]
 *@@changed V0.9.1 (99-12-20) [umoeller]: renamed from winhEnableDlgItems
 */

ULONG winhEnableControls(HWND hwndDlg,                  // in: dialog window
                         USHORT usIDFirst,              // in: first affected control ID
                         USHORT usIDLast,               // in: last affected  control ID (inclusive)
                         BOOL fEnable)
{
    HENUM   henum1 = NULLHANDLE;
    HWND    hwndThis = NULLHANDLE;
    ULONG   ulCount = 0;

    henum1 = WinBeginEnumWindows(hwndDlg);
    while ((hwndThis = WinGetNextWindow(henum1)) != NULLHANDLE)
    {
        USHORT usIDCheckFirst = WinQueryWindowUShort(hwndThis, QWS_ID),
               usIDCheckLast;
        if (usIDCheckFirst == usIDFirst)
        {
            WinEnableWindow(hwndThis, fEnable);
            ulCount++;

            while ((hwndThis = WinGetNextWindow(henum1)) != NULLHANDLE)
            {
                WinEnableWindow(hwndThis, fEnable);
                ulCount++;
                usIDCheckLast = WinQueryWindowUShort(hwndThis, QWS_ID);
                if (usIDCheckLast == usIDLast)
                    break;
            }

            break;  // outer loop
        }
    }
    WinEndEnumWindows(henum1);
    return (ulCount);
}

/*
 *@@ winhCreateStdWindow:
 *      much like WinCreateStdWindow, but this one
 *      allows you to have the standard window
 *      positioned automatically, using a given
 *      SWP structure (*pswpFrame).
 *
 *      Alternatively, you can set pswpFrame to NULL
 *      and specify FCF_SHELLPOSITION with flFrameCreateFlags.
 *      If you want the window to be shown, specify
 *      SWP_SHOW (and maybe SWP_ACTIVATE) in *pswpFrame.
 *
 *@@added V0.9.0 [umoeller]
 *@@changed V0.9.5 (2000-08-13) [umoeller]: flStyleClient never worked, fixed
 */

HWND winhCreateStdWindow(HWND hwndFrameParent,      // in: normally HWND_DESKTOP
                         PSWP pswpFrame,            // in: frame wnd pos
                         ULONG flFrameCreateFlags,  // in: FCF_* flags
                         ULONG ulFrameStyle,        // in: WS_* flags (e.g. WS_VISIBLE, WS_ANIMATE)
                         PSZ pszFrameTitle,
                         ULONG ulResourcesID,       // in: according to FCF_* flags
                         PSZ pszClassClient,
                         ULONG flStyleClient,
                         ULONG ulID,                // in: frame window ID
                         PVOID pClientCtlData,      // in: pCtlData structure pointer for client
                         PHWND phwndClient)         // out: created client wnd
{
    FRAMECDATA  fcdata;
    HWND        hwndFrame;
    RECTL       rclClient;

    fcdata.cb            = sizeof(FRAMECDATA);
    fcdata.flCreateFlags = flFrameCreateFlags;
    fcdata.hmodResources = (HMODULE)NULL;
    fcdata.idResources   = ulResourcesID;

    /* Create the frame and client windows.  */
    hwndFrame = WinCreateWindow(hwndFrameParent,
                                WC_FRAME,
                                pszFrameTitle,
                                ulFrameStyle,
                                0,0,0,0,         // size and position = 0
                                NULLHANDLE,      // no owner
                                HWND_TOP,        // z-order
                                ulID,            // frame window ID
                                &fcdata,         // frame class data
                                NULL);           // no presparams

    if (hwndFrame)
    {
        *phwndClient = WinCreateWindow(hwndFrame,      // parent
                                       pszClassClient, // class
                                       NULL,           // no title
                                       flStyleClient,  // style
                                       0,0,0,0,        // size and position = 0
                                       hwndFrame,      // owner
                                       HWND_BOTTOM,    // bottom z-order
                                       FID_CLIENT,     // frame window ID
                                       pClientCtlData, // class data
                                       NULL);          // no presparams

        if (*phwndClient)
        {
            if (pswpFrame)
                // position frame
                WinSetWindowPos(hwndFrame,
                                pswpFrame->hwndInsertBehind,
                                pswpFrame->x,
                                pswpFrame->y,
                                pswpFrame->cx,
                                pswpFrame->cy,
                                pswpFrame->fl);

            // position client
            WinQueryWindowRect(hwndFrame, &rclClient);
            WinCalcFrameRect(hwndFrame, &rclClient,
                             TRUE);     // calc client from frame
            WinSetWindowPos(*phwndClient,
                            HWND_TOP,
                            rclClient.xLeft,
                            rclClient.yBottom,
                            rclClient.xRight - rclClient.xLeft,
                            rclClient.yTop - rclClient.yBottom,
                            SWP_MOVE | SWP_SIZE | SWP_SHOW);
        }
    }
    return (hwndFrame);
}

/*
 *@@ winhRepaintWindows:
 *      this repaints all children of hwndParent.
 *      If this is passed as HWND_DESKTOP, the
 *      whole screen is repainted.
 */

VOID winhRepaintWindows(HWND hwndParent)
{
    HWND    hwndTop;
    HENUM   henum = WinBeginEnumWindows(HWND_DESKTOP);
    while ((hwndTop = WinGetNextWindow(henum)))
        if (WinIsWindowShowing(hwndTop))
            WinInvalidateRect(hwndTop, NULL, TRUE);
    WinEndEnumWindows(henum);
}

/*
 *@@ winhFindMsgQueue:
 *      returns the message queue which matches
 *      the given process and thread IDs. Since,
 *      per IBM definition, every thread may only
 *      have one MQ, this should be unique.
 *
 *@@added V0.9.2 (2000-03-08) [umoeller]
 */

HMQ winhFindMsgQueue(PID pid,           // in: process ID
                     TID tid,           // in: thread ID
                     HAB* phab)         // out: anchor block
{
    HWND    hwndThis = 0,
            rc = 0;
    HENUM   henum = WinBeginEnumWindows(HWND_OBJECT);
    while ((hwndThis = WinGetNextWindow(henum)))
    {
        CHAR    szClass[200];
        if (WinQueryClassName(hwndThis, sizeof(szClass), szClass))
        {
            if (strcmp(szClass, "#32767") == 0)
            {
                // message queue window:
                PID pidWin = 0;
                TID tidWin = 0;
                WinQueryWindowProcess(hwndThis,
                                      &pidWin,
                                      &tidWin);
                if (    (pidWin == pid)
                     && (tidWin == tid)
                   )
                {
                    // get HMQ from window words
                    rc = WinQueryWindowULong(hwndThis, QWL_HMQ);
                    if (rc)
                        if (phab)
                            *phab = WinQueryAnchorBlock(hwndThis);
                    break;
                }
            }
        }
    }
    WinEndEnumWindows(henum);

    return (rc);
}

/*
 *@@ winhFindHardErrorWindow:
 *      this searches all children of HWND_OBJECT
 *      for the PM hard error windows, which are
 *      invisible most of the time. When a hard
 *      error occurs, that window is made a child
 *      of HWND_DESKTOP instead.
 *
 *      Stolen from ProgramCommander/2 (C) Roman Stangl.
 *
 *@@added V0.9.3 (2000-04-27) [umoeller]
 */

VOID winhFindPMErrorWindows(HWND *phwndHardError,  // out: hard error window
                            HWND *phwndSysError)   // out: system error window
{
    PID     pidObject;  // HWND_OBJECT's process and thread id
    TID     tidObject;
    PID     pidObjectChild;     // HWND_OBJECT's child window process and thread id
    TID     tidObjectChild;
    HENUM   henumObject;  // HWND_OBJECT enumeration handle
    HWND    hwndObjectChild;   // Window handle of current HWND_OBJECT child
    UCHAR   ucClassName[32];  // Window class e.g. #1 for WC_FRAME
    CLASSINFO classinfoWindow;  // Class info of current HWND_OBJECT child

    *phwndHardError = NULLHANDLE;
    *phwndSysError = NULLHANDLE;

    // query HWND_OBJECT's window process
    WinQueryWindowProcess(WinQueryObjectWindow(HWND_DESKTOP), &pidObject, &tidObject);
    // enumerate all child windows of HWND_OBJECT
    henumObject = WinBeginEnumWindows(HWND_OBJECT);
    while ((hwndObjectChild = WinGetNextWindow(henumObject)) != NULLHANDLE)
    {
        // see if the current HWND_OBJECT child window runs in the
        // process of HWND_OBJECT (PM)
        WinQueryWindowProcess(hwndObjectChild, &pidObjectChild, &tidObjectChild);
        if (pidObject == pidObjectChild)
        {
            // get the child window's data
            WinQueryClassName(hwndObjectChild,
                              sizeof(ucClassName),
                              (PCH)ucClassName);
            WinQueryClassInfo(WinQueryAnchorBlock(hwndObjectChild),
                              (PSZ)ucClassName,
                              &classinfoWindow);
            if (    (!strcmp((PSZ)ucClassName, "#1")
                 || (classinfoWindow.flClassStyle & CS_FRAME))
               )
            {
                // if the child window is a frame window and running in
                // HWND_OBJECT's (PM's) window process, it must be the
                // PM Hard Error or System Error window
                WinQueryClassName(WinWindowFromID(hwndObjectChild,
                                                  FID_CLIENT),
                                  sizeof(ucClassName),
                                  (PSZ)ucClassName);
                if (!strcmp((PSZ)ucClassName, "PM Hard Error"))
                {
                    *phwndHardError = hwndObjectChild;
                    if (*phwndSysError)
                        // we found the other one already:
                        // stop searching, we got both
                        break;
                }
                else
                {
                    printf("Utility: Found System Error %08X\n", (int)hwndObjectChild);
                    *phwndSysError = hwndObjectChild;
                    if (*phwndHardError)
                        // we found the other one already:
                        // stop searching, we got both
                        break;
                }
            }
        } // end if (pidObject == pidObjectChild)
    } // end while ((hwndObjectChild = WinGetNextWindow(henumObject)) != NULLHANDLE)
    WinEndEnumWindows(henumObject);
}

/*
 *@@ winhCreateFakeDesktop:
 *      this routine creates and displays a frameless window over
 *      the whole screen in the color of PM's Desktop to fool the
 *      user that all windows have been closed (which in fact might
 *      not be the case).
 *      This window's background color is set to the Desktop's
 *      (PM's one, not the WPS's one).
 *      Returns the HWND of this window.
 */

HWND winhCreateFakeDesktop(HWND hwndSibling)
{
    // presparam for background
    typedef struct _BACKGROUND
    {
        ULONG   cb;     // length of the aparam parameter, in bytes
        ULONG   id;     // attribute type identity
        ULONG   cb2;    // byte count of the ab parameter
        RGB     rgb;    // attribute value
    } BACKGROUND;

    BACKGROUND  background;
    LONG        lDesktopColor;

    // create fake desktop window = empty window with
    // the size of full screen
    lDesktopColor = WinQuerySysColor(HWND_DESKTOP,
                                     SYSCLR_BACKGROUND,
                                     0);
    background.cb = sizeof(background.id)
                  + sizeof(background.cb)
                  + sizeof(background.rgb);
    background.id = PP_BACKGROUNDCOLOR;
    background.cb2 = sizeof(RGB);
    background.rgb.bBlue = (CHAR1FROMMP(lDesktopColor));
    background.rgb.bGreen= (CHAR2FROMMP(lDesktopColor));
    background.rgb.bRed  = (CHAR3FROMMP(lDesktopColor));

    return (WinCreateWindow(HWND_DESKTOP,  // parent window
                            WC_FRAME,      // class name
                            "",            // window text
                            WS_VISIBLE,    // window style
                            0, 0,          // position and size
                            WinQuerySysValue(HWND_DESKTOP, SV_CXSCREEN),
                            WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN),
                            NULLHANDLE,    // owner window
                            hwndSibling,   // sibling window
                            1,             // window id
                            NULL,          // control data
                            &background)); // presentation parms
}

/*
 *@@ winhAssertWarp4Notebook:
 *      this takes hwndDlg as a notebook dialog page and
 *      goes thru all its controls. If a control with an
 *      ID <= udIdThreshold is found, this is assumed to
 *      be a button which is to be given the BS_NOTEBOOKBUTTON
 *      style. You should therefore give all your button
 *      controls which should be moved such an ID.
 *
 *      You can also specify how many dialog units
 *      all the other controls will be moved downward in
 *      ulDownUnits; this is useful to fill up the space
 *      which was used by the buttons before moving them.
 *      Returns TRUE if anything was changed.
 *
 *      This function is useful if you wish to create
 *      notebook pages using dlgedit.exe which are compatible
 *      with both Warp 3 and Warp 4. This should be executed
 *      in WM_INITDLG of the notebook dlg function if the app
 *      has determined that it is running on Warp 4.
 */

BOOL winhAssertWarp4Notebook(HWND hwndDlg,
                             USHORT usIdThreshold,    // in: ID threshold
                             ULONG ulDownUnits)       // in: dialog units or 0
{
    BOOL brc = FALSE;

    if (doshIsWarp4())
    {
        POINTL ptl;
        HWND hwndItem;
        HENUM henum = 0;

        BOOL    fIsVisible = WinIsWindowVisible(hwndDlg);
        if (ulDownUnits)
        {
            ptl.x = 0;
            ptl.y = ulDownUnits;
            WinMapDlgPoints(hwndDlg, &ptl, 1, TRUE);
        }

        if (fIsVisible)
            WinEnableWindowUpdate(hwndDlg, FALSE);

        henum = WinBeginEnumWindows(hwndDlg);
        while ((hwndItem = WinGetNextWindow(henum)))
        {
            USHORT usId = WinQueryWindowUShort(hwndItem, QWS_ID);
            // _Pmpf(("hwndItem: 0x%lX, ID: 0x%lX", hwndItem, usId));
            if (usId <= usIdThreshold)
            {
                // pushbutton to change:
                // _Pmpf(("  Setting bit"));
                WinSetWindowBits(hwndItem,
                                 QWL_STYLE,
                                 BS_NOTEBOOKBUTTON, BS_NOTEBOOKBUTTON);
                brc = TRUE;
            }
            else
                // no pushbutton to change: move downwards
                // if desired
                if (ulDownUnits)
                {
                    SWP swp;
                    // _Pmpf(("Moving downwards %d pixels", ptl.y));
                    WinQueryWindowPos(hwndItem, &swp);
                    WinSetWindowPos(hwndItem, 0,
                                    swp.x,
                                    swp.y - ptl.y,
                                    0, 0,
                                    SWP_MOVE);
                }
        }
        WinEndEnumWindows(henum);

        if (fIsVisible)
            WinShowWindow(hwndDlg, TRUE);
    }

    return (brc);
}

/*
 *@@ winhDrawFormattedText:
 *      this func takes a rectangle and draws pszText into
 *      it, breaking the words as neccessary. The line spacing
 *      is determined from the font currently selected in hps.
 *
 *      As opposed to WinDrawText, this can draw several lines
 *      at once, and format the _complete_ text according to the
 *      flCmd parameter, which is like with WinDrawText.
 *
 *      After this function returns, *prcl is modified like this:
 *      -- yTop and yBottom contain the upper and lower boundaries
 *         which were needed to draw the text. This depends on
 *         whether DT_TOP etc. were specified.
 *         To get the height of the rectangle used, calculate the
 *         delta between yTop and yBottom.
 *
 *      -- xLeft and xRight are modified to contain the outmost
 *         left and right coordinates which were needed to draw
 *         the text. This will be set to the longest line which
 *         was encountered.
 *
 *      You can specify DT_QUERYEXTENT with flDraw to only have
 *      these text boundaries calculated without actually drawing.
 *
 *      This returns the number of lines drawn.
 *
 *      Note that this calls WinDrawText with DT_TEXTATTRS set,
 *      that is, the current text primitive attributes will be
 *      used (fonts and colors).
 *
 *@@changed V0.9.0 [umoeller]: prcl.xLeft and xRight are now updated too upon return
 */

ULONG winhDrawFormattedText(HPS hps,     // in: presentation space; its settings
                                         // are used, but not altered
                            PRECTL prcl, // in/out: rectangle to use for drawing
                                         // (modified)
                            PSZ pszText, // in: text to draw (zero-terminated)
                            ULONG flCmd) // in: flags like in WinDrawText; I have
                                         // only tested DT_TOP and DT_LEFT though.
                                         // DT_WORDBREAK | DT_TEXTATTRS are always
                                         // set.
                                         // You can specify DT_QUERYEXTENT to only
                                         // have prcl calculated without drawing.
{
    PSZ     p = pszText;
    LONG    lDrawn = 1,
            lTotalDrawn = 0,
            lLineCount = 0,
            lOrigYTop = prcl->yTop;
    ULONG   ulTextLen = strlen(pszText),
            ulCharHeight,
            flCmd2,
            xLeftmost = prcl->xRight,
            xRightmost = prcl->xLeft;
    RECTL   rcl2;

    flCmd2 = flCmd | DT_WORDBREAK | DT_TEXTATTRS;

    ulCharHeight = gpihQueryLineSpacing(hps, pszText);

    while (   (lDrawn)
           && (lTotalDrawn < ulTextLen)
          )
    {
        memcpy(&rcl2, prcl, sizeof(rcl2));
        lDrawn = WinDrawText(hps,
                             ulTextLen-lTotalDrawn,
                             p,
                             &rcl2,
                             0, 0,                       // colors
                             flCmd2);

        // update char counters
        p += lDrawn;
        lTotalDrawn += lDrawn;

        // update x extents
        if (rcl2.xLeft < xLeftmost)
            xLeftmost = rcl2.xLeft;
        if (rcl2.xRight > xRightmost)
            xRightmost = rcl2.xRight;

        // update y for next line
        prcl->yTop -= ulCharHeight;

        // increase line count
        lLineCount++;
    }
    prcl->xLeft = xLeftmost;
    prcl->xRight = xRightmost;
    prcl->yBottom = prcl->yTop;
    prcl->yTop = lOrigYTop;

    return (lLineCount);
}

/*
 *@@ winhKillTasklist:
 *      this will destroy the Tasklist (window list) window.
 *      Note: you will only be able to get it back after a
 *      reboot, not a WPS restart. Only for use at shutdown and such.
 *      This trick by Uri J. Stern at
 *      http://zebra.asta.fh-weingarten.de/os2/Snippets/Howt8881.HTML
 */

VOID winhKillTasklist(VOID)
{
    SWBLOCK  swblock;
    HWND     hwndTasklist;
    // the tasklist has entry #0 in the SWBLOCK
    WinQuerySwitchList(NULLHANDLE, &swblock, sizeof(SWBLOCK));
    hwndTasklist = swblock.aswentry[0].swctl.hwnd;
    WinPostMsg(hwndTasklist,
               0x0454,     // undocumented msg for killing tasklist
               NULL, NULL);
}

// the following must be added for EMX (99-10-22) [umoeller]
#ifndef NERR_BufTooSmall
      #define NERR_BASE       2100
      #define NERR_BufTooSmall        (NERR_BASE+23)
            // the API return buffer is too small
#endif

/*
 *@@ winhQueryPendingSpoolJobs:
 *      returns the number of pending print jobs in the spooler
 *      or 0 if none. Useful for testing before shutdown.
 */

ULONG winhQueryPendingSpoolJobs(VOID)
{
    // BOOL    rcPending = FALSE;
    ULONG       ulTotalJobCount = 0;

    SPLERR      splerr;
    USHORT      jobCount;
    ULONG       cbBuf;
    ULONG       cTotal;
    ULONG       cReturned;
    ULONG       cbNeeded;
    ULONG       ulLevel;
    ULONG       i,j;
    PSZ         pszComputerName;
    PBYTE       pBuf = NULL;
    PPRQINFO3   prq;
    PPRJINFO2   prj2;

    ulLevel = 4L;
    pszComputerName = (PSZ)NULL;
    splerr = SplEnumQueue(pszComputerName, ulLevel, pBuf, 0L, // cbBuf
                          &cReturned, &cTotal,
                          &cbNeeded, NULL);
    if (    (splerr == ERROR_MORE_DATA)
         || (splerr == NERR_BufTooSmall)
       )
    {
        if (!DosAllocMem((PPVOID)&pBuf,
                         cbNeeded,
                         PAG_READ | PAG_WRITE | PAG_COMMIT))
        {
            cbBuf = cbNeeded;
            splerr = SplEnumQueue(pszComputerName, ulLevel, pBuf, cbBuf,
                                  &cReturned, &cTotal,
                                  &cbNeeded, NULL);
            if (splerr == NO_ERROR)
            {
                // set pointer to point to the beginning of the buffer
                prq = (PPRQINFO3)pBuf;

                // cReturned has the count of the number of PRQINFO3 structures
                for (i = 0;
                     i < cReturned;
                     i++)
                {
                    // save the count of jobs; there are this many PRJINFO2
                    // structures following the PRQINFO3 structure
                    jobCount = prq->cJobs;
                    // _Pmpf(( "Job count in this queue is %d",jobCount ));

                    // increment the pointer past the PRQINFO3 structure
                    prq++;

                    // set a pointer to point to the first PRJINFO2 structure
                    prj2=(PPRJINFO2)prq;
                    for (j = 0;
                         j < jobCount;
                         j++)
                    {
                        // increment the pointer to point to the next structure
                        prj2++;
                        // increase the job count, which we'll return
                        ulTotalJobCount++;

                    } // endfor jobCount

                    // after doing all the job structures, prj2 points to the next
                    // queue structure; set the pointer for a PRQINFO3 structure
                    prq = (PPRQINFO3)prj2;
                } //endfor cReturned
            } // endif NO_ERROR
            DosFreeMem(pBuf);
        }
    } // end if Q level given

    return (ulTotalJobCount);
}

/*
 *@@ winhSetNumLock:
 *      this sets the NumLock key on or off, depending
 *      on fState.
 *
 *      Based on code from WarpEnhancer, (C) Achim Hasenmller.
 *
 *@@added V0.9.1 (99-12-18) [umoeller]
 */

VOID winhSetNumLock(BOOL fState)
{
    // BOOL  fRestoreKBD = FALSE;  //  Assume we're not going to close Kbd
    BYTE KeyStateTable[256];
    ULONG ulActionTaken; //  Used by DosOpen
    HFILE hKbd;

    // read keyboard state table
    if (WinSetKeyboardStateTable(HWND_DESKTOP, &KeyStateTable[0],
                                 FALSE))
    {
        // first set the PM state
        if (fState)
            KeyStateTable[VK_NUMLOCK] |= 0x01; //  Turn numlock on
        else
            KeyStateTable[VK_NUMLOCK] &= 0xFE; //  Turn numlock off

        // set keyboard state table with new state values
        WinSetKeyboardStateTable(HWND_DESKTOP, &KeyStateTable[0], TRUE);
    }

    // now set the OS/2 keyboard state

    // try to open OS/2 keyboard driver
    if (!DosOpen("KBD$",
                 &hKbd, &ulActionTaken,
                 0,     // cbFile
                 FILE_NORMAL,
                 OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
                 OPEN_SHARE_DENYNONE | OPEN_ACCESS_READWRITE,
                 NULL))
    {
        SHIFTSTATE ShiftState;
        ULONG DataLen = sizeof(SHIFTSTATE);

        memset(&ShiftState, '\0', DataLen);
        DosDevIOCtl(hKbd, IOCTL_KEYBOARD, KBD_GETSHIFTSTATE,
                    NULL, 0L, NULL,
                    &ShiftState, DataLen, &DataLen);

        if (fState)
            ShiftState.fsState |= 0x0020; // turn NumLock on
        else
            ShiftState.fsState &= 0xFFDF; // turn NumLock off

        DosDevIOCtl(hKbd, IOCTL_KEYBOARD, KBD_SETSHIFTSTATE,
                    &ShiftState, DataLen, &DataLen,
                    NULL, 0L, NULL);
        // now close OS/2 keyboard driver
        DosClose(hKbd);
    }
    return;
}

/*
 *@@category: Helpers\PM helpers\Workplace Shell\WPS class list
 */

/* ******************************************************************
 *                                                                  *
 *   WPS Class List helpers                                         *
 *                                                                  *
 ********************************************************************/

/*
 *@@ winhQueryWPSClassList:
 *      this returns the WPS class list in a newly
 *      allocated buffer. This is just a shortcut to
 *      the usual double WinEnumObjectClasses call.
 *
 *      The return value is actually of the POBJCLASS type,
 *      so you better cast this manually. We declare this
 *      this as PBYTE though because POBJCLASS requires
 *      INCL_WINWORKPLACE.
 *      See WinEnumObjectClasses() for details.
 *
 *      The buffer is allocated using malloc(), so
 *      you should free() it when it is no longer
 *      needed.
 *
 *      This returns NULL if an error occured.
 *
 *@@added V0.9.0 [umoeller]
 */

PBYTE winhQueryWPSClassList(VOID)
{
    ULONG       ulSize;
    POBJCLASS   pObjClass = 0;

    // get WPS class list size
    if (WinEnumObjectClasses(NULL, &ulSize))
    {
        // allocate buffer
        pObjClass = (POBJCLASS)malloc(ulSize+1);
        // and load the classes into it
        WinEnumObjectClasses(pObjClass, &ulSize);
    }

    return ((PBYTE)pObjClass);
}

/*
 *@@ winhQueryWPSClass:
 *      this returns the POBJCLASS item if pszClass is registered
 *      with the WPS or NULL if the class could not be found.
 *
 *      The return value is actually of the POBJCLASS type,
 *      so you better cast this manually. We declare this
 *      this as PBYTE though because POBJCLASS requires
 *      INCL_WINWORKPLACE.
 *
 *      This takes as input the return value of winhQueryWPSClassList,
 *      which you must call first.
 *
 *      <B>Usage:</B>
 +          PBYTE   pClassList = winhQueryWPSClassList(),
 +                  pWPFolder;
 +          if (pClassList)
 +          {
 +              if (pWPFolder = winhQueryWPSClass(pClassList, "WPFolder"))
 +                  ...
 +              free(pClassList);
 +          }
 *
 *@@added V0.9.0 [umoeller]
 */

PBYTE winhQueryWPSClass(PBYTE pObjClass,  // in: buffer returned by
                                          // winhQueryWPSClassList
                        const char *pszClass)     // in: class name to query
{
    PBYTE   pbReturn = 0;

    POBJCLASS pocThis = (POBJCLASS)pObjClass;
    // now go thru the WPS class list
    while (pocThis)
    {
        if (strcmp(pocThis->pszClassName, pszClass) == 0)
        {
            pbReturn = (PBYTE)pocThis;
            break;
        }
        // next class
        pocThis = pocThis->pNext;
    } // end while (pocThis)

    return (pbReturn);
}

/*
 *@@ winhRegisterClass:
 *      this works just like WinRegisterObjectClass,
 *      except that it returns a more meaningful
 *      error code than just FALSE in case registering
 *      fails.
 *
 *      This returns NO_ERROR if the class was successfully
 *      registered (WinRegisterObjectClass returned TRUE).
 *
 *      Otherwise, we do a DosLoadModule if maybe the DLL
 *      couldn't be loaded in the first place. If DosLoadModule
 *      did not return NO_ERROR, this function returns that
 *      return code, which can be:
 *
 *      --  2   ERROR_FILE_NOT_FOUND: pcszModule does not exist
 *      --  2   ERROR_FILE_NOT_FOUND
 *      --  3   ERROR_PATH_NOT_FOUND
 *      --  4   ERROR_TOO_MANY_OPEN_FILES
 *      --  5   ERROR_ACCESS_DENIED
 *      --  8   ERROR_NOT_ENOUGH_MEMORY
 *      --  11  ERROR_BAD_FORMAT
 *      --  26  ERROR_NOT_DOS_DISK (unknown media type)
 *      --  32  ERROR_SHARING_VIOLATION
 *      --  33  ERROR_LOCK_VIOLATION
 *      --  36  ERROR_SHARING_BUFFER_EXCEEDED
 *      --  95  ERROR_INTERRUPT (interrupted system call)
 *      --  108 ERROR_DRIVE_LOCKED (by another process)
 *      --  123 ERROR_INVALID_NAME (illegal character or FS name not valid)
 *      --  127 ERROR_PROC_NOT_FOUND (DosQueryProcAddr error)
 *      --  180 ERROR_INVALID_SEGMENT_NUMBER
 *      --  182 ERROR_INVALID_ORDINAL
 *      --  190 ERROR_INVALID_MODULETYPE (probably an application)
 *      --  191 ERROR_INVALID_EXE_SIGNATURE (probably not LX DLL)
 *      --  192 ERROR_EXE_MARKED_INVALID (by linker)
 *      --  194 ERROR_ITERATED_DATA_EXCEEDS_64K (in a DLL segment)
 *      --  195 ERROR_INVALID_MINALLOCSIZE
 *      --  196 ERROR_DYNLINK_FROM_INVALID_RING
 *      --  198 ERROR_INVALID_SEGDPL
 *      --  199 ERROR_AUTODATASEG_EXCEEDS_64K
 *      --  201 ERROR_RELOCSRC_CHAIN_EXCEEDS_SEGLIMIT
 *      --  206 ERROR_FILENAME_EXCED_RANGE (not matching 8+3 spec)
 *      --  295 ERROR_INIT_ROUTINE_FAILED (DLL init routine failed)
 *
 *      In all these cases, pszBuf may contain a meaningful
 *      error message from DosLoadModule, especially if an import
 *      could not be resolved.
 *
 *      Still worse, if DosLoadModule returned NO_ERROR, we
 *      probably have some SOM internal error. A probable
 *      reason is that the parent class of pcszClassName
 *      is not installed, but that's WPS/SOM internal
 *      and cannot be queried from outside the WPS context.
 *
 *      In that case, ERROR_OPEN_FAILED (110) is returned.
 *      That one sounded good to me. ;-)
 */

APIRET winhRegisterClass(const char* pcszClassName, // in: e.g. "XFolder"
                         const char* pcszModule,    // in: e.g. "C:\XFOLDER\XFLDR.DLL"
                         PSZ pszBuf,                // out: error message from DosLoadModule
                         ULONG cbBuf)               // in: sizeof(*pszBuf), passed to DosLoadModule
{
    APIRET arc = NO_ERROR;

    if (!WinRegisterObjectClass((PSZ)pcszClassName, (PSZ)pcszModule))
    {
        // failed: do more error checking then, try DosLoadModule
        HMODULE hmod = NULLHANDLE;
        arc = DosLoadModule(pszBuf, cbBuf,
                            (PSZ)pcszModule,
                            &hmod);
        if (arc == NO_ERROR)
        {
            // DosLoadModule succeeded:
            // some SOM error then
            DosFreeModule(hmod);
            arc = ERROR_OPEN_FAILED;
        }
    }
    // else: ulrc still 0 (== no error)

    return (arc);
}

/*
 *@@ winhIsClassRegistered:
 *      quick one-shot function which checks if
 *      a class is currently registered. Calls
 *      winhQueryWPSClassList and winhQueryWPSClass
 *      in turn.
 *
 *@@added V0.9.2 (2000-02-26) [umoeller]
 */

BOOL winhIsClassRegistered(const char *pcszClass)
{
    BOOL    brc = FALSE;
    PBYTE   pClassList = winhQueryWPSClassList();
    if (pClassList)
    {
        if (winhQueryWPSClass(pClassList, pcszClass))
            brc = TRUE;
        free(pClassList);
    }

    return (brc);
}

/*
 *@@category: Helpers\PM helpers\Workplace Shell
 */

/*
 *@@ winhResetWPS:
 *      restarts the WPS using PrfReset. Returns
 *      one of the following:
 *
 *      -- 0: no error.
 *      -- 1: PrfReset failed.
 *      -- 2 or 4: PrfQueryProfile failed.
 *      -- 3: malloc() failed.
 *
 *@@added V0.9.4 (2000-07-01) [umoeller]
 */

ULONG winhResetWPS(HAB hab)
{
    ULONG ulrc = 0;
    // find out current profile names
    PRFPROFILE Profiles;
    Profiles.cchUserName = Profiles.cchSysName = 0;
    // first query their file name lengths
    if (PrfQueryProfile(hab, &Profiles))
    {
        // allocate memory for filenames
        Profiles.pszUserName  = (PSZ)malloc(Profiles.cchUserName);
        Profiles.pszSysName  = (PSZ)malloc(Profiles.cchSysName);

        if (Profiles.pszSysName)
        {
            // get filenames
            if (PrfQueryProfile(hab, &Profiles))
            {

                // "change" INIs to these filenames:
                // THIS WILL RESET THE WPS
                if (PrfReset(hab, &Profiles) == FALSE)
                    ulrc = 1;
                free(Profiles.pszSysName);
                free(Profiles.pszUserName);
            }
            else
                ulrc = 2;
        }
        else
            ulrc = 3;
    }
    else
        ulrc = 4;

    return (ulrc);
}
