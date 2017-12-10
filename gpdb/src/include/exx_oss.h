#ifndef EXX_OSS_H
#define EXX_OSS_H

#include <stdint.h>
typedef uint32_t exx_datum_hashfn_t(Datum datum, uint32_t hval);
extern uint32_t exx_datum_hashnull(Oid);
extern void exx_reset_datum_hasnfn(void);
extern exx_datum_hashfn_t* exx_get_datum_hashfn(Oid);
extern void exx_reset_datum_hashfn(void);

#endif
