// include/display_buffer.h
#ifndef DISPLAY_BUFFER_H
#define DISPLAY_BUFFER_H

#include <pthread.h>
#include <opencv2/core/core.hpp>

class DisplayBuffer {
public:
    DisplayBuffer();
    ~DisplayBuffer();

    // Called by S3 to publish frame for display
    void put(const cv::Mat& frame);

    // Called by display thread to grab latest frame (blocks until new frame)
    bool get(cv::Mat& out, unsigned long last_seen, long timeout_ns);

private:
    pthread_mutex_t mtx_;
    pthread_cond_t cv_;
    cv::Mat frame_;
    unsigned long seq_;
};

#endif
