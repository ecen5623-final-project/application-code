// include/shared.h
#ifndef SHARED_H
#define SHARED_H

#include <signal.h>
#include "camera_buffer.h"
#include "display_buffer.h"

extern volatile sig_atomic_t g_stop;
extern volatile sig_atomic_t g_camera_ready;
extern CameraBuffer camera_buffer;
extern DisplayBuffer display_buffer;

#endif
