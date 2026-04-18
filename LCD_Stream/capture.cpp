/*
 *
 *  Example by Sam Siewert 
 *
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <stdint.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core_c.h>

// Waveshare headers
extern "C" {
    #include "LCD_2inch.h"
    #include "DEV_Config.h"
}

using namespace cv;
using namespace std;

static uint16_t rgb888_to_rgb565(unsigned char r, unsigned char g, unsigned char b)
{
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}


int main(int argc, char** argv)
{
    /* Module Init */
	if(DEV_ModuleInit() != 0){
        DEV_ModuleExit();
        exit(0);
    }

    LCD_2IN_Init();
    LCD_2IN_Clear(WHITE);
    LCD_SetBacklight(1023);

    CvCapture* capture = cvCreateCameraCapture(0);
    IplImage* frame;

    if (!capture)
        return -1;

    // LCD buffer: 240 x 320
    vector<uint8_t> lcd_buf(240 * 320 * 2);

    while (1)
    {
        frame = cvQueryFrame(capture);
        if (!frame) break;

        // Convert old IplImage* to cv::Mat
        Mat src = cvarrToMat(frame);

        // Resize to LCD size and rotate
        Mat resized;
        resize(src, resized, Size(LCD_2IN_HEIGHT, LCD_2IN_WIDTH));
        transpose(resized, resized);
        flip(resized, resized, 0);   // counter-clockwise

        for (int y = 0; y < resized.rows; y++)
        {
            for (int x = 0; x < resized.cols; x++)
            {
                Vec3b pixel = resized.at<Vec3b>(y, x);
                uint16_t pix = rgb888_to_rgb565(pixel[2], pixel[1], pixel[0]);
                int idx = 2 * (y * resized.cols + x);
                lcd_buf[idx]     = (pix >> 8) & 0xFF;
                lcd_buf[idx + 1] = pix & 0xFF;
            }
        }

        LCD_2IN_Display(lcd_buf.data());

        char c = cvWaitKey(33);
        if (c == 27) break;
    }

    cvReleaseCapture(&capture);
    DEV_ModuleExit();

    return 0;
}
