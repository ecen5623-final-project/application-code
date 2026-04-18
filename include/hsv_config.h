// include/hsv_config.h
#ifndef HSV_CONFIG_H
#define HSV_CONFIG_H

#include <opencv2/core/core.hpp>

extern cv::Scalar hsv_lo;
extern cv::Scalar hsv_hi;

void hsv_init(void);
void hsv_lock(void);
void hsv_unlock(void);
void set_hsv_preset(char key);

#endif
