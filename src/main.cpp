#include <iostream>
#include <thread>

// message of 73 bytes
// queue is 8MB

#include "spmc_queue_trivially_copiable.hpp"

enum class Side : char {
    None = 0,
    Bid = 'B',
    Ask = 'S'
};

struct BestLvlChange {
    uint64_t qty;
    uint32_t price;
    Side side;
};

void strat_1(SPMCQueue<BestLvlChange>::Consumer& queue) {
    BestLvlChange storage[64];

    while (true) {
        size_t read = queue.consume({storage, 64});
        std::cout << "Read: " << read << '\n';
    }
}

void strat_2(SPMCQueue<BestLvlChange>::Consumer& queue) {
    BestLvlChange storage[64];

    while (true) {
        size_t read = queue.consume({storage, 64});
        std::cout << "Read: " << read << '\n';
    }
}

int main() {
    SPMCQueue<BestLvlChange> spmc_queue;

    auto consumer_strat_1 = spmc_queue.make_consumer();
    auto consumer_strat_2 = spmc_queue.make_consumer();

    std::thread strat_1_thread(strat_1, std::ref(consumer_strat_1));
    std::thread strat_2_thread(strat_2, std::ref(consumer_strat_2));

    strat_1_thread.join();
    strat_2_thread.join();
}
