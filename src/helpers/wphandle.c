
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
 *      Thanks go out to Henk Kelder for telling me the
 *      format of the WPS INI data. With V0.9.16, this
 *      file was completely rewritten and no longer uses
 *      his code though.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@header "helpers\wphandle.h"
 */

/*
 *      This file Copyright (C) 1997-2001 Ulrich M”ller,
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

#define OS2EMX_PLAIN_CHAR
    // this is needed for "os2emx.h"; if this is defined,
    // emx will define PSZ as _signed_ char, otherwise
    // as unsigned char

#define INCL_DOSEXCEPTIONS
#define INCL_DOSPROCESS
#define INCL_DOSERRORS

#define INCL_WINSHELLDATA
#include <os2.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <io.h>
#include <setjmp.h>

#include "setup.h"                      // code generation and debugging options

#include "helpers\except.h"
#include "helpers\prfh.h"
#include "helpers\standards.h"
#include "helpers\stringh.h"
#include "helpers\wphandle.h"
#include "helpers\xstring.h"

/*
 *@@category: Helpers\PM helpers\Workplace Shell\Handles (OS2SYS.INI)
 *      See wphandle.c.
 */

/* ******************************************************************
 *
 *   Load handles functions
 *
 ********************************************************************/

/*
 *@@ wphQueryActiveHandles:
 *      returns the value of PM_Workplace:ActiveHandles
 *      in OS2SYS.INI as a new buffer.
 *
 *      There are always two buffers in OS2SYS.INI for object
 *      handles, called "PM_Workplace:HandlesX" with "X" either
 *      being "0" or "1".
 *
 *      It seems that every time the WPS does something in the
 *      handles section, it writes the data to the inactive
 *      buffer first and then makes it the active buffer by
 *      changing the "active handles" key. You can test this
 *      by creating a shadow on your Desktop.
 *
 *      This returns a new PSZ which the caller must free()
 *      after use.
 *
 *      This gets called by the one-shot function
 *      wphQueryHandleFromPath.
 *
 *@@changed V0.9.16 (2001-10-02) [umoeller]: rewritten
 */

APIRET wphQueryActiveHandles(HINI hiniSystem,
                             PSZ *ppszActiveHandles)        // out: active handles string (new buffer)
{
    PSZ pszActiveHandles;
    if (pszActiveHandles = prfhQueryProfileData(hiniSystem,
                                                WPINIAPP_ACTIVEHANDLES,
                                                WPINIAPP_HANDLESAPP,
                                                NULL))
    {
        *ppszActiveHandles = pszActiveHandles;
        return (NO_ERROR);
    }

    return (ERROR_WPH_NO_ACTIVEHANDLES_DATA);
}

/*
 *@@ wphQueryBaseClassesHiwords:
 *      returns the hiwords for the WPS base
 *      classes. Unless the user's system is
 *      really badly configured, this should
 *      set
 *
 *      --  pusHiwordAbstract to 2;
 *      --  pusHiwordFileSystem to 3.
 *
 *      Returns:
 *
 *      --  NO_ERROR
 *
 *      --  ERROR_WPH_NO_BASECLASS_DATA
 *
 *      --  ERROR_WPH_INCOMPLETE_BASECLASS_DATA
 *
 *      This gets called automatically from wphLoadHandles.
 *
 *@@added V0.9.16 (2001-10-02) [umoeller]
 */

APIRET wphQueryBaseClassesHiwords(HINI hiniUser,
                                  PUSHORT pusHiwordAbstract,
                                  PUSHORT pusHiwordFileSystem)
{
    APIRET arc = NO_ERROR;

    // get the index of WPFileSystem from the base classes list...
    // we need this to determine the hiword for file-system handles
    // properly. Normally, this should be 3.
    ULONG cbBaseClasses = 0;
    PSZ pszBaseClasses;
    if (pszBaseClasses = prfhQueryProfileData(hiniUser,
                                              "PM_Workplace:BaseClass",
                                              "ClassList",
                                              &cbBaseClasses))
    {
        // parse that buffer... these has the base class names,
        // separated by 0. List is terminated by two zeroes.
        PSZ     pszClassThis = pszBaseClasses;
        ULONG   ulHiwordThis = 1;
        while (    (*pszClassThis)
                && (pszClassThis - pszBaseClasses < cbBaseClasses)
              )
        {
            if (!strcmp(pszClassThis, "WPFileSystem"))
                *pusHiwordFileSystem = ulHiwordThis;
            else if (!strcmp(pszClassThis, "WPAbstract"))
                *pusHiwordAbstract = ulHiwordThis;

            ulHiwordThis++;
            pszClassThis += strlen(pszClassThis) + 1;
        }

        // now check if we found both
        if (    (!(*pusHiwordFileSystem))
             || (!(*pusHiwordAbstract))
           )
            arc = ERROR_WPH_INCOMPLETE_BASECLASS_DATA;

        free(pszBaseClasses);
    }
    else
        arc = ERROR_WPH_NO_BASECLASS_DATA;

    return (arc);
}

/*
 *@@ wphRebuildNodeHashTable:
 *
 *      Returns:
 *
 *      --  NO_ERROR
 *
 *      --  ERROR_INVALID_PARAMETER
 *
 *      --  ERROR_WPH_CORRUPT_HANDLES_DATA
 *
 *@@added V0.9.16 (2001-10-02) [umoeller]
 */

APIRET wphRebuildNodeHashTable(PHANDLESBUF pHandlesBuf)
{
    APIRET arc = NO_ERROR;

    if (    (!pHandlesBuf)
         || (!pHandlesBuf->pbData)
         || (!pHandlesBuf->cbData)
       )
        arc = ERROR_INVALID_PARAMETER;
    else
    {
        // start at beginning of buffer
        PBYTE pCur = pHandlesBuf->pbData + 4;
        PBYTE pEnd = pHandlesBuf->pbData + pHandlesBuf->cbData;

        memset(pHandlesBuf->NodeHashTable, 0, sizeof(pHandlesBuf->NodeHashTable));

        // now set up hash table
        while (pCur < pEnd)
        {
            if (!memicmp(pCur, "DRIV", 4))
            {
                // pCur points to a DRIVE node:
                // these never have handles, so skip this
                PDRIV pDriv = (PDRIV)pCur;
                pCur += sizeof(DRIV) + strlen(pDriv->szName);
            }
            else if (!memicmp(pCur, "NODE", 4))
            {
                // pCur points to a regular NODE: offset pointer first
                PNODE pNode = (PNODE)pCur;
                // store PNODE in hash table
                pHandlesBuf->NodeHashTable[pNode->usHandle] = pNode;
                pCur += sizeof(NODE) + pNode->usNameSize;
            }
            else
            {
                arc = ERROR_WPH_CORRUPT_HANDLES_DATA;
                break;
            }
        }
    }

    if (!arc)
        pHandlesBuf->fNodeHashTableValid = TRUE;

    return (arc);
}

/*
 *@@ wphLoadHandles:
 *      returns a HANDLESBUF structure which will hold
 *      all the handles from OS2SYS.INI. In addition,
 *      this calls wphQueryBaseClassesHiwords and puts
 *      the hiwords for WPAbstract and WPFileSystem into
 *      the HANDLESBUF as well.
 *
 *      Prerequisite before using any of the other wph*
 *      functions.
 *
 *      Call wphFreeHandles to free all data allocated
 *      by this function.
 *
 *      Returns:
 *
 *      --  NO_ERROR
 *
 *      --  ERROR_NOT_ENOUGH_MEMORY
 *
 *      --  ERROR_INVALID_PARAMETER
 *
 *      --  ERROR_WPH_NO_HANDLES_DATA: cannot read handle blocks.
 *
 *      --  ERROR_WPH_CORRUPT_HANDLES_DATA: cannot read handle blocks.
 *
 *@@added V0.9.16 (2001-10-02) [umoeller]
 */

APIRET wphLoadHandles(HINI hiniUser,      // in: HINI_USER or other INI handle
                      HINI hiniSystem,    // in: HINI_SYSTEM or other INI handle
                      const char *pcszActiveHandles,
                      PHANDLESBUF *ppHandlesBuf)
{
    APIRET arc = NO_ERROR;

    if (!ppHandlesBuf)
        arc = ERROR_INVALID_PARAMETER;
    else
    {
        PSZ pszKeysList;
        if (!(arc = prfhQueryKeysForApp(hiniSystem,
                                        pcszActiveHandles,
                                        &pszKeysList)))
        {
            PHANDLESBUF pReturn = NULL;

            ULONG   ulHighestBlock = 0,
                    ul,
                    cbTotal;
            PBYTE   pbData;

            const char *pKey2 = pszKeysList;
            while (*pKey2)
            {
                if (!memicmp((PVOID)pKey2, "BLOCK", 5))
                {
                    ULONG ulBlockThis = atoi(pKey2 + 5);
                    if (ulBlockThis > ulHighestBlock)
                        ulHighestBlock = ulBlockThis;
                }

                pKey2 += strlen(pKey2)+1; // next key
            }

            free(pszKeysList);

            if (!ulHighestBlock)
                arc = ERROR_WPH_NO_HANDLES_DATA;
            else
            {
                // now go read the data
                // (BLOCK1, BLOCK2, ..., BLOCKn)
                cbTotal = 0;
                pbData = NULL;
                for (ul = 1;
                     ul <= ulHighestBlock;
                     ul++)
                {
                    ULONG   cbBlockThis;
                    CHAR    szBlockThis[10];
                    sprintf(szBlockThis, "BLOCK%d", ul);
                    if (!PrfQueryProfileSize(hiniSystem,
                                             (PSZ)pcszActiveHandles,
                                             szBlockThis,
                                             &cbBlockThis))
                    {
                        arc = ERROR_WPH_CORRUPT_HANDLES_DATA;
                        break;
                    }
                    else
                    {
                        ULONG   cbTotalOld = cbTotal;
                        cbTotal += cbBlockThis;
                        if (!(pbData = (BYTE*)realloc(pbData, cbTotal)))
                                // on first call, pbData is NULL and this
                                // behaves like malloc()
                        {
                            arc = ERROR_NOT_ENOUGH_MEMORY;
                            break;
                        }

                        if (!PrfQueryProfileData(hiniSystem,
                                                 (PSZ)pcszActiveHandles,
                                                 szBlockThis,
                                                 pbData + cbTotalOld,
                                                 &cbBlockThis))
                        {
                            arc = ERROR_WPH_CORRUPT_HANDLES_DATA;
                            break;
                        }
                    }
                }
            }

            if (!arc)
            {
                // all went OK:
                if (pReturn = NEW(HANDLESBUF))
                {
                    ZERO(pReturn);

                    pReturn->pbData = pbData;
                    pReturn->cbData = cbTotal;

                    // and load the hiwords too
                    if (!(arc = wphQueryBaseClassesHiwords(hiniUser,
                                                           &pReturn->usHiwordAbstract,
                                                           &pReturn->usHiwordFileSystem)))
                        *ppHandlesBuf = pReturn;
                }
                else
                    arc = ERROR_NOT_ENOUGH_MEMORY;
            }

            if (arc)
                // error:
                wphFreeHandles(&pReturn);
        }
    }

    return (arc);
}

/*
 *@@ wphFreeHandles:
 *      frees all data allocated by wphLoadHandles
 *      and sets *ppHandlesBuf to NULL, for safety.
 *
 *@@added V0.9.16 (2001-10-02) [umoeller]
 */

APIRET wphFreeHandles(PHANDLESBUF *ppHandlesBuf)
{
    APIRET arc = NO_ERROR;

    if (ppHandlesBuf && *ppHandlesBuf)
    {
        PBYTE pbData;
        if (pbData = (*ppHandlesBuf)->pbData)
            free(pbData);

        free(*ppHandlesBuf);
        *ppHandlesBuf = NULL;
    }
    else
        arc = ERROR_INVALID_PARAMETER;

    return (arc);
}

/* ******************************************************************
 *
 *   Get HOBJECT from filename
 *
 ********************************************************************/

/*
 *@@ wphSearchBufferForHandle:
 *      returns the four-digit object handle which corresponds
 *      to pszFilename, searching pHandlesBuffer. Note that you
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
                                PSZ pszFilename) // in: fully qlf'd filename to search for
{
    PDRIV pDriv;
    PNODE pNode;
    PBYTE pCur;                 // current
    PBYTE p,
          pEnd;                 // *end of buffer
    USHORT usPartSize;

    // _Pmpf(("Entering wphSearchBufferForHandle for %s", pszFilename));

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
    p = strchr(pszFilename, '\\');
    if (p)
        // backslash found:
        usPartSize = p - pszFilename;      // extract first part
    else
        usPartSize = strlen(pszFilename);

    // now set the pointer for the end of the BLOCKs buffer
    pEnd = pHandlesBuffer + ulBufSize;

    // pCur is our variable pointer where we're at now; there
    // is some offset of 4 bytes at the beginning (duh)
    pCur = pHandlesBuffer + 4;

    // _Pmpf(("  Searching for: %s, usPartSize: %d", pszFilename, usPartSize));

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
                    if (memicmp(pszFilename, pNode->szName, usPartSize) == 0)
                    {
                        // OK!! proper NODE found!
                        // _Pmpf(("      FOUND %s!!", pNode->szName));

                        // now check if this was the last NODE
                        // we were looking for
                        if (strlen(pszFilename) == usPartSize)
                           // yes: return ID
                           return (pNode->usHandle);

                        // else: update our status;
                        // get next partname
                        pszFilename += usPartSize + 1;
                        // calc next partname length
                        p = strchr(pszFilename, '\\');
                        if (p)
                            usPartSize = p - pszFilename;
                        else
                            usPartSize = strlen(pszFilename);

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
 *      finds the object handle for the given fully qualified
 *      filename.
 *      This is a one-shot function, using wphQueryActiveHandles,
 *      wphReadAllBlocks, and wphSearchBufferForHandle.
 *
 *      Returns:
 *
 *      --  NO_ERROR: *phobj has received the object handle.
 *
 *      --  ERROR_FILE_NOT_FOUND: file does not exist.
 *
 *      plus the error codes of the other wph* functions called.
 *
 *@@changed V0.9.16 (2001-10-02) [umoeller]: rewritten
 */

APIRET wphQueryHandleFromPath(HINI hiniUser,      // in: HINI_USER or other INI handle
                              HINI hiniSystem,    // in: HINI_SYSTEM or other INI handle
                              const char *pcszName,    // in: fully qlf'd filename
                              HOBJECT *phobj)      // out: object handle found if NO_ERROR
{
    APIRET      arc = NO_ERROR;

    PSZ         pszActiveHandles = NULL;
    PHANDLESBUF pHandlesBuf = NULL;

    TRY_LOUD(excpt1)
    {
        // not found there: check the handles then

        if (arc = wphQueryActiveHandles(hiniSystem, &pszActiveHandles))
            _Pmpf((__FUNCTION__ ": wphQueryActiveHandles returned %d", arc));
        else
        {
            if (arc = wphLoadHandles(hiniUser,
                                     hiniSystem,
                                     pszActiveHandles,
                                     &pHandlesBuf))
                _Pmpf((__FUNCTION__ ": wphLoadHandles returned %d", arc));
            else
            {
                USHORT      usObjID;
                CHAR        szFullPath[2*CCHMAXPATH];
                _fullpath(szFullPath, (PSZ)pcszName, sizeof(szFullPath));

                // search that buffer
                if (usObjID = wphSearchBufferForHandle(pHandlesBuf->pbData,
                                                       pHandlesBuf->cbData,
                                                       0,                  // usParent
                                                       szFullPath))
                    // found: OR 0x30000
                    *phobj = usObjID | (pHandlesBuf->usHiwordFileSystem << 16);
                else
                    arc = ERROR_FILE_NOT_FOUND;
            }
        }
    }
    CATCH(excpt1)
    {
        arc = ERROR_WPH_CRASHED;
    } END_CATCH();

    if (pszActiveHandles)
        free(pszActiveHandles);
    if (pHandlesBuf)
        wphFreeHandles(&pHandlesBuf);

    return (arc);
}

/* ******************************************************************
 *
 *   Get filename from HOBJECT
 *
 ********************************************************************/

/*
 *@@ ComposeThis:
 *      helper for wphComposePath recursion.
 *
 *@@added V0.9.16 (2001-10-02) [umoeller]
 */

APIRET ComposeThis(PHANDLESBUF pHandlesBuf,
                   USHORT usHandle,         // in: handle to search for
                   PXSTRING pstrFilename,   // in/out: filename
                   PNODE *ppNode)           // out: node found (ptr can be NULL)
{
    APIRET arc = NO_ERROR;
    PNODE pNode;
    if (pNode = pHandlesBuf->NodeHashTable[usHandle])
    {
        // handle exists:
        if (pNode->usParentHandle)
        {
            // node has parent:
            // recurse first
            if (arc = ComposeThis(pHandlesBuf,
                                  pNode->usParentHandle,
                                  pstrFilename,
                                  ppNode))
            {
                if (arc == ERROR_INVALID_HANDLE)
                    // parent handle not found:
                    arc = ERROR_WPH_INVALID_PARENT_HANDLE;
                // else leave the APIRET, this might be dangerous
            }
            else
            {
                // no error:
                xstrcatc(pstrFilename, '\\');
                xstrcat(pstrFilename, pNode->szName, pNode->usNameSize);
            }
        }
        else
            // no parent:
            xstrcpy(pstrFilename, pNode->szName, pNode->usNameSize);
    }
    else
        arc = ERROR_INVALID_HANDLE;

    if (!arc)
        if (ppNode)
            *ppNode = pNode;

    return (arc);
}

/*
 *@@ wphComposePath:
 *      returns the fully qualified path name for the specified
 *      file-system handle. This function is very fast because
 *      it uses a hash table for all the handles internally.
 *
 *      Warning: This calls a helper, which recurses.
 *
 *      This returns:
 *
 *      --  NO_ERROR
 *
 *      --  ERROR_WPH_CORRUPT_HANDLES_DATA: buffer data cannot be parsed.
 *
 *      --  ERROR_WPH_INVALID_HANDLE: usHandle cannot be found.
 *
 *      --  ERROR_WPH_INVALID_PARENT_HANDLE: a handle was found
 *          that has a broken parent handle.
 *
 *      --  ERROR_BUFFER_OVERFLOW: cbFilename is too small to
 *          hold the full path that was composed.
 *
 *@@added V0.9.16 (2001-10-02) [umoeller]
 */

APIRET wphComposePath(PHANDLESBUF pHandlesBuf,
                      USHORT usHandle,      // in: loword of handle to search for
                      PSZ pszFilename,
                      ULONG cbFilename,
                      PNODE *ppNode)        // out: node found (ptr can be NULL)
{
    APIRET arc = NO_ERROR;

    TRY_LOUD(excpt1)
    {
        if (!pHandlesBuf->fNodeHashTableValid)
            arc = wphRebuildNodeHashTable(pHandlesBuf);

        if (!arc)
        {
            XSTRING str;
            xstrInit(&str, CCHMAXPATH);
            if (!(arc = ComposeThis(pHandlesBuf,
                                    usHandle,
                                    &str,
                                    ppNode)))
                if (str.ulLength > cbFilename - 1)
                    arc = ERROR_BUFFER_OVERFLOW;
                else
                    memcpy(pszFilename,
                           str.psz,
                           str.ulLength + 1);
        }
    }
    CATCH(excpt1)
    {
        arc = ERROR_WPH_CRASHED;
    } END_CATCH();

    return (arc);
}

/*
 *@@ wphQueryPathFromHandle:
 *      reverse to wphQueryHandleFromPath, this gets the
 *      filename for hObject.
 *      This is a one-shot function, using wphQueryActiveHandles,
 *      wphReadAllBlocks, and wphFindPartName.
 *
 *      Returns:
 *
 *      --  NO_ERROR
 *
 *      --  ERROR_INVALID_HANDLE: hObject is invalid.
 *
 *@@changed V0.9.16 (2001-10-02) [umoeller]: rewritten
 */

APIRET wphQueryPathFromHandle(HINI hiniUser,      // in: HINI_USER or other INI handle
                              HINI hiniSystem,    // in: HINI_SYSTEM or other INI handle
                              HOBJECT hObject,    // in: 32-bit object handle
                              PSZ pszFilename,    // out: filename, if found
                              ULONG cbFilename)   // in: sizeof(*pszFilename)
{
    APIRET arc = NO_ERROR;

    TRY_LOUD(excpt1)
    {
        PSZ pszActiveHandles;
        if (arc = wphQueryActiveHandles(hiniSystem, &pszActiveHandles))
            _Pmpf((__FUNCTION__ ": wphQueryActiveHandles returned %d", arc));
        else
        {
            PHANDLESBUF pHandlesBuf;
            if (arc = wphLoadHandles(hiniUser,
                                     hiniSystem,
                                     pszActiveHandles,
                                     &pHandlesBuf))
                _Pmpf((__FUNCTION__ ": wphLoadHandles returned %d", arc));
            else
            {
                // is this really a file-system object?
                if (HIUSHORT(hObject) == pHandlesBuf->usHiwordFileSystem)
                {
                    // use loword only
                    USHORT      usObjID = LOUSHORT(hObject);

                    memset(pszFilename, 0, cbFilename);
                    arc = wphComposePath(pHandlesBuf,
                                         usObjID,
                                         pszFilename,
                                         cbFilename,
                                         NULL);

                    _Pmpf((__FUNCTION__ ": wphFindPartName returned %d", arc));
                }

                wphFreeHandles(&pHandlesBuf);
            }

            free(pszActiveHandles);
        }
    }
    CATCH(excpt1)
    {
        arc = ERROR_WPH_CRASHED;
    } END_CATCH();

    return (arc);
}



