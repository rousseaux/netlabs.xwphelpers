/*
 *@@sourcefile expat_setup.h:
 *      this is just for getting the damn calling conventions right.
 *      I have added the EXPATENTRY specifier to all function prototypes
 *      in expat (most importantly the callbacks) because otherwise
 *      the compiler will chose the calling convention, and this will
 *      be different depending on whether the func has been prototyped
 *      or not or if C or C++ is used.
 *
 *      If EXPATENTRY is not defined in the project's setup.h, it
 *      will be set to _Optlink here.
 */

#ifndef EXPAT_SETUP_H_INCLUDED
    #define EXPAT_SETUP_H_INCLUDED

    #ifndef EXPATENTRY
        #define EXPATENTRY _Optlink
    #endif

    #define XML_DTD 1

#endif


