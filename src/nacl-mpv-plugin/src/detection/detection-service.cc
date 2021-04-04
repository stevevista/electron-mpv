extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include "safe-task-queue.hpp"
#include "detection-service.h"
#include <vector>
#include <atomic>

using namespace std;
using namespace bella;

///////////////////////////////////////////////////////////
//
// Singleton Tasks

class SpinLock {
  std::atomic_flag locked = ATOMIC_FLAG_INIT;
public:
  void lock() {
    while (locked.test_and_set(std::memory_order_acquire)) { ; }
  }
  void unlock() {
    locked.clear(std::memory_order_release);
  }
};

class DetectionTask {
public:
  // frame == nullptr means cancelled
  struct TaskNode {
    int serviceId;
    AVFrame *frame;
    pfn_on_detections callback;
    void* opaque;
    SpinLock access;

    TaskNode(int serviceId, AVFrame *frame, pfn_on_detections callback, void* opaque) {
      this->serviceId = serviceId;
      this->frame = frame;
      this->callback = callback;
      this->opaque = opaque;
    }

    TaskNode(TaskNode&& other) {
      this->serviceId = other.serviceId;
      this->frame = other.frame;
      this->callback = other.callback;
      this->opaque = other.opaque;
    }

    TaskNode(const TaskNode& other) = delete;
    TaskNode &operator=(const TaskNode& other) = delete;
  };

  using QueueType = SafeTaskQueue<TaskNode, 1>;

  DetectionTask();

  ~DetectionTask();

  // allocate a global id to identify user
  int allocateId() {
    return idSeed_++;
  }

  int queueTask(
    int *pserviceId,
    const AVFrame* frame,
    pfn_on_detections callback,
    void *opaque
  );

  void cancel(QueueType::Cleaner fn) {
    q_.cancel(fn);
  }

private:
  int ensureModelLoaded();

private:
  // ensure singleton access
  AVFrame *rgb24Frame_{nullptr};
  network* net_{nullptr};
  bool disabled_{false};
  QueueType q_;
  atomic<int> idSeed_{1};
};

static int scale_frame_to_rgb24(const AVFrame* src, AVFrame *dst) {
  auto sws = sws_getContext(src->width, src->height, (AVPixelFormat)src->format, dst->width, dst->height, AV_PIX_FMT_RGB24,
          0, NULL, NULL, NULL);

  if (sws_scale(sws, src->data, src->linesize, 0, src->height, dst->data, dst->linesize) < 0) {
    sws_freeContext(sws);
    return -1;
  }
  sws_freeContext(sws);
  return 0;
}

DetectionTask::~DetectionTask() {
  q_.clear(nullptr);
  q_.join();
  av_frame_free(&rgb24Frame_);
  free_networkp(&net_);
}

DetectionTask::DetectionTask() {
  q_.setExecutor([this](TaskNode &t) {
    {
      lock_guard<SpinLock> lk(t.access);
      if (!t.callback) {
        return;
      }

      if (ensureModelLoaded() < 0) {
        av_frame_free(&t.frame);
        t.callback(t.opaque, nullptr, -1);
        t.callback = nullptr;
        return;
      }

      int r = scale_frame_to_rgb24(t.frame, rgb24Frame_);
      av_frame_free(&t.frame);
      if (r < 0) {
        t.callback(t.opaque, nullptr, -1);
        t.callback = nullptr;
        return;
      }
    }
  
    network_forward_rbg24(net_, rgb24Frame_->data[0], rgb24Frame_->width, rgb24Frame_->height, rgb24Frame_->linesize[0]);

    int nboxes = 0;
    ai_detection_t* dets = get_network_detections(net_, .45f, .24f, &nboxes);

    lock_guard<SpinLock> lk(t.access);
    if (!t.callback) {
      return;
    }

    if (dets) {
      t.callback(t.opaque, dets, nboxes);
      free(dets);
    } else {
      t.callback(t.opaque, nullptr, 0);
    }
    t.callback = nullptr;
  });
}

int DetectionTask::ensureModelLoaded() {
  if (net_) {
    return 0;
  }

  if (disabled_) 
    return -3;

  net_ = load_network("yolov4-tiny.weights");
  if (!net_) {
    disabled_ = true;
    return -3;
  }

  rgb24Frame_ = av_frame_alloc();
  rgb24Frame_->width = net_->w;
  rgb24Frame_->height = net_->h;
  rgb24Frame_->format = AV_PIX_FMT_RGB24;
  if (av_frame_get_buffer(rgb24Frame_, 1) < 0) {
    free_networkp(&net_);
    disabled_ = true;
    return -3;
  }
  av_frame_make_writable(rgb24Frame_);

  return 0;
}

int DetectionTask::queueTask(
    int *pserviceId,
    const AVFrame* frame,
    pfn_on_detections callback,
    void *opaque
  )
{
  if (!q_.pushCheck()) {
    return -1;
  }

  if (disabled_) 
    return -3;

  bool idAlloced = false;
  int serviceId = pserviceId ? *pserviceId : 0;
  if (serviceId < 0) {
    serviceId = allocateId();
    idAlloced = true;
  }

  AVFrame *cloned = av_frame_clone(frame);
  if (!cloned) {
    return -1;
  }

  int r = q_.push(move(TaskNode{serviceId, cloned, callback, opaque}));
  if (r < 0) {
    av_frame_free(&cloned);
    return r;
  }

  if (idAlloced && pserviceId) {
    *pserviceId = serviceId;
  }
  return serviceId;
}

static DetectionTask detections;

int netwrok_async_detection(
    int *serviceId, const AVFrame* frame, pfn_on_detections callback, void *opaque) {
  return detections.queueTask(serviceId, frame, callback, opaque);
}

void netwrok_async_detection_cancel(int serviceId) {
  detections.cancel([serviceId](DetectionTask::TaskNode &t) {
    if (t.serviceId != serviceId) {
      return;
    }

    lock_guard<SpinLock> lk(t.access);
    if (!t.callback) {
      return;
    }

    av_frame_free(&t.frame);
    t.callback(t.opaque, nullptr, -1);
    t.callback = nullptr;
  });
}

namespace network_tiny {

struct opaque_wrapper {
  on_detections_callback callback;
};

static void on_detections_wrapper(void *opaque, const ai_detection_t * dets, int count) {
  auto e = static_cast<opaque_wrapper*>(opaque);
  if (dets && count > 0) {
    e->callback(std::vector<ai_detection>{dets, dets + count});
  } else if (count == 0) {
    e->callback(std::vector<ai_detection>{});
  }
  delete e;
}

int netwrok_async_detection(
    int *serviceId, const AVFrame* frame, on_detections_callback callback) {
  return ::netwrok_async_detection(serviceId, frame, on_detections_wrapper, static_cast<void*>(new opaque_wrapper{callback}));
}

}
