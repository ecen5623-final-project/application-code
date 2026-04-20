// main.cpp

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/mman.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "shared.h"
#include "hsv_config.h"

// Global shared state
CameraBuffer camera_buffer;
DisplayBuffer display_buffer;
volatile sig_atomic_t g_stop = 0;
volatile sig_atomic_t g_camera_ready = 0;


static void sigint_handler(int) { g_stop = 1; }

// Thread entry points
extern void* camera_thread(void*);
extern void* sequencer_thread(void*);

int main(int argc, char** argv)
{
    bool headless = (argc > 1 && strcmp(argv[1], "--headless") == 0);

    openlog("capture", LOG_PID | LOG_NDELAY | LOG_PERROR, LOG_USER);

    mlockall(MCL_CURRENT | MCL_FUTURE);
    signal(SIGINT, sigint_handler);
    hsv_init();

    pthread_t t_cam, t_seq;

    pthread_create(&t_cam, NULL, camera_thread, NULL);
    pthread_create(&t_seq, NULL, sequencer_thread, NULL);

    cv::Mat frame;
    unsigned long last_seq = 0;

    if (headless) {
        printf("Running in headless mode. Press Ctrl+C to quit.\n");
        fflush(stdout);
        while (!g_stop) {
            if (display_buffer.get(frame, last_seq, 100000000L)) {
                last_seq++;
            }
            usleep(10000);
        }
    } else {
        printf("Running. Press r/g/b/y to change color, q to quit.\n");
        while (!g_stop) {
            if (display_buffer.get(frame, last_seq, 100000000L)) {
                last_seq++;
                cv::imshow("Output", frame);
            }

            char c = cv::waitKey(10);
            if (c == 'q' || c == 27) {
                g_stop = 1;
                break;
            }
            if (c == 'r' || c == 'g' || c == 'b' || c == 'y') {
                set_hsv_preset(c);
                printf("Color: %c\n", c);
            }
        }
        cv::destroyAllWindows();
    }

    pthread_join(t_cam, NULL);
    pthread_join(t_seq, NULL);

    printf("Done.\n");
    closelog();
    return 0;
}
