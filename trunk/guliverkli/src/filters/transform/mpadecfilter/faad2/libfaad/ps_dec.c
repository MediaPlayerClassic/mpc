/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR and PS decoding
** Copyright (C) 2003-2004 M. Bakker, Ahead Software AG, http://www.nero.com
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Ahead Software through Mpeg4AAClicense@nero.com.
**
** $Id$
**/

#include "common.h"

#ifdef PS_DEC

#include <stdlib.h>
#include <memory.h>
#include "ps_dec.h"
#include "ps_tables.h"

/* constants */
#define NEGATE_IPD_MASK            (0x1000)
#define DECAY_SLOPE                FRAC_CONST(0.05)
#define COEF_SQRT2                 COEF_CONST(sqrt(2.0))

/* tables */
/* filters are mirrored in coef 6, second half left out */
static const real_t p8_13_20[7] =
{
    FRAC_CONST(0.00746082949812),
    FRAC_CONST(0.02270420949825),
    FRAC_CONST(0.04546865930473),
    FRAC_CONST(0.07266113929591),
    FRAC_CONST(0.09885108575264),
    FRAC_CONST(0.11793710567217),
    FRAC_CONST(0.125)
};

static const real_t p2_13_20[7] =
{
    FRAC_CONST(0.0),
    FRAC_CONST(0.01899487526049),
    FRAC_CONST(0.0),
    FRAC_CONST(-0.07293139167538),
    FRAC_CONST(0.0),
    FRAC_CONST(0.30596630545168),
    FRAC_CONST(0.5)
};

static const real_t p12_13_34[7] =
{
    FRAC_CONST(0.04081179924692),
    FRAC_CONST(0.03812810994926),
    FRAC_CONST(0.05144908135699),
    FRAC_CONST(0.06399831151592),
    FRAC_CONST(0.07428313801106),
    FRAC_CONST(0.08100347892914),
    FRAC_CONST(0.08333333333333)
};

static const real_t p8_13_34[7] =
{
    FRAC_CONST(0.01565675600122),
    FRAC_CONST(0.03752716391991),
    FRAC_CONST(0.05417891378782),
    FRAC_CONST(0.08417044116767),
    FRAC_CONST(0.10307344158036),
    FRAC_CONST(0.12222452249753),
    FRAC_CONST(0.125)
};

static const real_t p4_13_34[7] =
{
    FRAC_CONST(-0.05908211155639),
    FRAC_CONST(-0.04871498374946),
    FRAC_CONST(0.0),
    FRAC_CONST(0.07778723915851),
    FRAC_CONST(0.16486303567403),
    FRAC_CONST(0.23279856662996),
    FRAC_CONST(0.25)
};

static const uint8_t delay_length_d[2][NO_ALLPASS_LINKS] = {
    { 1, 2, 3 } /* d_24kHz */,
    { 3, 4, 5 } /* d_48kHz */
};
static const real_t filter_a[NO_ALLPASS_LINKS] = { /* a(m) = exp(-d_48kHz(m)/7) */
    FRAC_CONST(0.65143905753106),
    FRAC_CONST(0.56471812200776),
    FRAC_CONST(0.48954165955695)
};

static const uint8_t group_border20[10+12 + 1] =
{
    6, 7, 0, 1, 2, 3, /* 6 subqmf subbands */
    9, 8,             /* 2 subqmf subbands */
    10, 11,           /* 2 subqmf subbands */
    3, 4, 5, 6, 7, 8, 9, 11, 14, 18, 23, 35, 64
};

static const uint8_t group_border34[32+18 + 1] =
{
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, /* 12 subqmf subbands */
     12, 13, 14, 15, 16, 17, 18, 19,                 /*  8 subqmf subbands */
     20, 21, 22, 23,                                 /*  4 subqmf subbands */
     24, 25, 26, 27,                                 /*  4 subqmf subbands */
     28, 29, 30, 31,                                 /*  4 subqmf subbands */
     32-27, 33-27, 34-27, 35-27, 36-27, 37-27, 38-27, 40-27, 42-27, 44-27, 46-27, 48-27, 51-27, 54-27, 57-27, 60-27, 64-27, 68-27, 91-27
};

static const uint16_t map_bins2group20[10+12] =
{
    (NEGATE_IPD_MASK | 1), (NEGATE_IPD_MASK | 0),
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19
};

static const uint16_t map_bins2group34[32+18] =
{
    0,  1,  2,  3,  4,  5,  6,  6,  7, (NEGATE_IPD_MASK | 2), (NEGATE_IPD_MASK | 1), (NEGATE_IPD_MASK | 0),
    10, 10, 4,  5,  6,  7,  8,  9,
    10, 11, 12, 9,
    14, 11, 12, 13,
    14, 15, 16, 13,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33
};

/* type definitions */
typedef struct
{
    uint8_t frame_len;
    uint8_t resolution20[3];
    uint8_t resolution34[5];

    qmf_t *work;
    qmf_t **buffer;
    qmf_t **temp;
} hyb_info;

/* static function declarations */
static void ps_data_decode(ps_info *ps);
static hyb_info *hybrid_init();
static void channel_filter2(hyb_info *hyb, uint8_t frame_len, const real_t *filter,
                            qmf_t *buffer, qmf_t **X_hybrid);
static void INLINE DCT3_4_unscaled(real_t *y, real_t *x);
static void channel_filter8(hyb_info *hyb, uint8_t frame_len, const real_t *filter,
                            qmf_t *buffer, qmf_t **X_hybrid);
static void hybrid_analysis(hyb_info *hyb, qmf_t X[32][64], qmf_t X_hybrid[32][32],
                            uint8_t use34);
static void hybrid_synthesis(hyb_info *hyb, qmf_t X[32][64], qmf_t X_hybrid[32][32],
                             uint8_t use34);
static int8_t delta_clip(int8_t i, int8_t min, int8_t max);
static void delta_decode(uint8_t enable, int8_t *index, uint8_t *index_prev,
                         uint8_t dt_flag, uint8_t nr_par, uint8_t stride,
                         int8_t min_index, int8_t max_index);
static void map20indexto34(int8_t *index, uint8_t bins);
#ifdef PS_LOW_POWER
static void map34indexto20(int8_t *index, uint8_t bins);
#endif
static void ps_data_decode(ps_info *ps);
static void ps_decorrelate(ps_info *ps, qmf_t X_left[38][64], qmf_t X_right[38][64],
                           qmf_t X_hybrid_left[32][32], qmf_t X_hybrid_right[32][32]);
static void ps_mix_phase(ps_info *ps, qmf_t X_left[38][64], qmf_t X_right[38][64],
                         qmf_t X_hybrid_left[32][32], qmf_t X_hybrid_right[32][32]);

/*  */


static hyb_info *hybrid_init()
{
    uint8_t i;

    hyb_info *hyb = (hyb_info*)faad_malloc(sizeof(hyb_info));

    hyb->resolution34[0] = 12;
    hyb->resolution34[1] = 8;
    hyb->resolution34[2] = 4;
    hyb->resolution34[3] = 4;
    hyb->resolution34[4] = 4;

    hyb->resolution20[0] = 8;
    hyb->resolution20[1] = 2;
    hyb->resolution20[2] = 2;

    hyb->frame_len = 32;

    hyb->work = (qmf_t*)faad_malloc((hyb->frame_len+12) * sizeof(qmf_t));
    memset(hyb->work, 0, (hyb->frame_len+12) * sizeof(qmf_t));

    hyb->buffer = (qmf_t**)faad_malloc(5 * sizeof(qmf_t*));
    for (i = 0; i < 5; i++)
    {
        hyb->buffer[i] = (qmf_t*)faad_malloc(hyb->frame_len * sizeof(qmf_t));
        memset(hyb->buffer[i], 0, hyb->frame_len * sizeof(qmf_t));
    }

    hyb->temp = (qmf_t**)faad_malloc(hyb->frame_len * sizeof(qmf_t*));
    for (i = 0; i < hyb->frame_len; i++)
    {
        hyb->temp[i] = (qmf_t*)faad_malloc(12 /*max*/ * sizeof(qmf_t));
    }

    return hyb;
}

/* real filter, size 2 */
static void channel_filter2(hyb_info *hyb, uint8_t frame_len, const real_t *filter,
                            qmf_t *buffer, qmf_t **X_hybrid)
{
    uint8_t i;

    for (i = 0; i < frame_len; i++)
    {
        real_t r0 = MUL_F(filter[0],(QMF_RE(buffer[0+i]) + QMF_RE(buffer[12+i])));
        real_t r1 = MUL_F(filter[1],(QMF_RE(buffer[1+i]) + QMF_RE(buffer[11+i])));
        real_t r2 = MUL_F(filter[2],(QMF_RE(buffer[2+i]) + QMF_RE(buffer[10+i])));
        real_t r3 = MUL_F(filter[3],(QMF_RE(buffer[3+i]) + QMF_RE(buffer[9+i])));
        real_t r4 = MUL_F(filter[4],(QMF_RE(buffer[4+i]) + QMF_RE(buffer[8+i])));
        real_t r5 = MUL_F(filter[5],(QMF_RE(buffer[5+i]) + QMF_RE(buffer[7+i])));
        real_t r6 = MUL_F(filter[6],QMF_RE(buffer[6+i]));
        real_t i0 = MUL_F(filter[0],(QMF_IM(buffer[0+i]) + QMF_IM(buffer[12+i])));
        real_t i1 = MUL_F(filter[1],(QMF_IM(buffer[1+i]) + QMF_IM(buffer[11+i])));
        real_t i2 = MUL_F(filter[2],(QMF_IM(buffer[2+i]) + QMF_IM(buffer[10+i])));
        real_t i3 = MUL_F(filter[3],(QMF_IM(buffer[3+i]) + QMF_IM(buffer[9+i])));
        real_t i4 = MUL_F(filter[4],(QMF_IM(buffer[4+i]) + QMF_IM(buffer[8+i])));
        real_t i5 = MUL_F(filter[5],(QMF_IM(buffer[5+i]) + QMF_IM(buffer[7+i])));
        real_t i6 = MUL_F(filter[6],QMF_IM(buffer[6+i]));

        /* q = 0 */
        QMF_RE(X_hybrid[i][0]) = r0 + r1 + r2 + r3 + r4 + r5 + r6;
        QMF_IM(X_hybrid[i][0]) = i0 + i1 + i2 + i3 + i4 + i5 + i6;

        /* q = 1 */
        QMF_RE(X_hybrid[i][1]) = r0 - r1 + r2 - r3 + r4 - r5 + r6;
        QMF_IM(X_hybrid[i][1]) = i0 - i1 + i2 - i3 + i4 - i5 + i6;
    }
}

static void INLINE DCT3_4_unscaled(real_t *y, real_t *x)
{
    real_t f0, f1, f2, f3, f4, f5, f6, f7, f8;

    f0 = MUL_F(x[2], FRAC_CONST(0.7071067811865476));
    f1 = x[0] - f0;
    f2 = x[0] + f0;
    f3 = x[1] + x[3];
    f4 = MUL_C(x[1], COEF_CONST(1.3065629648763766));
    f5 = MUL_F(f3, FRAC_CONST(-0.9238795325112866));
    f6 = MUL_F(x[3], FRAC_CONST(-0.5411961001461967));
    f7 = f4 + f5;
    f8 = f6 - f5;
    y[3] = f2 - f8;
    y[0] = f2 + f8;
    y[2] = f1 - f7;
    y[1] = f1 + f7;
}

/* complex filter, size 8 */
static void channel_filter8(hyb_info *hyb, uint8_t frame_len, const real_t *filter,
                            qmf_t *buffer, qmf_t **X_hybrid)
{
    uint8_t i, n;
    real_t input_re1[4], input_re2[4], input_im1[4], input_im2[4];
    real_t x[4];

    for (i = 0; i < frame_len; i++)
    {
        input_re1[0] =  MUL_F(filter[6],QMF_RE(buffer[6+i]));
        input_re1[1] =  MUL_F(filter[5],(QMF_RE(buffer[5+i]) + QMF_RE(buffer[7+i])));
        input_re1[2] = -MUL_F(filter[0],(QMF_RE(buffer[0+i]) + QMF_RE(buffer[12+i]))) + MUL_F(filter[4],(QMF_RE(buffer[4+i]) + QMF_RE(buffer[8+i])));
        input_re1[3] = -MUL_F(filter[1],(QMF_RE(buffer[1+i]) + QMF_RE(buffer[11+i]))) + MUL_F(filter[3],(QMF_RE(buffer[3+i]) + QMF_RE(buffer[9+i])));

        input_im1[0] = MUL_F(filter[5],(QMF_IM(buffer[7+i]) - QMF_IM(buffer[5+i])));
        input_im1[1] = MUL_F(filter[0],(QMF_IM(buffer[12+i]) - QMF_IM(buffer[0+i]))) + MUL_F(filter[4],(QMF_IM(buffer[8+i]) - QMF_IM(buffer[4+i])));
        input_im1[2] = MUL_F(filter[1],(QMF_IM(buffer[11+i]) - QMF_IM(buffer[1+i]))) + MUL_F(filter[3],(QMF_IM(buffer[9+i]) - QMF_IM(buffer[3+i])));
        input_im1[3] = MUL_F(filter[2],(QMF_IM(buffer[10+i]) - QMF_IM(buffer[2+i])));

        for (n = 0; n < 4; n++)
        {
            x[n] = input_re1[n] - input_im1[3-n];
        }
        DCT3_4_unscaled(x, x);
        QMF_RE(X_hybrid[i][7]) = x[0];
        QMF_RE(X_hybrid[i][5]) = x[2];
        QMF_RE(X_hybrid[i][3]) = x[3];
        QMF_RE(X_hybrid[i][1]) = x[1];

        for (n = 0; n < 4; n++)
        {
            x[n] = input_re1[n] + input_im1[3-n];
        }
        DCT3_4_unscaled(x, x);
        QMF_RE(X_hybrid[i][6]) = x[1];
        QMF_RE(X_hybrid[i][4]) = x[3];
        QMF_RE(X_hybrid[i][2]) = x[2];
        QMF_RE(X_hybrid[i][0]) = x[0];

        input_im2[0] =  MUL_F(filter[6],QMF_IM(buffer[6+i]));
        input_im2[1] =  MUL_F(filter[5],(QMF_IM(buffer[5+i]) + QMF_IM(buffer[7+i])));
        input_im2[2] = -MUL_F(filter[0],(QMF_IM(buffer[0+i]) + QMF_IM(buffer[12+i]))) + MUL_F(filter[4],(QMF_IM(buffer[4+i]) + QMF_IM(buffer[8+i])));
        input_im2[3] = -MUL_F(filter[1],(QMF_IM(buffer[1+i]) + QMF_IM(buffer[11+i]))) + MUL_F(filter[3],(QMF_IM(buffer[3+i]) + QMF_IM(buffer[9+i])));

        input_re2[0] = MUL_F(filter[5],(QMF_RE(buffer[7+i]) - QMF_RE(buffer[5+i])));
        input_re2[1] = MUL_F(filter[0],(QMF_RE(buffer[12+i]) - QMF_RE(buffer[0+i]))) + MUL_F(filter[4],(QMF_RE(buffer[8+i]) - QMF_RE(buffer[4+i])));
        input_re2[2] = MUL_F(filter[1],(QMF_RE(buffer[11+i]) - QMF_RE(buffer[1+i]))) + MUL_F(filter[3],(QMF_RE(buffer[9+i]) - QMF_RE(buffer[3+i])));
        input_re2[3] = MUL_F(filter[2],(QMF_RE(buffer[10+i]) - QMF_RE(buffer[2+i])));

        for (n = 0; n < 4; n++)
        {
            x[n] = input_im2[n] + input_re2[3-n];
        }
        DCT3_4_unscaled(x, x);
        QMF_IM(X_hybrid[i][7]) = x[0];
        QMF_IM(X_hybrid[i][5]) = x[2];
        QMF_IM(X_hybrid[i][3]) = x[3];
        QMF_IM(X_hybrid[i][1]) = x[1];

        for (n = 0; n < 4; n++)
        {
            x[n] = input_im2[n] - input_re2[3-n];
        }
        DCT3_4_unscaled(x, x);
        QMF_IM(X_hybrid[i][6]) = x[1];
        QMF_IM(X_hybrid[i][4]) = x[3];
        QMF_IM(X_hybrid[i][2]) = x[2];
        QMF_IM(X_hybrid[i][0]) = x[0];
    }
}

/* Hybrid analysis: further split up QMF subbands
 * to improve frequency resolution
 */
static void hybrid_analysis(hyb_info *hyb, qmf_t X[32][64], qmf_t X_hybrid[32][32],
                            uint8_t use34)
{
    uint8_t k, n, band;
    uint8_t offset = 0;
    uint8_t qmf_bands = (use34) ? 5 : 3;
    uint8_t *resolution = (use34) ? hyb->resolution34 : hyb->resolution20;

    for (band = 0; band < qmf_bands; band++)
    {
        /* build working buffer */
        memcpy(hyb->work, hyb->buffer[band], 12 * sizeof(qmf_t));

        /* add new samples */
        for (n = 0; n < hyb->frame_len; n++)
        {
            QMF_RE(hyb->work[12 + n]) = QMF_RE(X[n + 6 /*delay*/][band]);
            QMF_IM(hyb->work[12 + n]) = QMF_IM(X[n + 6 /*delay*/][band]);
        }

        /* store samples */
        memcpy(hyb->buffer[band], hyb->work + hyb->frame_len, 12 * sizeof(qmf_t));


        switch(resolution[band])
        {
        case 2:
            channel_filter2(hyb, hyb->frame_len, p2_13_20, hyb->work, hyb->temp);
            break;
        case 4:
            channel_filter2(hyb, hyb->frame_len, p4_13_34, hyb->work, hyb->temp);
            break;
        case 8:
            channel_filter8(hyb, hyb->frame_len, (use34) ? p8_13_34 : p8_13_20,
                hyb->work, hyb->temp);
            break;
        case 12:
            channel_filter2(hyb, hyb->frame_len, p12_13_34, hyb->work, hyb->temp);
            break;
        }

        for (n = 0; n < hyb->frame_len; n++)
        {
            for (k = 0; k < resolution[band]; k++)
            {
                QMF_RE(X_hybrid[n][offset + k]) = QMF_RE(hyb->temp[n][k]);
                QMF_IM(X_hybrid[n][offset + k]) = QMF_IM(hyb->temp[n][k]);
            }
        }
        offset += resolution[band];
    }

    /* group hybrid channels */
    if (!use34)
    {
        for (n = 0; n < 32 /*30?*/; n++)
        {
            QMF_RE(X_hybrid[n][3]) += QMF_RE(X_hybrid[n][4]);
            QMF_IM(X_hybrid[n][3]) += QMF_IM(X_hybrid[n][4]);
            QMF_RE(X_hybrid[n][4]) = 0;
            QMF_IM(X_hybrid[n][4]) = 0;

            QMF_RE(X_hybrid[n][2]) += QMF_RE(X_hybrid[n][5]);
            QMF_IM(X_hybrid[n][2]) += QMF_IM(X_hybrid[n][5]);
            QMF_RE(X_hybrid[n][5]) = 0;
            QMF_IM(X_hybrid[n][5]) = 0;
        }
    }
}

static void hybrid_synthesis(hyb_info *hyb, qmf_t X[32][64], qmf_t X_hybrid[32][32],
                             uint8_t use34)
{
    uint8_t k, n, band;
    uint8_t offset = 0;
    uint8_t qmf_bands = (use34) ? 5 : 3;
    uint8_t *resolution = (use34) ? hyb->resolution34 : hyb->resolution20;

    for(band = 0; band < qmf_bands; band++)
    {
        for (n = 0; n < hyb->frame_len; n++)
        {
            QMF_RE(X[n][band]) = 0;
            QMF_IM(X[n][band]) = 0;

            for (k = 0; k < resolution[band]; k++)
            {
                QMF_RE(X[n][band]) += QMF_RE(X_hybrid[n][offset + k]);
                QMF_IM(X[n][band]) += QMF_IM(X_hybrid[n][offset + k]);
            }
        }
        offset += resolution[band];
    }
}

/* limits the value i to the range [min,max] */
static int8_t delta_clip(int8_t i, int8_t min, int8_t max)
{
    if (i < min)
        return min;
    else if (i > max)
        return max;
    else
        return i;
}

/* delta decode array */
static void delta_decode(uint8_t enable, int8_t *index, uint8_t *index_prev,
                         uint8_t dt_flag, uint8_t nr_par, uint8_t stride,
                         int8_t min_index, int8_t max_index)
{
    int8_t i;

    if (enable == 1)
    {
        if (dt_flag == 0)
        {
            /* delta coded in frequency direction */
            index[0] = 0 + index[0];
            index[0] = delta_clip(index[0], min_index, max_index);

            for (i = 1; i < nr_par; i++)
            {
                index[i] = index[i-1] + index[i];
                index[i] = delta_clip(index[i], min_index, max_index);
            }
        } else {
            /* delta coded in time direction */
            for (i = 0; i < nr_par; i++)
            {
                index[i] = index_prev[i*stride] + index[i];
                index[i] = delta_clip(index[i], min_index, max_index);
            }
        }
    } else {
        /* set indices to zero */
        for (i = 0; i < nr_par; i++)
        {
            index[i] = 0;
        }
    }

    /* coarse */
    if (stride == 2)
    {
        for (i = (nr_par<<1)-1; i > 0; i--)
        {
            index[i] = index[i>>1];
        }
    }
}

#ifdef PS_LOW_POWER
static void map34indexto20(int8_t *index, uint8_t bins)
{
    index[0] = (2*index[0]+index[1])/3;
    index[1] = (index[1]+2*index[2])/3;
    index[2] = (2*index[3]+index[4])/3;
    index[3] = (index[4]+2*index[5])/3;
    index[4] = (index[6]+index[7])/2;
    index[5] = (index[8]+index[9])/2;
    index[6] = index[10];
    index[7] = index[11];
    index[8] = (index[12]+index[13])/2;
    index[9] = (index[14]+index[15])/2;
    index[10] = index[16];

    if (bins == 34)
    {
        index[11] = index[17];
        index[12] = index[18];
        index[13] = index[19];
        index[14] = (index[20]+index[21])/2;
        index[15] = (index[22]+index[23])/2;
        index[16] = (index[24]+index[25])/2;
        index[17] = (index[26]+index[27])/2;
        index[18] = (index[28]+index[29]+index[30]+index[31])/4;
        index[19] = (index[32]+index[33])/2;
    }
}
#endif

static void map20indexto34(int8_t *index, uint8_t bins)
{
    index[0] = index[0];
    index[1] = (index[0] + index[1])/2;
    index[2] = index[1];
    index[3] = index[2];
    index[4] = (index[2] + index[3])/2;
    index[5] = index[3];
    index[6] = index[4];
    index[7] = index[4];
    index[8] = index[5];
    index[9] = index[5];
    index[10] = index[6];
    index[11] = index[7];
    index[12] = index[8];
    index[13] = index[8];
    index[14] = index[9];
    index[15] = index[9];
    index[16] = index[10];

    if (bins == 34)
    {
        index[17] = index[11];
        index[18] = index[12];
        index[19] = index[13];
        index[20] = index[14];
        index[21] = index[14];
        index[22] = index[15];
        index[23] = index[15];
        index[24] = index[16];
        index[25] = index[16];
        index[26] = index[17];
        index[27] = index[17];
        index[28] = index[18];
        index[29] = index[18];
        index[30] = index[18];
        index[31] = index[18];
        index[32] = index[19];
        index[33] = index[19];
    }
}

/* parse the bitstream data decoded in ps_data() */
static void ps_data_decode(ps_info *ps)
{
    uint8_t env, bin;

    /* ps data not available, use data from previous frame */
    if (ps->ps_data_available == 0)
    {
        ps->num_env = 0;
    }

    for (env = 0; env < ps->num_env; env++)
    {
        int8_t *iid_index_prev;
        int8_t *icc_index_prev;
        int8_t *ipd_index_prev;
        int8_t *opd_index_prev;

        int8_t num_iid_steps = (ps->iid_mode < 3) ? 7 : 15 /*fine quant*/;

        if (env == 0)
        {
            /* take last envelope from previous frame */
            iid_index_prev = ps->iid_index_prev;
            icc_index_prev = ps->icc_index_prev;
            ipd_index_prev = ps->ipd_index_prev;
            opd_index_prev = ps->opd_index_prev;
        } else {
            /* take index values from previous envelope */
            iid_index_prev = ps->iid_index[env - 1];
            icc_index_prev = ps->icc_index[env - 1];
            ipd_index_prev = ps->ipd_index[env - 1];
            opd_index_prev = ps->opd_index[env - 1];
        }

        /* delta decode iid parameters */
        delta_decode(ps->enable_iid, ps->iid_index[env], iid_index_prev,
            ps->iid_dt[env], ps->nr_iid_par,
            (ps->iid_mode == 0 || ps->iid_mode == 3) ? 2 : 1,
            -num_iid_steps, num_iid_steps);

        /* delta decode icc parameters */
        delta_decode(ps->enable_icc, ps->icc_index[env], icc_index_prev,
            ps->icc_dt[env], ps->nr_icc_par,
            (ps->icc_mode == 0 || ps->icc_mode == 3) ? 2 : 1,
            0, 7);

        /* delta decode ipd parameters */
        delta_decode(ps->enable_ipdopd, ps->ipd_index[env], ipd_index_prev,
            ps->ipd_dt[env], ps->nr_ipdopd_par, 1, -8, 8);

        /* delta decode opd parameters */
        delta_decode(ps->enable_ipdopd, ps->opd_index[env], opd_index_prev,
            ps->opd_dt[env], ps->nr_ipdopd_par, 1, -8, 8);
    }

    /* handle error case */
    if (ps->num_env == 0)
    {
        /* force to 1 */
        ps->num_env = 1;

        if (ps->enable_iid)
        {
            for (bin = 0; bin < 34; bin++)
                ps->iid_index[0][bin] = ps->iid_index_prev[bin];
        } else {
            for (bin = 0; bin < 34; bin++)
                ps->iid_index[0][bin] = 0;
        }

        if (ps->enable_icc)
        {
            for (bin = 0; bin < 34; bin++)
                ps->icc_index[0][bin] = ps->icc_index_prev[bin];
        } else {
            for (bin = 0; bin < 34; bin++)
                ps->icc_index[0][bin] = 0;
        }

        if (ps->enable_ipdopd)
        {
            for (bin = 0; bin < 17; bin++)
            {
                ps->ipd_index[0][bin] = ps->ipd_index_prev[bin];
                ps->opd_index[0][bin] = ps->opd_index_prev[bin];
            }
        } else {
            for (bin = 0; bin < 17; bin++)
            {
                ps->ipd_index[0][bin] = 0;
                ps->opd_index[0][bin] = 0;
            }
        }
    }

    /* update previous indices */
    for (bin = 0; bin < 34; bin++)
        ps->iid_index_prev[bin] = ps->iid_index[ps->num_env-1][bin];
    for (bin = 0; bin < 34; bin++)
        ps->icc_index_prev[bin] = ps->icc_index[ps->num_env-1][bin];
    for (bin = 0; bin < 17; bin++)
    {
        ps->ipd_index_prev[bin] = ps->ipd_index[ps->num_env-1][bin];
        ps->opd_index_prev[bin] = ps->opd_index[ps->num_env-1][bin];
    }

    ps->ps_data_available = 0;

    if (ps->frame_class == 0)
    {
        ps->border_position[0] = 0;
        for (env = 1; env < ps->num_env; env++)
        {
            ps->border_position[env] = (env * 32 /* 30 for 960? */) / ps->num_env;
        }
        ps->border_position[ps->num_env] = 32 /* 30 for 960? */;
    } else {
        ps->border_position[0] = 0;

        if (ps->border_position[ps->num_env] < 32 /* 30 for 960? */)
        {
            ps->num_env++;
            ps->border_position[ps->num_env] = 32 /* 30 for 960? */;
            for (bin = 0; bin < 34; bin++)
            {
                ps->iid_index[ps->num_env][bin] = ps->iid_index[ps->num_env-1][bin];
                ps->icc_index[ps->num_env][bin] = ps->icc_index[ps->num_env-1][bin];
            }
            for (bin = 0; bin < 17; bin++)
            {
                ps->ipd_index[ps->num_env][bin] = ps->ipd_index[ps->num_env-1][bin];
                ps->opd_index[ps->num_env][bin] = ps->opd_index[ps->num_env-1][bin];
            }
        }

        for (env = 1; env < ps->num_env; env++)
        {
            int8_t thr = 32 /* 30 for 960? */ - (ps->num_env - env);

            if (ps->border_position[env] > thr)
            {
                ps->border_position[env] = thr;
            } else {
                thr = ps->border_position[env-1]+1;
                if (ps->border_position[env] < thr)
                {
                    ps->border_position[env] = thr;
                }
            }
        }
    }

    /* make sure that the indices of all parameters can be mapped
     * to the same hybrid synthesis filterbank
     */
#ifdef PS_LOW_POWER
    for (env = 0; env < ps->num_env; env++)
    {
        if (ps->iid_mode == 2 || ps->iid_mode == 5)
            map34indexto20(ps->iid_index[env], 34);
        if (ps->icc_mode == 2 || ps->icc_mode == 5)
            map34indexto20(ps->icc_index[env], 34);

        /* disable ipd/opd */
        for (bin = 0; bin < 17; bin++)
        {
            ps->aaIpdIndex[env][bin] = 0;
            ps->aaOpdIndex[env][bin] = 0;
        }
    }
#else
    if (ps->use34hybrid_bands)
    {
        for (env = 0; env < ps->num_env; env++)
        {
            if (ps->iid_mode != 2 && ps->iid_mode != 5)
                map20indexto34(ps->iid_index[env], 34);
            if (ps->icc_mode != 2 && ps->icc_mode != 5)
                map20indexto34(ps->icc_index[env], 34);
            if (ps->ipd_mode != 2 && ps->ipd_mode != 5)
            {
                map20indexto34(ps->ipd_index[env], 17);
                map20indexto34(ps->opd_index[env], 17);
            }
        }
    }
#endif

#if 0
    for (env = 0; env < ps->num_env; env++)
    {
        printf("iid[env:%d]:", env);
        for (bin = 0; bin < 34; bin++)
        {
            printf(" %d", ps->iid_index[env][bin]);
        }
        printf("\n");
    }
    for (env = 0; env < ps->num_env; env++)
    {
        printf("icc[env:%d]:", env);
        for (bin = 0; bin < 34; bin++)
        {
            printf(" %d", ps->icc_index[env][bin]);
        }
        printf("\n");
    }
    for (env = 0; env < ps->num_env; env++)
    {
        printf("ipd[env:%d]:", env);
        for (bin = 0; bin < 17; bin++)
        {
            printf(" %d", ps->ipd_index[env][bin]);
        }
        printf("\n");
    }
    for (env = 0; env < ps->num_env; env++)
    {
        printf("opd[env:%d]:", env);
        for (bin = 0; bin < 17; bin++)
        {
            printf(" %d", ps->opd_index[env][bin]);
        }
        printf("\n");
    }
    printf("\n");
#endif
}

/* decorrelate the mono signal using an allpass filter */
static void ps_decorrelate(ps_info *ps, qmf_t X_left[38][64], qmf_t X_right[38][64],
                           qmf_t X_hybrid_left[32][32], qmf_t X_hybrid_right[32][32])
{
    uint8_t gr, m, n, bk;
    uint8_t temp_delay;
    uint8_t sb, maxsb;
    const complex_t *Phi_Fract_SubQmf;
    uint8_t temp_delay_ser[NO_ALLPASS_LINKS];
    real_t P_SmoothPeakDecayDiffNrg, nrg;
    real_t P[32][34];
    real_t G_TransientRatio[32][34];
    complex_t inputLeft;


    /* chose hybrid filterbank: 20 or 34 band case */
    if (ps->use34hybrid_bands)
    {
        Phi_Fract_SubQmf = Phi_Fract_SubQmf34;
    } else{
        Phi_Fract_SubQmf = Phi_Fract_SubQmf20;
    }

    for (n = 0; n < 32; n++)
    {
        for (bk = 0; bk < 34; bk++)
        {
            P[n][bk] = 0;
        }
    }

    for (gr = 0; gr < ps->num_groups; gr++)
    {
        bk = (~NEGATE_IPD_MASK) & ps->map_bins2group[gr];
        maxsb = (gr < ps->num_hybrid_groups) ? ps->group_border[gr]+1 : ps->group_border[gr+1];

        for (sb = ps->group_border[gr]; sb < maxsb; sb++)
        {
            for (n = ps->border_position[0] ; n < ps->border_position[ps->num_env] ; n++)
            {
                if (gr < ps->num_hybrid_groups)
                {
                    RE(inputLeft) = QMF_RE(X_hybrid_left[n][sb]);
                    IM(inputLeft) = QMF_IM(X_hybrid_left[n][sb]);
                } else {
                    RE(inputLeft) = QMF_RE(X_left[n][sb]);
                    IM(inputLeft) = QMF_IM(X_left[n][sb]);
                }
                P[n][bk] += MUL_R(RE(inputLeft),RE(inputLeft)) + MUL_R(IM(inputLeft),IM(inputLeft));
            }
        }
    }

    for (bk = 0; bk < ps->nr_par_bands; bk++)
    {
        for (n = ps->border_position[0]; n < ps->border_position[ps->num_env]; n++)
        {
            const real_t gamma = COEF_CONST(1.5);

            ps->P_PeakDecayNrg[bk] = MUL_F(ps->P_PeakDecayNrg[bk], ps->alpha_decay);
            if (ps->P_PeakDecayNrg[bk] < P[n][bk])
                ps->P_PeakDecayNrg[bk] = P[n][bk];

            P_SmoothPeakDecayDiffNrg = ps->P_SmoothPeakDecayDiffNrg_prev[bk];
            P_SmoothPeakDecayDiffNrg += MUL_F((ps->P_PeakDecayNrg[bk] - P[n][bk] - ps->P_SmoothPeakDecayDiffNrg_prev[bk]), ps->alpha_smooth);
            ps->P_SmoothPeakDecayDiffNrg_prev[bk] = P_SmoothPeakDecayDiffNrg;

            nrg = ps->P_prev[bk];
            nrg += MUL_F((P[n][bk] - ps->P_prev[bk]), ps->alpha_smooth);
            ps->P_prev[bk] = nrg;
            if (MUL_C(P_SmoothPeakDecayDiffNrg, gamma) <= nrg)
            {
                G_TransientRatio[n][bk] = REAL_CONST(1.0);
            } else {
                G_TransientRatio[n][bk] = SBR_DIV(nrg, (MUL_C(P_SmoothPeakDecayDiffNrg, gamma)));
            }
        }
    }

    for (gr = 0; gr < ps->num_groups; gr++)
    {
        if (gr < ps->num_hybrid_groups)
            maxsb = ps->group_border[gr] + 1;
        else
            maxsb = ps->group_border[gr + 1];

        /* QMF channel */
        for (sb = ps->group_border[gr]; sb < maxsb; sb++)
        {
            real_t g_DecaySlope;

            /* g_DecaySlope: [0..1] */
            if (gr < ps->num_hybrid_groups || sb <= ps->decay_cutoff)
            {
                g_DecaySlope = FRAC_CONST(1.0);
            } else {
                int8_t decay = ps->decay_cutoff - sb;
                if (decay <= -20 /* -1/DECAY_SLOPE */)
                {
                    g_DecaySlope = 0;
                } else {
                    /* decay(int)*decay_slope(frac) = g_DecaySlope(frac) */
                    g_DecaySlope = FRAC_CONST(1.0) + DECAY_SLOPE * decay;
                }
            }


            /* set delay indices */
            temp_delay = ps->saved_delay;
            for (n = 0; n < NO_ALLPASS_LINKS; n++)
                temp_delay_ser[n] = ps->delay_buf_index_ser[n];

            for (n = ps->border_position[0]; n < ps->border_position[ps->num_env]; n++)
            {
                complex_t tmp, tmp0, R0;

                if (gr < ps->num_hybrid_groups)
                {
                    RE(inputLeft) = QMF_RE(X_hybrid_left[n][sb]);
                    IM(inputLeft) = QMF_IM(X_hybrid_left[n][sb]);
                } else {
                    RE(inputLeft) = QMF_RE(X_left[n][sb]);
                    IM(inputLeft) = QMF_IM(X_left[n][sb]);
                }

                if (sb > ps->nr_allpass_bands && gr >= ps->num_hybrid_groups)
                {
                    /* delay */

                    /* never SubQMF here */
                    RE(tmp) = RE(ps->delay_Qmf[ps->delay_buf_index_delay[sb]][sb]);
                    IM(tmp) = IM(ps->delay_Qmf[ps->delay_buf_index_delay[sb]][sb]);
                    RE(R0) = RE(tmp);
                    IM(R0) = IM(tmp);
                    RE(ps->delay_Qmf[ps->delay_buf_index_delay[sb]][sb]) = RE(inputLeft);
                    IM(ps->delay_Qmf[ps->delay_buf_index_delay[sb]][sb]) = IM(inputLeft);
                } else {
                    /* allpass filter */
                    uint8_t m;
                    complex_t Phi_Fract;

                    /* fetch parameters */
                    if (gr < ps->num_hybrid_groups)
                    {
                        RE(tmp0) = RE(ps->delay_SubQmf[temp_delay][sb]);
                        IM(tmp0) = IM(ps->delay_SubQmf[temp_delay][sb]);

                        RE(ps->delay_SubQmf[temp_delay][sb]) = RE(inputLeft);
                        IM(ps->delay_SubQmf[temp_delay][sb]) = IM(inputLeft);

                        RE(Phi_Fract) = RE(Phi_Fract_SubQmf[sb]);
                        IM(Phi_Fract) = IM(Phi_Fract_SubQmf[sb]);
                    } else {
                        RE(tmp0) = RE(ps->delay_Qmf[temp_delay][sb]);
                        IM(tmp0) = IM(ps->delay_Qmf[temp_delay][sb]);

                        RE(ps->delay_Qmf[temp_delay][sb]) = RE(inputLeft);
                        IM(ps->delay_Qmf[temp_delay][sb]) = IM(inputLeft);

                        RE(Phi_Fract) = RE(Phi_Fract_Qmf[sb]);
                        IM(Phi_Fract) = IM(Phi_Fract_Qmf[sb]);
                    }

                    /* delay by fraction */
                    ComplexMult(&RE(tmp), &IM(tmp), RE(tmp0), IM(tmp0), RE(Phi_Fract), IM(Phi_Fract));

                    RE(R0) = RE(tmp);
                    IM(R0) = IM(tmp);
                    for (m = 0; m < NO_ALLPASS_LINKS ; m++)
                    {
                        complex_t Q_Fract_allpass, tmp2;

                        /* fetch parameters */
                        if (gr < ps->num_hybrid_groups)
                        {
                            RE(tmp0) = RE(ps->delay_SubQmf_ser[m][temp_delay_ser[m]][sb]);
                            IM(tmp0) = IM(ps->delay_SubQmf_ser[m][temp_delay_ser[m]][sb]);

                            if (ps->use34hybrid_bands)
                            {
                                RE(Q_Fract_allpass) = RE(Q_Fract_allpass_SubQmf34[sb][m]);
                                IM(Q_Fract_allpass) = IM(Q_Fract_allpass_SubQmf34[sb][m]);
                            } else {
                                RE(Q_Fract_allpass) = RE(Q_Fract_allpass_SubQmf20[sb][m]);
                                IM(Q_Fract_allpass) = IM(Q_Fract_allpass_SubQmf20[sb][m]);
                            }
                        } else {
                            RE(tmp0) = RE(ps->delay_Qmf_ser[m][temp_delay_ser[m]][sb]);
                            IM(tmp0) = IM(ps->delay_Qmf_ser[m][temp_delay_ser[m]][sb]);

                            RE(Q_Fract_allpass) = RE(Q_Fract_allpass_Qmf[sb][m]);
                            IM(Q_Fract_allpass) = IM(Q_Fract_allpass_Qmf[sb][m]);
                        }

                        ComplexMult(&RE(tmp), &IM(tmp), RE(tmp0), IM(tmp0), RE(Q_Fract_allpass), IM(Q_Fract_allpass));

                        RE(tmp) += -MUL_F(g_DecaySlope, MUL_F(filter_a[m], RE(R0)));
                        IM(tmp) += -MUL_F(g_DecaySlope, MUL_F(filter_a[m], IM(R0)));

                        RE(tmp2) = RE(R0) + MUL_F(g_DecaySlope, MUL_F(filter_a[m], RE(tmp)));
                        IM(tmp2) = IM(R0) + MUL_F(g_DecaySlope, MUL_F(filter_a[m], IM(tmp)));
                        if (gr < ps->num_hybrid_groups)
                        {
                            RE(ps->delay_SubQmf_ser[m][temp_delay_ser[m]][sb]) = RE(tmp2);
                            IM(ps->delay_SubQmf_ser[m][temp_delay_ser[m]][sb]) = IM(tmp2);
                        } else {
                            RE(ps->delay_Qmf_ser[m][temp_delay_ser[m]][sb]) = RE(tmp2);
                            IM(ps->delay_Qmf_ser[m][temp_delay_ser[m]][sb]) = IM(tmp2);
                        }

                        RE(R0) = RE(tmp);
                        IM(R0) = IM(tmp);
                    }
                }

                bk = (~NEGATE_IPD_MASK) & ps->map_bins2group[gr];

                /* duck if a past transient is found */
                if (gr < ps->num_hybrid_groups)
                {
                    QMF_RE(X_hybrid_right[n][sb]) = MUL_R(G_TransientRatio[n][bk], RE(R0));
                    QMF_IM(X_hybrid_right[n][sb]) = MUL_R(G_TransientRatio[n][bk], IM(R0));
                } else {
                    QMF_RE(X_right[n][sb]) = MUL_R(G_TransientRatio[n][bk], RE(R0));
                    QMF_IM(X_right[n][sb]) = MUL_R(G_TransientRatio[n][bk], IM(R0));
                }

                /* Update delay buffer index */
                if (++temp_delay >= 2)
                    temp_delay = 0;

                if (sb > ps->nr_allpass_bands && gr >= ps->num_hybrid_groups)
                {
                    if (++ps->delay_buf_index_delay[sb] >= ps->delay_D[sb])
                    {
                        ps->delay_buf_index_delay[sb] = 0;
                    }
                }

                for (m = 0; m < NO_ALLPASS_LINKS; m++)
                {
                    if (++temp_delay_ser[m] >= ps->num_sample_delay_ser[m])
                        temp_delay_ser[m] = 0;
                }
            }
        }
    }

    ps->saved_delay = temp_delay;
    for (m = 0; m < NO_ALLPASS_LINKS; m++)
        ps->delay_buf_index_ser[m] = temp_delay_ser[m];
}

static void ps_mix_phase(ps_info *ps, qmf_t X_left[38][64], qmf_t X_right[38][64],
                         qmf_t X_hybrid_left[32][32], qmf_t X_hybrid_right[32][32])
{
    uint8_t i;
    uint8_t gr;
    uint8_t bk = 0;
    uint8_t sb, maxsb;
    uint8_t env;
    uint8_t nr_ipdopd_par;
    real_t scaleL, scaleR;
    complex_t h11, h12, h21, h22;
    complex_t H11, H12, H21, H22;
    complex_t deltaH11, deltaH12, deltaH21, deltaH22;
    complex_t tempLeft;
    complex_t tempRight;
    complex_t phaseLeft;
    complex_t phaseRight;
    real_t L;
    const real_t *sf_iid;
    uint8_t no_iid_steps;

    if (ps->iid_mode >= 3)
    {
        no_iid_steps = 15;
        sf_iid = sf_iid_fine;
    } else {
        no_iid_steps = 7;
        sf_iid = sf_iid_normal;
    }

    if (ps->ipd_mode == 0 || ps->ipd_mode == 3)
    {
        nr_ipdopd_par = 11; /* resolution */
    } else {
        nr_ipdopd_par = ps->nr_ipdopd_par;
    }

    for (gr = 0; gr < ps->num_groups; gr++)
    {
        bk = (~NEGATE_IPD_MASK) & ps->map_bins2group[gr];

        /* use one channel per group in the subqmf domain */
        maxsb = (gr < ps->num_hybrid_groups) ? ps->group_border[gr] + 1 : ps->group_border[gr + 1];

        for (env = 0; env < ps->num_env; env++)
        {
            if (ps->icc_mode < 3)
            {
                /* type 'A' mixing */
                real_t cosa, sina;
                real_t cosb, sinb;
                real_t ab1, ab2;
                real_t ab3, ab4;

                /*
                scaleR = sqrt(2.0 / (1.0 + pow(10.0, quant_iid[no_iid_steps + iid_index] / 10.0)));
                scaleL = sqrt(2.0 / (1.0 + pow(10.0, quant_iid[no_iid_steps - iid_index] / 10.0)));
                alpha = 0.5 * acos(quant_rho[icc_index]);
                beta = alpha * ( scaleR - scaleL ) / sqrt(2.0);
                */

                scaleR = sf_iid[no_iid_steps + ps->iid_index[env][bk]];
                scaleL = sf_iid[no_iid_steps - ps->iid_index[env][bk]];

                cosa = cos_alphas[ps->icc_index[env][bk]];
                sina = sin_alphas[ps->icc_index[env][bk]];

                if (ps->iid_mode >= 3)
                {
                    if (ps->iid_index[env][bk] < 0)
                    {
                        cosb =  cos_betas_fine[-ps->iid_index[env][bk]][ps->icc_index[env][bk]];
                        sinb = -sin_betas_fine[-ps->iid_index[env][bk]][ps->icc_index[env][bk]];
                    } else {
                        cosb = cos_betas_fine[ps->iid_index[env][bk]][ps->icc_index[env][bk]];
                        sinb = sin_betas_fine[ps->iid_index[env][bk]][ps->icc_index[env][bk]];
                    }
                } else {
                    if (ps->iid_index[env][bk] < 0)
                    {
                        cosb =  cos_betas_normal[-ps->iid_index[env][bk]][ps->icc_index[env][bk]];
                        sinb = -sin_betas_normal[-ps->iid_index[env][bk]][ps->icc_index[env][bk]];
                    } else {
                        cosb = cos_betas_normal[ps->iid_index[env][bk]][ps->icc_index[env][bk]];
                        sinb = sin_betas_normal[ps->iid_index[env][bk]][ps->icc_index[env][bk]];
                    }
                }

                ab1 = MUL_C(cosb, cosa);
                ab2 = MUL_C(sinb, sina);
                ab3 = MUL_C(sinb, cosa);
                ab4 = MUL_C(cosb, sina);

                /* hxx: COEF */
                RE(h11) = MUL_C(scaleL, (ab1 - ab2));
                RE(h12) = MUL_C(scaleR, (ab1 + ab2));
                RE(h21) = MUL_C(scaleL, (ab3 + ab4));
                RE(h22) = MUL_C(scaleR, (ab3 - ab4));

                //printf("%d %f %f %f %f\n", ps->iid_index[env][bin], scaleR, scaleL, alpha, beta);

            } else {
                /* type 'B' mixing */
                real_t sina, cosa;
                real_t cosg, sing;

                /*
                real_t c, rho, mu, alpha, gamma;
                uint8_t i;

                i = ps->iid_index[env][bk];
                c = (real_t)pow(10.0, ((i)?(((i>0)?1:-1)*quant_iid[((i>0)?i:-i)-1]):0.)/20.0);
                rho = quant_rho[ps->icc_index[env][bk]];

                if (rho == 0.0f && c == 1.)
                {
                    alpha = (real_t)M_PI/4.0f;
                    rho = 0.05f;
                } else {
                    if (rho <= 0.05f)
                    {
                        rho = 0.05f;
                    }
                    alpha = 0.5f*(real_t)atan( (2.0f*c*rho) / (c*c-1.0f) );

                    if (alpha < 0.)
                    {
                        alpha += (real_t)M_PI/2.0f;
                    }
                    if (rho < 0.)
                    {
                        alpha += (real_t)M_PI;
                    }
                }
                mu = c+1.0f/c;
                mu = 1+(4.0f*rho*rho-4.0f)/(mu*mu);
                gamma = (real_t)atan(sqrt((1.0f-sqrt(mu))/(1.0f+sqrt(mu))));
                */

                if (ps->iid_mode >= 3)
                {
                    uint8_t abs_iid = abs(ps->iid_index[env][bk]);

                    cosa = sincos_alphas_B_fine[no_iid_steps + ps->iid_index[env][bk]][ps->icc_index[env][bk]];
                    sina = sincos_alphas_B_fine[30 - (no_iid_steps + ps->iid_index[env][bk])][ps->icc_index[env][bk]];
                    cosg = cos_gammas_fine[abs_iid][ps->icc_index[env][bk]];
                    sing = sin_gammas_fine[abs_iid][ps->icc_index[env][bk]];
                } else {
                    uint8_t abs_iid = abs(ps->iid_index[env][bk]);

                    cosa = sincos_alphas_B_normal[no_iid_steps + ps->iid_index[env][bk]][ps->icc_index[env][bk]];
                    sina = sincos_alphas_B_normal[14 - (no_iid_steps + ps->iid_index[env][bk])][ps->icc_index[env][bk]];
                    cosg = cos_gammas_normal[abs_iid][ps->icc_index[env][bk]];
                    sing = sin_gammas_normal[abs_iid][ps->icc_index[env][bk]];
                }

                RE(h11) = MUL_C(COEF_SQRT2, MUL_C(cosa, cosg));
                RE(h12) = MUL_C(COEF_SQRT2, MUL_C(sina, cosg));
                RE(h21) = MUL_C(COEF_SQRT2, MUL_C(-cosa, sing));
                RE(h22) = MUL_C(COEF_SQRT2, MUL_C(sina, sing));
            }

            if ((ps->enable_ipdopd) && (bk < nr_ipdopd_par))
            {
                real_t ipd, opd;
                real_t xxyy, ppqq;
                real_t yq, xp, xq, py, tmp;

                ipd = (float)( M_PI/4.0f ) * ps->ipd_index[env][bk];
                opd = (float)( M_PI/4.0f ) * ps->opd_index[env][bk];

                i = ps->phase_hist;

                /* previous value */
                RE(tempLeft)  = MUL_F(RE(ps->ipd_prev[bk][i]), FRAC_CONST(0.25));
                IM(tempLeft)  = MUL_F(IM(ps->ipd_prev[bk][i]), FRAC_CONST(0.25));
                RE(tempRight) = MUL_F(RE(ps->opd_prev[bk][i]), FRAC_CONST(0.25));
                IM(tempRight) = MUL_F(IM(ps->opd_prev[bk][i]), FRAC_CONST(0.25));

                /* save current value */
                RE(ps->ipd_prev[bk][i]) = (float)cos(ipd);
                IM(ps->ipd_prev[bk][i]) = (float)sin(ipd);
                RE(ps->opd_prev[bk][i]) = (float)cos(opd);
                IM(ps->opd_prev[bk][i]) = (float)sin(opd);

                /* add current value */
                RE(tempLeft)  += RE(ps->ipd_prev[bk][i]);
                IM(tempLeft)  += IM(ps->ipd_prev[bk][i]);
                RE(tempRight) += RE(ps->opd_prev[bk][i]);
                IM(tempRight) += IM(ps->opd_prev[bk][i]);

                if (i == 0)
                { 
                    i = 2;
                }
                i--;

                /* get value before previous */
                RE(tempLeft)  += MUL_F(RE(ps->ipd_prev[bk][i]), FRAC_CONST(0.5));
                IM(tempLeft)  += MUL_F(IM(ps->ipd_prev[bk][i]), FRAC_CONST(0.5));
                RE(tempRight) += MUL_F(RE(ps->opd_prev[bk][i]), FRAC_CONST(0.5));
                IM(tempRight) += MUL_F(IM(ps->opd_prev[bk][i]), FRAC_CONST(0.5));

#if 0
                ipd = (float)atan2(IM(tempLeft), RE(tempLeft));
                opd = (float)atan2(IM(tempRight), RE(tempRight));

                /* phase rotation */
                RE(phaseLeft) = (float)cos(opd);
                IM(phaseLeft) = (float)sin(opd);
                opd -= ipd;
                RE(phaseRight) = (float)cos(opd);
                IM(phaseRight) = (float)sin(opd);
#else
                // cos(atan2(x,y)) = 1/sqrt(1 + (x*x)/(y*y))
                // sin(atan2(x,y)) = x/(y*sqrt(1 + (x*x)/(y*y)))
                // cos(atan2(x,y)-atan2(p,q)) = (y*q+x*p)/(y*q * sqrt(1 + (x*x)/(y*y)) * sqrt(1 + (p*p)/(q*q)));
                // sin(atan2(x,y)-atan2(p,q)) = (x*q-p*y)/(y*q * sqrt(1 + (x*x)/(y*y)) * sqrt(1 + (p*p)/(q*q)));
#define DIV_C(A,B) ((A)/(B))
                xxyy = DIV_C(MUL_C(IM(tempLeft),IM(tempLeft)), MUL_C(RE(tempLeft),RE(tempLeft)));
                ppqq = DIV_C(MUL_C(IM(tempRight),IM(tempRight)), MUL_C(RE(tempRight),RE(tempRight)));

                xxyy += COEF_CONST(1);
                ppqq += COEF_CONST(1);

                xxyy = sqrt(xxyy);
                ppqq = sqrt(ppqq);

                yq = MUL_C(RE(tempLeft), RE(tempRight));
                xp = MUL_C(IM(tempLeft), IM(tempRight));
                xq = MUL_C(IM(tempLeft), RE(tempRight));
                py = MUL_C(IM(tempRight), RE(tempLeft));

                RE(phaseLeft) = 1.f/xxyy;
                IM(phaseLeft) = IM(tempLeft)/(RE(tempLeft)*xxyy);

                tmp = 1.f / (yq * xxyy * ppqq);
                RE(phaseRight) = (yq+xp) * tmp;
                IM(phaseRight) = (xq-py) * tmp;
#endif

                IM(h11) = RE(h11) * IM(phaseLeft);
                IM(h12) = RE(h12) * IM(phaseRight);
                IM(h21) = RE(h21) * IM(phaseLeft);
                IM(h22) = RE(h22) * IM(phaseRight);

                RE(h11) *= RE(phaseLeft);
                RE(h12) *= RE(phaseRight);
                RE(h21) *= RE(phaseLeft);
                RE(h22) *= RE(phaseRight);
            }

            /* length of the envelope (in time samples) */
            L = (real_t)(ps->border_position[env + 1] - ps->border_position[env]);

            RE(deltaH11) = (RE(h11) - RE(ps->h11_prev[gr])) / L;
            RE(deltaH12) = (RE(h12) - RE(ps->h12_prev[gr])) / L;
            RE(deltaH21) = (RE(h21) - RE(ps->h21_prev[gr])) / L;
            RE(deltaH22) = (RE(h22) - RE(ps->h22_prev[gr])) / L;

            RE(H11) = RE(ps->h11_prev[gr]);
            RE(H12) = RE(ps->h12_prev[gr]);
            RE(H21) = RE(ps->h21_prev[gr]);
            RE(H22) = RE(ps->h22_prev[gr]);

            RE(ps->h11_prev[gr]) = RE(h11);
            RE(ps->h12_prev[gr]) = RE(h12);
            RE(ps->h21_prev[gr]) = RE(h21);
            RE(ps->h22_prev[gr]) = RE(h22);

            if ((ps->enable_ipdopd) && (bk < nr_ipdopd_par))
            {
                IM(deltaH11) = (IM(h11) - IM(ps->h11_prev[gr])) / L;
                IM(deltaH12) = (IM(h12) - IM(ps->h12_prev[gr])) / L;
                IM(deltaH21) = (IM(h21) - IM(ps->h21_prev[gr])) / L;
                IM(deltaH22) = (IM(h22) - IM(ps->h22_prev[gr])) / L;

                IM(H11) = IM(ps->h11_prev[gr]);
                IM(H12) = IM(ps->h12_prev[gr]);
                IM(H21) = IM(ps->h21_prev[gr]);
                IM(H22) = IM(ps->h22_prev[gr]);

                if ((NEGATE_IPD_MASK & ps->map_bins2group[gr]) != 0)
                {
                    IM(deltaH11) = -IM(deltaH11);
                    IM(deltaH12) = -IM(deltaH12);
                    IM(deltaH21) = -IM(deltaH21);
                    IM(deltaH22) = -IM(deltaH22);

                    IM(H11) = -IM(H11);
                    IM(H12) = -IM(H12);
                    IM(H21) = -IM(H21);
                    IM(H22) = -IM(H22);
                }

                IM(ps->h11_prev[gr]) = IM(h11);
                IM(ps->h12_prev[gr]) = IM(h12);
                IM(ps->h21_prev[gr]) = IM(h21);
                IM(ps->h22_prev[gr]) = IM(h22);
            }

            for (i = ps->border_position[env]; i < ps->border_position[env + 1]; i++)
            {
                RE(H11) += RE(deltaH11);
                RE(H12) += RE(deltaH12);
                RE(H21) += RE(deltaH21);
                RE(H22) += RE(deltaH22);
                if ((ps->enable_ipdopd) && (bk < nr_ipdopd_par))
                {
                    IM(H11) += IM(deltaH11);
                    IM(H12) += IM(deltaH12);
                    IM(H21) += IM(deltaH21);
                    IM(H22) += IM(deltaH22);
                }

                /* channel is an alias to the subband */
                for (sb = ps->group_border[gr]; sb < maxsb; sb++)
                {
                    complex_t inLeft, inRight;

                    if (gr < ps->num_hybrid_groups)
                    {
                        RE(inLeft) =  RE(X_hybrid_left[i][sb]);
                        IM(inLeft) =  IM(X_hybrid_left[i][sb]);
                        RE(inRight) = RE(X_hybrid_right[i][sb]);
                        IM(inRight) = IM(X_hybrid_right[i][sb]);
                    } else {
                        RE(inLeft) =  RE(X_left[i][sb]);
                        IM(inLeft) =  IM(X_left[i][sb]);
                        RE(inRight) = RE(X_right[i][sb]);
                        IM(inRight) = IM(X_right[i][sb]);
                    }

                    RE(tempLeft) =  RE(H11) * RE(inLeft) + RE(H21) * RE(inRight);
                    IM(tempLeft) =  RE(H11) * IM(inLeft) + RE(H21) * IM(inRight);
                    RE(tempRight) = RE(H12) * RE(inLeft) + RE(H22) * RE(inRight);
                    IM(tempRight) = RE(H12) * IM(inLeft) + RE(H22) * IM(inRight);

                    if ((ps->enable_ipdopd) && (bk < nr_ipdopd_par))
                    {
                        RE(tempLeft)  -= IM(H11) * IM(inLeft) + IM(H21) * IM(inRight); 
                        IM(tempLeft)  += IM(H11) * RE(inLeft) + IM(H21) * RE(inRight);
                        RE(tempRight) -= IM(H12) * IM(inLeft) + IM(H22) * IM(inRight);
                        IM(tempRight) += IM(H12) * RE(inLeft) + IM(H22) * RE(inRight);
                    }

                    if (gr < ps->num_hybrid_groups)
                    {
                        RE(X_hybrid_left[i][sb]) =  RE(tempLeft);
                        IM(X_hybrid_left[i][sb]) =  IM(tempLeft);
                        RE(X_hybrid_right[i][sb]) = RE(tempRight);
                        IM(X_hybrid_right[i][sb]) = IM(tempRight);
                    } else {
                        RE(X_left[i][sb]) =  RE(tempLeft);
                        IM(X_left[i][sb]) =  IM(tempLeft);
                        RE(X_right[i][sb]) = RE(tempRight);
                        IM(X_right[i][sb]) = IM(tempRight);
                    }
                }
            }

            /* shift phase smoother's circular buffer index */
            ps->phase_hist++;
            if (ps->phase_hist == 2)
            {
                ps->phase_hist = 0;
            }
        }
    }
}

ps_info *ps_init(uint8_t sr_index)
{
    uint8_t i;
    uint8_t short_delay_band;

    ps_info *ps = (ps_info*)faad_malloc(sizeof(ps_info));
    memset(ps, 0, sizeof(ps_info));

    ps->hyb = hybrid_init();

    ps->ps_data_available = 0;

    /* delay stuff*/
    ps->saved_delay = 0;

    for (i = 0; i < 64; i++)
    {
        ps->delay_buf_index_delay[i] = 0;
    }

    for (i = 0; i < NO_ALLPASS_LINKS; i++)
    {
        ps->delay_buf_index_ser[i] = 0;
        if (sr_index <= 5) /* >= 32 kHz*/
        {
            ps->num_sample_delay_ser[i] = delay_length_d[1][i];
        } else {
            ps->num_sample_delay_ser[i] = delay_length_d[0][i];
        }
    }

    if (sr_index <= 5) /* >= 32 kHz*/
    {
        short_delay_band = 35;
        ps->nr_allpass_bands = 22;
        ps->alpha_decay = FRAC_CONST(0.76592833836465);
        ps->alpha_smooth = FRAC_CONST(0.25);
    } else {
        short_delay_band = 64;
        ps->nr_allpass_bands = 45;
        ps->alpha_decay = FRAC_CONST(0.58664621951003);
        ps->alpha_smooth = FRAC_CONST(0.6);
    }

    for (i = 0; i < short_delay_band; i++)
    {
        ps->delay_D[i] = 14;
    }
    for (i = short_delay_band; i < 64; i++)
    {
        ps->delay_D[i] = 1;
    }

    /* mixing and phase */
    for (i = 0; i < 50; i++)
    {
        RE(ps->h11_prev[i]) = 1;
        IM(ps->h12_prev[i]) = 1;
        RE(ps->h11_prev[i]) = 1;
        IM(ps->h12_prev[i]) = 1;
    }

    ps->phase_hist = 0;

    for (i = 0; i < 20; i++)
    {
        RE(ps->ipd_prev[i][0]) = 0;
        IM(ps->ipd_prev[i][0]) = 0;
        RE(ps->ipd_prev[i][1]) = 0;
        IM(ps->ipd_prev[i][1]) = 0;
        RE(ps->opd_prev[i][0]) = 0;
        IM(ps->opd_prev[i][0]) = 0;
        RE(ps->opd_prev[i][1]) = 0;
        IM(ps->opd_prev[i][1]) = 0;
    }

    return ps;
}

/* main Parametric Stereo decoding function */
uint8_t ps_decode(ps_info *ps, qmf_t X_left[38][64], qmf_t X_right[38][64])
{
    qmf_t X_hybrid_left[32][32] = {{0}};
    qmf_t X_hybrid_right[32][32] = {{0}};

    /* delta decoding of the bitstream data */
    ps_data_decode(ps);

    /* set up some parameters depending on filterbank type */
    if (ps->use34hybrid_bands)
    {
        ps->group_border = (uint8_t*)group_border34;
        ps->map_bins2group = (uint16_t*)map_bins2group34;
        ps->num_groups = 32+18;
        ps->num_hybrid_groups = 32;
        ps->nr_par_bands = 34;
        ps->decay_cutoff = 5;
    } else {
        ps->group_border = (uint8_t*)group_border20;
        ps->map_bins2group = (uint16_t*)map_bins2group20;
        ps->num_groups = 10+12;
        ps->num_hybrid_groups = 10;
        ps->nr_par_bands = 20;
        ps->decay_cutoff = 3;
    }

    /* Perform further analysis on the lowest subbands to get a higher
     * frequency resolution
     */
    hybrid_analysis((hyb_info*)ps->hyb, X_left, X_hybrid_left,
        ps->use34hybrid_bands);

    /* decorrelate mono signal */
    ps_decorrelate(ps, X_left, X_right, X_hybrid_left, X_hybrid_right);

    /* apply mixing and phase parameters */
    ps_mix_phase(ps, X_left, X_right, X_hybrid_left, X_hybrid_right);

    /* hybrid synthesis, to rebuild the SBR QMF matrices */
    hybrid_synthesis((hyb_info*)ps->hyb, X_left, X_hybrid_left,
        ps->use34hybrid_bands);

    hybrid_synthesis((hyb_info*)ps->hyb, X_right, X_hybrid_right,
        ps->use34hybrid_bands);

    return 0;
}

#endif

