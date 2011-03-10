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
 * Implementation of the prime elliptic curve utilities.
 *
 * @version $Id$
 * @ingroup ep
 */

#include "relic_core.h"
#include "relic_md.h"
#include "relic_ep.h"
#include "relic_error.h"
#include "relic_conf.h"
#include "relic_fp_low.h"

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

int ep_is_infty(ep_t p) {
	return (fp_is_zero(p->z) == 1);
}

void ep_set_infty(ep_t p) {
	fp_zero(p->x);
	fp_zero(p->y);
	fp_zero(p->z);
	p->norm = 1;
}

void ep_copy(ep_t r, ep_t p) {
	fp_copy(r->x, p->x);
	fp_copy(r->y, p->y);
	fp_copy(r->z, p->z);
	r->norm = p->norm;
}

int ep_cmp(ep_t p, ep_t q) {
	if (fp_cmp(p->x, q->x) != CMP_EQ) {
		return CMP_NE;
	}

	if (fp_cmp(p->y, q->y) != CMP_EQ) {
		return CMP_NE;
	}

	if (fp_cmp(p->z, q->z) != CMP_EQ) {
		return CMP_NE;
	}

	return CMP_EQ;
}

void ep_rand(ep_t p) {
	bn_t n, k;

	bn_null(k);
	bn_null(n);

	TRY {
		bn_new(k);
		bn_new(n);

		ep_curve_get_ord(n);

		bn_rand(k, BN_POS, bn_bits(n));
		bn_mod(k, k, n);

		ep_mul_gen(p, k);
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		bn_free(k);
		bn_free(n);
	}
}

void ep_print(ep_t p) {
	fp_print(p->x);
	fp_print(p->y);
	if (!p->norm) {
		for (int i = FP_DIGS - 1; i >= 0; i--) {
			util_print("%.*lX ", (int)(2 * sizeof(dig_t)),
					(unsigned long int)p->z[i]);
		}
		util_print("\n");
	} else {
		fp_print(p->z);
	}
}