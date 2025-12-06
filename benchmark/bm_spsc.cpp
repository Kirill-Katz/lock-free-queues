#include <benchmark/benchmark.h>
#include "spsc_queue.hpp"

static void BM_QueueWriteRead(benchmark::State& state) {
    const size_t payload = state.range(0);

    std::vector<std::byte> data(payload, std::byte(10));
    benchmark::DoNotOptimize(data);

    SPSCQueue q;
    benchmark::DoNotOptimize(q);

    for (auto _ : state) {
        q.write(std::span<const std::byte>(data.data(), data.size()));
        benchmark::ClobberMemory();

        auto view = q.read(data.size());
        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * payload);
}


BENCHMARK(BM_QueueWriteRead)
    ->Arg(64)->Arg(256)->Arg(1024)->Arg(4096)->Arg(8096)
    ->ReportAggregatesOnly(true);



