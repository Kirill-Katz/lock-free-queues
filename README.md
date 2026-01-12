# Lock free queues
For now it includes 3 SPSC queues implementations.
### SPSC Buffer
A simple SPSC Ring Buffer the implementation of which can be found in `include/spsc_buffer.hpp` and `src/spsc_buffer.cpp
`.
## SPSC IPC Buffer
A simple SPSC Ring Buffer which can be constructed directly in shared memory to allow for interprocess communication (IPC).
Implementation can be found at `src/spsc_buffer_ips.cpp` and `include/spsc_buffer_ipc.hpp`.
The data structure logic is identical to the non-IPC version, but the IPC variant embeds its storage so the object can be placed directly in shared memory.
Additional care is required for initialization and process coordination, i.e don't initialize the ring buffer twice.

### SPSC Queue
A simple generic SPSC Queue the implementation of which can be found in `include/spsc_queue.hpp` as a single header file.
