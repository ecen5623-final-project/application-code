// camera_thread.cpp

#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
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

#define CAMERA_INDEX 0
#define HRES 640
#define VRES 480
#define FPS  30
#define WARMUP_FRAMES 30

void* camera_thread(void* arg)
{
    pin_to_core(1);

    /*// Suppress libjpeg warnings about corrupt MJPEG data
    //by forcing all "STDERR" messages to the devnull file
    int devnull = open("/dev/null", O_WRONLY);
    if (devnull >= 0) {
        dup2(devnull, STDERR_FILENO);
        close(devnull);
    }

    VideoCapture cap(CAMERA_INDEX, CAP_V4L2);*/

    VideoCapture cap(CAMERA_INDEX);

    if (!cap.isOpened()) {
        syslog(LOG_ERR, "camera failed to open");
        g_stop = 1;
        return NULL;
    }

    cap.set(CAP_PROP_FOURCC, VideoWriter::fourcc('M','J','P','G')); // warning message from this line with some versions
    cap.set(CAP_PROP_FRAME_WIDTH,  HRES);
    cap.set(CAP_PROP_FRAME_HEIGHT, VRES);
    cap.set(CAP_PROP_FPS,          FPS);
    cap.set(CAP_PROP_BUFFERSIZE,   2); // warning message from this line with some versions

    Mat frame;
    int empty_count = 0;

    syslog(LOG_INFO, "Camera warmup...");
    for (int i = 0; i < WARMUP_FRAMES && !g_stop; i++) {
        cap >> frame;
    }
    syslog(LOG_INFO, "Camera ready");
    g_camera_ready = 1;

    while (!g_stop) {
        cap >> frame;
        if (frame.empty()) {
            if (++empty_count > 10) {
                syslog(LOG_ERR, "Camera failed after %d empty frames", empty_count);
                g_stop = 1;
                break;
            }
            continue;
        }
        empty_count = 0;
        camera_buffer.put(frame);
    }

    cap.release();
    return NULL;
}
