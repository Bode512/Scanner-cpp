#include "thread_pool.hpp"
#include <cassert>

namespace scanner {

ThreadPool::ThreadPool(size_t num_threads, size_t max_queue_depth)
    : max_queue_depth_(max_queue_depth == 0 ? num_threads * 4
                                            : max_queue_depth) {
  for (size_t i = 0; i < num_threads; ++i) {
    workers_.emplace_back([this](std::stop_token st) { worker_loop(st); });
  }
}

ThreadPool::~ThreadPool() { shutdown(); }

void ThreadPool::worker_loop(std::stop_token st) {
  while (!st.stop_requested()) {
    std::function<void()> task;
    {
      std::unique_lock lock(mutex_);
      cv_work_.wait(lock, [this, &st] {
        return stopping_ || !tasks_.empty() || st.stop_requested();
      });
      if (stopping_ || st.stop_requested()) {
        break;
      }
      if (tasks_.empty())
        continue;
      task = std::move(tasks_.front().func);
      tasks_.pop();
      ++active_tasks_;
    }

    // Ejecutar tarea (puede lanzar excepciones; las capturamos para no matar
    // worker)
    try {
      task();
    } catch (...) {
      // Registrar excepción? Por ahora ignoramos.
    }

    {
      std::unique_lock lock(mutex_);
      --active_tasks_;
      if (tasks_.empty() && active_tasks_ == 0) {
        cv_work_.notify_all(); // Para wait_all
      }
    }
  }
}

bool ThreadPool::enqueue(std::function<void()> task, std::stop_token st) {
  std::unique_lock lock(mutex_);
  if (stopping_ || st.stop_requested()) {
    return false;
  }
  if (tasks_.size() >= max_queue_depth_) {
    return false; // Cola llena, no bloquea
  }
  tasks_.push({std::move(task)});
  cv_work_.notify_one();
  return true;
}

bool ThreadPool::run_one_task() {
  std::function<void()> task;
  {
    std::unique_lock lock(mutex_);
    if (tasks_.empty()) {
      return false;
    }
    task = std::move(tasks_.front().func);
    tasks_.pop();
    ++active_tasks_;
  }

  try {
    task();
  } catch (...) {
  }

  {
    std::unique_lock lock(mutex_);
    --active_tasks_;
    if (tasks_.empty() && active_tasks_ == 0) {
      cv_work_.notify_all(); // Para wait_all
    }
  }
  return true;
}

void ThreadPool::wait_all(std::stop_token st) {
  std::unique_lock lock(mutex_);
  cv_work_.wait(lock, [this, &st] {
    return (tasks_.empty() && active_tasks_ == 0) || stopping_ ||
           st.stop_requested();
  });
}

void ThreadPool::shutdown() {
  {
    std::unique_lock lock(mutex_);
    stopping_ = true;
  }
  cv_work_.notify_all();
  for (auto &w : workers_) {
    w.request_stop();
  }
  for (auto &w : workers_) {
    if (w.joinable())
      w.join();
  }
  workers_.clear();
}

} // namespace scanner