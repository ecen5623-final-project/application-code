// sequencer.cpp

#include <stdio.h>
#include <time.h>
#include <csignal>
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

static long timespec_diff_ns(struct timespec* start, struct timespec* end)
{
    return (end->tv_sec - start->tv_sec) * 1000000000L +
           (end->tv_nsec - start->tv_nsec);
}

// Read from camera buffer
static void service1_read(Mat& frame)
{
    unsigned long seq;
    if (!camera_buffer.get(frame, seq)) {
        frame = Mat();
    }
}

// HSV color filtering
static void service2_transform(const Mat& in, Mat& out)
{
    if (in.empty()) {
        out = Mat();
        return;
    }

    hsv_lock();
    Scalar lo = hsv_lo;
    Scalar hi = hsv_hi;
    hsv_unlock();

    Mat hsv, mask, gray, gray_bgr;
    cvtColor(in, hsv, COLOR_BGR2HSV);
    inRange(hsv, lo, hi, mask);

    cvtColor(in, gray, COLOR_BGR2GRAY);
    cvtColor(gray, gray_bgr, COLOR_GRAY2BGR);

    gray_bgr.copyTo(out);
    in.copyTo(out, mask);
}

// Write to display buffer
static void service3_write(const Mat& frame)
{
    if (!frame.empty()) {
        display_buffer.put(frame);
    }
}

void* sequencer_thread(void* arg)
{
    set_fifo_prio(90);
    pin_to_core(2);

    // Wait for camera to be ready
    while (!g_camera_ready && !g_stop) {
        struct timespec ts = {0, 10000000L}; // 10ms
        nanosleep(&ts, NULL);
    }

    Mat frame_in, frame_out;
    struct timespec frame_start, frame_end;

    struct timespec next_release;
    clock_gettime(CLOCK_MONOTONIC, &next_release);

    while (!g_stop) {
        clock_gettime(CLOCK_MONOTONIC, &frame_start);

        service1_read(frame_in);
        service2_transform(frame_in, frame_out);
        service3_write(frame_out);

        clock_gettime(CLOCK_MONOTONIC, &frame_end);

        long elapsed_ns = timespec_diff_ns(&frame_start, &frame_end);
        if (elapsed_ns > T_NS)
            printf("Deadline MISSED: %.2f ms\n", elapsed_ns / 1000000.0);
        else
            printf("Deadline met: %.2f ms\n", elapsed_ns / 1000000.0);

        next_release.tv_nsec += T_NS;
        while (next_release.tv_nsec >= 1000000000L) {
            next_release.tv_nsec -= 1000000000L;
            next_release.tv_sec += 1;
        }
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_release, NULL);
    }

    return NULL;
}
