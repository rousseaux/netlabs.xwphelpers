
/*
 *@@sourcefile
 *
 *
 *@@added V0.9.9 (2001-02-10) [umoeller]
 */

#if __cplusplus
extern "C" {
#endif

#ifndef ENC_BASE_HEADER_INCLUDED
    #define ENC_BASE_HEADER_INCLUDED

    /*
     *@@ XWPENCODINGMAP:
     *      entry in an 8-bit to Unicode conversion table.
     */

    typedef struct _XWPENCODINGMAP
    {
        unsigned short      usFrom;
        unsigned short      usUni;
    } XWPENCODINGMAP, *PXWPENCODINGMAP;

    /*
     *@@ XWPENCODINGID:
     *      enum identifying each encoding set which is
     *      generally supported. Each ID corresponds to
     *      one header file in include\encodings\.
     */

    typedef enum _XWPENCODINGID
    {
        enc_cp437,
        enc_cp737,
        enc_cp775,
        enc_cp850,
        enc_cp852,
        enc_cp855,
        enc_cp857,
        enc_cp860,
        enc_cp861,
        enc_cp862,
        enc_cp863,
        enc_cp864,
        enc_cp865,
        enc_cp866,
        enc_cp869,
        enc_cp874,
        enc_cp932,
        enc_cp936,
        enc_cp949,
        enc_cp950,
        enc_cp1250,
        enc_cp1251,
        enc_cp1252,
        enc_cp1253,
        enc_cp1254,
        enc_cp1255,
        enc_cp1256,
        enc_cp1257,
        enc_cp1258,
        enc_iso8859_1,
        enc_iso8859_2,
        enc_iso8859_3,
        enc_iso8859_4,
        enc_iso8859_5,
        enc_iso8859_6,
        enc_iso8859_7,
        enc_iso8859_8,
        enc_iso8859_9,
        enc_iso8859_10,
        enc_iso8859_13,
        enc_iso8859_14,
        enc_iso8859_15
    } XWPENCODINGID;

    unsigned long encDecodeUTF8(const char **ppch);

#endif

#if __cplusplus
}
#endif

