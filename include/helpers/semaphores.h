
/*
 *@@sourcefile semaphores.h:
 *      header file for semaphores.c. See remarks there.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@include #define INCL_DOSSEMAPHORES
 *@@include #include <os2.h>
 *@@include #include "semaphores.h"
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

#if __cplusplus
extern "C" {
#endif

#ifndef SEMAPHORES_HEADER_INCLUDED
    #define SEMAPHORES_HEADER_INCLUDED

    typedef unsigned long HRW;
    typedef HRW *PHRW;

    APIRET semCreateRWMutex(PHRW phrw);

    APIRET semDeleteRWMutex(PHRW phrw);


#endif

#if __cplusplus
}
#endif

