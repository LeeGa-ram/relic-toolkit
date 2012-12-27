/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (C) 2007-2012 RELIC Authors
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
 * Implementation of the core functions for computing pairings over binary
 * fields.
 *
 * @version $Id$
 * @ingroup pb
 */

#include <math.h>

#include "relic_core.h"
#include "relic_conf.h"
#include "relic_bench.h"
#include "relic_pb.h"
#include "relic_util.h"
#include "relic_error.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

#ifdef PB_PARAL

/**
 * Precomputed table for computing fixed 2^i powers in the pairing.
 */
fb_st pb_tab_sqr[CORES][FB_TABLE];

/**
 * Pointers to the precomputed table of fixed 2^i powers.
 */
fb_st *pb_sqr_ptr[CORES][FB_TABLE];

/**
 * Precomputed table for computing fixed 1/(2^i) powers in the pairing.
 */
fb_st pb_tab_srt[CORES][FB_TABLE];

/**
 * Pointers to the precomputed table of fixed 1/(2^i) powers.
 */
fb_st *pb_srt_ptr[CORES][FB_TABLE];

/**
 * Partition for the parallel execution.
 */
double pb_par[CORES];

#endif

/**
 * Precomputed table for the final exponentiation.
 */
fb_st pb_tab_exp[FB_TABLE];

/**
 * Pointers to the precomputed table for the final exponentiation.
 */
fb_st *pb_exp_ptr[FB_TABLE];

/**
 * Computes a partition of the main loop in the pairing algorithm.
 */
static void pb_compute_par() {
	fb_t a, b;
	long long r;

	fb_null(a);
	fb_null(b);

	TRY {
		fb_new(a);
		fb_new(b);

		fb_rand(a);
		fb_rand(b);

		bench_reset();
		bench_before();
		for (int i = 0; i < BENCH; i++) {
			fb_mul(a, a, b);
		}
		bench_after();
		bench_compute(BENCH);
		r = bench_total();

		bench_reset();
		bench_before();
		for (int i = 0; i < BENCH; i++) {
			fb_sqr(a, b);
		}
		bench_after();
		bench_compute(BENCH);
		r /= bench_total();
	} CATCH_ANY {
		THROW(ERR_CAUGHT);
	} FINALLY {
		fb_free(a);
		fb_free(b);
	}
}

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

void pb_map_init() {

	for (int i = 0; i < FB_TABLE; i++) {
		pb_exp_ptr[i] = &(pb_tab_exp[i]);
	}

#if PB_MAP == ETATS || PB_MAP == ETATN
	fb_itr_pre(pb_map_get_tab(), 4 * (((FB_BITS + 1) / 2) / 4));
#endif

#if PB_MAP == ETAT2 || PB_MAP == OETA2
	fb_itr_pre(pb_map_get_tab(), 6 * (((FB_BITS - 1) / 2) / 6));
#endif

#if defined(PB_PARAL) && (PB_MAP == ETATS || PB_MAP == ETATN)

	pb_compute_par();

	int chunk = (int)ceilf((FB_BITS - 1) / (2.0 * CORES));

	for (int i = 0; i < CORES; i++) {
		for (int j = 0; j < FB_TABLE; j++) {
			pb_sqr_ptr[i][j] = &(pb_tab_sqr[i][j]);
			pb_srt_ptr[i][j] = &(pb_tab_srt[i][j]);
		}
	}

	for (int i = 0; i < CORES; i++) {
		fb_itr_pre(pb_map_get_sqr(i), i * chunk);
		fb_itr_pre(pb_map_get_srt(i), -i * chunk);
	}

#endif
}

void pb_map_clean() {

}

fb_t *pb_map_get_tab() {
#if ALLOC == AUTO
	return (fb_t *)*pb_exp_ptr;
#else
	return (fb_t *)pb_exp_ptr;
#endif
}

fb_t *pb_map_get_sqr(int core) {
#ifdef PB_PARAL

#if ALLOC == AUTO
	return (fb_t *)*pb_sqr_ptr[core];
#else
	return (fb_t *)pb_sqr_ptr[core];
#endif

#else
	return NULL;
#endif
}

fb_t *pb_map_get_srt(int core) {
#ifdef PB_PARAL

#if ALLOC == AUTO
	return (fb_t *)*pb_srt_ptr[core];
#else
	return (fb_t *)pb_srt_ptr[core];
#endif

#else
	return NULL;
#endif
}

int pb_map_get_par(int core) {
	int chunk = (int)ceilf((FB_BITS - 1) / (2.0 * CORES));

	if (core == CORES) {
		return MIN((FB_BITS - 1) / 2, core * chunk);
	} else {
		return core * chunk;
	}
}