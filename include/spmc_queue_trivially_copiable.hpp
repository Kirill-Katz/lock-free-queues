#pragma once

#include <atomic>
#include <memory>
#include <iostream>
#include <cstring>

template<typename T>
concept QueueMsg =
    std::is_trivially_copyable_v<T> &&
    std::is_trivially_destructible_v<T>;

template<QueueMsg T>
class SPMCQueue {
public:
    SPMCQueue() {};
    SPMCQueue(const SPMCQueue&) = delete;
    SPMCQueue(SPMCQueue&&) = delete;

    SPMCQueue& operator=(const SPMCQueue&) = delete;
    SPMCQueue& operator=(SPMCQueue&&) = delete;

    struct Slot {
        std::atomic<uint64_t> version{0};
        T data;
    };

    class Consumer {
        public:
            bool pop(T& dst);

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

    writer.store(writer + 1, std::memory_order_release);
}

template<QueueMsg T>
bool SPMCQueue<T>::Consumer::pop(T& dst) {
    size_t r = reader.load(std::memory_order_acquire);
    size_t w = queue.writer.load(std::memory_order_acquire);

    if (w - r > buffer_size) {
        std::cerr << "Consumer too slow";
        std::abort();
    }

    if (w == r) {
        return false;
    }

    size_t r_idx = (r & wrap_mask);
    auto v1 = queue.buffer[r_idx].version.load(std::memory_order_acquire);
    if (v1 & 1LL) {
        return false;
    }

    T temp = queue.buffer[r_idx];

    auto v2 = queue.buffer[r_idx].version.load(std::memory_order_acquire);
    if (v1 != v2) {
        return false;
    }

    dst = temp;
    reader.store(r + 1, std::memory_order_release);
    return true;
}
