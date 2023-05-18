[![tests](https://github.com/alugowski/task_thread_pool/actions/workflows/tests.yml/badge.svg)](https://github.com/alugowski/task_thread_pool/actions/workflows/tests.yml)

`task_thread_pool` is a fast and lightweight C++11 thread pool.

Use this to easily add parallelism to your project without introducing heavy dependencies.

### Strengths
* Single header file and permissive license means easy integration
* Automated testing on all major platforms and compilers
  * Linux, macOS, Windows
  * GCC, Clang, MSVC
  * C++11 through C++23
* Comprehensive test suite, including stress tests
* Benchmarks help confirm good performance

### Weaknesses
If you need to parallelize some `for` loops, use OpenMP instead.

If you have many tiny tasks, or you have many physical cores, use a more heavy-duty executor like those in Threading Building Blocks.

# Usage

```c++
#include <task_thread_pool.hpp>
```

### Create pool

Create a pool with thread count equal to number of physical cores on the machine:
```c++
task_thread_pool::task_thread_pool pool;
```

You may also specify the number of worker threads:

```c++
task_thread_pool::task_thread_pool pool{4};
```

### Submit tasks
Submit any [*Callable*](https://en.cppreference.com/w/cpp/named_req/Callable) and its arguments (if any). A Callable can be a function, a lambda, `std::packaged_task`, `std::function`, etc.

This returns a [`std::future`](https://en.cppreference.com/w/cpp/thread/future) for tracking the execution of this task. Use `get()` on this future to wait for the task to complete and get its return value (or thrown exception).
```c++
std::future<int> future = pool.submit([] { return 1; });

int result = future.get(); // returns 1
```

If you do not care about the `std::future` then use `submit_detach()` instead:
```c++
pool.submit_detach([] { /* some work */ });
```

### Waiting for tasks

```c++
pool.wait_for_tasks();
```
blocks the calling thread until all tasks have been completed.

### Pausing

Use `pool.pause()` to stop workers from starting new tasks. The destructor or `pool.unpause()` resume execution.


# Installation


### Copy
`task_thread_pool` is a single header file.

You may simply copy [task_thread_pool.hpp](include/task_thread_pool.hpp) into your project or your system `include/`.

### CMake

You may use CMake to fetch directly from GitHub:
```cmake
include(FetchContent)
FetchContent_Declare(
        task_thread_pool
        GIT_REPOSITORY https://github.com/alugowski/task_thread_pool
        GIT_TAG main
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(task_thread_pool)

target_link_libraries(YOUR_TARGET task_thread_pool::task_thread_pool)
```

Use `GIT_TAG main` to always use the latest version, or replace `main` with a version number to pin a fixed version.


# How it works

Given that simplicity is a major goal, this thread pool does what you'd expect. Submitted tasks are added to a queue
and worker threads pull from this queue.

Care is taken that this process is efficient. The `submit` methods are optimized to only do what they need. Worker threads only lock the queue once per task. Excess synchronization is avoided.

That said, this simple design is best used in low contention scenarios. If you have many tiny tasks or many (10+) physical CPU cores then this single queue becomes a hotspot. In that case avoid lightweight pools like this one and use something like Threading Building Blocks. They include work-stealing executors that avoid this hotspot at the cost of extra complexity and project dependencies.

# Benchmarking

We include some Google Benchmarks for some pool operations in [benchmark/](benchmark).

If there is an operation you care about feel free to open an issue or submit your own benchmark code.

```
-------------------------------------------------------------------------------
Benchmark                                     Time             CPU   Iterations
-------------------------------------------------------------------------------
pool_create_destroy                       45857 ns        25694 ns        27657
submit_detach_packaged_task/paused:1        244 ns          232 ns      3140309
submit_detach_packaged_task/paused:0        410 ns          336 ns      2085823
submit_detach_void_lambda/paused:1          217 ns          212 ns      3190389
submit_detach_void_lambda/paused:0          403 ns          340 ns      2222653
submit_void_lambda/paused:1                 333 ns          290 ns      2327491
submit_void_lambda/paused:0                 338 ns          289 ns      2076566
submit_void_lambda_future/paused:1          342 ns          300 ns      2422933
submit_void_lambda_future/paused:0          299 ns          258 ns      2677786
submit_int_lambda_future/paused:1           382 ns          344 ns      2017204
submit_int_lambda_future/paused:0           744 ns          607 ns      1209817
run_1k_packaged_tasks                    462047 ns       369652 ns         1864
run_1k_void_lambdas                      468706 ns       389721 ns         1831
```