#pragma once
#include <atomic>
#include <memory>
#include <span>

static constexpr size_t slotSize_ = 64;
struct Slot {
    alignas(64) std::byte storage[64];
};

template<typename T>
class SPMCQueue {
public:
    size_t consume_from(std::span<T> dst);

private:
    SPMCQueue(const SPMCQueue&) = delete;
    SPMCQueue(SPMCQueue&&) = delete;

    SPMCQueue& operator=(const SPMCQueue&) = delete;
    SPMCQueue& operator=(SPMCQueue&&) = delete;

    alignas(64) std::atomic<size_t> writer = 0;

    constexpr static size_t buffer_size_bytes = 8 * 1024 * 1024;
    constexpr static size_t buffer_size_slots = buffer_size_bytes / sizeof(Slot);
    std::unique_ptr<Slot[]> buffer = std::make_unique<Slot[]>(buffer_size_slots);

    static_assert(std::popcount(buffer_size_slots) == 1);
};

template<typename T>
class Consumer {
    Consumer(SPMCQueue<T>& queue_)
    : queue{queue_}
    {}

private:
    friend class SPMCQueue<T>;

    alignas(64) SPMCQueue<T>& queue;
    std::atomic<size_t> reader = 0;
};
