# CPU Scheduler Simulation

A C-based operating systems project that simulates process creation, scheduling, execution, and termination on a single CPU with infinite memory.

## Features

- Round Robin, Shortest Job First, and Shortest Remaining Time Next scheduling.
- Process Control Blocks with running, ready, and blocked states.
- Separate process generator, scheduler, clock, and worker-process components.
- Inter-process communication and process creation with POSIX APIs.
- Scheduler log and performance reports for CPU utilization and turnaround metrics.

## Build And Run

This project targets a POSIX-like environment with GCC and the standard process, signal, and IPC APIs.

```bash
make
./process_generator
```

The input process list is read from `processes.txt`. Generated binaries, logs, and performance outputs are ignored by Git.
