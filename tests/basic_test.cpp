// Copyright (C) 2023 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license, the MIT license, or at your choosing the BSL-1.0 license found in the LICENSE.*.txt files.
// SPDX-License-Identifier: BSD-2-Clause OR MIT OR BSL-1.0

#include <catch2/catch_test_macros.hpp>
#include <task_thread_pool.hpp>

#include "common.hpp"

TEST_CASE("constructor", "") {
    {
        task_thread_pool::task_thread_pool pool;
        REQUIRE(pool.get_num_threads() == std::thread::hardware_concurrency());
        REQUIRE(measure_number_of_threads(pool) == pool.get_num_threads());
    }
    {
        task_thread_pool::task_thread_pool pool(2);
        REQUIRE(pool.get_num_threads() == 2);
        REQUIRE(measure_number_of_threads(pool) == pool.get_num_threads());
    }
}

TEST_CASE("get_thread_count", "") {
    for (unsigned int num_threads : {1, 4, 100}) {
        task_thread_pool::task_thread_pool pool(num_threads);
        REQUIRE(pool.get_num_threads() == num_threads);
        REQUIRE(measure_number_of_threads(pool) == num_threads);
    }
    {
        task_thread_pool::task_thread_pool pool;
        REQUIRE(pool.get_num_threads() == std::thread::hardware_concurrency());
    }
}

TEST_CASE("set_thread_count", "") {
    for (unsigned int init_threads : {1, 4, 100}) {
        task_thread_pool::task_thread_pool pool(init_threads);
        for (unsigned int num_threads : {1, 4, 100}) {
            pool.set_num_threads(num_threads);
            REQUIRE(pool.get_num_threads() == num_threads);
            REQUIRE(measure_number_of_threads(pool) == num_threads);
        }
    }
    {
        task_thread_pool::task_thread_pool pool(23);
        pool.set_num_threads(0);
        REQUIRE(pool.get_num_threads() == std::thread::hardware_concurrency());
    }
}

TEST_CASE("get-methods", "") {
    {
        task_thread_pool::task_thread_pool pool;
        pool.pause();
        REQUIRE(pool.get_num_queued_tasks() == 0);
        REQUIRE(pool.get_num_running_tasks() == 0);
        REQUIRE(pool.get_num_tasks() == 0);

        pool.submit_detach([]{});

        REQUIRE(pool.get_num_queued_tasks() == 1);
        REQUIRE(pool.get_num_running_tasks() == 0);
        REQUIRE(pool.get_num_tasks() == 1);

        pool.unpause();
    }

    {
        task_thread_pool::task_thread_pool pool;
        pool.pause();
        std::atomic<bool> go{false};
        std::atomic<bool> task_started{false};
        pool.submit_detach([&] { task_started = true; while (!go) {}});

        REQUIRE(pool.get_num_queued_tasks() == 1);
        REQUIRE(pool.get_num_running_tasks() == 0);
        REQUIRE(pool.get_num_tasks() == 1);

        pool.unpause();
        while (!task_started) {}

        REQUIRE(pool.get_num_queued_tasks() == 0);
        REQUIRE(pool.get_num_running_tasks() == 1);
        REQUIRE(pool.get_num_tasks() == 1);

        go = true;
    }
}

TEST_CASE("clear_task_queue", "") {
    task_thread_pool::task_thread_pool pool;
    pool.pause();

    REQUIRE(pool.get_num_queued_tasks() == 0);
    pool.submit_detach([&] { });
    REQUIRE(pool.get_num_queued_tasks() == 1);
    pool.clear_task_queue();
    REQUIRE(pool.get_num_queued_tasks() == 0);
}


TEST_CASE("sum", "") {
    std::atomic<int> count{0};
    {
        task_thread_pool::task_thread_pool pool;

        for (int i = 0; i < 100; ++i) {
            pool.submit_detach([&] { ++count; });
        }
    }
    REQUIRE(count == 100);
}

TEST_CASE("pause", "") {
    {
        task_thread_pool::task_thread_pool pool;
        REQUIRE(pool.is_paused() == false);

        std::atomic<int> count{0};

        pool.pause();
        REQUIRE(pool.is_paused() == true);

        for (int i = 0; i < 50; ++i) {
            pool.submit_detach([&] { ++count; });
        }

        REQUIRE(pool.is_paused() == true);
        REQUIRE(count == 0);

        pool.unpause();
        REQUIRE(pool.is_paused() == false);
        pool.wait_for_tasks();
        REQUIRE(pool.is_paused() == false);
        REQUIRE(count == 50);

        for (int i = 0; i < 50; ++i) {
            pool.submit_detach([&] { ++count; });
        }
        REQUIRE(pool.is_paused() == false);
        pool.wait_for_tasks();
        REQUIRE(count == 100);
    }

    // test destroying a paused pool
    {
        std::atomic<bool> task_ran{false};
        {
            task_thread_pool::task_thread_pool pool;
            pool.pause();

            pool.submit_detach([&] { task_ran = true; });
        }
        REQUIRE(task_ran == true);
    }
}

TEST_CASE("future", "") {
    std::vector<std::future<int>> futures;

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

TEST_CASE("submit-with-args", "") {
    std::vector<std::future<int>> futures;

    {
        task_thread_pool::task_thread_pool pool;

        for (int i = 0; i < 5; ++i) {
            futures.push_back(pool.submit([](int arg) { return arg; }, i));
        }
    }
    REQUIRE(futures.size() == 5);
    int sum = 0;
    for (auto& f : futures) {
        sum += f.get();
    }
    REQUIRE(sum == 10);
}

TEST_CASE("submit_detach-with-args", "") {
    std::atomic<int> sum{0};

    {
        task_thread_pool::task_thread_pool pool;

        for (int i = 0; i < 5; ++i) {
            pool.submit_detach([&](int arg) { sum += arg; }, i);
        }
    }
    REQUIRE(sum == 10);
}

TEST_CASE("task-throws", "") {
    task_thread_pool::task_thread_pool pool;
    auto f = pool.submit([]{ throw std::invalid_argument("test"); });
    REQUIRE_THROWS_AS(f.get(), std::invalid_argument);
}

TEST_CASE("rerun-packaged-task", "") {
    // std::packaged_task() can only be called once, it throws on subsequent runs.
    // Ensure this is handled correctly.

    task_thread_pool::task_thread_pool pool{1};

    std::atomic<int> task_run_count{0};

    std::packaged_task<void()> task([&]{ ++task_run_count; });

    // run the task once
    task();

#if !_MSC_VER
    // Move the task directly into the queue so that the second (illegal) call of task() will throw directly in the worker thread.
    pool.submit_detach(std::move(task));
#else
    // MSVC's std::package_task cannot be moved even though the standard says it should be movable.
    // The bugfix is waiting for an ABI change to fix.
    // See https://developercommunity.visualstudio.com/t/unable-to-move-stdpackaged-task-into-any-stl-conta/108672
    // This test is therefore simpler on MSVC.

    auto f2 = pool.submit([&]{ task(); });
    REQUIRE_THROWS_AS(f2.get(), std::future_error);
#endif
    pool.wait_for_tasks();

    REQUIRE(task_run_count == 1);

    // verify that the pool still works
    auto f3 = pool.submit([&]{ ++task_run_count; });
    REQUIRE_NOTHROW(f3.get());
    REQUIRE(task_run_count == 2);
}
