#pragma once
#include <span>
#include <atomic>

struct ReadView {
    std::span<const std::byte> first;
    std::span<const std::byte> second;
};

class IPCSPSCBuffer {
public:
    IPCSPSCBuffer() {};

    IPCSPSCBuffer(const IPCSPSCBuffer& q) = delete;
    IPCSPSCBuffer(IPCSPSCBuffer&& q) = delete;

    IPCSPSCBuffer& operator=(const IPCSPSCBuffer& q) = delete;
    IPCSPSCBuffer& operator=(IPCSPSCBuffer&& q) = delete;

    void read(std::span<std::byte> dst, size_t n);
    bool try_write(std::span<const std::byte> data);
    size_t available(size_t writer, size_t reader) const;

private:
    static constexpr size_t bufferSize_ = 8 * 1024 * 1024;
    alignas(64) std::byte buffer_[bufferSize_];
    alignas(64) std::atomic<uint64_t> reader_ = 0;
    alignas(64) std::atomic<uint64_t> writer_ = 0;
};
