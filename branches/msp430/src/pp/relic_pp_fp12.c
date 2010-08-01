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
 * Implementation of the dodecic extension prime field arithmetic module.
 *
 * @version $Id$
 * @ingroup fp12
 */

#include <string.h>

#include "relic_core.h"
#include "relic_conf.h"
#include "relic_pp.h"
#include "relic_util.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/**
 * Squaring in the internal quadratic extension over fp2.
 *
 * @param c					- the first component of the result.
 * @param d					- the second component of the result.
 * @param a					- the first component of the input.
 * @param b					- the second component of the input.
 */
static void fp4_sqr(fp2_t c, fp2_t d, fp2_t a, fp2_t b) {
	fp2_t t0, t1;

	fp2_null(t0);
	fp2_null(t1);

	TRY {
		/* t0 = a^2. */
		fp2_sqr(t0, a);
		/* t1 = b^2. */
		fp2_sqr(t1, b);

		/* c = a^2  + b^2 * E. */
		fp2_mul_nor(c, t1);
		fp2_add(c, c, t0);

		/* d = (a + b)^2 - a^2 - b^2 = 2 * a * b. */
		fp2_add(d, a, b);
		fp2_sqr(d, d);
		fp2_sub(d, d, t0);
		fp2_sub(d, d, t1);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		fp2_free(t0);
		fp2_free(t1);
	}
}

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void fp12_copy(fp12_t c, fp12_t a) {
	fp6_copy(c[0], a[0]);
	fp6_copy(c[1], a[1]);
}

void fp12_neg(fp12_t c, fp12_t a) {
	fp6_neg(c[0], a[0]);
	fp6_neg(c[1], a[1]);
}

void fp12_zero(fp12_t a) {
	fp6_zero(a[0]);
	fp6_zero(a[1]);
}

int fp12_is_zero(fp12_t a) {
	return (fp6_is_zero(a[0]) && fp6_is_zero(a[1]));
}

void fp12_rand(fp12_t a) {
	fp6_rand(a[0]);
	fp6_rand(a[1]);
}

void fp12_print(fp12_t a) {
	fp6_print(a[0]);
	fp6_print(a[1]);
}

int fp12_cmp(fp12_t a, fp12_t b) {
	return ((fp6_cmp(a[0], b[0]) == CMP_EQ) &&
			(fp6_cmp(a[1], b[1]) == CMP_EQ) ? CMP_EQ : CMP_NE);
}

void fp12_add(fp12_t c, fp12_t a, fp12_t b) {
	fp6_add(c[0], a[0], b[0]);
	fp6_add(c[1], a[1], b[1]);
}

void fp12_sub(fp12_t c, fp12_t a, fp12_t b) {
	fp6_sub(c[0], a[0], b[0]);
	fp6_sub(c[1], a[1], b[1]);
}

void fp12_mul(fp12_t c, fp12_t a, fp12_t b) {
	fp6_t t0, t1, t2;

	fp6_null(t0);
	fp6_null(t1);
	fp6_null(t2);

	TRY {
		fp6_new(t0);
		fp6_new(t1);
		fp6_new(t2);

		/* Karatsuba algorithm. */
		fp6_mul(t0, a[0], b[0]);
		fp6_mul(t1, a[1], b[1]);
		fp6_add(t2, b[0], b[1]);
		fp6_add(c[1], a[1], a[0]);
		fp6_mul(c[1], c[1], t2);
		fp6_sub(c[1], c[1], t0);
		fp6_sub(c[1], c[1], t1);
		fp6_mul_art(t1, t1);
		fp6_add(c[0], t0, t1);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		fp6_free(t0);
		fp6_free(t1);
		fp6_free(t2);
	}
}

void fp12_mul_dxs(fp12_t c, fp12_t a, fp12_t b) {
	fp6_t v0, v1, t0;

	fp6_null(v0);
	fp6_null(v1);
	fp6_null(t0);

	TRY {
		fp6_new(v0);
		fp6_new(v1);
		fp6_new(t0);

		/* c1 = (a0 + a1)(b0 + b1) */
		fp6_add(v0, a[0], a[1]);
		fp2_add(v1[0], b[0][0], b[1][0]);
		fp2_copy(v1[1], b[1][1]);
		fp6_mul_dxs(t0, v0, v1);

		/* v0 = a0b0 */
		fp6_mul_dxq(v0, a[0], b[0][0]);

		/* v1 = a1b1 */
		fp6_mul(v1, a[1], b[1]);

		/* c1 = c1 - v0 - v1 */
		fp6_sub(c[1], t0, v0);
		fp6_sub(c[1], c[1], v1);

		/* c0 = v0 + v * v1 */
		fp6_mul_art(v1, v1);
		fp6_add(c[0], v0, v1);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		fp6_free(v0);
		fp6_free(v1);
		fp6_free(t0);
	}
}

void fp12_sqr(fp12_t c, fp12_t a) {
	fp6_t t0, t1;

	fp6_null(t0);
	fp6_null(t1);

	TRY {
		fp6_new(t0);
		fp6_new(t1);

		fp6_add(t0, a[0], a[1]);
		fp6_mul_art(t1, a[1]);
		fp6_add(t1, a[0], t1);
		fp6_mul(t0, t0, t1);
		fp6_mul(c[1], a[0], a[1]);
		fp6_sub(c[0], t0, c[1]);
		fp6_mul_art(t1, c[1]);
		fp6_sub(c[0], c[0], t1);
		fp6_dbl(c[1], c[1]);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		fp6_free(t0);
		fp6_free(t1);
	}
}

void fp12_sqr_uni(fp12_t c, fp12_t a) {
	fp2_t t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t;

	/* Define z = sqrt(E). */

	/* Now a is seen as (t0,t1) + (t2,t3) * w + (t4,t5) * w^2. */

	/* (t0, t1) = (a00 + a11*z)^2. */
	fp4_sqr(t0, t1, a[0][0], a[1][1]);
	/* (t2, t3) = (a10 + a02*z)^2. */
	fp4_sqr(t2, t3, a[1][0], a[0][2]);
	/* (t4, t5) = (a01 + a12*z)^2. */
	fp4_sqr(t4, t5, a[0][1], a[1][2]);

	/* (t6, t9) = -2 * conj(a00, a11). */
	fp2_dbl(t6, a[0][0]);
	fp2_neg(t6, t6);
	fp2_dbl(t9, a[1][1]);
	/* (t7, t10) = -2 * conj(a01, a12). */
	fp2_dbl(t7, a[0][1]);
	fp2_neg(t7, t7);
	fp2_dbl(t11, a[1][2]);
	/* (t8, t11) = -2 * conj(a10, a02). */
	fp2_dbl(t8, a[0][2]);
	fp2_neg(t8, t8);
	fp2_dbl(t10, a[1][0]);

	/* (t4, t5) = E * (t4, t5). */
	fp2_mul_nor(t, t5);
	fp2_copy(t5, t4);
	fp2_copy(t4, t);

	/* (t0, t1) = 3 * (t0, t1). */
	fp2_dbl(t, t0);
	fp2_add(t0, t0, t);
	fp2_dbl(t, t1);
	fp2_add(t1, t1, t);
	/* (t2, t3) = 3 * (t2, t3). */
	fp2_dbl(t, t2);
	fp2_add(t2, t2, t);
	fp2_dbl(t, t3);
	fp2_add(t3, t3, t);
	/* (t4, t5) = 3 * (t4, t5). */
	fp2_dbl(t, t4);
	fp2_add(t4, t4, t);
	fp2_dbl(t, t5);
	fp2_add(t5, t5, t);

	fp2_add(t0, t0, t6);
	fp2_add(t1, t1, t9);
	fp2_add(t2, t2, t7);
	fp2_add(t3, t3, t11);
	fp2_add(t4, t4, t10);
	fp2_add(t5, t5, t8);

	fp2_copy(c[0][0], t0);
	fp2_copy(c[0][1], t2);
	fp2_copy(c[0][2], t5);
	fp2_copy(c[1][0], t4);
	fp2_copy(c[1][1], t1);
	fp2_copy(c[1][2], t3);
}

void fp12_inv(fp12_t c, fp12_t a) {
	fp6_t t0;
	fp6_t t1;

	fp6_null(t0);
	fp6_null(t1);

	TRY {
		fp6_new(t0);
		fp6_new(t1);

		fp6_sqr(t0, a[0]);
		fp6_sqr(t1, a[1]);
		fp6_mul_art(t1, t1);
		fp6_sub(t0, t0, t1);
		fp6_inv(t0, t0);

		fp6_mul(c[0], a[0], t0);
		fp6_neg(c[1], a[1]);
		fp6_mul(c[1], c[1], t0);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		fp6_free(t0);
		fp6_free(t1);
	}
}

void fp12_inv_uni(fp12_t c, fp12_t a) {
	/* In this case, it's a simple conjugate. */
	fp6_copy(c[0], a[0]);
	fp6_neg(c[1], a[1]);
}

void fp12_frb(fp12_t c, fp12_t a) {
	fp2_t t;

	fp2_null(t);

	TRY {
		fp2_new(t);

		fp2_const_get(t);
		fp6_frb(c[0], a[0]);
		fp6_frb(c[1], a[1]);
		fp6_mul_dxq(c[1], c[1], t);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		fp2_free(t);
	}
}

void fp12_exp(fp12_t c, fp12_t a, bn_t b) {
	fp12_t t;

	fp12_null(t);

	TRY {
		fp12_new(t);

		fp12_copy(t, a);

		for (int i = bn_bits(b) - 2; i >= 0; i--) {
			fp12_sqr(t, t);
			if (bn_test_bit(b, i)) {
				fp12_mul(t, t, a);
			}
		}
		fp12_copy(c, t);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fp12_free(t);
	}
}

void fp12_exp_uni(fp12_t c, fp12_t a, bn_t b) {
	fp12_t t;

	fp12_null(t);

	TRY {
		fp12_new(t);

		fp12_copy(t, a);

		for (int i = bn_bits(b) - 2; i >= 0; i--) {
			fp12_sqr_uni(t, t);
			if (bn_test_bit(b, i)) {
				fp12_mul(t, t, a);
			}
		}
		fp12_copy(c, t);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fp12_free(t);
	}
}