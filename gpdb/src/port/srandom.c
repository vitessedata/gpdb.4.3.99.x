/*-------------------------------------------------------------------------
 *
 * srandom.c
 *	  srandom() wrapper
 *
 * Portions Copyright (c) 1996-2009, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $PostgreSQL: pgsql/src/port/srandom.c,v 1.10 2009/01/01 17:24:04 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "c.h"

#include <math.h>

static long int dbg_seed;

void
srandom(unsigned int seed)
{
    dbg_seed = seed;
	srand48((long int) seed);
}
