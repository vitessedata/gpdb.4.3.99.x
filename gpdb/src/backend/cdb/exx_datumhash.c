#include "postgres.h"
#include "utils/numeric.h"
#include "utils/date.h"
#include "utils/nabstime.h"
#include "utils/inet.h"
#include "utils/varbit.h"
#include "utils/acl.h"
#include "utils/cash.h"
#include "cdb/cdbhash_int.h"
#include "cdb/cdbhash.h"
#include "access/tuptoaster.h"

#include "exx_oss.h" 

/* legacy hash ... see hashDatum() in cdbhash.c */

typedef exx_datum_hashfn_t hfn_t;

static uint32_t hfn_INT2OID(Datum datum, uint32_t hval)
{
    int64_t val;
    val = (int64_t) DatumGetInt16(datum);
    return fnv1_32_buf(&val, sizeof(val), hval);
}


static uint32_t hfn_INT4OID(Datum datum, uint32_t hval)
{
    int64_t val;
    val = (int64_t) DatumGetInt32(datum);
    return fnv1_32_buf(&val, sizeof(val), hval);
}

static uint32_t hfn_INT8OID(Datum datum, uint32_t hval)
{
    int64_t val;
    val = DatumGetInt64(datum);
    return fnv1_32_buf(&val, sizeof(val), hval);
}

static uint32_t hfn_FLOAT4OID(Datum datum, uint32_t hval)
{
    float val;
    val = DatumGetFloat4(datum);
    if (val == 0) val = 0.0;
    return fnv1_32_buf(&val, sizeof(val), hval);
}

static uint32_t hfn_FLOAT8OID(Datum datum, uint32_t hval)
{
    double val;
    val = DatumGetFloat8(datum);
    if (val == 0) val = 0.0;
    return fnv1_32_buf(&val, sizeof(val), hval);
}


static uint32_t hfn_NUMERICOID(Datum datum, uint32_t hval)
{
    Numeric num = DatumGetNumeric(datum);
    uint32_t nanbuf;
    void* buf;
    int len;
    
    if (NUMERIC_IS_NAN(num)) {
        nanbuf = NAN_VAL;
        buf = &nanbuf;
        len = sizeof(nanbuf);
    } else {
        /* not a nan */
        buf = num->n_data;
        len = (VARSIZE(num) - NUMERIC_HDRSZ);
    }

    uint32_t ret = fnv1_32_buf(buf, len, hval);
    if (num != DatumGetPointer(datum))
        pfree(num);

    return ret;
}


static uint32_t hfn_CHAROID(Datum datum, uint32_t hval)
{
    char ch = DatumGetChar(datum);
    return fnv1_32_buf(&ch, 1, hval);
}


static uint32_t hfn_VARCHAROID(Datum datum, uint32_t hval)
{
    int len;
    char* buf;
    void* tofree;
    varattrib_untoast_ptr_len(datum, &buf, &len, &tofree);
    /* adjust length to not include trailing blanks */
    if (len > 1)
        len = ignoreblanks(buf, len);

    uint32_t ret = fnv1_32_buf(buf, len, hval);
    if (tofree)
        pfree(tofree);
    return ret;
}

static hfn_t* hfn_BPCHAROID = hfn_VARCHAROID;
static hfn_t* hfn_TEXTOID = hfn_VARCHAROID;

static uint32_t hfn_BYTEAOID(Datum datum, uint32_t hval)
{
    int len;
    char* buf;
    void* tofree;
    varattrib_untoast_ptr_len(datum, &buf, &len, &tofree);
    uint32_t ret = fnv1_32_buf(buf, len, hval);
    if (tofree)
        pfree(tofree);
    return ret;
}


static uint32_t hfn_NAMEOID(Datum datum, uint32_t hval)
{
    Name namebuf = DatumGetName(datum);
    int len = NAMEDATALEN;
    char* buf = NameStr(*namebuf);
    if (len > 1)
        len = ignoreblanks(buf, len);
    return fnv1_32_buf(buf, len, hval);
}

static uint32_t hfn_OIDOID(Datum datum, uint32_t hval)
{
    int64_t val = (int64_t) DatumGetUInt32(datum);
    return fnv1_32_buf(&val, sizeof(val), hval);
}

static hfn_t* hfn_REGPROCOID = hfn_OIDOID;
static hfn_t* hfn_REGPROCEDUREOID = hfn_OIDOID;
static hfn_t* hfn_REGOPEROID = hfn_OIDOID;
static hfn_t* hfn_REGOPERATOROID = hfn_OIDOID;
static hfn_t* hfn_REGCLASSOID = hfn_OIDOID;
static hfn_t* hfn_REGTYPEOID = hfn_OIDOID;

static uint32_t hfn_TIDOID(Datum datum, uint32_t hval)
{
    char* p = DatumGetPointer(datum);
    return fnv1_32_buf(p, SizeOfIptrData, hval);
}

static uint32_t hfn_TIMESTAMPOID(Datum datum, uint32_t hval)
{
    Timestamp val = DatumGetTimestamp(datum);
    return fnv1_32_buf(&val, sizeof(val), hval);
}
static uint32_t hfn_TIMESTAMPTZOID(Datum datum, uint32_t hval)
{
    TimestampTz val = DatumGetTimestampTz(datum);
    return fnv1_32_buf(&val, sizeof(val), hval);
}

static uint32_t hfn_DATEOID(Datum datum, uint32_t hval)
{
    DateADT val = DatumGetDateADT(datum);
    return fnv1_32_buf(&val, sizeof(val), hval);
}

static uint32_t hfn_TIMEOID(Datum datum, uint32_t hval)
{
    TimeADT val = DatumGetTimeADT(datum);
    return fnv1_32_buf(&val, sizeof(val), hval);
}

static uint32_t hfn_TIMETZOID(Datum datum, uint32_t hval)
{
    TimeTzADT* p = DatumGetTimeTzADTP(datum);
    /*
     * Specify hash length as sizeof(double) + sizeof(int4), not as
     * sizeof(TimeTzADT), so that any garbage pad bytes in the structure
     * won't be included in the hash!
     */
    int len = sizeof(p->time) + sizeof(p->zone);
    return fnv1_32_buf(p, len, hval);
}

static uint32_t hfn_INTERVALOID(Datum datum, uint32_t hval)
{
    Interval* p = DatumGetIntervalP(datum);
    /*
     * Specify hash length as sizeof(double) + sizeof(int4), not as
     * sizeof(Interval), so that any garbage pad bytes in the structure
     * won't be included in the hash!
     */
    int len = sizeof(p->time) + sizeof(p->month);
    return fnv1_32_buf(p, len, hval);
}

static uint32_t hfn_ABSTIMEOID(Datum datum, uint32_t hval)
{
	AbsoluteTime abstime = DatumGetAbsoluteTime(datum);
    void* buf;
    int len;
    uint32_t invalidbuf;
    if (abstime == INVALID_ABSTIME) {
        invalidbuf = INVALID_VAL;
        len = sizeof(invalidbuf);
        buf = &invalidbuf;
    } else {
        len = sizeof(abstime);
        buf = &abstime;
    }
    return fnv1_32_buf(buf, len, hval);
}
        
static uint32_t hfn_RELTIMEOID(Datum datum, uint32_t hval)
{
    RelativeTime reltime =  DatumGetRelativeTime(datum);
    void* buf;
    int len;
    uint32_t invalidbuf;
			
    if (reltime == INVALID_RELTIME) {
        /* hash to a constant value */
        invalidbuf = INVALID_VAL;
        len = sizeof(invalidbuf);
        buf = &invalidbuf;
    } else {
        len = sizeof(reltime);
        buf = &reltime;
    }
    return fnv1_32_buf(buf, len, hval);
}

static uint32_t hfn_TINTERVALOID(Datum datum, uint32_t hval)
{
	TimeInterval tinterval = DatumGetTimeInterval(datum);
	AbsoluteTime tinterval_len;
    void* buf;
    int len;
    uint32_t invalidbuf;
			
    /*
     * check if a valid interval. the '0' status code
     * stands for T_INTERVAL_INVAL which is defined in
     * nabstime.c. We use the actual value instead
     * of defining it again here.
     */
    if(tinterval->status == 0 ||
       tinterval->data[0] == INVALID_ABSTIME ||
       tinterval->data[1] == INVALID_ABSTIME) {
        /* hash to a constant value */
        invalidbuf = INVALID_VAL;
        len = sizeof(invalidbuf);
        buf = &invalidbuf;				
    } else {
        /* normalize on length of the time interval */
        tinterval_len = tinterval->data[1] -  tinterval->data[0];
        len = sizeof(tinterval_len);
        buf = &tinterval_len;	
    }

    return fnv1_32_buf(buf, len, hval);
}


static uint32_t hfn_INETOID(Datum datum, uint32_t hval)
{
    inet* p = DatumGetInetP(datum);
    unsigned char buf[sizeof(inet_struct)];
    int len = inet_getkey(p, buf, sizeof(buf));
    return fnv1_32_buf(buf, len, hval);
}

static hfn_t* hfn_CIDROID = hfn_INETOID;


static uint32_t hfn_MACADDROID(Datum datum, uint32_t hval)
{
    macaddr* p = DatumGetMacaddrP(datum);
    return fnv1_32_buf(p, sizeof(*p), hval);
}

static uint32_t hfn_BITOID(Datum datum, uint32_t hval)
{
    /*
     * Note that these are essentially strings.
     * we don't need to worry about '10' and '010'
     * to compare, b/c they will not, by design.
     * (see SQL standard, and varbit.c)
     */
    VarBit* p  = DatumGetVarBitP(datum);
    int len = VARBITBYTES(p);
    return fnv1_32_buf(p, len, hval);
}    

static hfn_t* hfn_VARBITOID = hfn_BITOID;

static uint32_t hfn_BOOLOID(Datum datum, uint32_t hval)
{
    bool b = DatumGetBool(datum);
    return fnv1_32_buf(&b, sizeof(b), hval);
}

static uint32_t hfn_ACLITEMOID(Datum datum, uint32_t hval)
{
    AclItem* p = DatumGetAclItemP(datum);
    uint32 buf = (uint32) (p->ai_privs + p->ai_grantee + p->ai_grantor);
    return fnv1_32_buf(&buf, sizeof(buf), hval);
}

static uint32_t hfn_ANYARRAYOID(Datum datum, uint32_t hval)
{
    ArrayType* p = DatumGetArrayTypeP(datum);
    char* buf = VARDATA(p);
    int len = VARSIZE(p) - VARHDRSZ;
    return fnv1_32_buf(buf, len, hval);
}

static uint32_t hfn_INT2VECTOROID(Datum datum, uint32_t hval)
{
    int2vector* p = (int2vector*) DatumGetPointer(datum);
    int len = p->dim1 * sizeof(int2);
    return fnv1_32_buf(p->values, len, hval);
}

static uint32_t hfn_OIDVECTOROID(Datum datum, uint32_t hval)
{
    oidvector* p = (oidvector*) DatumGetPointer(datum);
    int len = p->dim1 * sizeof(Oid);
    return fnv1_32_buf(p->values, len, hval);
}

static uint32_t hfn_CASHOID(Datum datum, uint32_t hval)
{
    Cash buf = * (Cash*) DatumGetPointer(datum);
    return fnv1_32_buf(&buf, sizeof(buf), hval);
}



typedef struct hfn_item_t hfn_item_t;
struct hfn_item_t {
    Oid oid;
    hfn_t* fn;
};
        
static hfn_t*** fntab = 0;
static hfn_item_t* xfntab = 0;
static int xfntabsz = 0;

static void OOM()
{
    ereport(FATAL,                           
            (errcode(ERRCODE_OUT_OF_MEMORY),
             errmsg("insufficient shared memory for datum hashfn")));
}



void exx_reset_datum_hashfn(void)
{
    if (xfntab) {
        free(xfntab);
        xfntab = 0;
        xfntabsz = 0;
    }
}


uint32_t exx_datum_hashnull(uint32_t hval)
{
    uint32_t val = NULL_VAL;
    return fnv1_32_buf(&val, sizeof(val), hval);
}


exx_datum_hashfn_t* exx_get_datum_hashfn(Oid type)
{
    if (! fntab) {
        /* these are static. once allocated and initialized, leave it until 
         * the program dies. i.e. we will never free(fntab).
         */
        fntab = (hfn_t***) calloc(20, sizeof(hfn_t**));
        if (!fntab) { OOM(); }
        
#define SET(x) { \
            int a = x / 128; \
            int b = x % 128; \
            Assert(a < 20); \
            if (!fntab[a]) { \
                fntab[a] = (hfn_t**) calloc(128, sizeof(hfn_t*)); \
                if (!fntab[a]) { OOM(); } \
            } \
            fntab[a][b] = hfn_ ## x; \
        }

        SET(INT2OID);
        SET(INT4OID);
        SET(INT8OID);
        SET(FLOAT4OID);
        SET(FLOAT8OID);
        SET(NUMERICOID);
        SET(CHAROID);
        SET(BPCHAROID);
        SET(TEXTOID);
        SET(VARCHAROID);
        SET(BYTEAOID);
        SET(NAMEOID);
        SET(OIDOID);
        SET(REGPROCOID);
        SET(REGPROCEDUREOID);
        SET(REGOPEROID);
        SET(REGOPERATOROID);
        SET(REGCLASSOID);
        SET(REGTYPEOID);
        SET(TIDOID);
        SET(TIMESTAMPOID);
        SET(TIMESTAMPTZOID);
        SET(DATEOID);
        SET(TIMEOID);
        SET(TIMETZOID);
        SET(INTERVALOID);
        SET(ABSTIMEOID);
        SET(RELTIMEOID);
        SET(TINTERVALOID);
        SET(INETOID);
        SET(CIDROID);
        SET(MACADDROID);
        SET(BITOID);
        SET(VARBITOID);
        SET(BOOLOID);
        SET(ACLITEMOID);
        SET(ANYARRAYOID);
        SET(INT2VECTOROID);
        SET(OIDVECTOROID);
        SET(CASHOID);
    }

    int a = type / 128;
    int b = type % 128;
    if (0 <= a && a < 20 && fntab[a]) {
        return fntab[a][b];
    }

    return 0;
}


    
