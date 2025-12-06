#pragma once
#include <span>
#include <memory>

struct ReadView {
    std::span<const std::byte> first;
    std::span<const std::byte> second;
};

class SPSCBuffer {
public:
    SPSCBuffer() {};

    SPSCBuffer(const SPSCBuffer& q) = delete;
    SPSCBuffer(SPSCBuffer&& q) = delete;

    SPSCBuffer& operator=(const SPSCBuffer& q) = delete;
    SPSCBuffer& operator=(SPSCBuffer&& q) = delete;

    ReadView read(size_t n);
    bool write(std::span<const std::byte> data);
    size_t available() const;

private:
    static constexpr size_t bufferSize_ = 8 * 1024 * 1024;
    std::unique_ptr<std::byte[]> buffer_ = std::make_unique<std::byte[]>(bufferSize_);
    size_t reader_ = 0;
    size_t writer_ = 0;
};


