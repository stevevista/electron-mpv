#pragma once
#include "tiny-network.h"

#ifdef __cplusplus
extern "C" {
#endif

int ai_render_detection_to_ass_lines(
  int resX,
  int resY,
  int locale,
  const ai_detection_t* det,
  char *lines[], int max_count);

#ifdef __cplusplus
}
#endif
