#ifndef EXX_CONTRIB_JSON_H
#define EXX_CONTRIB_JSON_H

#define GEN_JSON_FUNC_DEF \
JSON_FUNC(json_in)      \
JSON_FUNC(json_out)     \
JSON_FUNC(json_recv)    \
JSON_FUNC(json_send)    \
JSON_FUNC(array_to_json)            \
JSON_FUNC(array_to_json_pretty)     \
JSON_FUNC(row_to_json)              \
JSON_FUNC(row_to_json_pretty)       \
JSON_FUNC(to_json)                  \
JSON_FUNC(escape_json)              \
JSON_FUNC(json_object_field)        \
JSON_FUNC(json_object_field_text)   \
JSON_FUNC(json_array_element)       \
JSON_FUNC(json_array_element_text)  \
JSON_FUNC(json_extract_path)        \
JSON_FUNC(json_extract_path_text)   \
JSON_FUNC(json_array_length)        \
JSON_FUNC(json_parse_valid)         \
JSON_FUNC(json_is_valid)            \



#define JSON_FUNC(fn)   extern Datum fn(PG_FUNCTION_ARGS); 
GEN_JSON_FUNC_DEF
#undef JSON_FUNC

#endif
