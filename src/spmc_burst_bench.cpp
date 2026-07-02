#include "spmc_burst_bench.hpp"

#include <algorithm>
#include <barrier>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <vector>
#include <x86intrin.h>

#include "benchmark_utils.hpp"
#include "spmc_queue_trivially_copiable.hpp"

namespace {

constexpr std::size_t kTotalMessages = 2'000'000;
constexpr std::size_t kBurstSize = 655'360;

enum class Side : char {
    None = 0,
    Bid = 'B',
    Ask = 'S'
};

struct BestLvlChange {
    uint64_t qty;
    uint32_t price;
    Side side;
};

std::vector<BestLvlChange> make_random_changes(
    std::size_t n,
    uint32_t min_price,
    uint32_t max_price,
    uint64_t min_qty,
    uint64_t max_qty,
    uint64_t seed = 123456789
) {
    std::mt19937_64 rng(seed);

    std::uniform_int_distribution<uint32_t> price_dist(min_price, max_price);
    std::uniform_int_distribution<uint64_t> qty_dist(min_qty, max_qty);
    std::uniform_int_distribution<int> side_dist(0, 1);

    std::vector<BestLvlChange> values;
    values.reserve(n);

    for (std::size_t i = 0; i < n; ++i) {
        BestLvlChange change;
        change.price = price_dist(rng);
        change.qty = qty_dist(rng);
        change.side = side_dist(rng) == 0 ? Side::Bid : Side::Ask;
        values.push_back(change);
    }

    return values;
}

struct EpochMetrics {
    std::size_t messages;
    uint64_t processing_cycles;
};

void export_epoch_metrics_csv(
    const std::vector<EpochMetrics>& metrics,
    const std::string& file_name
) {
    std::ofstream out(file_name);
    if (!out) {
        std::abort();
    }

    const uint64_t tsc_freq = calibrate_tsc();

    out << "epoch,messages,processing_cycles,processing_ns,processing_ops_per_s\n";

    for (std::size_t epoch = 0; epoch < metrics.size(); ++epoch) {
        const auto& metric = metrics[epoch];
        const double processing_sec =
            static_cast<double>(metric.processing_cycles) / static_cast<double>(tsc_freq);

        out << epoch
            << ',' << metric.messages
            << ',' << metric.processing_cycles
            << ',' << cycles_to_ns(metric.processing_cycles, tsc_freq)
            << ',' << (static_cast<double>(metric.messages) / processing_sec)
            << '\n';
    }
}

}  // namespace

void run_spmc_burst_bench(int consumer_count) {
    if (consumer_count <= 0) {
        throw std::invalid_argument("consumer_count must be positive");
    }

    SPMCQueue<BestLvlChange> queue;
    auto changes = make_random_changes(kTotalMessages, 1000, 100'000, 10, 100'000);

    const std::size_t epoch_count = (kTotalMessages + kBurstSize - 1) / kBurstSize;
    std::vector<EpochMetrics> metrics(epoch_count);
    std::vector<uint64_t> consumer_done_cycles(consumer_count, 0);

    std::barrier epoch_start{consumer_count + 1};
    std::barrier epoch_end{consumer_count + 1};

    std::vector<SPMCQueue<BestLvlChange>::Consumer> consumers;
    consumers.reserve(consumer_count);
    for (int i = 0; i < consumer_count; ++i) {
        consumers.push_back(queue.make_consumer());
    }

    std::vector<std::thread> threads;
    threads.reserve(consumer_count);

    for (int consumer_index = 0; consumer_index < consumer_count; ++consumer_index) {
        threads.emplace_back([&, consumer_index]() {
            pin_thread_to_cpu(consumer_index + 2);

            BestLvlChange value;

            for (std::size_t epoch = 0; epoch < epoch_count; ++epoch) {
                const std::size_t start = epoch * kBurstSize;
                const std::size_t count = std::min(kBurstSize, kTotalMessages - start);

                epoch_start.arrive_and_wait();

                for (std::size_t i = 0; i < count; ++i) {
                    while (!consumers[consumer_index].pop(value)) {
                        _mm_pause();
                    }
                }

                unsigned aux;
                consumer_done_cycles[consumer_index] = __rdtscp(&aux);
                epoch_end.arrive_and_wait();
            }
        });
    }

    pin_thread_to_cpu(1);

    unsigned aux;
    for (std::size_t epoch = 0; epoch < epoch_count; ++epoch) {
        const std::size_t start = epoch * kBurstSize;
        const std::size_t count = std::min(kBurstSize, kTotalMessages - start);

        epoch_start.arrive_and_wait();

        const uint64_t producer_start = __rdtscp(&aux);
        for (std::size_t i = 0; i < count; ++i) {
            queue.push(changes[start + i]);
        }
        epoch_end.arrive_and_wait();

        const uint64_t slowest_consumer_done =
            *std::max_element(consumer_done_cycles.begin(), consumer_done_cycles.end());

        metrics[epoch] = EpochMetrics{
            .messages = count,
            .processing_cycles = slowest_consumer_done - producer_start,
        };
    }

    for (auto& thread : threads) {
        thread.join();
    }

    export_epoch_metrics_csv(metrics, "../results/spmc_burst_epochs.csv");

    const uint64_t total_processing_cycles = std::accumulate(
        metrics.begin(), metrics.end(), uint64_t{0},
        [](uint64_t sum, const EpochMetrics& metric) {
            return sum + metric.processing_cycles;
        }
    );
    const uint64_t tsc_freq = calibrate_tsc();

    std::cout << "spmc_burst_bench\n";
    std::cout << "consumers: " << consumer_count << '\n';
    std::cout << "messages per epoch: " << kBurstSize << '\n';
    std::cout << "epochs: " << epoch_count << '\n';
    std::cout << "processing throughput (ops/s): "
              << (static_cast<double>(kTotalMessages) * static_cast<double>(tsc_freq) /
                  static_cast<double>(total_processing_cycles))
              << '\n';
    std::cout << "processing time per burst (cycles, summed): "
              << total_processing_cycles
              << '\n';
}
