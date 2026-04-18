// include/camera_buffer.h
#ifndef CAMERA_BUFFER_H
#define CAMERA_BUFFER_H

#include <pthread.h>
#include <opencv2/core/core.hpp>

class CameraBuffer {
public:
    CameraBuffer();
    ~CameraBuffer();

    // Called by camera thread to publish new frame
    void put(const cv::Mat& frame);

    // Called by S1 to grab latest frame (non-blocking, returns false if no frame yet)
    bool get(cv::Mat& out, unsigned long& seq_out);

private:
    pthread_mutex_t mtx_;
    cv::Mat frame_;
    unsigned long seq_;
};

#endif
