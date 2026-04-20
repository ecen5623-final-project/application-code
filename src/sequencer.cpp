// sequencer.cpp

#include <time.h>
#include <syslog.h>
#include <csignal>
#include <sched.h>
#include <pthread.h>
#include <semaphore.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "camera_buffer.h"
#include "display_buffer.h"
#include "hsv_config.h"
#include "realtime.h"

using namespace cv;

extern volatile sig_atomic_t g_stop;
extern volatile sig_atomic_t g_camera_ready;
extern CameraBuffer camera_buffer;
extern DisplayBuffer display_buffer;

#define T_NS 33333333L  // 30 fps period

static sem_t sem_s1, sem_s2, sem_s3, sem_done;
static Mat shared_frame_in;
static Mat shared_frame_out;

static long timespec_diff_ns(struct timespec* start, struct timespec* end)
{
    return (end->tv_sec - start->tv_sec) * 1000000000L +
           (end->tv_nsec - start->tv_nsec);
}

// Service 1: Read from camera buffer
static void* service1_thread(void* arg)
{
    int rt_max_prio = sched_get_priority_max(SCHED_FIFO);
    set_fifo_prio(rt_max_prio - 1);
    pin_to_core(2);

    while (!g_stop) {
        sem_wait(&sem_s1);
        if (g_stop) break;

        unsigned long seq;
        Mat frame;
        if (camera_buffer.get(frame, seq)) {
            shared_frame_in = frame;
        } else {
            shared_frame_in = Mat();
        }

        sem_post(&sem_s2);
    }
    return NULL;
}

// Service 2: HSV color filtering
static void* service2_thread(void* arg)
{
    int rt_max_prio = sched_get_priority_max(SCHED_FIFO);
    set_fifo_prio(rt_max_prio - 2);
    pin_to_core(2);

    while (!g_stop) {
        sem_wait(&sem_s2);
        if (g_stop) break;

        // Reuse buffers across frames: OpenCV only reallocates if size/type
        // changes, so these stabilize after the first frame.
        static Mat hsv, mask, gray, out;

        Mat in = shared_frame_in;
        if (!in.empty()) {
            hsv_lock();
            Scalar lo = hsv_lo;
            Scalar hi = hsv_hi;
            hsv_unlock();

            cvtColor(in, hsv, COLOR_BGR2HSV);
            inRange(hsv, lo, hi, mask);

            // Desaturated 3-channel backdrop written directly into `out`,
            // then colored pixels overlaid where mask is set.
            cvtColor(in, gray, COLOR_BGR2GRAY);
            cvtColor(gray, out, COLOR_GRAY2BGR);
            in.copyTo(out, mask);
        } else {
            out = Mat();
        }

        shared_frame_out = out;
        sem_post(&sem_s3);
    }
    return NULL;
}

// Service 3: Write to display buffer
static void* service3_thread(void* arg)
{
    int rt_max_prio = sched_get_priority_max(SCHED_FIFO);
    set_fifo_prio(rt_max_prio - 3);
    pin_to_core(2);

    while (!g_stop) {
        sem_wait(&sem_s3);
        if (g_stop) break;

        Mat frame = shared_frame_out;
        if (!frame.empty()) {
            display_buffer.put(frame);
        }

        sem_post(&sem_done);
    }
    return NULL;
}

void* sequencer_thread(void* arg)
{
    int rt_max_prio = sched_get_priority_max(SCHED_FIFO);
    set_fifo_prio(rt_max_prio);
    pin_to_core(2);

    sem_init(&sem_s1, 0, 0);
    sem_init(&sem_s2, 0, 0);
    sem_init(&sem_s3, 0, 0);
    sem_init(&sem_done, 0, 0);

    pthread_t t1, t2, t3;
    pthread_create(&t1, NULL, service1_thread, NULL);
    pthread_create(&t2, NULL, service2_thread, NULL);
    pthread_create(&t3, NULL, service3_thread, NULL);

    // Wait for camera to be ready
    while (!g_camera_ready && !g_stop) {
        struct timespec ts = {0, 10000000L}; // 10ms
        nanosleep(&ts, NULL);
    }

    struct timespec frame_start, frame_end;
    struct timespec next_release;
    clock_gettime(CLOCK_MONOTONIC, &next_release);

    while (!g_stop) {
        clock_gettime(CLOCK_MONOTONIC, &frame_start);

        // Release S1, which chains to S2, then S3
        sem_post(&sem_s1);

        // Wait for S3 to complete
        sem_wait(&sem_done);

        clock_gettime(CLOCK_MONOTONIC, &frame_end);

        long elapsed_ns = timespec_diff_ns(&frame_start, &frame_end);
        if (elapsed_ns > T_NS)
            syslog(LOG_WARNING, "Deadline MISSED: %.2f ms", elapsed_ns / 1000000.0);
        else
            syslog(LOG_INFO, "Deadline met: %.2f ms", elapsed_ns / 1000000.0);

        next_release.tv_nsec += T_NS;
        while (next_release.tv_nsec >= 1000000000L) {
            next_release.tv_nsec -= 1000000000L;
            next_release.tv_sec += 1;
        }
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_release, NULL);
    }

    // Signal threads to exit
    sem_post(&sem_s1);
    sem_post(&sem_s2);
    sem_post(&sem_s3);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);

    sem_destroy(&sem_s1);
    sem_destroy(&sem_s2);
    sem_destroy(&sem_s3);
    sem_destroy(&sem_done);

    return NULL;
}
