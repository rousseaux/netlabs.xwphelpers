
/*
 *@@sourcefile prfh.c:
 *      contains those Presentation Manager helper functions
 *      which deal with Profile (Prf*) functions.
 *      This file is new with V0.82.
 *
 *      Usage: All OS/2 programs.
 *
 *      Function prefixes:
 *      --  prfh*   Prf (profile, INI) helper functions
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@header "helpers\prfh.h"
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

#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_WIN
#include <os2.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "setup.h"                      // code generation and debugging options

#include "helpers\prfh.h"

#pragma hdrstop

/*
 *@@category: Helpers\Profile (INI) helpers
 */

/*
 *@@ prfhQueryKeysForApp:
 *      returns the keys list for an INI application. This
 *      list is copied into a newly allocated buffer, of which
 *      the address is returned.
 *
 *      Returns NULL upon errors.
 *
 *      If the return value is != NULL, the PSZ points to a new
 *      buffer which contains all the keys within the pszApp
 *      application. Each key name in the list is terminated with
 *      a null character. The last string in the list is terminated
 *      with two null characters.
 *
 *      The returned buffer should be freed later using free().
 *
 *      <B>Example</B> for iterating over a keys list:
 +          PSZ pszKeysList = prfhQueryKeysForApp(...);
 +          if (pszKeysList)
 +          {
 +              PSZ pKey2 = pszKeysList;
 +
 +              while (*pKey2 != 0)
 +              {
 +                  ...     // pKey2 has the current key now
 +                  pKey2 += strlen(pKey2)+1; // next key
 +              }
 +              free(pszKeysList);
 +          }
 *
 *      You can also use this function to query the applications
 *      list for hIni, if you specifiy pszApp as NULL.
 */

PSZ prfhQueryKeysForApp(HINI hIni,      // in: INI handle (can be HINI_USER or HINI_SYSTEM)
                        PSZ pszApp)     // in: application to query list for (or NULL for applications list)
{
    PSZ     pKeys = NULL;
    ULONG   ulSizeOfKeysList = 0;

    // get size of keys list for pszApp
    if (PrfQueryProfileSize(hIni, pszApp, NULL, &ulSizeOfKeysList))
    {
        pKeys = (PSZ)malloc(ulSizeOfKeysList);
        if (!PrfQueryProfileData(hIni, pszApp, NULL, pKeys, &ulSizeOfKeysList))
        {
            free(pKeys);
            pKeys = NULL;
        }
    }
    return (pKeys);
}

#ifdef __XWPMEMDEBUG__ // setup.h, helpers\memdebug.c

/*
 *@@ prfhQueryProfileDataDebug:
 *      debug version of prfhQueryProfileData, which is
 *      automatically mapped to if __XWPMEMDEBUG__ is defined.
 *
 *@@added V0.9.1 (99-12-20) [umoeller]
 *@@changed V0.9.3 (2000-04-20) [umoeller]: this called malloc(0) if the key existed, but was empty. Fixed.
 */

PSZ prfhQueryProfileDataDebug(HINI hIni,      // in: INI handle (can be HINI_USER or HINI_SYSTEM)
                              PSZ pszApp,      // in: application to query
                              PSZ pszKey,      // in: key to query
                              PULONG pcbBuf,   // out: size of the returned buffer
                              const char *file,
                              unsigned long line,
                              const char *function)
{
    PSZ     pData = NULL;
    ULONG   ulSizeOfData = 0;

    // get size of data for pszApp/pszKey
    if (PrfQueryProfileSize(hIni, pszApp, pszKey, &ulSizeOfData))
    {
        if (ulSizeOfData)
        {
            pData = (PSZ)memdMalloc(ulSizeOfData, file, line, function);
            if (!PrfQueryProfileData(hIni, pszApp, pszKey, pData, &ulSizeOfData))
            {
                free(pData);
                pData = NULL;
            }
        }
    }

    if (pcbBuf)
        *pcbBuf = ulSizeOfData;

    return (pData);
}

#else

/*
 *@@ prfhQueryProfileData:
 *      similar to PrfQueryProfileData, but this one copies
 *      the data into a newly allocated buffer, of which the
 *      address is returned.
 *
 *      Returns NULL upon errors, for example if the specified
 *      key doesn't exist or doesn't contain any data.
 *
 *      If pcbBuf != NULL, this func will write the size of
 *      the allocated buffer into *pcbBuf.
 *
 *      The returned buffer should be freed later using free().
 *
 *@@added V0.9.0 [umoeller]
 *@@changed V0.9.3 (2000-04-20) [umoeller]: this called malloc(0) if the key existed, but was empty. Fixed.
 */

PSZ prfhQueryProfileData(HINI hIni,      // in: INI handle (can be HINI_USER or HINI_SYSTEM)
                        PSZ pszApp,      // in: application to query
                        PSZ pszKey,      // in: key to query
                        PULONG pcbBuf)   // out: size of the returned buffer
{
    PSZ     pData = NULL;
    ULONG   ulSizeOfData = 0;

    // get size of data for pszApp/pszKey
    if (PrfQueryProfileSize(hIni, pszApp, pszKey, &ulSizeOfData))
    {
        if (ulSizeOfData)
        {
            pData = (PSZ)malloc(ulSizeOfData);
            if (!PrfQueryProfileData(hIni, pszApp, pszKey, pData, &ulSizeOfData))
            {
                free(pData);
                pData = NULL;
            }
        }
    }

    if (pcbBuf)
        *pcbBuf = ulSizeOfData;

    return (pData);
}

#endif

/*
 *@@ prfhQueryProfileChar:
 *      this query the first character of profile data.
 *      This is mostly useful with the PM country settings
 *      in OS2.INI:
 *      -- date separator: "PM_National", "sDate"
 *      -- time separator: "PM_National", "sTime"
 */

CHAR prfhQueryProfileChar(HINI hini,        // in: INI handle (can be HINI_USER or HINI_SYSTEM)
                          PSZ pszApp,       // in: application to query
                          PSZ pszKey,       // in: key to query
                          CHAR cDefault)    // in: default to return if not found
{
    // CHAR    crc = 0;
    CHAR    szTemp[5],
            szDefault[5];
    szDefault[0] = cDefault;
    szDefault[1] = 0;
    PrfQueryProfileString(HINI_USER, pszApp, pszKey,
                          szDefault,
                          szTemp, sizeof(szTemp)-1);
    return (szTemp[0]);
}

/*
 *@@ prfhQueryCountrySettings:
 *      this returns the most frequently used country settings
 *      all at once into a COUNTRYSETTINGS structure (prfh.h).
 *      This data corresponds to the user settings in the
 *      WPS "Country" object (which writes the data in "PM_National"
 *      in OS2.INI).
 *
 *      In case a key cannot be found, the following (English)
 *      default values are set:
 *      --  ulDateFormat = 0 (English date format, mm.dd.yyyy);
 *      --  ulTimeFormat = 0 (12-hour clock);
 *      --  cDateSep = '/' (date separator);
 *      --  cTimeSep = ':' (time separator);
 *      --  cThousands = ',' (thousands separator).
 *
 *@@added V0.9.0 [umoeller]
 */

VOID prfhQueryCountrySettings(PCOUNTRYSETTINGS pcs)
{
    if (pcs)
    {
        pcs->ulDateFormat = PrfQueryProfileInt(HINI_USER, "PM_National", "iDate", 0);
        pcs->ulTimeFormat = PrfQueryProfileInt(HINI_USER, "PM_National", "iTime", 0);
        pcs->cDateSep = prfhQueryProfileChar(HINI_USER, "PM_National", "sDate", '/');
        pcs->cTimeSep = prfhQueryProfileChar(HINI_USER, "PM_National", "sTime", ':');
        pcs->cThousands = prfhQueryProfileChar(HINI_USER, "PM_National", "sThousand", ',');
    }
}

/*
 *@@ prfhQueryColor:
 *      returns a system color in OS2.INI's PM_Colors as a LONG.
 */

LONG prfhQueryColor(PSZ pszKeyName, PSZ pszDefault)
{
    CHAR szColor[30];
    LONG r, g, b;
    PrfQueryProfileString(
                HINI_USER,
                "PM_Colors",
                pszKeyName,
                pszDefault,
                szColor,
                sizeof(szColor)-1);
    sscanf(szColor, "%d %d %d ", &r, &g, &b);
    return (r*0x10000 + g*0x100 + b);
}

/*
 *@@ prfhCopyKey:
 *      this copies one key from the given profile and application
 *      to another one.
 *
 *      pszTargetApp may be in the same profile (and must be
 *      different from pszSourceApp then) or in a different
 *      profile (and can be the same then).
 *
 *      You must specify all parameters. You cannot specify pszKey
 *      as NULL to have a whole application copied. Use prfhCopyApp
 *      for that.
 *      No check is made for this.
 *
 *      Returns:
 *      --  0: no error
 *      --  PRFERR_DATASIZE: couldn't query data size for key
 *      --  PRFERR_MEMORY: couldn't allocate memory
 *      --  PRFERR_READ: couldn't read data from source (PrfQueryProfileData error)
 *      --  PRFERR_WRITE: couldn't write data to target (PrfWriteProfileData error)
 *
 *@@added V0.9.0 [umoeller]
 */

ULONG prfhCopyKey(HINI hiniSource,       // in: source profile (can be HINI_USER or HINI_SYSTEM)
                 PSZ pszSourceApp,      // in: source application
                 PSZ pszKey,            // in: source/target key
                 HINI hiniTarget,       // in: target profile (can be HINI_USER or HINI_SYSTEM)
                 PSZ pszTargetApp)      // in: target app
{
    ULONG   ulSizeOfData = 0,
            ulrc = 0;       // return: no error

    if (PrfQueryProfileSize(hiniSource, pszSourceApp, pszKey, &ulSizeOfData))
    {
        PSZ pData = 0;

        // copy data
        if (ulSizeOfData == 0)
        {
            // data size == 0: this shouldn't really happen,
            // but if it does, we'll just create a NULL string.
            // Users have reported that some INI files seem to
            // contain those "empty" keys. I don't see how these
            // can exist, but they seem to...
            pData = (PSZ)malloc(1);
            *pData = 0;
        }
        else
            pData = (PSZ)malloc(ulSizeOfData);

        if (pData)
        {
            if (PrfQueryProfileData(hiniSource,
                                    pszSourceApp,
                                    pszKey,
                                    pData,
                                    &ulSizeOfData))
            {
                if (!PrfWriteProfileData(hiniTarget,
                                         pszTargetApp,
                                         pszKey,
                                         pData,
                                         ulSizeOfData))
                    ulrc = PRFERR_WRITE;
            }
            else
                ulrc = PRFERR_READ;

            free(pData);
        }
        else
            ulrc = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
        ulrc = PRFERR_DATASIZE;

    return (ulrc);
}

/*
 *@@ prfhCopyApp:
 *      this copies one key from the given profile and application
 *      to another one.
 *
 *      You can use this function in several contexts:
 *      -- copy one application within the same profile
 *         (i.e. hiniSource == hiniTarget);
 *         in this case, pszSourceApp must be != pszTargetApp;
 *      -- copy an application from one profile to another
 *         (i.e. hiniSource != hiniTarget);
 *         in this case, pszSourceApp can be == pszTargetApp
 *         (but can be different also).
 *
 *      WARNING: This does _not_ check for whether the target
 *      application exists already. This has two consequences:
 *      --  existing data will be overwritten without warning;
 *      --  if the existing target application has keys that are
 *          not in the source application, they are not deleted.
 *          As a result, you might end up with more keys than
 *          in the source application.
 *
 *      So you should delete the target application before
 *      calling this function, like this:
 +          PrfWriteProfileString(hiniTarget, pszTargetApp, NULL, NULL);
 *
 *      You must specify all parameters. You cannot specify pszApp
 *      as NULL to have a whole profile copied. Use prfhCopyProfile
 *      for that.
 *      No check is made for this.
 *
 *      Returns:
 *      --  0: no error
 *      --  PRFERR_KEYSLIST: couldn't query keys for pszSourceApp
 *      --  PRFERR_DATASIZE: couldn't query data size for key
 *      --  PRFERR_MEMORY: couldn't allocate memory
 *      --  PRFERR_READ: couldn't read data from source (PrfQueryProfileData error)
 *      --  PRFERR_WRITE: couldn't write data to target (PrfWriteProfileData error)
 *
 *@@added V0.9.0 [umoeller]
 */

ULONG prfhCopyApp(HINI hiniSource,   // in: source profile (can be HINI_USER or HINI_SYSTEM)
                  PSZ pszSourceApp,  // in: source application
                  HINI hiniTarget,   // in: target profile (can be HINI_USER or HINI_SYSTEM)
                  PSZ pszTargetApp,  // in: name of pszSourceApp in hiniTarget
                  PSZ pszErrorKey)   // out: failing key in case of error; ptr can be NULL
{
    ULONG   ulrc;
    PSZ pszKeysList;

    if (pszErrorKey)
        *pszErrorKey = 0;

    pszKeysList = prfhQueryKeysForApp(hiniSource, pszSourceApp);
    if (pszKeysList)
    {
        PSZ pKey2 = pszKeysList;

        while (*pKey2 != 0)
        {
            // copy this key
            ulrc = prfhCopyKey(hiniSource,
                               pszSourceApp,
                               pKey2,
                               hiniTarget,
                               pszTargetApp);
            if (ulrc)
            {
                // error: copy failing key to buffer
                if (pszErrorKey)
                    strcpy(pszErrorKey, pKey2);
                break;
            }
            pKey2 += strlen(pKey2)+1;
        } // end while (*pKey2 != 0)

        free (pszKeysList);
    }
    else
        ulrc = PRFERR_KEYSLIST;

    return (ulrc);
}

/*
 *@@ prfhSetUserProfile:
 *      calls PrfReset to change the current user
 *      profile (normally OS2.INI) to the specified
 *      INI file.
 *
 *@@added V0.9.4 (2000-07-19) [umoeller]
 */

BOOL prfhSetUserProfile(HAB hab,
                        PSZ pszUserProfile)     // in: new user profile (.INI)
{
    BOOL    brc = FALSE;
    // find out current profile names
    PRFPROFILE Profiles;
    Profiles.cchUserName = Profiles.cchSysName = 0;
    // first query their file name lengths
    if (PrfQueryProfile(hab, &Profiles))
    {
        // allocate memory for filenames
        Profiles.pszUserName  = (PSZ)malloc(Profiles.cchUserName);
        Profiles.pszSysName  = (PSZ)malloc(Profiles.cchSysName);

        if ((Profiles.pszSysName) && (Profiles.pszUserName))
        {
            // get filenames
            if (PrfQueryProfile(hab, &Profiles))
            {
                _Pmpf(("Old user profile: %s", Profiles.pszUserName));

                // change INIs
                free(Profiles.pszUserName);
                Profiles.pszUserName = pszUserProfile;
                Profiles.cchUserName = strlen(pszUserProfile) + 1;
                brc = PrfReset(hab, &Profiles);
                free(Profiles.pszSysName);
            }
        }
    }

    return (brc);
}


