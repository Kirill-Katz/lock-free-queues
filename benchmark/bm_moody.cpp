#include <benchmark/benchmark.h>
#include "moodycammel_queue.hpp"
#include <vector>
#include <cstddef>

static void BM_MCQueueWrite(benchmark::State& state) {
    const size_t payload = state.range(0);

    std::vector<std::byte> data(payload);
    benchmark::DoNotOptimize(data);

    moodycamel::ReaderWriterQueue<void*> q(1024);
    benchmark::DoNotOptimize(q);

    void* ptr = data.data();

    for (auto _ : state) {
        bool ok = q.try_enqueue(ptr);
        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * payload);
}

static void BM_MCQueueRead(benchmark::State& state) {
    const size_t payload = state.range(0);

    std::vector<std::byte> data(payload);
    benchmark::DoNotOptimize(data);

    moodycamel::ReaderWriterQueue<void*> q(1024);
    benchmark::DoNotOptimize(q);

    void* ptr = data.data();

    for (auto _ : state) {
        bool ok = q.try_enqueue(ptr);

        void* out;
        bool ok2 = q.try_dequeue(out);
        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * payload);
}

BENCHMARK(BM_MCQueueWrite)
    ->Arg(64)->Arg(256)->Arg(1024)->Arg(4096)
    ->ReportAggregatesOnly(true);

BENCHMARK(BM_MCQueueRead)
    ->Arg(64)->Arg(256)->Arg(1024)->Arg(4096)
    ->ReportAggregatesOnly(true);

