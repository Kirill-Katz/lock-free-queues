#pragma once
#include <span>
#include <memory>
#include <atomic>

class SPSCBuffer {
public:
    SPSCBuffer() {};

    SPSCBuffer(const SPSCBuffer& q) = delete;
    SPSCBuffer(SPSCBuffer&& q) = delete;

    SPSCBuffer& operator=(const SPSCBuffer& q) = delete;
    SPSCBuffer& operator=(SPSCBuffer&& q) = delete;

    size_t read(std::span<std::byte> dst);
    bool try_write(std::span<const std::byte> data);
    size_t available(size_t writer, size_t reader) const;

private:
    static constexpr size_t bufferSize_ = 8 * 1024 * 1024;
    std::unique_ptr<std::byte[]> buffer_ = std::make_unique<std::byte[]>(bufferSize_);
    alignas(64) std::atomic<size_t> reader_ = 0;
    alignas(64) std::atomic<size_t> writer_ = 0;
};
