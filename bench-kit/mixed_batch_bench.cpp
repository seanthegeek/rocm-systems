// Measures async-handler callback latency for an interrupt-backed signal on
// the shared AsyncEventsLoop thread, optionally with an unsatisfied
// polling-only IPC signal registered on the same thread ("mixed" mode).
//
// This reproduces the mixed-batch case from the PR #7898 review: one signal
// without an EopEvent forces the whole batch into polling mode, so the
// question is what that does to (a) callback latency of interrupt-backed
// signals and (b) idle CPU burn of the async-events thread.
//
// Usage: mixed_batch_bench [pure|mixed] [iters]
#include <hsa.h>
#include <hsa_ext_amd.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <random>
#include <semaphore.h>
#include <sys/resource.h>
#include <unistd.h>
#include <vector>

static sem_t g_sem;
static timespec g_cb_time;

static bool Callback(hsa_signal_value_t, void*) {
  clock_gettime(CLOCK_MONOTONIC, &g_cb_time);
  sem_post(&g_sem);
  return false;  // deregister; re-armed each iteration
}

static bool IpcCallback(hsa_signal_value_t, void*) { return false; }

static double DiffUs(const timespec& a, const timespec& b) {
  return (b.tv_sec - a.tv_sec) * 1e6 + (b.tv_nsec - a.tv_nsec) / 1e3;
}

static double CpuMs(const rusage& a, const rusage& b) {
  auto ms = [](const timeval& t) { return t.tv_sec * 1e3 + t.tv_usec / 1e3; };
  return (ms(b.ru_utime) - ms(a.ru_utime)) + (ms(b.ru_stime) - ms(a.ru_stime));
}

#define CHECK(expr)                                                    \
  do {                                                                 \
    hsa_status_t s_ = (expr);                                          \
    if (s_ != HSA_STATUS_SUCCESS) {                                    \
      fprintf(stderr, "FAIL %s -> %d at line %d\n", #expr, s_, __LINE__); \
      return 1;                                                        \
    }                                                                  \
  } while (0)

int main(int argc, char** argv) {
  bool mixed = argc > 1 && strcmp(argv[1], "mixed") == 0;
  int iters = argc > 2 ? atoi(argv[2]) : 300;

  CHECK(hsa_init());
  sem_init(&g_sem, 0, 0);

  // Interrupt-backed signal (InterruptSignal when g_use_interrupt_wait).
  hsa_signal_t sig;
  CHECK(hsa_signal_create(1, 0, nullptr, &sig));

  hsa_signal_t ipc_attached = {0};
  if (mixed) {
    // A signal attached via the IPC path is an IPCSignal (BusyWaitSignal,
    // EopEvent()==NULL) even on an interrupt-enabled native system. Attaching
    // in-process is valid: it just maps the shared block a second time.
    hsa_signal_t ipc_base;
    CHECK(hsa_amd_signal_create(1, 0, nullptr, HSA_AMD_SIGNAL_IPC, &ipc_base));
    hsa_amd_ipc_signal_t handle;
    CHECK(hsa_amd_ipc_signal_create(ipc_base, &handle));
    CHECK(hsa_amd_ipc_signal_attach(&handle, &ipc_attached));
    // Never-satisfied handler: parks a polling-only signal on the shared
    // async-events thread for the whole run.
    CHECK(hsa_amd_signal_async_handler(ipc_attached, HSA_SIGNAL_CONDITION_EQ,
                                       -12345, IpcCallback, nullptr));
  }

  // Arm the interrupt-signal handler so the async thread has its wait batch.
  CHECK(hsa_amd_signal_async_handler(sig, HSA_SIGNAL_CONDITION_EQ, 0, Callback,
                                     nullptr));

  // Phase 1: idle CPU. Nothing completes for 2s of wall time; measure process
  // CPU. In mixed mode this is dominated by the async-events thread's
  // poll/nap (PR) or busy-spin (baseline) behavior.
  rusage ru0, ru1;
  timespec w0, w1;
  getrusage(RUSAGE_SELF, &ru0);
  clock_gettime(CLOCK_MONOTONIC, &w0);
  usleep(2 * 1000 * 1000);
  getrusage(RUSAGE_SELF, &ru1);
  clock_gettime(CLOCK_MONOTONIC, &w1);
  double idle_cpu_ms = CpuMs(ru0, ru1);
  double idle_wall_ms = DiffUs(w0, w1) / 1e3;

  // Phase 2: sparse-completion latency. Randomized 3-8ms gaps so the
  // completion lands at a uniformly random phase of the nap cycle.
  std::mt19937 rng(12345);
  std::uniform_int_distribution<int> gap_us(3000, 8000);
  std::vector<double> lat;
  lat.reserve(iters);
  for (int i = 0; i < iters; ++i) {
    usleep(gap_us(rng));
    timespec t0;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    hsa_signal_store_screlease(sig, 0);
    sem_wait(&g_sem);
    lat.push_back(DiffUs(t0, g_cb_time));
    hsa_signal_store_screlease(sig, 1);
    CHECK(hsa_amd_signal_async_handler(sig, HSA_SIGNAL_CONDITION_EQ, 0,
                                       Callback, nullptr));
  }

  std::sort(lat.begin(), lat.end());
  auto pct = [&](double p) { return lat[(size_t)(p * (lat.size() - 1))]; };
  double sum = 0;
  for (double v : lat) sum += v;

  printf("mode=%s iters=%d\n", mixed ? "mixed" : "pure", iters);
  printf("idle_cpu_ms=%.1f over wall_ms=%.1f (%.1f%% of one core)\n",
         idle_cpu_ms, idle_wall_ms, 100.0 * idle_cpu_ms / idle_wall_ms);
  printf("latency_us min=%.1f p50=%.1f p90=%.1f p99=%.1f max=%.1f avg=%.1f\n",
         lat.front(), pct(0.50), pct(0.90), pct(0.99), lat.back(),
         sum / lat.size());

  hsa_shut_down();
  return 0;
}
