
;/*
; * Interlocked apis
; *
; * Copyright 1999 Sander van Leeuwen
; * Copyright 1999 Patrick Haller
; *
; * Based on WINE code: win32\thread.c (990815)
; *
; * Copyright 1995 Martin von Loewis
; * Copyright 1997 Onno Hovers
; *
; * Project Odin Software License can be found in LICENSE.TXT
; *
; */
.586P
               NAME    interlocked

CODE32         SEGMENT DWORD PUBLIC USE32 'CODE'

;* LONG DosInterlockedIncrement(PLONG);
;*
;* InterlockedIncrement adds 1 to a long variable and returns
;*  -  a negative number if the result < 0
;*  -  zero if the result == 0
;*  -  a positive number if the result > 0
;*
;* The returned number need not be equal to the result!!!!

public  DosInterlockedIncrement
DosInterlockedIncrement proc near
        ;we are allowed to trash edx
        mov     edx, dword ptr [esp+4] ; LPLONG lpAddend
        mov     eax, 1
        lock    xadd dword ptr [edx], eax
        inc     eax
        ret     4
DosInterlockedIncrement endp

;* LONG DosInterlockedDecrement(PLONG);
;*
;* InterlockedIncrement adds 1 to a long variable and returns
;*  -  a negative number if the result < 0
;*  -  zero if the result == 0
;*  -  a positive number if the result > 0
;*
;* The returned number need not be equal to the result!!!!

public  DosInterlockedDecrement
DosInterlockedDecrement proc near
        ;we are allowed to trash edx
        mov     edx, dword ptr [esp+4] ; LPLONG lpAddend
        mov     eax, -1
        lock    xadd dword ptr [edx], eax
        dec     eax
        ret     4
DosInterlockedDecrement endp


; * LONG DosInterlockedExchange(PLONG, LONG);
; *
; * Atomically exchanges a pair of values.
; *
; * RETURNS
; * Prior value of value pointed to by Target

public  DosInterlockedExchange
DosInterlockedExchange proc near
        push    edx
        mov     eax, [esp+12]           ; LONG value
        mov     edx,[esp+8]             ; LPLONG target
        lock    xchg eax, dword ptr [edx]
        pop     edx
        ret     8
DosInterlockedExchange endp


;/************************************************************************
; * LONG DosInterlockedCompareExchange(PLONG dest, LONG xchg, LONG compare)
; *
; * Atomically compares Destination and Comperand, and if found equal exchanges
; * the value of Destination with Exchange
; *
; */

public DosInterlockedCompareExchange
DosInterlockedCompareExchange proc near
        push    ebp
        mov     ebp, esp
        push    edx
        push    ebx

        mov     ebx, dword ptr [ebp+8]  ;PVOID *Destination, /* Address of 32-bit value to exchange */
        mov     eax, [ebp+16]       ;PVOID Comperand      /* value to compare, 32 bits */
        mov     edx, [ebp+12]       ;PVOID Exchange,      /* change value, 32 bits */
        lock    cmpxchg dword ptr [ebx],edx

        pop     ebx
        pop     edx
        pop     ebp
        ret     12
DosInterlockedCompareExchange endp

; * LONG DosInterlockedExchangeAdd(PLONG dest, LONG incr);
; *
; * Atomically adds Increment to Addend and returns the previous value of
; * Addend
; *
; * RETURNS
; * Prior value of value pointed to by cwAddendTarget
; */

public DosInterlockedExchangeAdd
DosInterlockedExchangeAdd proc near
        push    edx
        mov     eax, dword ptr [esp+12] ;LONG Increment /* Value to add */
        mov     edx, dword ptr [esp+8]  ;PLONG Addend, /* Address of 32-bit value to exchange */
        lock    xadd dword ptr [edx], eax
        pop     edx
        ret     8
DosInterlockedExchangeAdd endp

CODE32          ENDS

                END
