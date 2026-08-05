# AsyncEventsLoop poll-backoff verification kit (PR ROCm/rocm-systems#7898)

Artifacts from the native-Linux verification of the mixed-batch fix
(commit bc2227f744 on `users/seanthegeek/async-events-poll-spin-wsl`).

- `libhsa-runtime64.so.1.base`  — PR head with the nap reverted (pre-PR busy-spin)
- `libhsa-runtime64.so.1.pr`    — PR head before the fix (single 2 ms ceiling)
- `libhsa-runtime64.so.1.fixed` — with bc2227f744 (2 ms WSL ceiling / 200 µs mixed ceiling)
- `mixed_batch_bench[.cpp]`     — latency + idle-CPU benchmark (source + Linux x86_64 binary)

## Pending: WSL2 re-verification of the fixed build

Goal: confirm the fixed library still idles at ~1% CPU on WSL2 (it should be
behaviorally identical there — WSL has `g_use_interrupt_wait == false`, which
selects the unchanged 2 ms ceiling — but this has not been re-measured since
the split-ceiling commit).

On the WSL2 side:

```sh
mkdir -p ~/rocr-fixed && cp libhsa-runtime64.so.1.fixed ~/rocr-fixed/libhsa-runtime64.so.1
# idle-spin repro from ROCm/librocdxg#60 (HIP program that touches the GPU then sleeps):
LD_LIBRARY_PATH=~/rocr-fixed ./idle_repro &
top -p $!   # expect ~1% CPU, matching the PR's original WSL numbers
```

Optionally rerun the original 200-iteration kernel+sync throughput check the
same way to confirm wall-clock is still unchanged.

Note: these .so files were built on native Ubuntu (glibc from Ubuntu 26.04,
Ubuntu clang/gcc); if the WSL distro's glibc is older, rebuild from the branch
instead of copying the binary.

## Native benchmark usage

```sh
mkdir -p /tmp/l && ln -sf $PWD/libhsa-runtime64.so.1.fixed /tmp/l/libhsa-runtime64.so.1
LD_LIBRARY_PATH=/tmp/l ./mixed_batch_bench mixed 300   # or: pure
```

Measured results (Ryzen 9950X3D, gfx1201 + gfx1100, 300 sparse completions):

| build | idle CPU | mixed-batch latency p50 / p99 |
| --- | ---: | ---: |
| base (spin)  | 99.9% | 0.2 / 0.7 µs |
| pr (2 ms)    | 0.2%  | 1219 / 2046 µs |
| fixed (200 µs) | 0.6% | 119 / 249 µs |

Pure interrupt batches: ~10 µs p50, 0% idle CPU on all three builds.
