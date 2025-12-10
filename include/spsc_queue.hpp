#pragma once

#include <memory>
#include <atomic>
#include <assert.h>
#include <cstring>

static constexpr size_t slotSize_ = 64;
struct Slot {
    alignas(64) std::byte storage[64];
};

template<typename T>
class SPSCQueue {
public:
    SPSCQueue() {};

    SPSCQueue(const SPSCQueue& q) = delete;
    SPSCQueue(SPSCQueue&& q) = delete;

    SPSCQueue& operator=(const SPSCQueue& q) = delete;
    SPSCQueue& operator=(SPSCQueue&& q) = delete;

    bool try_pop(T& dst);
    bool try_push(const T& data);
    bool try_push(T&& data);
    size_t available(size_t writer, size_t reader) const;

private:
    static constexpr size_t bufferSizeBytes_ = 8 * 1024 * 1024;
    static constexpr size_t bufferSizeSlots_ = bufferSizeBytes_ / 64;
    std::unique_ptr<Slot[]> buffer_ = std::make_unique<Slot[]>(bufferSizeSlots_);

    std::atomic<size_t> reader_ = 0;
    std::atomic<size_t> writer_ = 0;
};

template<typename T>
size_t SPSCQueue<T>::available(size_t writer, size_t reader) const {
    return (writer - reader) & (bufferSizeSlots_ - 1);
}

template<typename T>
bool SPSCQueue<T>::try_push(const T& data) {
    assert(sizeof(T) <= 64);
    assert(alignof(T) <= alignof(Slot));

    size_t writer = writer_.load(std::memory_order_acquire);
    size_t reader = reader_.load(std::memory_order_acquire);

    size_t avail = available(writer, reader);
    size_t freeSpace = bufferSizeSlots_ - 1 - avail;
    if (freeSpace < 1) {
        return false;
    }

    Slot& s = buffer_[writer];
    if constexpr (std::is_trivially_copyable_v<T>) {
        std::memcpy(s.storage, &data, sizeof(T));
    } else {
        new (s.storage) T(data);
    }

    writer_.store((writer + 1) & (bufferSizeSlots_ - 1), std::memory_order_release);
    return true;
}

template<typename T>
bool SPSCQueue<T>::try_push(T&& data) {
    static_assert(sizeof(T) <= 64);
    static_assert(alignof(T) <= alignof(Slot));

    size_t writer = writer_.load(std::memory_order_acquire);
    size_t reader = reader_.load(std::memory_order_acquire);

    size_t avail = available(writer, reader);
    size_t freeSpace = bufferSizeSlots_ - 1 - avail;
    if (freeSpace < 1) {
        return false;
    }

    Slot& s = buffer_[writer];
    if constexpr (std::is_trivially_copyable_v<T>) {
        std::memcpy(s.storage, &data, sizeof(T));
    } else {
        new (s.storage) T(std::move(data));
    }

    writer_.store((writer + 1) & (bufferSizeSlots_ - 1), std::memory_order_release);
    return true;
}

template<typename T>
bool SPSCQueue<T>::try_pop(T& dst) {
    static_assert(sizeof(T) <= 64);
    static_assert(alignof(T) <= alignof(Slot));

    size_t reader = reader_.load(std::memory_order_acquire);
    size_t writer = writer_.load(std::memory_order_acquire);

    if (reader == writer) {
        return false;
    }

    Slot& s = buffer_[reader];
    if constexpr (std::is_trivially_copyable_v<T>) {
        std::memcpy(&dst, s.storage, sizeof(T));
    } else {
        T* ptr = std::launder(reinterpret_cast<T*>(s.storage));
        dst = std::move(*ptr);
        ptr->~T();
    }

    reader_.store((reader + 1) & (bufferSizeSlots_ - 1), std::memory_order_release);
    return true;
}


