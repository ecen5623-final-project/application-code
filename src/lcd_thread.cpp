/*********************
 * 
 * 
 * 
 */

#include <unistd.h>
#include <stdio.h>
#include <csignal>
#include <vector>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/videoio/videoio.hpp>
#include "display_buffer.h"
#include "realtime.h"

#define TIMEOUT 75000000

// Waveshare headers
extern "C" {
    #include "LCD_2inch.h"
    #include "DEV_Config.h"
}

using namespace cv;
using namespace std;

extern DisplayBuffer display_buffer;
extern volatile sig_atomic_t g_stop;

// RGB88 to 565 conversion -------------------------------------------------------------- May want to push this to the transform service?
static inline uint16_t rgb888_to_rgb565(unsigned char r, unsigned char g, unsigned char b)
{
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// LCD Initilization function
void lcd_init(void)
{
    /* SPI Module Init */
    if(DEV_ModuleInit() != 0){
	DEV_ModuleExit();
	exit(0);
    }
    
    // LCD Init
    LCD_2IN_Init();
    LCD_2IN_Clear(WHITE);
    LCD_SetBacklight(1023);
}

// LCD and SPI Stop
void lcd_stop(void)
{
    LCD_2IN_Clear(BLACK);
    LCD_SetBacklight(0);
    DEV_ModuleExit();
}

// LCD Thread
void* lcd_thread(void* arg)
{
    // struct timespec start, stop;
    static vector<uint8_t> lcd_buf(LCD_2IN_WIDTH * LCD_2IN_HEIGHT * 2);
    Mat frame_lcd;
    unsigned long last_seq = 0;
    
    lcd_init();

    while (!g_stop)
    {
		// Get from buffer:
		if (!display_buffer.get(frame_lcd, last_seq, TIMEOUT)) continue;
		if (frame_lcd.empty()) continue;
			
		last_seq++;

		// Convert to RGB565
		for (int y = 0; y < frame_lcd.rows; y++)
		{
			for (int x = 0; x < frame_lcd.cols; x++)
			{
				Vec3b pixel = frame_lcd.at<Vec3b>(y, x);
				uint16_t pix = rgb888_to_rgb565(pixel[2], pixel[1], pixel[0]);
				int idx = 2 * (y * frame_lcd.cols + x);
				lcd_buf[idx] = (pix >> 8) & 0xFF;
				lcd_buf[idx + 1] = pix & 0xFF;
			}
		}

		// Write frame to LCD
		LCD_2IN_Display(lcd_buf.data());
    }

    
    lcd_stop();
    return NULL;	
}
