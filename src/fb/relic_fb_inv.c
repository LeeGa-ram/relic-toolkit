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
 * Implementation of binary field inversion functions.
 *
 * @version $Id$
 * @ingroup fb
 */

#include "relic_core.h"
#include "relic_fb.h"
#include "relic_fb_low.h"
#include "relic_bn_low.h"
#include "relic_util.h"
#include "relic_rand.h"
#include "relic_error.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if FB_INV == BASIC || !defined(STRIP)

void fb_inv_basic(fb_t c, fb_t a) {
	fb_t t, u, v;
	int i, x;

	fb_null(t);
	fb_null(u);
	fb_null(v);

	TRY {
		fb_new(t);
		fb_new(u);
		fb_new(v);

		/* u = a^2, v = 1, x = (m - 1)/2. */
		fb_sqr(u, a);
		fb_set_dig(v, 1);
		x = (FB_BITS - 1) >> 1;

		while (x != 0) {
			/* u = u * a^{2x}. */
			fb_copy(t, u);
			for (i = 0; i < x; i++) {
				fb_sqr(t, t);
			}
			fb_mul(u, u, t);
			if ((x & 0x01) == 0) {
				x = x >> 1;
			} else {
				/* v = v * u, u = u^2, x = (x - 1)/2. */
				fb_mul(v, v, u);
				fb_sqr(u, u);
				x = (x - 1) >> 1;
			}
		}
		fb_copy(c, v);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fb_free(t);
		fb_free(u);
		fb_free(v);
	}
}

#endif

#if FB_INV == BINAR || !defined(STRIP)

void fb_inv_binar(fb_t c, fb_t a) {
	int lu, lv;
	fb_t u, v, g1, g2;

	fb_null(u);
	fb_null(v);
	fb_null(g1);
	fb_null(g2);

	TRY {
		fb_new(u);
		fb_new(v);
		fb_new(g1);
		fb_new(g2);

		/* u = a, v = f, g1 = 1, g2 = 0. */
		fb_copy(u, a);
		fb_copy(v, fb_poly_get());
		fb_set_dig(g1, 1);
		fb_zero(g2);

		lu = FB_DIGS;
		lv = FB_DIGS;

		/* While (u != 1 && v != 1. */
		while (1) {
			/* While z divides u do. */
			while ((u[0] & 0x01) == 0) {
				/* u = u/z. */
				bn_rsh1_low(u, u, lu);
				/* If z divides g1 then g1 = g1/z; else g1 = (g1 + f)/z. */
				if ((g1[0] & 0x01) == 1) {
					fb_poly_add(g1, g1);
				}
				fb_rsh1_low(g1, g1);
			}

			while (u[lu - 1] == 0)
				lu--;
			if (lu == 1 && u[0] == 1)
				break;

			/* While z divides v do. */
			while ((v[0] & 0x01) == 0) {
				/* v = v/z. */
				bn_rsh1_low(v, v, lv);
				/* If z divides g2 then g2 = g2/z; else (g2 = g2 + f)/z. */
				if ((g2[0] & 0x01) == 1) {
					fb_poly_add(g2, g2);
				}
				fb_rsh1_low(g2, g2);
			}

			while (v[lv - 1] == 0)
				lv--;
			if (lv == 1 && v[0] == 1)
				break;

			/* If deg(u) > deg(v) then u = u + v, g1 = g1 + g2. */
			if (lu > lv || (lu == lv && u[lu - 1] > v[lv - 1])) {
				fb_addd_low(u, u, v, lv);
				fb_add(g1, g1, g2);
			} else {
				/* Else v = v + u, g2 = g2 + g1. */
				fb_addd_low(v, v, u, lu);
				fb_add(g2, g2, g1);
			}
		}

		/* If u == 1 then return g1; else return g2. */
		if (lu == 1 && u[0] == 1) {
			fb_copy(c, g1);
		} else {
			fb_copy(c, g2);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fb_free(u);
		fb_free(v);
		fb_free(g1);
		fb_free(g2);
	}
}

#endif

#if FB_INV == EXGCD || !defined(STRIP)

void fb_inv_exgcd(fb_t c, fb_t a) {
	int j, d, lu, lv, lt, l1, l2, bu, bv;
	dv_t _u, _v, _g1, _g2;
	dig_t *t = NULL, *u = NULL, *v = NULL, *g1 = NULL, *g2 = NULL, carry;

	dv_null(_u);
	dv_null(_v);
	dv_null(_g1);
	dv_null(_g2);

	TRY {
		dv_new(_u);
		dv_new(_v);
		dv_new(_g1);
		dv_new(_g2);
		dv_zero(_g1, FB_DIGS + 1);
		dv_zero(_g2, FB_DIGS + 1);

		u = _u;
		v = _v;
		g1 = _g1;
		g2 = _g2;

		/* u = a, v = f, g1 = 1, g2 = 0. */
		fb_copy(u, a);
		fb_copy(v, fb_poly_get());
		g1[0] = 1;

		lu = lv = FB_DIGS;
		l1 = l2 = 1;

		bu = fb_bits(u);
		bv = FB_BITS + 1;
		j = bu - bv;

		/* While (u != 1). */
		while (1) {
			/* If j < 0 then swap(u, v), swap(g1, g2), j = -j. */
			if (j < 0) {
				t = u;
				u = v;
				v = t;

				lt = lu;
				lu = lv;
				lv = lt;

				t = g1;
				g1 = g2;
				g2 = t;

				lt = l1;
				l1 = l2;
				l2 = lt;

				j = -j;
			}

			SPLIT(j, d, j, FB_DIG_LOG);

			/* u = u + v * z^j. */
			if (j > 0) {
				carry = fb_lshadd_low(u + d, v, j, lv);
				u[d + lv] ^= carry;
			} else {
				fb_addd_low(u + d, u + d, v, lv);
			}

			/* g1 = g1 + g2 * z^j. */
			if (j > 0) {
				carry = fb_lshadd_low(g1 + d, g2, j, l2);
				l1 = (l2 + d >= l1 ? l2 + d : l1);
				if (carry) {
					g1[d + l2] ^= carry;
					l1 = (l2 + d >= l1 ? l1 + 1 : l1);
				}
			} else {
				fb_addd_low(g1 + d, g1 + d, g2, l2);
				l1 = (l2 + d > l1 ? l2 + d : l1);
			}

			while (u[lu - 1] == 0)
				lu--;
			while (v[lv - 1] == 0)
				lv--;

			if (lu == 1 && u[0] == 1)
				break;

			/* j = deg(u) - deg(v). */
			lt = util_bits_dig(u[lu - 1]) - util_bits_dig(v[lv - 1]);
			j = ((lu - lv) << FB_DIG_LOG) + lt;
		}
		/* Return g1. */
		fb_copy(c, g1);

	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		dv_free(_u);
		dv_free(_v);
		dv_free(_g1);
		dv_free(_g2);
	}
}

#endif

#if FB_INV == ALMOS || !defined(STRIP)

void fb_inv_almos(fb_t c, fb_t a) {
	int lu, lv, lt;
	fb_t _b, _d, _u, _v;
	dig_t *t = NULL, *u = NULL, *v = NULL, *b = NULL, *d = NULL;

	fb_null(_b);
	fb_null(_d);
	fb_null(_u);
	fb_null(_v);

	TRY {
		fb_new(_b);
		fb_new(_d);
		fb_new(_u);
		fb_new(_v);

		b = _b;
		d = _d;
		u = _u;
		v = _v;

		/* b = 1, d = 0, u = a, v = f. */
		fb_set_dig(b, 1);
		fb_zero(d);
		fb_copy(u, a);
		fb_copy(v, fb_poly_get());

		lu = FB_DIGS;
		lv = FB_DIGS;
		while (1) {
			/* While z divides u do. */
			while ((u[0] & 0x01) == 0) {
				/* u = u/z. */
				bn_rsh1_low(u, u, lu);
				/* If z divide v then b = b/z; else b = (b + f)/z. */
				if ((b[0] & 0x01) == 1) {
					fb_poly_add(b, b);
				}
				/* b often has FB_DIGS digits. */
				fb_rsh1_low(b, b);
			}
			/* If u = 1, return b. */
			while (u[lu - 1] == 0)
				lu--;
			if (lu == 1 && u[0] == 1) {
				break;
			}
			/* If deg(u) < deg(v) then swap(u, v), swap(b, d). */
			if ((lu < lv) || ((lu == lv) && (u[lu - 1] < v[lv - 1]))) {
				t = u;
				u = v;
				v = t;

				/* Swap lu and lv too. */
				lt = lu;
				lu = lv;
				lv = lt;

				t = b;
				b = d;
				d = t;
			}
			/* u = u + v, b = b + d. */
			fb_addd_low(u, u, v, lu);
			fb_addn_low(b, b, d);
		}
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		fb_copy(c, b);
		fb_free(_b);
		fb_free(_d);
		fb_free(_u);
		fb_free(_v);
	}
}

#endif

#if FB_INV == ITOHT || !defined(STRIP)

void fb_inv_itoht(fb_t c, fb_t a) {
	int i, j, x, y;
	int *chain, len;

	chain = fb_poly_get_chain(&len);

	int u[len + 1];
	fb_t table[len + 1];
	for (i = 0; i <= len; i++) {
		fb_null(table[i]);
	}

	TRY {
		fb_new(t);
		fb_new(u);
		fb_new(v);
		for (i = 0; i <= len; i++) {
			fb_new(table[i]);
		}

		u[0] = 1;
		u[1] = 2;
		fb_copy(table[0], a);
		fb_sqr(table[1], table[0]);
		fb_mul(table[1], table[1], table[0]);
		for (i = 2; i <= len; i++) {
			x = chain[i - 1] >> 8;
			y = chain[i - 1] - (x << 8);
			if (x == y) {
				u[i] = 2 * u[i - 1];
			} else {
				u[i] = u[x] + u[y];
			}
			fb_sqr(table[i], table[x]);
			for (j = 1; j < u[y]; j++) {
				fb_sqr(table[i], table[i]);
			}
			fb_mul(table[i], table[i], table[y]);
		}
		fb_sqr(c, table[len]);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		for (i = 0; i <= len; i++) {
			fb_free(table[i]);
		}
	}
}

#endif
#if FB_INV == LOWER || !defined(STRIP)

void fb_inv_lower(fb_t c, fb_t a) {
	fb_invn_low(c, a);
}
#endif

void fb_inv_sim(fb_t *c, fb_t *a, int n) {
	int i;
	fb_t u, t[n];

	for (i = 0; i < n; i++) {
		fb_null(t[i]);
	}
	fb_null(u);

	TRY {
		for (i = 0; i < n; i++) {
			fb_new(t[i]);
		}
		fb_new(u);

		fb_copy(c[0], a[0]);
		fb_copy(t[0], a[0]);

		for (i = 1; i < n; i++) {
			fb_copy(t[i], a[i]);
			fb_mul(c[i], c[i - 1], a[i]);
		}

		fb_inv(u, c[n - 1]);

		for (i = n - 1; i > 0; i--) {
			fb_mul(c[i], u, c[i - 1]);
			fb_mul(u, u, t[i]);
		}
		fb_copy(c[0], u);
	}
	CATCH_ANY {
		THROW(ERR_CAUGHT);
	}
	FINALLY {
		for (i = 0; i < n; i++) {
			fb_free(t[i]);
		}
		fb_free(u);
	}
}