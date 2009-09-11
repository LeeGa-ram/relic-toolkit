/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (C) 2007, 2008, 2009 RELIC Authors
 *
 * This file is part of RELIC. RELIC is legal property of its developers,
 * whose names are not listed here. Please refer to the COPYRIGHT file
 * for contact information.
 *
 * RELIC is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * RELIC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with RELIC. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file
 *
 * Implementation of the low-level prime field modular reduction functions.
 *
 * @version $Id$
 * @ingroup fp
 */

#include "relic_core.h"
#include "relic_fp.h"
#include "relic_fp_low.h"
#include "relic_bn_low.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/**
 * Accumulates a double precision digit in a triple register variable.
 *
 * @param[in,out] R2		- most significant word of the triple register.
 * @param[in,out] R1		- middle word of the triple register.
 * @param[in,out] R0		- lowest significant word of the triple register.
 * @param[in] A				- the first digit to multiply.
 * @param[in] B				- the second digit to multiply.
 */
#define COMBA_STEP(R2, R1, R0, A, B)										\
	dbl_t r = (dbl_t)(A) * (dbl_t)(B);										\
	dig_t _r = (R1);														\
	(R0) += (dig_t)(r);														\
	(R1) += (R0) < (dig_t)(r);												\
	(R2) += (R1) < _r;														\
	(R1) += (dig_t)(r >> (dbl_t)BN_DIGIT);								\
	(R2) += (R1) < (dig_t)(r >> (dbl_t)BN_DIGIT);						\

/**
 * Accumulates a single precision digit in a triple register variable.
 *
 * @param[in,out] R2		- most significant word of the triple register.
 * @param[in,out] R1		- middle word of the triple register.
 * @param[in,out] R0		- lowest significant word of the triple register.
 * @param[in] A				- the first digit to accumulate.
 */
#define COMBA_ADD(R2, R1, R0, A)											\
	dig_t __r = (R1);														\
	(R0) += (A);															\
	(R1) += (R0) < (A);														\
	(R2) += (R1) < __r;														\

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fp_rdcs_low(dig_t *c, dig_t *a, dig_t *m) {
	align dig_t q[2 * FP_DIGS], _q[2 * FP_DIGS], r[2 * FP_DIGS], t[2 * FP_DIGS];
	int *sform, len;
	int first, i, j, b0, d0, b1, d1;
	dig_t carry, carry2;

	sform = fp_prime_get_sform();
	len = 0;
	for (int i = 0; sform[i] != 0; i++) {
		len++;
	}

	SPLIT(b0, d0, FP_BITS, FP_DIG_LOG);
	first = (d0) + (b0 == 0 ? 0 : 1);

	/* q = floor(x/b^k) */
	dv_zero(q, 2 * FP_DIGS);
	bn_rshd_low(q, a, 2 * FP_DIGS, d0);
	bn_rshb_low(q, q, 2 * FP_DIGS, b0);

	dv_zero(c, FP_DIGS);
	dv_copy(c, a, first + 1);
	c[first] &= MASK(b0);
	dv_zero(r, 2 * FP_DIGS);
	fp_copy(r, c);

	printf("\na = ");
	dv_print(a, 2 * FP_DIGS);

	carry = i = 0;
	while (!fp_is_zero(q)) {
		printf("q = ");
		dv_print(q, 2 * FP_DIGS);
		dv_zero(_q, 2 * FP_DIGS);
		for (i = len - 2; i > 0; i--) {
			j = (sform[i] < 0 ? -sform[i] : sform[i]);
			SPLIT(b1, d1, j, FP_DIG_LOG);
			dv_zero(t, 2 * FP_DIGS);
			bn_lshd_low(t, q, FP_DIGS, d1);
			bn_lshb_low(t, t, 2 * FP_DIGS, b1);
			if (sform[i] > 0) {
				bn_subn_low(_q, _q, t, 2 * FP_DIGS);
			} else {
				bn_addn_low(_q, _q, t, 2 * FP_DIGS);
			}
			printf("_q = ");
			dv_print(_q, 2 * FP_DIGS);
		}
		if (sform[0] > 0) {
			bn_subn_low(_q, _q, q, FP_DIGS);
		} else {
			bn_addn_low(_q, _q, q, FP_DIGS);
		}
		printf("_q = ");
		dv_print(_q, 2 * FP_DIGS);
		bn_rshd_low(q, _q, 2 * FP_DIGS, d0);
		bn_rshb_low(q, q, 2 * FP_DIGS, b0);
		dv_copy(r, _q, first + 1);
		r[first] &= MASK(b0);
		i++;
		if (i % 2 == 0) {
			carry = fp_subn_low(c, c, r);
		} else {
			carry2 = fp_addn_low(c, c, r);
		}
		printf("q = ");
		dv_print(q, FP_DIGS);
	}

	if (!carry) {
		while (fp_cmpn_low(c, m) == CMP_GT) {
			fp_subn_low(c, c, m);
		}
	} else {
		while (carry) {
			fp_addn_low(c, c, m);
		}
	}
	dv_print(c, FP_DIGS);
}

dig_t fp_rdcn_low(dig_t *c, dig_t *a, dig_t *m, dig_t u) {
	int i, j;
	dig_t r0, r1, r2;
	dig_t *tmp, *tmpm, *tmpc;

	tmpc = c;

	r0 = r1 = r2 = 0;
	for (i = 0; i < FP_DIGS; i++, tmpc++, a++) {
		tmp = c;
		tmpm = m + i;
		for (j = 0; j < i; j++, tmp++, tmpm--) {
			COMBA_STEP(r2, r1, r0, *tmp, *tmpm);
		}
		COMBA_ADD(r2, r1, r0, *a);
		*tmpc = (dig_t)(r0 * u);
		COMBA_STEP(r2, r1, r0, *tmpc, *m);
		r0 = r1;
		r1 = r2;
		r2 = 0;
	}
	for (i = FP_DIGS; i < 2 * FP_DIGS - 1; i++, a++) {
		tmp = c + (i - FP_DIGS + 1);
		tmpm = m + FP_DIGS - 1;
		for (j = i - FP_DIGS + 1; j < FP_DIGS; j++, tmp++, tmpm--) {
			COMBA_STEP(r2, r1, r0, *tmp, *tmpm);
		}
		COMBA_ADD(r2, r1, r0, *a);
		c[i - FP_DIGS] = r0;
		r0 = r1;
		r1 = r2;
		r2 = 0;
	}
	COMBA_ADD(r2, r1, r0, *a);
	c[FP_DIGS - 1] = r0;

	return r1;
}
