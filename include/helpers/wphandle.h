
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
 *      This file is part of the "XWorkplace helpers" source package.
 *      This is free software; you can redistribute it and/or modify
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

    /* ******************************************************************
     *
     *   Errors
     *
     ********************************************************************/

    #define ERROR_WPH_FIRST                         41000
    #define ERROR_WPH_CRASHED                       (ERROR_WPH_FIRST +   0)
    #define ERROR_WPH_NO_BASECLASS_DATA             (ERROR_WPH_FIRST +   1)
    #define ERROR_WPH_NO_ACTIVEHANDLES_DATA         (ERROR_WPH_FIRST +   2)
    #define ERROR_WPH_INCOMPLETE_BASECLASS_DATA     (ERROR_WPH_FIRST +   3)
    #define ERROR_WPH_NO_HANDLES_DATA               (ERROR_WPH_FIRST +   4)
    #define ERROR_WPH_CORRUPT_HANDLES_DATA          (ERROR_WPH_FIRST +   5)
    #define ERROR_WPH_INVALID_PARENT_HANDLE         (ERROR_WPH_FIRST +   6)

    /* ******************************************************************
     *
     *   Strings
     *
     ********************************************************************/

    // DECLARE_WPH_STRING is a handy macro which saves us from
    // keeping two string lists in both the .h and the .c file.
    // If this include file is included from the .c file, the
    // string is defined as a global variable. Otherwise
    // it is only declared as "extern" so other files can
    // see it.

    #ifdef INCLUDE_WPHANDLE_PRIVATE
        #define DECLARE_WPH_STRING(str, def) const char *str = def
    #else
        #define DECLARE_WPH_STRING(str, def) extern const char *str;
    #endif

    /*
     * OS2.INI applications
     *
     */

    // abstract objects per folder handle
    DECLARE_WPH_STRING(WPINIAPP_FDRCONTENT, "PM_Abstract:FldrContent");
    // all defined abstract objects on the system
    DECLARE_WPH_STRING(WPINIAPP_OBJECTS, "PM_Abstract:Objects");
    // their icons, if set individually
    DECLARE_WPH_STRING(WPINIAPP_ICONS, "PM_Abstract:Icons");

    // object ID's (<WP_DESKTOP> etc.)
    DECLARE_WPH_STRING(WPINIAPP_LOCATION, "PM_Workplace:Location");

    // folder positions
    DECLARE_WPH_STRING(WPINIAPP_FOLDERPOS, "PM_Workplace:FolderPos");

    // palette positions
    DECLARE_WPH_STRING(WPINIAPP_PALETTEPOS, "PM_Workplace:PalettePos");
    // ???
    DECLARE_WPH_STRING(WPINIAPP_STATUSPOS, "PM_Workplace:StatusPos");
    // startup folders
    DECLARE_WPH_STRING(WPINIAPP_STARTUP, "PM_Workplace:Startup");
    // all the defined templates on the system
    DECLARE_WPH_STRING(WPINIAPP_TEMPLATES, "PM_Workplace:Templates");

    // all work area folders
    DECLARE_WPH_STRING(WPINIAPP_WORKAREARUNNING, "FolderWorkareaRunningObjects");
    // spooler windows ?!?
    DECLARE_WPH_STRING(WPINIAPP_JOBCNRPOS, "PM_PrintObject:JobCnrPos");

    // associations by type ("Plain Text")
    DECLARE_WPH_STRING(WPINIAPP_ASSOCTYPE, "PMWP_ASSOC_TYPE");
    // associations by filter ("*.txt")
    DECLARE_WPH_STRING(WPINIAPP_ASSOCFILTER, "PMWP_ASSOC_FILTER");
    // checksums ?!?
    DECLARE_WPH_STRING(WPINIAPP_ASSOC_CHECKSUM, "PMWP_ASSOC_CHECKSUM");

    /*
     * OS2SYS.INI applications
     *
     */

    DECLARE_WPH_STRING(WPINIAPP_ACTIVEHANDLES, "PM_Workplace:ActiveHandles");
    DECLARE_WPH_STRING(WPINIAPP_HANDLES, "PM_Workplace:Handles");
    DECLARE_WPH_STRING(WPINIAPP_HANDLESAPP, "HandlesAppName");

    /*
     * some default WPS INI keys:
     *
     */

    DECLARE_WPH_STRING(WPOBJID_DESKTOP, "<WP_DESKTOP>");

    DECLARE_WPH_STRING(WPOBJID_KEYB, "<WP_KEYB>");
    DECLARE_WPH_STRING(WPOBJID_MOUSE, "<WP_MOUSE>");
    DECLARE_WPH_STRING(WPOBJID_CNTRY, "<WP_CNTRY>");
    DECLARE_WPH_STRING(WPOBJID_SOUND, "<WP_SOUND>");
    DECLARE_WPH_STRING(WPOBJID_SYSTEM, "<WP_SYSTEM>"); // V0.9.9
    DECLARE_WPH_STRING(WPOBJID_POWER, "<WP_POWER>");
    DECLARE_WPH_STRING(WPOBJID_WINCFG, "<WP_WINCFG>");

    DECLARE_WPH_STRING(WPOBJID_HIRESCLRPAL, "<WP_HIRESCLRPAL>");
    DECLARE_WPH_STRING(WPOBJID_LORESCLRPAL, "<WP_LORESCLRPAL>");
    DECLARE_WPH_STRING(WPOBJID_FNTPAL, "<WP_FNTPAL>");
    DECLARE_WPH_STRING(WPOBJID_SCHPAL96, "<WP_SCHPAL96>");

    DECLARE_WPH_STRING(WPOBJID_LAUNCHPAD, "<WP_LAUNCHPAD>");
    DECLARE_WPH_STRING(WPOBJID_WARPCENTER, "<WP_WARPCENTER>");

    DECLARE_WPH_STRING(WPOBJID_SPOOL, "<WP_SPOOL>");
    DECLARE_WPH_STRING(WPOBJID_VIEWER, "<WP_VIEWER>");
    DECLARE_WPH_STRING(WPOBJID_SHRED, "<WP_SHRED>");
    DECLARE_WPH_STRING(WPOBJID_CLOCK, "<WP_CLOCK>");

    DECLARE_WPH_STRING(WPOBJID_START, "<WP_START>");
    DECLARE_WPH_STRING(WPOBJID_TEMPS, "<WP_TEMPS>");
    DECLARE_WPH_STRING(WPOBJID_DRIVES, "<WP_DRIVES>");

    /* ******************************************************************
     *
     *   Definitions
     *
     ********************************************************************/

    #pragma pack(1)

    /*
     *@@ NODE:
     *      file or directory node in the BLOCKS
     *      in OS2SYS.INI. See wphandle.c.
     *
     *@@added V0.9.16 (2001-10-02) [umoeller]
     */

    typedef struct _NODE
    {
        CHAR    achName[4];         // = 'NODE'
        USHORT  usUsage;            // always == 1
        USHORT  usHandle;           // object handle of this NODE
        USHORT  usParentHandle;     // object handle of parent NODE
        BYTE    achFiller[20];      // filler
        USHORT  usNameSize;         // size of non-qualified filename
        CHAR    szName[1];          // variable length: non-qualified filename
                                    // (zero-terminated)
    } NODE, *PNODE;

    /*
     *@@ DRIVE:
     *      drive node in the BLOCKS
     *      in OS2SYS.INI. See wphandle.c.
     *
     *@@added V0.9.16 (2001-10-02) [umoeller]
     */

    typedef struct _DRIVE
    {
        CHAR    achName[4];  // = 'DRIV'
        USHORT  usUnknown1[4];
        ULONG   ulSerialNr;
        USHORT  usUnknown2[2];
        CHAR    szName[1];
    } DRIV, *PDRIV;

    #pragma pack()

    /*
     *@@ WPHANDLESBUF:
     *      structure created by wphLoadHandles.
     *
     *@@added V0.9.16 (2001-10-02) [umoeller]
     */

    typedef struct _WPHANDLESBUF
    {
        PBYTE       pbData;         // ptr to all handles (buffers from OS2SYS.INI)
        ULONG       cbData;         // byte count of *p

        USHORT      usHiwordAbstract,   // hiword for WPAbstract handles
                    usHiwordFileSystem; // hiword for WPFileSystem handles

        PNODE       NodeHashTable[65536];
        BOOL        fNodeHashTableValid;    // TRUE after wphRebuildNodeHashTable

    } HANDLESBUF, *PHANDLESBUF;

    /* ******************************************************************
     *
     *   Load handles functions
     *
     ********************************************************************/

    APIRET wphQueryActiveHandles(HINI hiniSystem,
                                 PSZ *ppszActiveHandles);

    APIRET wphQueryBaseClassesHiwords(HINI hiniUser,
                                      PUSHORT pusHiwordAbstract,
                                      PUSHORT pusHiwordFileSystem);

    APIRET wphRebuildNodeHashTable(PHANDLESBUF pHandlesBuf);

    APIRET wphLoadHandles(HINI hiniUser,
                          HINI hiniSystem,
                          const char *pcszActiveHandles,
                          PHANDLESBUF *ppHandlesBuf);

    APIRET wphFreeHandles(PHANDLESBUF *ppHandlesBuf);

    APIRET wphQueryHandleFromPath(HINI hiniUser,
                                  HINI hiniSystem,
                                  const char *pcszName,
                                  HOBJECT *phobj);

    APIRET wphComposePath(PHANDLESBUF pHandlesBuf,
                          USHORT usHandle,
                          PSZ pszFilename,
                          ULONG cbFilename,
                          PNODE *ppNode);

    APIRET wphQueryPathFromHandle(HINI hiniUser,
                                  HINI hiniSystem,
                                  HOBJECT hObject,
                                  PSZ pszFilename,
                                  ULONG cbFilename);

#endif

#if __cplusplus
}
#endif

