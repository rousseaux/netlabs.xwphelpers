
/*
 *@@sourcefile math.c:
 *      some math helpers.
 *
 *      This file is new with V0.9.14 (2001-07-07) [umoeller]
 *      Unless marked otherwise, these things are based
 *      on public domain code found at
 *      "ftp://ftp.cdrom.com/pub/algorithms/c/".
 *
 *      Usage: All C programs.
 *
 *      Function prefix:
 *
 *      --  math*: semaphore helpers.
 *
 *@@added V0.9.14 (2001-07-07) [umoeller]
 *@@header "helpers\math.h"
 */

#define OS2EMX_PLAIN_CHAR
    // this is needed for "os2emx.h"; if this is defined,
    // emx will define PSZ as _signed_ char, otherwise
    // as unsigned char

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "setup.h"                      // code generation and debugging options

#include "helpers\math.h"
#include "helpers\linklist.h"
#include "helpers\standards.h"

#pragma hdrstop

/*
 *@@category: Helpers\Math helpers
 *      see math.c.
 */

/*
 *@@ mathGCD:
 *      returns the greatest common denominator that
 *      evenly divides m and n.
 *
 *      For example, mathGCD(10, 12) would return 2.
 */

int mathGCD(int m, int n)
{
    int d = m;
    int r = n;

    while (r != 0)
    {
        d = r;
        r = m % r;
    }

    return d;
}

/*
 *@@ mathIsSquare:
 *      returns 1 if x is a perfect square, that is, if
 *
 +      sqrt(x) ^ 2 ==x
 */

int mathIsSquare(int x)
{
    int t;
    int z = x % 4849845; // 4849845 = 3*5*7*11*13*17*19
    double r;
    // do some quick tests on x to see if we can quickly
    // eliminate it as a square using quadratic-residues.
    if (z % 3 == 2)
        return 0;

    t =  z % 5;
    if((t==2) || (t==3))
        return 0;
    t =  z % 7;
    if((t==3) || (t==5) || (t==6))
        return 0;
    t = z % 11;
    if((t==2) || (t==6) || (t==7) || (t==8) || (t==10))
        return 0;
    t = z % 13;
    if((t==2) || (t==5) || (t==6) || (t==7) || (t==8) || (t==11))
        return 0;
    t = z % 17;
    if((t==3) || (t==5) || (t==6) || (t==7) || (t==10) || (t==11) || (t==12) || (t==14))
        return 0;
    t = z % 19;
    if((t==2) || (t==3) || (t==8) || (t==10) || (t==12) || (t==13) || (t==14) || (t==15) || (t==18))
        return 0;

    // If we get here, we'll have to just try taking
    // square-root & compare its square...
    r = sqrt(abs(x));
    if (r*r == abs(x))
        return 1;

    return 0;
}

/*
 *@@ mathFindFactor:
 *      returns the smallest factor > 1 of n or 1 if n is prime.
 *
 *      From "http://tph.tuwien.ac.at/~oemer/doc/quprog/node28.html".
 */

int mathFindFactor(int n)
{
    int i,
        max;
    if (n <= 0)
        return 0;

    max = floor(sqrt(n));

    for (i=2;
         i <= max;
         i++)
    {
        if (n % i == 0)
            return i;
    }
    return 1;
}

/*
 *@@ testprime:
 *      returns 1 if n is a prime number.
 *
 *      From "http://tph.tuwien.ac.at/~oemer/doc/quprog/node28.html".
 */

int mathIsPrime(unsigned n)
{
    int i,
        max = floor(sqrt(n));

    if (n <= 1)
        return 0;

    for (i=2;
         i <= max;
         i++)
    {
        if (n % i == 0)
            return 0;
    }

    return 1;
}

/*
 *@@ mathFactorBrute:
 *      calls pfnCallback with every integer that
 *      evenly divides n ("factor").
 *
 *      pfnCallback must be declared as:
 *
 +          int pfnCallback(int iFactor,
 +                          int iPower,
 +                          void *pUser);
 *
 *      pfnCallback will receive the factor as its
 *      first parameter and pUser as its third.
 *      The second parameter will always be 1.
 *
 *      The factor will not necessarily be prime,
 *      and it will never be 1 nor n.
 *
 *      If the callback returns something != 0,
 *      computation is stopped.
 *
 *      Returns the no. of factors found or 0 if
 *      n is prime.
 *
 *      Example: mathFactor(42) will call the
 *      callback with
 *
 +          2 3 6 7 14 21
 *
 +      This func is terribly slow.
 */

int mathFactorBrute(int n,                           // in: integer to factor
                    PFNFACTORCALLBACK pfnCallback,   // in: callback func
                    void *pUser)                     // in: user param for callback
{
    int rc = 0;

    if (n > 2)
    {
        int i;
        int max = n / 2;

        for (i = 2;
             i <= max;
             i++)
        {
            if ((n % i) == 0)
            {
                rc++;
                // call callback with i as the factor
                if (pfnCallback(i,
                                1,
                                pUser))
                    // stop then
                    break;
            }
        }
    }

    return (rc);
}

/*
 *@@ mathFactorPrime:
 *      calls pfnCallback for every prime factor that
 *      evenly divides n.
 *
 *      pfnCallback must be declared as:
 *
 +          int pfnCallback(int iFactor,
 +                          int iPower,
 +                          void *pUser);
 *
 *      pfnCallback will receive the prime as its
 *      first parameter, its power as its second,
 *      and pUser as its third.
 *
 *      For example, with n = 200, pfnCallback will
 *      be called twice:
 *
 *      1)  iFactor = 2, iPower = 3
 *
 *      2)  iFactor = 5, iPower = 2
 *
 *      because 2^3 * 5^2 is 200.
 *
 *      Returns the no. of times that the callback
 *      was called. This will be the number of
 *      prime factors found or 0 if n is prime
 *      itself.
 */

int mathFactorPrime(double n,
                    PFNFACTORCALLBACK pfnCallback,   // in: callback func
                    void *pUser)                     // in: user param for callback
{
    int rc = 0;

    double d;
    double k;

    if (n <= 3)
       // this is a prime for sure
       return 0;

    d = 2;
    for (k = 0;
         fmod(n, d) == 0;
         k++)
    {
        n /= d;
    }

    if (k)
    {
        rc++;
        pfnCallback(d,
                    k,
                    pUser);
    }

    for (d = 3;
         d * d <= n;
         d += 2)
    {
        for (k = 0;
             fmod(n,d) == 0.0;
             k++ )
        {
            n /= d;
        }

        if (k)
        {
            rc++;
            pfnCallback(d,
                        k,
                        pUser);
        }
    }

    if (n > 1)
    {
        if (!rc)
            return (0);

        rc++;
        pfnCallback(n,
                    1,
                    pUser);
    }

    return (rc);
}

typedef struct _PRIMEDATA
{
    LINKLIST    llPrimes;
    int         iCurrentInt;
} PRIMEDATA, *PPRIMEDATA;


typedef struct _PRIMEENTRY
{
    int         iPrime;
    int         iLowestPowerFound;
                // lowest power that was found for this prime number;
                // if 0, a prime was previously present and then not
                // for a later prime
    int         iLastInt;
} PRIMEENTRY, *PPRIMEENTRY;

/*
 *@@ GCDMultiCallbackCreate:
 *      first callback for mathGCDMulti.
 */

int XWPENTRY GCDMultiCallbackCreate(int n,             // in: prime
                                    int p,             // in: power
                                    void *pUser)       // in: user param, here root of tree
{
    // see if we had this
    PPRIMEDATA pData = (PPRIMEDATA)pUser;
    PLISTNODE pNode;
    PPRIMEENTRY pEntry;
    for (pNode = lstQueryFirstNode(&pData->llPrimes);
         pNode;
         pNode = pNode->pNext)
    {
        pEntry = (PPRIMEENTRY)pNode->pItemData;
        if (pEntry->iPrime == n)
        {
            // mark this as processed so we can later see
            // if the current integer had this prime factor
            // at all
            pEntry->iLastInt = pData->iCurrentInt;

            // printf(" %d^%d", n, p);

            // and stop
            return 0;
        }
    }

    // no entry for this yet:
    // printf(" %d^%d", n, p);
    pEntry = NEW(PRIMEENTRY);
    pEntry->iPrime = n;
    pEntry->iLowestPowerFound = p;
    pEntry->iLastInt = pData->iCurrentInt;
    lstAppendItem(&pData->llPrimes, pEntry);

    return (0);
}

/*
 *@@ GCDMultiCallbackLowest:
 *      second callback for mathGCDMulti.
 */

int XWPENTRY GCDMultiCallbackLowest(int n,             // in: prime
                                    int p,             // in: power
                                    void *pUser)       // in: user param, here root of tree
{
    // see if we had this
    PPRIMEDATA pData = (PPRIMEDATA)pUser;
    PLISTNODE pNode;
    PPRIMEENTRY pEntry;
    for (pNode = lstQueryFirstNode(&pData->llPrimes);
         pNode;
         pNode = pNode->pNext)
    {
        pEntry = (PPRIMEENTRY)pNode->pItemData;
        if (pEntry->iPrime == n)
        {
            if (p < pEntry->iLowestPowerFound)
            {
                pEntry->iLowestPowerFound = p;
                // printf(" %d^%d", n, p);
            }
            pEntry->iLastInt = pData->iCurrentInt;

            // and stop
            return 0;
        }
    }

    exit(1);

    return (0);
}

/*
 *@@ mathGCDMulti:
 *      finds the greatest common divisor for a
 *      whole array of integers.
 *
 *      For example, if you pass in three integers
 *      1000, 1500, and 1800, this would return 100.
 *      If you only pass in 1000 and 1500, you'd
 *      get 500.
 *
 *      Potentially expensive since this calls
 *      mathFactorPrime for each integer in the
 *      array first.
 */

int mathGCDMulti(int *paNs,             // in: array of integers
                 int cNs)               // in: array item count (NOT array size)
{
    int i;
    double dGCD;

    PLISTNODE pNode;
    PPRIMEENTRY pEntry;

    PRIMEDATA Data;
    lstInit(&Data.llPrimes, TRUE);

    // this is done by prime-factoring each frequency
    // and then multiplying all primes with the lowest
    // common power that we found:

    // Example 1: If we have two timers with freq.
    // 1000 and 1500, obviously, the master timer
    // should run at 500 ms.

    // With prime factorization, we end up like this:

    // 1000 = 2^3 *       5^3
    // 1500 = 2^2 * 3^1 * 5^3

    // so the highest power for factor 2 is 2;
    //    the highest power for factor 3 is 0;
    //    the highest power for factor 5 is 3;

    // freq = 2^2 *       5^3 = 500

    // Example 2: If we have three timers with freq.
    // 1000 and 1500 and 1800:

    // 1000 = 2^3 *       5^3
    // 1500 = 2^2 * 3^1 * 5^3
    // 1800 = 2^3 * 3^2 * 5^2

    // so the highest power for factor 2 is 2;
    //    the highest power for factor 3 is 0;
    //    the highest power for factor 5 is 2;

    // freq = 2^2 *     * 5^2 = 100

    // Example 3: If we have four timers with freq.
    // 1200 and 1500 and 1800 and 60:

    // 1200 = 2^4 * 3^1 * 5^2
    // 1500 = 2^2 * 3^1 * 5^3
    // 1800 = 2^3 * 3^2 * 5^2
    //   60 = 2^2 * 3^1 * 5^1

    // so the highest power for factor 2 is 2;
    //    the highest power for factor 3 is 1;
    //    the highest power for factor 5 is 1;

    // freq = 2^2 * 3^1 * 5^1 = 60

    // go thru the ints array to create
    // an entry for each prime there is
    for (i = 0;
         i < cNs;
         i++)
    {
        // printf("\nFactoring %d\n", paNs[i]);
        Data.iCurrentInt = paNs[i];
        mathFactorPrime(paNs[i],
                        GCDMultiCallbackCreate,
                        &Data);
    }

    // now run this a second time to find the
    // lowest prime for each
    for (i = 0;
         i < cNs;
         i++)
    {
        // printf("\nTouching %d\n", paNs[i]);
        Data.iCurrentInt = paNs[i];
        mathFactorPrime(paNs[i],
                        GCDMultiCallbackLowest,
                        &Data);

        // all list items that were just touched
        // now have iLastInt set to the current
        // integer; so go thru the list and set
        // all items which do NOT have that set
        // to power 0 because we must not use them
        // in factoring
        for (pNode = lstQueryFirstNode(&Data.llPrimes);
             pNode;
             pNode = pNode->pNext)
        {
            pEntry = (PPRIMEENTRY)pNode->pItemData;
            if (pEntry->iLastInt != paNs[i])
            {
                pEntry->iLowestPowerFound = 0;
                // printf(" ###%d^%d", pEntry->iPrime, 0);
            }
        }
    }

    // printf("\nNew:\n");

    // now compose the GCD
    dGCD = 1;
    for (pNode = lstQueryFirstNode(&Data.llPrimes);
         pNode;
         pNode = pNode->pNext)
    {
        pEntry = (PPRIMEENTRY)pNode->pItemData;

        // printf(" %d^%d", pEntry->iPrime, pEntry->iLowestPowerFound);

        if (pEntry->iLowestPowerFound)
            dGCD *= pow(pEntry->iPrime, pEntry->iLowestPowerFound);
    }

    lstClear(&Data.llPrimes);

    return dGCD;
}

// testcase

#ifdef BUILD_MAIN

#define INCL_DOSMISC
#include <os2.h>

int callback(int n,
             int p,
             void *pUser)
{
    if (p > 1)
        printf("%d^%d ", n, p);
    else
        printf("%d ", n);
    fflush(stdout);

    return (0);
}

int main (int argc, char *argv[])
{
    if (argc > 1)
    {
        int i,
            c,
            cInts = argc - 1;
        ULONG ulms, ulms2;
        int *aInts = malloc(sizeof(int) * cInts);

        for (c = 0;
             c < cInts;
             c++)
        {
            aInts[c] = atoi(argv[c + 1]);
        }

        DosQuerySysInfo(QSV_MS_COUNT,
                        QSV_MS_COUNT,
                        &ulms,
                        sizeof(ulms));

        c = mathGCDMulti(aInts,
                         cInts);

        DosQuerySysInfo(QSV_MS_COUNT,
                        QSV_MS_COUNT,
                        &ulms2,
                        sizeof(ulms2));

        printf("\nGCD is %d, %d ms time.\n",
               c,
               ulms2 - ulms);

        for (i = 0;
             i < cInts;
             i++)
        {
            printf("  %d / %d = %d rem. %d\n",
                   aInts[i], c, aInts[i] / c, aInts[i] % c);
        }

        /* c = mathFactorBrute(atoi(argv[1]),
                            callback,
                            0);
        printf("\n%d factors found.\n", c); */
    }

    return (0);
}

#endif
