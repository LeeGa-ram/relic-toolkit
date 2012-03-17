/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (C) 2007-2011 RELIC Authors
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
 * Implementation of point multiplication on binary elliptic curves.
 *
 * @version $Id$
 * @ingroup eb
 */

#include "string.h"

#include "relic_core.h"
#include "relic_fb_low.h"
#include "relic_eb.h"
#include "relic_error.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

#if EB_MUL == LWNAF || !defined(STRIP)

#if defined(EB_KBLTZ)

/**
 * Multiplies a binary elliptic curve point by an integer using the w-TNAF
 * method.
 *
 * @param[out] r 				- the result.
 * @param[in] p					- the point to multiply.
 * @param[in] k					- the integer.
 */
static void eb_mul_ltnaf_imp(eb_t r, eb_t p, bn_t k) {
	int len, i, n;
	signed char tnaf[FB_BITS + 8], *t, u;
	eb_t table[1 << (EB_WIDTH - 2)];
	bn_t vm, s0, s1;

	bn_null(vm);
	bn_null(s0);
	bn_null(s1);

	if (eb_curve_opt_a() == OPT_ZERO) {
		u = -1;
	} else {
		u = 1;
	}

	TRY {
		bn_new(vm);
		bn_new(s0);
		bn_new(s1);

		/* Prepare the precomputation table. */
		for (i = 0; i < (1 << (EB_WIDTH - 2)); i++) {
			eb_new(table[i]);
		}
		/* Compute the precomputation table. */
		eb_tab(table, p, EB_WIDTH);

		/* Compute the w-TNAF representation of k. */
		eb_curve_get_vm(vm);
		eb_curve_get_s0(s0);
		eb_curve_get_s1(s1);
		bn_rec_tnaf(tnaf, &len, k, vm, s0, s1, u, FB_BITS, EB_WIDTH);

		t = tnaf + len - 1;
		eb_set_infty(r);
		for (i = len - 1; i >= 0; i--, t--) {
			eb_frb(r, r);

			n = *t;
			if (n > 0) {
				eb_add(r, r, table[n / 2]);
			}
			if (n < 0) {
				eb_sub(r, r, table[-n / 2]);
			}
		}
		/* Convert r to affine coordinates. */
		eb_norm(r, r);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(vm);
		bn_free(s0);
		bn_free(s1);

		/* Free the precomputation table. */
		for (i = 0; i < (1 << (EB_WIDTH - 2)); i++) {
			eb_free(table[i]);
		}
	}
}

#endif

#if defined(EB_ORDIN) || defined(EB_SUPER)

/**
 * Multiplies a binary elliptic curve point by an integer using the
 * left-to-right w-NAF method.
 *
 * @param[out] r 				- the result.
 * @param[in] p					- the point to multiply.
 * @param[in] k					- the integer.
 */
static void eb_mul_lnaf_imp(eb_t r, eb_t p, bn_t k) {
	int len, i, n;
	signed char naf[FB_BITS + 1], *t;
	eb_t table[1 << (EB_WIDTH - 2)];

	for (i = 0; i < (1 << (EB_WIDTH - 2)); i++) {
		eb_null(table[i]);
	}

	TRY {
		/* Prepare the precomputation table. */
		for (i = 0; i < (1 << (EB_WIDTH - 2)); i++) {
			eb_new(table[i]);
			eb_set_infty(table[i]);
			fb_set_dig(table[i]->z, 1);
			table[i]->norm = 1;
		}
		/* Compute the precomputation table. */
		eb_tab(table, p, EB_WIDTH);

		/* Compute the w-TNAF representation of k. */
		bn_rec_naf(naf, &len, k, EB_WIDTH);

		t = naf + len - 1;

		eb_set_infty(r);
		for (i = len - 1; i >= 0; i--, t--) {
			eb_dbl(r, r);

			n = *t;
			if (n > 0) {
				eb_add(r, r, table[n / 2]);
			}
			if (n < 0) {
				eb_sub(r, r, table[-n / 2]);
			}
		}
		/* Convert r to affine coordinates. */
		eb_norm(r, r);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		/* Free the precomputation table. */
		for (i = 0; i < (1 << (EB_WIDTH - 2)); i++) {
			eb_free(table[i]);
		}
	}
}

#endif /* EB_ORDIN || EB_SUPER */
#endif /* EB_MUL == LWNAF */

#if EB_MUL == RWNAF || !defined(STRIP)

#if defined(EB_KBLTZ)

/**
 * Multiplies a binary elliptic curve point by an integer using the w-TNAF
 * method.
 *
 * @param[out] r 				- the result.
 * @param[in] p					- the point to multiply.
 * @param[in] k					- the integer.
 */
static void eb_mul_rtnaf_imp(eb_t r, eb_t p, bn_t k) {
	int len, i, n;
	signed char tnaf[FB_BITS + 8], *t, u;
	eb_t table[1 << (EB_WIDTH - 2)];
	bn_t vm, s0, s1;

	bn_null(vm);
	bn_null(s0);
	bn_null(s1);

	if (eb_curve_opt_a() == OPT_ZERO) {
		u = -1;
	} else {
		u = 1;
	}

	TRY {
		bn_new(vm);
		bn_new(s0);
		bn_new(s1);

		/* Prepare the precomputation table. */
		for (i = 0; i < (1 << (EB_WIDTH - 2)); i++) {
			eb_new(table[i]);
			eb_set_infty(table[i]);
		}
		/* Compute the precomputation table. */

		eb_curve_get_vm(vm);
		eb_curve_get_s0(s0);
		eb_curve_get_s1(s1);
		/* Compute the w-TNAF representation of k. */
		bn_rec_tnaf(tnaf, &len, k, vm, s0, s1, u, FB_BITS, EB_WIDTH);

		t = tnaf;
		eb_copy(r, p);
		for (i = 0; i < len; i++, t++) {
			n = *t;
			if (n > 0) {
				eb_add(table[n / 2], table[n / 2], r);
			}
			if (n < 0) {
				eb_sub(table[-n / 2], table[-n / 2], r);
			}

			/* We can avoid a function call here. */
			fb_sqr(r->x, r->x);
			fb_sqr(r->y, r->y);
		}

		eb_copy(r, table[0]);

#if defined(EB_MIXED) && defined(STRIP) && (EB_WIDTH > 2)
		eb_norm_sim(table + 1, table + 1, (1 << (EB_WIDTH - 2)) - 1);
#endif

#if EB_WIDTH == 3
		eb_frb(table[0], table[1]);
		if (u == 1) {
			eb_sub(table[1], table[1], table[0]);
		} else {
			eb_add(table[1], table[1], table[0]);
		}
#endif

#if EB_WIDTH == 4
		eb_frb(table[0], table[3]);
		eb_frb(table[0], table[0]);
		eb_frb(table[0], table[0]);

		if (u == 1) {
			eb_neg(table[0], table[0]);
		}
		eb_sub(table[3], table[0], table[3]);

		eb_frb(table[0], table[1]);
		eb_frb(table[0], table[0]);
		eb_sub(table[1], table[0], table[1]);

		eb_frb(table[0], table[2]);
		eb_frb(table[0], table[0]);
		eb_add(table[2], table[0], table[2]);
#endif

#if EB_WIDTH == 5
		eb_frb(table[0], table[3]);
		eb_frb(table[0], table[0]);
		eb_frb(table[0], table[0]);
		if (u == 1) {
			eb_neg(table[0], table[0]);
		}
		eb_sub(table[3], table[0], table[3]);

		eb_frb(table[0], table[1]);
		eb_frb(table[0], table[0]);
		eb_sub(table[1], table[0], table[1]);

		eb_frb(table[0], table[2]);
		eb_frb(table[0], table[0]);
		eb_add(table[2], table[0], table[2]);

		eb_frb(table[0], table[4]);
		eb_frb(table[0], table[0]);
		eb_add(table[0], table[0], table[4]);
		eb_frb(table[0], table[0]);
		eb_frb(table[0], table[0]);
		eb_frb(table[0], table[0]);
		if (u == 1) {
			eb_neg(table[0], table[0]);
		}
		eb_add(table[4], table[0], table[4]);

		eb_frb(table[0], table[5]);
		eb_frb(table[0], table[0]);
		eb_add(table[0], table[0], table[5]);
		eb_frb(table[0], table[0]);
		eb_frb(table[0], table[0]);
		eb_neg(table[0], table[0]);
		eb_sub(table[5], table[0], table[5]);

		eb_frb(table[0], table[6]);
		eb_frb(table[0], table[0]);
		eb_add(table[0], table[0], table[6]);
		eb_frb(table[0], table[0]);
		eb_frb(table[0], table[0]);
		eb_neg(table[0], table[0]);
		eb_add(table[6], table[0], table[6]);

		eb_frb(table[0], table[7]);
		eb_frb(table[0], table[0]);
		eb_frb(table[0], table[0]);
		eb_frb(table[0], table[0]);
		eb_sub(table[7], table[0], table[7]);
#endif

#if EB_WIDTH == 6
		eb_frb(table[0], table[1]);
		eb_frb(table[0], table[0]);
		eb_frb(table[0], table[0]);
		if (u == -1) {
			eb_neg(table[0], table[0]);
		}
		eb_add(table[0], table[0], table[1]);
		eb_frb(table[0], table[0]);
		eb_frb(table[0], table[0]);
		eb_sub(table[1], table[0], table[1]);

		eb_frb(table[0], table[2]);
		eb_frb(table[0], table[0]);
		eb_frb(table[0], table[0]);
		if (u == -1) {
			eb_neg(table[0], table[0]);
		}
		eb_add(table[0], table[0], table[2]);
		eb_frb(table[0], table[0]);
		eb_frb(table[0], table[0]);
		eb_add(table[2], table[0], table[2]);

		eb_frb(table[0], table[3]);
		eb_frb(table[0], table[0]);
		eb_add(table[0], table[0], table[3]);
		eb_neg(table[0], table[0]);
		eb_frb(table[0], table[0]);
		eb_frb(table[0], table[0]);
		eb_frb(table[0], table[0]);
		if (u == -1) {
			eb_neg(table[0], table[0]);
		}
		eb_sub(table[3], table[0], table[3]);

		eb_frb(table[0], table[4]);
		eb_frb(table[0], table[0]);
		eb_add(table[0], table[0], table[4]);
		eb_neg(table[0], table[0]);
		eb_frb(table[0], table[0]);
		eb_frb(table[0], table[0]);
		eb_frb(table[0], table[0]);
		if (u == -1) {
			eb_neg(table[0], table[0]);
		}
		eb_add(table[4], table[0], table[4]);

		eb_frb(table[0], table[5]);
		eb_frb(table[0], table[0]);
		eb_add(table[0], table[0], table[5]);
		eb_neg(table[0], table[0]);
		eb_frb(table[0], table[0]);
		eb_frb(table[0], table[0]);
		eb_sub(table[5], table[0], table[5]);

		eb_frb(table[0], table[6]);
		eb_frb(table[0], table[0]);
		eb_add(table[0], table[0], table[6]);
		eb_neg(table[0], table[0]);
		eb_frb(table[0], table[0]);
		eb_frb(table[0], table[0]);
		eb_add(table[6], table[0], table[6]);

		eb_frb(table[0], table[7]);
		eb_frb(table[0], table[0]);
		eb_frb(table[0], table[0]);
		eb_frb(table[0], table[0]);
		eb_sub(table[7], table[0], table[7]);

		eb_frb(table[0], table[8]);
		eb_frb(table[0], table[0]);
		eb_frb(table[0], table[0]);
		eb_frb(table[0], table[0]);
		eb_add(table[8], table[0], table[8]);

		eb_frb(table[0], table[9]);
		eb_frb(table[0], table[0]);
		eb_frb(table[0], table[0]);
		if (u == -1) {
			eb_neg(table[0], table[0]);
		}
		eb_add(table[0], table[0], table[9]);
		eb_frb(table[0], table[0]);
		eb_frb(table[0], table[0]);
		eb_sub(table[0], table[0], table[9]);
		eb_frb(table[0], table[0]);
		eb_frb(table[0], table[0]);
		eb_add(table[0], table[0], table[9]);
		eb_neg(table[9], table[0]);

		eb_frb(table[0], table[10]);
		eb_frb(table[0], table[0]);
		eb_neg(table[0], table[0]);
		eb_add(table[0], table[0], table[10]);
		eb_frb(table[0], table[0]);
		eb_frb(table[0], table[0]);
		eb_add(table[10], table[0], table[10]);

		eb_frb(table[0], table[11]);
		eb_frb(table[0], table[0]);
		eb_frb(table[0], table[0]);
		if (u == -1) {
			eb_neg(table[0], table[0]);
		}
		eb_sub(table[11], table[0], table[11]);

		eb_frb(table[0], table[12]);
		eb_frb(table[0], table[0]);
		eb_frb(table[0], table[0]);
		if (u == -1) {
			eb_neg(table[0], table[0]);
		}
		eb_add(table[12], table[0], table[12]);

		eb_frb(table[0], table[13]);
		eb_frb(table[0], table[0]);
		eb_add(table[0], table[0], table[13]);
		eb_neg(table[13], table[0]);

		eb_frb(table[0], table[14]);
		eb_frb(table[0], table[0]);
		eb_neg(table[0], table[0]);
		eb_add(table[14], table[0], table[14]);

		eb_frb(table[0], table[15]);
		eb_frb(table[0], table[0]);
		eb_frb(table[0], table[0]);
		eb_frb(table[0], table[0]);
		eb_frb(table[0], table[0]);
		if (u == -1) {
			eb_neg(table[0], table[0]);
		}
		eb_sub(table[15], table[0], table[15]);
#endif

#if defined(EB_MIXED) && defined(STRIP) && (EB_WIDTH > 2)
		eb_norm_sim(table + 1, table + 1, (1 << (EB_WIDTH - 2)) - 1);
#endif

		/* Add accumulators */
		for (i = 1; i < (1 << (EB_WIDTH - 2)); i++) {
			if (r->norm) {
				eb_add(r, table[i], r);
			} else {
				eb_add(r, r, table[i]);
			}
		}
		/* Convert r to affine coordinates. */
		eb_norm(r, r);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		bn_free(vm);
		bn_free(s0);
		bn_free(s1);

		/* Free the precomputation table. */
		for (i = 0; i < (1 << (EB_WIDTH - 2)); i++) {
			eb_free(table[i]);
		}
	}
}

#endif /* EB_KBLTZ */

#if defined(EB_ORDIN) || defined(EB_SUPER)

/**
 * Multiplies a binary elliptic curve point by an integer using the
 * right-to-left w-NAF method.
 *
 * @param[out] r 				- the result.
 * @param[in] p					- the point to multiply.
 * @param[in] k					- the integer.
 */
static void eb_mul_rnaf_imp(eb_t r, eb_t p, bn_t k) {
	int len, i, n;
	signed char naf[FB_BITS + 1], *t;
	eb_t table[1 << (EB_WIDTH - 2)];

	for (i = 0; i < (1 << (EB_WIDTH - 2)); i++) {
		eb_null(table[i]);
	}

	TRY {
		/* Prepare the accumulator table. */
		for (i = 0; i < (1 << (EB_WIDTH - 2)); i++) {
			eb_new(table[i]);
			eb_set_infty(table[i]);
		}

		/* Compute the w-TNAF representation of k. */
		bn_rec_naf(naf, &len, k, EB_WIDTH);

		t = naf;

		eb_copy(r, p);
		for (i = 0; i < len; i++, t++) {
			n = *t;
			if (n > 0) {
				eb_add(table[n / 2], table[n / 2], r);
			}
			if (n < 0) {
				eb_sub(table[-n / 2], table[-n / 2], r);
			}

			eb_dbl(r, r);
		}

		eb_copy(r, table[0]);

#if EB_WIDTH >= 3
		/* Compute 3 * T[1]. */
		eb_dbl(table[0], table[1]);
		eb_add(table[1], table[0], table[1]);
#endif
#if EB_WIDTH >= 4
		/* Compute 5 * T[2]. */
		eb_dbl(table[0], table[2]);
		eb_dbl(table[0], table[0]);
		eb_add(table[2], table[0], table[2]);

		/* Compute 7 * T[3]. */
		eb_dbl(table[0], table[3]);
		eb_dbl(table[0], table[0]);
		eb_dbl(table[0], table[0]);
		eb_sub(table[3], table[0], table[3]);
#endif
#if EB_WIDTH >= 5
		/* Compute 9 * T[4]. */
		eb_dbl(table[0], table[4]);
		eb_dbl(table[0], table[0]);
		eb_dbl(table[0], table[0]);
		eb_add(table[4], table[0], table[4]);

		/* Compute 11 * T[5]. */
		eb_dbl(table[0], table[5]);
		eb_dbl(table[0], table[0]);
		eb_add(table[0], table[0], table[5]);
		eb_dbl(table[0], table[0]);
		eb_add(table[5], table[0], table[5]);

		/* Compute 13 * T[6]. */
		eb_dbl(table[0], table[6]);
		eb_add(table[0], table[0], table[6]);
		eb_dbl(table[0], table[0]);
		eb_dbl(table[0], table[0]);
		eb_add(table[6], table[0], table[6]);

		/* Compute 15 * T[7]. */
		eb_dbl(table[0], table[7]);
		eb_dbl(table[0], table[0]);
		eb_dbl(table[0], table[0]);
		eb_dbl(table[0], table[0]);
		eb_sub(table[7], table[0], table[7]);
#endif
#if EB_WIDTH == 6
		for (i = 8; i < 15; i++) {
			eb_mul_dig(table[i], table[i], 2 * i + 1);
		}
		eb_dbl(table[0], table[15]);
		eb_dbl(table[0], table[0]);
		eb_dbl(table[0], table[0]);
		eb_dbl(table[0], table[0]);
		eb_dbl(table[0], table[0]);
		eb_sub(table[15], table[0], table[15]);
#endif

		/* Add accumulators */
		for (i = 1; i < (1 << (EB_WIDTH - 2)); i++) {
			if (r->norm) {
				eb_add(r, table[i], r);
			} else {
				eb_add(r, r, table[i]);
			}
		}
		/* Convert r to affine coordinates. */
		eb_norm(r, r);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		/* Free the accumulator table. */
		for (i = 0; i < (1 << (EB_WIDTH - 2)); i++) {
			eb_free(table[i]);
		}
	}
}

#endif /* EB_ORDIN || EB_SUPER */
#endif /* EB_MUL == RWNAF */

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if EB_MUL == BASIC || !defined(STRIP)

void eb_mul_basic(eb_t r, eb_t p, bn_t k) {
	int i, l;
	eb_t t;

	eb_null(t);

	if (bn_is_zero(k)) {
		eb_set_infty(r);
		return;
	}

	TRY {
		eb_new(t);

		l = bn_bits(k);

		eb_copy(t, p);

		for (i = l - 2; i >= 0; i--) {
			eb_dbl(t, t);
			if (bn_test_bit(k, i)) {
				eb_add(t, t, p);
			}
		}

		eb_norm(r, t);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		eb_free(t);
	}
}

#endif

#if defined(EB_ORDIN) || defined(EB_KBLTZ)

#if EB_MUL == LODAH || !defined(STRIP)

void eb_mul_lodah(eb_t r, eb_t p, bn_t k) {
	int i, t;
	dv_t x1, z1, x2, z2, r1, r2, r3, r4, r5;
	dig_t *b;

	if (eb_curve_is_super()) {
		THROW(ERR_NO_VALID);
	}

	dv_null(x1);
	dv_null(z1);
	dv_null(x2);
	dv_null(z2);
	dv_null(r1);
	dv_null(r2);
	dv_null(r3);
	dv_null(r4);
	dv_null(r5);

	TRY {
		dv_new(x1);
		dv_new(z1);
		dv_new(x2);
		dv_new(z2);
		dv_new(r1);
		dv_new(r2);
		dv_new(r3);
		dv_new(r4);
		dv_new(r5);

		fb_copy(x1, p->x);
		fb_zero(z1);
		fb_set_bit(z1, 0, 1);
		fb_sqr(z2, p->x);
		fb_sqr(x2, z2);
		dv_zero(r5, 2 * FB_DIGS);

		b = eb_curve_get_b();

		switch (eb_curve_opt_b()) {
			case OPT_ZERO:
				break;
			case OPT_ONE:
				fb_add_dig(x2, x2, (dig_t)1);
				break;
			case OPT_DIGIT:
				fb_add_dig(x2, x2, b[0]);
				break;
			default:
				fb_add(x2, x2, b);
				break;
		}

		t = bn_bits(k);
		for (i = t - 2; i >= 0; i--) {
			fb_mul(r1, x1, z2);
			fb_mul(r2, x2, z1);
			fb_add(r3, r1, r2);
			fb_muln_low(r4, r1, r2);
			if (bn_test_bit(k, i) == 1) {
				fb_sqr(z1, r3);
				fb_muln_low(r1, z1, p->x);
				fb_addd_low(x1, r1, r4, 2 * FB_DIGS);
				fb_rdcn_low(x1, x1);
				fb_sqr(r1, z2);
				fb_sqr(r2, x2);
				fb_mul(z2, r1, r2);
				switch (eb_curve_opt_b()) {
					case OPT_ZERO:
						fb_sqr(x2, r2);
						break;
					case OPT_ONE:
						fb_add(r1, r1, r2);
						fb_sqr(x2, r1);
						break;
					case OPT_DIGIT:
						fb_sqr(r1, r1);
						fb_sqrl_low(x2, r2);
						fb_mul1_low(r5, r1, b[0]);
						fb_addd_low(x2, x2, r5, FB_DIGS + 1);
						fb_rdcn_low(x2, x2);
						break;
					default:
						fb_sqr(r1, r1);
						fb_sqrl_low(x2, r2);
						fb_muln_low(r5, r1, b);
						fb_addd_low(x2, x2, r5, 2 * FB_DIGS);
						fb_rdcn_low(x2, x2);
						break;
				}
			} else {
				fb_sqr(z2, r3);
				fb_muln_low(r1, z2, p->x);
				fb_addd_low(x2, r1, r4, 2 * FB_DIGS);
				fb_rdcn_low(x2, x2);
				fb_sqr(r1, z1);
				fb_sqr(r2, x1);
				fb_mul(z1, r1, r2);
				switch (eb_curve_opt_b()) {
					case OPT_ZERO:
						fb_sqr(x1, r2);
						break;
					case OPT_ONE:
						fb_add(r1, r1, r2);
						fb_sqr(x1, r1);
						break;
					case OPT_DIGIT:
						fb_sqr(r1, r1);
						fb_sqrl_low(x1, r2);
						fb_mul1_low(r5, r1, b[0]);
						fb_addd_low(x1, x1, r5, FB_DIGS + 1);
						fb_rdcn_low(x1, x1);
						break;
					default:
						fb_sqr(r1, r1);
						fb_sqrl_low(x1, r2);
						fb_muln_low(r5, r1, b);
						fb_addd_low(x1, x1, r5, 2 * FB_DIGS);
						fb_rdcn_low(x1, x1);
						break;
				}
			}
		}

		if (fb_is_zero(z1)) {
			/* The point q is at infinity. */
			eb_set_infty(r);
		} else {
			if (fb_is_zero(z2)) {
				fb_copy(r->x, p->x);
				fb_add(r->y, p->x, p->y);
				fb_zero(r->z);
				fb_set_bit(r->z, 0, 1);
			} else {
				/* r3 = z1 * z2. */
				fb_mul(r3, z1, z2);
				/* z1 = (x1 + x * z1). */
				fb_mul(z1, z1, p->x);
				fb_add(z1, z1, x1);
				/* z2 = x * z2. */
				fb_mul(z2, z2, p->x);
				/* x1 = x1 * z2. */
				fb_mul(x1, x1, z2);
				/* z2 = (x2 + x * z2)(x1 + x * z1). */
				fb_add(z2, z2, x2);
				fb_mul(z2, z2, z1);

				/* r4 = (x^2 + y) * z1 * z2 + (x2 + x * z2)(x1 + x * z1). */
				fb_sqr(r4, p->x);
				fb_add(r4, r4, p->y);
				fb_mul(r4, r4, r3);
				fb_add(r4, r4, z2);

				/* r3 = (z1 * z2 * x)^{-1}. */
				fb_mul(r3, r3, p->x);
				fb_inv(r3, r3);
				/* r4 = (x^2 + y) * z1 * z2 + (x2 + x * z2)(x1 + x * z1) * r3. */
				fb_mul(r4, r4, r3);
				/* x2 = x1 * x * z2 * (z1 * z2 * x)^{-1} = x1/z1. */
				fb_mul(x2, x1, r3);
				/* z2 = x + x1/z1. */
				fb_add(z2, x2, p->x);

				/* z2 = z2 * r4 + y. */
				fb_mul(z2, z2, r4);
				fb_add(z2, z2, p->y);

				fb_copy(r->x, x2);
				fb_copy(r->y, z2);
				fb_zero(r->z);
				fb_set_bit(r->z, 0, 1);

				r->norm = 1;
			}
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		dv_free(x1);
		dv_free(z1);
		dv_free(x2);
		dv_free(z2);
		dv_free(r1);
		dv_free(r2);
		dv_free(r3);
		dv_free(r4);
		dv_free(r5);
	}
}

#endif /* EB_ORDIN || EB_KBLTZ */
#endif /* EB_MUL == LODAH */

#if EB_MUL == LWNAF || !defined(STRIP)

void eb_mul_lwnaf(eb_t r, eb_t p, bn_t k) {
#if defined(EB_KBLTZ)
	if (eb_curve_is_kbltz()) {
		eb_mul_ltnaf_imp(r, p, k);
		return;
	}
#endif

#if defined(EB_ORDIN) || defined(EB_SUPER)
	eb_mul_lnaf_imp(r, p, k);
#endif
}

#endif

#if EB_MUL == RWNAF || !defined(STRIP)

void eb_mul_rwnaf(eb_t r, eb_t p, bn_t k) {
#if defined(EB_KBLTZ)
	if (eb_curve_is_kbltz()) {
		eb_mul_rtnaf_imp(r, p, k);
		return;
	}
#endif

#if defined(EB_ORDIN) || defined(EB_SUPER)
#if defined(EB_MIXED) && defined(STRIP)
	/* It is impossible to run a right-to-left algorithm using ordinary curves
	 * and only mixed additions. */
	THROW(ERR_NO_CONFIG);
#else
	eb_mul_rnaf_imp(r, p, k);
#endif
#endif
}

#endif

#if EB_MUL == HALVE || !defined(STRIP)

void eb_mul_halve(eb_t r, eb_t p, bn_t k) {
	int len, i, j, l, cof;
	signed char naf[FB_BITS + 1] = { 0 }, *tmp;
	eb_t t, q, table[1 << (EB_WIDTH - 2)];
	bn_t n, _k;
	fb_t u, v, w, z;

	bn_null(_k);
	bn_null(n);
	eb_null(q);
	for (i = 0; i < (1 << (EB_WIDTH - 2)); i++) {
		eb_null(table[i]);
	}
	fb_null(u);
	fb_null(v);
	fb_null(w);
	fb_null(z);

	TRY {
		bn_new(n);
		bn_new(_k);
		eb_new(q);
		eb_new(t);
		fb_new(u);
		fb_new(v);
		fb_new(w);
		fb_new(z);

		/* Prepare the precomputation table. */
		for (i = 0; i < (1 << (EB_WIDTH - 2)); i++) {
			eb_new(table[i]);
			eb_set_infty(table[i]);
		}

		/* Convert k to alternate representation k' = (2^{t-1}k mod n). */
		eb_curve_get_ord(n);
		bn_lsh(_k, k, bn_bits(n) - 1);
		bn_mod(_k, _k, n);

		/* Compute the w-NAF representation of k'. */
		bn_rec_naf(naf, &len, _k, EB_WIDTH);

		for (i = len; i <= bn_bits(n); i++) {
			naf[i] = 0;
		}
		if (naf[bn_bits(n)] == 1) {
			eb_dbl(table[0], p);
		}
		len = bn_bits(n);
		tmp = naf + len - 1;

		eb_copy(q, p);
		eb_curve_get_cof(n);

		/* Test if curve has a cofactor bigger than 2. */
		if (bn_cmp_dig(n, 2) == CMP_GT) {
			cof = 1;
		} else {
			cof = 0;
		}

		l = fb_trc(eb_curve_get_a());

		if (cof) {
			/* Curves with cofactor > 2, u = sqrt(a), v = Solve(u). */
			fb_srt(u, eb_curve_get_a());
			fb_slv(v, u);

			bn_rand(n, BN_POS, len);

			for (i = len - 1; i >= 0; i--, tmp--) {
				j = *tmp;
				if (j > 0) {
					eb_norm(t, q);
					eb_add(table[j / 2], table[j / 2], t);
				}
				if (j < 0) {
					eb_norm(t, q);
					eb_sub(table[-j / 2], table[-j / 2], t);
				}

				/* T = 1/2(Q). */
				eb_hlv(t, q);

				/* If Tr(x_T) != Tr(a). */
				if (fb_trc(t->x) != 0) {
					/* z = l_t, w = sqrt(l_Q), l_T = l_T + sqrt(l_Q) + v. */
					fb_copy(z, t->y);
					fb_srt(w, q->y);
					fb_add(t->y, t->y, w);
					fb_add(t->y, t->y, v);
					/* z = (z + x_Q + v + sqrt(a)). */
					fb_add(z, z, q->x);
					fb_add(z, z, v);
					fb_add(z, z, u);
					/* w = sqrt(w + x_Q + l_Q + sqrt(a)). */
					fb_add(w, w, q->x);
					fb_add(w, w, q->y);
					fb_add(w, w, u);
					/* x_T = sqrt(w * z), . */
					fb_mul(w, w, z);
					fb_srt(t->x, w);
					fb_set_dig(t->z, 1);
					t->norm = 2;
				}
				eb_copy(q, t);
			}
		} else {
			for (i = len - 1; i >= 0; i--, tmp--) {
				j = *tmp;
				if (j > 0) {
					eb_norm(q, q);
					eb_add(table[j / 2], table[j / 2], q);
				}
				if (j < 0) {
					eb_norm(q, q);
					eb_sub(table[-j / 2], table[-j / 2], q);
				}
				eb_hlv(q, q);
			}
		}

#if EB_WIDTH == 2
		eb_norm(r, table[0]);
#else
		/* Compute Q_i = Q_i + Q_{i+2} for i from 2^{w-1}-3 to 1. */
		for (i = (1 << (EB_WIDTH - 1)) - 3; i >= 1; i -= 2) {
			eb_add(table[i / 2], table[i / 2], table[(i + 2) / 2]);
		}
		/* Compute R = Q_1 + 2 * sum_{i != 1}Q_i. */
		eb_copy(r, table[1]);
		for (i = 2; i < (1 << (EB_WIDTH - 2)); i++) {
			eb_add(r, r, table[i]);
		}
		eb_dbl(r, r);
		eb_add(r, r, table[0]);
		eb_norm(r, r);
#endif

		/* We may need to fix an error of a 2-torsion point if the curve has a
		 * 4-cofactor. */
		if (cof) {
			eb_hlv(t, r);
			if (fb_trc(t->x) != l) {
				fb_zero(t->x);
				fb_srt(t->y, eb_curve_get_b());
				fb_set_dig(t->z, 1);
				eb_add(r, r, t);
				eb_norm(r, r);
			}
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		/* Free the precomputation table. */
		for (i = 0; i < (1 << (EB_WIDTH - 2)); i++) {
			eb_free(table[i]);
		}
		bn_free(n);
		bn_free(_k);
		eb_free(t);
		eb_free(q);
		fb_free(u);
		fb_free(v);
		fb_free(w);
		fb_free(z);
	}
}

#endif

void eb_mul_gen(eb_t r, bn_t k) {
#ifdef EB_PRECO
	eb_mul_fix(r, eb_curve_get_tab(), k);
#else
	eb_t gen;

	eb_null(gen);

	TRY {
		eb_new(gen);
		eb_curve_get_gen(gen);
		eb_mul(r, gen, k);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		eb_free(gen);
	}
#endif
}

void eb_mul_dig(eb_t r, eb_t p, dig_t k) {
	int i, l;
	eb_t t;

	eb_null(t);

	if (k == 0) {
		eb_set_infty(r);
		return;
	}

	TRY {
		eb_new(t);

		l = util_bits_dig(k);

		eb_copy(t, p);

		for (i = l - 2; i >= 0; i--) {
			eb_dbl(t, t);
			if (k & ((dig_t)1 << i)) {
				eb_add(t, t, p);
			}
		}

		eb_norm(r, t);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		eb_free(t);
	}
}