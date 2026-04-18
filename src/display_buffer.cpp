// src/display_buffer.cpp
#include <time.h>
#include "display_buffer.h"

DisplayBuffer::DisplayBuffer()
{
    seq_ = 0;
    pthread_mutex_init(&mtx_, NULL);
    pthread_cond_init(&cv_, NULL);
}

DisplayBuffer::~DisplayBuffer()
{
    pthread_cond_destroy(&cv_);
    pthread_mutex_destroy(&mtx_);
}

void DisplayBuffer::put(const cv::Mat& frame)
{
    pthread_mutex_lock(&mtx_);
    frame_ = frame.clone();
    seq_ += 1;
    pthread_cond_signal(&cv_);
    pthread_mutex_unlock(&mtx_);
}

bool DisplayBuffer::get(cv::Mat& out, unsigned long last_seen, long timeout_ns)
{
    pthread_mutex_lock(&mtx_);

    if (seq_ == last_seen) {
        struct timespec abs;
        clock_gettime(CLOCK_REALTIME, &abs);
        abs.tv_nsec += timeout_ns;
        while (abs.tv_nsec >= 1000000000L) {
            abs.tv_nsec -= 1000000000L;
            abs.tv_sec++;
        }
        pthread_cond_timedwait(&cv_, &mtx_, &abs);
    }

    bool have_new = (seq_ != last_seen) && !frame_.empty();
    if (have_new) {
        out = frame_.clone();
    }
    pthread_mutex_unlock(&mtx_);
    return have_new;
}
