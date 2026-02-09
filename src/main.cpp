#include <atomic>
#include <iostream>
#include <thread>
#include <vector>
#include <random>
#include <x86intrin.h>
#include <absl/container/flat_hash_map.h>
#include "benchmark_utils.hpp"
#include "spmc_queue_trivially_copiable.hpp"

std::atomic running{true};

constexpr uint64_t samples_count = 100'000;

enum class Side : char {
    None = 0,
    Bid = 'B',
    Ask = 'S'
};

struct BestLvlChange {
    uint64_t tsc;
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

    std::vector<BestLvlChange> v;
    v.reserve(n);

    for (std::size_t i = 0; i < n; ++i) {
        BestLvlChange c;
        c.price = price_dist(rng);
        c.qty = qty_dist(rng);
        c.side = side_dist(rng) == 0 ? Side::Bid : Side::Ask;
        v.push_back(c);
    }

    return v;
}

void consumer(SPMCQueue<BestLvlChange>::Consumer& queue, const std::string& consumer) {
    BestLvlChange best_lvl_change;

    std::vector<uint64_t> samples(samples_count);

    unsigned aux_end;
    int i = 0;
    while (true) {
        bool read = queue.pop(best_lvl_change);
        auto t0 = best_lvl_change.tsc;

        if (read) {
            auto t1 = __rdtscp(&aux_end);
            samples[i++] = t1 - t0;
        }

        if (!running.load(std::memory_order_relaxed) && !read) break;
    }

    export_latency_samples_csv(samples, "../results/consumer_latency_" + consumer + ".csv");
}

void sleep_ns(long ns) {
    timespec ts;
    ts.tv_sec  = ns / 1'000'000'000L;
    ts.tv_nsec = ns % 1'000'000'000L;
    nanosleep(&ts, nullptr);
}

int main() {
    SPMCQueue<BestLvlChange> spmc_queue;

    auto changes = make_random_changes(samples_count, 1000, 100'000, 10, 100'000);

    constexpr int thread_count = 3;

    std::vector<std::thread> threads;
    std::vector<SPMCQueue<BestLvlChange>::Consumer> qconsumers;
    qconsumers.reserve(thread_count);

    for (int i = 0; i < thread_count; ++i) {
        qconsumers.push_back(spmc_queue.make_consumer());

        std::thread cthread(consumer, std::ref(qconsumers[i]), std::to_string(i));
        threads.push_back(std::move(cthread));
    }

    unsigned aux_start;

    std::vector<uint64_t> samples(samples_count);
    int i = 0;
    for (auto& change : changes) {
        change.tsc = __rdtscp(&aux_start);

        auto t0 = __rdtscp(&aux_start);
        spmc_queue.push(change);
        auto t1 = __rdtscp(&aux_start);

        samples[i] = t1 - t0;
        sleep_ns(1);
        ++i;
    }

    running.store(false, std::memory_order_release);

    for (int i = 0; i < thread_count; ++i) {
        threads[i].join();
    }

    export_latency_samples_csv(samples, "../results/push.csv");
    std::cout << "Done" << '\n';
}
