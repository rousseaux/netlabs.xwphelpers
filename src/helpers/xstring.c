
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
 *@@category: Helpers\C helpers\String management\XStrings (with memory management)
 *      See xstring.c.
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
 *@@changed V0.9.7 (2000-12-31) [umoeller]: added ulExtraAllocate
 */

void xstrInitCopy(PXSTRING pxstr,
                  const char *pcszSource,
                  ULONG ulExtraAllocate)          // in: if > 0, extra memory to allocate
{
    if (pxstr)
    {
        memset(pxstr, 0, sizeof(XSTRING));

        if (pcszSource)
            pxstr->ulLength = strlen(pcszSource);

        if (pxstr->ulLength)
        {
            // we do have a source string:
            pxstr->cbAllocated = pxstr->ulLength + 1 + ulExtraAllocate;
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
 *@@ xstrReserve:
 *      this function makes sure that the specified
 *      XSTRING has at least ulBytes bytes allocated.
 *
 *      This function is useful if you plan to do
 *      a lot of string replacements or appends and
 *      want to avoid that the buffer is reallocated
 *      with each operation. Before those operations,
 *      call this function to make room for the operations.
 *
 *      If ulBytes is smaller than the current allocation,
 *      this function does nothing.
 *
 *      The XSTRING must be initialized before the
 *      call.
 *
 *      Returns the new total no. of allocated bytes.
 *
 *@@added V0.9.7 (2001-01-07) [umoeller]
 */

ULONG xstrReserve(PXSTRING pxstr,
                  ULONG ulBytes)
{
    ULONG   cbNeeded = ulBytes;

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
        {
            // appendee has no memory:
            pxstr->psz = (PSZ)malloc(cbNeeded);
            *(pxstr->psz) = 0;
        }

        pxstr->cbAllocated = cbNeeded;
                // ulLength is unchanged
    }

    return (pxstr->cbAllocated);
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
 *      With ulSourceLength, specify the length of pcszSource.
 *      If you specify 0, this function will run strlen(pcszSource)
 *      itself. If you already know the length of pcszSource (or
 *      only want to copy a substring), you can speed this function
 *      up a bit this way.
 *
 *      Returns the length of the new string (excluding the null
 *      terminator), or null upon errors.
 *
 *      Example:
 *
 +          XSTRING str;
 +          xstrInit(&str, 0);
 +          xstrcpy(&str, "blah", 0);
 *
 *      This sequence can be abbreviated using xstrInitCopy.
 *
 *@@changed V0.9.2 (2000-04-01) [umoeller]: renamed from strhxcpy
 *@@changed V0.9.6 (2000-11-01) [umoeller]: rewritten
 *@@changed V0.9.7 (2001-01-15) [umoeller]: added ulSourceLength
 */

ULONG xstrcpy(PXSTRING pxstr,               // in/out: string
              const char *pcszSource,       // in: source, can be NULL
              ULONG ulSourceLength)         // in: length of pcszSource or 0
{
    // xstrClear(pxstr);        NOOOO! this frees the string, we want to keep the memory

    if (pxstr)
    {
        if (pcszSource)
        {
            if (ulSourceLength == 0)
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

                // strcpy(pxstr->psz, pcszSource);
                memcpy(pxstr->psz,
                       pcszSource,
                       ulSourceLength);
                *(pxstr->psz + ulSourceLength) = 0;
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
 *      With ulSourceLength, specify the length of pcszSource.
 *      If you specify 0, this function will run strlen(pcszSource)
 *      itself. If you already know the length of pcszSource (or
 *      only want to copy a substring), you can speed this function
 *      up a bit this way.
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
 +          xstrcpy(&str, "blah", 0);
 +          xstrcat(&str, "blup", 0);
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
 *@@changed V0.9.7 (2001-01-15) [umoeller]: added ulSourceLength
 */

ULONG xstrcat(PXSTRING pxstr,               // in/out: string
              const char *pcszSource,       // in: source, can be NULL
              ULONG ulSourceLength)         // in: length of pcszSource or 0
{
    ULONG   ulrc = 0;

    if (pxstr)
    {
        if (pcszSource)
        {
            if (ulSourceLength == 0)
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
        }

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
 *@@ xstrrpl:
 *      replaces cSearchLen characters in pxstr, starting
 *      at the position ulStart, with the string
 *      in pxstrReplaceWith.
 *
 *      Returns the new length of the string, excluding
 *      the null terminator, or 0 if the replacement failed
 *      (e.g. because the offsets were too large).
 *
 *      This has been extracted from xstrFindReplace because
 *      if you already know the position of a substring,
 *      you can now call this directly. This properly
 *      reallocates the string if more memory is needed.
 *
 *      Example:
 *
 +          XSTRING xstr, xstrReplacement;
 +          xstrInitCopy(&xstr, "This is a test string.");
 +          //  positions:       0123456789012345678901
 +          //                             1         2
 +
 +          xstrInitCopy(&xstrReplacement, "stupid");
 +
 +          xstrrpl(&xstr,
 +                  10,     // position of "test"
 +                  4,      // length of "test"
 +                  &xstrReplacement);
 *
 *      This would yield "This is a stupid string."
 *
 *@@added V0.9.7 (2001-01-15) [umoeller]
 */

ULONG xstrrpl(PXSTRING pxstr,                   // in/out: string
              ULONG ulFirstReplOfs,             // in: ofs of first char to replace
              ULONG cReplLen,                   // in: no. of chars to replace
              const XSTRING *pstrReplaceWith)   // in: string to replace chars with
{
    ULONG   ulrc = 0;

    // security checks...
    if (    (ulFirstReplOfs + cReplLen <= pxstr->ulLength)
         && (pstrReplaceWith)
         // && (pstrReplaceWith->ulLength)      no, this can be empty
       )
    {
        ULONG   cReplaceLen = pstrReplaceWith->ulLength;
                    // can be 0!

        // length of new string
        ULONG   cbNeeded = pxstr->ulLength
                         + cReplaceLen
                         - cReplLen
                         + 1;                  // null terminator
        // offset where pszSearch was found
                // ulFirstReplOfs = pFound - pxstr->psz; now ulFirstReplOfs
        PSZ     pFound = pxstr->psz + ulFirstReplOfs;

        // now check if we have enough memory...
        if (pxstr->cbAllocated < cbNeeded)
        {
            // no, we need more memory:
            // allocate new buffer
            PSZ pszNew = (PSZ)malloc(cbNeeded);

            if (ulFirstReplOfs)
                // "found" was not at the beginning:
                // copy from beginning up to found-offset
                memcpy(pszNew,
                       pxstr->psz,
                       ulFirstReplOfs);     // up to "found"

            if (cReplaceLen)
            {
                // we have a replacement:
                // insert it next
                memcpy(pszNew + ulFirstReplOfs,
                       pstrReplaceWith->psz,
                       cReplaceLen + 1);        // include null terminator
            }

            // copy rest:
            // pxstr      frontFOUNDtail
            //            0         1
            //            01234567890123
            //            ³    ³    ³  ³
            //            ³    ³    ÀÄ ulFirstReplOfs + cReplLen = 10
            //            ³    ³       ³
            //            ³    ÀÄ ulFirstReplOfs = 5
            //            ³            ³
            //            pxstr->ulLength = 14
            memcpy(pszNew + ulFirstReplOfs + cReplaceLen,
                   pFound + cReplLen,
                   // remaining bytes:
                   pxstr->ulLength - ulFirstReplOfs - cReplLen // 9
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
            ULONG   cTailLength = pxstr->ulLength - ulFirstReplOfs - cReplLen;

            // if "replace" is longer than "found",
            // make a backup of the stuff after "found",
            // or this would get overwritten
            if (cReplaceLen > cReplLen)
            {
                pszAfterFoundBackup = (PSZ)malloc(cTailLength + 1);
                memcpy(pszAfterFoundBackup,
                       pFound + cReplLen,
                       cTailLength + 1);
            }

            // now overwrite "found" in the middle
            if (cReplaceLen)
            {
                memcpy(pxstr->psz + ulFirstReplOfs,
                       pstrReplaceWith->psz,
                       cReplaceLen);        // no null terminator
            }

            // now append tail (stuff after "found") again...
            if (pszAfterFoundBackup)
            {
                // we made a backup above:
                memcpy(pxstr->psz + ulFirstReplOfs + cReplaceLen,
                       pszAfterFoundBackup,
                       cTailLength + 1);
                free(pszAfterFoundBackup);
                        // done!
            }
            else
                // no backup:
                if (cReplaceLen < cReplLen)
                    // "replace" is shorter than "found:
                    memcpy(pxstr->psz + ulFirstReplOfs + cReplaceLen,
                           pFound + cReplLen,
                           cTailLength + 1);
                // else (cReplaceLen == cReplLen):
                // we can leave the tail as it is

            pxstr->ulLength = cbNeeded - 1;
        }

        ulrc = cbNeeded - 1;
    } // end checks

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
 *@@ xstrFindReplace:
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
 *
 *      If the string was found, *pulOffset will be set to the
 *      first character after the new replacement string. This
 *      allows you to call this func again with the same strings
 *      to have several occurences replaced (see the example below).
 *
 *      There are two wrappers around this function which
 *      work on C strings instead (however, thus losing the
 *      speed advantage):
 *
 *      -- strhFindReplace operates on C strings only;
 *
 *      -- xstrFindReplaceC uses C strings for the search and replace
 *         parameters.
 *
 *      <B>Example usage:</B>
 *
 +          XSTRING strBuf,
 +                  strFind,
 +                  strRepl;
 +          size_t  ShiftTable[256];
 +          BOOL    fRepeat = FALSE;
 +          ULONG   ulOffset = 0;
 +
 +          xstrInitCopy(&strBuf, "Test phrase 1. Test phrase 2.", 0);
 +          xstrInitSet(&strFind, "Test");
 +          xstrInitSet(&strRepl, "Dummy");
 +          while (xstrFindReplace(&str,
 +                                 &ulPos,      // in/out: offset
 +                                 &strFind,    // search
 +                                 &strRepl,    // replace
 +                                 ShiftTable,
 +                                 &fRepeat))
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
 *@@changed V0.9.7 (2001-01-15) [umoeller]: renamed from xstrrpl; extracted new xstrrpl
 */

ULONG xstrFindReplace(PXSTRING pxstr,               // in/out: string
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
            ULONG   ulOfs = *pulOfs;
            const char *pFound
                = (const char *)strhmemfind(pxstr->psz + ulOfs, // in: haystack
                                            pxstr->ulLength - ulOfs,
                                            pstrSearch->psz,
                                            cSearchLen,
                                            pShiftTable,
                                            pfRepeatFind);
            if (pFound)
            {
                ULONG ulFirstReplOfs = pFound - pxstr->psz;
                // found in buffer from ofs:
                // replace pFound with pstrReplace
                ulrc = xstrrpl(pxstr,
                               ulFirstReplOfs,              // where to start
                               cSearchLen,                  // chars to replace
                               pstrReplace);

                // return new length
                *pulOfs = ulFirstReplOfs + pstrReplace->ulLength;
            } // end if (pFound)
        } // end if (    (*pulOfs < pxstr->ulLength) ...
    } // end if ((pxstr) && (pstrSearch) && (pstrReplace))

    return (ulrc);
}

/*
 *@@ xstrFindReplaceC:
 *      wrapper around xstrFindReplace() which allows using
 *      C strings for the find and replace parameters.
 *
 *      This creates two temporary XSTRING's for pcszSearch
 *      and pcszReplace and thus cannot use the shift table
 *      for repetitive searches. As a result, this is slower
 *      than xstrFindReplace.
 *
 *      If you search with the same strings several times,
 *      you'll be better off using xstrFindReplace() directly.
 *
 *@@added V0.9.6 (2000-11-01) [umoeller]
 *@@changed V0.9.7 (2001-01-15) [umoeller]: renamed from xstrcrpl
 */

ULONG xstrFindReplaceC(PXSTRING pxstr,              // in/out: string
                       PULONG pulOfs,               // in: where to begin search (0 = start);
                                                    // out: ofs of first char after replacement string
                       const char *pcszSearch,      // in: search string; cannot be NULL
                       const char *pcszReplace)     // in: replacement string; cannot be NULL
{
    XSTRING xstrFind,
            xstrReplace;
    size_t  ShiftTable[256];
    BOOL    fRepeat = FALSE;
    // initialize find/replace strings... note that the
    // C strings are not free()'able, so we MUST NOT use xstrClear
    // before leaving
    xstrInitSet(&xstrFind, (PSZ)pcszSearch);
    xstrInitSet(&xstrReplace, (PSZ)pcszReplace);

    return (xstrFindReplace(pxstr, pulOfs, &xstrFind, &xstrReplace, ShiftTable, &fRepeat));
}

/*
 *@@ xstrConvertLineFormat:
 *      converts between line formats.
 *
 *      If (fToCFormat == TRUE), all \r\n pairs are replaced
 *      with \n chars (UNIX or C format).
 *
 *      Reversely, if (fToCFormat == FALSE), all \n chars
 *      are converted to \r\n pairs (DOS and OS/2 formats).
 *      No check is made whether this has already been done.
 *
 *@@added V0.9.7 (2001-01-15) [umoeller]
 */

VOID xstrConvertLineFormat(PXSTRING pxstr,
                           BOOL fToCFormat) // in: if TRUE, to C format; if FALSE, to OS/2 format.
{
    XSTRING     strFind,
                strRepl;
    size_t      ShiftTable[256];
    BOOL        fRepeat = FALSE;
    ULONG       ulOfs = 0;

    if (fToCFormat)
    {
        // OS/2 to C:
        xstrInitSet(&strFind, "\r\n");
        xstrInitSet(&strRepl, "\n");
    }
    else
    {
        // C to OS/2:
        xstrInitSet(&strFind, "\n");
        xstrInitSet(&strRepl, "\r\n");
    }

    while (xstrFindReplace(pxstr,
                           &ulOfs,
                           &strFind,
                           &strRepl,
                           ShiftTable,
                           &fRepeat))
            ;
}

// test case

/* int main(void)
{
    XSTRING str,
            strFind,
            strReplace;
    size_t  shift[256];
    BOOL    fRepeat = FALSE;
    ULONG   ulOfs = 0;

    xstrInit(&str, 100);
    xstrInit(&strFind, 0);
    xstrInit(&strReplace, 0);

    xstrcpy(&str, "Test string 1. Test string 2. Test string 3. !", 0);
    xstrcpy(&strFind, "Test", 0);
    xstrcpy(&strReplace, "Dummy", 0);

    printf("Old string is: \"%s\" (%d/%d)\n", str.psz, str.ulLength, str.cbAllocated);

    printf("Replacing \"%s\" with \"%s\".\n", strFind.psz, strReplace.psz);

    fRepeat = FALSE;
    ulOfs = 0;
    while (xstrFindReplace(&str,
                           &ulOfs,
                           &strFind,
                           &strReplace,
                           shift, &fRepeat));
        ;

    printf("New string is: \"%s\" (%d/%d)\n", str.psz, str.ulLength, str.cbAllocated);

    xstrcpy(&strFind, strReplace.psz, 0);
    xstrClear(&strReplace);

    printf("Replacing \"%s\" with \"%s\".\n", strFind.psz, strReplace.psz);

    fRepeat = FALSE;
    ulOfs = 0;
    while (xstrFindReplace(&str,
                   &ulOfs,
                   &strFind,
                   &strReplace,
                   shift, &fRepeat));
        ;

    printf("New string is: \"%s\" (%d/%d)\n", str.psz, str.ulLength, str.cbAllocated);

    xstrcpy(&strFind, " ", 0);
    xstrcpy(&strReplace, ".", 0);

    printf("Replacing \"%s\" with \"%s\".\n", strFind.psz, strReplace.psz);

    fRepeat = FALSE;
    ulOfs = 0;
    while (xstrFindReplace(&str,
                   &ulOfs,
                   &strFind,
                   &strReplace,
                   shift, &fRepeat));
        ;

    printf("New string is: \"%s\" (%d/%d)\n", str.psz, str.ulLength, str.cbAllocated);

    xstrcpy(&strFind, ".", 0);
    xstrcpy(&strReplace, "*.........................*", 0);

    printf("Replacing \"%s\" with \"%s\".\n", strFind.psz, strReplace.psz);

    fRepeat = FALSE;
    ulOfs = 0;
    while (xstrFindReplace(&str,
                   &ulOfs,
                   &strFind,
                   &strReplace,
                   shift, &fRepeat));
        ;

    printf("New string is: \"%s\" (%d/%d)\n", str.psz, str.ulLength, str.cbAllocated);

    printf("Reserving extra mem.\n");

    xstrReserve(&str, 6000);
    printf("New string is: \"%s\" (%d/%d)\n", str.psz, str.ulLength, str.cbAllocated);

    xstrcpy(&strFind, "..........", 0);
    xstrcpy(&strReplace, "@", 0);

    printf("Replacing \"%s\" with \"%s\".\n", strFind.psz, strReplace.psz);

    fRepeat = FALSE;
    ulOfs = 0;
    while (xstrFindReplace(&str,
                   &ulOfs,
                   &strFind,
                   &strReplace,
                   shift, &fRepeat));
        ;

    printf("New string is: \"%s\" (%d/%d)\n", str.psz, str.ulLength, str.cbAllocated);

    return (0);
}
*/


