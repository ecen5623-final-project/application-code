// include/realtime.h
#ifndef REALTIME_H
#define REALTIME_H

void set_fifo_prio(int prio);   // SCHED_FIFO with given priority (1..99)
void pin_to_core(int core);     // Set CPU affinity to a single core

#endif
