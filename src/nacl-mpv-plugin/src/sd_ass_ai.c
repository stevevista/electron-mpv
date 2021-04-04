#include "libavutil/intreadwrite.h"

#include "config.h"
#include "options/m_config.h"
#include "options/options.h"
#include "common/common.h"
#include "common/msg.h"
#include "demux/demux.h"
#include "video/csputils.h"
#include "video/mp_image.h"
#include "sub/dec_sub.h"
#include "sub/sd.h"
#include "sub/ass_mp.h"
#include "sub/img_convert.h"
#include "video/out/bitmap_packer.h"
#include <ass/ass.h>
#include <libavcodec/avcodec.h>

#include "detection/ai_render.h"

extern const struct sd_functions orgin_sd_ass;

struct sd_detection_priv {
  ai_renderer renderer;
  struct mp_ass_packer *packer;
  struct sub_bitmap_copy_cache *copy_cache;
};

static void ai_enable_output(struct sd *sd, bool enable)
{
}

static int ai_init(struct sd *sd) {
  if (strcmp(sd->codec->codec, "eia_608") != 0) {
    int ret = orgin_sd_ass.init(sd);
    if (ret == 0) {
      // replace sd driver
      sd->driver = &orgin_sd_ass;
    }
    return ret;
  }

  struct mp_subtitle_opts *opts = sd->opts;
  struct sd_detection_priv *ctx = talloc_zero(sd, struct sd_detection_priv);
  sd->priv = ctx;

  ctx->renderer = ai_renderer_init();
  ctx->packer = mp_ass_packer_alloc(ctx);
  return 0;
}

static void ai_uninit(struct sd *sd) {
  struct sd_detection_priv *ctx = sd->priv;
  ai_renderer_free(ctx->renderer);
  talloc_free(ctx->copy_cache);
  talloc_free(ctx);
}

static void render_ai_detection(struct sd *sd, ai_detection_t *t, double pts, double duration) {
  struct sd_detection_priv *ctx = sd->priv;
  ai_render_generate_events(ctx->renderer, t,
          llrint(pts*1000), llrint(duration*1000) - 1);
}

static void ai_decode(struct sd *sd, struct demux_packet *packet) {
  struct sd_detection_priv *ctx = sd->priv;

  const uint8_t *data = packet->buffer;
  int size = packet->len;

#ifdef MPV_CUSTOMIZE_USE_HOT_DETECTION
  if (size >= 4 && !memcmp(data, "HOTD", 4)) {
    ai_render_update_events_duration(
      ctx->renderer,
      llrint(packet->pts*1000));

    ai_detection_t *detections = data + 4;
    int count = (size - 4) / sizeof(ai_detection_t);
    for (int i = 0; i < count; i++) {
      render_ai_detection(sd, &detections[i], packet->pts, 1);
    }
    return;
  }
#endif
}

static struct sub_bitmaps *ai_get_bitmaps(struct sd *sd, struct mp_osd_res dim,
                                       int format, double pts) {
  struct sd_detection_priv *ctx = sd->priv;

  struct mp_subtitle_opts *opts = sd->opts;
  const char* ai_options = opts->ass_styles_file;
  struct sub_bitmaps *res = &(struct sub_bitmaps){0};

  if (pts == MP_NOPTS_VALUE)
    goto done;

  if (ai_options && (strstr(ai_options, "|zh-cn") || strstr(ai_options, "|zh-CN"))) {
    ai_renderer_set_locale(ctx->renderer, 1);
  } else {
    ai_renderer_set_locale(ctx->renderer, 0);
  }

  long long ts = llrint(pts*1000);

  int changed;
  ASS_Image *imgs = ai_renderer_render_ass_image(
                      ctx->renderer,
                      dim.w, dim.h,
                      dim.ml, dim.mt, dim.mr, dim.mb,
                      ts, &changed);
  mp_ass_packer_pack(ctx->packer, &imgs, 1, changed, format, res);

done:
  res = sub_bitmaps_copy(&ctx->copy_cache, res);

  return res;
}

static char *ai_get_text(struct sd *sd, double pts, enum sd_text_type type)
{
  return NULL;
}

static struct sd_times ai_get_times(struct sd *sd, double pts)
{
  struct sd_detection_priv *ctx = sd->priv;
  struct sd_times res = { .start = MP_NOPTS_VALUE, .end = MP_NOPTS_VALUE };
  return res;
}

static int ai_control(struct sd *sd, enum sd_ctrl cmd, void *arg)
{
  return -1;
}

static void ai_reset(struct sd *sd)
{
}

const struct sd_functions sd_ass = {
    .name = "ass",
    .accept_packets_in_advance = true,
    .init = ai_init,
    .decode = ai_decode,
    .get_bitmaps = ai_get_bitmaps,
    .get_text = ai_get_text,
    .get_times = ai_get_times,
    .select = ai_enable_output,
    .reset = ai_reset,
    .control = ai_control,
    .uninit = ai_uninit,
};
