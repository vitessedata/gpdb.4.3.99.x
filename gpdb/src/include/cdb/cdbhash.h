/*--------------------------------------------------------------------------
*
* cdbhash.h
*	 Definitions and API functions for cdbhash.c
*
* Copyright (c) 2005-2008, Greenplum inc
*
*--------------------------------------------------------------------------
*/
#ifndef CDBHASH_H
#define CDBHASH_H

/*
 * Structure that holds Greenplum Database hashing information.
 */
typedef struct CdbHash
{
	uint32		hash;			/* The result hash value							*/
	int			numsegs;		/* number of segments in Greenplum Database used for
								 * partitioning  */
	int         mask;			/* only used when numsegs is powerof 2 */
	uint32		rrindex;		/* round robin index for empty policy tables		*/
	
} CdbHash;


typedef void (*datumHashFunction)(void *clientData, void *buf, size_t len);

/*
 * Create and initialize a CdbHash in the current memory context.
 * Parameter numsegs - number of segments in Greenplum Database.
 * Parameter algorithm - the hash algorithm, either HASH_FNV_1 or HASH_FNV_1A
 */
extern CdbHash *makeCdbHash(int numsegs);

/*
 * Initialize CdbHash for hashing the next tuple values.
 */
extern void cdbhashinit(CdbHash *h);

/*
 * Add an attribute to the hash calculation.
 */
extern void cdbhash(CdbHash *h, Datum val, Oid typid, FmgrInfo *hfn);

/*
 * Add a NULL attribute to the hash calculation.
 */
extern void cdbhashnull(CdbHash *h);

/*
 * Hash a tuple for a relation with an empty (no hash keys) partitioning policy.
 */
extern void cdbhashnokey(CdbHash *h);

/*
 * Reduce the hash to a segment number.
 */
static inline unsigned int cdbhashreduce(CdbHash* h)
{
	return h->mask ? (h->hash & h->mask) : (h->hash % h->numsegs);
}

extern unsigned int cdbhashreduce(CdbHash *h);

/*
 * Return true if Oid is hashable internally in Greenplum Database.
 */
extern bool isGreenplumDbHashable(Oid typid);

/*
 * Return true if the Oid is an array type.  This can be used prior
 *   to hashing the datum because array typeoids are expected to
 *   have been converted to any array oid.
 */
extern bool typeIsArrayType(Oid typeoid);

extern int ignoreblanks(char *data, int len);


#endif   /* CDBHASH_H */
