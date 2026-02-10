# Lock free queues
A set of lock free queues I use throughout my projects.
## SPSC Buffer
A simple SPSC Ring Buffer the implementation of which can be found in `include/spsc_buffer.hpp` and `src/spsc_buffer.cpp
`.
## SPSC IPC Buffer
A simple SPSC Ring Buffer which can be constructed directly in shared memory to allow for interprocess communication (IPC).
Implementation can be found at `src/spsc_buffer_ips.cpp` and `include/spsc_buffer_ipc.hpp`.
The data structure logic is identical to the non-IPC version, but the IPC variant embeds its storage so the object can be placed directly in shared memory.
Additional care is required for initialization and process coordination, i.e don't initialize the ring buffer twice.

## SPSC Queue
A simple generic SPSC Queue the implementation of which can be found in `include/spsc_queue.hpp` as a single header file.

## SPMC Queue for POD messages
An SPMC queue with a wait-free producer and lock-free consumers the implementation of which can be found at `include/spmc_queue_trivially_copiable.hpp` as a single header file.
The queue uses seq locks to avoid race conditions on slots, more can be found in this [talk](https://youtu.be/8uAW5FQtcvE?t=2325). The general idea is that each slot has a version and if the version of a
slot is odd it is being written to so it is not safe to read from, otherwise if the version is even then it is safe to read from. The queue can be used to dispatch order book updates to a set of
strategies running on different threads, and it guarantees that the producer can never be blocked by a consumer, as slow consumer std::abort().
**This queue is not portable as it contains a small isolated data race which is considered UB by the C++ standard, but from x86 hardware perspective it is not critical. For more info check [this talk](https://youtu.be/sX2nF1fW7kI?t=3117), which describes
similar code, but for an entirely different SPMC queue design.**
### SPMC Throughput per consumer count: 
<img width="611" height="378" alt="chart" src="https://github.com/user-attachments/assets/aac88471-7ffe-4417-bfa8-921da86a8747" />

### SPMC Latency distribution 1 consumer: 
<img width="3000" height="1800" alt="latency" src="https://github.com/user-attachments/assets/4fcd348e-2cad-472b-b0a9-941ccc418304" />


### SPMC Latency distribution 2 consumers: 
<img width="3000" height="1800" alt="latency" src="https://github.com/user-attachments/assets/c942e3c0-3876-4e35-8679-fb048f983829" />


### SPMC Latency distribution 3 consumers: 
<img width="3000" height="1800" alt="latency" src="https://github.com/user-attachments/assets/4d55b30c-f8fb-4c8d-98fd-dd5de698f1aa" />
