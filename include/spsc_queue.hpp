#pragma once
//#include <atomic>
#include <span>
#include <memory>


struct ReadView {
    std::span<const std::byte> first;
    std::span<const std::byte> second;
};

class SPSCQueue {
public:
    SPSCQueue() {};

    SPSCQueue(const SPSCQueue& q) = delete;
    SPSCQueue(SPSCQueue&& q) = delete;

    SPSCQueue& operator=(const SPSCQueue& q) = delete;
    SPSCQueue& operator=(SPSCQueue&& q) = delete;

    ReadView read(size_t n);
    bool write(std::span<const std::byte> data);
    size_t available() const;

private:
    static constexpr size_t bufferSize_ = 8 * 1024 * 1024;
    std::unique_ptr<std::byte[]> buffer_ = std::make_unique<std::byte[]>(bufferSize_);
    size_t reader_ = 0;
    size_t writer_ = 0;
};


