// src/realtime.cpp
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "realtime.h"

void set_fifo_prio(int prio)
{
    struct sched_param sp;
    memset(&sp, 0, sizeof(sp));
    sp.sched_priority = prio;
    int rc = pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);
    if (rc != 0)
        fprintf(stderr, "set_fifo_prio(%d) failed: %s\n", prio, strerror(rc));
}

void pin_to_core(int core)
{
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(core, &set);
    int rc = pthread_setaffinity_np(pthread_self(), sizeof(set), &set);
    if (rc != 0)
        fprintf(stderr, "pin_to_core(%d) failed: %s\n", core, strerror(rc));
}
