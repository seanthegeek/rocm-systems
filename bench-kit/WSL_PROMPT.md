# Prompt for Claude Code on the WSL2 side

Paste everything below the line into Claude Code running inside your WSL2 ROCm environment.

---

I need to re-verify a ROCr fix on WSL2. Context: PR https://github.com/ROCm/rocm-systems/pull/7898 throttles the `AsyncEventsLoop` polling fallback (the idle-spin fix for https://github.com/ROCm/librocdxg/issues/60). A later commit on the PR branch, `bc2227f744` ("fix(rocr): cap polling nap at 200us when interrupts are available"), split the nap ceiling: 2 ms when `g_use_interrupt_wait == false` (the WSL/dxg case — behaviorally identical to what was already measured here) and 200 µs for mixed batches on interrupt-capable native Linux (already verified on my native box: mixed-batch p99 went 2046→249 µs at 0.6% idle CPU). The only gap is that WSL2 idle CPU has not been re-measured since that commit. Expected result: unchanged, ~1% idle CPU, because WSL always selects the 2 ms branch.

Please:

1. In my clone of `seanthegeek/rocm-systems`, fetch and check out branch `users/seanthegeek/async-events-poll-spin-wsl` and confirm `bc2227f744` is present (`git log --oneline -3`).
2. Build `libhsa-runtime64` from `projects/rocr-runtime` (cmake + ninja, Release). I have built this branch on this WSL setup before, so the toolchain should work. Known build gotchas from the native-Linux build, in case you hit them here:
   - If using Ubuntu stock clang (not AMD's), `trap_handler_gfx12.s` fails to assemble: rename `HW_REG_WAVE_STATE_PRIV`→`HW_REG_STATE_PRIV`, `HW_REG_WAVE_MODE`→`HW_REG_MODE`, and replace `HW_REG_WAVE_SCHED_MODE`→`26`, `HW_REG_XNACK_STATE_PRIV`→`33`, `HW_REG_XNACK_MASK`→`34` (numeric hwreg IDs from AMD's LLVM SIDefines.h). Keep this local, never commit it.
   - If `libnuma-dev` can't be installed, `apt-get download libnuma-dev` + `dpkg -x` to a local prefix works; link against the system `libnuma.so.1`, not the extracted static `libnuma.a` (not PIC).
   - If the system Clang cmake package is broken, only the `clang` and `llvm-objcopy` executables are needed — a minimal `ClangConfig.cmake` shim defining `clang` as an IMPORTED executable suffices.
3. Re-run the idle-spin check from librocdxg#60: a minimal HIP program that touches the GPU once then sleeps (source is in that issue; recreate it if needed). Run it with `LD_LIBRARY_PATH` pointing at the freshly built `libhsa-runtime64.so.1` and measure the process CPU% while idle (e.g. `pidstat -p <pid> 1 10` or `top -b`). Also run it against the stock ROCm runtime for the before/after contrast.
4. Optionally re-run the 200-iteration back-to-back kernel + `hipDeviceSynchronize` throughput check from the PR body and confirm wall-clock is unchanged.
5. Report the numbers vs the PR body's table (stock ~182% idle CPU; patched ~1.07%). Don't post anything to GitHub — just give me the results and, if they confirm, a short draft comment I can review before posting to the PR.
