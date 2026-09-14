```markdown
# xv6 MLFQ Scheduler

## Overview
This directory contains the modified xv6 kernel implementing a Multi-Level Feedback Queue (MLFQ) scheduling policy, alongside the default xv6 Round Robin (RR) scheduler.

## Build Instructions
The scheduler policy is selected at compile time using the `SCHEDULER` macro in the Makefile.

**To build and run with the MLFQ scheduler:**
```bash
make clean
make qemu SCHEDULER=MLFQ

```

**To build and run with the default Round Robin scheduler:**

```bash
make clean
make qemu

```

## Testing the Scheduler

A custom user program, `schedulertest`, is included to demonstrate the MLFQ mechanics. It spawns three CPU-bound child processes that continuously consume CPU cycles to trigger time-slice demotions.

1. Boot into xv6 using the MLFQ flag.
2. Run the test program from the shell:
```bash
$ schedulertest

```


3. While the processes are running, press `Ctrl+P` to trigger the modified `procdump`.
4. The output will display each process's PID, State, Name, Current Queue Level (`Q:`), and Ticks Consumed in the current slice (`Ticks:`).

## MLFQ Design & Implementation Details

* **Queue Structure:** 4 priority queues numbered 0 to 3, with 0 being the highest priority.
* **Time Slices:**
* Queue 0: 1 tick
* Queue 1: 4 ticks
* Queue 2: 8 ticks
* Queue 3: 16 ticks (Operates as Round Robin at the lowest level).


* **Placement & Yielding:** Newly created processes are pushed to the tail of Queue 0. Processes that voluntarily yield the CPU (e.g., for I/O) before their time slice expires remain in their current queue and are placed at the tail when they become runnable again.
* **Strict Priority & Preemption:** The scheduler always selects the process at the head of the highest non-empty priority queue. A running process is preempted at the tick boundary if a new process arrives in a higher priority queue.
* **Demotion:** If a process exhausts its allotted time slice for its current queue without yielding, it is demoted to the next lower queue (or re-inserted at the tail of Queue 3).
* **Priority Boosting (Anti-Starvation):** To prevent starvation of CPU-bound processes, a system-wide priority boost occurs every 48 ticks. All processes are immediately moved to Queue 0 and their tick counters are reset.

## Graph Generation & Data

The Python script `graph.py` (located in the project root) was used to generate `mlfq_plot.png`. It parses the queue transitions over time based on the `procdump` data gathered during the `schedulertest` execution, visualizing the time-slice demotions and the 48-tick priority boost.

```


```