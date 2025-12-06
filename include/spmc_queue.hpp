#pragma once
#include <atomic>
#include <span>
#include <memory>

class SPMCQueue {
public:
    SPMCQueue(size_t bufferSize):
        bufferSize_(),
        buffer_(std::make_unique<std::byte[]>(bufferSize))
    {}

    void write(std::span<std::byte> buffer) {


    }

    int32_t try_read(std::span<std::byte> buffer) {

    }

private:
    size_t bufferSize_;


    alignas(64) std::atomic<uint64_t> readCounter_{0};
    alignas(64) std::atomic<uint64_t> writeCounter_{0};
    alignas(64) std::unique_ptr<std::byte[]> buffer_;

};

struct QConsumer {
    SPMCQueue& queue;
};

struct QProducer {
    SPMCQueue& queue;
};
