
/*
 *@@sourcefile nls.h:
 *      header file for nls.c. See notes there.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@include #define INCL_DOSDATETIME
 *@@include #include <os2.h>
 *@@include #include "helpers\nls.h"
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

#if __cplusplus
extern "C" {
#endif

#ifndef NLS_HEADER_INCLUDED
    #define NLS_HEADER_INCLUDED

    /*
     *@@ COUNTRYSETTINGS:
     *      structure used for returning country settings
     *      with nlsQueryCountrySettings.
     */

    typedef struct _COUNTRYSETTINGS
    {
        ULONG   ulDateFormat,
                        // date format:
                        // --  0   mm.dd.yyyy  (English)
                        // --  1   dd.mm.yyyy  (e.g. German)
                        // --  2   yyyy.mm.dd  (Japanese)
                        // --  3   yyyy.dd.mm
                ulTimeFormat;
                        // time format:
                        // --  0   12-hour clock
                        // --  >0  24-hour clock
        CHAR    cDateSep,
                        // date separator (e.g. '/')
                cTimeSep,
                        // time separator (e.g. ':')
                cDecimal,
                        // decimal separator (e.g. '.')
                cThousands;
                        // thousands separator (e.g. ',')
    } COUNTRYSETTINGS, *PCOUNTRYSETTINGS;

    VOID nlsQueryCountrySettings(PCOUNTRYSETTINGS pcs);

    PSZ APIENTRY nlsThousandsULong(PSZ pszTarget, ULONG ul, CHAR cThousands);
    typedef PSZ APIENTRY NLSTHOUSANDSULONG(PSZ pszTarget, ULONG ul, CHAR cThousands);
    typedef NLSTHOUSANDSULONG *PNLSTHOUSANDSULONG;

    PSZ nlsThousandsDouble(PSZ pszTarget, double dbl, CHAR cThousands);

    PSZ nlsVariableDouble(PSZ pszTarget, double dbl, PSZ pszUnits,
                           CHAR cThousands);

    VOID nlsFileDate(PSZ pszBuf,
                      FDATE *pfDate,
                      ULONG ulDateFormat,
                      CHAR cDateSep);

    VOID nlsFileTime(PSZ pszBuf,
                      FTIME *pfTime,
                      ULONG ulTimeFormat,
                      CHAR cTimeSep);

    VOID APIENTRY nlsDateTime(PSZ pszDate,
                               PSZ pszTime,
                               DATETIME *pDateTime,
                               ULONG ulDateFormat,
                               CHAR cDateSep,
                               ULONG ulTimeFormat,
                               CHAR cTimeSep);
    typedef VOID APIENTRY NLSDATETIME(PSZ pszDate,
                               PSZ pszTime,
                               DATETIME *pDateTime,
                               ULONG ulDateFormat,
                               CHAR cDateSep,
                               ULONG ulTimeFormat,
                               CHAR cTimeSep);
    typedef NLSDATETIME *PNLSDATETIME;

#endif

#if __cplusplus
}
#endif

