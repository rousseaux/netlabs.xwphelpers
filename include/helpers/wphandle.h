/* $Id$ */


/*
 *@@sourcefile wphandle.h:
 *      header file for wphandle.c, which contains the logic for
 *      dealing with those annoying WPS object handles in OS2SYS.INI.
 *
 *      This code is mostly written by Henk Kelder and published
 *      with his kind permission.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@include #define INCL_WINSHELLDATA
 *@@include #define INCL_WINWORKPLACE
 *@@include #include <os2.h>
 *@@include #include "wphandle.h"
 */

/*      This file Copyright (C) 1997-2000 Ulrich M”ller,
 *                                        Henk Kelder.
 *      This file is part of the XWorkplace source package.
 *      XWorkplace is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation, in version 2 as it comes in the COPYING
 *      file of the XWorkplace main distribution.
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 */

#if __cplusplus
extern "C" {
#endif

#ifndef WPHANDLE_HEADER_INCLUDED
    #define WPHANDLE_HEADER_INCLUDED

    /*
     *@@ IsObjectAbstract:
     *      this macro returns TRUE if the passed hObject points
     *      to an abstract object; the WPS sets the HIWORD of
     *      an abstract hObject to 2 or 3 for these, to 1 for
     *      file system objects
     */

    #define IsObjectAbstract(h)     (BOOL)((HIUSHORT(h)) != 3)

    /*
     *  Two structures needed for finding
     *  a filename based on a object handle
     *  (OS2SYS.INI). These are only needed
     *  to parse the BLOCK data manually.
     */

    #pragma pack(1)

    typedef struct _NodeDev
    {
        BYTE   chName[4];       // = 'NODE'
        USHORT usLevel;         // always == 1
        USHORT usHandle;        // object handle of this NODE
        USHORT usParentHandle;  // object handle of parent NODE
        BYTE   ch[20];
        USHORT usNameSize;      // size of non-qualified filename
        BYTE   szName[1];       // variable length: non-qualified filename
                                // (zero-terminated)
    } NODE, *PNODE;

    typedef struct _DrivDev
    {
        BYTE  chName[4];  // = 'DRIV'
        USHORT usUnknown1[4];
            // or BYTE  ch1[8], if you prefer
        ULONG ulSerialNr;
        USHORT usUnknown2[2];
            // or BYTE  ch2[4], if you prefer
        BYTE  szName[1];
    } DRIV, *PDRIV;

    /*
     * prototypes
     *
     */

    BOOL wphQueryActiveHandles(HINI hIniSystem,
                               PSZ pszHandlesAppName,
                               USHORT usMax);

    BOOL wphReadAllBlocks(HINI hiniSystem,
                          PSZ pszActiveHandles,
                          PBYTE* ppBlock,
                          PULONG pulSize);

    HOBJECT wphQueryHandleFromPath(HINI hIniUser,
                                   HINI hIniSystem,
                                   PSZ pszName);

    PNODE wphFindPartName(PBYTE pHandlesBuffer,
                          ULONG ulBufSize,
                          USHORT usHandle,
                          PSZ pszFname,
                          USHORT usMax);

    BOOL wphQueryPathFromHandle(HINI hIniSystem,
                                HOBJECT hObject,
                                PSZ pszFname,
                                USHORT usMax);
    /*
     * OS2.INI applications
     *
     */

    // abstract objects per folder handle
    #define FOLDERCONTENT  "PM_Abstract:FldrContent"
    // all defined abstract objects on the system
    #define OBJECTS        "PM_Abstract:Objects"
    // their icons, if set individually
    #define ICONS          "PM_Abstract:Icons"

    // folder positions
    #define FOLDERPOS      "PM_Workplace:FolderPos"

    // object ID's (<WP_DESKTOP> etc.)
    #define LOCATION       "PM_Workplace:Location"
    // palette positions
    #define PALETTEPOS     "PM_Workplace:PalettePos"
    // ???
    #define STATUSPOS      "PM_Workplace:StatusPos"
    // startup folders
    #define STARTUP        "PM_Workplace:Startup"
    // all the defined templates on the system
    #define TEMPLATES      "PM_Workplace:Templates"
    // associations by filter ("*.txt")
    #define ASSOC_FILTER   "PMWP_ASSOC_FILTER"
    // associations by type ("Plain Text")
    #define ASSOC_TYPE     "PMWP_ASSOC_TYPE"
    // checksums ?!?
    #define ASSOC_CHECKSUM "PMWP_ASSOC_CHECKSUM"
    // all work area folders
    #define WORKAREARUNNING "FolderWorkareaRunningObjects"
    // spooler windows ?!?
    #define JOBCNRPOS       "PM_PrintObject:JobCnrPos"

    /*
     * OS2SYS.INI applications
     *
     */

    #define HANDLEBLOCK    "BLOCK1"
    #define ACTIVEHANDLES  "PM_Workplace:ActiveHandles"
    #define HANDLES        "PM_Workplace:Handles"
    #define HANDLESAPP     "HandlesAppName"

    /*
     * Extended Attrribute types
     *
     */

    #ifdef FORGET_IT
        #define EAT_BINARY      0xFFFE      // length preceeded binary
        #define EAT_ASCII       0xFFFD      // length preceeded ASCII
        #define EAT_BITMAP      0xFFFB      // length preceeded bitmap
        #define EAT_METAFILE    0xFFFA      // length preceeded metafile
        #define EAT_ICON        0xFFF9      // length preceeded icon
        #define EAT_EA          0xFFEE      // length preceeded ASCII
                                            // name of associated data (#include)
        #define EAT_MVMT        0xFFDF      // multi-valued, multi-typed field
        #define EAT_MVST        0xFFDE      // multi-valued, single-typed field
        #define EAT_ASN1        0xFFDD      // ASN.1 field
    #endif

    /*
     * Several defines to read EA's
     *
     */

    #define EA_LPBINARY      EAT_BINARY      // Length preceeded binary
    #define EA_LPASCII       EAT_ASCII       // Length preceeded ascii
    #define EA_ASCIIZ        0xFFFC          // Asciiz
    #define EA_LPBITMAP      EAT_BITMAP      // Length preceeded bitmap
    #define EA_LPMETAFILE    EAT_METAFILE    // metafile
    #define EA_LPICON        EAT_ICON        // Length preceeded icon
    #define EA_ASCIIZFN      0xFFEF          // Asciiz file name of associated dat
    #define EA_ASCIIZEA      EAT_EA          // Name of associated data, LP Ascii
    #define EA_MVMT          EAT_MVMT        // Multi value, multi type
    #define EA_MVST          EAT_MVST        // Multi value, single type
    #define EA_ASN1          EAT_ASN1        // ASN.1 Field

    /*
     * The high word of a HOBJECT determines its type:
     *
     */

    // Abstract objects _always_ have an object handle,
    // because they cannot be referenced by file name.
    // The high word is either 1 or 2.
    #define OBJECT_ABSTRACT     0x0001       // before Warp 3, I guess (UM)
    #define OBJECT_ABSTRACT2    0x0002
    // File-system objects do not necessarily have an
    // object handle. If they do, these handles are stored
    // in those ugly PM_Workplace:Handles BLOCK's in
    // OS2SYS.INI.
    // The high word is always 3.
    #define OBJECT_FSYS         0x0003

    /*
     * Several datatags:
     *      all the following are used by the wpRestoreData/
     *      wpSaveData methods to specify the type of data to
     *      be worked on (ulKey parameter). This is specific
     *      to each class.
     *
     *      Object instance data is stored in OS2.INI for
     *      abstract, in file EA's for file-system objects
     *      (.CLASSDATA).
     *
     *      Many of these are also declared in the WPS header
     *      files (IDKEY_* parameters; see wpfolder.h for
     *      examples).
     */

    // WPObject
    #define WPOBJECT_HELPPANEL   2
    #define WPOBJECT_SZID        6
    #define WPOBJECT_STYLE       7
    #define WPOBJECT_MINWIN      8
    #define WPOBJECT_CONCURRENT  9
    #define WPOBJECT_VIEWBUTTON 10
    #define WPOBJECT_DATA       11
    #define WPOBJECT_STRINGS    12
    #define WPOBJ_STR_OBJID      1

    // WPAbstract
    #define WPABSTRACT_TITLE     1
    #define WPABSTRACT_STYLE     2      // appears to contain the same as WPOBJECT 7

    // WPShadow
    #define WPSHADOW_LINK      104       // target object

    // WPProgram
    #define WPPROGRAM_PROGTYPE   1
    #define WPPROGRAM_EXEHANDLE  2       // object handle of executable
    #define WPPROGRAM_PARAMS     3
    #define WPPROGRAM_DIRHANDLE  4       // object handle of startup dir
    #define WPPROGRAM_DOSSET     6
    #define WPPROGRAM_STYLE      7
    #define WPPROGRAM_EXENAME    9
    #define WPPROGRAM_DATA      11
    #define WPPROGRAM_STRINGS   10
    #define WPPGM_STR_EXENAME    0
    #define WPPGM_STR_ARGS       1

    // WPFileSystem
    #define WPFSYS_MENUCOUNT     4
    #define WPFSYS_MENUARRAY     3

    // WPFolder (also see the additional def's in wpfolder.h)
    #define WPFOLDER_FOLDERFLAG  13
    #define WPFOLDER_ICONVIEW  2900     /* IDKEY_FDRCONTENTATTR */
    #define WPFOLDER_DATA      2931     /* IDKEY_FDRLONGARRAY */
    #define WPFOLDER_FONTS     2932     /* IDKEY_FDRSTRARRAY */

    // WPProgramFile
    #define WPPROGFILE_PROGTYPE  1
    #define WPPROGFILE_DOSSET    5
    #define WPPROGFILE_STYLE     6
    #define WPPROGFILE_DATA     10
    #define WPPROGFILE_STRINGS  11
    #define WPPRGFIL_STR_ARGS    0

    // printer queues?!?
    #define IDKEY_PRNQUEUENAME 3
    #define IDKEY_PRNCOMPUTER  5
    #define IDKEY_PRNJOBDIALOG 9
    #define IDKEY_PRNREMQUEUE 13

    #define IDKEY_RPRNNETID    1

    // WPDisk?!?
    #define IDKEY_DRIVENUM     1

    /*
     *  Two structures needed for reading
     *  WPAbstract's object information.
     *
     *  These are only needed if the instance
     *  data in OS2.INI is to be parsed manually.
     */

    typedef struct _ObjectInfo
    {
        USHORT cbName;       // Size of szName
        USHORT cbData;       // Size of variable length data, starting after szName
        BYTE   szName[1];    // The name of the datagroup
    } OINFO, *POINFO;

    typedef struct _TagInfo
    {
        USHORT usTagFormat;  // Format of data
        USHORT usTag;        // The data-identifier
        USHORT cbTag;        // The size of the data
    } TAG, *PTAG;

    typedef struct _WPProgramRefData
    {
        HOBJECT hExeHandle;
        HOBJECT hCurDirHandle;
        ULONG   ulFiller1;
        ULONG   ulProgType;
        BYTE    bFiller[12];
    } WPPGMDATA;

    typedef struct _WPProgramFileData
    {
        HOBJECT hCurDirHandle;
        ULONG   ulProgType;
        ULONG   ulFiller1;
        ULONG   ulFiller2;
        ULONG   ulFiller3;
    } WPPGMFILEDATA;

    typedef struct _WPObjectData
    {
        LONG  lDefaultView;
        ULONG ulHelpPanel;
        ULONG ulUnknown3;
        ULONG ulObjectStyle;
        ULONG ulMinWin;
        ULONG ulConcurrent;
        ULONG ulViewButton;
        ULONG ulMenuFlag;
    } WPOBJDATA;

    typedef struct _WPFolderData
    {
        ULONG ulIconView;
        ULONG ulTreeView;
        ULONG ulDetailsView;
        ULONG ulFolderFlag;
        ULONG ulTreeStyle;
        ULONG ulDetailsStyle;
        BYTE  rgbIconTextBkgnd[4];
        BYTE  Filler1[4];
        BYTE  rgbIconTextColor[4];
        BYTE  Filler2[8];
        BYTE  rgbTreeTextColor[4];
        BYTE  rgbDetailsTextColor[4];
        BYTE  Filler3[4];
        USHORT fIconTextVisible;
        USHORT fIconViewFlags;
        USHORT fTreeTextVisible;
        USHORT fTreeViewFlags;
        BYTE  Filler4[4];
        ULONG ulMenuFlag;
        BYTE  rgbIconShadowColor[4];
        BYTE  rgbTreeShadowColor[4];
        BYTE  rgbDetailsShadowColor[4];
    } WPFOLDATA;

    typedef struct _FsysMenu
    {
        USHORT  usIDMenu   ;
        USHORT  usIDParent ;
        USHORT  usMenuType; //  1 = Cascade, 2 = condcascade, 3 = choice
        HOBJECT hObject;
        BYTE    szTitle[32];
    } FSYSMENU, *PFSYSMENU;

    typedef struct _FolderSort
    {
        LONG lDefaultSortIndex;
        BOOL fAlwaysSort;
        LONG lCurrentSort;
        BYTE bFiller[12];
    } FOLDERSORT, *PFOLDERSORT;

    typedef struct _FldBkgnd
    {
        ULONG  ulUnknown;
        BYTE   bBlue;
        BYTE   bGreen;
        BYTE   bRed;
        BYTE   bExtra;
        //RGB    rgb;
        //BYTE   bFiller;
        BYTE   bColorOnly; // 0x28 Image, 0x27 Color only
        BYTE   bFiller2;
        BYTE   bImageType; // 2=Normal, 3=tiled, 4=scaled
        BYTE   bFiller3;
        BYTE   bScaleFactor;
        BYTE   bFiller4;
    } FLDBKGND, *PFLDBKGND;

    #pragma pack()              // added V0.9.0

#endif

#if __cplusplus
}
#endif

