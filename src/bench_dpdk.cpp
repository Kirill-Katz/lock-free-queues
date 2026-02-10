#include <atomic>
#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <thread>
#include <vector>
#include <x86intrin.h>

#include <rte_eal.h>
#include <rte_errno.h>
#include <rte_ring.h>

#include "benchmark_utils.hpp"

std::atomic<bool> running{true};

constexpr uint64_t samples_count = 2'000'000;

enum class Side : char {
    None = 0,
    Bid = 'B',
    Ask = 'S'
};

struct BestLvlChange {
    uint64_t padding;
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

void consumer(rte_ring* ring, uint64_t consumer_id) {
    pin_thread_to_cpu(static_cast<int>(consumer_id + 2));

    void* obj = nullptr;

    std::vector<uint64_t> samples(samples_count);

    unsigned aux_end;
    uint64_t i = 0;
    while (true && i < samples_count) {
        auto t0 = __rdtscp(&aux_end);
        int rc = rte_ring_dequeue(ring, &obj);
        bool read = (rc == 0);

        if (read) {
            auto t1 = __rdtscp(&aux_end);
            samples[i++] = t1 - t0;
        }

        if (!running.load(std::memory_order_relaxed) && !read) {
            break;
        }
    }

    if (i != samples_count) {
        throw std::runtime_error("failed to process all messages. Only: " + std::to_string(i));
    }

    export_latency_samples_csv(
        samples,
        "../results/consumer_latency_" + std::to_string(consumer_id) + ".csv",
        "consumer_" + std::to_string(consumer_id)
    );
}

int main(int argc, char** argv) {
    pin_thread_to_cpu(1);

    int eal_rc = rte_eal_init(argc, argv);
    if (eal_rc < 0) {
        std::cerr << "rte_eal_init failed: " << rte_strerror(rte_errno) << "\n";
        return 1;
    }

    constexpr unsigned ring_size = 1u << 22;

    rte_ring* ring = rte_ring_create(
        "spmc_ring",
        ring_size,
        rte_socket_id(),
        RING_F_SP_ENQ | RING_F_MC_RTS_DEQ
    );
    if (!ring) {
        std::cerr << "rte_ring_create failed: " << rte_strerror(rte_errno) << "\n";
        return 1;
    }

    auto changes = make_random_changes(samples_count, 1000, 100'000, 10, 100'000);
    constexpr int thread_count = 2;

    std::vector<std::thread> threads;
    threads.reserve(thread_count);

    for (uint64_t i = 0; i < static_cast<uint64_t>(thread_count); ++i) {
        threads.emplace_back(consumer, ring, i);
    }

    std::vector<uint64_t> samples(samples_count);

    unsigned aux_start;
    for (uint64_t i = 0; i < samples_count; ++i) {
        auto t0 = __rdtscp(&aux_start);
        int rc = rte_ring_enqueue(ring, const_cast<BestLvlChange*>(&changes[i]));
        auto t1 = __rdtscp(&aux_start);

        if (rc != 0) {
            std::cerr << "enqueue failed (ring full): rc=" << rc << "\n";
            break;
        }

        for (int k = 0; k < 3; ++k) {
            _mm_pause();
        }

        samples[i] = t1 - t0;
    }

    running.store(false, std::memory_order_release);

    for (auto& t : threads) {
        t.join();
    }

    export_latency_samples_csv(samples, "../results/push.csv", "producer");
    std::cout << "Done\n";
    return 0;
}
