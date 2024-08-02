/*
 * Copyright: (c) 2006-2020, 2020 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:	sfft.h
 * Purpose:	Software Fast Fourier Transformationn
 * History:
 *	Jun, 2020	xlai 	Create
 */
#ifndef __FFT_H__
#define __FFT_H__
#include "types.h"

typedef struct complex_s {
	double real;
	double imag;
} complex_t;

extern double complex_abs(complex_t *in);
extern void complex_add(complex_t *a, complex_t *b, complex_t *out);
extern void complex_sub(complex_t *a, complex_t *b, complex_t *out);
extern void complex_mul(complex_t *a, complex_t *b, complex_t *out);
extern void complex_copy(complex_t *dst, complex_t *src);

extern int32_t sfft_init_twiddles(complex_t t[], int32_t n);
extern int32_t sfft(complex_t a[], uint32_t n, complex_t twiddles[]);

#endif	/* end of __FFT_H__ */
