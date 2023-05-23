// Copyright (C) 2023 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license, the MIT license, or at your choosing the BSL-1.0 license found in the LICENSE.*.txt files.
// SPDX-License-Identifier: BSD-2-Clause OR MIT OR BSL-1.0

#include <algorithm>
#include <chrono>
#include <random>

#include <catch2/catch_test_macros.hpp>

#include <task_thread_pool.hpp>

#include "common.hpp"

using namespace std::chrono_literals;

#define REPEATS 10000

TEST_CASE("submit_detach", "[stress]") {
    // Test lots of tasks
    for (int j = 0; j < REPEATS; ++j) {
        std::atomic<int> count{0};
        {
            task_thread_pool::task_thread_pool pool;

            for (int i = 0; i < 100; ++i) {
                pool.submit_detach([&] { ++count; });
            }
        }
        REQUIRE(count == 100);
    }
}

TEST_CASE("future", "[stress]") {
    std::random_device r;
    auto rng = std::default_random_engine{r()};

    // Test that futures work as expected
    for (int j = 0; j < REPEATS; ++j) {
        std::vector<std::future<int>> futures;
        futures.reserve(100);

        task_thread_pool::task_thread_pool pool;

        for (int i = 0; i < 100; ++i) {
            futures.push_back(pool.submit([] { return 1; }));
        }

        REQUIRE(futures.size() == 100);
        int sum = 0;

        // shuffle the futures to get() them out of order.
        std::shuffle(std::begin(futures), std::end(futures), rng);

        for (auto& f : futures) {
            sum += f.get();
        }
        REQUIRE(sum == 100);
    }
}

TEST_CASE("constructor", "[stress]") {
    // Test that pool can be constructed and destructed without issues.
    for (int i = 0; i < REPEATS; ++i) {
        task_thread_pool::task_thread_pool pool;
    }
}

TEST_CASE("wait_for_tasks", "[stress]") {
    // Test that wait_for_tasks can be called concurrently from multiple threads.
    for (int i = 0; i < REPEATS / 10; ++i) {
        task_thread_pool::task_thread_pool pool(1);
        std::atomic<bool> go{false};
        std::atomic<bool> thread_started{false};

        auto t = std::thread([&]{
            thread_started = true;
            while (!go) {}
            pool.wait_for_tasks();
        });
        pool.submit_detach([]{std::this_thread::sleep_for(2ms);});
        while (!thread_started) {}
        go = true;
        pool.wait_for_tasks();

        t.join();
    }
}
