// camera_thread.cpp

#include <unistd.h>
#include <stdio.h>
#include <csignal>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/videoio/videoio.hpp>
#include "camera_buffer.h"
#include "realtime.h"

using namespace cv;

extern volatile sig_atomic_t g_stop;
extern volatile sig_atomic_t g_camera_ready;
extern CameraBuffer camera_buffer;

#define DEVICE_PATH "/dev/video0"
#define HRES 640
#define VRES 480
#define FPS  30
#define WARMUP_FRAMES 30

void* camera_thread(void* arg)
{
    pin_to_core(1);

    VideoCapture cap(DEVICE_PATH, CAP_V4L2);
    if (!cap.isOpened()) {
        printf("camera failed to open\n");
        g_stop = 1;
        return NULL;
    }

    cap.set(CAP_PROP_FOURCC, VideoWriter::fourcc('M','J','P','G'));
    cap.set(CAP_PROP_FRAME_WIDTH,  HRES);
    cap.set(CAP_PROP_FRAME_HEIGHT, VRES);
    cap.set(CAP_PROP_FPS,          FPS);
    cap.set(CAP_PROP_BUFFERSIZE,   2);

    Mat frame;

    // Warmup: capture a few frames to initialize V4L2 buffers
    printf("Camera warmup...\n");
    for (int i = 0; i < WARMUP_FRAMES && !g_stop; i++) {
        cap >> frame;
    }
    printf("Camera ready.\n");
    g_camera_ready = 1;

    while (!g_stop) {
        cap >> frame;
        if (frame.empty()) {
            printf("empty frame\n");
            continue;
        }
        camera_buffer.put(frame);
    }

    cap.release();
    return NULL;
}
