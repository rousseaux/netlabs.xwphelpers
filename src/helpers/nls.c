
/*
 *@@sourcefile nls.c:
 *      contains a few helpers for National Language Support (NLS),
 *      such as printing strings with the format specified by
 *      the "Country" object.
 *
 *      Usage: All OS/2 programs.
 *
 *      Function prefixes (new with V0.81):
 *      --  nls*        NLS helpers
 *
 *      This file is new with 0.9.16, but contains functions
 *      formerly in stringh.c.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@header "helpers\nls.h"
 *@@added V0.9.16 (2001-10-11) [umoeller]
 */

/*
 *      Copyright (C) 1997-2001 Ulrich M”ller.
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

#define INCL_DOSNLS
#define INCL_DOSERRORS
#define INCL_WINSHELLDATA
#include <os2.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "setup.h"                      // code generation and debugging options

#include "helpers\prfh.h"
#include "helpers\nls.h"

#pragma hdrstop

/*
 *@@category: Helpers\C helpers\National Language Support
 *      See stringh.c and xstring.c.
 */

/*
 *@@ nlsQueryCountrySettings:
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
 *      --  cDecimal = '.' (decimal separator).
 *      --  cThousands = ',' (thousands separator).
 *
 *@@added V0.9.0 [umoeller]
 *@@changed V0.9.7 (2000-12-02) [umoeller]: added cDecimal
 */

VOID nlsQueryCountrySettings(PCOUNTRYSETTINGS pcs)
{
    if (pcs)
    {
        pcs->ulDateFormat = PrfQueryProfileInt(HINI_USER,
                                               (PSZ)PMINIAPP_NATIONAL,
                                               "iDate",
                                               0);
        pcs->ulTimeFormat = PrfQueryProfileInt(HINI_USER,
                                               (PSZ)PMINIAPP_NATIONAL,
                                               "iTime",
                                               0);
        pcs->cDateSep = prfhQueryProfileChar(HINI_USER,
                                             (PSZ)PMINIAPP_NATIONAL,
                                             "sDate",
                                             '/');
        pcs->cTimeSep = prfhQueryProfileChar(HINI_USER,
                                             (PSZ)PMINIAPP_NATIONAL,
                                             "sTime",
                                             ':');
        pcs->cDecimal = prfhQueryProfileChar(HINI_USER,
                                             (PSZ)PMINIAPP_NATIONAL,
                                             "sDecimal",
                                             '.');
        pcs->cThousands = prfhQueryProfileChar(HINI_USER,
                                               (PSZ)PMINIAPP_NATIONAL,
                                               "sThousand",
                                               ',');
    }
}

/*
 *@@ nlsThousandsULong:
 *      converts a ULONG into a decimal string, while
 *      inserting thousands separators into it. Specify
 *      the separator character in cThousands.
 *
 *      Returns pszTarget so you can use it directly
 *      with sprintf and the "%s" flag.
 *
 *      For cThousands, you should use the data in
 *      OS2.INI ("PM_National" application), which is
 *      always set according to the "Country" object.
 *      You can use nlsQueryCountrySettings to
 *      retrieve this setting.
 *
 *      Use nlsThousandsDouble for "double" values.
 */

PSZ nlsThousandsULong(PSZ pszTarget,       // out: decimal as string
                      ULONG ul,            // in: decimal to convert
                      CHAR cThousands)     // in: separator char (e.g. '.')
{
    USHORT ust, uss, usc;
    CHAR   szTemp[40];
    sprintf(szTemp, "%lu", ul);

    ust = 0;
    usc = strlen(szTemp);
    for (uss = 0; uss < usc; uss++)
    {
        if (uss)
            if (((usc - uss) % 3) == 0)
            {
                pszTarget[ust] = cThousands;
                ust++;
            }
        pszTarget[ust] = szTemp[uss];
        ust++;
    }
    pszTarget[ust] = '\0';

    return (pszTarget);
}

/*
 * strhThousandsULong:
 *      wrapper around nlsThousandsULong for those
 *      who used the XFLDR.DLL export.
 *
 *added V0.9.16 (2001-10-11) [umoeller]
 */

PSZ APIENTRY strhThousandsULong(PSZ pszTarget,       // out: decimal as string
                                ULONG ul,            // in: decimal to convert
                                CHAR cThousands)     // in: separator char (e.g. '.')
{
    return (nlsThousandsULong(pszTarget, ul, cThousands));
}

/*
 *@@ nlsThousandsDouble:
 *      like nlsThousandsULong, but for a "double"
 *      value. Note that after-comma values are truncated.
 */

PSZ nlsThousandsDouble(PSZ pszTarget,
                       double dbl,
                       CHAR cThousands)
{
    USHORT ust, uss, usc;
    CHAR   szTemp[40];
    sprintf(szTemp, "%.0f", floor(dbl));

    ust = 0;
    usc = strlen(szTemp);
    for (uss = 0; uss < usc; uss++)
    {
        if (uss)
            if (((usc - uss) % 3) == 0)
            {
                pszTarget[ust] = cThousands;
                ust++;
            }
        pszTarget[ust] = szTemp[uss];
        ust++;
    }
    pszTarget[ust] = '\0';

    return (pszTarget);
}

/*
 *@@ nlsVariableDouble:
 *      like nlsThousandsULong, but for a "double" value, and
 *      with a variable number of decimal places depending on the
 *      size of the quantity.
 *
 *@@added V0.9.6 (2000-11-12) [pr]
 */

PSZ nlsVariableDouble(PSZ pszTarget,
                      double dbl,
                      PSZ pszUnits,
                      CHAR cThousands)
{
    if (dbl < 100.0)
        sprintf(pszTarget, "%.2f%s", dbl, pszUnits);
    else
        if (dbl < 1000.0)
            sprintf(pszTarget, "%.1f%s", dbl, pszUnits);
        else
            strcat(nlsThousandsDouble(pszTarget, dbl, cThousands),
                   pszUnits);

    return(pszTarget);
}

/*
 *@@ nlsFileDate:
 *      converts file date data to a string (to pszBuf).
 *      You can pass any FDATE structure to this function,
 *      which are returned in those FILEFINDBUF* or
 *      FILESTATUS* structs by the Dos* functions.
 *
 *      ulDateFormat is the PM setting for the date format,
 *      as set in the "Country" object, and can be queried using
 +              PrfQueryProfileInt(HINI_USER, "PM_National", "iDate", 0);
 *
 *      meaning:
 *      --  0   mm.dd.yyyy  (English)
 *      --  1   dd.mm.yyyy  (e.g. German)
 *      --  2   yyyy.mm.dd  (Japanese, ISO)
 *      --  3   yyyy.dd.mm
 *
 *      cDateSep is used as a date separator (e.g. '.').
 *      This can be queried using:
 +          prfhQueryProfileChar(HINI_USER, "PM_National", "sDate", '/');
 *
 *      Alternatively, you can query all the country settings
 *      at once using nlsQueryCountrySettings (prfh.c).
 *
 *@@changed V0.9.0 (99-11-07) [umoeller]: now calling nlsDateTime
 */

VOID nlsFileDate(PSZ pszBuf,           // out: string returned
                 FDATE *pfDate,        // in: date information
                 ULONG ulDateFormat,   // in: date format (0-3)
                 CHAR cDateSep)        // in: date separator (e.g. '.')
{
    DATETIME dt;
    dt.day = pfDate->day;
    dt.month = pfDate->month;
    dt.year = pfDate->year + 1980;

    nlsDateTime(pszBuf,
                 NULL,          // no time
                 &dt,
                 ulDateFormat,
                 cDateSep,
                 0, 0);         // no time
}

/*
 *@@ nlsFileTime:
 *      converts file time data to a string (to pszBuf).
 *      You can pass any FTIME structure to this function,
 *      which are returned in those FILEFINDBUF* or
 *      FILESTATUS* structs by the Dos* functions.
 *
 *      ulTimeFormat is the PM setting for the time format,
 *      as set in the "Country" object, and can be queried using
 +              PrfQueryProfileInt(HINI_USER, "PM_National", "iTime", 0);
 *      meaning:
 *      --  0   12-hour clock
 *      --  >0  24-hour clock
 *
 *      cDateSep is used as a time separator (e.g. ':').
 *      This can be queried using:
 +              prfhQueryProfileChar(HINI_USER, "PM_National", "sTime", ':');
 *
 *      Alternatively, you can query all the country settings
 *      at once using nlsQueryCountrySettings (prfh.c).
 *
 *@@changed V0.8.5 (99-03-15) [umoeller]: fixed 12-hour crash
 *@@changed V0.9.0 (99-11-07) [umoeller]: now calling nlsDateTime
 */

VOID nlsFileTime(PSZ pszBuf,           // out: string returned
                 FTIME *pfTime,        // in: time information
                 ULONG ulTimeFormat,   // in: 24-hour time format (0 or 1)
                 CHAR cTimeSep)        // in: time separator (e.g. ':')
{
    DATETIME dt;
    dt.hours = pfTime->hours;
    dt.minutes = pfTime->minutes;
    dt.seconds = pfTime->twosecs * 2;

    nlsDateTime(NULL,          // no date
                 pszBuf,
                 &dt,
                 0, 0,          // no date
                 ulTimeFormat,
                 cTimeSep);
}

/*
 *@@ nlsDateTime:
 *      converts Control Program DATETIME info
 *      into two strings. See nlsFileDate and nlsFileTime
 *      for more detailed parameter descriptions.
 *
 *@@added V0.9.0 (99-11-07) [umoeller]
 *@@changed V0.9.16 (2001-12-05) [pr]: fixed AM/PM hour bug
 */

VOID nlsDateTime(PSZ pszDate,          // out: date string returned (can be NULL)
                 PSZ pszTime,          // out: time string returned (can be NULL)
                 DATETIME *pDateTime,  // in: date/time information
                 ULONG ulDateFormat,   // in: date format (0-3); see nlsFileDate
                 CHAR cDateSep,        // in: date separator (e.g. '.')
                 ULONG ulTimeFormat,   // in: 24-hour time format (0 or 1); see nlsFileTime
                 CHAR cTimeSep)        // in: time separator (e.g. ':')
{
    if (pszDate)
    {
        switch (ulDateFormat)
        {
            case 0:  // mm.dd.yyyy  (English)
                sprintf(pszDate, "%02d%c%02d%c%04d",
                        pDateTime->month,
                            cDateSep,
                        pDateTime->day,
                            cDateSep,
                        pDateTime->year);
            break;

            case 1:  // dd.mm.yyyy  (e.g. German)
                sprintf(pszDate, "%02d%c%02d%c%04d",
                        pDateTime->day,
                            cDateSep,
                        pDateTime->month,
                            cDateSep,
                        pDateTime->year);
            break;

            case 2: // yyyy.mm.dd  (Japanese)
                sprintf(pszDate, "%04d%c%02d%c%02d",
                        pDateTime->year,
                            cDateSep,
                        pDateTime->month,
                            cDateSep,
                        pDateTime->day);
            break;

            default: // yyyy.dd.mm
                sprintf(pszDate, "%04d%c%02d%c%02d",
                        pDateTime->year,
                            cDateSep,
                        pDateTime->day,
                            cDateSep,
                        pDateTime->month);
            break;
        }
    }

    if (pszTime)
    {
        if (ulTimeFormat == 0)
        {
            // for 12-hour clock, we need additional INI data
            CHAR szAMPM[10] = "err";

            if (pDateTime->hours >= 12)  // V0.9.16 (2001-12-05) [pr] if (pDateTime->hours > 12)
            {
                // >= 12h: PM.
                PrfQueryProfileString(HINI_USER,
                                      "PM_National",
                                      "s2359",        // key
                                      "PM",           // default
                                      szAMPM, sizeof(szAMPM)-1);
                sprintf(pszTime, "%02d%c%02d%c%02d %s",
                        // leave 12 == 12 (not 0)
                        pDateTime->hours % 12,
                            cTimeSep,
                        pDateTime->minutes,
                            cTimeSep,
                        pDateTime->seconds,
                        szAMPM);
            }
            else
            {
                // < 12h: AM
                PrfQueryProfileString(HINI_USER,
                                      "PM_National",
                                      "s1159",        // key
                                      "AM",           // default
                                      szAMPM, sizeof(szAMPM)-1);
                sprintf(pszTime, "%02d%c%02d%c%02d %s",
                        pDateTime->hours,
                            cTimeSep,
                        pDateTime->minutes,
                            cTimeSep,
                        pDateTime->seconds,
                        szAMPM);
            }
        }
        else
            // 24-hour clock
            sprintf(pszTime, "%02d%c%02d%c%02d",
                    pDateTime->hours,
                        cTimeSep,
                    pDateTime->minutes,
                        cTimeSep,
                    pDateTime->seconds);
    }
}

/*
 * strhDateTime:
 *      wrapper around nlsDateTime for those who used
 *      the XFLDR.DLL export.
 */

VOID APIENTRY strhDateTime(PSZ pszDate,          // out: date string returned (can be NULL)
                           PSZ pszTime,          // out: time string returned (can be NULL)
                           DATETIME *pDateTime,  // in: date/time information
                           ULONG ulDateFormat,   // in: date format (0-3); see nlsFileDate
                           CHAR cDateSep,        // in: date separator (e.g. '.')
                           ULONG ulTimeFormat,   // in: 24-hour time format (0 or 1); see nlsFileTime
                           CHAR cTimeSep)        // in: time separator (e.g. ':')
{
    nlsDateTime(pszDate,
                pszTime,
                pDateTime,
                ulDateFormat,
                cDateSep,
                ulTimeFormat,
                cTimeSep);
}


/*
 *@@ nlsUpper:
 *      quick hack for upper-casing a string.
 *
 *      This uses DosMapCase with the default system country
 *      code and the process's codepage. WARNING: DosMapCase
 *      is a 16-bit API and therefore quite slow. Use this
 *      with care.
 *
 *@@added V0.9.16 (2001-10-25) [umoeller]
 */

APIRET nlsUpper(PSZ psz,            // in/out: string
                ULONG ulLength)     // in: string length; if 0, we run strlen(psz)
{
    COUNTRYCODE cc;

    if (psz)
    {
        if (!ulLength)
            ulLength = strlen(psz);

        if (ulLength)
        {
            cc.country = 0;         // use system country code
            cc.codepage = 0;        // use process default codepage
            return (DosMapCase(ulLength,
                               &cc,
                               psz));
        }
    }

    return (ERROR_INVALID_PARAMETER);
}
