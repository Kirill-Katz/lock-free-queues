#pragma once

#include <atomic>
#include <memory>
#include <iostream>
#include <cstring>

template<typename T>
concept QueueMsg =
    std::is_trivially_copyable_v<T> &&
    std::is_trivially_destructible_v<T>;

// no micro op cache polution
[[gnu::noinline]] inline void UNEXPECTED(bool condition) {
    if (condition) {
        std::cerr << "Consumer is too slow, aborting now!" << '\n';
        std::abort();
    }
}

template<QueueMsg T>
class SPMCQueue {
public:
    SPMCQueue() {};
    SPMCQueue(const SPMCQueue&) = delete;
    SPMCQueue(SPMCQueue&&) = delete;

    SPMCQueue& operator=(const SPMCQueue&) = delete;
    SPMCQueue& operator=(SPMCQueue&&) = delete;

    using VersionT = uint64_t;
    struct Slot {
        std::atomic<VersionT> version{0};
        T data;
    };

    class Consumer {
        public:
            bool pop(T& dst);

        private:
            friend class SPMCQueue;
            Consumer(SPMCQueue& q)
            : queue{q},
              reader{q.writer.load(std::memory_order_acquire)}
            {}

            Consumer(const Consumer&) = delete;
            Consumer(Consumer&) = delete;

            Consumer& operator=(const Consumer&) = delete;
            Consumer& operator=(Consumer&&) = delete;

            size_t reader{0};

            SPMCQueue& queue;
    };

    void push(const T&);

    Consumer make_consumer() {
        return Consumer{*this};
    }

private:
    constexpr static size_t buffer_size {4 * 1024};
    constexpr static size_t wrap_mask {buffer_size - 1};

    alignas(64) std::atomic<size_t> writer{0};
    std::unique_ptr<Slot[]> buffer = std::make_unique<Slot[]>(buffer_size);

    static_assert(std::popcount(buffer_size) == 1);
};

template<QueueMsg T>
void SPMCQueue<T>::push(const T& val) {
    size_t w = writer.load(std::memory_order_acquire);
    auto& slot = buffer[w & wrap_mask];

    slot.version.fetch_add(1, std::memory_order_release);
    slot.data = val;
    slot.version.fetch_add(1, std::memory_order_release);

    writer.fetch_add(1, std::memory_order_release);
}

template<QueueMsg T>
bool SPMCQueue<T>::Consumer::pop(T& dst) {
    size_t r_idx = (reader & wrap_mask);
    size_t gen = reader >> std::countr_zero(buffer_size);

    size_t expected_version = 2 * (gen + 1);

    auto& slot = queue.buffer[r_idx];
    auto v1 = slot.version.load(std::memory_order_acquire);

    UNEXPECTED(v1 > expected_version);
    if (v1 & 1) {
        return false;
    }

    // this is UB, because it is a data-race
    // this is deliberate and it is not portable, but it does works on x86
    T temp = queue.buffer[r_idx].data;

    auto v2 = queue.buffer[r_idx].version.load(std::memory_order_acquire);
    UNEXPECTED(v2 != expected_version);

    dst = temp;
    reader++;

    return true;
}
