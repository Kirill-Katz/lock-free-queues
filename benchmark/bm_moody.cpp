#include <benchmark/benchmark.h>
#include "moodycammel_queue.hpp"
#include <cstddef>
#include <memory.h>

static void BM_MCQueueWriteRead(benchmark::State& state) {
    const size_t payload = state.range(0);

    auto blob = new std::byte[payload];
    memset(blob, 0, payload);

    moodycamel::ReaderWriterQueue<decltype(blob)> q(1024);

    for (auto _ : state) {
        bool ok1 = q.try_enqueue(blob);
        benchmark::DoNotOptimize(ok1);

        bool ok2 = q.try_dequeue(blob);
        benchmark::DoNotOptimize(ok2);

        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed(
        static_cast<int64_t>(state.iterations()) * payload // write + read
    );
}


BENCHMARK(BM_MCQueueWriteRead)
    ->Arg(64)->Arg(256)->Arg(1024)->Arg(4096)->Arg(8096)
    ->ReportAggregatesOnly(true);

