/*
 *@@sourcefile threads.h:
 *      header file for treads.c. See remarks there.
 *
 *      Note: Version numbering in this file relates to XWorkplace version
 *            numbering.
 *
 *@@include #define INCL_DOSPROCESS
 *@@include #include <os2.h>
 *@@include #include "threads.h"
 */

/*
 *      Copyright (C) 1997-2000 Ulrich M”ller.
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

#ifndef TIMER_HEADER_INCLUDED
    #define TIMER_HEADER_INCLUDED

    USHORT APIENTRY tmrStartTimer(HWND hwnd, USHORT usTimerID, ULONG ulTimeout);
    typedef USHORT APIENTRY TMRSTARTTIMER(HWND hwnd, USHORT usTimerID, ULONG ulTimeout);
    typedef TMRSTARTTIMER *PTMRSTARTTIMER;

    BOOL APIENTRY tmrStopTimer(HWND hwnd, USHORT usTimerID);
    typedef BOOL APIENTRY TMRSTOPTIMER(HWND hwnd, USHORT usTimerID);
    typedef TMRSTOPTIMER *PTMRSTOPTIMER;

    VOID tmrStopAllTimers(HWND hwnd);
    typedef VOID TMRSTOPALLTIMERS(HWND hwnd);
    typedef TMRSTOPALLTIMERS *PTMRSTOPALLTIMERS;

#endif

#if __cplusplus
}
#endif

