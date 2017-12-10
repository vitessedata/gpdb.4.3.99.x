#include "postgres.h"
#include "fmgr.h"
#include "funcapi.h"
#include "json.h"

PG_MODULE_MAGIC;

#define JSON_FUNC(fn)                               \
extern Datum exx_ ## fn(PG_FUNCTION_ARGS);          \
PG_FUNCTION_INFO_V1(exx_ ## fn);                    \
Datum exx_ ## fn(PG_FUNCTION_ARGS) { return fn(fcinfo); }  \
\

GEN_JSON_FUNC_DEF
#undef JSON_FUNC
