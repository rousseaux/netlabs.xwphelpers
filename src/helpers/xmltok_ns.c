
/*
 *@@sourcefile xmltok_ns.c
 *      part of the expat implementation. See xmlparse.c.
 *
 *      NOTE: This file must not be compiled directly. It is
 *      #include'd from xmltok.c several times.
 */

/*
 *      Copyright (C) 2001 Ulrich M�ller.
 *      Copyright (c) 1998, 1999, 2000 Thai Open Source Software Center Ltd.
 *                                     and Clark Cooper.
 *
 *      Permission is hereby granted, free of charge, to any person obtaining
 *      a copy of this software and associated documentation files (the
 *      "Software"), to deal in the Software without restriction, including
 *      without limitation the rights to use, copy, modify, merge, publish,
 *      distribute, sublicense, and/or sell copies of the Software, and to
 *      permit persons to whom the Software is furnished to do so, subject to
 *      the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included
 *      in all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *      EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

const ENCODING* EXPATENTRY NS(XmlGetUtf8InternalEncoding) (void)
{
    return &ns(internal_utf8_encoding).enc;
}

const ENCODING* EXPATENTRY NS(XmlGetUtf16InternalEncoding) (void)
{
#if XML_BYTE_ORDER == 12
    return &ns(internal_little2_encoding).enc;
#elif XML_BYTE_ORDER == 21
    return &ns(internal_big2_encoding).enc;
#else
    const short n = 1;

    return *(const char *)&n ? &ns(internal_little2_encoding).enc : &ns(internal_big2_encoding).enc;
#endif
}

static
const ENCODING *NS(encodings)[] =
{
    &ns(latin1_encoding).enc,
        &ns(ascii_encoding).enc,
        &ns(utf8_encoding).enc,
        &ns(big2_encoding).enc,
        &ns(big2_encoding).enc,
        &ns(little2_encoding).enc,
        &ns(utf8_encoding).enc  /* NO_ENC */
};

static int EXPATENTRY NS(initScanProlog)(const ENCODING * enc,
                                          const char *ptr,
                                          const char *end,
                                          const char **nextTokPtr)
{
    return initScan(NS(encodings), (const INIT_ENCODING *)enc, XML_PROLOG_STATE, ptr, end, nextTokPtr);
}

static int EXPATENTRY NS(initScanContent)(const ENCODING * enc,
                                           const char *ptr,
                                           const char *end,
                                           const char **nextTokPtr)
{
    return initScan(NS(encodings), (const INIT_ENCODING *)enc, XML_CONTENT_STATE, ptr, end, nextTokPtr);
}

int EXPATENTRY NS(XmlInitEncoding)(INIT_ENCODING * p, const ENCODING ** encPtr, const char *name)
{
    int i = getEncodingIndex(name);

    if (i == UNKNOWN_ENC)
        return 0;
    SET_INIT_ENC_INDEX(p, i);
    p->initEnc.scanners[XML_PROLOG_STATE] = NS(initScanProlog);
    p->initEnc.scanners[XML_CONTENT_STATE] = NS(initScanContent);
    p->initEnc.updatePosition = initUpdatePosition;
    p->encPtr = encPtr;
    *encPtr = &(p->initEnc);
    return 1;
}

static const ENCODING* EXPATENTRY NS(findEncoding)(const ENCODING * enc,
                                                   const char *ptr,
                                                   const char *end)
{
#define ENCODING_MAX 128
    char buf[ENCODING_MAX];
    char *p = buf;
    int i;

    XmlUtf8Convert(enc, &ptr, end, &p, p + ENCODING_MAX - 1);
    if (ptr != end)
        return 0;
    *p = 0;
    if (streqci(buf, KW_UTF_16) && enc->minBytesPerChar == 2)
        return enc;
    i = getEncodingIndex(buf);
    if (i == UNKNOWN_ENC)
        return 0;
    return NS(encodings)[i];
}

int NS(XmlParseXmlDecl) (int isGeneralTextEntity,
                         const ENCODING * enc,
                         const char *ptr,
                         const char *end,
                         const char **badPtr,
                         const char **versionPtr,
                         const char **versionEndPtr,
                         const char **encodingName,
                         const ENCODING ** encoding,
                         int *standalone)
{
    return doParseXmlDecl(NS(findEncoding),
                          isGeneralTextEntity,
                          enc,
                          ptr,
                          end,
                          badPtr,
                          versionPtr,
                          versionEndPtr,
                          encodingName,
                          encoding,
                          standalone);
}
