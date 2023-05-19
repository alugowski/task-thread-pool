// Copyright (C) 2023 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#include <catch2/catch_test_macros.hpp>
#include <task_thread_pool.hpp>

#include "common.hpp"

#define REPEATS 10000

TEST_CASE("sum", "[stress]") {
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
    std::vector<std::future<int>> futures;
    for (int j = 0; j < REPEATS; ++j) {
        futures.clear();

        {
            task_thread_pool::task_thread_pool pool;

            for (int i = 0; i < 100; ++i) {
                futures.push_back(pool.submit([] { return 1; }));
            }
        }
        REQUIRE(futures.size() == 100);
        int sum = 0;
        for (auto& f : futures) {
            sum += f.get();
        }
        REQUIRE(sum == 100);
    }
}

TEST_CASE("constructor", "[stress]") {
    for (int i = 0; i < REPEATS; ++i) {
        task_thread_pool::task_thread_pool pool;
    }
}

