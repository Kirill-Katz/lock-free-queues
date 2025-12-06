#include "spsc_buffer.hpp"
#include <cstring>

size_t SPSCBuffer::available() const {
    if (reader_ > writer_) {
        return (bufferSize_ - reader_) + writer_;
    } else {
        return writer_ - reader_;
    }
}

ReadView SPSCBuffer::read(size_t n) {
    size_t avail = available();
    if (reader_ == writer_) {
        return { {}, {} };
    }

    if (n > avail) {
        n = avail;
    }

    size_t spaceToEnd = bufferSize_ - reader_;

    ReadView readView;
    if (n <= spaceToEnd) {
        readView = ReadView{
            std::span<const std::byte>(buffer_.get() + reader_, n),
            {}
        };

        reader_ += n;
        if (reader_ == bufferSize_) {
            reader_ = 0;
        }
    } else {
        size_t firstLen = spaceToEnd;
        size_t secondLen = n - firstLen;

        readView = ReadView{
            std::span<const std::byte>(buffer_.get() + reader_, firstLen),
            std::span<const std::byte>(buffer_.get(), secondLen),
        };

        reader_ = secondLen;
    }

    return readView;
}

bool SPSCBuffer::write(std::span<const std::byte> data) {
    size_t freeSpace = bufferSize_ - 1 - available();
    if (freeSpace < data.size_bytes()) {
        return false;
    }

    if (writer_ + data.size_bytes() < bufferSize_) {
        std::memcpy(buffer_.get() + writer_, data.data(), data.size_bytes());
        writer_ += data.size_bytes();
    } else {
        size_t firstPart = bufferSize_ - writer_;

        std::memcpy(buffer_.get() + writer_, data.data(), bufferSize_ - writer_);
        std::memcpy(buffer_.get(), data.data() + firstPart, data.size_bytes() - firstPart);

        writer_ = data.size_bytes() - firstPart;
    }

    return true;
}
