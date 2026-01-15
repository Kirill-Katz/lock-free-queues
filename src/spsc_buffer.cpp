#include "spsc_buffer.hpp"
#include <atomic>
#include <cstring>
#include <algorithm>

size_t SPSCBuffer::available(size_t writer, size_t reader) const {
    if (reader > writer) {
        return (bufferSize_ - reader) + writer;
    } else {
        return writer - reader;
    }
}

size_t SPSCBuffer::read(std::span<std::byte> dst) {
    size_t reader = reader_.load(std::memory_order_acquire);
    size_t writer = writer_.load(std::memory_order_acquire);

    size_t avail = available(writer, reader);
    if (!avail) {
        return 0;
    }

    size_t n = std::min({ avail, dst.size_bytes() });
    size_t spaceToEnd = bufferSize_ - reader;

    if (n <= spaceToEnd) {
        std::memcpy(dst.data(), buffer_.get() + reader, n);
        size_t newReader = reader + n;
        if (newReader == bufferSize_) {
            newReader = 0;
        }

        reader_.store(newReader, std::memory_order_release);
    } else {
        size_t firstLen = spaceToEnd;
        size_t secondLen = n - firstLen;

        std::memcpy(dst.data(), buffer_.get() + reader, firstLen);
        std::memcpy(dst.data() + firstLen, buffer_.get(), secondLen);

        reader_.store(secondLen, std::memory_order_release);
    }

    return n;
}

bool SPSCBuffer::try_write(std::span<const std::byte> data) {
    size_t writer = writer_.load(std::memory_order_acquire);
    size_t reader = reader_.load(std::memory_order_acquire);

    size_t freeSpace = bufferSize_ - 1 - available(writer, reader);
    if (freeSpace < data.size_bytes()) {
        return false;
    }

    if (writer + data.size_bytes() < bufferSize_) {
        std::memcpy(buffer_.get() + writer, data.data(), data.size_bytes());

        size_t newWriter = writer + data.size_bytes();
        writer_.store(newWriter, std::memory_order_release);
    } else {
        size_t firstPart = bufferSize_ - writer;

        std::memcpy(buffer_.get() + writer, data.data(), bufferSize_ - writer);
        std::memcpy(buffer_.get(), data.data() + firstPart, data.size_bytes() - firstPart);

        size_t newWriter = data.size_bytes() - firstPart;
        writer_.store(newWriter, std::memory_order_release);
    }

    return true;
}
