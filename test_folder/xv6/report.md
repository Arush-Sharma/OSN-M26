# Mini-Project 1: C-Shell and xv6 MLFQ Scheduler

## 2.3.1 Implementation Summary

**Makefile & SCHEDULER Macro:**
Modified the xv6 Makefile to accept a `SCHEDULER` macro at compile time. By appending `-D$(SCHEDULER)` to the `CFLAGS`, the compiler selectively activates the MLFQ code blocks. If no flag is passed, it defaults to the original Round Robin logic.

**Process Structure (`proc.h`):**
Extended `struct proc` with three new variables: `queue_level` (0-3) to track current priority, `ticks_consumed` to track the time slice used at the current level, and `enter_time` to track when a process entered a queue for round-robin tie-breaking within the same priority level.

**Process Creation & Wakeup (`proc.c`):**
In `allocproc()`, newly created processes initialize at queue level 0 with 0 ticks consumed. In `wakeup()`, when a sleeping process becomes runnable (e.g., after voluntarily yielding for I/O), its `enter_time` is updated to the current tick so it is placed at the tail of its current queue.

**Queue Selection & Preemption (`proc.c` & `trap.c`):**
The `scheduler()` loop now strictly selects the runnable process in the highest priority queue (0 being highest, 3 being lowest). Within the same queue, it uses `enter_time` to pick the process that has been waiting the longest. In `usertrap` and `kerneltrap`, a running process increments its `ticks_consumed`. It is preempted (forced to `yield`) if it exhausts its queue's time slice (1, 4, 8, or 16 ticks) or if a new process arrives in a higher priority queue.

**Time-slice Handling & Yielding (`proc.c`):**
Inside `yield()`, if a process is demoted because it exhausted its time slice, its `queue_level` increments (up to a max of 3), and its `ticks_consumed` resets. If it yields voluntarily before the slice expires, it remains in its current queue.

**Priority Boosting (`trap.c`):**
Inside `clockintr()`, a check occurs every tick. If `ticks % 48 == 0`, a system-wide priority boost triggers. All processes (regardless of state) have their `queue_level` reset to 0 and `ticks_consumed` reset to 0, preventing starvation of CPU-bound processes in lower queues.

**Procdump Changes:**
Modified `procdump()` to conditionally print `queue_level` and `ticks_consumed` alongside the PID and state when MLFQ is active, enabling real-time verification of queue migrations.

---

## 2.3.2 MLFQ Analysis

The procdump logs track three CPU-bound test processes (PIDs 8, 9, and 10) spawned by the schedulertest parent process (PID 7). The data clearly demonstrates the MLFQ time-slicing and demotion mechanics in action.

Initially, the child processes consume their 1-tick time slice in Queue 0 and are instantly demoted to Queue 1. As execution continues, we observe them accumulating ticks in Queue 1 simultaneously (reaching 1, 2, and 3 ticks). PID 10 successfully exhausts its 4-tick limit in Queue 1 and is observed dynamically demoting to Queue 2, resetting its tick count back to 0. PIDs 8 and 9 finish their CPU-burst execution while still in Queue 1. This proves that the scheduler correctly punishes CPU-heavy processes by pushing them to lower priority queues, while allowing slightly shorter tasks to finish in higher queues.

---

## 2.3.3 Cross-Scheduler Comparison

*Note: Data derived from standard workload testing.*

| Scheduler | Avg Turnaround Time | Avg Waiting Time | Avg Response Time |
| --- | --- | --- | --- |
| **FIFO** | High | High | High |
| **Round Robin (RR)** | High | Medium | Low |
| **MLFQ** | Low / Medium | Low | Very Low |

**Trade-offs Discussion:**
FIFO suffers from the "convoy effect," resulting in high waiting and response times because short jobs get stuck behind long CPU-bound jobs. Round Robin solves the response time issue by time-slicing, making it highly responsive for interactive tasks, but this constant context switching significantly bloats the average turnaround time. MLFQ provides the optimal balance. By granting new processes immediate access to the CPU in Queue 0, it achieves a response time identical or superior to RR. However, by punishing CPU hogs and pushing them to lower queues with longer time slices, it reduces the context-switch overhead of RR, leading to much better turnaround times for both short interactive tasks and long background workloads.

---