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
#include "LCD_2inch.h"

using namespace cv;

extern volatile sig_atomic_t g_stop;
extern volatile sig_atomic_t g_camera_ready;
extern CameraBuffer camera_buffer;

#define CAMERA_INDEX 0
#define HRES LCD_2IN_WIDTH   // max 1280
#define VRES LCD_2IN_HEIGHT  // max 720
#define FPS  30
#define WARMUP_FRAMES 30

void* camera_thread(void* arg)
{
    pin_to_core(1);
    set_fifo_prio(80);

    // Suppress libjpeg warnings about corrupt MJPEG data
    //by forcing all "STDERR" messages to the devnull file
    int devnull = open("/dev/null", O_WRONLY);
    if (devnull >= 0) {
        dup2(devnull, STDERR_FILENO);
        close(devnull);
    }

    VideoCapture cap(CAMERA_INDEX, CAP_V4L2);

    if (!cap.isOpened()) {
        syslog(LOG_ERR, "camera failed to open");
        g_stop = 1;
        return NULL;
    }

    cap.set(CAP_PROP_FOURCC, VideoWriter::fourcc('Y','U','Y','V'));
    cap.set(CAP_PROP_FRAME_WIDTH,  HRES);
    cap.set(CAP_PROP_FRAME_HEIGHT, VRES);
    cap.set(CAP_PROP_FPS,          FPS);
    cap.set(CAP_PROP_BUFFERSIZE,   1);

    // Manual exposure — OpenCV maps CAP_PROP_AUTO_EXPOSURE to the V4L2 menu:
    //   1 = Manual Mode, 3 = Aperture Priority. Manual locks the exposure time
    //   so the sensor can't stretch a frame interval past the FPS period.
    // CAP_PROP_EXPOSURE is in 100-µs units on UVC; 50 => 5 ms.
    cap.set(CAP_PROP_AUTO_EXPOSURE, 1);
    cap.set(CAP_PROP_EXPOSURE,      50);

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
