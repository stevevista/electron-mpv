#pragma once
#include "tiny-network.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void(*pfn_on_detections)(void *opaque, const ai_detection_t*, int);

int netwrok_async_detection(
    int *serviceId, const AVFrame* frame, pfn_on_detections callback, void *opaque);
void netwrok_async_detection_cancel(int serviceId);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#include <functional>
#include <vector>

namespace network_tiny {

// c++ version interface

using on_detections_callback = std::function<void(const std::vector<ai_detection_t>)>;


_declspec(dllexport) int netwrok_async_detection(
    int *serviceId, const AVFrame* frame, on_detections_callback callback);

}

#endif
