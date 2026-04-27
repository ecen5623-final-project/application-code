/*********************
 * 
 * 
 * 
 */

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <csignal>
#include <vector>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/videoio/videoio.hpp>
#include "display_buffer.h"
#include "realtime.h"

#include <time.h> //for logging timestamps of WCET
#include <syslog.h>

#define TIMEOUT 106250000

// Waveshare headers
extern "C" {
    #include "LCD_2inch.h"
    #include "DEV_Config.h"
}

using namespace cv;
using namespace std;

extern DisplayBuffer display_buffer;
extern volatile sig_atomic_t g_stop;

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
    pin_to_core(3);
    set_fifo_prio(95);
    static vector<uint8_t> lcd_buf(LCD_2IN_WIDTH * LCD_2IN_HEIGHT * 2);
    Mat frame_lcd;
    unsigned long last_seq = 0;
    
    struct timespec frame_start, frame_end;
    
    lcd_init();

    while (!g_stop)
    {
		clock_gettime(CLOCK_MONOTONIC, &frame_start);
		
		// Get from buffer:
		if (!display_buffer.get(frame_lcd, last_seq, TIMEOUT)) continue;
		if (frame_lcd.empty()) continue;
				
		last_seq++;

		// Pack BGR888 -> RGB565 with OpenCV's cvtColor. It's SIMD-vectorized,
		// so it runs an order of magnitude faster than the per-pixel
		// Mat::at<Vec3b> loop this replaces, which was the main S3 bottleneck.
		// The ST7789 expects big-endian pixels over SPI but cvtColor packs
		// them little-endian, hence the byte-swap loop below.
		//
		// Things to check on hardware:
		//   - If reds and blues look swapped, try COLOR_BGR2RGB565 instead.
		//   - The byte-swap loop can go away if we send RAMCTRL (cmd 0xB0)
		//     during init to put the panel in little-endian mode. Left as
		//     a follow-up since it needs datasheet verification for this
		//     exact Waveshare board.
		static Mat rgb565;
		cvtColor(frame_lcd, rgb565, COLOR_BGR2BGR565);

		const size_t nbytes = (size_t)rgb565.rows * rgb565.cols * 2;
		const uint8_t* src = rgb565.ptr<uint8_t>(0);
		uint8_t* dst = lcd_buf.data();
		for (size_t i = 0; i < nbytes; i += 2) {
			dst[i]     = src[i + 1];
			dst[i + 1] = src[i];
		}

		// Write frame to LCD
		LCD_2IN_Display(lcd_buf.data());
			
		clock_gettime(CLOCK_MONOTONIC, &frame_end);
		syslog(LOG_DEBUG, ",lcd_thread,%.3f,ms", delta_t_ms(frame_start, frame_end));
    }

    
    lcd_stop();
    return NULL;	
}
