// src/hsv_config.cpp
#include <pthread.h>
#include "hsv_config.h"

using namespace cv;

pthread_mutex_t hsv_mutex = PTHREAD_MUTEX_INITIALIZER;
Scalar hsv_lo;
Scalar hsv_hi;

void hsv_init(void)
{
    // default to green
    hsv_lo = Scalar(40, 80, 70);
    hsv_hi = Scalar(80, 255, 255);
}

void hsv_lock(void)
{
    pthread_mutex_lock(&hsv_mutex);
}

void hsv_unlock(void)
{
    pthread_mutex_unlock(&hsv_mutex);
}

void set_hsv_preset(char key)
{
    pthread_mutex_lock(&hsv_mutex);
    switch (key) {
        case 'r':
            hsv_lo = Scalar(0, 120, 70);
            hsv_hi = Scalar(10, 255, 255);
            break;
        case 'g':
            hsv_lo = Scalar(40, 80, 70);
            hsv_hi = Scalar(80, 255, 255);
            break;
        case 'b':
            hsv_lo = Scalar(100, 120, 70);
            hsv_hi = Scalar(130, 255, 255);
            break;
        case 'y':
            hsv_lo = Scalar(20, 120, 70);
            hsv_hi = Scalar(35, 255, 255);
            break;
    }
    pthread_mutex_unlock(&hsv_mutex);
}
