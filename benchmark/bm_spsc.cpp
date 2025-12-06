#include <benchmark/benchmark.h>
#include "spsc_queue.hpp"

static void BM_QueueWrite(benchmark::State& state) {
    const size_t payload = state.range(0);

    std::vector<std::byte> data(payload);
    benchmark::DoNotOptimize(data);

    SPSCQueue q;
    benchmark::DoNotOptimize(q);

    for (auto _ : state) {
        q.write(std::span<const std::byte>(data.data(), data.size()));
        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * payload);
}

BENCHMARK(BM_QueueWrite)
    ->Arg(64)->Arg(256)->Arg(1024)->Arg(4096)
    ->Repetitions(20)
    ->ReportAggregatesOnly(true);

