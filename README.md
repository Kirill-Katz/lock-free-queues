# Lock-free queues
A set of lock-free queues I use throughout my projects.
## SPSC Buffer
A simple SPSC ring buffer, the implementation of which can be found in `include/spsc_buffer.hpp` and `src/spsc_buffer.cpp`.
## SPSC IPC Buffer
A simple SPSC ring buffer that can be constructed directly in shared memory to allow for interprocess communication (IPC).
Implementation can be found at `src/spsc_buffer_ipc.cpp` and `include/spsc_buffer_ipc.hpp`.
The data structure logic is identical to the non-IPC version, but the IPC variant embeds its storage so the object can be placed directly in shared memory.
Additional care is required for initialization and process coordination; that is, don't initialize the ring buffer twice.

## SPSC Queue
A simple generic SPSC queue, the implementation of which can be found in `include/spsc_queue.hpp` as a single header file.

### Latency distribution
<img width="3000" height="1800" alt="latency" src="https://github.com/user-attachments/assets/3360ef32-95d3-425b-b460-454533f6175f" />


## SPMC Fan-Out Queue for POD messages (each consumer receives every message)
An SPMC queue with a wait-free producer and lock-free consumers, the implementation of which can be found at `include/spmc_queue_trivially_copiable.hpp` as a single header file.
The queue uses seqlocks to avoid race conditions on slots. More can be found in this [talk](https://youtu.be/8uAW5FQtcvE?t=2325). The general idea is that each slot has a version. If the version of a
slot is odd, it is being written to and is not safe to read from. Otherwise, if the version is even, it is safe to read from. The queue can be used to dispatch order book updates to a set of
strategies running on different threads, and it guarantees that the producer can never be blocked by a consumer, as slow consumers call `std::abort()`.
**This queue is not portable as it contains a small isolated data race which is considered UB by the C++ standard, but from an x86 hardware perspective it is not critical. For more info, check [this talk](https://youtu.be/sX2nF1fW7kI?t=3117), which describes
similar code, but for an entirely different SPMC queue design.**
### SPMC Throughput per consumer count:
<img width="600" height="371" alt="chart" src="https://github.com/user-attachments/assets/2dde2347-43de-43ce-ae53-093faa4a101b" />

### SPMC Latency distribution 1 consumer:
<img width="3000" height="1800" alt="latency" src="https://github.com/user-attachments/assets/4fcd348e-2cad-472b-b0a9-941ccc418304" />


### SPMC Latency distribution 2 consumers:
<img width="3000" height="1800" alt="latency" src="https://github.com/user-attachments/assets/c942e3c0-3876-4e35-8679-fb048f983829" />


### SPMC Latency distribution 3 consumers:
<img width="3000" height="1800" alt="latency" src="https://github.com/user-attachments/assets/4d55b30c-f8fb-4c8d-98fd-dd5de698f1aa" />


# Benchmark methodology
The results were obtained on an i7-12700H CPU with turbo boost on (4.653 GHz peak), Hyper-Threading turned off, and the CPU frequency scaling governor set to performance on an idle machine. The machine is an Asus ROG Zephyrus M16 GU603ZM_GU603ZM. The OS is Ubuntu 24.04.3 LTS with an unmodified Linux 6.14.0-37-generic kernel. The code was compiled with g++ 13.3.0 using the `-DNDEBUG -O3 -march=native` flags. Latency was measured using the `rdtscp` instruction and then converted into ns by estimating the frequency of `rdtscp`. The results were obtained using 16-byte structs passed between threads through the queues. The `std::thread`s were pinned to physical cores using the `pthread_setaffinity_np()` function.
