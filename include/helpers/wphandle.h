
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

