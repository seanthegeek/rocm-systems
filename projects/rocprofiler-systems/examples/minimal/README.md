# Minimal

Small, single-file programs that each exercise one specific component or code path of the rocprofiler-systems instrumentation in isolation. They are intentionally trivial so any discrepancy between expected and recorded behavior can be attributed to the profiler rather than the workload. These are targeted reproducers and regression checks, not benchmarks.

## Source Files

- `recursion.cpp` - Calls a single `__attribute__((noinline))` `recurse` function to a configurable depth (default `100`, override via `argv[1]`). Used to verify that recursive frames are not collapsed by the per-thread region cache.
- `pthreads.cpp` - Spawns a single worker thread with `pthread_create` and joins it. Used to verify that the `pthread_create` gotcha records the call's argument annotations on success.
- `numa_alloc.cpp` - Performs a single `numa_alloc(4096)` and releases it with `numa_free(ptr, 4096)`. Used to verify that the `numa_alloc` gotcha records a `size` trace-arg of `4096` (the matching `numa_free` region records a `size` trace-arg of `4096` as well).

## Building

```bash
cmake -B <build_dir> -S <project_root>/examples/minimal
cmake --build <build_dir>
```

| Target | Description |
| -------- | ------------- |
| `minimal-recursion` | Recursion test for the per-thread region cache |
| `minimal-pthreads` | `pthread_create` gotcha trace-args (argument annotations + `return`) |
| `minimal-numa-alloc` | `numa_alloc` gotcha trace-args (`size`). Built only when libnuma is available |
