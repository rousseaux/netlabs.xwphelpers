
/*
 *@@sourcefile stringh.c:
 *      contains string/text helper functions. These are good for
 *      parsing/splitting strings and other stuff used throughout XWorkplace.
 *
 *      Usage: All OS/2 programs.
 *
 *      Function prefixes (new with V0.81):
 *      --  strh*       string helper functions.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@header "helpers\stringh.h"
 */

/*
 *      Copyright (C) 1997-2000 Ulrich Mîller.
 *      Parts Copyright (C) 1991-1999 iMatix Corporation.
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

#define INCL_WINSHELLDATA
#include <os2.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "setup.h"                      // code generation and debugging options

#include "helpers\stringh.h"
#include "helpers\xstring.h"            // extended string helpers

#pragma hdrstop

/*
 *@@category: Helpers\C helpers\String management
 */

/*
 *@@ strhdup:
 *      like strdup, but this one
 *      doesn't crash if pszSource is NULL,
 *      but returns NULL also.
 *
 *@@added V0.9.0 [umoeller]
 */

PSZ strhdup(const char *pszSource)
{
    if (pszSource)
        return (strdup(pszSource));
    else
        return (0);
}

/*
 *@@ strhistr:
 *      like strstr, but case-insensitive.
 *
 *@@changed V0.9.0 [umoeller]: crashed if null pointers were passed, thanks RÅdiger Ihle
 */

PSZ strhistr(const char *string1, const char *string2)
{
    PSZ prc = NULL;

    if ((string1) && (string2))
    {
        PSZ pszSrchIn = strdup(string1);
        PSZ pszSrchFor = strdup(string2);

        if ((pszSrchIn) && (pszSrchFor))
        {
            strupr(pszSrchIn);
            strupr(pszSrchFor);

            prc = strstr(pszSrchIn, pszSrchFor);
            if (prc)
            {
                // prc now has the first occurence of the string,
                // but in pszSrchIn; we need to map this
                // return value to the original string
                prc = (prc-pszSrchIn) // offset in pszSrchIn
                      + (PSZ)string1;
            }
        }
        if (pszSrchFor)
            free(pszSrchFor);
        if (pszSrchIn)
            free(pszSrchIn);
    }
    return (prc);
}

/*
 *@@ strhncpy0:
 *      like strncpy, but always appends a 0 character.
 */

ULONG strhncpy0(PSZ pszTarget,
                const char *pszSource,
                ULONG cbSource)
{
    ULONG ul = 0;
    PSZ pTarget = pszTarget,
        pSource = (PSZ)pszSource;

    for (ul = 0; ul < cbSource; ul++)
        if (*pSource)
            *pTarget++ = *pSource++;
        else
            break;
    *pTarget = 0;

    return (ul);
}

/*
 * strhCount:
 *      this counts the occurences of c in pszSearch.
 */

ULONG strhCount(const char *pszSearch,
                CHAR c)
{
    PSZ         p = (PSZ)pszSearch;
    ULONG       ulCount = 0;
    while (TRUE)
    {
        p = strchr(p, c);
        if (p)
        {
            ulCount++;
            p++;
        }
        else
            break;
    }
    return (ulCount);
}

/*
 *@@ strhIsDecimal:
 *      returns TRUE if psz consists of decimal digits only.
 */

BOOL strhIsDecimal(PSZ psz)
{
    PSZ p = psz;
    while (*p != 0)
    {
        if (isdigit(*p) == 0)
            return (FALSE);
        p++;
    }

    return (TRUE);
}

/*
 *@@ strhSubstr:
 *      this creates a new PSZ containing the string
 *      from pBegin to pEnd, excluding the pEnd character.
 *      The new string is null-terminated.
 *
 *      Example:
 +              "1234567890"
 +                ^      ^
 +                p1     p2
 +          strhSubstr(p1, p2)
 *      would return a new string containing "2345678".
 */

PSZ strhSubstr(const char *pBegin, const char *pEnd)
{
    ULONG cbSubstr = (pEnd - pBegin);
    PSZ pszSubstr = (PSZ)malloc(cbSubstr + 1);
    strhncpy0(pszSubstr, pBegin, cbSubstr);
    return (pszSubstr);
}

/*
 *@@ strhExtract:
 *      searches pszBuf for the cOpen character and returns
 *      the data in between cOpen and cClose, excluding
 *      those two characters, in a newly allocated buffer
 *      which you must free() afterwards.
 *
 *      Spaces and newlines/linefeeds are skipped.
 *
 *      If the search was successful, the new buffer
 *      is returned and, if (ppEnd != NULL), *ppEnd points
 *      to the first character after the cClose character
 *      found in the buffer.
 *
 *      If the search was not successful, NULL is
 *      returned, and *ppEnd is unchanged.
 *
 *      If another cOpen character is found before
 *      cClose, matching cClose characters will be skipped.
 *      You can therefore nest the cOpen and cClose
 *      characters.
 *
 *      This function ignores cOpen and cClose characters
 *      in C-style comments and strings surrounded by
 *      double quotes.
 *
 *      Example:
 +          PSZ pszBuf = "KEYWORD { --blah-- } next",
 +              pEnd;
 +          strhExtract(pszBuf,
 +                      '{', '}',
 +                      &pEnd)
 *      would return a new buffer containing " --blah-- ",
 *      and ppEnd would afterwards point to the space
 *      before "next" in the static buffer.
 *
 *@@added V0.9.0 [umoeller]
 */

PSZ strhExtract(PSZ pszBuf,     // in: search buffer
                CHAR cOpen,     // in: opening char
                CHAR cClose,    // in: closing char
                PSZ *ppEnd)     // out: if != NULL, receives first character after closing char
{
    PSZ pszReturn = NULL;

    if (pszBuf)
    {
        PSZ pOpen = strchr(pszBuf, cOpen);
        if (pOpen)
        {
            // opening char found:
            // now go thru the whole rest of the buffer
            PSZ     p = pOpen+1;
            LONG    lLevel = 1;        // if this goes 0, we're done
            while (*p)
            {
                if (*p == cOpen)
                    lLevel++;
                else if (*p == cClose)
                {
                    lLevel--;
                    if (lLevel <= 0)
                    {
                        // matching closing bracket found:
                        // extract string
                        pszReturn = strhSubstr(pOpen+1,  // after cOpen
                                               p);          // excluding cClose
                        if (ppEnd)
                            *ppEnd = p+1;
                        break;      // while (*p)
                    }
                }
                else if (*p == '\"')
                {
                    // beginning of string:
                    PSZ p2 = p+1;
                    // find end of string
                    while ((*p2) && (*p2 != '\"'))
                        p2++;

                    if (*p2 == '\"')
                        // closing quote found:
                        // search on after that
                        p = p2;     // raised below
                    else
                        break;      // while (*p)
                }

                p++;
            }
        }
    }

    return (pszReturn);
}

/*
 *@@ strhQuote:
 *      similar to strhExtract, except that
 *      opening and closing chars are the same,
 *      and therefore no nesting is possible.
 *      Useful for extracting stuff between
 *      quotes.
 *
 *@@added V0.9.0 [umoeller]
 */

PSZ strhQuote(PSZ pszBuf,
              CHAR cQuote,
              PSZ *ppEnd)
{
    PSZ pszReturn = NULL,
        p1 = NULL;
    if ((p1 = strchr(pszBuf, cQuote)))
    {
        PSZ p2 = strchr(p1+1, cQuote);
        if (p2)
        {
            pszReturn = strhSubstr(p1+1, p2);
            if (ppEnd)
                // store closing char
                *ppEnd = p2 + 1;
        }
    }

    return (pszReturn);
}

/*
 *@@ strhStrip:
 *      removes all double spaces.
 *      This copies within the "psz" buffer.
 *      If any double spaces are found, the
 *      string will be shorter than before,
 *      but the buffer is _not_ reallocated,
 *      so there will be unused bytes at the
 *      end.
 *
 *      Returns the number of spaces removed.
 *
 *@@added V0.9.0 [umoeller]
 */

ULONG strhStrip(PSZ psz)         // in/out: string
{
    PSZ     p;
    ULONG   cb = strlen(psz),
            ulrc = 0;

    for (p = psz; p < psz+cb; p++)
    {
        if ((*p == ' ') && (*(p+1) == ' '))
        {
            PSZ p2 = p;
            while (*p2)
            {
                *p2 = *(p2+1);
                p2++;
            }
            cb--;
            p--;
            ulrc++;
        }
    }
    return (ulrc);
}

/*
 * strhWords:
 *      returns the no. of words in "psz".
 *      A string is considered a "word" if
 *      it is surrounded by spaces only.
 *
 *@@added V0.9.0 [umoeller]
 */

ULONG strhWords(PSZ psz)
{
    PSZ p;
    ULONG cb = strlen(psz),
          ulWords = 0;
    if (cb > 1)
    {
        ulWords = 1;
        for (p = psz; p < psz+cb; p++)
            if (*p == ' ')
                ulWords++;
    }
    return (ulWords);
}

/*
 *@@ strhThousandsULong:
 *      converts a ULONG into a decimal string, while
 *      inserting thousands separators into it. Specify
 *      the separator char in cThousands.
 *      Returns pszTarget so you can use it directly
 *      with sprintf and the "%s" flag.
 *      For cThousands, you should use the data in
 *      OS2.INI ("PM_National" application), which is
 *      always set according to the "Country" object.
 *      Use strhThousandsDouble for "double" values.
 */

PSZ strhThousandsULong(PSZ pszTarget,       // out: decimal as string
                       ULONG ul,            // in: decimal to convert
                       CHAR cThousands)     // in: separator char (e.g. '.')
{
    USHORT ust, uss, usc;
    CHAR   szTemp[40];
    sprintf(szTemp, "%d", ul);

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
 *@@ strhThousandsDouble:
 *      like strhThousandsULong, but for a "double"
 *      value. Note that after-comma values are truncated.
 */

PSZ strhThousandsDouble(PSZ pszTarget, double dbl, CHAR cThousands)
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
 *@@ strhFileDate:
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
 +              prfhQueryProfileChar(HINI_USER, "PM_National", "sDate", '/');
 *
 *      Alternatively, you can query all the country settings
 *      at once using prfhQueryCountrySettings (prfh.c, new with V0.9.0).
 *
 *@@changed (99-11-07) [umoeller]: now calling strhDateTime
 */

VOID strhFileDate(PSZ pszBuf,           // out: string returned
                  FDATE *pfDate,        // in: date information
                  ULONG ulDateFormat,   // in: date format (0-3)
                  CHAR cDateSep)        // in: date separator (e.g. '.')
{
    DATETIME dt;
    dt.day = pfDate->day;
    dt.month = pfDate->month;
    dt.year = pfDate->year + 1980;

    strhDateTime(pszBuf,
                 NULL,          // no time
                 &dt,
                 ulDateFormat,
                 cDateSep,
                 0, 0);         // no time
}

/*
 *@@ strhFileTime:
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
 *      at once using prfhQueryCountrySettings (prfh.c, new with V0.9.0).
 *
 *@@changed 99-03-15 fixed 12-hour crash
 *@@changed (99-11-07) [umoeller]: now calling strhDateTime
 */

VOID strhFileTime(PSZ pszBuf,           // out: string returned
                  FTIME *pfTime,        // in: time information
                  ULONG ulTimeFormat,   // in: 24-hour time format (0 or 1)
                  CHAR cTimeSep)        // in: time separator (e.g. ':')
{
    DATETIME dt;
    dt.hours = pfTime->hours;
    dt.minutes = pfTime->minutes;
    dt.seconds = pfTime->twosecs * 2;

    strhDateTime(NULL,          // no date
                 pszBuf,
                 &dt,
                 0, 0,          // no date
                 ulTimeFormat,
                 cTimeSep);
}

/*
 *@@ strhDateTime:
 *      converts Control Programe DATETIME info
 *      to two strings. See strhFileDate and strhFileTime
 *      for more detailed parameter descriptions.
 *
 *@@added V0.9.0 (99-11-07) [umoeller]
 */

VOID strhDateTime(PSZ pszDate,          // out: date string returned (can be NULL)
                  PSZ pszTime,          // out: time string returned (can be NULL)
                  DATETIME *pDateTime,  // in: date/time information
                  ULONG ulDateFormat,   // in: date format (0-3); see strhFileDate
                  CHAR cDateSep,        // in: date separator (e.g. '.')
                  ULONG ulTimeFormat,   // in: 24-hour time format (0 or 1); see strhFileTime
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

            if (pDateTime->hours > 12)
            {
                // > 12h: PM.

                // Note: 12:xx noon is 12 AM, not PM (even though
                // AM stands for "ante meridiam", but English is just
                // not logical), so that's handled below.

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
                // <= 12h: AM
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
 *@@ strhGetWord:
 *      finds word boundaries.
 *
 *      *ppszStart is used as the beginning of the
 *      search.
 *
 *      If a word is found, *ppszStart is set to
 *      the first character of the word which was
 *      found and *ppszEnd receives the address
 *      of the first character _after_ the word,
 *      which is probably a space or a \n or \r char.
 *      We then return TRUE.
 *
 *      The search is stopped if a null character
 *      is found or pLimit is reached. In that case,
 *      FALSE is returned.
 *
 *@@added V0.9.1 (2000-02-13) [umoeller]
 */

BOOL strhGetWord(PSZ *ppszStart,        // in: start of search,
                                        // out: start of word (if TRUE is returned)
                 const char *pLimit,    // in: ptr to last char after *ppszStart to be
                                        // searched; if the word does not end before
                                        // or with this char, FALSE is returned
                 const char *pcszBeginChars, // stringh.h defines STRH_BEGIN_CHARS
                 const char *pcszEndChars, // stringh.h defines STRH_END_CHARS
                 PSZ *ppszEnd)          // out: first char _after_ word
                                        // (if TRUE is returned)
{
    // characters after which a word can be started
    // const char *pcszBeginChars = "\x0d\x0a ";
    // const char *pcszEndChars = "\x0d\x0a /-";

    PSZ pStart = *ppszStart;

    // find start of word
    while (     (pStart < (PSZ)pLimit)
             && (strchr(pcszBeginChars, *pStart))
          )
        // if char is a "before word" char: go for next
        pStart++;

    if (pStart < (PSZ)pLimit)
    {
        // found a valid "word start" character
        // (which is not in pcszBeginChars):

        // find end of word
        PSZ  pEndOfWord = pStart;
        while (     (pEndOfWord <= (PSZ)pLimit)
                 && (strchr(pcszEndChars, *pEndOfWord) == 0)
              )
            // if char is not an "end word" char: go for next
            pEndOfWord++;

        if (pEndOfWord <= (PSZ)pLimit)
        {
            // whoa, got a word:
            *ppszStart = pStart;
            *ppszEnd = pEndOfWord;
            return (TRUE);
        }
    }

    return (FALSE);
}

/*
 *@@ strhFindWord:
 *      searches for pszSearch in pszBuf, which is
 *      returned if found (or NULL if not).
 *
 *      As opposed to strstr, this finds pszSearch
 *      only if it is a "word". A search string is
 *      considered a word if the character _before_
 *      it is in pcszBeginChars and the char _after_
 *      it is in pcszEndChars.
 *
 *      Example:
 +          strhFindWord("This is an example.", "is");
 +          returns ...........^ this, but not the "is" in "This".
 *
 *      The algorithm here uses strstr to find pszSearch in pszBuf
 *      and performs additional "is-word" checks for each item found.
 *      With VAC++ 3.0, this is still much faster than searching
 *      words first and then comparing each word with pszSearch.
 *      I've tried it that way too, and that took nearly double as
 *      long. Apparently, the VAC++ runtime library uses some
 *      optimized search algorithm here, so we better use that one.
 *
 *@@added V0.9.0 (99-11-08) [umoeller]
 *@@changed (99-11-10) [umoeller]: tried second algorithm, reverted to original...
 */

PSZ strhFindWord(const char *pszBuf,
                 const char *pszSearch,
                 const char *pcszBeginChars,    // suggestion: "\x0d\x0a ()/\\-,."
                 const char *pcszEndChars)      // suggestion: "\x0d\x0a ()/\\-,.:;"
{
    PSZ     pszReturn = 0;
    ULONG   cbBuf = strlen(pszBuf),
            cbSearch = strlen(pszSearch);

    if ((cbBuf) && (cbSearch))
    {
        const char *p = pszBuf;

        /*  // go thru all characters
        while (*p)
        {
            // check if current character is either the
            // very first or a "begin word" character
            if (    (p == pszBuf)
                 || (strchr(pcszBeginChars, *p) == 0)
               )
            {
                // yes: go for next
                if (*(++p))
                {
                    // compare with search string
                    if (strcmp(p, pszSearch) == 0)
                    {
                        // is the same:
                        // check if still in buffer
                        if (p < pszBuf + cbBuf)
                        {
                            CHAR    cAfterEndOfWord = *(p + cbSearch);
                            if (cAfterEndOfWord == 0)
                            {
                                // end of string:
                                // that's ok
                                pszReturn = (PSZ)p;
                                break;
                            }
                            else
                            {
                                // check if in "end of word" list
                                char *pc2 = strchr(pcszEndChars, cAfterEndOfWord);
                                if (pc2)
                                    // OK, is end char: avoid doubles of that char,
                                    // but allow spaces
                                    if (    (cAfterEndOfWord+1 != *pc2)
                                         || (cAfterEndOfWord+1 == ' ')
                                         || (cAfterEndOfWord+1 == 0)
                                       )
                                    {
                                        // end of string:
                                        // that's ok
                                        pszReturn = (PSZ)p;
                                        break;
                                    }
                            }
                        }
                    }
                }
                else
                    // end of string:
                    break;
            }

            ++p;
        } // end while
        */

        do  // while p
        {
            p = strstr(p, pszSearch);
            if (p)
            {
                // string found:
                // check if that's a word

                // check previous char
                if (    (p == pszBuf)
                     || (strchr(pcszBeginChars, *(p-1)))
                   )
                {
                    // OK, valid begin char:
                    BOOL    fEndOK = FALSE;
                    // check end char
                    CHAR    cNextChar = *(p + cbSearch);
                    if (cNextChar == 0)
                        fEndOK = TRUE;
                    else
                    {
                        char *pc = strchr(pcszEndChars, cNextChar);
                        if (pc)
                            // OK, is end char: avoid doubles of that char,
                            // but allow spaces
                            if (    (cNextChar+1 != *pc)
                                 || (cNextChar+1 == ' ')
                                 || (cNextChar+1 == 0)
                               )
                                fEndOK = TRUE;
                    }

                    if (fEndOK)
                    {
                        // valid end char:
                        pszReturn = (PSZ)p;
                        break;
                    }
                }
                p += cbSearch;
            }
        } while (p);

    }
    return (pszReturn);
}

/*
 *@@ strhFindEOL:
 *      returns a pointer to the next \r, \n or null character
 *      following pszSearchIn. Stores the offset in *pulOffset.
 *
 *      This should never return NULL because at some point,
 *      there will be a null byte in your string.
 *
 *@@added V0.9.4 (2000-07-01) [umoeller]
 */

PSZ strhFindEOL(PSZ pszSearchIn,        // in: where to search
                PULONG pulOffset)       // out: offset (ptr can be NULL)
{
    PSZ     p = pszSearchIn,
            prc = NULL;
    while (TRUE)
    {
        if ( (*p == '\r') || (*p == '\n') || (*p == 0) )
        {
            prc = p;
            break;
        }
        p++;
    }

    if (pulOffset)
        *pulOffset = prc - pszSearchIn;
    return (prc);
}

/*
 *@@ strhFindNextLine:
 *      like strhFindEOL, but this returns the character
 *      _after_ \r or \n. Note that this might return
 *      a pointer to terminating NULL character also.
 */

PSZ strhFindNextLine(PSZ pszSearchIn, PULONG pulOffset)
{
    PSZ pEOL = strhFindEOL(pszSearchIn, NULL);
    // pEOL now points to the \r char or the terminating 0 byte;
    // if not null byte, advance pointer
    PSZ pNextLine = pEOL;
    if (*pNextLine == '\r')
        pNextLine++;
    if (*pNextLine == '\n')
        pNextLine++;
    if (pulOffset)
        *pulOffset = pNextLine - pszSearchIn;
    return (pNextLine);
}

/*
 *@@ strhFindKey:
 *      finds pszKey in pszSearchIn; similar to strhistr,
 *      but this one makes sure the key is at the beginning
 *      of a line. Spaces before the key are tolerated.
 *      Returns NULL if the key was not found.
 *      Used by strhGetParameter/strhSetParameter; useful
 *      for analyzing CONFIG.SYS settings.
 *
 *@@changed V0.9.0 [umoeller]: fixed bug in that this would also return something if only the first chars matched
 *@@changed V0.9.0 [umoeller]: fixed bug which could cause character before pszSearchIn to be examined
 */

PSZ strhFindKey(PSZ pszSearchIn,     // in: text buffer to search
                PSZ pszKey,          // in: key to search for
                PBOOL pfIsAllUpperCase) // out: TRUE if key is completely in upper case;
                                     // can be NULL if not needed
{
    PSZ     p = NULL,
            pReturn = NULL;
    // BOOL    fFound = FALSE;

    p = pszSearchIn;
    do {
        p = strhistr(p, pszKey);

        if ((p) && (p >= pszSearchIn))
        {
            // make sure the key is at the beginning of a line
            // by going backwards until we find a char != " "
            PSZ p2 = p;
            while (     (*p2 == ' ')
                     && (p2 > pszSearchIn)
                  )
                p2--;

            // if previous char is an EOL sign, go on
            if (    (p2 == pszSearchIn)     // order fixed V0.9.0, RÅdiger Ihle
                 || (*(p2-1) == '\r')
                 || (*(p2-1) == '\n')
               )
            {
                // now check whether the char after the search
                // is a "=" char
                // ULONG cbKey = strlen(pszKey);

                // tolerate spaces before "="
                /* PSZ p3 = p;
                while (*(p3+cbKey) == ' ')
                    p3++;

                if (*(p3+cbKey) == '=') */
                {
                    // found:
                    pReturn = p; // go on, p contains found key

                    // test for all upper case?
                    if (pfIsAllUpperCase)
                    {
                        ULONG cbKey2 = strlen(pszKey),
                              ul = 0;
                        *pfIsAllUpperCase = TRUE;
                        for (ul = 0; ul < cbKey2; ul++)
                            if (islower(*(p+ul)))
                            {
                                *pfIsAllUpperCase = FALSE;
                                break; // for
                            }
                    }

                    break; // do
                }
            } // else search next key

            p++; // search on after this key
        }
    } while ((!pReturn) && (p != NULL) && (p != pszSearchIn));

    return (pReturn);
}

/*
 *@@ strhGetParameter:
 *      searches pszSearchIn for the key pszKey; if found, it
 *      returns a pointer to the following characters in pszSearchIn
 *      and, if pszCopyTo != NULL, copies the rest of the line to
 *      that buffer, of which cbCopyTo specified the size.
 *      If the key is not found, NULL is returned.
 *      String search is done by calling strhFindKey.
 *      This is useful for querying CONFIG.SYS settings.
 *
 *      <B>Example:</B> this would return "YES" if you searched
 *      for "PAUSEONERROR=", and "PAUSEONERROR=YES" existed in pszSearchIn.
 */

PSZ strhGetParameter(PSZ pszSearchIn,   // in: text buffer to search
                     PSZ pszKey,        // in: key to search for
                     PSZ pszCopyTo,     // out: key value
                     ULONG cbCopyTo)    // out: sizeof(*pszCopyTo)
{
    PSZ p = strhFindKey(pszSearchIn, pszKey, NULL),
        prc = NULL;
    if (p)
    {
        prc = p + strlen(pszKey);
        if (pszCopyTo)
        // copy to pszCopyTo
        {
            ULONG cb;
            PSZ pEOL = strhFindEOL(prc, &cb);
            if (pEOL)
            {
                if (cb > cbCopyTo)
                    cb = cbCopyTo-1;
                strhncpy0(pszCopyTo, prc, cb);
            }
        }
    }

    return (prc);
}

/*
 *@@ strhSetParameter:
 *      searches *ppszSearchIn for the key pszKey; if found, it
 *      replaces the characters following this key up to the
 *      end of the line with pszParam. If pszKey is not found in
 *      pszSearchIn, it is appended to the file in a new line.
 *
 *      If any changes are made, *ppszSearchIn is re-allocated.
 *
 *      This function searches w/out case sensitivity.
 *
 *      Returns a pointer to the new parameter inside the buffer.
 *
 *@@changed V0.9.0 [umoeller]: changed function prototype to PSZ* ppszSearchIn
 */

PSZ strhSetParameter(PSZ* ppszSearchIn,    // in: text buffer to search
                     PSZ pszKey,         // in: key to search for
                     PSZ pszNewParam,    // in: new parameter to set for key
                     BOOL fRespectCase)  // in: if TRUE, pszNewParam will
                             // be converted to upper case if the found key is
                             // in upper case also. pszNewParam should be in
                             // lower case if you use this.
{
    BOOL fIsAllUpperCase = FALSE;
    PSZ pKey = strhFindKey(*ppszSearchIn, pszKey, &fIsAllUpperCase),
        prc = NULL;

    if (pKey)
    {
        // key found in file:
        // replace existing parameter
        PSZ pOldParam = pKey + strlen(pszKey);
        prc = pOldParam;
        // pOldParam now has the old parameter, which we
        // will overwrite now

        if (pOldParam)
        {
            ULONG cbOldParam;
            PSZ pEOL = strhFindEOL(pOldParam, &cbOldParam);
            // pEOL now has first end-of-line after the parameter

            if (pEOL)
            {
                PSZ pszOldCopy = (PSZ)malloc(cbOldParam+1);
                strncpy(pszOldCopy, pOldParam, cbOldParam);
                pszOldCopy[cbOldParam] = '\0';

                // check for upper case desired?
                if (fRespectCase)
                    if (fIsAllUpperCase)
                        strupr(pszNewParam);

                xstrrpl(ppszSearchIn, 0, pszOldCopy, pszNewParam, NULL);

                free(pszOldCopy);
            }
        }
    }
    else
    {
        PSZ pszNew = (PSZ)malloc(strlen(*ppszSearchIn)
                              + strlen(pszKey)
                              + strlen(pszNewParam)
                              + 5);     // 2 * \r\n + null byte
        // key not found: append to end of file
        sprintf(pszNew, "%s\r\n%s%s\r\n",
                *ppszSearchIn, pszKey, pszNewParam);
        free(*ppszSearchIn);
        *ppszSearchIn = pszNew;
    }

    return (prc);
}

/*
 *@@ strhDeleteLine:
 *      this deletes the line in pszSearchIn which starts with
 *      the key pszKey. Returns TRUE if the line was found and
 *      deleted.
 *
 *      This copies within pszSearchIn.
 */

BOOL strhDeleteLine(PSZ pszSearchIn,        // in: buffer to search
                    PSZ pszKey)             // in: key to find
{
    BOOL fIsAllUpperCase = FALSE;
    PSZ pKey = strhFindKey(pszSearchIn, pszKey, &fIsAllUpperCase);
    BOOL brc = FALSE;

    if (pKey) {
        PSZ pEOL = strhFindEOL(pKey, NULL);
        // pEOL now has first end-of-line after the key
        if (pEOL)
        {
            // delete line by overwriting it with
            // the next line
            strcpy(pKey, pEOL+2);
        }
        else
        {
            // EOL not found: we must be at the end of the file
            *pKey = '\0';
        }
        brc = TRUE;
    }

    return (brc);
}

/*
 *@@ strhBeautifyTitle:
 *      replaces all line breaks (0xd, 0xa) with spaces.
 */

BOOL strhBeautifyTitle(PSZ psz)
{
    BOOL rc = FALSE;
    CHAR *p;
    while ((p = strchr(psz, 0xa)))
    {
        *p = ' ';
        rc = TRUE;
    }
    while ((p = strchr(psz, 0xd)))
    {
        *p = ' ';
        rc = TRUE;
    }
    return (rc);
}

/*
 * strhFindAttribValue:
 *      searches for pszAttrib in pszSearchIn; if found,
 *      returns the first character after the "=" char.
 *      If "=" is not found, a space, \r, and \n are
 *      also accepted. This function searches without
 *      respecting case.
 *
 *      <B>Example:</B>
 +          strhFindAttribValue("<PAGE BLAH="data">, "BLAH")
 +
 +          returns ....................... ^ this address.
 *
 *@@added V0.9.0 [umoeller]
 *@@changed V0.9.3 (2000-05-19) [umoeller]: some speed optimizations
 */

PSZ strhFindAttribValue(const char *pszSearchIn, const char *pszAttrib)
{
    PSZ     prc = 0;
    PSZ     pszSearchIn2 = (PSZ)pszSearchIn,
            p,
            p2;
    ULONG   cbAttrib = strlen(pszAttrib);

    // 1) find space char
    while ((p = strchr(pszSearchIn2, ' ')))
    {
        CHAR c;
        p++;
        c = *(p+cbAttrib);     // V0.9.3 (2000-05-19) [umoeller]
        // now check whether the p+strlen(pszAttrib)
        // is a valid end-of-tag character
        if (    (memicmp(p, (PVOID)pszAttrib, cbAttrib) == 0)
             && (   (c == ' ')
                 || (c == '>')
                 || (c == '=')
                 || (c == '\r')
                 || (c == '\n')
                 || (c == 0)
                )
           )
        {
            // yes:
            CHAR c2;
            p2 = p + cbAttrib;
            c2 = *p2;
            while (     (   (c2 == ' ')
                         || (c2 == '=')
                         || (c2 == '\n')
                         || (c2 == '\r')
                        )
                    &&  (c2 != 0)
                  )
                c2 = *++p2;
            prc = p2;
            break; // first while
        }
        pszSearchIn2++;
    }
    return (prc);
}

/*
 * strhGetNumAttribValue:
 *      stores the numerical parameter value of an HTML-style
 *      tag in *pl.
 *
 *      Returns the address of the tag parameter in the
 *      search buffer, if found, or NULL.
 *
 *      <B>Example:</B>
 +          strhGetNumAttribValue("<PAGE BLAH=123>, "BLAH", &l);
 *
 *      stores 123 in the "l" variable.
 *
 *@@added V0.9.0 [umoeller]
 */

PSZ strhGetNumAttribValue(const char *pszSearchIn,       // in: where to search
                          const char *pszTag,            // e.g. "INDEX"
                          PLONG pl)              // out: numerical value
{
    PSZ pParam;
    if ((pParam = strhFindAttribValue(pszSearchIn, pszTag)))
        sscanf(pParam, "%d", pl);

    return (pParam);
}

/*
 * strhGetTextAttr:
 *      retrieves the attribute value of a textual HTML-style tag
 *      in a newly allocated buffer, which is returned,
 *      or NULL if attribute not found.
 *      If an attribute value is to contain spaces, it
 *      must be enclosed in quotes.
 *
 *      The offset of the attribute data in pszSearchIn is
 *      returned in *pulOffset so that you can do multiple
 *      searches.
 *
 *      This returns a new buffer, which should be free()'d after use.
 *
 *      <B>Example:</B>
 +          ULONG   ulOfs = 0;
 +          strhGetTextAttr("<PAGE BLAH="blublub">, "BLAH", &ulOfs)
 +                           ............^ ulOfs
 *
 *      returns a new string with the value "blublub" (without
 *      quotes) and sets ulOfs to 12.
 *
 *@@added V0.9.0 [umoeller]
 */

PSZ strhGetTextAttr(const char *pszSearchIn,
                    const char *pszTag,
                    PULONG pulOffset)       // out: offset where found
{
    PSZ     pParam,
            pParam2,
            prc = NULL;
    ULONG   ulCount = 0;
    LONG    lNestingLevel = 0;

    if ((pParam = strhFindAttribValue(pszSearchIn, pszTag)))
    {
        // determine end character to search for: a space
        CHAR cEnd = ' ';
        if (*pParam == '\"')
        {
            // or, if the data is enclosed in quotes, a quote
            cEnd = '\"';
            pParam++;
        }

        if (pulOffset)
            // store the offset
            (*pulOffset) = pParam - (PSZ)pszSearchIn;

        // now find end of attribute
        pParam2 = pParam;
        while (*pParam)
        {
            if (*pParam == cEnd)
                // end character found
                break;
            else if (*pParam == '<')
                // yet another opening tag found:
                // this is probably some "<" in the attributes
                lNestingLevel++;
            else if (*pParam == '>')
            {
                lNestingLevel--;
                if (lNestingLevel < 0)
                    // end of tag found:
                    break;
            }
            ulCount++;
            pParam++;
        }

        // copy attribute to new buffer
        if (ulCount)
        {
            prc = (PSZ)malloc(ulCount+1);
            memcpy(prc, pParam2, ulCount);
            *(prc+ulCount) = 0;
        }
    }
    return (prc);
}

/*
 * strhFindEndOfTag:
 *      returns a pointer to the ">" char
 *      which seems to terminate the tag beginning
 *      after pszBeginOfTag.
 *
 *      If additional "<" chars are found, we look
 *      for additional ">" characters too.
 *
 *      Note: You must pass the address of the opening
 *      '<' character to this function.
 *
 *      Example:
 +          PSZ pszTest = "<BODY ATTR=\"<BODY>\">";
 +          strhFindEndOfTag(pszTest)
 +      returns.................................^ this.
 *
 *@@added V0.9.0 [umoeller]
 */

PSZ strhFindEndOfTag(const char *pszBeginOfTag)
{
    PSZ     p = (PSZ)pszBeginOfTag,
            prc = NULL;
    LONG    lNestingLevel = 0;

    while (*p)
    {
        if (*p == '<')
            // another opening tag found:
            lNestingLevel++;
        else if (*p == '>')
        {
            // closing tag found:
            lNestingLevel--;
            if (lNestingLevel < 1)
            {
                // corresponding: return this
                prc = p;
                break;
            }
        }
        p++;
    }

    return (prc);
}

/*
 * strhGetBlock:
 *      this complex function searches the given string
 *      for a pair of opening/closing HTML-style tags.
 *
 *      If found, this routine returns TRUE and does
 *      the following:
 *
 *          1)  allocate a new buffer, copy the text
 *              enclosed by the opening/closing tags
 *              into it and set *ppszBlock to that
 *              buffer;
 *
 *          2)  if the opening tag has any attributes,
 *              allocate another buffer, copy the
 *              attributes into it and set *ppszAttrs
 *              to that buffer; if no attributes are
 *              found, *ppszAttrs will be NULL;
 *
 *          3)  set *pulOffset to the offset from the
 *              beginning of *ppszSearchIn where the
 *              opening tag was found;
 *
 *          4)  advance *ppszSearchIn to after the
 *              closing tag, so that you can do
 *              multiple searches without finding the
 *              same tags twice.
 *
 *      All buffers should be freed using free().
 *
 *      This returns the following:
 *      --  0: no error
 *      --  1: tag not found at all (doesn't have to be an error)
 *      --  2: begin tag found, but no corresponding end tag found. This
 *             is a real error.
 *      --  3: begin tag is not terminated by "&gt;" (e.g. "&lt;BEGINTAG whatever")
 *
 *      <B>Example:</B>
 +          PSZ pSearch = "&lt;PAGE INDEX=1&gt;This is page 1.&lt;/PAGE&gt;More text."
 +          PSZ pszBlock, pszAttrs;
 +          ULONG ulOfs;
 +          strhGetBlock(&pSearch, "PAGE", &pszBlock, &pszAttrs, &ulOfs)
 *
 *      would do the following:
 *
 *      1)  set pszBlock to a new string containing "This is page 1."
 *          without quotes;
 *
 *      2)  set pszAttrs to a new string containing "&lt;PAGE INDEX=1&gt;";
 *
 *      3)  set ulOfs to 0, because "&lt;PAGE" was found at the beginning;
 *
 *      4)  pSearch would be advanced to point to the "More text"
 *          string in the original buffer.
 *
 *      Hey-hey. A one-shot function, fairly complicated, but indispensable
 *      for HTML parsing.
 *
 *@@added V0.9.0 [umoeller]
 *@@changed V0.9.1 (2000-01-03) [umoeller]: fixed heap overwrites (thanks to string debugging)
 *@@changed V0.9.1 (2000-01-06) [umoeller]: changed prototype
 *@@changed V0.9.3 (2000-05-06) [umoeller]: NULL string check was missing
 */

ULONG strhGetBlock(const char *pszSearchIn, // in: buffer to search
                   PULONG pulSearchOffset, // in/out: offset where to start search (0 for beginning)
                   PSZ pszTag,
                   PSZ *ppszBlock,      // out: block enclosed by the tags
                   PSZ *ppszAttribs,    // out: attributes of the opening tag
                   PULONG pulOfsBeginTag, // out: offset from pszSearchIn where opening tag was found
                   PULONG pulOfsBeginBlock) // out: offset from pszSearchIn where beginning of block was found
{
    ULONG   ulrc = 1;
    PSZ     pszBeginTag = (PSZ)pszSearchIn + *pulSearchOffset,
            pszSearch2 = pszBeginTag,
            pszClosingTag;
    ULONG   cbTag = strlen(pszTag);

    // go thru the block and check all tags if it's the
    // begin tag we're looking for
    while ((pszBeginTag = strchr(pszBeginTag, '<')))
    {
        if (memicmp(pszBeginTag+1, pszTag, strlen(pszTag)) == 0)
            // yes: stop
            break;
        else
            pszBeginTag++;
    }

    if (pszBeginTag)
    {
        // we found <TAG>:
        ULONG   ulNestingLevel = 0;

        PSZ     pszEndOfBeginTag = strhFindEndOfTag(pszBeginTag);
                                    // strchr(pszBeginTag, '>');
        if (pszEndOfBeginTag)
        {
            // does the caller want the attributes?
            if (ppszAttribs)
            {
                // yes: then copy them
                ULONG   ulAttrLen = pszEndOfBeginTag - pszBeginTag;
                PSZ     pszAttrs = (PSZ)malloc(ulAttrLen + 1);
                strncpy(pszAttrs, pszBeginTag, ulAttrLen);
                // add terminating 0
                *(pszAttrs + ulAttrLen) = 0;

                *ppszAttribs = pszAttrs;
            }

            // output offset of where we found the begin tag
            if (pulOfsBeginTag)
                *pulOfsBeginTag = pszBeginTag - (PSZ)pszSearchIn;

            // now find corresponding closing tag (e.g. "</BODY>"
            pszBeginTag = pszEndOfBeginTag+1;
            // now we're behind the '>' char of the opening tag
            // increase offset of that too
            if (pulOfsBeginBlock)
                *pulOfsBeginBlock = pszBeginTag - (PSZ)pszSearchIn;

            // find next closing tag;
            // for the first run, pszSearch2 points to right
            // after the '>' char of the opening tag
            pszSearch2 = pszBeginTag;
            while (     (pszSearch2)        // fixed V0.9.3 (2000-05-06) [umoeller]
                    &&  (pszClosingTag = strstr(pszSearch2, "<"))
                  )
            {
                // if we have another opening tag before our closing
                // tag, we need to have several closing tags before
                // we're done
                if (memicmp(pszClosingTag+1, pszTag, cbTag) == 0)
                    ulNestingLevel++;
                else
                {
                    // is this ours?
                    if (    (*(pszClosingTag+1) == '/')
                         && (memicmp(pszClosingTag+2, pszTag, cbTag) == 0)
                       )
                    {
                        // we've found a matching closing tag; is
                        // it ours?
                        if (ulNestingLevel == 0)
                        {
                            // our closing tag found:
                            // allocate mem for a new buffer
                            // and extract all the text between
                            // open and closing tags to it
                            ULONG ulLen = pszClosingTag - pszBeginTag;
                            if (ppszBlock)
                            {
                                PSZ pNew = (PSZ)malloc(ulLen + 1);
                                strhncpy0(pNew, pszBeginTag, ulLen);
                                *ppszBlock = pNew;
                            }

                            // raise search offset to after the closing tag
                            *pulSearchOffset = (pszClosingTag + cbTag + 1) - (PSZ)pszSearchIn;

                            ulrc = 0;

                            break;
                        } else
                            // not our closing tag:
                            ulNestingLevel--;
                    }
                }
                // no matching closing tag: search on after that
                pszSearch2 = strhFindEndOfTag(pszClosingTag);
            } // end while (pszClosingTag = strstr(pszSearch2, "<"))

            if (!pszClosingTag)
                // no matching closing tag found:
                // return 2 (closing tag not found)
                ulrc = 2;
        } // end if (pszBeginTag)
        else
            // no matching ">" for opening tag found:
            ulrc = 3;
    }

    return (ulrc);
}

/* ******************************************************************
 *                                                                  *
 *   Miscellaneous                                                  *
 *                                                                  *
 ********************************************************************/

/*
 *@@ strhArrayAppend:
 *      this appends a string to a "string array".
 *
 *      A string array is considered a sequence of
 *      zero-terminated strings in memory. That is,
 *      after each string's null-byte, the next
 *      string comes up.
 *
 *      This is useful for composing a single block
 *      of memory from, say, list box entries, which
 *      can then be written to OS2.INI in one flush.
 *
 *      To append strings to such an array, call this
 *      function for each string you wish to append.
 *      This will re-allocate *ppszRoot with each call,
 *      and update *pcbRoot, which then contains the
 *      total size of all strings (including all null
 *      terminators).
 *
 *      Pass *pcbRoot to PrfSaveProfileData to have the
 *      block saved.
 *
 *      Note: On the first call, *ppszRoot and *pcbRoot
 *      _must_ be both NULL, or this crashes.
 */

VOID strhArrayAppend(PSZ *ppszRoot,         // in: root of array
                     PSZ pszNew,            // in: string to append
                     PULONG pcbRoot)        // in/out: size of array
{
    ULONG cbNew = strlen(pszNew);
    PSZ pszTemp = (PSZ)malloc(*pcbRoot
                              + cbNew
                              + 1);    // two null bytes
    if (*ppszRoot)
    {
        // not first loop: copy old stuff
        memcpy(pszTemp,
               *ppszRoot,
               *pcbRoot);
        free(*ppszRoot);
    }
    // append new string
    strcpy(pszTemp + *pcbRoot,
           pszNew);
    // update root
    *ppszRoot = pszTemp;
    // update length
    *pcbRoot += cbNew + 1;
}

/*
 *@@ strhCreateDump:
 *      this dumps a memory block into a string
 *      and returns that string in a new buffer.
 *
 *      You must free() the returned PSZ after use.
 *
 *      The output looks like the following:
 *
 +          0000:  FE FF 0E 02 90 00 00 00   ........
 +          0008:  FD 01 00 00 57 50 46 6F   ....WPFo
 +          0010:  6C 64 65 72 00 78 01 34   lder.x.4
 *
 *      Each line is terminated with a newline (\n)
 *      character only.
 *
 *@@added V0.9.1 (2000-01-22) [umoeller]
 */

PSZ strhCreateDump(PBYTE pb,            // in: start address of buffer
                   ULONG ulSize,        // in: size of buffer
                   ULONG ulIndent)      // in: indentation of every line
{
    PSZ     pszReturn = NULL;
    CHAR    szTemp[1000];

    PBYTE   pbCurrent = pb;                 // current byte
    ULONG   ulCount = 0,
            ulCharsInLine = 0;              // if this grows > 7, a new line is started
    CHAR    szLine[400] = "",
            szAscii[30] = "         ";      // ASCII representation; filled for every line
    PSZ     pszLine = szLine,
            pszAscii = szAscii;

    for (pbCurrent = pb;
         ulCount < ulSize;
         pbCurrent++, ulCount++)
    {
        if (ulCharsInLine == 0)
        {
            memset(szLine, ' ', ulIndent);
            pszLine += ulIndent;
        }
        pszLine += sprintf(pszLine, "%02lX ", *pbCurrent);

        if ( (*pbCurrent > 31) && (*pbCurrent < 127) )
            // printable character:
            *pszAscii = *pbCurrent;
        else
            *pszAscii = '.';
        pszAscii++;

        ulCharsInLine++;
        if (    (ulCharsInLine > 7)         // 8 bytes added?
             || (ulCount == ulSize-1)       // end of buffer reached?
           )
        {
            // if we haven't had eight bytes yet,
            // fill buffer up to eight bytes with spaces
            ULONG   ul2;
            for (ul2 = ulCharsInLine;
                 ul2 < 8;
                 ul2++)
                pszLine += sprintf(pszLine, "   ");

            sprintf(szTemp, "%04lX:  %s  %s\n",
                            (ulCount & 0xFFFFFFF8),  // offset in hex
                            szLine,         // bytes string
                            szAscii);       // ASCII string
            xstrcat(&pszReturn, szTemp);

            // restart line buffer
            pszLine = szLine;

            // clear ASCII buffer
            strcpy(szAscii, "         ");
            pszAscii = szAscii;

            // reset line counter
            ulCharsInLine = 0;
        }
    }

    return (pszReturn);
}

/* ******************************************************************
 *                                                                  *
 *   Wildcard matching                                              *
 *                                                                  *
 ********************************************************************/

/*
 *  The following code has been taken from "fnmatch.zip".
 *
 *      (c) 1994-1996 by Eberhard Mattes.
 */

/* In OS/2 and DOS styles, both / and \ separate components of a path.
 * This macro returns true iff C is a separator. */

#define IS_OS2_COMP_SEP(C)  ((C) == '/' || (C) == '\\')


/* This macro returns true if C is at the end of a component of a
 * path. */

#define IS_OS2_COMP_END(C)  ((C) == 0 || IS_OS2_COMP_SEP (C))

/*
 *@@ skip_comp_os2:
 *      Return a pointer to the next component of the path SRC, for OS/2
 *      and DOS styles.  When the end of the string is reached, a pointer
 *      to the terminating null character is returned.
 *
 *      (c) 1994-1996 by Eberhard Mattes.
 */

static const unsigned char* skip_comp_os2(const unsigned char *src)
{
    /* Skip characters until hitting a separator or the end of the
     * string. */

    while (!IS_OS2_COMP_END(*src))
        ++src;

    /* Skip the separator if we hit a separator. */

    if (*src != 0)
        ++src;
    return src;
}

/*
 * has_colon:
 *      returns true iff the path P contains a colon.
 *
 *      (c) 1994-1996 by Eberhard Mattes.
 */

static int has_colon(const unsigned char *p)
{
    while (*p != 0)
        if (*p == ':')
            return 1;
        else
            ++p;
    return 0;
}

/*
 * match_comp_os2:
 *      Compare a single component (directory name or file name) of the
 *      paths, for OS/2 and DOS styles.  MASK and NAME point into a
 *      component of the wildcard and the name to be checked, respectively.
 *      Comparing stops at the next separator.  The FLAGS argument is the
 *      same as that of fnmatch().  HAS_DOT is true if a dot is in the
 *      current component of NAME.  The number of dots is not restricted,
 *      even in DOS style.  Return FNM_MATCH iff MASK and NAME match.
 *      Note that this function is recursive.
 *
 *      (c) 1994-1996 by Eberhard Mattes.
 */

static int match_comp_os2(const unsigned char *mask,
                          const unsigned char *name,
                          unsigned flags,
                          int has_dot)
{
    int             rc;

    for (;;)
        switch (*mask)
        {
            case 0:

                /* There must be no extra characters at the end of NAME when
                 * reaching the end of MASK unless _FNM_PATHPREFIX is set:
                 * in that case, NAME may point to a separator. */

                if (*name == 0)
                    return FNM_MATCH;
                if ((flags & _FNM_PATHPREFIX) && IS_OS2_COMP_SEP(*name))
                    return FNM_MATCH;
                return FNM_NOMATCH;

            case '/':
            case '\\':

                /* Separators match separators. */

                if (IS_OS2_COMP_SEP(*name))
                    return FNM_MATCH;

                /* If _FNM_PATHPREFIX is set, a trailing separator in MASK
                 * is ignored at the end of NAME. */

                if ((flags & _FNM_PATHPREFIX) && mask[1] == 0 && *name == 0)
                    return FNM_MATCH;

                /* Stop comparing at the separator. */

                return FNM_NOMATCH;

            case '?':

                /* A question mark matches one character.  It does not match
                 * a dot.  At the end of the component (and before a dot),
                 * it also matches zero characters. */

                if (*name != '.' && !IS_OS2_COMP_END(*name))
                    ++name;
                ++mask;
                break;

            case '*':

                /* An asterisk matches zero or more characters.  In DOS
                 * mode, dots are not matched. */

                do
                {
                    ++mask;
                }
                while (*mask == '*');
                for (;;)
                {
                    rc = match_comp_os2(mask, name, flags, has_dot);
                    if (rc != FNM_NOMATCH)
                        return rc;
                    if (IS_OS2_COMP_END(*name))
                        return FNM_NOMATCH;
                    if (*name == '.' && (flags & _FNM_STYLE_MASK) == _FNM_DOS)
                        return FNM_NOMATCH;
                    ++name;
                }

            case '.':

                /* A dot matches a dot.  It also matches the implicit dot at
                 * the end of a dot-less NAME. */

                ++mask;
                if (*name == '.')
                    ++name;
                else if (has_dot || !IS_OS2_COMP_END(*name))
                    return FNM_NOMATCH;
                break;

            default:

                /* All other characters match themselves. */

                if (flags & _FNM_IGNORECASE)
                {
                    if (tolower(*mask) != tolower(*name))
                        return FNM_NOMATCH;
                }
                else
                {
                    if (*mask != *name)
                        return FNM_NOMATCH;
                }
                ++mask;
                ++name;
                break;
        }
}

/*
 * match_comp:
 *      compare a single component (directory name or file name) of the
 *      paths, for all styles which need component-by-component matching.
 *      MASK and NAME point to the start of a component of the wildcard and
 *      the name to be checked, respectively.  Comparing stops at the next
 *      separator.  The FLAGS argument is the same as that of fnmatch().
 *      Return FNM_MATCH iff MASK and NAME match.
 *
 *      (c) 1994-1996 by Eberhard Mattes.
 */

static int match_comp(const unsigned char *mask,
                      const unsigned char *name,
                      unsigned flags)
{
    const unsigned char *s;

    switch (flags & _FNM_STYLE_MASK)
    {
        case _FNM_OS2:
        case _FNM_DOS:

            /* For OS/2 and DOS styles, we add an implicit dot at the end of
             * the component if the component doesn't include a dot. */

            s = name;
            while (!IS_OS2_COMP_END(*s) && *s != '.')
                ++s;
            return match_comp_os2(mask, name, flags, *s == '.');

        default:
            return FNM_ERR;
    }
}

/* In Unix styles, / separates components of a path.  This macro
 * returns true iff C is a separator. */

#define IS_UNIX_COMP_SEP(C)  ((C) == '/')


/* This macro returns true if C is at the end of a component of a
 * path. */

#define IS_UNIX_COMP_END(C)  ((C) == 0 || IS_UNIX_COMP_SEP (C))

/*
 * match_unix:
 *      match complete paths for Unix styles.  The FLAGS argument is the
 *      same as that of fnmatch().  COMP points to the start of the current
 *      component in NAME.  Return FNM_MATCH iff MASK and NAME match.  The
 *      backslash character is used for escaping ? and * unless
 *      FNM_NOESCAPE is set.
 *
 *      (c) 1994-1996 by Eberhard Mattes.
 */

static int match_unix(const unsigned char *mask,
                      const unsigned char *name,
                      unsigned flags,
                      const unsigned char *comp)
{
    unsigned char   c1, c2;
    char            invert, matched;
    const unsigned char *start;
    int             rc;

    for (;;)
        switch (*mask)
        {
            case 0:

                /* There must be no extra characters at the end of NAME when
                 * reaching the end of MASK unless _FNM_PATHPREFIX is set:
                 * in that case, NAME may point to a separator. */

                if (*name == 0)
                    return FNM_MATCH;
                if ((flags & _FNM_PATHPREFIX) && IS_UNIX_COMP_SEP(*name))
                    return FNM_MATCH;
                return FNM_NOMATCH;

            case '?':

                /* A question mark matches one character.  It does not match
                 * the component separator if FNM_PATHNAME is set.  It does
                 * not match a dot at the start of a component if FNM_PERIOD
                 * is set. */

                if (*name == 0)
                    return FNM_NOMATCH;
                if ((flags & FNM_PATHNAME) && IS_UNIX_COMP_SEP(*name))
                    return FNM_NOMATCH;
                if (*name == '.' && (flags & FNM_PERIOD) && name == comp)
                    return FNM_NOMATCH;
                ++mask;
                ++name;
                break;

            case '*':

                /* An asterisk matches zero or more characters.  It does not
                 * match the component separator if FNM_PATHNAME is set.  It
                 * does not match a dot at the start of a component if
                 * FNM_PERIOD is set. */

                if (*name == '.' && (flags & FNM_PERIOD) && name == comp)
                    return FNM_NOMATCH;
                do
                {
                    ++mask;
                }
                while (*mask == '*');
                for (;;)
                {
                    rc = match_unix(mask, name, flags, comp);
                    if (rc != FNM_NOMATCH)
                        return rc;
                    if (*name == 0)
                        return FNM_NOMATCH;
                    if ((flags & FNM_PATHNAME) && IS_UNIX_COMP_SEP(*name))
                        return FNM_NOMATCH;
                    ++name;
                }

            case '/':

                /* Separators match only separators.  If _FNM_PATHPREFIX is
                 * set, a trailing separator in MASK is ignored at the end
                 * of NAME. */

                if (!(IS_UNIX_COMP_SEP(*name)
                      || ((flags & _FNM_PATHPREFIX) && *name == 0
                      && (mask[1] == 0
                          || (!(flags & FNM_NOESCAPE) && mask[1] == '\\'
                  && mask[2] == 0)))))
                    return FNM_NOMATCH;

                ++mask;
                if (*name != 0)
                    ++name;

                /* This is the beginning of a new component if FNM_PATHNAME
                 * is set. */

                if (flags & FNM_PATHNAME)
                    comp = name;
                break;

            case '[':

                /* A set of characters.  Always case-sensitive. */

                if (*name == 0)
                    return FNM_NOMATCH;
                if ((flags & FNM_PATHNAME) && IS_UNIX_COMP_SEP(*name))
                    return FNM_NOMATCH;
                if (*name == '.' && (flags & FNM_PERIOD) && name == comp)
                    return FNM_NOMATCH;

                invert = 0;
                matched = 0;
                ++mask;

                /* If the first character is a ! or ^, the set matches all
                 * characters not listed in the set. */

                if (*mask == '!' || *mask == '^')
                {
                    ++mask;
                    invert = 1;
                }

                /* Loop over all the characters of the set.  The loop ends
                 * if the end of the string is reached or if a ] is
                 * encountered unless it directly follows the initial [ or
                 * [-. */

                start = mask;
                while (!(*mask == 0 || (*mask == ']' && mask != start)))
                {
                    /* Get the next character which is optionally preceded
                     * by a backslash. */

                    c1 = *mask++;
                    if (!(flags & FNM_NOESCAPE) && c1 == '\\')
                    {
                        if (*mask == 0)
                            break;
                        c1 = *mask++;
                    }

                    /* Ranges of characters are written as a-z.  Don't
                     * forget to check for the end of the string and to
                     * handle the backslash.  If the character after - is a
                     * ], it isn't a range. */

                    if (*mask == '-' && mask[1] != ']')
                    {
                        ++mask; /* Skip the - character */
                        if (!(flags & FNM_NOESCAPE) && *mask == '\\')
                            ++mask;
                        if (*mask == 0)
                            break;
                        c2 = *mask++;
                    }
                    else
                        c2 = c1;

                    /* Now check whether this character or range matches NAME. */

                    if (c1 <= *name && *name <= c2)
                        matched = 1;
                }

                /* If the end of the string is reached before a ] is found,
                 * back up to the [ and compare it to NAME. */

                if (*mask == 0)
                {
                    if (*name != '[')
                        return FNM_NOMATCH;
                    ++name;
                    mask = start;
                    if (invert)
                        --mask;
                }
                else
                {
                    if (invert)
                        matched = !matched;
                    if (!matched)
                        return FNM_NOMATCH;
                    ++mask;     /* Skip the ] character */
                    if (*name != 0)
                        ++name;
                }
                break;

            case '\\':
                ++mask;
                if (flags & FNM_NOESCAPE)
                {
                    if (*name != '\\')
                        return FNM_NOMATCH;
                    ++name;
                }
                else if (*mask == '*' || *mask == '?')
                {
                    if (*mask != *name)
                        return FNM_NOMATCH;
                    ++mask;
                    ++name;
                }
                break;

            default:

                /* All other characters match themselves. */

                if (flags & _FNM_IGNORECASE)
                {
                    if (tolower(*mask) != tolower(*name))
                        return FNM_NOMATCH;
                }
                else
                {
                    if (*mask != *name)
                        return FNM_NOMATCH;
                }
                ++mask;
                ++name;
                break;
        }
}

/*
 * _fnmatch_unsigned:
 *      Check whether the path name NAME matches the wildcard MASK.
 *
 *      Return:
 *      --   0 (FNM_MATCH) if it matches,
 *      --   _FNM_NOMATCH if it doesn't,
 *      --   FNM_ERR on error.
 *
 *      The operation of this function is controlled by FLAGS.
 *      This is an internal function, with unsigned arguments.
 *
 *      (c) 1994-1996 by Eberhard Mattes.
 */

static int _fnmatch_unsigned(const unsigned char *mask,
                             const unsigned char *name,
                             unsigned flags)
{
    int             m_drive, n_drive,
                    rc;

    /* Match and skip the drive name if present. */

    m_drive = ((isalpha(mask[0]) && mask[1] == ':') ? mask[0] : -1);
    n_drive = ((isalpha(name[0]) && name[1] == ':') ? name[0] : -1);

    if (m_drive != n_drive)
    {
        if (m_drive == -1 || n_drive == -1)
            return FNM_NOMATCH;
        if (!(flags & _FNM_IGNORECASE))
            return FNM_NOMATCH;
        if (tolower(m_drive) != tolower(n_drive))
            return FNM_NOMATCH;
    }

    if (m_drive != -1)
        mask += 2;
    if (n_drive != -1)
        name += 2;

    /* Colons are not allowed in path names, except for the drive name,
     * which was skipped above. */

    if (has_colon(mask) || has_colon(name))
        return FNM_ERR;

    /* The name "\\server\path" should not be matched by mask
     * "\*\server\path".  Ditto for /. */

    switch (flags & _FNM_STYLE_MASK)
    {
        case _FNM_OS2:
        case _FNM_DOS:

            if (IS_OS2_COMP_SEP(name[0]) && IS_OS2_COMP_SEP(name[1]))
            {
                if (!(IS_OS2_COMP_SEP(mask[0]) && IS_OS2_COMP_SEP(mask[1])))
                    return FNM_NOMATCH;
                name += 2;
                mask += 2;
            }
            break;

        case _FNM_POSIX:

            if (name[0] == '/' && name[1] == '/')
            {
                int             i;

                name += 2;
                for (i = 0; i < 2; ++i)
                    if (mask[0] == '/')
                        ++mask;
                    else if (mask[0] == '\\' && mask[1] == '/')
                        mask += 2;
                    else
                        return FNM_NOMATCH;
            }

            /* In Unix styles, treating ? and * w.r.t. components is simple.
             * No need to do matching component by component. */

            return match_unix(mask, name, flags, name);
    }

    /* Now compare all the components of the path name, one by one.
     * Note that the path separator must not be enclosed in brackets. */

    while (*mask != 0 || *name != 0)
    {

        /* If _FNM_PATHPREFIX is set, the names match if the end of MASK
         * is reached even if there are components left in NAME. */

        if (*mask == 0 && (flags & _FNM_PATHPREFIX))
            return FNM_MATCH;

        /* Compare a single component of the path name. */

        rc = match_comp(mask, name, flags);
        if (rc != FNM_MATCH)
            return rc;

        /* Skip to the next component or to the end of the path name. */

        mask = skip_comp_os2(mask);
        name = skip_comp_os2(name);
    }

    /* If we reached the ends of both strings, the names match. */

    if (*mask == 0 && *name == 0)
        return FNM_MATCH;

    /* The names do not match. */

    return FNM_NOMATCH;
}

/*
 *@@ strhMatchOS2:
 *      this matches wildcards, similar to what DosEditName does.
 *      However, this does not require a file to be present, but
 *      works on strings only.
 */

BOOL strhMatchOS2(const unsigned char* pcszMask,     // in: mask (e.g. "*.txt")
                  const unsigned char* pcszName)     // in: string to check (e.g. "test.txt")
{
    return ((BOOL)(_fnmatch_unsigned(pcszMask,
                                     pcszName,
                                     _FNM_OS2 | _FNM_IGNORECASE)
                   == FNM_MATCH)
           );
}

/* ******************************************************************
 *                                                                  *
 *   Fast string searches                                           *
 *                                                                  *
 ********************************************************************/

#define ASSERT(a)

/*
 *      The following code has been taken from the "Standard
 *      Function Library", file sflfind.c, and only slightly
 *      modified to conform to the rest of this file.
 *
 *      Written:    96/04/24  iMatix SFL project team <sfl@imatix.com>
 *      Revised:    98/05/04
 *
 *      Copyright:  Copyright (c) 1991-99 iMatix Corporation.
 *
 *      The SFL Licence allows incorporating SFL code into other
 *      programs, as long as the copyright is reprinted and the
 *      code is marked as modified, so this is what we do.
 */

/*
 *@@ strhfind:
 *      searches for a pattern in a string using the Boyer-Moore-
 *      Horspool-Sunday algorithm.  The string and pattern are null-terminated
 *      strings.  Returns a pointer to the pattern if found within the string,
 *      or NULL if the pattern was not found.  If you repeatedly scan for the
 *      same pattern, use the repeat_find argument.  If this is TRUE, the
 *      function does not re-parse the pattern.  You must of course call the
 *      function with repeat_find equal to FALSE the first time.  This function
 *      is meant to handle  character data, and is most effective when you work
 *      with large strings.  To search binary data use strhmemfind().  Will not work
 *      on multibyte characters.
 *
 *      Examples:
 +      char *result;
 +
 +      result = strhfind ("abracadabra", "cad", FALSE);
 +      if (result)
 +          puts (result);
 +
 *      Taken from the "Standard Function Library", file sflfind.c.
 *      Copyright:  Copyright (c) 1991-99 iMatix Corporation.
 *      Slightly modified.
 *
 *@@added V0.9.3 (2000-05-08) [umoeller]
 */

char* strhfind (const char *string,            //  String containing data
                const char *pattern,           //  Pattern to search for
                BOOL repeat_find)              //  Same pattern as last time
{
    static size_t
        searchbuf [256];                //  Fixed search buffer

    ASSERT (string);                    //  Expect non-NULL pointers, but
    ASSERT (pattern);                   //  fall through if not debugging

    return (char *) strhmemfind_rb (string,    strlen (string),
                                pattern,   strlen (pattern),
                                searchbuf, &repeat_find);
}

/*
 *@@ strhfind_r:
 *      searches for a pattern in a string using the Boyer-Moore-
 *      Horspool-Sunday algorithm.  The string and pattern are null-terminated
 *      strings.  Returns a pointer to the pattern if found within the string,
 *      or NULL if the pattern was not found.  This function is meant to handle
 *      character data, and is most effective when you work with large strings.
 *      To search binary data use strhmemfind().  Will not work on multibyte
 *      characters.  Reentrant.
 *
 *      Examples:
 +      char *result;
 +
 +      result = strhfind_r ("abracadabra", "cad");
 +      if (result)
 +          puts (result);
 *
 *      Taken from the "Standard Function Library", file sflfind.c.
 *      Copyright:  Copyright (c) 1991-99 iMatix Corporation.
 *      Slightly modified.
 *
 *@@added V0.9.3 (2000-05-08) [umoeller]
 */

char* strhfind_r (const char *string,          //  String containing data
                  const char *pattern)         //  Pattern to search for
{
    size_t
        searchbuf [256];                //  One-time search buffer
    BOOL
        secondtime = FALSE;             //  Search buffer init needed

    ASSERT (string);                    //  Expect non-NULL pointers, but
    ASSERT (pattern);                   //  fall through if not debugging

    return (char *) strhmemfind_rb (string,    strlen (string),
                                    pattern,   strlen (pattern),
                                    searchbuf, &secondtime);
}

/*
 *@@ strhfind_rb:
 *      searches for a pattern in a string using the Boyer-Moore-
 *      Horspool-Sunday algorithm.  The string and pattern are null-terminated
 *      strings.  Returns a pointer to the pattern if found within the string,
 *      or NULL if the pattern was not found.  Supports more efficient repeat
 *      searches (for the same pattern), through a supplied search buffer.  The
 *      search buffer must be long enough to contain 256 (2**8) size_t entries.
 *      On the first call repeat_find must be set to FALSE.  After the search
 *      buffer has been initialised, repeat_find will be set to TRUE by the
 *      function, avoiding the search buffer initialisation on later calls.
 *
 *      This function is most effective when repeated searches are made for
 *      the same pattern in one or more strings.  This function is meant to
 *      handle  character data, and is most effective when you work with
 *      large strings.  To search binary data use strhmemfind().  Will not work
 *      on multibyte characters.  Reentrant.
 *
 *      Examples:
 +      char   *result;
 +      BOOL   repeat_search = FALSE;
 +      size_t searchbuf[256];
 +
 +      result = strhfind_rb ("abracadabra", "cad", searchbuf, &repeat_search);
 +      if (result)
 +        {
 +          puts (result);
 +          result = strhfind_rb ("cad/cam", "cad", searchbuf, &repeat_search);
 +          if (result)
 +              puts (result);
 +        }
 *
 *      Taken from the "Standard Function Library", file sflfind.c.
 *      Copyright:  Copyright (c) 1991-99 iMatix Corporation.
 *      Slightly modified.
 *
 *@@added V0.9.3 (2000-05-08) [umoeller]
 */

char* strhfind_rb (const char *string,         //  String containing data
                   const char *pattern,        //  Pattern to search for
                   size_t     *shift,          //  Working buffer between searches
                   BOOL       *repeat_find)    //  Flag for first/later search
{
    ASSERT (string);                    //  Expect non-NULL pointers, but
    ASSERT (pattern);                   //  fall through if not debugging
    ASSERT (shift);
    ASSERT (repeat_find);

    return (char *) strhmemfind_rb (string,    strlen (string),
                                    pattern,   strlen (pattern),
                                    shift,     repeat_find);
}

/*
 *@@ strhmemfind:
 *      searches for a pattern in a block of memory using the Boyer-
 *      Moore-Horspool-Sunday algorithm.  The block and pattern may contain any
 *      values; you must explicitly provide their lengths.  Returns a pointer to
 *      the pattern if found within the block, or NULL if the pattern was not
 *      found.  If you repeatedly scan for the same pattern, use the repeat_find
 *      argument.  If this is TRUE, the function does not re-parse the pattern.
 *      This function is meant to handle binary data.  If you need to search
 *      strings, use the strhfind_r or strhfind_rb() functions.  Non-Reentrant.
 *
 *      Taken from the "Standard Function Library", file sflfind.c.
 *      Copyright:  Copyright (c) 1991-99 iMatix Corporation.
 *      Slightly modified.
 *
 *@@added V0.9.3 (2000-05-08) [umoeller]
 */

void* strhmemfind (const void  *block,            //  Block containing data
                   size_t       block_size,       //  Size of block in bytes
                   const void  *pattern,          //  Pattern to search for
                   size_t       pattern_size,     //  Size of pattern block
                   BOOL         repeat_find)      //  Same pattern as last time
{
    static size_t
        searchbuf [256];                //  Static shared search buffer

    ASSERT (block);                     //  Expect non-NULL pointers, but
    ASSERT (pattern);                   //  full through if not debugging

    return strhmemfind_rb (block, block_size, pattern, pattern_size,
                       searchbuf, &repeat_find);
}

/*
 *@@ strhmemfind_r:
 *      searches for a pattern in a block of memory using the Boyer-
 *      Moore-Horspool-Sunday algorithm.  The block and pattern may contain any
 *      values; you must explicitly provide their lengths.  Returns a pointer to
 *      the pattern if found within the block, or NULL if the pattern was not
 *      found.
 *
 *      This function is meant to handle binary data, for a single search for
 *      a given pattern.  If you need to search strings, use the strhfind_r()
 *      or strhfind_rb() functions.  If you want to do efficient repeated searches
 *      for one pattern, use strhmemfind_rb().  Reentrant.
 *
 *      Taken from the "Standard Function Library", file sflfind.c.
 *      Copyright:  Copyright (c) 1991-99 iMatix Corporation.
 *      Slightly modified.
 *
 *@@added V0.9.3 (2000-05-08) [umoeller]
 */

void* strhmemfind_r (const void  *block,          //  Block containing data
                     size_t       block_size,     //  Size of block in bytes
                     const void  *pattern,        //  Pattern to search for
                     size_t       pattern_size)   //  Size of pattern block
{
    size_t
        searchbuf [256];                //  One-time search buffer
    BOOL
        secondtime  = FALSE;

    ASSERT (block);                     //  Expect non-NULL pointers, but
    ASSERT (pattern);                   //  full through if not debugging

    return strhmemfind_rb (block, block_size, pattern, pattern_size,
                           searchbuf, &secondtime);
}

/*
 *@@ strhmemfind_rb:
 *      searches for a pattern in a block of memory using the Boyer-
 *      Moore-Horspool-Sunday algorithm.  The block and pattern may contain any
 *      values; you must explicitly provide their lengths.  Returns a pointer to
 *      the pattern if found within the block, or NULL if the pattern was not
 *      found.  On the first search with a given pattern, *repeat_find should
 *      be FALSE.  It will be set to TRUE after the shift table is initialised,
 *      allowing the initialisation phase to be skipped on subsequent searches.
 *      shift must point to an array big enough to hold 256 (8**2) size_t values.
 *
 *      This function is meant to handle binary data, for repeated searches
 *      for the same pattern.  If you need to search strings, use the
 *      strhfind_r() or strhfind_rb() functions.  If you wish to search for a
 *      pattern only once consider using strhmemfind_r(). Reentrant.
 *
 *      Taken from the "Standard Function Library", file sflfind.c.
 *      Copyright:  Copyright (c) 1991-99 iMatix Corporation.
 *      Slightly modified.
 *
 *@@added V0.9.3 (2000-05-08) [umoeller]
 */

void* strhmemfind_rb (const void  *in_block,      //  Block containing data
                      size_t       block_size,    //  Size of block in bytes
                      const void  *in_pattern,    //  Pattern to search for
                      size_t       pattern_size,  //  Size of pattern block
                      size_t      *shift,         //  Shift table (search buffer)
                      BOOL        *repeat_find)   //  TRUE: search buffer already init
{
    size_t
        byte_nbr,                       //  Distance through block
        match_size;                     //  Size of matched part
    const unsigned char
        *match_base = NULL,             //  Base of match of pattern
        *match_ptr  = NULL,             //  Point within current match
        *limit      = NULL;             //  Last potiental match point
    const unsigned char
        *block   = (unsigned char *) in_block,   //  Concrete pointer to block data
        *pattern = (unsigned char *) in_pattern; //  Concrete pointer to search value

    ASSERT (block);                     //  Expect non-NULL pointers, but
    ASSERT (pattern);                   //  fail gracefully if not debugging
    ASSERT (shift);                     //  NULL repeat_find => is false
    if (block == NULL || pattern == NULL || shift == NULL)
        return (NULL);

    //  Pattern must be smaller or equal in size to string
    if (block_size < pattern_size)
        return (NULL);                  //  Otherwise it's not found

    if (pattern_size == 0)              //  Empty patterns match at start
        return ((void *)block);

    //  Build the shift table unless we're continuing a previous search

    //  The shift table determines how far to shift before trying to match
    //  again, if a match at this point fails.  If the byte after where the
    //  end of our pattern falls is not in our pattern, then we start to
    //  match again after that byte; otherwise we line up the last occurence
    //  of that byte in our pattern under that byte, and try match again.

    if (!repeat_find || !*repeat_find)
      {
        for (byte_nbr = 0; byte_nbr < 256; byte_nbr++)
            shift [byte_nbr] = pattern_size + 1;
        for (byte_nbr = 0; byte_nbr < pattern_size; byte_nbr++)
            shift [(unsigned char) pattern [byte_nbr]] = pattern_size - byte_nbr;

        if (repeat_find)
            *repeat_find = TRUE;
      }

    //  Search for the block, each time jumping up by the amount
    //  computed in the shift table

    limit = block + (block_size - pattern_size + 1);
    ASSERT (limit > block);

    for (match_base = block;
         match_base < limit;
         match_base += shift [*(match_base + pattern_size)])
      {
        match_ptr  = match_base;
        match_size = 0;

        //  Compare pattern until it all matches, or we find a difference
        while (*match_ptr++ == pattern [match_size++])
          {
            ASSERT (match_size <= pattern_size &&
                    match_ptr == (match_base + match_size));

            //  If we found a match, return the start address
            if (match_size >= pattern_size)
              return ((void*)(match_base));

          }
      }
    return (NULL);                      //  Found nothing
}

/*
 *@@ strhtxtfind:
 *      searches for a case-insensitive text pattern in a string
 *      using the Boyer-Moore-Horspool-Sunday algorithm.  The string and
 *      pattern are null-terminated strings.  Returns a pointer to the pattern
 *      if found within the string, or NULL if the pattern was not found.
 *      Will match strings irrespective of case.  To match exact strings, use
 *      strhfind().  Will not work on multibyte characters.
 *
 *      Examples:
 +      char *result;
 +
 +      result = strhtxtfind ("AbracaDabra", "cad");
 +      if (result)
 +          puts (result);
 +
 *      Taken from the "Standard Function Library", file sflfind.c.
 *      Copyright:  Copyright (c) 1991-99 iMatix Corporation.
 *      Slightly modified.
 *
 *@@added V0.9.3 (2000-05-08) [umoeller]
 */

char* strhtxtfind (const char *string,            //  String containing data
                   const char *pattern)           //  Pattern to search for
{
    size_t
        shift [256];                    //  Shift distance for each value
    size_t
        string_size,
        pattern_size,
        byte_nbr,                       //  Index into byte array
        match_size;                     //  Size of matched part
    const char
        *match_base = NULL,             //  Base of match of pattern
        *match_ptr  = NULL,             //  Point within current match
        *limit      = NULL;             //  Last potiental match point

    ASSERT (string);                    //  Expect non-NULL pointers, but
    ASSERT (pattern);                   //  fail gracefully if not debugging
    if (string == NULL || pattern == NULL)
        return (NULL);

    string_size  = strlen (string);
    pattern_size = strlen (pattern);

    //  Pattern must be smaller or equal in size to string
    if (string_size < pattern_size)
        return (NULL);                  //  Otherwise it cannot be found

    if (pattern_size == 0)              //  Empty string matches at start
        return (char *) string;

    //  Build the shift table

    //  The shift table determines how far to shift before trying to match
    //  again, if a match at this point fails.  If the byte after where the
    //  end of our pattern falls is not in our pattern, then we start to
    //  match again after that byte; otherwise we line up the last occurence
    //  of that byte in our pattern under that byte, and try match again.

    for (byte_nbr = 0; byte_nbr < 256; byte_nbr++)
        shift [byte_nbr] = pattern_size + 1;

    for (byte_nbr = 0; byte_nbr < pattern_size; byte_nbr++)
        shift [(unsigned char) tolower (pattern [byte_nbr])] = pattern_size - byte_nbr;

    //  Search for the string.  If we don't find a match, move up by the
    //  amount we computed in the shift table above, to find location of
    //  the next potiental match.

    limit = string + (string_size - pattern_size + 1);
    ASSERT (limit > string);

    for (match_base = string;
         match_base < limit;
         match_base += shift [(unsigned char) tolower (*(match_base + pattern_size))])
      {
        match_ptr  = match_base;
        match_size = 0;

        //  Compare pattern until it all matches, or we find a difference
        while (tolower (*match_ptr++) == tolower (pattern [match_size++]))
          {
            ASSERT (match_size <= pattern_size &&
                    match_ptr == (match_base + match_size));

            //  If we found a match, return the start address
            if (match_size >= pattern_size)
                return ((char *)(match_base));
          }
      }
    return (NULL);                      //  Found nothing
}

