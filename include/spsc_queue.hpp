#pragma once

#include <memory>

template<typename T>
class SPSCQueue {
public:
    SPSCQueue() {};

    SPSCQueue(const SPSCQueue& q) = delete;
    SPSCQueue(SPSCQueue&& q) = delete;

    SPSCQueue& operator=(const SPSCQueue& q) = delete;
    SPSCQueue& operator=(SPSCQueue&& q) = delete;

private:
    size_t reader_ = 0;
    size_t writer_ = 0;

};
