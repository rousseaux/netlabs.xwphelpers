
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
 *         you should NEVER modify the psz pointer yourself
 *         because then these functions will get into trouble.
 *
 *         Also, you should never assume that the "psz"
 *         pointer has not changed after you have called
 *         one of the xstr* functions because these can
 *         always reallocate the buffer if needed.
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
 *      rewritten with V0.9.6 (2000-11-01).
 *
 *      Note: Version numbering in this file relates to XWorkplace
 *            version numbering.
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
 *      Do not use this on an XSTRING which is already
 *      initialized. Use xstrset instead.
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
 *      of an existing string.
 *
 *      Do not use this on an XSTRING which is already
 *      initialized. Use xstrcpy instead.
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
 *      appends pcszSource to pxstr, for which memory is allocated
 *      as necessary.
 *
 *      If pxstr is empty, this behaves just like xstrcpy.
 *
 *      Returns the length of the new string (excluding the null
 *      terminator), or null upon errors.
 *
 *      Example:
 *
 +          XSTRING str;
 +          xstrInit(&str, 0);
 +          xstrcpy(&str, "blah");
 +          xstrcat(&str, "blup");
 *
 *      would do the following:
 *      a)  free the old value of str ("blah");
 *      b)  reallocate str;
 *      c)  so that psz afterwards points to a new string containing
 *          "blahblup".
 *
 *@@changed V0.9.1 (99-12-20) [umoeller]: fixed memory leak
 *@@changed V0.9.1 (2000-01-03) [umoeller]: crashed if pszString was null; fixed
 *@@changed V0.9.2 (2000-04-01) [umoeller]: renamed from strhxcat
 *@@changed V0.9.3 (2000-05-11) [umoeller]: returned 0 if pszString was initially empty; fixed
 *@@changed V0.9.6 (2000-11-01) [umoeller]: rewritten
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
            strcpy(pxstr->psz + pxstr->ulLength,
                   pcszSource);

            // in all cases, set new length
            pxstr->ulLength += ulSourceLength;
            ulrc = ulSourceLength;
        }
        // else no source specified or source is empty:
        // do nothing
    }

    return (ulrc);
}

/*
 *@@ xstrrpl:
 *      replaces pstrSearch with pstrReplace in pxstr.
 *
 *      Starting with V0.9.6, this operates entirely on
 *      XSTRING's for speed because we then know the string
 *      lengths already and can use memcpy instead of strcpy.
 *      This new version should be magnitudes faster.
 *
 *      None of the pointers can be NULL, but if pstrReplace
 *      is empty, this effectively erases pstrSearch in pxstr.
 *
 *      Returns the length of the new string (exclusing the
 *      null terminator) or 0 if pszSearch was not found
 *      (and pxstr was therefore not changed).
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
 +          ULONG ulPos = 0;
 +          xstrInit(&str, 0);
 +          xstrcpy(&str, "Test phrase 1. Test phrase 2.");
 +          while (xstrrpl(&str,
 +                         ulPos,
 +                         "Test",      // search
 +                         "Dummy",     // replace
 +                         &ulPos))
 +              ;
 *
 *      would replace all occurences of "Test" in str with
 *      "Dummy".
 *
 *@@changed V0.9.0 [umoeller]: totally rewritten.
 *@@changed V0.9.0 (99-11-08) [umoeller]: crashed if *ppszBuf was NULL. Fixed.
 *@@changed V0.9.2 (2000-04-01) [umoeller]: renamed from strhxrpl
 *@@changed V0.9.6 (2000-11-01) [umoeller]: rewritten
 */

ULONG xstrrpl(PXSTRING pxstr,               // in/out: string
              ULONG ulOfs,                  // in: where to begin search (0 = start)
              const XSTRING *pstrSearch,    // in: search string; cannot be NULL
              const XSTRING *pstrReplace,   // in: replacement string; cannot be NULL
              PULONG pulAfterOfs)           // out: offset where found (ptr can be NULL)
{
    ULONG    ulrc = 0;

    if ((pxstr) && (pstrSearch) && (pstrReplace))
    {
        ULONG   cSearchLen = pstrSearch->ulLength;

        // can we search this?
        if (    (ulOfs < pxstr->ulLength)
             && (cSearchLen)
           )
        {
            // yes:
            PSZ     pFound = strstr(pxstr->psz + ulOfs,
                                    pstrSearch->psz);

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
                if (pulAfterOfs)
                    *pulAfterOfs = ulFoundOfs + cReplaceLen;
            }
        }
    }

    return (ulrc);
}

/*
 *@@ xstrcrpl:
 *      wrapper around xstrrpl which allows using C strings
 *      for the find and replace parameters.
 *
 *@@added V0.9.6 (2000-11-01) [umoeller]
 */

ULONG xstrcrpl(PXSTRING pxstr,              // in/out: string
               ULONG ulOfs,                 // in: where to begin search (0 = start)
               const char *pcszSearch,      // in: search string; cannot be NULL
               const char *pcszReplace,     // in: replacement string; cannot be NULL
               PULONG pulAfterOfs)          // out: offset where found (ptr can be NULL)
{
    ULONG   ulrc = 0;
    XSTRING xstrFind,
            xstrReplace;
    xstrInit(&xstrFind, 0);
    xstrset(&xstrFind, (PSZ)pcszSearch);
    xstrInit(&xstrReplace, 0);
    xstrset(&xstrReplace, (PSZ)pcszReplace);

    return (xstrrpl(pxstr, ulOfs, &xstrFind, &xstrReplace, pulAfterOfs));
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

