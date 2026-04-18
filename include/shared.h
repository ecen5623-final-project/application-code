// include/shared.h
#ifndef SHARED_H
#define SHARED_H

#include <signal.h>
#include "camera_buffer.h"

extern volatile sig_atomic_t g_stop;
extern CameraBuffer camera_buffer;

#endif
