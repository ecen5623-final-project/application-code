// src/camera_buffer.cpp
#include "camera_buffer.h"

CameraBuffer::CameraBuffer()
{
    seq_ = 0;
    pthread_mutex_init(&mtx_, NULL);
}

CameraBuffer::~CameraBuffer()
{
    pthread_mutex_destroy(&mtx_);
}

void CameraBuffer::put(const cv::Mat& frame)
{
    pthread_mutex_lock(&mtx_);
    frame_ = frame.clone();
    seq_ += 1;
    pthread_mutex_unlock(&mtx_);
}

bool CameraBuffer::get(cv::Mat& out, unsigned long& seq_out)
{
    pthread_mutex_lock(&mtx_);
    bool have_frame = !frame_.empty();
    if (have_frame) {
        out = frame_.clone();
        seq_out = seq_;
    }
    pthread_mutex_unlock(&mtx_);
    return have_frame;
}
