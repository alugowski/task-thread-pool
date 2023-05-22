[![tests](https://github.com/alugowski/task-thread-pool/actions/workflows/tests.yml/badge.svg)](https://github.com/alugowski/task-thread-pool/actions/workflows/tests.yml)
[![codecov](https://codecov.io/gh/alugowski/task-thread-pool/branch/main/graph/badge.svg?token=M9J4azRYyI)](https://codecov.io/gh/alugowski/task-thread-pool)
![File size in bytes](https://img.shields.io/github/size/alugowski/task-thread-pool/include/task_thread_pool.hpp)
[![License](https://img.shields.io/badge/License-BSD_2--Clause-orange.svg)](https://opensource.org/licenses/BSD-2-Clause)
[![License](https://img.shields.io/badge/License-Boost_1.0-lightblue.svg)](https://www.boost.org/LICENSE_1_0.txt)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

`task-thread-pool` is a fast and lightweight thread pool for C++11 and newer.

Easily add parallelism to your project without introducing heavy dependencies.

* Focus on correctness, ease of use, simplicity, performance.
* Small [Single header file](https://raw.githubusercontent.com/alugowski/task-thread-pool/main/include/task_thread_pool.hpp) and permissive licensing means easy integration
* Tested on all major platforms and compilers
  * Linux, macOS, Windows
  * GCC, Clang, MSVC
  * C++11, C++14, C++17, C++20, C++23
* Comprehensive test suite, including stress tests
* Benchmarks help confirm good performance

# Usage

```c++
#include <task_thread_pool.hpp>
```

```c++
task_thread_pool::task_thread_pool pool; // num_threads = number of physical cores
// or
task_thread_pool::task_thread_pool pool{4}; // num_threads = 4
```

### Submit tasks
Submit a function, a lambda, `std::packaged_task`, `std::function`, or any [*Callable*](https://en.cppreference.com/w/cpp/named_req/Callable), and its arguments (if any).
```c++
pool.submit_detach([](int arg) { /* some work */ }, 123456);
```

If your task returns a value (or throws an exception that you care about), then `submit()` returns a [`std::future`](https://en.cppreference.com/w/cpp/thread/future)
for tracking the task:

Use `get()` on this future to wait for the task to complete and get its return value (or thrown exception):
```c++
std::future<int> future = pool.submit([] { return 1; });

int result = future.get(); // returns 1
```

### Waiting for tasks

```c++
pool.wait_for_tasks();
```
blocks the calling thread until all tasks have been completed.

If you only wish to wait for a particular task, use the `std::future` returned by `submit()`.

### Pausing

Use `pool.pause()` to stop workers from starting new tasks. The destructor or `pool.unpause()` resume execution.


# Installation


### Copy
`task-thread-pool` is a single header file.

You may simply copy [task_thread_pool.hpp](https://raw.githubusercontent.com/alugowski/task-thread-pool/main/include/task_thread_pool.hpp) into your project or your system `include/`.

### CMake

You may use CMake to fetch directly from GitHub:
```cmake
include(FetchContent)
FetchContent_Declare(
        task-thread-pool
        GIT_REPOSITORY https://github.com/alugowski/task-thread-pool
        GIT_TAG main
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(task-thread-pool)

target_link_libraries(YOUR_TARGET task-thread-pool::task-thread-pool)
```

Use `GIT_TAG main` to always use the latest version, or replace `main` with a version number to pin a fixed version.


# How it works

Simplicity is a major goal so this thread pool does what you'd expect. Submitted tasks are added to a queue
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
pool_create_destroy                       46318 ns        26004 ns        26489
submit_detach_packaged_task/paused:1        254 ns          244 ns      2875157
submit_detach_packaged_task/paused:0        362 ns          304 ns      2296008
submit_detach_void_lambda/paused:1          263 ns          260 ns      3072412
submit_detach_void_lambda/paused:0          418 ns          374 ns      2020779
submit_void_lambda/paused:1                 399 ns          385 ns      1942879
submit_void_lambda/paused:0                 667 ns          543 ns      1257161
submit_void_lambda_future/paused:1          391 ns          376 ns      1897255
submit_void_lambda_future/paused:0          649 ns          524 ns      1238653
submit_int_lambda_future/paused:1           395 ns          376 ns      1902789
submit_int_lambda_future/paused:0           643 ns          518 ns      1146038
run_1k_packaged_tasks                    462965 ns       362080 ns         1939
run_1k_void_lambdas                      492022 ns       411069 ns         1712
run_1k_int_lambdas                       679579 ns       533813 ns         1368
```