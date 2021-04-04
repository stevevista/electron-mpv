#include <ass/ass.h>
#include "ai_render.h"
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

// i18n
// zh-cn
// age:  "\xe5\xb2\x81",
// male: "\xe7\x94\xb7",
// female "\xe5\xa5\xb3",
// wear：  "\xe6\x88\xb4\xe5\x8f\xa3\xe7\xbd\xa9"
// stranger: \xe9\x99\x8c\xe7\x94\x9f\xe4\xba\xba

// coco
static const char *coco_names_zhcn[] = {
    "\xe4\xba\xba",
  "\xe8\x87\xaa\xe8\xa1\x8c\xe8\xbd\xa6",
  "\xe6\xb1\xbd\xe8\xbd\xa6",
  "\xe6\x91\xa9\xe6\x89\x98\xe8\xbd\xa6",
  "\xe9\xa3\x9e\xe6\x9c\xba",
  "\xe5\x85\xac\xe5\x85\xb1\xe6\xb1\xbd\xe8\xbd\xa6",
  "\xe7\x81\xab\xe8\xbd\xa6",
  "\xe5\x8d\xa1\xe8\xbd\xa6",
  "\xe8\x88\xb9",
  "\xe7\xba\xa2\xe7\xbb\xbf\xe7\x81\xaf",
  "\xe6\xb6\x88\xe9\x98\xb2\xe6\xa0\x93",
  "\xe7\xa6\x81\xe6\xad\xa2\xe9\x80\x9a\xe8\xa1\x8c",
  "\xe8\xae\xa1\xe8\xb4\xb9\xe8\xa1\xa8",
  "\xe9\x95\xbf\xe6\xa4\x85",
  "\xe9\xb8\x9f",
  "\xe7\x8c\xab",
  "\xe7\x8b\x97",
  "\xe9\xa9\xac",
  "\xe7\xbe\x8a",
  "\xe5\xa5\xb6\xe7\x89\x9b",
  "\xe5\xa4\xa7\xe8\xb1\xa1",
  "\xe7\x86\x8a",
  "\xe6\x96\x91\xe9\xa9\xac",
  "\xe9\x95\xbf\xe9\xa2\x88\xe9\xb9\xbf",
  "\xe8\x83\x8c\xe5\x8c\x85",
  "\xe9\x9b\xa8\xe4\xbc\x9e",
  "\xe6\x89\x8b\xe6\x8f\x90\xe5\x8c\x85",
  "\xe9\xa2\x86\xe5\xb8\xa6",
  "\xe6\x89\x8b\xe6\x8f\x90\xe7\xae\xb1",
  "\xe9\xa3\x9e\xe7\x9b\x98",
  "\xe6\xbb\x91\xe9\x9b\xaa\xe6\x9d\xbf",
  "\xe6\xbb\x91\xe9\x9b\xaa\xe6\x9d\xbf",
  "\xe8\xbf\x90\xe5\x8a\xa8\xe7\x90\x83",
  "\xe9\xa3\x8e\xe7\xad\x9d",
  "\xe6\xa3\x92\xe7\x90\x83\xe5\xb8\xbd",
  "\xe6\xa3\x92\xe7\x90\x83\xe6\x89\x8b\xe5\xa5\x97",
  "\xe6\xbb\x91\xe6\x9d\xbf",
  "\xe5\x86\xb2\xe6\xb5\xaa\xe6\x9d\xbf",
  "\xe7\xbd\x91\xe7\x90\x83\xe6\x8b\x8d",
  "\xe7\x93\xb6\xe5\xad\x90",
  "\xe9\x85\x92\xe6\x9d\xaf",
  "\xe6\x9d\xaf\xe5\xad\x90",
  "\xe5\x8f\x89\xe5\xad\x90",
  "\xe5\x88\x80",
  "\xe5\x8b\xba\xe5\xad\x90",
  "\xe7\xa2\x97",
  "\xe9\xa6\x99\xe8\x95\x89",
  "\xe8\x8b\xb9\xe6\x9e\x9c",
  "\xe4\xb8\x89\xe6\x98\x8e\xe6\xb2\xbb",
  "\xe6\xa1\x94\xe5\xad\x90",
  "\xe8\x8a\xb1\xe6\xa4\xb0\xe8\x8f\x9c",
  "\xe8\x83\xa1\xe8\x90\x9d\xe5\x8d\x9c",
  "\xe7\x83\xad\xe7\x8b\x97",
  "\xe6\x8a\xab\xe8\x90\xa8",
  "\xe7\x94\x9c\xe7\x94\x9c\xe5\x9c\x88",
  "\xe8\x9b\x8b\xe7\xb3\x95",
  "\xe6\xa4\x85\xe5\xad\x90",
  "\xe6\xb2\x99\xe5\x8f\x91",
  "\xe7\x9b\x86\xe6\xa0\xbd",
  "\xe5\xba\x8a",
  "\xe9\xa4\x90\xe6\xa1\x8c",
  "\xe9\xa9\xac\xe6\xa1\xb6",
  "\xe6\x98\xbe\xe7\xa4\xba\xe5\x99\xa8",
  "\xe7\xac\x94\xe8\xae\xb0\xe6\x9c\xac",
  "\xe9\xbc\xa0\xe6\xa0\x87",
  "\xe9\x81\xa5\xe6\x8e\xa7\xe5\x99\xa8",
  "\xe9\x94\xae\xe7\x9b\x98",
  "\xe6\x89\x8b\xe6\x9c\xba",
  "\xe5\xbe\xae\xe6\xb3\xa2\xe7\x82\x89",
  "\xe7\x83\xa4\xe7\xae\xb1",
  "\xe7\x83\xa4\xe9\x9d\xa2\xe5\x8c\x85\xe6\x9c\xba",
  "\xe6\xb0\xb4\xe6\xa7\xbd",
  "\xe5\x86\xb0\xe7\xae\xb1",
  "\xe4\xb9\xa6",
  "\xe6\x97\xb6\xe9\x92\x9f",
  "\xe8\x8a\xb1\xe7\x93\xb6",
  "\xe5\x89\xaa\xe5\x88\x80",
  "\xe6\xb3\xb0\xe8\xbf\xaa\xe7\x86\x8a",
  "\xe5\x90\xb9\xe9\xa3\x8e\xe6\x9c\xba",
  "\xe7\x89\x99\xe5\x88\xb7"
};

static const char *coco_names[] = {
  "person",
  "bicycle",
  "car",
  "motorbike",
  "aeroplane",
  "bus",
  "train",
  "truck",
  "boat",
  "traffic light",
  "fire hydrant",
  "stop sign",
  "parking meter",
  "bench",
  "bird",
  "cat",
  "dog",
  "horse",
  "sheep",
  "cow",
  "elephant",
  "bear",
  "zebra",
  "giraffe",
  "backpack",
  "umbrella",
  "handbag",
  "tie",
  "suitcase",
  "frisbee",
  "skis",
  "snowboard",
  "sports ball",
  "kite",
  "baseball bat",
  "baseball glove",
  "skateboard",
  "surfboard",
  "tennis racket",
  "bottle",
  "wine glass",
  "cup",
  "fork",
  "knife",
  "spoon",
  "bowl",
  "banana",
  "apple",
  "sandwich",
  "orange",
  "broccoli",
  "carrot",
  "hot dog",
  "pizza",
  "donut",
  "cake",
  "chair",
  "sofa",
  "pottedplant",
  "bed",
  "diningtable",
  "toilet",
  "tvmonitor",
  "laptop",
  "mouse",
  "remote",
  "keyboard",
  "cell phone",
  "microwave",
  "oven",
  "toaster",
  "sink",
  "refrigerator",
  "book",
  "clock",
  "vase",
  "scissors",
  "teddy bear",
  "hair drier",
  "toothbrush"
};

static const int coco_classes = sizeof(coco_names)/sizeof(coco_names[0]);

static const char *get_coco_classname(int class_id, int locale) {
  if (class_id < 0 || class_id >= coco_classes) {
    return "";
  }
  return locale == 1 ?  coco_names_zhcn[class_id] : coco_names[class_id];
}

static void i18n_age(char *out, uint8_t age, int locale) {
  if (locale == 1)
    sprintf(out, "%d\xe5\xb2\x81", age);
  else
    sprintf(out, "Age %d", age);
}

static void i18n_face_db(char *out, int32_t db_id, int locale) {
  if (db_id > 0) {
    sprintf(out, "ID %d", db_id);
  } else {
    if (locale == 1) {
      sprintf(out, "\xe9\x99\x8c\xe7\x94\x9f\xe4\xba\xba");
    } else {
      sprintf(out, "Stranger");
    }
  }
}

static const float colors[6][3] = { {1,0,1}, {0,0,1},{0,1,1},{0,1,0},{1,1,0},{1,0,0} };

static float get_color(int c, int x, int max)
{
    float ratio = ((float)x/max)*5;
    int i = floor(ratio);
    int j = ceil(ratio);
    ratio -= i;
    float r = (1-ratio) * colors[i][c] + ratio*colors[j][c];
    //printf("%f\n", r);
    return r;
}

static void get_coco_classcolor(int class_id, uint8_t *R, uint8_t *G, uint8_t *B) {
  int offset = class_id * 123457 % coco_classes;
  float red = get_color(2, offset, coco_classes);
  float green = get_color(1, offset, coco_classes);
  float blue = get_color(0, offset, coco_classes);

  *R = red * 255;
  *G = green * 255;
  *B = blue * 255;
}


struct ai_renderer_priv {
  struct ass_library *library;
  struct ass_renderer *renderer;
  struct ass_track *track;

  int default_style;
  int locale;
};

#define MP_ASS_RGBA(r, g, b, a) \
    (((unsigned)(r) << 24) | ((g) << 16) | ((b) << 8) | (0xFF - (a)))


static int ai_ass_add_style(ASS_Track *track, const char* name, const char* override_font) {
  int sid = ass_alloc_style(track);
  ASS_Style *style = track->styles + sid;

  style->Name = _strdup(name);
  if (override_font) {
    if (!style->FontName || strcmp(style->FontName, override_font) != 0) {
      free(style->FontName);
      style->FontName = _strdup(override_font);
    }
  }

  style->FontSize = 20;
  style->PrimaryColour = MP_ASS_RGBA(0, 0, 0, 255);
  style->SecondaryColour = style->PrimaryColour;
  style->OutlineColour = MP_ASS_RGBA(0, 0, 0, 255);
  style->BackColour = MP_ASS_RGBA(0, 0, 0, 0);
  style->BorderStyle = 4; // opaque box
  style->Outline = 0;
  style->Shadow = 1;
  style->Spacing = 0;
  style->MarginL = 0;
  style->MarginR = 0;
  style->MarginV = 0;
  style->ScaleX = 1.;
  style->ScaleY = 1.;
  style->Alignment = 5;
#ifdef ASS_JUSTIFY_LEFT
  style->Justify = 0;
#endif
  style->Blur = 0;
  style->Bold = 0;
  style->Italic = 0;

  return sid;
}

static void ai_ass_add_eventf(ai_renderer ctx, ASS_Track *track, uint32_t label,
                                    long long start, long long duration,
                                    const char *fmt, ...) {

  va_list ap;

  int n = ass_alloc_event(track);
  ASS_Event *event = track->events + n;
  event->Start = start;
  event->Duration = duration;
  event->Style = ctx->default_style;
  

  va_start(ap, fmt);

  size_t len = strlen(fmt);
  len  = len + 256; // estimate

  char *line = malloc(len);
  int nres = vsnprintf(line, len, fmt, ap);

  while (nres >= len) {
    len += 256;
    free(line);
    line = malloc(len);
    nres = vsnprintf(line, len, fmt, ap);
  }

  event->Text = line;

  va_end(ap);
}

ai_renderer ai_renderer_init() {
  ai_renderer priv = malloc(sizeof(struct ai_renderer_priv));
  memset(priv, 0, sizeof(struct ai_renderer_priv));

  priv->library = ass_library_init();
  priv->renderer = ass_renderer_init(priv->library);
  priv->track = ass_new_track(priv->library);
  priv->track->track_type = TRACK_TYPE_ASS;

  ai_ass_add_style(priv->track, "Default", "Courier New");
  priv->default_style = ai_ass_add_style(priv->track, "face", "Courier New");

  ass_set_fonts(priv->renderer, NULL, "Courier New", ASS_FONTPROVIDER_AUTODETECT, NULL, 1);

  // default
  ai_renderer_set_res(priv, 1024, 768);

  return priv;
}

void ai_renderer_free(ai_renderer priv) {
  if (priv) {
    ass_free_track(priv->track);
    ass_renderer_done(priv->renderer);
    ass_library_done(priv->library);
    free(priv);
  }
}

void ai_renderer_set_locale(ai_renderer ctx, int locale) {
  if (ctx && ctx->locale != locale) {
    ctx->locale = locale;
  }
}

void ai_renderer_set_res(ai_renderer priv, int w, int h) {
  if (priv && priv->track) {
    priv->track->PlayResX = w;
    priv->track->PlayResY = h;
  }
}

void ai_renderer_flush_events(ai_renderer priv) {
  if (priv && priv->track) {
    ass_flush_events(priv->track);
  }
}

static void ass_flush_old_events(ASS_Track *track, long long ts)
{
    int n = 0;
    for (; n < track->n_events; n++) {
        if ((track->events[n].Start + track->events[n].Duration) >= ts)
            break;
        ass_free_event(track, n);
        track->n_events--;
    }
    for (int i = 0; n > 0 && i < track->n_events; i++) {
        track->events[i] = track->events[i+n];
    }
}

ASS_Image *ai_renderer_render_ass_image(
    ai_renderer priv,
    int width,
    int height,
    int ml, int mt, int mr, int mb,
    long long ts, int *detect_change) {
  if (!priv || !priv->renderer || !priv->track)
    return NULL;

  struct ass_track *track = priv->track;

  int dispW = width - ml - mr;
  int dispH = height - mt - mb;
  double dispScale = (double)dispW / dispH;
  double resScale = (double)track->PlayResX / track->PlayResY;
  double scalex = dispScale/resScale;

  // enable/disable output by scaling
  ASS_Style *style = track->styles + priv->default_style;
  style->ScaleX = scalex;
  style->ScaleY = 1;

  ass_set_frame_size(priv->renderer, width, height);
  ass_set_margins(priv->renderer, mt, mb, ml, mr);

  if (ts > 0)
    ass_flush_old_events(track, ts);

  return ass_render_frame(priv->renderer, track, ts, detect_change);
}

static const char* get_frame_color(int label) {
  if (label == 1) {
    return "00FF00";
  } else if (label == 2) {
    return "FFFF00";
  } else {
    return "FF";
  }
}

int ai_render_generate_events(
  ai_renderer ctx,
  ai_detection_t* det,
  long long start, long long duration) {

  bool validArea = det->bottom > det->top && det->right > det->left;
  if (!validArea) {
    // no need to render
    return 0;
  }

  int resX = ctx->track->PlayResX;
  int resY = ctx->track->PlayResY;

  int X0 = det->left * resX;
  int Y0 = det->top * resY;
  const int X1 = det->right * resX;
  const int Y1 = det->bottom * resY;
  if (Y0 < 0) Y0 = 0;
  if (X0 < 0) X0 = 0;
  const int boarder = (resX + 1024) / 1024;
  const int W = X1 - X0;
  const int H = Y1 - Y0;
  const int Wd = W / 4;
  const int Hd = H / 4;

  const char* color = get_frame_color(det->label);

  char color_buf[10];

    int class_id = det->label;
    uint8_t R, G, B;
    get_coco_classcolor(class_id, &R, &G, &B);
    sprintf(color_buf, "%02X%02X%02X", B, G, R);
    color = &color_buf[0];
    const char *label_text = get_coco_classname(class_id, ctx->locale);
    bool bold = false;


  // left-top
  ai_ass_add_eventf(ctx, ctx->track, det->label, start, duration,
      "{\\3c&H%s&\\3a&H55&\\1a&HFF&\\bord%d}{\\pos(0,0)\\clip(%d,%d,%d,%d)}{\\p1} m %d %d l %d %d %d %d %d %d{\\p0}",
      color,
      boarder,
      X0 - (boarder+2), Y0 - (boarder+2), X0 + Wd, Y0 + Hd,
      X0, Y0, X0 + (Wd+2), Y0, X0 + (Wd+2), Y0 + (Hd+2), X0, Y0 + (Hd+2));

  // right-top
  ai_ass_add_eventf(ctx, ctx->track, det->label, start, duration,
      "{\\3c&H%s&\\3a&H55&\\1a&HFF&\\bord%d}{\\pos(0,0)\\clip(%d,%d,%d,%d)}{\\p1} m %d %d l %d %d %d %d %d %d{\\p0}", 
      color,
      2,//boarder,
      X1 - Wd, Y0 - (boarder+2), X1 + (boarder+2),  Y0 + Hd,
      X1 - (Wd+2), Y0, X1, Y0, X1, Y0 + (Hd+2), X1 - (Wd+2), Y0 + (Hd+2));

  // right-bottom
  ai_ass_add_eventf(ctx, ctx->track, det->label, start, duration,
      "{\\3c&H%s&\\3a&H55&\\1a&HFF&\\bord%d}{\\pos(0,0)\\clip(%d,%d,%d,%d)}{\\p1} m %d %d l %d %d %d %d %d %d{\\p0}", 
      color,
      2,//boarder,
      X1 - Wd, Y1 - Hd, X1 + (boarder+2),  Y1 + (boarder+2),
      X1 - (Wd+2), Y1 - (Hd+2), X1, Y1 - (Hd+2), X1, Y1, X1 - (Wd+2), Y1);

  // left-bottom
  ai_ass_add_eventf(ctx, ctx->track, det->label, start, duration,
      "{\\3c&H%s&\\3a&H55&\\1a&HFF&\\bord%d}{\\pos(0,0)\\clip(%d,%d,%d,%d)}{\\p1} m %d %d l %d %d %d %d %d %d{\\p0}",
      color,
      2,//boarder,
      X0 - (boarder+2), Y1 - Hd, X0 + Wd, Y1 + (boarder+2),
      X0, Y1 - (Hd+2), X0 + (Wd+2), Y1 - (Hd+2), X0 + (Wd+2), Y1, X0, Y1);

      
  if (Y0 < 30) 
    Y0 += (30 - Y0);

  // label
  ai_ass_add_eventf(ctx, ctx->track, det->label, start, duration,
    "{\\pos(%d,%d)\\1a&H00&\\1c&HFFFFFF&\\4a&H66&\\4c&H%s&\\bord0}\\h{\\b%d}%s{\\b0} %.2f\\h", 
      X0, Y0 - 25, color, bold ? 1:0, label_text, det->confidence);

  return 1;
}

int ai_render_update_events_duration(
  ai_renderer ctx,
  long long pts)
{
  if (ctx->track->events) {
    for (int n = 0; n < ctx->track->n_events; n++) {
      ASS_Event *event = ctx->track->events + n;
      if (/*event->Duration > 100000 && */ event->Start <= pts) {
        event->Duration = pts - event->Start - 1;
      }
    }
  }

  return 0;
}
