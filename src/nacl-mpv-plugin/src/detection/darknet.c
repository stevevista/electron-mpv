#include "tiny-network.h"
#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

// compare to sort detection** by bbox.x
int compare_by_lefts(const void *a_ptr, const void *b_ptr) {
    const ai_detection_t* a = (ai_detection_t*)a_ptr;
    const ai_detection_t* b = (ai_detection_t*)b_ptr;
    const float delta = (a->left) - (b->left);
    return delta < 0 ? -1 : delta > 0 ? 1 : 0;
}

// compare to sort detection** by best_class probability
int compare_by_probs(const void *a_ptr, const void *b_ptr) {
    const ai_detection_t* a = (ai_detection_t*)a_ptr;
    const ai_detection_t* b = (ai_detection_t*)b_ptr;
    float delta = a->confidence - b->confidence;
    return delta < 0 ? -1 : delta > 0 ? 1 : 0;
}

void draw_detections_v3(int w, int h, ai_detection_t *selected_detections, int selected_detections_num, float thresh, int classes)
{
    // text output
    qsort(selected_detections, selected_detections_num, sizeof(*selected_detections), compare_by_lefts);
    int i;
    for (i = 0; i < selected_detections_num; ++i) {
        const int best_class = selected_detections[i].label;
        printf("%s: %.0f%%", get_network_classname(best_class, 0),    selected_detections[i].confidence * 100);
        printf("\t(left_x: %4.0f   top_y: %4.0f   width: %4.0f   height: %4.0f)\n",
                round((selected_detections[i].left)*w),
                round((selected_detections[i].top)*h),
                round((selected_detections[i].right - selected_detections[i].left)*w), 
                round((selected_detections[i].bottom - selected_detections[i].top)*h));
    }
}

void test_detector(char *weightfile, char *filename, float thresh,
    float hier_thresh, int dont_show, int ext_output, int save_labels)
{
  network *net = load_network(weightfile);
  if (!net) {
    return;
  }

  float nms = .45f;    // 0.4F

  int w, h, c;
  unsigned char *data = stbi_load(filename, &w, &h, &c, 3);
  
  DWORD time = GetTickCount();
  int nboxes = 0;
  network_forward_rbg24(net, data, w, h, 0);
  printf("%s: Predicted in %d milli-seconds.\n", filename, (GetTickCount() - time));

        ai_detection_t* dets = get_network_detections(net, nms, thresh, &nboxes);
        draw_detections_v3(w, h, dets, nboxes, thresh, 80/*l.classes*/);
        free(dets);

  nboxes = 0;
  time = GetTickCount();
  network_forward_rbg24(net, data, w, h, 0);
  printf("%s: Predicted in %d milli-seconds.\n", filename, (GetTickCount() - time));
  
        dets = get_network_detections(net, nms, thresh, &nboxes);
        draw_detections_v3(w, h, dets, nboxes, thresh, 80/*l.classes*/);
        free(dets);
  
  free(data);
  free_networkp(&net);
}


int main(int argc, char **argv)
{
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    printf(" _DEBUG is used \n");
#endif

  float thresh = .24f;
  test_detector(argv[1], argv[2], thresh, 0.5, 0, 0, 0);

  return 0;
}
