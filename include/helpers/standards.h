
/*
 *@@sourcefile standards.h:
 *      some things that are always needed and never
 *      declared in a common place. Here you go.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@include #include "standards.h"
 */

/*      Copyright (C) 2001 Ulrich M”ller.
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

#ifndef STANDARDS_HEADER_INCLUDED
    #define STANDARDS_HEADER_INCLUDED

    /*
     *@@ NEW:
     *      wrapper around the typical malloc struct
     *      sequence.
     *
     *      Usage:
     *
     +          STRUCT *p = NEW(STRUCT);
     *
     *      This expands to:
     *
     +          STRUCT *p = (STRUCT *)malloc(sizeof(STRUCT));
     *
     *@@added V0.9.9 (2001-04-01) [umoeller]
     */

    #define NEW(type) (type *)malloc(sizeof(type))

    /*
     *@@ ZERO:
     *      wrapper around the typical zero-struct
     *      sequence.
     *
     *      Usage:
     *
     +          ZERO(p)
     *
     *      This expands to:
     *
     +          memset(p, 0, sizeof(*p));
     *
     *@@added V0.9.9 (2001-04-01) [umoeller]
     */

    #define ZERO(ptr) memset(ptr, 0, sizeof(*ptr))

    /*
     *@@ ARRAYITEMCOUNT:
     *      helpful macro to count the count of items
     *      in an array. Use this to avoid typos when
     *      having to pass the array item count to
     *      a function.
     *
     *      ULONG   aul[] = { 0, 1, 2, 3, 4 };
     *
     *      ARRAYITEMCOUNT(aul) then expands to:
     *
     +          sizeof(aul) / sizeof(aul[0])
     *
     *      which should return 5. Note that the compiler
     *      should calculate this at compile-time, so that
     *      there is no run-time overhead... and this will
     *      never miscount the array item size.
     *
     *@@added V0.9.9 (2001-01-29) [umoeller]
     */

    #define ARRAYITEMCOUNT(array) sizeof(array) / sizeof(array[0])

#endif


