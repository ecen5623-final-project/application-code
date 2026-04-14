/*
 *
 *  Example by Sam Siewert 
 *
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <syslog.h>
#include <time.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace cv;
using namespace std;

static double delta_t_ms(const struct timespec &start, const struct timespec &stop)
{
    double sec = (double)(stop.tv_sec - start.tv_sec) * 1000.0;
    double nsec = (double)(stop.tv_nsec - start.tv_nsec) / 1000000.0;
    return sec + nsec;
}


int main( int argc, char** argv )
{
    struct timespec capture_start, capture_stop;
    double capture_time_ms;
    
    openlog("frame_capture", LOG_PID | LOG_CONS, LOG_USER);
    
    cvNamedWindow("Capture Example", CV_WINDOW_AUTOSIZE);
    CvCapture* capture = cvCreateCameraCapture(0);
    IplImage* frame;

    while(1)
    {
        clock_gettime(CLOCK_MONOTONIC_RAW, &capture_start);
        
        frame=cvQueryFrame(capture);
        
        clock_gettime(CLOCK_MONOTONIC_RAW, &capture_stop);
        capture_time_ms = delta_t_ms(capture_start, capture_stop);
        
        syslog(LOG_INFO, "Frame capture time,%.3f,ms", capture_time_ms);
     
        if(!frame) break;

        cvShowImage("Capture Example", frame);

        char c = cvWaitKey(33);
        if( c == 27 ) break;
    }

    cvReleaseCapture(&capture);
    cvDestroyWindow("Capture Example");
    
};
