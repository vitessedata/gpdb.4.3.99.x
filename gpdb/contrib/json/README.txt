Implementation of json.

The code is largely taken from Postgres 9.3.  Instead of a native
implementation, we used a contrib module.  Main reason is that we
do not want to touch catalog at this moment, so that we can claim
binary-compatible-without-loading-data.

The following functions are not implemented.
* SRF (set returning functions).
* populate_record
* json_agg

PG 9.3 introduced VARIADIC arguments.  VARIADIC text[] is implemented
as simple text[].

