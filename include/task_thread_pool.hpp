// SPDX-License-Identifier: BSD-2-Clause
/**
 * @brief A fast and lightweight C++11 thread pool.
 * @see https://github.com/alugowski/task_thread_pool
 * @author Adam Lugowski
 *
 * BSD-2-Clause license:
 *
 * Copyright (C) 2023 Adam Lugowski
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AL_TASK_THREAD_POOL_HPP
#define AL_TASK_THREAD_POOL_HPP

// Version macros.
#define TASK_THREAD_POOL_VERSION_MAJOR 1
#define TASK_THREAD_POOL_VERSION_MINOR 0
#define TASK_THREAD_POOL_VERSION_PATCH 3

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>

// MSVC does not set the __cplusplus macro by default, so we must read it from _MSVC_LANG
// See https://devblogs.microsoft.com/cppblog/msvc-now-correctly-reports-__cplusplus/
#if __cplusplus >= 201703L || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#define TTP_CXX17 1
#else
#define TTP_CXX17 0
#endif

#if TTP_CXX17
#define NODISCARD [[nodiscard]]
#else
#define NODISCARD
#endif

namespace task_thread_pool {

#if !TTP_CXX17
    /**
     * A reimplementation of std::decay_t, which is only available since C++14.
     */
    template< class T >
    using decay_t = typename std::decay<T>::type;
#endif

    /**
     * A fast and lightweight thread pool that uses C++11 threads.
     */
    class task_thread_pool {
    public:
        /**
         * Create a task_thread_pool and start worker threads.
         *
         * @param num_threads Number of worker threads. If 0 then number of threads is equal to the number of physical cores on the machine, as given by std::thread::hardware_concurrency().
         */
        explicit task_thread_pool(unsigned int num_threads = 0) {
            if (num_threads < 1) {
                num_threads = std::thread::hardware_concurrency();
                if (num_threads < 1) {
                    num_threads = 1;
                }
            }
            start_threads(num_threads);
        }

        /**
         * Finish all tasks left in the queue then shut down worker threads.
         * If the pool is currently paused then it is resumed.
         */
        ~task_thread_pool() {
            unpause();
            wait_for_queued_tasks();
            stop_all_threads();
        }

        /**
         * Drop all tasks that have been submitted but not yet started by a worker.
         *
         * Tasks already in progress continue executing.
         */
        void clear_task_queue() {
            const std::lock_guard<std::mutex> tasks_lock(task_mutex);
            tasks = {};
        }

        /**
         * Get number of enqueued tasks.
         *
         * @return Number of tasks that have been enqueued but not yet started.
         */
        NODISCARD size_t get_num_queued_tasks() const {
            const std::lock_guard<std::mutex> tasks_lock(task_mutex);
            return tasks.size();
        }

        /**
         * Get number of in-progress tasks.
         *
         * @return Approximate number of tasks currently being processed by worker threads.
         */
        NODISCARD size_t get_num_running_tasks() const {
            const std::lock_guard<std::mutex> tasks_lock(task_mutex);
            return num_inflight_tasks;
        }

        /**
         * Get total number of tasks in the pool.
         *
         * @return Approximate number of tasks both enqueued and running.
         */
        NODISCARD size_t get_num_tasks() const {
            const std::lock_guard<std::mutex> tasks_lock(task_mutex);
            return tasks.size() + num_inflight_tasks;
        }

        /**
         * Get number of worker threads.
         *
         * @return Number of worker threads.
         */
        NODISCARD unsigned int get_num_threads() const {
            const std::lock_guard<std::mutex> threads_lock(thread_mutex);
            return static_cast<unsigned int>(threads.size());
        }

        /**
         * Stop executing new tasks. Use `unpause()` to resume. Note: Destroying the pool will implicitly unpause.
         *
         * Any in-progress tasks continue executing.
         */
        void pause() {
            const std::lock_guard<std::mutex> tasks_lock(task_mutex);
            pool_paused = true;
        }

        /**
         * Resume executing new tasks.
         */
        void unpause() {
            const std::lock_guard<std::mutex> tasks_lock(task_mutex);
            pool_paused = false;
            task_cv.notify_all();
        }

        /**
         * Check whether the pool is paused.
         *
         * @return true if pause() has been called without an intervening unpause().
         */
        NODISCARD bool is_paused() const {
            const std::lock_guard<std::mutex> tasks_lock(task_mutex);
            return pool_paused;
        }

        /**
         * Submit a Callable for the pool to execute and return a std::future.
         *
         * @param func The Callable to execute. Can be a function, a lambda, std::packaged_task, std::function, etc.
         * @param args Arguments for func. Optional.
         * @return std::future that can be used to get func's return value or thrown exception.
         */
        template <typename F, typename... A,
#if TTP_CXX17
            typename R = std::invoke_result_t<std::decay_t<F>, std::decay_t<A>...>
#else
            typename R = typename std::result_of<decay_t<F>(decay_t<A>...)>::type
#endif
            >
        NODISCARD std::future<R> submit(F&& func, A&&... args) {
            // different implementations for standard and MSVC to work around a MSVC bug.
#if _MSC_VER
            // MSVC's std::package_task cannot be moved even though the standard says it should be movable.
            // The bugfix is waiting for an ABI change to fix.
            // See https://developercommunity.visualstudio.com/t/unable-to-move-stdpackaged-task-into-any-stl-conta/108672
            std::shared_ptr<std::packaged_task<R()>> ptask = std::make_shared<std::packaged_task<R()>>(std::bind(std::forward<F>(func), std::forward<A>(args)...));
            submit_detach([ptask] { (*ptask)(); });
            return ptask->get_future();
#else
            std::packaged_task<R()> task(std::bind(std::forward<F>(func), std::forward<A>(args)...));
            std::future<R> f = task.get_future();
            submit_detach(std::move(task));
            return f;
#endif
        }

        /**
         * Submit a zero-argument Callable for the pool to execute.
         *
         * @param func The Callable to execute. Can be a function, a lambda, std::packaged_task, std::function, etc.
         */
        template <typename F>
        void submit_detach(F&& func) {
            const std::lock_guard<std::mutex> tasks_lock(task_mutex);
            tasks.emplace(std::forward<F>(func));
            task_cv.notify_one();
        }

        /**
         * Submit a Callable with arguments for the pool to execute.
         *
         * @param func The Callable to execute. Can be a function, a lambda, std::packaged_task, std::function, etc.
         */
        template <typename F, typename... A>
        void submit_detach(F&& func, A&&... args) {
            const std::lock_guard<std::mutex> tasks_lock(task_mutex);
            tasks.emplace(std::bind(std::forward<F>(func), std::forward<A>(args)...));
            task_cv.notify_one();
        }

        /**
         * Block until the task queue is empty. Some tasks may be in-progress when this method returns.
         */
        void wait_for_queued_tasks() {
            std::unique_lock<std::mutex> tasks_lock(task_mutex);
            notify_task_finish = true;
            task_finished_cv.wait(tasks_lock, [&] { return tasks.empty(); });
            notify_task_finish = false;
        }

        /**
         * Block until all tasks have finished.
         */
        void wait_for_tasks() {
            std::unique_lock<std::mutex> tasks_lock(task_mutex);
            notify_task_finish = true;
            task_finished_cv.wait(tasks_lock, [&] { return tasks.empty() && num_inflight_tasks == 0; });
            notify_task_finish = false;
        }

    protected:

        /**
         * Main function for worker threads.
         */
        void worker_main() {
            bool finished_task = false;

            while (true) {
                std::packaged_task<void()> task;

                {
                    std::unique_lock<std::mutex> tasks_lock(task_mutex);

                    if (finished_task) {
                        --num_inflight_tasks;
                        if (notify_task_finish) {
                            task_finished_cv.notify_all();
                        }
                        finished_task = false;
                    }

                    task_cv.wait(tasks_lock, [&]() { return !pool_running || (!pool_paused && !tasks.empty()); });

                    if (!pool_running) {
                        break;
                    }

                    if (pool_paused || tasks.empty()) {
                        continue;
                    }

                    task = std::move(tasks.front());
                    tasks.pop();
                    ++num_inflight_tasks;
                }

                try {
                    task();
                } catch (...) {
                    // std::packaged_task::operator() may throw in some error conditions, such as if the task
                    // had already been run. Nothing that the pool can do anything about.
                }

                finished_task = true;
            }
        }

        /**
         * Start worker threads.
         *
         * @param num_threads How many threads to start.
         */
        void start_threads(const unsigned int num_threads) {
            const std::lock_guard<std::mutex> threads_lock(thread_mutex);

            for (unsigned int i = 0; i < num_threads; ++i) {
                threads.emplace_back(&task_thread_pool::worker_main, this);
            }
        }

        /**
         * Stop, join, and destroy all worker threads.
         */
        void stop_all_threads() {
            const std::lock_guard<std::mutex> threads_lock(thread_mutex);

            pool_running = false;

            {
                const std::lock_guard<std::mutex> tasks_lock(task_mutex);
                task_cv.notify_all();
            }

            for (auto& thread : threads) {
                thread.join();
            }
            threads.clear();
        }

        /**
         * The worker threads.
         *
         * Access protected by thread_mutex
         */
        std::vector<std::thread> threads;

        /**
         * A mutex for methods that start/stop threads.
         */
        mutable std::mutex thread_mutex;

        /**
         * The task queue.
         *
         * Access protected by task_mutex.
         */
        std::queue<std::packaged_task<void()>> tasks = {};

        /**
         * A mutex for all variables related to tasks.
         */
        mutable std::mutex task_mutex;

        /**
         * Used to notify changes to the task queue, such as a new task added, pause/unpause, etc.
         */
        std::condition_variable task_cv;

        /**
         * Used to notify of finished tasks.
         */
        std::condition_variable task_finished_cv;

        /**
         * A signal for worker threads that the pool is either running or shutting down.
         *
         * Access protected by task_mutex.
         */
        bool pool_running = true;

        /**
         * A signal for worker threads to not pull new tasks from the queue.
         *
         * Access protected by task_mutex.
         */
        bool pool_paused = false;

        /**
         * A signal for worker threads that they should notify task_finished_cv when they finish a task.
         *
         * Access protected by task_mutex.
         */
        bool notify_task_finish = false;

        /**
         * A counter of the number of tasks in-progress by worker threads.
         * Incremented when a task is popped off the task queue and decremented when that task is complete.
         *
         * Access protected by task_mutex.
         */
        int num_inflight_tasks = 0;
    };
}

// clean up
#undef NODISCARD
#undef TTP_CXX17

#endif
