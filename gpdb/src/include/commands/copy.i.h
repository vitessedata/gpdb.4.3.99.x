#ifndef _COPY_I_H_
#define _COPY_I_H_

/*
 * exx version of copy header.
 */

typedef void* ExxCopyState;

enum { 
    EXX_COPY_CSV_MODE,
    EXX_COPY_TEXT_MODE,
};

typedef struct exx_copy_options_t {
    int         mode;
    bool        *convert_select_flags; 
    bool        *force_notnull_flags;
    bool        *force_null_flags;
    char        *null_print;
    TupleDesc   tupDesc;

    bool        header_line;
    char        delim;
    char        quote;
    char        escape;
    char        relname[128];
	
	bool		exx_intts;
	char		exx_comment;

    void*       scan_desc; 
} exx_copy_options_t;

#if 0
#ifdef __cplusplus
extern "C" bool ExxCopyCanHandle(void *cstate, exx_copy_options_t *opts); 
extern "C" void ExxCopyAddColumn(void *exx_cstate, int attnum);
extern "C" void ExxCopyAddColumnsFromState(void *cstate, void *exx_cstate);
extern "C" ExxCopyState ExxBeginCopyFrom(void *cstate); 
extern "C" void ExxEndCopyFrom(void *exx_cstate);
extern "C" bool ExxNextCopyFrom(void *exx_cstate, uintptr_t *values, bool *nulls);
extern "C" void ExxCopyMarkReq(void *cstate, int ncol);
extern "C" void ExxCopyMarkReqImpl(void *exx_cstate, int ncol);
extern "C" int ExxCopyLoadBuf(void* exx_cstate, void *buf, int bufsz);
extern "C" int ExxCopyErrorContextGetLineInfo(void *cstate, char **pbuf);
extern "C" int ExxCopyGetLineInfo(void *exx_cstate, char **pbuf);
#else 
extern bool ExxCopyCanHandle(void *cstate, exx_copy_options_t *opts);
extern void ExxCopyAddColumn(void *exx_cstate, int attnum);
extern void ExxCopyAddColumnsFromState(void *cstate, void *exx_cstate);
extern ExxCopyState ExxBeginCopyFrom(void *cstate); 
extern void ExxEndCopyFrom(void *exx_cstate);
extern bool ExxNextCopyFrom(void *exx_cstate, uintptr_t *values, bool *nulls);
extern void ExxCopyMarkReq(void *cstate, int ncol);
extern void ExxCopyMarkReqImpl(void *exx_cstate, int ncol);
extern int ExxCopyLoadBuf(void* cstate, void *buf, int bufsz);
extern int ExxCopyErrorContextGetLineInfo(void *cstate, char **pbuf);
extern int ExxCopyGetLineInfo(void *p, char **pbuf);
#endif        /* __cplusplus */
#endif

#endif        /* _COPY_I_H_ */

