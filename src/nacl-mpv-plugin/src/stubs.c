#include <stdint.h>
#include <stdbool.h>
#include "config.h"
#include "libmpv/render.h"
#include "video/out/vo.h"
#include "player/client.h"
#include "common/msg.h"
#include "video/out/gpu/context.h"
#include "video/out/gpu/spirv.h"
#include "misc/json.h"

#ifdef BUILD_PEPPER
static int preinit(struct vo *vo)
{
  return -1;
}

const struct vo_driver video_out_gpu = {
  .description = "dummy render",
  .name = "dummy",
  .caps = VO_CAP_ROTATE90,
  .preinit = preinit,
};
#endif

// to make spirv happy
#if HAVE_SHADERC
static bool fake_shaderc_compile(struct spirv_compiler *spirv, void *tactx,
                            enum glsl_shader type, const char *glsl,
                            struct bstr *out_spirv) {
    return false;
}

static bool fake_shaderc_init(struct ra_ctx *ctx) {
    return false;
}

static void fake_shaderc_uninit(struct ra_ctx *ctx) {
}

const struct spirv_compiler_fns spirv_shaderc = {
    .compile_glsl = fake_shaderc_compile,
    .init = fake_shaderc_init,
    .uninit = fake_shaderc_uninit,
};
#endif


int mpv_command_json_async(mpv_handle *ctx, uint64_t ud, const char *jsonstr) {
    void *tmp = talloc_new(NULL);

    char *line0 = jsonstr;
    json_skip_whitespace(&line0);

    if (line0[0] == '{' || line0[0] == '[') {
        mpv_node msg_node;
        int rc = json_parse(tmp, &msg_node, &line0, 50);
        if (rc < 0) {
            // mp_err(log, "malformed JSON received: '%s'\n", src);
            goto error;
        }
        rc = mpv_command_node_async(ctx, ud, &msg_node);
        if (rc < 0) {
            goto error;
        }
    } else {
        goto error;
    }

    talloc_free(tmp);
    return 0;

error:
    talloc_free(tmp);
    return -1;
}
