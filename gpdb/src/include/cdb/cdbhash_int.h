#ifndef CDBHASH_INT_H
#define CDBHASH_INT_H

/*
 * 32 bit FNV-1 and FNV-1a non-zero initial basis
 * NOTE: The FNV-1a initial basis is the same value as FNV-1 by definition.
 */
#define FNV1_32_INIT ((uint32)0x811c9dc5)
#define FNV1_32A_INIT FNV1_32_INIT

/* Constant prime value used for an FNV1/FNV1A hash */
#define FNV_32_PRIME ((uint32)0x01000193)

/* Constant used for hashing a NULL value */
#define NULL_VAL ((uint32)0XF0F0F0F1)

/* Constant used for hashing a NAN value  */
#define NAN_VAL ((uint32)0XE0E0E0E1)

/* Constant used for hashing an invalid value  */
#define INVALID_VAL ((uint32)0XD0D0D0D1)

/* Constant used to help defining upper limit for random generator */
#define UPPER_VAL ((uint32)0XA0B0C0D1)

uint32 fnv1_32_buf(void *buf, size_t len, uint32 hashval);
int    inet_getkey(inet *addr, unsigned char *inet_key, int key_size);

#endif /* CDBHASH_INT_H */
