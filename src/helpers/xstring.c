
/*
 *@@sourcefile xstring.c:
 *      string functions with memory management.
 *
 *      Usage: All OS/2 programs.
 *
 *      The functions in this file are intended as a replacement
 *      to the C library string functions (such as strcpy, strcat)
 *      in cases where the length of the string is unknown and
 *      dynamic memory management is desirable.
 *
 *      Instead of char* pointers, the functions in this file
 *      operate on an XSTRING structure, which contains a char*
 *      pointer instead.
 *
 *      Using these functions has the following advantages:
 *
 *      -- Automatic memory management. For example, xstrcat will
 *         automatically allocate new memory if the new string
 *         does not fit into the present buffer.
 *
 *      -- The length of the string is always known. Instead
 *         of running strlen (which consumes time), XSTRING.ulLength
 *         always contains the current length of the string.
 *
 *      -- The functions also differentiate between allocated
 *         memory and the length of the string. That is, for
 *         iterative appends, you can pre-allocate memory to
 *         avoid excessive reallocations.
 *
 *      Usage:
 *
 *      1) Allocate an XSTRING structure on the stack. Always
 *         call xstrInit on the structure, like this:
 *
 +              XSTRING str;
 +              xstrInit(&str, 0);      // no pre-allocation
 *
 *         Alternatively, use xstrCreate to have an XSTRING
 *         allocated from the heap.
 *
 *         Always call xstrClear(&str) to free allocated
 *         memory. Otherwise you'll get memory leaks.
 *         (For heap XSTRING's from xstrCreate, use xstrFree.)
 *
 *      2) To copy something into the string, use xstrcpy.
 *         To append something to the string, use xstrcat.
 *         See those functions for samples.
 *
 *      3) If you need the char* pointer (e.g. for a call
 *         to another function), use XSTRING.psz. However,
 *         you should ONLY modify the psz pointer yourself
 *         if the other XSTRING members are updated accordingly.
 *         You may, for example, change single characters
 *         in the psz buffer. By contrast, if you change the
 *         length of the string, you must update XSTRING.ulLength.
 *         Otherwise these functions will get into trouble.
 *
 *         Also, you should never assume that the "psz"
 *         pointer has not changed after you have called
 *         one of the xstr* functions because these can
 *         always reallocate the buffer if more memory
 *         was needed.
 *
 *      4) If (and only if) you have a char* buffer which
 *         is free()'able (e.g. from strdup()), you can
 *         use xstrset to avoid duplicate copying.
 *
 *      Function prefixes:
 *      --  xstr*       extended string functions.
 *
 *      The functions in this file used to be in stringh.c
 *      before V0.9.3 (2000-04-01). These have been largely
 *      rewritten with V0.9.6 (2000-11-01) and are now much
 *      more efficient.
 *
 *      Note: Version numbering in this file relates to XWorkplace
 *            version numbering.
 *
 *@@added V0.9.3 (2000-04-01) [umoeller]
 *@@header "helpers\xstring.h"
 */

/*
 *      Copyright (C) 1999-2000 Ulrich M”ller.
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
 *@@ xstrInit:
 *      initializes an empty XSTRING.
 *
 *      If (ulPreAllocate != 0), memory is pre-allocated
 *      for the string, but the string will be empty.
 *      This is useful if you plan to add more stuff to
 *      the string later so we don't have to reallocate
 *      all the time in xstrcat.
 *
 *      Do not use this on an XSTRING which is already
 *      initialized. Use xstrset instead.
 *
 *@@added V0.9.6 (2000-11-01) [umoeller]
 */

void xstrInit(PXSTRING pxstr,               // in/out: string
              ULONG ulPreAllocate)          // in: if > 0, memory to allocate
{
    memset(pxstr, 0, sizeof(XSTRING));
    if (ulPreAllocate)
    {
        pxstr->psz = (PSZ)malloc(ulPreAllocate);
        pxstr->cbAllocated = ulPreAllocate;
                // ulLength is still zero
        *(pxstr->psz) = 0;
    }
}

/*
 *@@ xstrInitSet:
 *      this can be used instead of xstrInit if you
 *      have a free()'able string you want to initialize
 *      the XSTRING with.
 *
 *      This does not create a copy of pszNew. Instead,
 *      pszNew is used as the member string in pxstr
 *      directly.
 *
 *      Do not use this on an XSTRING which is already
 *      initialized. Use xstrset instead.
 *
 *      Example:
 *
 +          XSTRING str;
 +          xstrInitSet(&str, strdup("blah"));
 *
 *@@added V0.9.6 (2000-11-01) [umoeller]
 */

void xstrInitSet(PXSTRING pxstr,
                 PSZ pszNew)
{
    pxstr->psz = pszNew;
    if (!pszNew)
    {
        pxstr->cbAllocated = 0;
        pxstr->ulLength = 0;
    }
    else
    {
        pxstr->ulLength = strlen(pszNew);
        pxstr->cbAllocated = pxstr->ulLength + 1;
    }
}

/*
 *@@ xstrInitCopy:
 *      this can be used instead of xstrInit if you
 *      want to initialize an XSTRING with a copy
 *      of an existing string. This is a shortcut
 *      for xstrInit() and then xstrcpy().
 *
 *      As opposed to xstrInitSet, this does create
 *      a copy of pcszSource.
 *
 *      Do not use this on an XSTRING which is already
 *      initialized. Use xstrcpy instead.
 *
 *      Example:
 *
 +          XSTRING str;
 +          xstrInitCopy(&str, "blah");
 *
 *@@added V0.9.6 (2000-11-01) [umoeller]
 */

void xstrInitCopy(PXSTRING pxstr,
                  const char *pcszSource)
{
    if (pxstr)
    {
        memset(pxstr, 0, sizeof(XSTRING));

        if (pcszSource)
            pxstr->ulLength = strlen(pcszSource);

        if (pxstr->ulLength)
        {
            // we do have a source string:
            pxstr->cbAllocated = pxstr->ulLength + 1;
            pxstr->psz = (PSZ)malloc(pxstr->cbAllocated);
            strcpy(pxstr->psz, pcszSource);
        }
    }
}

/*
 *@@ xstrClear:
 *      clears the specified stack XSTRING and
 *      frees allocated memory.
 *
 *      This is the reverse to xstrInit.
 *
 *@@added V0.9.6 (2000-11-01) [umoeller]
 */

void xstrClear(PXSTRING pxstr)              // in/out: string
{
    if (pxstr->psz)
        free(pxstr->psz);
    memset(pxstr, 0, sizeof(XSTRING));
}

/*
 *@@ xstrCreate:
 *      allocates a new XSTRING from the heap
 *      and calls xstrInit on it.
 *
 *      Always use xstrFree to free associated
 *      resources.
 *
 *@@added V0.9.6 (2000-11-01) [umoeller]
 */

PXSTRING xstrCreate(ULONG ulPreAllocate)
{
    PXSTRING pxstr = (PXSTRING)malloc(sizeof(XSTRING));
    if (pxstr)
        xstrInit(pxstr, ulPreAllocate);

    return (pxstr);
}

/*
 *@@ xstrFree:
 *      frees the specified heap XSTRING, which must
 *      have been created using xstrCreate.
 *
 *@@added V0.9.6 (2000-11-01) [umoeller]
 */

VOID xstrFree(PXSTRING pxstr)               // in/out: string
{
    if (pxstr)
    {
        xstrClear(pxstr);
        free(pxstr);
    }
}

/*
 *@@ xstrset:
 *      sets the specified XSTRING to a new string
 *      without copying it.
 *
 *      pxstr is cleared before the new string is set.
 *
 *      This ONLY works if pszNew has been allocated from
 *      the heap using malloc() or strdup() and is thus
 *      free()'able.
 *
 *      This assumes that exactly strlen(pszNew) + 1
 *      bytes have been allocated for pszNew, which
 *      is true if pszNew comes from strdup().
 *
 *@@added V0.9.6 (2000-11-01) [umoeller]
 */

ULONG xstrset(PXSTRING pxstr,               // in/out: string
              PSZ pszNew)                   // in: heap PSZ to use
{
    xstrClear(pxstr);
    pxstr->psz = pszNew;
    if (pszNew)
    {
        pxstr->ulLength = strlen(pszNew);
        pxstr->cbAllocated = pxstr->ulLength + 1;
    }
    // else null string: cbAllocated and ulLength are 0 already

    return (pxstr->ulLength);
}

/*
 *@@ xstrcpy:
 *      copies pcszSource to pxstr, for which memory is allocated
 *      as necessary.
 *
 *      If pxstr contains something, its contents are destroyed.
 *
 *      Returns the length of the new string (excluding the null
 *      terminator), or null upon errors.
 *
 *      Example:
 *
 +          XSTRING str;
 +          xstrInit(&str, 0);
 +          xstrcpy(&str, "blah");
 *
 *      This sequence can be abbreviated using xstrInitCopy.
 *
 *@@changed V0.9.2 (2000-04-01) [umoeller]: renamed from strhxcpy
 *@@changed V0.9.6 (2000-11-01) [umoeller]: rewritten
 */

ULONG xstrcpy(PXSTRING pxstr,               // in/out: string
              const char *pcszSource)       // in: source, can be NULL
{
    xstrClear(pxstr);

    if (pxstr)
    {
        ULONG   ulSourceLength = 0;
        if (pcszSource)
            ulSourceLength = strlen(pcszSource);

        if (ulSourceLength)
        {
            // we do have a source string:
            ULONG cbNeeded = ulSourceLength + 1;
            if (cbNeeded > pxstr->cbAllocated)
            {
                // we need more memory than we have previously
                // allocated:
                pxstr->cbAllocated = cbNeeded;
                pxstr->psz = (PSZ)malloc(cbNeeded);
            }
            // else: we have enough memory

            strcpy(pxstr->psz, pcszSource);
        }
        else
        {
            // no source specified or source is empty:
            if (pxstr->cbAllocated)
                // we did have a string: set to empty,
                // but leave allocated memory intact
                *(pxstr->psz) = 0;
            // else: pxstr->psz is still NULL
        }

        // in all cases, set new length
        pxstr->ulLength = ulSourceLength;
    }

    return (pxstr->ulLength);
}

/*
 *@@ xstrcat:
 *      appends pcszSource to pxstr, for which memory is
 *      reallocated if necessary.
 *
 *      If pxstr is empty, this behaves just like xstrcpy.
 *
 *      Returns the length of the new string (excluding the null
 *      terminator) if the string was changed, or 0 if nothing
 *      happened.
 *
 *      Note: To append a single character, xstrcatc is faster
 *      than xstrcat.
 *
 *      Example:
 *
 +          XSTRING str;
 +          xstrInit(&str, 0);
 +          xstrcpy(&str, "blah");
 +          xstrcat(&str, "blup");
 *
 *      After this, str.psz points to a new string containing
 *      "blahblup".
 *
 *@@changed V0.9.1 (99-12-20) [umoeller]: fixed memory leak
 *@@changed V0.9.1 (2000-01-03) [umoeller]: crashed if pszString was null; fixed
 *@@changed V0.9.2 (2000-04-01) [umoeller]: renamed from strhxcat
 *@@changed V0.9.3 (2000-05-11) [umoeller]: returned 0 if pszString was initially empty; fixed
 *@@changed V0.9.6 (2000-11-01) [umoeller]: rewritten
 *@@changed V0.9.7 (2000-12-10) [umoeller]: return value was wrong
 */

ULONG xstrcat(PXSTRING pxstr,               // in/out: string
              const char *pcszSource)       // in: source, can be NULL
{
    ULONG   ulrc = 0;

    if (pxstr)
    {
        ULONG   ulSourceLength = 0;
        if (pcszSource)
            ulSourceLength = strlen(pcszSource);

        if (ulSourceLength)
        {
            // we do have a source string:

            // 1) memory management
            ULONG   cbNeeded = pxstr->ulLength + ulSourceLength + 1;
            if (cbNeeded > pxstr->cbAllocated)
            {
                // we need more memory than we have previously
                // allocated:
                if (pxstr->cbAllocated)
                    // appendee already had memory:
                    // reallocate
                    pxstr->psz = (PSZ)realloc(pxstr->psz,
                                              cbNeeded);
                else
                    // appendee has no memory:
                    pxstr->psz = (PSZ)malloc(cbNeeded);

                pxstr->cbAllocated = cbNeeded;
                        // ulLength is unchanged yet
            }
            // else: we have enough memory, both if appendee
            //       is empty or not empty

            // now we have:
            // -- if appendee (pxstr) had enough memory, no problem
            // -- if appendee (pxstr) needed more memory
            //      -- and was not empty: pxstr->psz now points to a
            //         reallocated copy of the old string
            //      -- and was empty: pxstr->psz now points to a
            //         new (unitialized) buffer

            // 2) append source string:
            memcpy(pxstr->psz + pxstr->ulLength,
                   pcszSource,
                   ulSourceLength + 1);     // null terminator

            // in all cases, set new length
            pxstr->ulLength += ulSourceLength;
            ulrc = pxstr->ulLength;     // V0.9.7 (2000-12-10) [umoeller]

        } // end if (ulSourceLength)
        // else no source specified or source is empty:
        // do nothing
    }

    return (ulrc);
}

/*
 *@@ xstrcatc:
 *      this is similar to xstrcat, except that this is
 *      for a single character. This is a bit faster than
 *      xstrcat.
 *
 *      If "c" is \0, nothing happens.
 *
 *      If pxstr is empty, this behaves just like xstrcpy.
 *
 *      Returns the length of the new string (excluding the null
 *      terminator) if the string was changed, or 0 if nothing
 *      happened.
 *
 *      Example:
 *
 +          XSTRING str;
 +          xstrInit(&str, 0);
 +          xstrcpy(&str, "blu");
 +          xstrcatc(&str, 'p');
 *
 *      After this, str.psz points to a new string containing
 *      "blup".
 *
 *@@added V0.9.7 (2000-12-10) [umoeller]
 */

ULONG xstrcatc(PXSTRING pxstr,     // in/out: string
               CHAR c)             // in: character to append, can be \0
{
    ULONG   ulrc = 0;

    if ((pxstr) && (c))
    {
        // ULONG   ulSourceLength = 1;
        // 1) memory management
        ULONG   cbNeeded = pxstr->ulLength  // existing length, without null terminator
                           + 1      // new character
                           + 1;     // null terminator
        if (cbNeeded > pxstr->cbAllocated)
        {
            // we need more memory than we have previously
            // allocated:
            if (pxstr->cbAllocated)
                // appendee already had memory:
                // reallocate
                pxstr->psz = (PSZ)realloc(pxstr->psz,
                                          cbNeeded);
            else
                // appendee has no memory:
                pxstr->psz = (PSZ)malloc(cbNeeded);

            pxstr->cbAllocated = cbNeeded;
                    // ulLength is unchanged yet
        }
        // else: we have enough memory, both if appendee
        //       is empty or not empty

        // now we have:
        // -- if appendee (pxstr) had enough memory, no problem
        // -- if appendee (pxstr) needed more memory
        //      -- and was not empty: pxstr->psz now points to a
        //         reallocated copy of the old string
        //      -- and was empty: pxstr->psz now points to a
        //         new (unitialized) buffer

        // 2) append character:
        pxstr->psz[pxstr->ulLength] = c;
        pxstr->psz[pxstr->ulLength + 1] = '\0';

        // in all cases, set new length
        pxstr->ulLength++;
        ulrc = pxstr->ulLength;

    } // end if ((pxstr) && (c))

    return (ulrc);
}

/*
 *@@ xstrFindWord:
 *      searches for pstrFind in pxstr, starting at ulOfs.
 *      However, this only finds pstrFind if it's a "word",
 *      i.e. surrounded by one of the characters in the
 *      pcszBeginChars and pcszEndChars array.
 *
 *      This is similar to strhFindWord, but this uses
 *      strhmemfind for fast searching, and it doesn't
 *      have to calculate the string lengths because these
 *      already in XSTRING.
 *
 *      Returns 0 if no "word" was found, or the offset
 *      of the "word" in pxstr if found.
 *
 *@@added V0.9.6 (2000-11-12) [umoeller]
 */

PSZ xstrFindWord(const XSTRING *pxstr,        // in: buffer to search ("haystack")
                 ULONG ulOfs,                 // in: where to begin search (0 = start)
                 const XSTRING *pstrFind,     // in: word to find ("needle")
                 size_t *pShiftTable,         // in: shift table (see strhmemfind)
                 PBOOL pfRepeatFind,          // in: repeat find? (see strhmemfind)
                 const char *pcszBeginChars,  // suggestion: "\x0d\x0a ()/\\-,."
                 const char *pcszEndChars)    // suggestion: "\x0d\x0a ()/\\-,.:;"
{
    PSZ     pReturn = 0;
    ULONG   ulFoundLen = pstrFind->ulLength;

    if ((pxstr->ulLength) && (ulFoundLen))
    {
        const char *p = pxstr->psz + ulOfs;

        do  // while p
        {
            // p = strstr(p, pstrFind->psz);
            p = (PSZ)strhmemfind(p,         // in: haystack
                                 pxstr->ulLength - (p - pxstr->psz),
                                            // remaining length of haystack
                                 pstrFind->psz,
                                 ulFoundLen,
                                 pShiftTable,
                                 pfRepeatFind);
            if (p)
            {
                // string found:
                // check if that's a word

                if (strhIsWord(pxstr->psz,
                               p,
                               ulFoundLen,
                               pcszBeginChars,
                               pcszEndChars))
                {
                    // valid end char:
                    pReturn = (PSZ)p;
                    break;
                }

                p += ulFoundLen;
            }
        } while (p);

    }
    return (pReturn);
}

/*
 *@@ xstrrpl:
 *      replaces the first occurence of pstrSearch with
 *      pstrReplace in pxstr.
 *
 *      Starting with V0.9.6, this operates entirely on
 *      XSTRING's for speed because we then know the string
 *      lengths already and can use memcpy instead of strcpy.
 *      This new version should be magnitudes faster,
 *      especially with large string bufffers.
 *
 *      None of the pointers can be NULL, but if pstrReplace
 *      is empty, this effectively erases pstrSearch in pxstr.
 *
 *      Returns the length of the new string (exclusing the
 *      null terminator) or 0 if pszSearch was not found
 *      (and pxstr was therefore not changed).
 *
 *      This starts the search at *pulOffset. If
 *      (*pulOffset == 0), this starts from the beginning
 *      of pxstr.
 *      If the string was found, *pulOffset will be set to the
 *      first character after the new replacement string. This
 *      allows you to call this func again with the same strings
 *      to have several occurences replaced (see the example below).
 *
 *      There are two wrappers around this function which
 *      work on C strings instead (however, thus losing the
 *      speed advantage):
 *
 *      -- strhrpl operates on C strings only;
 *
 *      -- xstrcrpl uses C strings for the search and replace
 *         parameters.
 *
 *      <B>Example usage:</B>
 *
 +          XSTRING str;
 +          ULONG ulOffset = 0;
 +          xstrInit(&str, 0);
 +          xstrcpy(&str, "Test phrase 1. Test phrase 2.");
 +          while (xstrrpl(&str,
 +                         &ulPos,      // in/out: offset
 +                         "Test",      // search
 +                         "Dummy")     // replace
 +              ;
 *
 *      would replace all occurences of "Test" in str with
 *      "Dummy".
 *
 *@@changed V0.9.0 [umoeller]: totally rewritten.
 *@@changed V0.9.0 (99-11-08) [umoeller]: crashed if *ppszBuf was NULL. Fixed.
 *@@changed V0.9.2 (2000-04-01) [umoeller]: renamed from strhxrpl
 *@@changed V0.9.6 (2000-11-01) [umoeller]: rewritten
 *@@changed V0.9.6 (2000-11-12) [umoeller]: now using strhmemfind
 */

ULONG xstrrpl(PXSTRING pxstr,               // in/out: string
              PULONG pulOfs,                // in: where to begin search (0 = start);
                                            // out: ofs of first char after replacement string
              const XSTRING *pstrSearch,    // in: search string; cannot be NULL
              const XSTRING *pstrReplace,   // in: replacement string; cannot be NULL
              size_t *pShiftTable,          // in: shift table (see strhmemfind)
              PBOOL pfRepeatFind)           // in: repeat find? (see strhmemfind)
{
    ULONG    ulrc = 0;      // default: not found

    if ((pxstr) && (pstrSearch) && (pstrReplace))
    {
        ULONG   cSearchLen = pstrSearch->ulLength;

        // can we search this?
        if (    (*pulOfs < pxstr->ulLength)
             && (cSearchLen)
           )
        {
            // yes:
            /* PSZ     pFound = strstr(pxstr->psz + *pulOfs,
                                    pstrSearch->psz);        */
            PSZ pFound = (PSZ)strhmemfind(pxstr->psz + *pulOfs, // in: haystack
                                          pxstr->ulLength - *pulOfs,
                                          pstrSearch->psz,
                                          cSearchLen,
                                          pShiftTable,
                                          pfRepeatFind);
            if (pFound)
            {
                // found in buffer from ofs:
                ULONG   cReplaceLen = pstrReplace->ulLength;
                            // can be 0!

                // length of new string
                ULONG   cbNeeded = pxstr->ulLength
                                 + cReplaceLen
                                 - cSearchLen
                                 + 1,                  // null terminator
                // offset where pszSearch was found
                        ulFoundOfs = pFound - pxstr->psz;

                // now check if we have enough memory...
                if (pxstr->cbAllocated < cbNeeded)
                {
                    // no, we need more memory:
                    // allocate new buffer
                    PSZ pszNew = (PSZ)malloc(cbNeeded);

                    if (ulFoundOfs)
                        // "found" was not at the beginning:
                        // copy from beginning up to found-offset
                        memcpy(pszNew,
                               pxstr->psz,
                               ulFoundOfs);     // up to "found"

                    if (cReplaceLen)
                    {
                        // we have a replacement:
                        // insert it next
                        memcpy(pszNew + ulFoundOfs,
                               pstrReplace->psz,
                               cReplaceLen + 1);        // include null terminator
                    }

                    // copy rest:
                    // pxstr      frontFOUNDtail
                    //            0         1
                    //            01234567890123
                    //            ³    ³    ³  ³
                    //            ³    ³    ÀÄ ulFoundOfs + cSearchLen = 10
                    //            ³    ³       ³
                    //            ³    ÀÄ ulFoundOfs = 5
                    //            ³            ³
                    //            pxstr->ulLength = 14
                    memcpy(pszNew + ulFoundOfs + cReplaceLen,
                           pFound + cSearchLen,
                           // remaining bytes:
                           pxstr->ulLength - ulFoundOfs - cSearchLen // 9
                                + 1); // null terminator

                    // replace old buffer with new one
                    free(pxstr->psz);
                    pxstr->psz = pszNew;
                    pxstr->ulLength = cbNeeded - 1;
                    pxstr->cbAllocated = cbNeeded;
                } // end if (pxstr->cbAllocated < cbNeeded)
                else
                {
                    // we have enough memory left,
                    // we can just overwrite in the middle...

                    PSZ     pszAfterFoundBackup = 0;
                    // calc length of string after "found"
                    ULONG   cTailLength = pxstr->ulLength - ulFoundOfs - cSearchLen;

                    // if "replace" is longer than "found",
                    // make a backup of the stuff after "found",
                    // or this would get overwritten
                    if (cReplaceLen > cSearchLen)
                    {
                        pszAfterFoundBackup = (PSZ)malloc(cTailLength + 1);
                        memcpy(pszAfterFoundBackup,
                               pFound + cSearchLen,
                               cTailLength + 1);
                    }

                    // now overwrite "found" in the middle
                    if (cReplaceLen)
                    {
                        memcpy(pxstr->psz + ulFoundOfs,
                               pstrReplace->psz,
                               cReplaceLen);        // no null terminator
                    }

                    // now append tail (stuff after "found") again...
                    if (pszAfterFoundBackup)
                    {
                        // we made a backup above:
                        memcpy(pxstr->psz + ulFoundOfs + cReplaceLen,
                               pszAfterFoundBackup,
                               cTailLength + 1);
                        free(pszAfterFoundBackup);
                                // done!
                    }
                    else
                        // no backup:
                        if (cReplaceLen < cSearchLen)
                            // "replace" is shorter than "found:
                            memcpy(pxstr->psz + ulFoundOfs + cReplaceLen,
                                   pFound + cSearchLen,
                                   cTailLength + 1);
                        // else (cReplaceLen == cSearchLen):
                        // we can leave the tail as it is

                    pxstr->ulLength = cbNeeded - 1;
                }

                // return new length
                ulrc = cbNeeded - 1;
                *pulOfs = ulFoundOfs + cReplaceLen;
            } // end if (pFound)
        } // end if (    (*pulOfs < pxstr->ulLength) ...
    } // end if ((pxstr) && (pstrSearch) && (pstrReplace))

    return (ulrc);
}

/*
 *@@ xstrcrpl:
 *      wrapper around xstrrpl() which allows using C strings
 *      for the find and replace parameters.
 *
 *      This creates two temporary XSTRING's for pcszSearch
 *      pcszReplace. As a result, this is slower than xstrrpl.
 *      If you search with the same strings several times,
 *      you'll be better off using xstrrpl() directly.
 *
 *@@added V0.9.6 (2000-11-01) [umoeller]
 */

ULONG xstrcrpl(PXSTRING pxstr,              // in/out: string
               PULONG pulOfs,               // in: where to begin search (0 = start);
                                            // out: ofs of first char after replacement string
               const char *pcszSearch,      // in: search string; cannot be NULL
               const char *pcszReplace)     // in: replacement string; cannot be NULL
{
    // ULONG   ulrc = 0;
    XSTRING xstrFind,
            xstrReplace;
    size_t  ShiftTable[256];
    BOOL    fRepeat = FALSE;
    // initialize find/replace strings... note that the
    // C strings are not free()'able, so we MUST NOT use xstrClear
    // before leaving
    xstrInitSet(&xstrFind, (PSZ)pcszSearch);
    xstrInitSet(&xstrReplace, (PSZ)pcszReplace);

    return (xstrrpl(pxstr, pulOfs, &xstrFind, &xstrReplace, ShiftTable, &fRepeat));
}

// test case

/* int main(void)
{
    XSTRING str,
            strFind,
            strReplace;
    ULONG   ulOfs = 0;

    xstrInit(&str, 100);
    xstrInit(&strFind, 0);
    xstrInit(&strReplace, 0);

    xstrcpy(&str, "Test string 1. Test string 2. Test string 3. !");
    xstrcpy(&strFind, "Test");
    xstrcpy(&strReplace, "Dummy");

    printf("Old string is: \"%s\" (%d/%d)\n", str.psz, str.ulLength, str.cbAllocated);

    while (xstrrpl(&str,
                   ulOfs,
                   &strFind,
                   &strReplace,
                   &ulOfs))
        ;

    printf("New string is: \"%s\" (%d/%d)\n", str.psz, str.ulLength, str.cbAllocated);

    xstrcpy(&strFind, strReplace.psz);
    xstrClear(&strReplace);
    ulOfs = 0;
    while (xstrrpl(&str,
                   ulOfs,
                   &strFind,
                   &strReplace,
                   &ulOfs))
        ;

    printf("New string is: \"%s\" (%d/%d)\n", str.psz, str.ulLength, str.cbAllocated);

    xstrcpy(&strFind, " ");
    xstrcpy(&strReplace, ".");
    ulOfs = 0;
    while (xstrrpl(&str,
                   ulOfs,
                   &strFind,
                   &strReplace,
                   &ulOfs))
        ;

    printf("New string is: \"%s\" (%d/%d)\n", str.psz, str.ulLength, str.cbAllocated);

    xstrcpy(&strFind, ".");
    xstrcpy(&strReplace, "***************************");
    ulOfs = 0;
    while (xstrrpl(&str,
                   ulOfs,
                   &strFind,
                   &strReplace,
                   &ulOfs))
        ;

    printf("New string is: \"%s\" (%d/%d)\n", str.psz, str.ulLength, str.cbAllocated);

    xstrcpy(&strFind, "*");
    xstrClear(&strReplace);
    ulOfs = 0;
    while (xstrrpl(&str,
                   ulOfs,
                   &strFind,
                   &strReplace,
                   &ulOfs))
        ;

    printf("New string is: \"%s\" (%d/%d)\n", str.psz, str.ulLength, str.cbAllocated);
} */

