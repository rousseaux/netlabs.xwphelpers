
/*
 *@@sourcefile wphandle.c:
 *      this file contains the logic for dealing with
 *      those annoying WPS object handles in OS2SYS.INI.
 *      This does not use WPS interfaces, but parses
 *      the profiles directly.
 *
 *      Usage: All OS/2 programs.
 *
 *      Function prefixes (new with V0.81):
 *      --  wph*   WPS object helper functions
 *
 *      This code is mostly written by Henk Kelder and published
 *      with his kind permission.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@header "helpers\wphandle.h"
 */

/*
 *      This file Copyright (C) 1997-2000 Ulrich M”ller,
 *                                        Henk Kelder.
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

#define INCL_DOS
#define INCL_WINSHELLDATA
#include <os2.h>

#define OPTIONS_SIZE 32767

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <io.h>

#include "setup.h"                      // code generation and debugging options

#include "helpers\wphandle.h"

/****************************************************
 *                                                  *
 *  helper functions                                *
 *                                                  *
 ****************************************************/

static USHORT wphSearchBufferForHandle(PBYTE pHandlesBuffer, ULONG ulBufSize, USHORT usParent, PSZ pszFname);
PNODE wphFindPartName(PBYTE pHandlesBuffer, ULONG ulBufSize, USHORT usID, PSZ pszFname, USHORT usMax);

#define MakeDiskHandle(usObjID) (usObjID | 0x30000)

#define IsObjectDisk(hObject) ((hObject & 0x30000) == 0x30000)

/*
 *@@category: Helpers\PM helpers\Workplace Shell\Handles (OS2SYS.INI)
 */

/* ******************************************************************
 *                                                                  *
 *   Helper functions                                               *
 *                                                                  *
 ********************************************************************/

/*
 *@@ wphQueryProfileData:
 *      like PrfQueryProfileData, but allocates sufficient
 *      memory and returns a pointer to that buffer.
 *      pulSize must point to a ULONG which will then
 *      contain the no. of copied bytes.
 */

PBYTE wphQueryProfileData(HINI hIniSystem,    // in: can be HINI_USER or HINI_SYSTEM
                          PSZ pApp, PSZ pKey, // in: load what?
                          PULONG pulSize)     // out: bytes loaded
{
    PBYTE pData = NULL;
    if (PrfQueryProfileSize(hIniSystem, pApp, pKey, pulSize))
    {
        pData = (PBYTE)malloc(*pulSize);
        PrfQueryProfileData(hIniSystem, pApp, pKey, pData, pulSize);
    }
    // _Pmpf(("  wphQueryProfileData(%lX, %s, %s, %d)", hIniSystem, pApp, pKey, *pulSize));
    return (pData);
}

/*
 *@@ wphEnumProfileKeys:
 *      allocates memory for a buffer and copies the keys
 *      for pApp into it.
 *      Returns the pointer to the buffer.
 *      pulKeysSize must point to a ULONG which will then
 *      contain the size of the returned buffer.
 */

PBYTE wphEnumProfileKeys(HINI hIniSystem,       // in: can be HINI_USER or HINI_SYSTEM
                         PSZ pApp,              // in: app to query
                         PULONG pulKeysSize)    // out: sizeof(return buffer)
{
    PBYTE pszKeys = NULL;
    if (PrfQueryProfileSize(hIniSystem, pApp, NULL, pulKeysSize))
    {
        pszKeys = (PBYTE)malloc(*pulKeysSize);
        if (pszKeys)
            PrfQueryProfileData(hIniSystem, pApp, NULL, pszKeys, pulKeysSize);
    }
    return (pszKeys);
}

/*
 * wphResetBlockBuffer:
 *      Reset the block buffer, make sure the buffer is re-read.
 */

/* VOID wphResetBlockBuffer(VOID)
{
   if (pHandlesBuffer)
   {
       free(pHandlesBuffer);
       pHandlesBuffer = NULL;
   }
} */

/*
 *@@ wphQueryActiveHandles:
 *      this copies the contents of PM_Workplace:ActiveHandles
 *      in OS2SYS.INI into a given buffer. There are always two
 *      buffers in OS2SYS.INI for object handles, called
 *      "PM_Workplace:HandlesX" with "X" either being "0" or "1".
 *      It seems that every time the WPS does something in the
 *      handles section, it writes the data to the inactive
 *      buffer first and then makes it the active buffer by
 *      changing the "active handles" key. You can test this
 *      by creating a shadow on your Desktop.
 *
 *      This function copies the key only, but not the actual
 *      handles blocks (use wphReadAllBlocks for that).
 *
 *      This gets called by the one-shot function
 *      wphQueryHandleFromPath.
 */

BOOL wphQueryActiveHandles(HINI hIniSystem,       // in: can be HINI_USER or HINI_SYSTEM
                           PSZ pszHandlesAppName, // out: active handles buffer.
                           USHORT usMax)          // in:  sizeof(pszHandlesAppName)
{
    PBYTE pszHandles;
    ULONG ulProfileSize;

    pszHandles = wphQueryProfileData(hIniSystem,
                                     ACTIVEHANDLES, HANDLESAPP,
                                     &ulProfileSize);
    if (!pszHandles)
    {
        strncpy(pszHandlesAppName, HANDLES, usMax-1);
        return TRUE;
    }
    // fNewFormat = TRUE;
    strncpy(pszHandlesAppName, pszHandles, usMax-1);
    free(pszHandles);
    return TRUE;
}

/*
 *@@ wphReadAllBlocks:
 *      this reads all the BLOCK's in the active handles
 *      section in OS2SYS.INI into one common buffer, for
 *      which sufficient memory is allocated.
 *
 *      Each of these blocks is a maximum of 64 KB, hell
 *      knows why, maybe the INIs can't handle more. This
 *      func combines all the BLOCK's into one buffer anyways,
 *      which can then be searched.
 *
 *      Use wphQueryActiveHandles to pass the active handles
 *      section to this function.
 *
 *      This gets called by the one-shot function
 *      wphQueryHandleFromPath.
 */

BOOL wphReadAllBlocks(HINI hiniSystem,       // in: can be HINI_USER or HINI_SYSTEM
                      PSZ pszActiveHandles,  // in: active handles section
                      PBYTE* ppBlock,        // in/out: pointer to buffer, which
                                             //    will point to the allocated
                                             //    memory afterwards
                      PULONG pulSize)        // out: size of the allocated buffer
{
    PBYTE   pbAllBlocks,
            pszBlockName,
            p;
    ULONG   ulBlockSize;
    ULONG   ulTotalSize;
    BYTE    rgfBlock[100];
    BYTE    szAppName[10];
    USHORT  usBlockNo;

    pbAllBlocks = wphEnumProfileKeys(hiniSystem, pszActiveHandles, &ulBlockSize);
    if (!pbAllBlocks)
       return FALSE;

    // query size of all individual blocks and calculate total
    memset(rgfBlock, 0, sizeof rgfBlock);
    ulTotalSize = 0L;
    pszBlockName = pbAllBlocks;
    while (*pszBlockName)
    {
        if (!memicmp(pszBlockName, "BLOCK", 5))
        {
           usBlockNo = atoi(pszBlockName + 5);
           if (usBlockNo < 100)
           {
               rgfBlock[usBlockNo] = TRUE;

               if (!PrfQueryProfileSize(hiniSystem,
                                        pszActiveHandles,
                                        pszBlockName,
                                        &ulBlockSize))
               {
                   free(pbAllBlocks);
                   return FALSE;
               }
               ulTotalSize += ulBlockSize;
           }
        }
        pszBlockName += strlen(pszBlockName) + 1;
    }

    // *pulSize now contains the total size of all blocks
    *ppBlock = (PBYTE)malloc(ulTotalSize);
    if (!(*ppBlock))
    {
       /* MessageBox("wphReadAllBlocks", "Not enough memory for profile data!"); */
         free(pbAllBlocks);
         return FALSE;
    }

    // now get all blocks into the memory we allocated
    p = *ppBlock;
    (*pulSize) = 0;
    for (usBlockNo = 1; usBlockNo < 100; usBlockNo++)
    {
        if (!rgfBlock[usBlockNo])
            break;

        sprintf(szAppName, "BLOCK%u", (UINT)usBlockNo);
        ulBlockSize = ulTotalSize;
        if (!PrfQueryProfileData(hiniSystem,
                                 pszActiveHandles,
                                 szAppName,
                                 p,
                                 &ulBlockSize))
        {
            free(pbAllBlocks);
            free(*ppBlock);
            return FALSE;
        }
        p += ulBlockSize;
        (*pulSize) += ulBlockSize;
        ulTotalSize -= ulBlockSize;
    }

    free(pbAllBlocks);
    return TRUE;
}

/* ******************************************************************
 *                                                                  *
 *   Get HOBJECT from filename                                      *
 *                                                                  *
 ********************************************************************/

/*
 *@@ wphSearchBufferForHandle:
 *      returns the four-digit object handle which corresponds
 *      to pszFname, searching pHandlesBuffer. Note that you
 *      must OR the return value with 0x30000 to make this
 *      a valid WPS file-system handle.
 *
 *      You must pass a handles buffer to this function which
 *      has been filled using wphReadAllBlocks above.
 *
 *      This gets called by the one-shot function
 *      wphQueryHandleFromPath.
 */

USHORT wphSearchBufferForHandle(PBYTE pHandlesBuffer, // in: handles buffer (all BLOCK's)
                                ULONG ulBufSize,      // in: sizeof(pHandlesBuffer)
                                USHORT usParent,      // in: parent NODE ID;
                                                      //     must be 0 initially
                                PSZ pszFname) // in: fully qlf'd filename to search for
{
    PDRIV pDriv;
    PNODE pNode;
    PBYTE pCur;                 // current
    PBYTE p,
          pEnd;                 // *end of buffer
    USHORT usPartSize;

    // _Pmpf(("Entering wphSearchBufferForHandle for %s", pszFname));

    // The composed BLOCKs in the handles buffer make up a tree of
    // DRIVE and NODE structures (see wphandle.h). Each NODE stands
    // for either a directory or a file. (We don't care about the
    // DRIVE structures because the root directory gets a NODE also.)
    // Each NODE contains the non-qualified file name, an object ID,
    // and the object ID of its parent NODE.
    // We can thus work our way through the buffer by splitting the
    // fully qualified filename that we're searching for into the
    // different directory names and, each time, searching for the
    // corresponding NODE. If we have found that, we go for the next.
    // Example for C:\OS2\E.EXE:
    //   1) first search for the "C:" NODE
    //   2) then find the "OS2" node which has "C" as its parent NODE
    //      (we do this by comparing the parent object handles)
    //   3) then find the "E.EXE" NODE the same way
    // The "E.EXE" NODE then has the object handle we're looking for.

    // So first find the length of the first filename part (which
    // should be 2 for the drive letter, "C:")
    p = strchr(pszFname, '\\');
    if (p)
        // backslash found:
        usPartSize = p - pszFname;      // extract first part
    else
        usPartSize = strlen(pszFname);

    // now set the pointer for the end of the BLOCKs buffer
    pEnd = pHandlesBuffer + ulBufSize;

    // pCur is our variable pointer where we're at now; there
    // is some offset of 4 bytes at the beginning (duh)
    pCur = pHandlesBuffer + 4;

    // _Pmpf(("  Searching for: %s, usPartSize: %d", pszFname, usPartSize));

    // go!
    while (pCur < pEnd)
    {
        // the first four chars tell us whether it's
        // a DRIVE or a NODE structure
        if (!memicmp(pCur, "DRIV", 4))
        {
            // pCur points to a DRIVE node:
            // we don't care about these, because the root
            // directory has a real NODE too, so we just
            // skip this
            pDriv = (PDRIV)pCur;
            pCur += sizeof(DRIV) + strlen(pDriv->szName);
        }
        else if (!memicmp(pCur, "NODE", 4))
        {
            // pCur points to a regular NODE: offset pointer first
            pNode = (PNODE)pCur;
            pCur += sizeof (NODE) + pNode->usNameSize;

            // does the NODE have the same parent that we
            // are currently searching for? This is "0"
            // for root directories (and initially for us
            // too)
            if (usParent == pNode->usParentHandle)
            {
                // yes:
                // _Pmpf(("  found matching parents (%lX): %s", usParent, pNode->szName));

                // does the NODE have the same partname length?
                if (pNode->usNameSize == usPartSize)
                {
                    // yes:
                    // _Pmpf(("    found matching partnames sizes: %d", pNode->usNameSize));

                    // do the partnames match too?
                    if (memicmp(pszFname, pNode->szName, usPartSize) == 0)
                    {
                        // OK!! proper NODE found!
                        // _Pmpf(("      FOUND %s!!", pNode->szName));

                        // now check if this was the last NODE
                        // we were looking for
                        if (strlen(pszFname) == usPartSize)
                           // yes: return ID
                           return (pNode->usHandle);

                        // else: update our status;
                        // get next partname
                        pszFname += usPartSize + 1;
                        // calc next partname length
                        p = strchr(pszFname, '\\');
                        if (p)
                           usPartSize = p - pszFname;
                        else
                           usPartSize = strlen(pszFname);

                        // get next parent to search for
                        // (which is the current handle)
                        usParent = pNode->usHandle;
                    }
                }
            }
        }
        else
            // neither DRIVE nor NODE: error
            return (0);

    } // end while

    // not found: end of buffer reached
    return (0);
}

/*
 *@@ wphQueryHandleFromPath:
 *      find the object handle for pszName; this
 *      can either be an object ID ("<WP_DESKTOP>")
 *      or a fully qualified filename.
 *      This is a one-shot function, using wphQueryActiveHandles,
 *      wphReadAllBlocks, and wphSearchBufferForHandle.
 *
 *      Returns:
 *      --  -1:     file does not exist
 *      --  -2:     error querying handles buffer
 *      --  0:      object handle not found; might not exist
 *      --  other:  found object handle
 *
 *      NOTE: This function uses C runtime library
 *      string comparison functions. These only work
 *      properly if you have set the locale for the
 *      C runtime properly. This is, for example, a
 *      problem with file names containing German umlauts,
 *      which are not found properly.
 *      You should put some
 +          setlocale(LC_ALL, "");
 *      statement somewhere, which reacts to the LANG
 *      variable which OS/2 puts into CONFIG.SYS per
 *      default.
 */

HOBJECT wphQueryHandleFromPath(HINI hIniUser,   // in: user ini file
                               HINI hIniSystem, // in: system ini file
                               PSZ pszName)     // in: fully qlf'd filename
{
    PBYTE       pHandlesBuffer = NULL;
    ULONG       cbHandlesBuffer = 0,
                cbLocation = 0;
    USHORT      usObjID;
    PVOID       pvData;
    HOBJECT     hObject = 0L;
    BYTE        szFullPath[300];
    CHAR        szActiveHandles[100];

    // _Pmpf(("wphQueryHandleFromPath: %s", pszName));

    // try to get the objectID via PM_Workplace:Location, since
    // pszName might also be a "<WP_DESKTOP>" like thingy
    pvData = wphQueryProfileData(hIniUser,
                                 LOCATION,   // "PM_Workplace:Location" (<WP_DESKTOP> etc.)
                                 pszName,
                                 &cbLocation);
    if (pvData)
    {
        // found there:
        if (cbLocation >= sizeof(HOBJECT))
            hObject = *(PULONG)pvData;
        free(pvData);
        // _Pmpf(("  object ID found, hObject: %lX", hObject));
    }
    if (hObject)
        return (hObject);

    // not found there: is pszName an existing pathname?
    if (access(pszName, 0))  // check existence
    {
        // == -1: file does not exist
        // _Pmpf(("  path not found, returning -1"));
        return (-1);
    }

    // else: make full path for pszName
    _fullpath(szFullPath, pszName, sizeof(szFullPath));

    // check if the HandlesBlock is valid
    wphQueryActiveHandles(hIniSystem, szActiveHandles, sizeof(szActiveHandles));
    // _Pmpf(("  szActiveHandles: %s", szActiveHandles));

    // now load all the BLOCKs into a common buffer
    if (!wphReadAllBlocks(hIniSystem,
                           szActiveHandles,
                           &pHandlesBuffer, &cbHandlesBuffer))
        // error:
        return (-2);

    // and search that buffer
    usObjID = wphSearchBufferForHandle(pHandlesBuffer,
                                       cbHandlesBuffer,
                                       0,                  // usParent
                                       szFullPath);

    if (usObjID)
        // found: OR 0x30000
        hObject = MakeDiskHandle(usObjID);

    free(pHandlesBuffer);

    return (hObject);
}

/* ******************************************************************
 *                                                                  *
 *   Get filename from HOBJECT                                      *
 *                                                                  *
 ********************************************************************/

/*
 *@@ wphFindPartName:
 *      this searches pHandlesBuffer for usHandle and, if found,
 *      appends the object partname to pszFname.
 *      This function recurses, if neccessary.
 *
 *      This gets called by the one-shot function
 *      wphQueryPathFromHandle.
 */

PNODE wphFindPartName(PBYTE pHandlesBuffer, // in: handles buffer
                      ULONG ulBufSize,      // in: buffer size
                      USHORT usHandle,      // in: handle to search for
                      PSZ pszFname,         // out: object partname
                      USHORT usMax)         // in: sizeof(pszFname)
{
    PDRIV pDriv;
    PNODE pNode;
    PBYTE p, pEnd;
    USHORT usSize;

    pEnd = pHandlesBuffer + ulBufSize;
    p = pHandlesBuffer + 4;
    while (p < pEnd)
    {
        if (!memicmp(p, "DRIV", 4))
        {
            pDriv = (PDRIV)p;
            p += sizeof(DRIV) + strlen(pDriv->szName);
        }
        else if (!memicmp(p, "NODE", 4))
        {
            pNode = (PNODE)p;
            p += sizeof (NODE) + pNode->usNameSize;
            if (pNode->usHandle == usHandle)
            {
                usSize = usMax - strlen(pszFname);
                if (usSize > pNode->usNameSize)
                    usSize = pNode->usNameSize;
                if (pNode->usParentHandle)
                {
                    if (!wphFindPartName(pHandlesBuffer, ulBufSize,
                                         pNode->usParentHandle,
                                         pszFname,
                                         usMax))
                       return (NULL);
                    strcat(pszFname, "\\");
                    strncat(pszFname, pNode->szName, usSize);
                    return pNode;
                }
                else
                {
                    strncpy(pszFname, pNode->szName, usSize);
                    return pNode;
                }
            }
        }
        else
           return (NULL);
    }
    return (NULL);
}

/*
 *@@ wphQueryPathFromHandle:
 *      reverse to wphQueryHandleFromPath, this gets the
 *      filename for hObject.
 *      This is a one-shot function, using wphQueryActiveHandles,
 *      wphReadAllBlocks, and wphFindPartName.
 *
 *@@changed V0.9.4 (2000-08-03) [umoeller]: now returning BOOL
 */

BOOL wphQueryPathFromHandle(HINI hIniSystem,    // in: HINI_SYSTEM or other INI handle
                            HOBJECT hObject,    // in: five-digit object handle
                            PSZ pszFname,       // out: filename, if found
                            USHORT usMax)       // in: sizeof(*pszFname)
{
    BOOL        brc = FALSE;

    if (IsObjectDisk(hObject))
    {
        // use lower byte only
        USHORT      usObjID = LOUSHORT(hObject);

        PBYTE       pHandlesBuffer = NULL;
        ULONG       cbHandlesBuffer = 0;

        CHAR        szActiveHandles[100];
        // PNODE       pReturnNode = 0;

        wphQueryActiveHandles(hIniSystem, szActiveHandles, sizeof(szActiveHandles));

        if (wphReadAllBlocks(hIniSystem,
                             szActiveHandles,
                             &pHandlesBuffer,
                             &cbHandlesBuffer))
        {
            memset(pszFname, 0, usMax);
            if (wphFindPartName(pHandlesBuffer,
                                cbHandlesBuffer,
                                usObjID,
                                pszFname,
                                usMax))
                brc = TRUE;

            free(pHandlesBuffer);
        }
    }

    return (brc);
}

/* ******************************************************************
 *                                                                  *
 *   Manipulation functions                                         *
 *                                                                  *
 ********************************************************************/

/*
 *@@ WriteAllBlocks:
 *      writes all blocks back to OS2SYS.INI.
 *
 *@@added V0.9.5 (2000-08-20) [umoeller]
 */

BOOL WriteAllBlocks(HINI hini, PSZ pszHandles, PBYTE pBuffer, ULONG ulSize)
{
    PSZ     p,
            pEnd;
    BYTE    szBlockName[10];
    INT     iCurBlock;
    ULONG   ulCurSize;
    PBYTE   pStart;
    PDRIV   pDriv;
    PNODE   pNode;

    pStart    = pBuffer;
    ulCurSize = 4;
    p    = pBuffer + 4;
    pEnd = pBuffer + ulSize;
    iCurBlock = 1;
    while (p < pEnd)
    {
        // ULONG   cbWrite = 0;
        while (p < pEnd)
        {
            ULONG   ulPartSize = 0;
            if (!memicmp(p, "DRIV", 4))
            {
                pDriv = (PDRIV)p;
                ulPartSize = sizeof(DRIV) + strlen(pDriv->szName);
            }
            else if (!memicmp(p, "NODE", 4))
            {
                pNode = (PNODE)p;
                ulPartSize = sizeof (NODE) + pNode->usNameSize;
            }

            if (ulCurSize + ulPartSize > 0x0000FFFF)
                break;

            ulCurSize += ulPartSize;
            p         += ulPartSize;
        }
        sprintf(szBlockName, "BLOCK%d", iCurBlock++);

        PrfWriteProfileData(hini,
                            pszHandles,     // app
                            szBlockName,    // key
                            pStart,
                            ulCurSize);
        pStart    = p;
        ulCurSize = 0;
    }

    while (iCurBlock < 20)
    {
        ULONG ulBlockSize;

        sprintf(szBlockName, "BLOCK%d", iCurBlock++);

        if (PrfQueryProfileSize(hini,
                                pszHandles,
                                szBlockName,
                                &ulBlockSize)
                && ulBlockSize > 0)
            // delete block:
            PrfWriteProfileData(hini,
                                pszHandles,
                                szBlockName,
                                NULL, 0);       // delete
            // WriteProfileData(pszHandles, szBlockName, hini, NULL, 0);
    }

    return TRUE;
}

/*
 *@@ DeleteNode:
 *      deletes a NODE in the specified buffer.
 *
 *@@added V0.9.5 (2000-08-20) [umoeller]
 */

ULONG DeleteNode(PBYTE pBuffer, PNODE pNode, ULONG ulSize)
{
    ULONG ulDelSize = sizeof (NODE) + pNode->usNameSize;
    USHORT usID = pNode->usHandle; // pNode->usID;
    ULONG ulMoveSize;

    if (memcmp(pNode->chName, "NODE", 4))
        return ulSize;

    ulMoveSize = (pBuffer + ulSize) - ((PBYTE)pNode + ulDelSize);
    ulSize -= ulDelSize;

    memmove(pNode, (PBYTE)pNode + ulDelSize, ulMoveSize);

    while (     (PBYTE)pNode < pBuffer + ulSize
             && !memcmp(pNode->chName, "NODE", 4)
             && pNode->usParentHandle == usID)
        ulSize = DeleteNode(pBuffer, pNode, ulSize);

    return ulSize;
 }

/*
 *@@ DeleteDrive:
 *      delete all information about a drive.
 *      in the specified buffer.
 *
 *@@added V0.9.5 (2000-08-20) [umoeller]
 */

ULONG DeleteDrive(PBYTE pBuffer, PDRIV pDriv, ULONG ulSize)
{
    ULONG ulDelSize;
    ULONG ulMoveSize;

    if (memcmp(pDriv->chName, "DRIV", 4))
        return ulSize;

    ulDelSize = sizeof (DRIV) + strlen(pDriv->szName);
    ulMoveSize = (pBuffer + ulSize) - ((PBYTE)pDriv + ulDelSize);
    ulSize -= ulDelSize;

    memmove(pDriv, (PBYTE)pDriv + ulDelSize, ulMoveSize);
    return ulSize;
}



