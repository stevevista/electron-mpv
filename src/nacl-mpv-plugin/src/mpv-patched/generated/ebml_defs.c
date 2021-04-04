// hack around demux/ebml.c
#undef EVALARGS
#undef E_SN
#undef E_S
#undef F
#define EVALARGS(F, ...) MP_EXPAND_ARGS(F(__VA_ARGS__))
#define E_SN(str, count, N) const struct ebml_elem_desc ebml_ ## N ## _desc = { str, EBML_TYPE_SUBELEMENTS, sizeof(struct ebml_ ## N), count, (const struct ebml_field_desc[]){
#define E_S(str, count) EVALARGS(E_SN, str, count, N)
#define F(id, name, multiple) EVALARGS(FN, id, name, multiple, N)
#include "ebml_defs_REAL.c"
