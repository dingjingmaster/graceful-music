//
// Created by dingjing on 23-2-20.
//

#ifndef GRACEFUL_MUSIC_FORMAT_PRINT_H
#define GRACEFUL_MUSIC_FORMAT_PRINT_H
#include "../../gbuf.h"

typedef struct _FormatOption                FormatOption;
typedef struct _FormatPrintLen              FormatPrintLen;

struct _FormatOption
{
    union
    {
        /* NULL is treated like "" */
        const char*     fo_str;
        int             fo_int;
        /* [h:]mm:ss. can be negative */
        int             fo_time;
        double          fo_double;
    };
    /* set to 1 if you want to disable printing */
    unsigned int        empty : 1;
    /* set to 1 if zero padding is allowed */
    unsigned int        pad_zero : 1;
    enum
    {
        FO_STR = 1,
        FO_INT,
        FO_TIME,
        FO_DOUBLE
    } type;
    char                ch;
    const char*         str;
};

struct _FormatPrintLen
{
    int                 llen;
    int                 mlen;
    int                 rlen;
};


/* gcc < 4.6 and icc < 12.0 can't properly initialize anonymous unions */
#if (defined(__GNUC__) && defined(__GNUC_MINOR__) && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 6))) || (defined(__INTEL_COMPILER) && __INTEL_COMPILER < 1200)
#define UNION_INIT(f, v) { .f = v }
#else
#define UNION_INIT(f, v) .f = v
#endif

#define DEF_FO_STR(c, s, z)    { UNION_INIT(fo_str,  ""),   .type = FO_STR,    .pad_zero = z, .ch = c, .str = s }
#define DEF_FO_INT(c, s, z)    { UNION_INIT(fo_int,  0),    .type = FO_INT,    .pad_zero = z, .ch = c, .str = s }
#define DEF_FO_TIME(c, s, z)   { UNION_INIT(fo_time, 0),    .type = FO_TIME,   .pad_zero = z, .ch = c, .str = s }
#define DEF_FO_DOUBLE(c, s, z) { UNION_INIT(fo_double, 0.), .type = FO_DOUBLE, .pad_zero = z, .ch = c, .str = s }
#define DEF_FO_END             { .type = 0 }

FormatPrintLen format_print (struct gbuf *buf, int strWidth, const char* format, const FormatOption* fopts);
int format_valid (const char* format, const FormatOption* fopts);


#endif //GRACEFUL_MUSIC_FORMAT_PRINT_H
