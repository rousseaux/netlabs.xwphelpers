
/*
 *@@sourcefile undoc.h:
 *      various OS/2 declarations which are either undocumented
 *      or only in the Warp 4 toolkit headers.
 *      This file does not correspond to any .C code file.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@include "undoc.h" // Note: Always include this last!!!
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
 */

#if __cplusplus
extern "C" {
#endif

#ifndef UNDOC_HEADER_INCLUDED
    #define UNDOC_HEADER_INCLUDED

    /*
     * HK_PREACCEL:
     *      additional undocumented PM hook type,
     *      for pre-accelerator table hooks
     *      (see xwphook.c for details);
     *      this definition taken out of the
     *      ProgramCommander/2 source (thanks,
     *      Roman Stangl).
     */

    #define HK_PREACCEL             17

    /*
     * Notebook button style (Warp 4 only):
     *
     */

    #ifndef BS_NOTEBOOKBUTTON
        #define BS_NOTEBOOKBUTTON          8L
    #endif

    /*
     * Original WPS sort criteria flags;:
     *      these appear to be the column indices into
     *      folder Details views.
     */

    #define WPSORTKEY_REALNAME      0x00000005
    #define WPSORTKEY_SIZE          0x00000006
    #define WPSORTKEY_WRITEDATE     0x00000007
    #define WPSORTKEY_ACCESSDATE    0x00000009
    #define WPSORTKEY_CREATIONDATE  0x0000000B
    // except for the following two, of which I have
    // no freaking idea how the WPS implements these;
    // probably some really ugly internal kludge to
    // which I have no access
    #define WPSORTKEY_NAME          0xFFFFFFFE
    #define WPSORTKEY_TYPE          0xFFFFFFFF

    /*
     *  WPS object styles
     *      V0.9.7 (2000-12-10) [umoeller]
     */

    #ifndef OBJSTYLE_LOCKEDINPLACE
        #define OBJSTYLE_LOCKEDINPLACE  0x00020000
    #endif

    /*
     * Some more OS/2 default menu items:
     *
     */

    #define ID_WPMI_PASTE                  0x2CB
    #define ID_WPMI_SHOWICONVIEW           0x2CC
    #define ID_WPMI_SHOWTREEVIEW           0x2CD
    #define ID_WPMI_SHOWDETAILSVIEW        0x2CE

    #define ID_WPMI_SORTBYNAME             0x1770
    #define ID_WPMI_SORTBYEXTENSION        0x9D87
    #define ID_WPMI_SORTBYTYPE             0x1771
    #define ID_WPMI_SORTBYREALNAME         0x1777
    #define ID_WPMI_SORTBYSIZE             0x1778
    #define ID_WPMI_SORTBYWRITEDATE        0x1779
    #define ID_WPMI_SORTBYACCESSDATE       0x177B
    #define ID_WPMI_SORTBYCREATIONDATE     0x177D

    #define ID_WPMI_ARRANGEFROMTOP         0x2DE
    #define ID_WPMI_ARRANGEFROMLEFT        0x2DF
    #define ID_WPMI_ARRANGEFROMRIGHT       0x2E0
    #define ID_WPMI_ARRANGEFROMBOTTOM      0x2E1
    #define ID_WPMI_ARRANGEPERIMETER       0x2E3
    #define ID_WPMI_ARRANGEHORIZONTALLY    0x2E4
    #define ID_WPMI_ARRANGEVERTICALLY      0x2E5

    #define ID_WPM_LOCKINPLACE             0x2DA
    #define ID_WPM_VIEW                    0x68

    #define ID_WPMI_FORMATDISK             0x7C
    #define ID_WPMI_CHECKDISK              0x80
    #define ID_WPMI_REFRESH                0x1F7

    /*
     * Return codes for wpConfirmObjectTitle:
     *      only def'd in the Warp 4 Toolkit
     *      (and partly in wpsystem.h).
     */

    #ifndef NAMECLASH_CANCEL
        #define NAMECLASH_CANCEL    0
    #endif
    #ifndef NAMECLASH_NONE
        #define NAMECLASH_NONE      1
    #endif
    #ifndef NAMECLASH_RENAME
        #define NAMECLASH_RENAME    2
    #endif
    #ifndef NAMECLASH_REPLACE
        #define NAMECLASH_REPLACE   8
    #endif
#endif

#if __cplusplus
}
#endif

