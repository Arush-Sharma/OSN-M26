# OSN Mini-Project 1: C-Shell & xv6 MLFQ Scheduler

## Overview
This repository contains the implementation for Mini-Project 1, divided into two primary components:
1. **C-Shell**: A fully functional, POSIX-compliant UNIX shell built from scratch with support for lexical parsing, I/O redirection, piping, background job control, and custom intrinsic commands.
2. **xv6 MLFQ Scheduler**: A modified xv6 kernel that introduces a Multi-Level Feedback Queue (MLFQ) scheduling policy alongside the default Round Robin scheduler.

## Directory Structure
```text
mini-project1/
├── c-shell/
│   ├── src/           # C source files (main, lexer, execute, builtin, jobs, etc.)
│   ├── include/       # C header files defining interfaces and structures
│   └── Makefile       # Build system for C-Shell
├── xv6/
│   ├── kernel/        # Modified xv6 kernel source (proc.c, trap.c, proc.h)
│   ├── user/          # User-space programs (including schedulertest)
│   ├── Makefile       # xv6 Build system (supports SCHEDULER macro)
│   └── report.pdf     # Scheduling analysis and cross-scheduler comparison
├── AI-usage.pdf       # Documentation of AI assistance used during development
├── mlfq_plot.png      # Scatter plot of MLFQ queue transitions
└── README.md          # Project documentation (this file)