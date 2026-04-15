/*
 *
 *  Started with the example by Sam Siewert
 *  and incorporated heavy borrowing from seqgen.c (EX5) 
 *
 * author(s): James Way, Jack Napoli, & Madeleine Monfort
 * date: 04/14/2026
 *
 * brief: 
 *
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <syslog.h>
#include <time.h>

#include <pthread.h>
#include <sched.h>
#include <semaphore.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace cv;
using namespace std;

#define NUM_CPU_CORES (1)
#define NUM_THREADS (3)
#define TRUE (1)
#define FALSE (0)

void *Service_1(void *threadp);
void *Service_2(void *threadp);
void *Service_3(void *threadp);

int abortTest=FALSE;
int abortS1=FALSE, abortS2=FALSE, abortS3=FALSE, abortS4=FALSE, abortS5=FALSE, abortS6=FALSE, abortS7=FALSE;
sem_t semS1, semS2, semS3, semS4, semS5, semS6, semS7;

static double delta_t_ms(const struct timespec &start, const struct timespec &stop)
{
    double sec = (double)(stop.tv_sec - start.tv_sec) * 1000.0;
    double nsec = (double)(stop.tv_nsec - start.tv_nsec) / 1000000.0;
    return sec + nsec;
}

int main( int argc, char** argv )
{
    //all necessary variables
    cpu_set_t allcpuset;
    cpu_set_t threadcpu;
    pthread_t threads[NUM_THREADS];
    pthread_attr_t rt_sched_attr[NUM_THREADS];
    int rt_max_prio, rt_min_prio;
    struct sched_param rt_param[NUM_THREADS];
    struct sched_param main_param;
    pthread_attr_t main_attr;
    pid_t mainpid;
    
    openlog("frame_capture", LOG_PID | LOG_CONS, LOG_USER);
    
    //set the CPU core
    CPU_ZERO(&allcpuset);
    for(i=0; i < NUM_CPU_CORES; i++)
       CPU_SET(i, &allcpuset);
    
    //prep priorities
    mainpid=getpid();
    rt_max_prio = sched_get_priority_max(SCHED_FIFO);
    rt_min_prio = sched_get_priority_min(SCHED_FIFO);

    //make SCHED_FIFO
    rc=sched_getparam(mainpid, &main_param);
    main_param.sched_priority=rt_max_prio;
    rc=sched_setscheduler(getpid(), SCHED_FIFO, &main_param);
    if(rc < 0) perror("main_param");
    
    // initialize the sequencer semaphores
    //
    if (sem_init (&semS1, 0, 0)) { printf ("Failed to initialize S1 semaphore\n"); exit (-1); }
    if (sem_init (&semS2, 0, 0)) { printf ("Failed to initialize S2 semaphore\n"); exit (-1); }
    if (sem_init (&semS3, 0, 0)) { printf ("Failed to initialize S3 semaphore\n"); exit (-1); }
    
    //setup the thread info for all the services
    for(i=0; i < NUM_THREADS; i++)
    {

      CPU_ZERO(&threadcpu);
      CPU_SET(3, &threadcpu);

      rc=pthread_attr_init(&rt_sched_attr[i]);
      rc=pthread_attr_setinheritsched(&rt_sched_attr[i], PTHREAD_EXPLICIT_SCHED);
      rc=pthread_attr_setschedpolicy(&rt_sched_attr[i], SCHED_FIFO);
      //rc=pthread_attr_setaffinity_np(&rt_sched_attr[i], sizeof(cpu_set_t), &threadcpu);

      rt_param[i].sched_priority=rt_max_prio-i;
      pthread_attr_setschedparam(&rt_sched_attr[i], &rt_param[i]);

    }
    
    //create the threads:
    // Create Service threads which will block awaiting release for:
    //

    // Servcie_1 = prio? @ rate?
    //
    rt_param[1].sched_priority=rt_max_prio-1;
    pthread_attr_setschedparam(&rt_sched_attr[1], &rt_param[1]);
    rc=pthread_create(&threads[1],               // pointer to thread descriptor
                      &rt_sched_attr[1],         // use specific attributes
                      //(void *)0,               // default attributes
                      Service_1,                 // thread function entry point
                      NULL                       // parameters to pass in
                     );
    if(rc < 0)
        perror("pthread_create for service 1");
    else
        printf("pthread_create successful for service 1\n");
    
    
    for(i=0;i<NUM_THREADS;i++)
       pthread_join(threads[i], NULL);

   printf("\nTEST COMPLETE\n");

    //clean up the image capturing!
    cvReleaseCapture(&capture);
    cvDestroyWindow("Capture Example");
    
};

void *Service_1(void *threadp)
{
    struct timespec capture_start, capture_stop;
    double capture_time_ms;
    
    cvNamedWindow("Capture Example", CV_WINDOW_AUTOSIZE);
    CvCapture* capture = cvCreateCameraCapture(0);
    IplImage* frame;
    

    while(!abortS1)
    {
        sem_wait(&semS1);
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

    pthread_exit((void *)0);
}

void *Service_2(void *threadp)
{
    struct timespec capture_start, capture_stop;
    double capture_time_ms;
    
    cvNamedWindow("Capture Example", CV_WINDOW_AUTOSIZE);
    CvCapture* capture = cvCreateCameraCapture(0);
    IplImage* frame;
    

    while(!abortS1)
    {
        sem_wait(&semS1);
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

    pthread_exit((void *)0);
}

void *Service_3(void *threadp)
{
    struct timespec capture_start, capture_stop;
    double capture_time_ms;
    
    cvNamedWindow("Capture Example", CV_WINDOW_AUTOSIZE);
    CvCapture* capture = cvCreateCameraCapture(0);
    IplImage* frame;
    

    while(!abortS1)
    {
        sem_wait(&semS1);
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

    pthread_exit((void *)0);
}