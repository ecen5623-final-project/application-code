// include/realtime.h
#ifndef REALTIME_H
#define REALTIME_H

// Timing helper function
inline double delta_t_ms(const struct timespec &start, const struct timespec &stop)
{
    double sec2ms = (double)(stop.tv_sec - start.tv_sec) * 1000.0;
    double nsec2ms = (double)(stop.tv_nsec - start.tv_nsec) / 1000000.0;
    return sec2ms + nsec2ms;
}

void set_fifo_prio(int prio);   // SCHED_FIFO with given priority (1..99)
void pin_to_core(int core);     // Set CPU affinity to a single core

#endif
