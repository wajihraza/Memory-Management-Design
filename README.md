# Memory-Management-Design

Virtual memory is a fundamental abstraction provided by modern operating systems to give 
processes the illusion of a large, contiguous address space while multiplexing limited 
physical memory. As workloads grow in complexity and multiple processes execute 
concurrently, memory pressure becomes significant. When the system is unable to keep 
each process’s active pages resident in physical memory, the number of page faults rises 
dramatically and the system may enter a state known as thrashing, where more time is spent 
handling page faults than executing useful work. Understanding how replacement 
algorithms, locality of reference, and working-set behavior interact is central to OS memory
management design. 

This project presents the design and evaluation of a complete memory-management 
simulator that models several key mechanisms used in real operating systems: page 
replacement, per-process working-set estimation, and thrashing detection. The simulator 
implements three replacement policies—FIFO, LRU, and a hybrid working-set–aware LRU 
(WS+LRU). In addition, it tracks the working set of each process using a sliding window of 
recent references and identifies thrashing based on both working-set overload and elevated 
page-fault rates. Synthetic workloads representing sequential scans, locality shifts, random 
access, and mixed multi-programmed activity are used to evaluate the algorithms under 
varying memory sizes and working-set window parameters. 
