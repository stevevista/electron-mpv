#pragma once
#include <list>
#include <thread>
#include <mutex>
#include <functional>

namespace bella {

using namespace std;

template<typename T, size_t limitSize>
class SafeTaskQueue {
public:
  using TaskExecutor = function<void(T&)>;
  using Cleaner = function<void(T&)>;

  void setExecutor(TaskExecutor exec) {
    executor_ = exec;
  }

  ~SafeTaskQueue() {
    clear(nullptr);
    join();
  }

  void join() {
    if (th_.joinable()) {
      th_.join();
    }
  }

  void clear(Cleaner fn) {
    if (fn) {
      cancel(fn);
    }

    lock_guard<mutex> lk(mtx_);
    queue_.clear();
  }

  void cancel(Cleaner fn) {
    lock_guard<mutex> lk(mtx_);
    for (auto& t : queue_) {
      fn(t);
    }
  }

  bool pushCheck() {
    lock_guard<mutex> lk(mtx_);
    return queue_.size() < limitSize;
  }

  int push(T&& t) {
    lock_guard<mutex> lk(mtx_);
    if (queue_.size() >= limitSize) {
      return -1;
    }
    queue_.emplace_back(move(t));
    size_t count = queue_.size();

    if (count == 1) {
      join();

      // trigger task thread
      th_ = thread([this] {
        while (true) {
          int newSize = popAndExecute();
          if (newSize <= 0) {
            break;
          }
        }
      });
    }

    return static_cast<int>(count);
  }

private:
  int popAndExecute() {
    unique_lock<mutex> lk(mtx_);
    if (queue_.empty()) {
      return -1;
    }
  
    auto &t = queue_.front();

    if (executor_) {
      lk.unlock();
      executor_(t);
      lk.lock();
    }
  
    // maybe cleared by SafeTaskQueue::clear
    if (!queue_.empty()) queue_.pop_front();
    return static_cast<int>(queue_.size());
  }

private:
  mutable mutex mtx_;
  list<T> queue_; /* (id, T) */
  TaskExecutor executor_;
  thread th_;
};

}
