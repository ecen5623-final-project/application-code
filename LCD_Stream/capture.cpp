/*
 *  Created by:
 *  J. Way, M. Monfort, J. Napoli
 * 
 *  Leveraging code from Sam Siewert (seqgen)
 *
 */
 
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <syslog.h>

#include <vector>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/videoio/videoio.hpp>

// Waveshare headers
extern "C" {
    #include "LCD_2inch.h"
    #include "DEV_Config.h"
}

// Namespace
using namespace cv;
using namespace std;

#define TRUE 1
#define FALSE 0

#define WHITE 0xFFFF
#define BLACK 0x0000

#define NUM_THREADS     4   // sequencer + 3 services
#define SEQ_PERIOD_NS   100000000L   // 100 ms = 10 Hz ------------------------- FRAME RATE

static volatile int abortTest = FALSE;
static volatile int abortS1 = FALSE, abortS2 = FALSE, abortS3 = FALSE;

static sem_t semS1, semS2, semS3;
static pthread_mutex_t raw_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t proc_mutex = PTHREAD_MUTEX_INITIALIZER;

static VideoCapture cap;
static Mat raw_frame;
static Mat processed_frame;
static vector<uint8_t> lcd_buf(LCD_2IN_WIDTH * LCD_2IN_HEIGHT * 2);

// Timing helper function
static double delta_t_ms(const struct timespec &start, const struct timespec &stop)
{
    double sec = (double)(stop.tv_sec - start.tv_sec) * 1000.0;
    double nsec = (double)(stop.tv_nsec - start.tv_nsec) / 1000000.0;
    return sec + nsec;
}

// RGB88 to 565 conversion
static uint16_t rgb888_to_rgb565(unsigned char r, unsigned char g, unsigned char b)
{
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void *Sequencer(void *threadp); // executive/frame rate control
void *Service_1(void *threadp); // capture
void *Service_2(void *threadp); // transform
void *Service_3(void *threadp); // push to LCD


/******************************
 *          MAIN
 ******************************/
int main(int argc, char** argv)
{
    pthread_t threads[NUM_THREADS];
    pthread_attr_t rt_sched_attr[NUM_THREADS];
    struct sched_param rt_param[NUM_THREADS];
    struct sched_param main_param;
    int rc;
    int rt_max_prio = sched_get_priority_max(SCHED_FIFO);
    
    //init syslog
    openlog("LCD_ex",LOG_CONS | LOG_ODELAY,LOG_USER);

    // Camera Init - using VideoCapture instead of CvCapture
    cap.open(0);
    if(!cap.isOpened()) {
        printf("camera failed\n");
        return -1;
    }
    
    /* SPI Module Init */
	if(DEV_ModuleInit() != 0){
        DEV_ModuleExit();
        exit(0);
    }
    
    // LCD Init
    LCD_2IN_Init();
    LCD_2IN_Clear(WHITE);
    LCD_SetBacklight(1023);

    // Semaphore Init
    if (sem_init (&semS1, 0, 0)) { printf ("Failed to initialize S1 semaphore\n"); exit (-1); }
    if (sem_init (&semS2, 0, 0)) { printf ("Failed to initialize S2 semaphore\n"); exit (-1); }
    if (sem_init (&semS3, 0, 0)) { printf ("Failed to initialize S3 semaphore\n"); exit (-1); }

    //  Configure Service parameters
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_attr_init(&rt_sched_attr[i]);
        pthread_attr_setinheritsched(&rt_sched_attr[i], PTHREAD_EXPLICIT_SCHED);
        pthread_attr_setschedpolicy(&rt_sched_attr[i], SCHED_FIFO);
        rt_param[i].sched_priority = rt_max_prio - i;
        pthread_attr_setschedparam(&rt_sched_attr[i], &rt_param[i]);
    }

    // Highest prio = sequencer
    pthread_create(&threads[0], &rt_sched_attr[0], Sequencer, NULL);
    pthread_create(&threads[1], &rt_sched_attr[1], Service_1, NULL);
    pthread_create(&threads[2], &rt_sched_attr[2], Service_2, NULL);
    pthread_create(&threads[3], &rt_sched_attr[3], Service_3, NULL);

    // LCD buffer: 240 x 320
    vector<uint8_t> lcd_buf(240 * 320 * 2);

    // Wait for threads
    for (int i = 0; i < NUM_THREADS; i++){
        pthread_join(threads[i], NULL);
    }

    LCD_2IN_Clear(BLACK);
    LCD_SetBacklight(0);
    DEV_ModuleExit();

    return 0;
}

/*****************************
 * 
 * Cyclic Executive to control frame rate
 * Releases Capture service S1
 * 
 ******************************/
void *Sequencer(void *threadp)
{
    struct timespec delay_time;
    delay_time.tv_sec = 0;
    delay_time.tv_nsec = SEQ_PERIOD_NS;

    int seqCnt = 0;
    while (!abortTest /*&& seqCnt < 3000*/)   // run 300 seconds at 10 Hz
    {
        nanosleep(&delay_time, NULL);
        sem_post(&semS1);
        seqCnt++;

        // Probably need to add some things to ID missed deadlines
    }

    // Cancel services
    abortS1 = TRUE;
    abortS2 = TRUE;
    abortS3 = TRUE;

    // last release for clean exit
    sem_post(&semS1);
    sem_post(&semS2);
    sem_post(&semS3);

    pthread_exit((void *)0);
}

/*****************************
 * 
 * Capture Service S1
 * Releases transform service S2
 * 
 ******************************/
void *Service_1(void *threadp)
{
    struct timespec start, stop;
    Mat local;

    while (!abortS1)
    {
        sem_wait(&semS1);
        if (abortS1) break;
        
        // Start time
        clock_gettime(CLOCK_MONOTONIC_RAW, &start);
        
        // Retrieve image
        cap >> local;

        if (local.empty()) continue;
        
        // Copy frame to new array
        pthread_mutex_lock(&raw_mutex);
        raw_frame = local.clone();
        pthread_mutex_unlock(&raw_mutex);
        
        // Stop time
        clock_gettime(CLOCK_MONOTONIC_RAW, &stop);

        // Log execution time
        syslog(LOG_INFO, ",S1,capture,%.3f,ms", delta_t_ms(start, stop));
        sem_post(&semS2);
    }

    pthread_exit((void *)0);
}

/*****************************
 * 
 * Transform Service S2
 * Releases display service S3
 * 
 ******************************/
void *Service_2(void *threadp)
{
    struct timespec start, stop;
    Mat local, resized;

    while (!abortS2)
    {
        sem_wait(&semS2);
        if (abortS2) break;

        pthread_mutex_lock(&raw_mutex);
        if (!raw_frame.empty()) local = raw_frame.clone();
        pthread_mutex_unlock(&raw_mutex);

        if (local.empty()) continue;

        clock_gettime(CLOCK_MONOTONIC_RAW, &start);

        resize(local, resized, Size(LCD_2IN_HEIGHT, LCD_2IN_WIDTH));
        transpose(resized, resized);
        flip(resized, resized, 0);

        // TODO: put color filtering here ---------------------------------------------

        pthread_mutex_lock(&proc_mutex);
        processed_frame = resized.clone();
        pthread_mutex_unlock(&proc_mutex);

        clock_gettime(CLOCK_MONOTONIC_RAW, &stop);
        syslog(LOG_INFO, ",S2,process,%.3f,ms", delta_t_ms(start, stop));

        sem_post(&semS3);
    }

    pthread_exit((void *)0);
}

/*****************************
 * 
 * LCD Display Service S3
 * 
 * 
 ******************************/
void *Service_3(void *threadp)
{
    struct timespec start, stop;
    Mat local;

    while (!abortS3)
    {
        sem_wait(&semS3);
        if (abortS3) break;

        pthread_mutex_lock(&proc_mutex);
        if (!processed_frame.empty()) local = processed_frame.clone();
        pthread_mutex_unlock(&proc_mutex);

        if (local.empty()) continue;
        
        // Capture start service time
        clock_gettime(CLOCK_MONOTONIC_RAW, &start);

        // Convert to RGB565
        for (int y = 0; y < local.rows; y++)
        {
            for (int x = 0; x < local.cols; x++)
            {
                Vec3b pixel = local.at<Vec3b>(y, x);
                uint16_t pix = rgb888_to_rgb565(pixel[2], pixel[1], pixel[0]);
                int idx = 2 * (y * local.cols + x);
                lcd_buf[idx]     = (pix >> 8) & 0xFF;
                lcd_buf[idx + 1] = pix & 0xFF;
            }
        }
        
        // Push image to LCD
        LCD_2IN_Display(lcd_buf.data());
        
        // capture service stop time and log elapsed from start
        clock_gettime(CLOCK_MONOTONIC_RAW, &stop);
        syslog(LOG_INFO, ",S3,display,%.3f,ms", delta_t_ms(start, stop));
    }

    pthread_exit((void *)0);
}
