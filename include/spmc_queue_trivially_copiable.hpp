#pragma once

#include <atomic>
#include <memory>
#include <span>
#include <iostream>
#include <cstring>

template<typename T>
class SPMCQueue {
public:
    SPMCQueue() {};
    SPMCQueue(const SPMCQueue&) = delete;
    SPMCQueue(SPMCQueue&&) = delete;

    SPMCQueue& operator=(const SPMCQueue&) = delete;
    SPMCQueue& operator=(SPMCQueue&&) = delete;

    class Consumer {
        public:
            size_t consume(std::span<T> dst);

        private:
            friend class SPMCQueue;
            Consumer(SPMCQueue& q) : queue{q} {}

            Consumer(const Consumer&) = delete;
            Consumer(Consumer&) = delete;

            Consumer& operator=(const Consumer&) = delete;
            Consumer& operator=(Consumer&&) = delete;

            alignas(64) std::atomic<size_t> reader{0};
            SPMCQueue& queue;
    };

    void push(const T&);

    Consumer make_consumer() {
        return Consumer{*this};
    }

private:
    constexpr static size_t buffer_size{4 * 1024};
    constexpr static size_t wrap_mask{buffer_size - 1};

    alignas(64) std::atomic<size_t> writer{0};
    std::unique_ptr<T[]> buffer = std::make_unique<T[]>(buffer_size);

    static_assert(std::popcount(buffer_size) == 1);
    static_assert(std::is_trivially_destructible_v<T>);
};

template<typename T>
void SPMCQueue<T>::push(const T& val) {
    size_t w = writer.load(std::memory_order_acquire);

    T& s = buffer[writer & wrap_mask];
    std::memcpy(s, &val, sizeof(T));

    writer.store(writer + 1, std::memory_order_release);
}

template<typename T>
size_t SPMCQueue<T>::Consumer::consume(std::span<T> dst) {
    size_t r = reader.load(std::memory_order_acquire);
    size_t w = queue.writer.load(std::memory_order_acquire);

    if (w - r > buffer_size) {
        std::cerr << "Consumer too slow";
        std::abort();
    }

    size_t available = w - r;
    size_t n = std::min(available, dst.size());

    size_t r_idx = (r & wrap_mask);
    for (size_t i = 0; i < n; ++i) {
        dst[i] = queue.buffer[r_idx + i];
    }

    r = reader.load(std::memory_order_acquire);
    if (w - r > buffer_size) {
        std::cerr << "Consumer too slow";
        std::abort();
    }

    reader.store(r + n, std::memory_order_release);
    return n;
}
