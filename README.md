# Concurrent Miner-Monitor System

This repository contains a highly concurrent, multi-process system developed in C designed to manage computationally intensive proof-of-work tasks. The system relies heavily on POSIX threads (`pthreads`) and advanced Inter-Process Communication (IPC) mechanisms to parallelize computation and manage shared resources safely.

## System Architecture
The project is divided into two primary subsystems that interact in a shared environment:
* **Monitor (`monitor`)**: Acts as the central controller, managing the allocation of search spaces, handling signals, and overseeing the worker processes.
* **Miner (`miner`)**: Worker processes that execute the proof-of-work algorithms (`pow.c`). They utilize multi-threading to parallelize the search space and report back to the monitor.

**Key Technical Implementations:**
* **Concurrency & Synchronization:** Deep use of POSIX threads, semaphores, and mutexes to prevent race conditions and deadlocks in shared memory spaces.
* **IPC (Inter-Process Communication):** Implementation of shared memory (`-lrt`), message queues, pipes, and signal handling for robust process-to-process communication.
* **Memory Management:** Rigorous dynamic memory allocation and pointer management to ensure zero memory leaks during extended execution.

## Compilation and Execution
The project uses `gcc` and a custom `Makefile` for automated compilation.

**Build all targets:**
```bash
make all
