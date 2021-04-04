#include "tiny-network.h"
#include "gemm.h"
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

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

static const int anchors[] = {10,14,  23,27,  37,58,  81,82,  135,169,  344,319};
static const int yolo_masks[][3] = {
  {3, 4, 5},
  {1, 2, 3}
};

static const int coco_classes = sizeof(coco_names)/sizeof(coco_names[0]);
static const int yolo_total = sizeof(anchors)/sizeof(anchors[0])/2;
static const int yolo_N = sizeof(anchors)/sizeof(anchors[0])/4;

static const float scale_x_y = 1.05f;

const char *get_network_classname(int class_id, int locale) {
  if (class_id < 0 || class_id >= coco_classes) {
    return "";
  }
  return coco_names[class_id];
}


static void forward_maxpool_layer(const layer l, network_state state);
static void forward_convolutional_layer(layer l, network_state state);
static void forward_yolo_layer(const layer l, network_state state);
static void forward_route_layer(const layer l, network_state state);
static void forward_upsample_layer(const layer l, network_state net);

static int convolutional_out_height(layer l, int h)
{
    return (h + 2*l.pad - l.size) / l.stride + 1;
}

static int convolutional_out_width(layer l, int w)
{
    return (w + 2*l.pad - l.size) / l.stride + 1;
}

static int get_convolutional_workspace_size(layer l) {
    return l.out_h*l.out_w*l.size*l.size*l.c*sizeof(float);
}

static layer make_upsample_layer(int h, int w, int c)
{
    layer l = { (LAYER_TYPE)0 };
    l.type = UPSAMPLE;
    l.w = w;
    l.h = h;
    l.c = c;
    l.out_w = w << 1;
    l.out_h = h << 1;
    l.out_c = c;
    l.outputs = l.out_w*l.out_h*l.out_c;

    l.output = (float*)calloc(l.outputs, sizeof(float));

    l.forward = forward_upsample_layer;

    return l;
}

static void forward_upsample_layer(const layer l, network_state net)
{
  for (int k = 0; k < l.out_c; ++k) {
    for (int j = 0; j < l.out_h; ++j) {
      for (int i = 0; i < l.out_w; ++i) {
        int in_index = k*l.w*l.h + (j >> 1)*l.w + (i >> 1);
        int out_index = k*l.out_w*l.out_h + j*l.out_w + i;
        l.output[out_index] = net.input[in_index];
      }
    }
  }
}

static layer make_link_layer(int shortcut, int shortcut_offset)
{
  layer l = { (LAYER_TYPE)0 };
  l.type = SHORTCUT;
  l.shortcut = shortcut;
  l.shortcut_offset = shortcut_offset;
  return l;
}

static layer make_route_layer(int first_layer, int second_layer, int first_size, int second_size)
{
  layer l = { (LAYER_TYPE)0 };
  l.type = ROUTE;
  l.first_layer = first_layer;
  l.second_layer = second_layer;
  l.outputs = first_size + second_size;
  l.output = (float*)calloc(l.outputs, sizeof(float));
  l.forward = forward_route_layer;
  return l;
}

static void forward_route_layer(const layer l, network_state state)
{
  layer first = state.net.layers[l.first_layer];
  layer second = state.net.layers[l.second_layer];
  memcpy(l.output, first.output, first.outputs * sizeof(float));
  memcpy(l.output + first.outputs, second.output, second.outputs * sizeof(float));
}

static layer make_convolutional_layer(int h, int w, int c, int n, int size, int stride, int batch_normalize, int *pws_size)
{
  layer l = { (LAYER_TYPE)0 };
  l.type = CONVOLUTIONAL;

    l.h = h;
    l.w = w;
    l.c = c;
    l.n = n;

    l.stride = stride;
    l.size = size;
    l.pad = size / 2;
    l.nweights = c * n * size * size;

    int out_h = convolutional_out_height(l, h);
    int out_w = convolutional_out_width(l, w);
    l.out_h = out_h;
    l.out_w = out_w;
    l.out_c = n;
    l.outputs = l.out_h * l.out_w * l.out_c;

    l.weights = (float*)calloc(l.nweights, sizeof(float));
    l.biases = (float*)calloc(n, sizeof(float));
    l.output = (float*)calloc(l.outputs, sizeof(float));

    l.forward = forward_convolutional_layer;

    int workspace_size = get_convolutional_workspace_size(l);
    if (workspace_size > *pws_size) {
      *pws_size = workspace_size;
    }

    return l;
}

void add_bias(float *output, float *biases, int n, int size)
{
    int i,j;

        for(i = 0; i < n; ++i){
            for(j = 0; j < size; ++j){
                output[i*size + j] += biases[i];
            }
        }
}

static void forward_convolutional_layer(layer l, network_state state)
{
  memset(l.output, 0, l.outputs*sizeof(float));

  int m = l.n;
  int k = l.size*l.size*l.c;
  int n = l.out_h*l.out_w;

            float *a = l.weights;
            float *b = state.workspace;
            float *c = l.output;

            {
                //printf(" l.index = %d - FP32 \n", l.index);
                float *im = state.input;
                if (l.size == 1 && l.stride == 1) {
                    b = im;
                }
                else {
                    im2col_cpu_ext(im,   // input
                        l.c,     // input channels
                        l.h, l.w,           // input size (h, w)
                        l.size, l.size,     // kernel size (h, w)
                        l.pad, l.pad,       // padding (h, w)
                        l.stride, l.stride, // stride (h, w)
                        b);                 // output

                }

                matrixmul(m, n, k, a, b, c);
                // bit-count to float
            }


  add_bias(l.output, l.biases, l.n, l.out_h*l.out_w);

  if (l.type == CONVOLUTIONAL) leaky_activate_array_cpu(l.output, l.outputs);
}

static layer make_yolo_layer(int h, int w, int c, int *pws_size)
{
    layer l = make_convolutional_layer(h, w, c, yolo_N*(coco_classes + 4 + 1), 1, 1, 0, pws_size);
    l.type = YOLO;
    l.classes = coco_classes;
    l.forward = forward_yolo_layer;
    return l;
}

box get_yolo_box(float *x, const int *biases, int n, int index, int i, int j, int lw, int lh, int w, int h, int stride)
{
    box b;
    // ln - natural logarithm (base = e)
    // x` = t.x * lw - i;   // x = ln(x`/(1-x`))   // x - output of previous conv-layer
    // y` = t.y * lh - i;   // y = ln(y`/(1-y`))   // y - output of previous conv-layer
                            // w = ln(t.w * net.w / anchors_w); // w - output of previous conv-layer
                            // h = ln(t.h * net.h / anchors_h); // h - output of previous conv-layer
    b.x = (i + x[index + 0*stride]) / lw;
    b.y = (j + x[index + 1*stride]) / lh;
    b.w = exp(x[index + 2*stride]) * biases[2*n]   / w;
    b.h = exp(x[index + 3*stride]) * biases[2*n+1] / h;
    return b;
}

static int entry_index(layer l, int location, int entry)
{
    int n =   location / (l.out_w*l.out_h);
    int loc = location % (l.out_w*l.out_h);
    return n*l.out_w*l.out_h*(4+l.classes+1) + entry*l.out_w*l.out_h + loc;
}

void scal_add_cpu(int N, float ALPHA, float BETA, float *X, int INCX)
{
    int i;
    for (i = 0; i < N; ++i) X[i*INCX] = X[i*INCX] * ALPHA + BETA;
}

static inline float logistic_activate(float x){return 1.f/(1.f + expf(-x));}

static void logistic_activate_array(float *x, const int n)
{
    int i;
    #pragma omp parallel for
    for (i = 0; i < n; ++i) {
      x[i] = logistic_activate(x[i]);
    }
}

static void forward_yolo_layer(const layer l, network_state state)
{
    forward_convolutional_layer(l, state);

    for (int n = 0; n < yolo_N; ++n) {
            int index = entry_index(l, n*l.out_w*l.out_h, 0);
            logistic_activate_array(l.output + index, 2 * l.out_w*l.out_h);        // x,y,
            scal_add_cpu(2 * l.out_w*l.out_h, scale_x_y, -0.5*(scale_x_y - 1), l.output + index, 1);    // scale x,y
            index = entry_index(l, n*l.out_w*l.out_h, 4);
            logistic_activate_array(l.output + index, (1 + l.classes)*l.out_w*l.out_h);
    }
}


// Converts output of the network to detection boxes
// w,h: image width,height
// netw,neth: network width,height
// relative: 1 (all callers seems to pass TRUE)
static void correct_yolo_boxes(detection *dets, int n, int netw, int neth)
{
    int i;
    // network height (or width)
    int new_w = 0;
    // network height (or width)
    int new_h = 0;
    // Compute scale given image w,h vs network w,h
    // I think this "rotates" the image to match network to input image w/h ratio
    // new_h and new_w are really just network width and height
    {
        new_w = netw;
        new_h = neth;
    }
    // difference between network width and "rotated" width
    float deltaw = netw - new_w;
    // difference between network height and "rotated" height
    float deltah = neth - new_h;
    // ratio between rotated network width and network width
    float ratiow = (float)new_w / netw;
    // ratio between rotated network width and network width
    float ratioh = (float)new_h / neth;
    for (i = 0; i < n; ++i) {

        box b = dets[i].bbox;
        // x = ( x - (deltaw/2)/netw ) / ratiow;
        //   x - [(1/2 the difference of the network width and rotated width) / (network width)]
        b.x = (b.x - deltaw / 2. / netw) / ratiow;
        b.y = (b.y - deltah / 2. / neth) / ratioh;
        // scale to match rotation of incoming image
        b.w *= 1 / ratiow;
        b.h *= 1 / ratioh;

        dets[i].bbox = b;
    }
}

int yolo_num_detections(layer l, float thresh)
{
    int i, n;
    int count = 0;
    for(n = 0; n < yolo_N; ++n) {
      for (i = 0; i < l.out_w*l.out_h; ++i) {
        int obj_index  = entry_index(l, n*l.out_w*l.out_h + i, 4);
        if(l.output[obj_index] > thresh) {
          ++count;
        }
      }
    }
    return count;
}

static int get_yolo_detections(layer l, int yolo_idx, int netw, int neth, float thresh, detection *dets)
{
    int i,j,n;
    float *predictions = l.output;
    // This snippet below is not necessary

    int count = 0;
    for (i = 0; i < l.out_w*l.out_h; ++i){
        int row = i / l.out_w;
        int col = i % l.out_w;
        for(n = 0; n < yolo_N; ++n){
            int obj_index  = entry_index(l, n*l.out_w*l.out_h + i, 4);
            float objectness = predictions[obj_index];
            //if(objectness <= thresh) continue;    // incorrect behavior for Nan values
            if (objectness > thresh) {
                //printf("\n objectness = %f, thresh = %f, i = %d, n = %d \n", objectness, thresh, i, n);
                int box_index = entry_index(l, n*l.out_w*l.out_h + i, 0);
                dets[count].bbox = get_yolo_box(predictions, anchors, yolo_masks[yolo_idx][n], box_index, col, row, l.out_w, l.out_h, netw, neth, l.out_w*l.out_h);
                dets[count].objectness = objectness;
                dets[count].classes = l.classes;

                for (j = 0; j < l.classes; ++j) {
                    int class_index = entry_index(l, n*l.out_w*l.out_h + i, 4 + 1 + j);
                    float prob = objectness*predictions[class_index];
                    dets[count].prob[j] = (prob > thresh) ? prob : 0;
                }
                ++count;
            }
        }
    }
    correct_yolo_boxes(dets, count, netw, neth);
    return count;
}

static void fill_network_boxes(network *net, float thresh, detection *dets)
{
    int yolo_idx = 0;
    for (int j = 0; j < net->n; ++j) {
        layer l = net->layers[j];
        if (l.type == YOLO) {
            int count = get_yolo_detections(l, yolo_idx++, net->w, net->h, thresh, dets);
            dets += count;
        }
    }
}

static int num_detections(network *net, float thresh)
{
    int i;
    int s = 0;
    for (i = 0; i < net->n; ++i) {
        layer l = net->layers[i];
        if (l.type == YOLO) {
            s += yolo_num_detections(l, thresh);
        }
    }
    return s;
}

detection *get_network_boxes(network *net, float thresh, int *num)
{
  int nboxes = num_detections(net, thresh);
  if (num) *num = nboxes;
  detection *dets = (detection*)calloc(nboxes, sizeof(detection));
  fill_network_boxes(net, thresh, dets);
  return dets;
}

// Creates array of detections with prob > thresh and fills best_class for them
ai_detection_t* get_actual_detections(detection *dets, int dets_num, float thresh, int* selected_detections_num)
{
    int selected_num = 0;
    ai_detection_t* result_arr = (ai_detection_t*)calloc(dets_num, sizeof(ai_detection_t));
    int i;
    for (i = 0; i < dets_num; ++i) {
        int best_class = -1;
        float best_class_prob = thresh;
        int j;
        for (j = 0; j < dets[i].classes; ++j) {
            if (dets[i].prob[j] > best_class_prob) {
                best_class = j;
                best_class_prob = dets[i].prob[j];
            }
        }
        if (best_class >= 0) {
          memset(&result_arr[selected_num], 0, sizeof(result_arr[selected_num]));
          result_arr[selected_num].label = best_class;
          result_arr[selected_num].confidence = dets[i].prob[best_class];
          result_arr[selected_num].left = dets[i].bbox.x - dets[i].bbox.w/2;
          result_arr[selected_num].right = dets[i].bbox.x + dets[i].bbox.w/2;
          result_arr[selected_num].top = dets[i].bbox.y - dets[i].bbox.h/2;
          result_arr[selected_num].bottom = dets[i].bbox.y + dets[i].bbox.h/2;
          ++selected_num;
        }
    }
    if (selected_detections_num)
        *selected_detections_num = selected_num;
    return result_arr;
}


float overlap(float x1, float w1, float x2, float w2)
{
    float l1 = x1 - w1/2;
    float l2 = x2 - w2/2;
    float left = l1 > l2 ? l1 : l2;
    float r1 = x1 + w1/2;
    float r2 = x2 + w2/2;
    float right = r1 < r2 ? r1 : r2;
    return right - left;
}

float box_intersection(box a, box b)
{
    float w = overlap(a.x, a.w, b.x, b.w);
    float h = overlap(a.y, a.h, b.y, b.h);
    if(w < 0 || h < 0) return 0;
    float area = w*h;
    return area;
}

float box_union(box a, box b)
{
    float i = box_intersection(a, b);
    float u = a.w*a.h + b.w*b.h - i;
    return u;
}

float box_iou(box a, box b)
{
    //return box_intersection(a, b)/box_union(a, b);

    float I = box_intersection(a, b);
    float U = box_union(a, b);
    if (I == 0 || U == 0) {
        return 0;
    }
    return I / U;
}

float box_iou_on_detection(const ai_detection_t *aa, const ai_detection_t *bb) {
  box a = {(aa->right + aa->left)/2, (aa->bottom + aa->top)/2, aa->right - aa->left, aa->bottom - aa->top};
  box b = {(bb->right + bb->left)/2, (bb->bottom + bb->top)/2, bb->right - bb->left, bb->bottom - bb->top};
  return box_iou(a, b);
}

int nms_comparator_v3(const void *pa, const void *pb)
{
    detection a = *(detection *)pa;
    detection b = *(detection *)pb;
    float diff = 0;
    if (b.sort_class >= 0) {
        diff = a.prob[b.sort_class] - b.prob[b.sort_class]; // there is already: prob = objectness*prob
    }
    else {
        diff = a.objectness - b.objectness;
    }
    if (diff < 0) return 1;
    else if (diff > 0) return -1;
    return 0;
}

// https://github.com/Zzh-tju/DIoU-darknet
// https://arxiv.org/abs/1911.08287
void diounms_sort(detection *dets, int total, int classes, float thresh)
{
    int i, j, k;
    k = total - 1;
    for (i = 0; i <= k; ++i) {
        if (dets[i].objectness == 0) {
            detection swap = dets[i];
            dets[i] = dets[k];
            dets[k] = swap;
            --k;
            --i;
        }
    }
    total = k + 1;

    for (k = 0; k < classes; ++k) {
        for (i = 0; i < total; ++i) {
            dets[i].sort_class = k;
        }
        qsort(dets, total, sizeof(detection), nms_comparator_v3);
        for (i = 0; i < total; ++i)
        {
            if (dets[i].prob[k] == 0) continue;
            box a = dets[i].bbox;
            for (j = i + 1; j < total; ++j) {
                box b = dets[j].bbox;
                if (box_iou(a, b) > thresh) {
                    dets[j].prob[k] = 0;
                }
            }
        }
    }
}

ai_detection_t* get_network_detections(network *net, float nms, float thresh, int* selected_detections_num) {
  int nboxes = 0;
  *selected_detections_num = 0;
  detection *dets = get_network_boxes(net, thresh, &nboxes);
  if (!dets)
    return NULL;

  diounms_sort(dets, nboxes, coco_classes, nms);
  ai_detection_t* act_dets = get_actual_detections(dets, nboxes, thresh, selected_detections_num);
  free(dets);
  return act_dets;
}


static layer make_maxpool_layer(int h, int w, int c, int size, int stride)
{
    layer l = { (LAYER_TYPE)0 };
    int padding = size - 1;
    l.type = MAXPOOL;
    l.h = h;
    l.w = w;
    l.c = c;
    l.pad = padding;
    l.out_w = (w + padding - size) / stride + 1;
    l.out_h = (h + padding - size) / stride + 1;
    l.out_c = c;

    l.outputs = l.out_h * l.out_w * l.out_c;
    l.size = size;
    l.stride = stride;
    int output_size = l.out_h * l.out_w * l.out_c;

    l.output = (float*)calloc(output_size, sizeof(float));
    l.forward = forward_maxpool_layer;

    return l;
}

static void forward_maxpool_layer(const layer l, network_state state)
{
  forward_maxpool_op(state.input, l.output, l.size, l.w, l.h, l.out_w, l.out_h, l.c, l.pad, l.stride);
}

network create_network()
{
    network net = {0};
    net.n = 36;
    net.layers = (layer*)calloc(net.n, sizeof(layer));
    net.h = 416;
    net.w = 416;

    int workspace_size = 0;

    int count = 0;
    layer l = make_convolutional_layer(net.h,net.w, 3, 32, 3, 2, 1, &workspace_size);
    net.layers[count++] = l;

    l = make_convolutional_layer(l.out_h, l.out_w, l.out_c, 64, 3, 2, 1, &workspace_size);
    net.layers[count++] = l;

    for (int nfilter = 32, _ = 0; _ < 3; _++, nfilter *= 2) {
        l = make_convolutional_layer(l.out_h, l.out_w, l.out_c, nfilter*2, 3, 1, 1, &workspace_size);
        net.layers[count++] = l;

        net.layers[count++] = make_link_layer(count-1, l.outputs/2);

        l = make_convolutional_layer(l.out_h, l.out_w, l.out_c/2, nfilter, 3, 1, 1, &workspace_size);
        net.layers[count++] = l;

        l = make_convolutional_layer(l.out_h, l.out_w, l.out_c, nfilter, 3, 1, 1, &workspace_size);
        net.layers[count++] = l;

        layer first = net.layers[count-1];
        layer second = net.layers[count-2];
        net.layers[count++] = make_route_layer(count-1, count-2, first.outputs, second.outputs);

        l = make_convolutional_layer(first.out_h, first.out_w, first.out_c + second.out_c, nfilter*2, 1, 1, 1, &workspace_size);
        net.layers[count++] = l;

        first = net.layers[count-6];
        second = net.layers[count-1];
        net.layers[count++] = make_route_layer(count-6, count-1, first.outputs, second.outputs);

        l = make_maxpool_layer(first.out_h, first.out_w, first.out_c + second.out_c, 2, 2);
        net.layers[count++] = l;
    }

    l = make_convolutional_layer(l.out_h, l.out_w, l.out_c, 512, 3, 1, 1, &workspace_size);
    net.layers[count++] = l;

    layer pin = l = make_convolutional_layer(l.out_h, l.out_w, l.out_c, 256, 1, 1, 1, &workspace_size);
    net.layers[count++] = l;

    l = make_convolutional_layer(l.out_h, l.out_w, l.out_c, 512, 3, 1, 1, &workspace_size);
    net.layers[count++] = l;

    l = make_yolo_layer(l.out_h, l.out_w, l.out_c, &workspace_size);
    net.layers[count++] = l;

    net.layers[count++] = make_link_layer(count-3, 0);

    l = make_convolutional_layer(pin.out_h, pin.out_w, pin.out_c, 128, 1, 1, 1, &workspace_size);
    net.layers[count++] = l;

    l = make_upsample_layer(l.out_h, l.out_w, l.out_c);
    net.layers[count++] = l;

    layer first = net.layers[count-1];
    layer second = net.layers[23];
    net.layers[count++] = make_route_layer(count-1, 23, first.outputs, second.outputs);

    l = make_convolutional_layer(first.out_h, first.out_w, first.out_c + second.out_c, 256, 3, 1, 1, &workspace_size);
    net.layers[count++] = l;

    l = make_yolo_layer(l.out_h, l.out_w, l.out_c, &workspace_size);
    net.layers[count++] = l;

    if (workspace_size) {
      net.workspace = (float*)calloc(1, workspace_size);
    }

    return net;
}

static void free_layer(layer l)
{
    if (l.biases)             free(l.biases), l.biases = NULL;
    if (l.weights)            free(l.weights), l.weights = NULL;
    if (l.output)             free(l.output), l.output = NULL;
}

static int load_weights_upto(network *net, const char *filename)
{
  FILE *fp = fopen(filename, "rb");
  if(!fp) 
    return -1;

  float scales[512]; // enough for our model
  float rolling_mean[512];
  float rolling_variance[512];

  int major;
  int minor;
  int revision;
  fread(&major, sizeof(int), 1, fp);
  fread(&minor, sizeof(int), 1, fp);
  fread(&revision, sizeof(int), 1, fp);
  if ((major * 10 + minor) >= 2) {
    fseek(fp, sizeof(uint64_t), SEEK_CUR);
  } else {
    fseek(fp, sizeof(uint32_t), SEEK_CUR);
  }

#define ENSURE_READ_N(b, x) \
  if ((int)fread(b, sizeof(float), x, fp) != x) return -1;

  for(int i = 0; i < net->n; ++i) {
    layer l = net->layers[i];
    if(l.type == CONVOLUTIONAL) {
      ENSURE_READ_N(l.biases, l.n)
      ENSURE_READ_N(scales, l.n)
      ENSURE_READ_N(rolling_mean, l.n)
      ENSURE_READ_N(rolling_variance, l.n)
      ENSURE_READ_N(l.weights, l.nweights)

      for (int f = 0; f < l.n; ++f) {
        l.biases[f] = l.biases[f] - (double)scales[f] * rolling_mean[f] / (sqrt((double)rolling_variance[f] + .00001));

        double precomputed = scales[f] / (sqrt((double)rolling_variance[f] + .00001));

        const size_t filter_size = l.size*l.size*l.c;
        for (int i = 0; i < filter_size; ++i) {
          int w_index = f*filter_size + i;
          l.weights[w_index] *= precomputed;
        }
      }
    } else if(l.type == YOLO) {
      ENSURE_READ_N(l.biases, l.n)
      ENSURE_READ_N(l.weights, l.nweights)
    }
  }

#undef ENSURE_READ_N

  fclose(fp);
  return 0;
}

// load network & get batch size from cfg-file
network *load_network(const char *filename) {
  network net = create_network();
  if (load_weights_upto(&net, filename) < 0)
    return NULL;

  net.input = (float*)calloc(net.h * net.w * 3, sizeof(float));
  network* pnet = (network*)calloc(1, sizeof(network));
  *pnet = net;
  return pnet;
}


void network_copy_rbg24(network *net, const uint8_t *rgb, int w, int h, int stride) {
  if (stride < w*3) stride = w*3;
  if (w != net->w || h != net->h) {
    // resize
    float w_scale = (float)(w - 1) / (net->w - 1);
    float h_scale = (float)(h - 1) / (net->h - 1);

    for(int r = 0; r < net->h; ++r) {
      float sy = r*h_scale;
      int iy = (int) sy;
      float dy = sy - iy;
      int ymagin = r == net->h-1 || h == 1;

      for(int c = 0; c < net->w; ++c) {
        float sx = c*w_scale;
        int ix = (int) sx;
        float dx = sx - ix;
        int xmagin = c == net->w-1 || w == 1;

        // 0, 1
        // 2, 3
        int src0 = iy * stride + ix * 3;
        int src1 = iy * stride + (ix + 1) * 3;
        int src2 = (iy + 1) * stride + ix * 3;
        int src3 = (iy + 1) * stride + (ix + 1) * 3;

        float W0 = (1 - dy) * (1 - dx);
        float W1 = (1 - dy) * dx;
        float W2 = dy * (1 - dx);
        float W3 = dy * dx;

        float R, G, B;

        if (!ymagin && !xmagin) {
          R = W0 * rgb[src0] + W1 * rgb[src1] + W2 * rgb[src2] + W3 * rgb[src3];
          G = W0 * rgb[src0+1] + W1 * rgb[src1+1] + W2 * rgb[src2+1] + W3 * rgb[src3+1];
          B = W0 * rgb[src0+2] + W1 * rgb[src1+2] + W2 * rgb[src2+2] + W3 * rgb[src3+2];
        } else if (ymagin && !xmagin) {
          R = W0 * rgb[src0] + W1 * rgb[src1];
          G = W0 * rgb[src0+1] + W1 * rgb[src1+1];
          B = W0 * rgb[src0+2] + W1 * rgb[src1+2];
        } else if (!ymagin && xmagin) {
          R = W0 * rgb[src0] + W2 * rgb[src2];
          G = W0 * rgb[src0+1] + W2 * rgb[src2+1];
          B = W0 * rgb[src0+2] + W2 * rgb[src2+2];
        } else {
          R = W0 * rgb[src0];
          G = W0 * rgb[src0+1];
          B = W0 * rgb[src0+2];
        }
        int dst = r * net->w + c;
        net->input[dst] = R / 255.0f;
        net->input[dst + net->w * net->h] = G / 255.0f;
        net->input[dst + net->w * net->h * 2] = B / 255.0f;
      }
    }
  } else {
    // copy
    for(int r = 0; r < h; ++r) {
      for(int c = 0; c < w; ++c) {
        int src = r * stride + c * 3;
        int dst = r * w + c;
        net->input[dst] = rgb[src] / 255.0f;
        net->input[dst + w * h] = rgb[src + 1] / 255.0f;
        net->input[dst + w * h * 2] = rgb[src + 2] / 255.0f;
      }
    }
  }
}

void network_forward(network *net)
{
  network_state state = {0};
  state.net = *net;
  state.index = 0;
  state.input = net->input;
  state.workspace = net->workspace;

  //DWORD time = GetTickCount();
  
  for(int i = 0; i < net->n; ++i){
    layer l = net->layers[i];

    if (l.type == SHORTCUT) {
      state.input = net->layers[l.shortcut].output + l.shortcut_offset;
    } else {
      l.forward(l, state);
      state.input = l.output;
    }
  }

  //printf("Predicted in %d milli-seconds.\n", (GetTickCount() - time));
}

void network_forward_rbg24(network *net, const uint8_t *rgb, int w, int h, int stride)
{
  network_copy_rbg24(net, rgb, w, h, stride);
  network_forward(net);
}

void free_network(network *net)
{
  for (int i = 0; i < net->n; ++i) {
    free_layer(net->layers[i]);
  }
  free(net->input);
  free(net->layers);
  free(net->workspace);
  free(net);
}

void free_networkp(network **ppnet) {
  if (!ppnet || !*ppnet) {
    return;
  }
  free_network(*ppnet);
  *ppnet = NULL;
}
