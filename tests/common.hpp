// Copyright (C) 2023 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license, the MIT license, or at your choosing the BSL-1.0 license found in the LICENSE.*.txt files.
// SPDX-License-Identifier: BSD-2-Clause OR MIT OR BSL-1.0

#include <task_thread_pool.hpp>

#include <chrono>
#include <set>

inline unsigned int measure_number_of_threads(task_thread_pool::task_thread_pool& pool) {
    using namespace std::chrono_literals;

    std::set<std::thread::id> seen_threads;
    std::mutex seen_mutex;
    std::condition_variable barrier;
    std::atomic<bool> go{false};

    for (unsigned int i = 0; i < 5 * pool.get_num_threads(); ++i) {
        pool.submit_detach([&]{
            std::unique_lock<std::mutex> lock(seen_mutex);
            barrier.wait(lock, [&]{ return go.load(); });
            seen_threads.insert(std::this_thread::get_id());
        });
    }

    // give the threads a chance to pick up the tasks
    std::this_thread::sleep_for(2ms);
    go = true;
    barrier.notify_all();
    pool.wait_for_tasks();

    return static_cast<unsigned int>(seen_threads.size());
}
