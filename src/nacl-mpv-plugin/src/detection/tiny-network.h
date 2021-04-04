#pragma once

#if defined(_MSC_VER) && _MSC_VER < 1900
#define inline __inline
#endif

#if defined(DEBUG) && !defined(_CRTDBG_MAP_ALLOC)
#define _CRTDBG_MAP_ALLOC
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct network;
typedef struct network network;

struct network_state;
typedef struct network_state network_state;

struct layer;
typedef struct layer layer;

struct detection;
typedef struct detection detection;

typedef struct ai_detection {
  float confidence;
  uint32_t label;
  float left;
  float top;
  float right;
  float bottom;
} ai_detection_t;

// layer.h
typedef enum {
    CONVOLUTIONAL,
    YOLO,
    MAXPOOL,
    ROUTE,
    SHORTCUT,
    UPSAMPLE
} LAYER_TYPE;

// layer.h
struct layer {
    LAYER_TYPE type;

    void(*forward)   (struct layer, struct network_state);

    int shortcut;
    int shortcut_offset;
    int first_layer;
    int second_layer;

    int outputs;
    int nweights;

    int h, w, c;
    int out_h, out_w, out_c;
    int n;

    int size;
    int side;
    int stride;
    int pad;

    int classes;

    float *biases;
    float *weights;
    float * output;
};

// network.h
typedef struct network {
    int n;
    layer *layers;

    int h, w;

    float *input;
    float *workspace;
} network;

// network.h
typedef struct network_state {
    float *input;
    float *workspace;
    int index;
    network net;
} network_state;

// box.h
typedef struct box {
    float x, y, w, h;
} box;

// box.h
typedef struct detection{
    box bbox;
    int classes;
    float prob[80];
    float objectness;
    int sort_class;
} detection;

network create_network();
network *load_network(const char *filename);
void free_network(network *net);
void free_networkp(network **ppnet);
void network_copy_rbg24(network *net, const uint8_t *rgb, int w, int h, int stride);
void network_forward(network *net);
void network_forward_rbg24(network *net, const uint8_t *rgb, int w, int h, int stride);
detection *get_network_boxes(network *net, float thresh, int *num);
ai_detection_t* get_network_detections(network *net, float nms, float thresh, int* num);
const char *get_network_classname(int class_id, int locale);
float box_iou_on_detection(const ai_detection_t *aa, const ai_detection_t *bb);


#ifdef __cplusplus
}
#endif  // __cplusplus
