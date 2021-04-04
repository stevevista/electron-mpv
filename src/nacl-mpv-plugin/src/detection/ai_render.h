#pragma once
#include "tiny-network.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct ai_renderer_priv *ai_renderer;

ai_renderer ai_renderer_init();
void ai_renderer_free(ai_renderer);

void ai_renderer_set_res(ai_renderer ctx, int w, int h);

// 1 for zh-CN
void ai_renderer_set_locale(ai_renderer ctx, int locale);

struct ass_image *ai_renderer_render_ass_image(
    ai_renderer ctx,
    int width,
    int height,
    int ml, int mt, int mr, int mb,
    long long now, int *detect_change);


void ai_renderer_flush_events(ai_renderer);

int ai_render_update_events_duration(
  ai_renderer ctx,
  long long pts);

int ai_render_generate_events(
    ai_renderer ctx,
    ai_detection_t* det,
    long long start, long long duration);


#ifdef __cplusplus
}
#endif
