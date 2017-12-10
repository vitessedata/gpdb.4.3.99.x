/*-------------------------------------------------------------------------
 *
 * jsonfuncs.c
 *		Functions to process JSON data type.
 *
 * Portions Copyright (c) 1996-2013, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/backend/utils/adt/jsonfuncs.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include <limits.h>

#include "fmgr.h"
#include "funcapi.h"
#include "miscadmin.h"
#include "access/htup.h" 
#include "catalog/pg_type.h"
#include "lib/stringinfo.h"
#include "mb/pg_wchar.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/hsearch.h"
#include "utils/json.h"
#include "utils/jsonapi.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/typcache.h"

/* semantic action functions for json_get* functions */
static void get_object_start(void *state);
static void get_object_field_start(void *state, char *fname, bool isnull);
static void get_object_field_end(void *state, char *fname, bool isnull);

static void get_array_start(void *state);
static void get_array_element_start(void *state, bool isnull);
static void get_array_element_end(void *state, bool isnull);
static void get_scalar(void *state, char *token, JsonTokenType tokentype);

/* common worker function for json getter functions */
static inline Datum get_path_all(PG_FUNCTION_ARGS, bool as_text);
static inline text *get_worker(text *json, char *field, int elem_index,
		   char **tpath, int *ipath, int npath,
		   bool normalize_results);

/* semantic action functions for json_array_length */
static void alen_object_start(void *state);
static void alen_scalar(void *state, char *token, JsonTokenType tokentype);
static void alen_array_element_start(void *state, bool isnull);

/* semantic action functions for get_json_object_as_hash */
static void hash_array_start(void *state);
static void hash_scalar(void *state, char *token, JsonTokenType tokentype);

/* search type classification for json_get* functions */
typedef enum
{
	JSON_SEARCH_OBJECT = 1,
	JSON_SEARCH_ARRAY,
	JSON_SEARCH_PATH
} JsonSearch;

/* state for json_get* functions */
typedef struct GetState
{
	JsonLexContext *lex;
	JsonSearch	search_type;
	int			search_index;
	int			array_index;
	char	   *search_term;
	char	   *result_start;
	text	   *tresult;
	bool		result_is_null;
	bool		normalize_results;
	bool		next_scalar;
	char	  **path;
	int			npath;
	char	  **current_path;
	bool	   *pathok;
	int		   *array_level_index;
	int		   *path_level_index;
}	GetState;

/* state for json_array_length */
typedef struct AlenState
{
	JsonLexContext *lex;
	int			count;
}	AlenState;

/* state for get_json_object_as_hash */
typedef struct JhashState
{
	JsonLexContext *lex;
	HTAB	   *hash;
	char	   *saved_scalar;
	char	   *save_json_start;
	bool		use_json_as_text;
	char	   *function_name;
}	JHashState;

/* used to build the hashtable */
typedef struct JsonHashEntry
{
	char		fname[NAMEDATALEN];
	char	   *val;
	char	   *json;
	bool		isnull;
}	JsonHashEntry;

/* these two are stolen from hstore / record_out, used in populate_record* */
typedef struct ColumnIOData
{
	Oid			column_type;
	Oid			typiofunc;
	Oid			typioparam;
	FmgrInfo	proc;
} ColumnIOData;

typedef struct RecordIOData
{
	Oid			record_type;
	int32		record_typmod;
	int			ncolumns;
	ColumnIOData columns[1];	/* VARIABLE LENGTH ARRAY */
} RecordIOData;

/* state for populate_recordset */
typedef struct PopulateRecordsetState
{
	JsonLexContext *lex;
	HTAB	   *json_hash;
	char	   *saved_scalar;
	char	   *save_json_start;
	bool		use_json_as_text;
	Tuplestorestate *tuple_store;
	TupleDesc	ret_tdesc;
	HeapTupleHeader rec;
	RecordIOData *my_extra;
	MemoryContext fn_mcxt;		/* used to stash IO funcs */
}	PopulateRecordsetState;


/*
 * json getter functions
 * these implement the -> ->> #> and #>> operators
 * and the json_extract_path*(json, text, ...) functions
 */


Datum
json_object_field(PG_FUNCTION_ARGS)
{
	text	   *json = PG_GETARG_TEXT_P(0);
	text	   *result;
	text	   *fname = PG_GETARG_TEXT_P(1);
	char	   *fnamestr = text_to_cstring(fname);

	result = get_worker(json, fnamestr, -1, NULL, NULL, -1, false);

	if (result != NULL)
		PG_RETURN_TEXT_P(result);
	else
		PG_RETURN_NULL();
}

Datum
json_object_field_text(PG_FUNCTION_ARGS)
{
	text	   *json = PG_GETARG_TEXT_P(0);
	text	   *result;
	text	   *fname = PG_GETARG_TEXT_P(1);
	char	   *fnamestr = text_to_cstring(fname);

	result = get_worker(json, fnamestr, -1, NULL, NULL, -1, true);

	if (result != NULL)
		PG_RETURN_TEXT_P(result);
	else
		PG_RETURN_NULL();
}

Datum
json_array_element(PG_FUNCTION_ARGS)
{
	text	   *json = PG_GETARG_TEXT_P(0);
	text	   *result;
	int			element = PG_GETARG_INT32(1);

	result = get_worker(json, NULL, element, NULL, NULL, -1, false);

	if (result != NULL)
		PG_RETURN_TEXT_P(result);
	else
		PG_RETURN_NULL();
}

Datum
json_array_element_text(PG_FUNCTION_ARGS)
{
	text	   *json = PG_GETARG_TEXT_P(0);
	text	   *result;
	int			element = PG_GETARG_INT32(1);

	result = get_worker(json, NULL, element, NULL, NULL, -1, true);

	if (result != NULL)
		PG_RETURN_TEXT_P(result);
	else
		PG_RETURN_NULL();
}

Datum
json_extract_path(PG_FUNCTION_ARGS)
{
	return get_path_all(fcinfo, false);
}

Datum
json_extract_path_text(PG_FUNCTION_ARGS)
{
	return get_path_all(fcinfo, true);
}

/*
 * common routine for extract_path functions
 */
static inline Datum
get_path_all(PG_FUNCTION_ARGS, bool as_text)
{
	text	   *json = PG_GETARG_TEXT_P(0);
	ArrayType  *path = PG_GETARG_ARRAYTYPE_P(1);
	text	   *result;
	Datum	   *pathtext;
	bool	   *pathnulls;
	int			npath;
	char	  **tpath;
	int		   *ipath;
	int			i;
	long		ind;
	char	   *endptr;

	if (array_contains_nulls(path))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("cannot call function with null path elements")));


	deconstruct_array(path, TEXTOID, -1, false, 'i',
					  &pathtext, &pathnulls, &npath);

	tpath = palloc(npath * sizeof(char *));
	ipath = palloc(npath * sizeof(int));


	for (i = 0; i < npath; i++)
	{
		tpath[i] = TextDatumGetCString(pathtext[i]);
		if (*tpath[i] == '\0')
			ereport(
					ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				   errmsg("cannot call function with empty path elements")));

		/*
		 * we have no idea at this stage what structure the document is so
		 * just convert anything in the path that we can to an integer and set
		 * all the other integers to -1 which will never match.
		 */
		ind = strtol(tpath[i], &endptr, 10);
		if (*endptr == '\0' && ind <= INT_MAX && ind >= 0)
			ipath[i] = (int) ind;
		else
			ipath[i] = -1;
	}


	result = get_worker(json, NULL, -1, tpath, ipath, npath, as_text);

	if (result != NULL)
		PG_RETURN_TEXT_P(result);
	else
		PG_RETURN_NULL();
}

/*
 * get_worker
 *
 * common worker for all the json getter functions
 */
static inline text *
get_worker(text *json,
		   char *field,
		   int elem_index,
		   char **tpath,
		   int *ipath,
		   int npath,
		   bool normalize_results)
{
	GetState   *state;
	JsonLexContext *lex = makeJsonLexContext(json, true);
	JsonSemAction *sem;

	/* only allowed to use one of these */
	Assert(elem_index < 0 || (tpath == NULL && ipath == NULL && field == NULL));
	Assert(tpath == NULL || field == NULL);

	state = palloc0(sizeof(GetState));
	sem = palloc0(sizeof(JsonSemAction));

	state->lex = lex;
	/* is it "_as_text" variant? */
	state->normalize_results = normalize_results;
	if (field != NULL)
	{
		/* single text argument */
		state->search_type = JSON_SEARCH_OBJECT;
		state->search_term = field;
	}
	else if (tpath != NULL)
	{
		/* path array argument */
		state->search_type = JSON_SEARCH_PATH;
		state->path = tpath;
		state->npath = npath;
		state->current_path = palloc(sizeof(char *) * npath);
		state->pathok = palloc0(sizeof(bool) * npath);
		state->pathok[0] = true;
		state->array_level_index = palloc(sizeof(int) * npath);
		state->path_level_index = ipath;

	}
	else
	{
		/* single integer argument */
		state->search_type = JSON_SEARCH_ARRAY;
		state->search_index = elem_index;
		state->array_index = -1;
	}

	sem->semstate = (void *) state;

	/*
	 * Not all	variants need all the semantic routines. only set the ones
	 * that are actually needed for maximum efficiency.
	 */
	sem->object_start = get_object_start;
	sem->array_start = get_array_start;
	sem->scalar = get_scalar;
	if (field != NULL || tpath != NULL)
	{
		sem->object_field_start = get_object_field_start;
		sem->object_field_end = get_object_field_end;
	}
	if (field == NULL)
	{
		sem->array_element_start = get_array_element_start;
		sem->array_element_end = get_array_element_end;
	}

	pg_parse_json(lex, sem);

	return state->tresult;
}

static void
get_object_start(void *state)
{
	GetState   *_state = (GetState *) state;

	/* json structure check */
	if (_state->lex->lex_level == 0 && _state->search_type == JSON_SEARCH_ARRAY)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("cannot extract array element from a non-array")));
}

static void
get_object_field_start(void *state, char *fname, bool isnull)
{
	GetState   *_state = (GetState *) state;
	bool		get_next = false;
	int			lex_level = _state->lex->lex_level;

	if (lex_level == 1 && _state->search_type == JSON_SEARCH_OBJECT &&
		strcmp(fname, _state->search_term) == 0)
	{

		_state->tresult = NULL;
		_state->result_start = NULL;
		get_next = true;
	}
	else if (_state->search_type == JSON_SEARCH_PATH &&
			 lex_level <= _state->npath &&
			 _state->pathok[_state->lex->lex_level - 1] &&
			 strcmp(fname, _state->path[lex_level - 1]) == 0)
	{
		/* path search, path so far is ok,	and we have a match */

		/* this object overrides any previous matching object */

		_state->tresult = NULL;
		_state->result_start = NULL;

		/* if not at end of path just mark path ok */
		if (lex_level < _state->npath)
			_state->pathok[lex_level] = true;

		/* end of path, so we want this value */
		if (lex_level == _state->npath)
			get_next = true;
	}

	if (get_next)
	{
		if (_state->normalize_results &&
			_state->lex->token_type == JSON_TOKEN_STRING)
		{
			/* for as_text variants, tell get_scalar to set it for us */
			_state->next_scalar = true;
		}
		else
		{
			/* for non-as_text variants, just note the json starting point */
			_state->result_start = _state->lex->token_start;
		}
	}
}

static void
get_object_field_end(void *state, char *fname, bool isnull)
{
	GetState   *_state = (GetState *) state;
	bool		get_last = false;
	int			lex_level = _state->lex->lex_level;


	/* same tests as in get_object_field_start, mutatis mutandis */
	if (lex_level == 1 && _state->search_type == JSON_SEARCH_OBJECT &&
		strcmp(fname, _state->search_term) == 0)
	{
		get_last = true;
	}
	else if (_state->search_type == JSON_SEARCH_PATH &&
			 lex_level <= _state->npath &&
			 _state->pathok[lex_level - 1] &&
			 strcmp(fname, _state->path[lex_level - 1]) == 0)
	{
		/* done with this field so reset pathok */
		if (lex_level < _state->npath)
			_state->pathok[lex_level] = false;

		if (lex_level == _state->npath)
			get_last = true;
	}

	/* for as_test variants our work is already done */
	if (get_last && _state->result_start != NULL)
	{
		/*
		 * make a text object from the string from the prevously noted json
		 * start up to the end of the previous token (the lexer is by now
		 * ahead of us on whatevere came after what we're interested in).
		 */
		int			len = _state->lex->prev_token_terminator - _state->result_start;

		if (isnull && _state->normalize_results)
			_state->tresult = (text *) NULL;
		else
			_state->tresult = cstring_to_text_with_len(_state->result_start, len);
	}

	/*
	 * don't need to reset _state->result_start b/c we're only returning one
	 * datum, the conditions should not occur more than once, and this lets us
	 * check cheaply that they don't (see object_field_start() )
	 */
}

static void
get_array_start(void *state)
{
	GetState   *_state = (GetState *) state;
	int			lex_level = _state->lex->lex_level;

	/* json structure check */
	if (lex_level == 0 && _state->search_type == JSON_SEARCH_OBJECT)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("cannot extract field from a non-object")));

	/*
	 * initialize array count for this nesting level Note: the lex_level seen
	 * by array_start is one less than that seen by the elements of the array.
	 */
	if (_state->search_type == JSON_SEARCH_PATH &&
		lex_level < _state->npath)
		_state->array_level_index[lex_level] = -1;
}

static void
get_array_element_start(void *state, bool isnull)
{
	GetState   *_state = (GetState *) state;
	bool		get_next = false;
	int			lex_level = _state->lex->lex_level;

	if (lex_level == 1 && _state->search_type == JSON_SEARCH_ARRAY)
	{
		/* single integer search */
		_state->array_index++;
		if (_state->array_index == _state->search_index)
			get_next = true;
	}
	else if (_state->search_type == JSON_SEARCH_PATH &&
			 lex_level <= _state->npath &&
			 _state->pathok[lex_level - 1])
	{
		/*
		 * path search, path so far is ok
		 *
		 * increment the array counter. no point doing this if we already know
		 * the path is bad.
		 *
		 * then check if we have a match.
		 */

		if (++_state->array_level_index[lex_level - 1] ==
			_state->path_level_index[lex_level - 1])
		{
			if (lex_level == _state->npath)
			{
				/* match and at end of path, so get value */
				get_next = true;
			}
			else
			{
				/* not at end of path just mark path ok */
				_state->pathok[lex_level] = true;
			}
		}

	}

	/* same logic as for objects */
	if (get_next)
	{
		if (_state->normalize_results &&
			_state->lex->token_type == JSON_TOKEN_STRING)
		{
			_state->next_scalar = true;
		}
		else
		{
			_state->result_start = _state->lex->token_start;
		}
	}
}

static void
get_array_element_end(void *state, bool isnull)
{
	GetState   *_state = (GetState *) state;
	bool		get_last = false;
	int			lex_level = _state->lex->lex_level;

	/* same logic as in get_object_end, modified for arrays */

	if (lex_level == 1 && _state->search_type == JSON_SEARCH_ARRAY &&
		_state->array_index == _state->search_index)
	{
		get_last = true;
	}
	else if (_state->search_type == JSON_SEARCH_PATH &&
			 lex_level <= _state->npath &&
			 _state->pathok[lex_level - 1] &&
			 _state->array_level_index[lex_level - 1] ==
			 _state->path_level_index[lex_level - 1])
	{
		/* done with this element so reset pathok */
		if (lex_level < _state->npath)
			_state->pathok[lex_level] = false;

		if (lex_level == _state->npath)
			get_last = true;
	}
	if (get_last && _state->result_start != NULL)
	{
		int			len = _state->lex->prev_token_terminator - _state->result_start;

		if (isnull && _state->normalize_results)
			_state->tresult = (text *) NULL;
		else
			_state->tresult = cstring_to_text_with_len(_state->result_start, len);
	}
}

static void
get_scalar(void *state, char *token, JsonTokenType tokentype)
{
	GetState   *_state = (GetState *) state;

	if (_state->lex->lex_level == 0 && _state->search_type != JSON_SEARCH_PATH)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("cannot extract element from a scalar")));
	if (_state->next_scalar)
	{
		/* a de-escaped text value is wanted, so supply it */
		_state->tresult = cstring_to_text(token);
		/* make sure the next call to get_scalar doesn't overwrite it */
		_state->next_scalar = false;
	}

}

/*
 * SQL function json_array_length(json) -> int
 */
Datum
json_array_length(PG_FUNCTION_ARGS)
{
	text	   *json = PG_GETARG_TEXT_P(0);

	AlenState  *state;
	JsonLexContext *lex = makeJsonLexContext(json, false);
	JsonSemAction *sem;

	state = palloc0(sizeof(AlenState));
	sem = palloc0(sizeof(JsonSemAction));

	/* palloc0 does this for us */
#if 0
	state->count = 0;
#endif
	state->lex = lex;

	sem->semstate = (void *) state;
	sem->object_start = alen_object_start;
	sem->scalar = alen_scalar;
	sem->array_element_start = alen_array_element_start;

	pg_parse_json(lex, sem);

	PG_RETURN_INT32(state->count);
}

/*
 * These next two check ensure that the json is an array (since it can't be
 * a scalar or an object).
 */

static void
alen_object_start(void *state)
{
	AlenState  *_state = (AlenState *) state;

	/* json structure check */
	if (_state->lex->lex_level == 0)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("cannot get array length of a non-array")));
}

static void
alen_scalar(void *state, char *token, JsonTokenType tokentype)
{
	AlenState  *_state = (AlenState *) state;

	/* json structure check */
	if (_state->lex->lex_level == 0)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("cannot get array length of a scalar")));
}

static void
alen_array_element_start(void *state, bool isnull)
{
	AlenState  *_state = (AlenState *) state;

	/* just count up all the level 1 elements */
	if (_state->lex->lex_level == 1)
		_state->count++;
}

static void
hash_array_start(void *state)
{
	JHashState *_state = (JHashState *) state;

	if (_state->lex->lex_level == 0)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			   errmsg("cannot call %s on an array", _state->function_name)));
}

static void
hash_scalar(void *state, char *token, JsonTokenType tokentype)
{
	JHashState *_state = (JHashState *) state;

	if (_state->lex->lex_level == 0)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			   errmsg("cannot call %s on a scalar", _state->function_name)));

	if (_state->lex->lex_level == 1)
		_state->saved_scalar = token;
}
