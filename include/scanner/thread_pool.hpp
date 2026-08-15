#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stop_token>
#include <thread>
#include <vector>

namespace scanner {

class ThreadPool {
public:
  explicit ThreadPool(size_t num_threads, size_t max_queue_depth);
  ~ThreadPool();

  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;

  // Encola una tarea. Si la cola está llena, retorna false inmediatamente (no
  // bloquea). Devuelve false si el pool está detenido o se solicita cancelación
  // mediante el stop_token.
  bool enqueue(std::function<void()> task, std::stop_token st);

  // Espera hasta que todas las tareas pendientes se completen (o se solicite
  // cancelación).
  void wait_all(std::stop_token st);

  // Ejecuta una tarea de la cola si hay alguna disponible.
  // Retorna true si ejecutó una tarea, false si la cola estaba vacía.
  bool run_one_task();

  // Solicita parada y espera a que los hilos terminen.
  void shutdown();

  size_t worker_count() const { return workers_.size(); }

private:
  struct Task {
    std::function<void()> func;
  };

  std::vector<std::jthread> workers_;
  std::queue<Task> tasks_;
  mutable std::mutex mutex_;
  std::condition_variable cv_work_;
  size_t max_queue_depth_;
  bool stopping_ = false;
  size_t active_tasks_ = 0;

  void worker_loop(std::stop_token st);
};

} // namespace scanner