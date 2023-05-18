// Copyright (C) 2023 Adam Lugowski. All rights reserved.
// Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

#include <benchmark/benchmark.h>
#include <task_thread_pool.hpp>

#define NUM_THREADS 4

/**
 * Measure pool constructor and destructor. This includes starting/stopping threads.
 * Use a fixed number of threads threads for consistency.
 */
static void pool_create_destroy(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        task_thread_pool::task_thread_pool pool(NUM_THREADS);
    }
}
BENCHMARK(pool_create_destroy);

/**
 * Measure submitting a pre-packaged std::packaged_task.
 */
static void submit_detach_packaged_task(benchmark::State& state) {
    task_thread_pool::task_thread_pool pool(NUM_THREADS);
    if (state.range(0)) {
        pool.pause();
    }

    auto func = []{};

    for ([[maybe_unused]] auto _ : state) {
        std::packaged_task<void()> task(func);
        pool.submit_detach(std::move(task));
    }
    pool.clear_task_queue();
}
BENCHMARK(submit_detach_packaged_task)->ArgName("paused")->Arg(true)->Arg(false);

/**
 * Measure submitting a lambda that returns void, not interested in a std::future.
 */
static void submit_detach_void_lambda(benchmark::State& state) {
    task_thread_pool::task_thread_pool pool(NUM_THREADS);
    if (state.range(0)) {
        pool.pause();
    }

    auto func = []{};

    for ([[maybe_unused]] auto _ : state) {
        pool.submit_detach(func);
    }
    pool.clear_task_queue();

}
BENCHMARK(submit_detach_void_lambda)->ArgName("paused")->Arg(true)->Arg(false);

/**
 * Measure submitting a lambda that returns void, with a std::future.
 */
static void submit_void_lambda(benchmark::State& state) {
    task_thread_pool::task_thread_pool pool(NUM_THREADS);
    if (state.range(0)) {
        pool.pause();
    }

    auto func = []{};

    for ([[maybe_unused]] auto _ : state) {
        auto f = pool.submit(func);
        benchmark::DoNotOptimize(f);
    }
    pool.clear_task_queue();

}
BENCHMARK(submit_void_lambda)->ArgName("paused")->Arg(true)->Arg(false);

/**
 * Measure submitting a lambda that returns void, with std::future.
 */
static void submit_void_lambda_future(benchmark::State& state) {
    task_thread_pool::task_thread_pool pool(NUM_THREADS);
    if (state.range(0)) {
        pool.pause();
    }

    auto func = []{};

    for ([[maybe_unused]] auto _ : state) {
        std::future<void> f = pool.submit(func);
        benchmark::DoNotOptimize(f);
    }

    pool.clear_task_queue();
}
BENCHMARK(submit_void_lambda_future)->ArgName("paused")->Arg(true)->Arg(false);


/**
 * Measure submitting a lambda that returns int, with std::future.
 */
static void submit_int_lambda_future(benchmark::State& state) {
    task_thread_pool::task_thread_pool pool(NUM_THREADS);
    if (state.range(0)) {
        pool.pause();
    }

    auto func = []{ return 1; };

    for ([[maybe_unused]] auto _ : state) {
        std::future<int> f = pool.submit(func);
        benchmark::DoNotOptimize(f);
    }

    pool.clear_task_queue();
}
BENCHMARK(submit_int_lambda_future)->ArgName("paused")->Arg(true)->Arg(false);

/**
 * Measure running a pre-packaged std::packaged_task.
 */
static void run_1k_packaged_tasks(benchmark::State& state) {
    auto func = []{};

    for ([[maybe_unused]] auto _ : state) {
        task_thread_pool::task_thread_pool pool(NUM_THREADS);
        for (int i = 0; i < 1000; ++i) {
            std::packaged_task<void()> task(func);
            pool.submit_detach(std::move(task));
        }
    }
}
BENCHMARK(run_1k_packaged_tasks);

/**
 * Measure running a lot of lambdas.
 */
static void run_1k_void_lambdas(benchmark::State& state) {
    auto func = []{};

    for ([[maybe_unused]] auto _ : state) {
        task_thread_pool::task_thread_pool pool(NUM_THREADS);
        for (int i = 0; i < 1000; ++i) {
            pool.submit_detach(func);
        }
    }
}
BENCHMARK(run_1k_void_lambdas);


BENCHMARK_MAIN();
