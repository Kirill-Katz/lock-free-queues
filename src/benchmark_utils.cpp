#pragma once

#include <vector>
#include <string>
#include <sys/wait.h>
#include <cstdint>
#include <iostream>
#include <algorithm>
#include <fstream>

pid_t run_perf_report();
pid_t run_perf_stat();
uint64_t calibrate_tsc();
void export_prices_csv(const std::vector<uint32_t>& prices, std::string outdir);

inline uint64_t cycles_to_ns(uint64_t cycles, uint64_t freq) {
    __int128 num = (__int128)cycles * 1'000'000'000 + (freq / 2);
    return (uint64_t)(num / freq);
}


void export_latency_samples_csv(
    std::vector<uint64_t>& samples,
    uint64_t elapsed_cycles,
    const std::string& file_name
) {
    if (samples.empty()) {
        return;
    }

    uint64_t tsc_freq = calibrate_tsc();

    std::sort(samples.begin(), samples.end());

    auto percentile = [&](double p) {
        size_t idx = static_cast<size_t>(p * (samples.size() - 1));
        return samples[idx];
    };

    uint64_t p50  = percentile(0.50);
    uint64_t p95  = percentile(0.95);
    uint64_t p99  = percentile(0.99);
    uint64_t p999 = percentile(0.999);

    std::ofstream out(file_name);
    if (!out) {
        std::abort();
    }

    out << "latency_ns\n";
    for (uint64_t cycles : samples) {
        out << cycles_to_ns(cycles, tsc_freq) << '\n';
    }

    double elapsed_sec = static_cast<double>(elapsed_cycles) /
                         static_cast<double>(tsc_freq);

    double throughput = samples.size() / elapsed_sec;

    std::cout << "samples: " << samples.size() << '\n';
    std::cout << "p50  (ns): " << cycles_to_ns(p50,  tsc_freq) << '\n';
    std::cout << "p95  (ns): " << cycles_to_ns(p95,  tsc_freq) << '\n';
    std::cout << "p99  (ns): " << cycles_to_ns(p99,  tsc_freq) << '\n';
    std::cout << "p999 (ns): " << cycles_to_ns(p999, tsc_freq) << '\n';
    std::cout << "throughput (ops/s): " << throughput << '\n';
}

