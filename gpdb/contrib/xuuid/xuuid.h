#ifndef EXX_CONTRIB_UUID_H
#define EXX_CONTRIB_UUID_H

#define GEN_UUID_FUNC_DEF \
UUID_FUNC(uuid_in)      \
UUID_FUNC(uuid_out)     \
UUID_FUNC(uuid_recv)    \
UUID_FUNC(uuid_send)    \
UUID_FUNC(uuid_eq)    \
UUID_FUNC(uuid_ne)    \
UUID_FUNC(uuid_lt)    \
UUID_FUNC(uuid_le)    \
UUID_FUNC(uuid_gt)    \
UUID_FUNC(uuid_ge)    \
UUID_FUNC(uuid_hash)    \
UUID_FUNC(uuid_cmp)    \
UUID_FUNC(uuid_gen)    \
UUID_FUNC(uuid_text)    \
UUID_FUNC(text_uuid)    \
UUID_FUNC(uuid_varchar)    \
UUID_FUNC(varchar_uuid)    \


#define UUID_FUNC(fn)   extern Datum fn(PG_FUNCTION_ARGS); 
GEN_UUID_FUNC_DEF
#undef UUID_FUNC

#endif
