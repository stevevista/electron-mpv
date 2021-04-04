#pragma once
#include "../mpv/mpv_talloc.h"

#undef ta_xnew_array_size
#define ta_xnew_array_size(...)         ta_oom_p(MP_EXPAND_ARGS(ta_new_array_size(__VA_ARGS__)))
