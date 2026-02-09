#include <benchmark/benchmark.h>
#include "spsc_buffer.hpp"

//static void BM_BufferWriteRead(benchmark::State& state) {
//    const size_t payload = state.range(0);
//
//    std::vector<std::byte> data(payload, std::byte(10));
//    benchmark::DoNotOptimize(data);
//
//    std::vector<std::byte> dst(payload, std::byte(10));
//    benchmark::DoNotOptimize(dst);
//
//    SPSCBuffer q;
//    benchmark::DoNotOptimize(q);
//
//    for (auto _ : state) {
//        q.try_write(std::span<const std::byte>(data.data(), data.size()));
//        benchmark::ClobberMemory();
//
//        q.read(dst, data.size());
//        benchmark::ClobberMemory();
//    }
//
//    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * payload);
//}
//
//BENCHMARK(BM_BufferWriteRead)
//    ->Arg(64)->Arg(256)->Arg(1024)->Arg(4096)->Arg(8096)
//    ->ReportAggregatesOnly(true);
