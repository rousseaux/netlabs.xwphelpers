
/*
 *@@sourcefile encodings.c:
 *      character encoding translations.
 *
 *@@header "encodings\base.h"
 *@@added V0.9.9 (2001-02-14) [umoeller]
 */

/*
 *      Copyright (C) 2001 Ulrich M”ller.
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

#include <stdlib.h>
#include <string.h>

#include "setup.h"                      // code generation and debugging options

#include "encodings\base.h"             // includes all other encodings

#pragma hdrstop

typedef struct _ENCODINGTABLE
{
    unsigned short  cEntries;
    unsigned short  ausEntries[1];        // variable size
} ENCODINGTABLE, *PENCODINGTABLE;


/*
 *@@ encRegisterEncoding:
 *      registers a new proprietary encoding with the engine.
 *
 *      Before you can translate encodings with this engine,
 *      you have to register them. This makes sure that the
 *      big encoding tables will only be linked to the executable
 *      code if they are explicitly referenced. As a result, you
 *      have to #include "encodings\base.h" and pass a pointer to
 *      one of the global tables in the header files to this
 *      function.
 *
 *      This returns an encoding handle that can then be used
 *      with the other encoding functions.
 *
 *      Example:
 *
 +          #include "encodings\base.h"
 +          #include "encodings\alltables.h"     // or a specific table only
 +
 +          int rc = encRegisterEncoding(&G_iso8859_1,
 +                                       sizeof(G_iso8859_1) / sizeof(G_iso8859_1[0]),
 +                                       enc_iso8859_1);    // ID to register with
 */

long encRegisterEncoding(PXWPENCODINGMAP pEncodingMap,
                         unsigned long cArrayEntries,    // count of array items
                         XWPENCODINGID EncodingID)
{
    long lrc = 0;

    unsigned short usHighest = 0;
    unsigned long ul;

    // step 1:
    // run through the table and calculate the highest
    // character entry used
    for (ul = 0;
         ul < cArrayEntries;
         ul++)
    {
        unsigned short usFrom = pEncodingMap[ul].usFrom;
        if (usFrom > usHighest)
            usHighest = usFrom;
    }

    // step 2: allocate encoding table
    if (usHighest)
    {
        // allocate memory as needed
        unsigned long cb =   sizeof(ENCODINGTABLE)
                           + (   (usHighest - 1)
                               * sizeof(unsigned short)
                             );

        PENCODINGTABLE pTableNew = (PENCODINGTABLE)malloc(cb);
        if (pTableNew)
        {
            memset(pTableNew, -1, cb);
            pTableNew->cEntries = usHighest;        // array size

            // step 3: fill encoding table
            // this only has the Unicode target USHORTs;
            // the source is simply the offset. So to
            // get Unicode for character 123 in the specific encoding,
            // do pTableNew->ausEntries[123].
            // If you get 0xFFFF, the encoding is undefined.

            for (ul = 0;
                 ul < cArrayEntries;
                 ul++)
            {
                PXWPENCODINGMAP pEntry = &pEncodingMap[ul];
                pTableNew->ausEntries[pEntry->usFrom] = pEntry->usUni;
            }

            lrc = (long)pTableNew;
        }
    }

    return (lrc);
}


