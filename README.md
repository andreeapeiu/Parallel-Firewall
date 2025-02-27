# Parallel-Firewall

## Objectives
- Design and implement parallel programs using the POSIX threading API.
- Convert a serial firewall program into a parallel one.
- Gain experience with concurrency, synchronization, and thread-safe data structures.

## Core Concepts
1. **Firewall Threads**  
   - A **producer** thread creates packets (with source, destination, timestamp, and payload) and places them into a **shared ring buffer**.
   - **Consumer** threads pick packets from the ring buffer, apply the firewall filter, and log the results (`PASS` or `DROP`) in a file.

2. **Ring Buffer (Circular Buffer)**  
   - Used for **thread-safe** packet storage.
   - Avoids moving elements by using `write_pos` and `read_pos` pointers.
   - Must use proper **synchronization primitives** (mutexes, condition variables, semaphores, etc.)—**no busy waiting** allowed.

3. **Logging Requirements**
   - Log entries must be **in ascending order of packet timestamp**.
   - Logging must be done **during** packet processing rather than after all threads finish.

## Implementation Details
- **Producer** inserts new packets into the ring buffer until all are generated, then signals consumers to stop.
- **Consumers** retrieve packets from the shared buffer, check them against the firewall filters, compute a hash, and write the decision (`PASS` or `DROP`) along with the packet timestamp to the log file.
- **Synchronize** producer and consumers using condition variables or semaphores to ensure no busy waiting.

## Restrictions
- Must not use **busy waiting**: threads should block until new data is available.
- **Log order** must be maintained by timestamp during processing, not after-the-fact sorting.
- The number of active threads ≥ `num_consumers + 1` (producer + all consumers).

## Grading Breakdown
- **10 points**: Single-consumer solution with ring buffer.
- **50 points**: Multi-consumer solution.
- **30 points**: Multi-consumer solution logging in **sorted** order (by timestamp), with required synchronization.

## Conclusion
The Parallel Firewall assignment focuses on converting a serial firewall processing system into a parallel one using threads, proper synchronization, and maintaining a sorted log of results in real time. The primary goals are to implement a thread-safe ring buffer, ensure orderly logging, and pass automated tests that validate correctness, performance, and coding style.

