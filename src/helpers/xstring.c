
/*
 *@@sourcefile xstring.c:
 *      string functions with memory management.
 *
 *      Usage: All OS/2 programs.
 *
 *      Function prefixes:
 *      --  xstr*       extended string functions.
 *
 *      The functions in this file used to be in stringh.c
 *      before V0.9.3 (2000-04-01).
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@added V0.9.3 (2000-04-01) [umoeller]
 *@@header "helpers\xstring.h"
 */

/*
 *      Copyright (C) 1999-2000 Ulrich M”ller.
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

#include <os2.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "setup.h"                      // code generation and debugging options

#include "helpers\stringh.h"
#include "helpers\xstring.h"            // extended string helpers

/*
 *@@category: Helpers\C helpers\String management
 */

/*
 *@@ xstrcpy:
 *      copies pszString to *ppszBuf, for which memory is allocated
 *      as necessary.
 *
 *      If *ppszBuf != NULL, the existing memory is freed.
 *
 *      Returns the length of the new string (including the null
 *      terminator), or null upon errors.
 *
 *      Example:
 +          PSZ psz = NULL;
 +          xstrcpy(&psz, "blah");
 *      would have "psz" point to newly allocated buffer containing
 *      "blah".
 *
 *@@changed V0.9.2 (2000-04-01) [umoeller]: renamed from strhxcpy
 */

ULONG xstrcpy(XSTR ppszBuf,
              const char *pszString)
{
    ULONG   ulrc = 0;
    if (ppszBuf)
    {
        if (*ppszBuf)
            free(*ppszBuf);
        ulrc = strlen(pszString) + 1;
        *ppszBuf = (PSZ)malloc(ulrc);
        strcpy(*ppszBuf, pszString);
    }
    return (ulrc);
}

#ifdef __XWPMEMDEBUG__ // setup.h, helpers\memdebug.c

/*
 *@@ xstrcatDebug:
 *      debug version of xstrcat.
 *
 *      stringh.h automatically maps xstrcat to this
 *      function if __XWPMEMDEBUG__ is defined.
 *
 *@@addded V0.9.1 (99-12-20) [umoeller]
 *@@changed V0.9.2 (2000-04-01) [umoeller]: renamed from strhxcatDebug
 */

ULONG xstrcatDebug(XSTR ppszBuf,
                   const char *pszString,
                   const char *file,
                   unsigned long line,
                   const char *function)
{
    ULONG   ulrc = 0;
    if ((ppszBuf) && (pszString))
    {
        if (*ppszBuf == NULL)
            xstrcpy(ppszBuf, pszString);
        else
        {
            ULONG   cbOld = strlen(*ppszBuf),
                    cbString = strlen(pszString);
            PSZ     pszOldCopy = strdup(*ppszBuf);

            ulrc = cbOld + cbString + 1;
            if (*ppszBuf)
                free(*ppszBuf);
            *ppszBuf = (PSZ)memdMalloc(ulrc, file, line, function);
            // copy old string
            memcpy(*ppszBuf,
                   pszOldCopy,
                   cbOld);
            // append new string
            memcpy(*ppszBuf + cbOld,
                   pszString,
                   cbString + 1);       // include null terminator
            free(pszOldCopy);       // fixed V0.9.1 (99-12-20) [umoeller]
        }
    }
    return (ulrc);
}

#else // __XWPMEMDEBUG__

/*
 *@@ xstrcat:
 *      appends pszString to *ppszBuf, which is re-allocated as
 *      necessary.
 *
 *      If *ppszBuf is NULL, this behaves just as xstrcpy.
 *
 *      Returns the length of the new string (including the null
 *      terminator), or null upon errors.
 *
 *      Example:
 +          PSZ psz = strdup("blah");
 +          xstrcat(&psz, "blup");
 *      would do the following:
 *      a)  free the old value of psz ("blah");
 *      b)  reallocate psz;
 *      c)  so that psz afterwards points to a new string containing
 *          "blahblup".
 *
 *@@changed V0.9.1 (99-12-20) [umoeller]: fixed memory leak
 *@@changed V0.9.1 (2000-01-03) [umoeller]: crashed if pszString was null; fixed
 *@@changed V0.9.2 (2000-04-01) [umoeller]: renamed from strhxcat
 *@@changed V0.9.3 (2000-05-11) [umoeller]: returned 0 if pszString was initially empty; fixed
 */

ULONG xstrcat(XSTR ppszBuf,
              const char *pszString)
{
    ULONG   ulrc = 0;
    if ((ppszBuf) && (pszString))
    {
        if (*ppszBuf == NULL)
            ulrc = xstrcpy(ppszBuf, pszString);
        else
        {
            ULONG   cbOld = strlen(*ppszBuf),
                    cbString = strlen(pszString);
            PSZ     pszOldCopy = strdup(*ppszBuf);

            ulrc = cbOld + cbString + 1;
            if (*ppszBuf)
                free(*ppszBuf);
            *ppszBuf = (PSZ)malloc(ulrc);
            // copy old string
            memcpy(*ppszBuf,
                   pszOldCopy,
                   cbOld);
            // append new string
            memcpy(*ppszBuf + cbOld,
                   pszString,
                   cbString + 1);       // include null terminator
            free(pszOldCopy);       // fixed V0.9.1 (99-12-20) [umoeller]
        }
    }
    return (ulrc);
}

#endif // else __XWPMEMDEBUG__

#ifdef __XWPMEMDEBUG__ // setup.h, helpers\memdebug.c

/*
 *@@ xstrrplDebug:
 *
 *@@added V0.9.3 (2000-04-11) [umoeller]
 */

ULONG xstrrplDebug(PSZ *ppszBuf,     // in/out: text buffer
                   ULONG ulOfs,      // in: where to begin search (can be 0)
                   const char *pszSearch,    // in: search string
                   const char *pszReplace,   // in: replacement string
                   PULONG pulAfterOfs,  // out: offset where found (can be NULL)
                   const char *file,
                   unsigned long line,
                   const char *function)
{
    ULONG    ulrc = 0;

    if ((ppszBuf) && (pszSearch) && (pszReplace))
    {
        ULONG   cbBuf = 0,
                cbSearch = strlen(pszSearch);
        if (*ppszBuf)                       // fixed V0.9.0 (99-11-08) [umoeller]
            cbBuf =  strlen(*ppszBuf);

        if ((ulOfs < cbBuf) && (cbSearch))
        {
            PSZ     pFound = strstr((*ppszBuf) + ulOfs,
                                    pszSearch);

            if (pFound)
            {
                ULONG   cbReplace = strlen(pszReplace),
                        // length of new string
                        cbNew = cbBuf
                                + cbReplace
                                - cbSearch
                                + 1,                  // null terminator
                        // offset where pszSearch was found
                        ulFoundOfs = pFound - *ppszBuf;

                // allocate new buffer
                PSZ     pszNew = (PSZ)memdMalloc(cbNew,
                                                 file, line, function);

                if (ulFoundOfs)
                {
                    // copy until offset
                    strncpy(pszNew,
                            *ppszBuf,
                            ulFoundOfs);
                }

                if (cbReplace)
                {
                    // copy replacement
                    strncpy(pszNew + ulFoundOfs,
                            pszReplace,
                            cbReplace);
                }
                // copy rest
                strcpy(pszNew + ulFoundOfs + cbReplace,
                       pFound + cbSearch);

                // replace PSZ pointer
                memdFree(*ppszBuf, file, line, function);
                *ppszBuf = pszNew;

                // return new length
                ulrc = cbNew;
                if (pulAfterOfs)
                    *pulAfterOfs = ulFoundOfs + cbReplace;
            }
        }
    }
    return (ulrc);
}

#else

/*
 *@@ xstrrpl:
 *      replaces pszSearch with pszReplace in *ppszBuf.
 *
 *      If pszSearch was found, *ppszBuf is
 *      re-allocated so the buffer cannot overflow. As
 *      a result, *ppszBuf must be free()'able.
 *
 *      Returns the length of the new string or 0 if
 *      pszSearch was not found (and ppszBuf was therefore
 *      not changed).
 *
 *      If the string was found and (pulAfterOfs != NULL),
 *      *pulAfterOfs will be set to the first character
 *      after the new replacement string. This allows you
 *      to call this func again with the same strings to
 *      have several occurences replaced.
 *
 *      Only the first occurence is replaced. To replace
 *      all occurences in a buffer, repeat calling this
 *      function until it returns 0.
 *
 *      <B>Example usage:</B>
 +          PSZ psz = strdup("Test string");
 +          xstrrpl(&psz, "Test", "Dummy");
 *
 *      would reallocate psz to point to a new string
 *      containing "Dummy string".
 *
 *@@changed V0.9.0 [umoeller]: totally rewritten.
 *@@changed V0.9.0 (99-11-08) [umoeller]: crashed if *ppszBuf was NULL. Fixed.
 *@@changed V0.9.2 (2000-04-01) [umoeller]: renamed from strhxrpl
 */

ULONG xstrrpl(PSZ *ppszBuf,     // in/out: text buffer; cannot be NULL
              ULONG ulOfs,      // in: where to begin search (can be 0)
              const char *pszSearch,    // in: search string; cannot be NULL
              const char *pszReplace,   // in: replacement string; cannot be NULL
              PULONG pulAfterOfs)  // out: offset where found (can be NULL)
{
    ULONG    ulrc = 0;

    if ((ppszBuf) && (pszSearch) && (pszReplace))
    {
        ULONG   cbBuf = 0,
                cbSearch = strlen(pszSearch);
        if (*ppszBuf)                       // fixed V0.9.0 (99-11-08) [umoeller]
            cbBuf =  strlen(*ppszBuf);

        if ((ulOfs < cbBuf) && (cbSearch))
        {
            PSZ     pFound = strstr((*ppszBuf) + ulOfs,
                                    pszSearch);

            if (pFound)
            {
                ULONG   cbReplace = strlen(pszReplace),
                        // length of new string
                        cbNew = cbBuf
                                + cbReplace
                                - cbSearch
                                + 1,                  // null terminator
                        // offset where pszSearch was found
                        ulFoundOfs = pFound - *ppszBuf;

                // allocate new buffer
                PSZ     pszNew = (PSZ)malloc(cbNew);

                if (ulFoundOfs)
                {
                    // copy until offset
                    strncpy(pszNew,
                            *ppszBuf,
                            ulFoundOfs);
                }

                if (cbReplace)
                {
                    // copy replacement
                    strncpy(pszNew + ulFoundOfs,
                            pszReplace,
                            cbReplace);
                }
                // copy rest
                strcpy(pszNew + ulFoundOfs + cbReplace,
                       pFound + cbSearch);

                // replace PSZ pointer
                free(*ppszBuf);
                *ppszBuf = pszNew;

                // return new length
                ulrc = cbNew;
                if (pulAfterOfs)
                    *pulAfterOfs = ulFoundOfs + cbReplace;
            }
        }
    }
    return (ulrc);
}

#endif // else __XWPMEMDEBUG__

/*
 *@@ xstrins:
 *      this inserts one string into another.
 *
 *      pszInsert is inserted into pszBuffer at offset
 *      ulInsertOfs (which counts from 0).
 *
 *      A newly allocated string is returned. pszBuffer is
 *      not changed. The new string should be free()'d after
 *      use.
 *
 *      Upon errors, NULL is returned.
 *
 *@@changed V0.9.0 [umoeller]: completely rewritten.
 *@@changed V0.9.2 (2000-04-01) [umoeller]: renamed from strhxins
 */

PSZ xstrins(PSZ pszBuffer,
            ULONG ulInsertOfs,
            const char *pszInsert)
{
    PSZ     pszNew = NULL;

    if ((pszBuffer) && (pszInsert))
    {
        do {
            ULONG   cbBuffer = strlen(pszBuffer);
            ULONG   cbInsert = strlen(pszInsert);

            // check string length
            if (ulInsertOfs > cbBuffer + 1)
                break;  // do

            // OK, let's go.
            pszNew = (PSZ)malloc(cbBuffer + cbInsert + 1);  // additional null terminator

            // copy stuff before pInsertPos
            memcpy(pszNew,
                   pszBuffer,
                   ulInsertOfs);
            // copy string to be inserted
            memcpy(pszNew + ulInsertOfs,
                   pszInsert,
                   cbInsert);
            // copy stuff after pInsertPos
            strcpy(pszNew + ulInsertOfs + cbInsert,
                   pszBuffer + ulInsertOfs);
        } while (FALSE);
    }

    return (pszNew);
}


