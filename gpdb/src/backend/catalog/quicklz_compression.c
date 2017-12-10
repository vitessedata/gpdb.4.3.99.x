/*
 * Copyright (c) 2011 EMC Corporation All Rights Reserved
 *
 * This software is protected, without limitation, by copyright law
 * and international treaties. Use of this software and the intellectual
 * property contained therein is expressly limited to the terms and
 * conditions of the License Agreement under which it is provided by
 * or on behalf of EMC.
 *
 * ---------------------------------------------------------------------
 *
 * The quicklz implementation is not provided due to licensing issues.  The
 * following stub implementation is built if a proprietary implementation is
 * not provided.
 *
 * Note that other compression algorithms actually work better for new
 * installations anyway.
 */

/*
 * EXX: EXX_IN_PG
 * 
 * Hi-jacking quicklz functions.   We use quicklz functions to actually 
 * call lz4 or zstd, so that we do not need to modify catalog to add 
 * new functions, function oids ...
 *
 * Impl loosely follows code in pg_compression.c, zlib_xxx functions.
 */

#include "postgres.h"
#include "utils/builtins.h"
#include "catalog/pg_compression.h"
#include "lz4.h"
#include "zstd.h"

#define COMPALG_LZ4     0 
#define COMPALG_ZSTD    0x100 

Datum
quicklz_constructor(PG_FUNCTION_ARGS)
{
    /* PG_GETARG_POINTER(0) is TupleDesc that is currently unused. 
     * It is passed in as NULL*/
    StorageAttributes *sa = PG_GETARG_POINTER(1);
    CompressionState *cs = palloc0(sizeof(CompressionState));

    /* bool compress = PG_GETARG_BOOL(2); */
    Insist(PointerIsValid(sa->comptype));
    if (pg_strcasecmp("lz4", sa->comptype) == 0) {
        if (sa->complevel == 0) {
            sa->complevel = 1;
        }
        cs->opaque = (void *) COMPALG_LZ4; 
    } else {
        intptr_t x = COMPALG_ZSTD;

        /* 
         * The internet cannot agree on the default compression lv of zstd, but generall people 
         * think it should be between 1-6
         */
        if (sa->complevel == 0) {
            sa->complevel = 5;
        }

        /*
         * Compression level 1-19 are considered OK, 20-22 are considered ultra.  Ultra consumes
         * more memory.  Let's not go there.
         */
        if (sa->complevel > 20) {
            sa->complevel = 19;
        }

        x = x | sa->complevel;
        cs->opaque = (void *) x; 
    }
    cs->desired_sz = NULL;

    PG_RETURN_POINTER(cs);
}

Datum
quicklz_destructor(PG_FUNCTION_ARGS)
{
    /* CompressionState *cs = PG_GETARG_POINTER(0); */
    /* As in zlib_destructor, we do not pfree cs. */
	PG_RETURN_VOID();
}

    Datum
quicklz_compress(PG_FUNCTION_ARGS)
{
    const void *src = PG_GETARG_POINTER(0);
    int32_t src_sz = PG_GETARG_INT32(1);
    void *dst = PG_GETARG_POINTER(2);
    int32_t dst_sz = PG_GETARG_INT32(3);
    int32_t *dst_used = (int32_t *) PG_GETARG_POINTER(4);
    CompressionState *cs = (CompressionState *) PG_GETARG_POINTER(5); 

    intptr_t flag = (intptr_t) cs->opaque;
    int compmethod = flag & 0xFF00;
    int complv = flag & 0xFF;

    if (compmethod == COMPALG_LZ4) { 
        int32_t zb = LZ4_compress_default(src, dst, src_sz, dst_sz);
        if (zb == 0) {
            /* 
             * XXX: Hack.
             * Compression failed, or there is not enough space.  In this case, we set 
             * dst_used to src_sz, caller will detect this as compression not benifitial
             * and handle.
             */

            *dst_used = src_sz;
        } else {
            *dst_used = zb;
        }
    } else {
        size_t zb;
        Assert(compmethod == COMPALG_ZSTD);
        Assert(complv > 0 && complv < 20);
        zb = ZSTD_compress(dst, dst_sz, src, src_sz, complv);
        if (ZSTD_isError(zb)) {
            *dst_used = src_sz;
        } else {
            *dst_used = zb;
        }
    }

    PG_RETURN_VOID();
}

    Datum
quicklz_decompress(PG_FUNCTION_ARGS)
{
    const char	   *src	= PG_GETARG_POINTER(0);
    int32_t			src_sz = PG_GETARG_INT32(1);
    void		   *dst	= PG_GETARG_POINTER(2);
    int32_t			dst_sz = PG_GETARG_INT32(3);
    int32_t		   *dst_used = PG_GETARG_POINTER(4);
    CompressionState *cs = (CompressionState *) PG_GETARG_POINTER(5);

    intptr_t flag = (intptr_t) cs->opaque;
    int compmethod = flag & 0xFF00;

    if (compmethod == COMPALG_LZ4) {
        int32_t zb;
        Insist(src_sz > 0 && dst_sz > 0);
        zb = LZ4_decompress_fast(src, dst, dst_sz); 
        if (zb != src_sz) {
            elog(ERROR, "lz4_decompress error");
        } 
        *dst_used = dst_sz;
    } else {
        size_t zb;
        Assert(compmethod == COMPALG_ZSTD);
        zb = ZSTD_decompress(dst, dst_sz, src, src_sz);
        if (ZSTD_isError(zb)) {
            elog(ERROR, "zstd decompress error");
        }
        *dst_used = zb;
    }

    PG_RETURN_VOID();
}

    Datum
quicklz_validator(PG_FUNCTION_ARGS)
{
    /*
     * Do not bother to validate? 
     */
    PG_RETURN_VOID();
}
