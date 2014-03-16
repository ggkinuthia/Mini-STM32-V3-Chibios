/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/
#define WAKEUP_TEST FALSE

#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "ch.h"
#include "hal.h"
//#include "test.h"

#include "shell.h"
#include "chprintf.h"
#include "chsprintf.h"
#include "chrtclib.h"

#include "gfx.h"
#include "math.h"


static RTCAlarm alarmspec;
static time_t unix_time;

/* The handles for our three consoles */
GHandle GW1, GW2, GW3;
/*
 * Red LED blinker thread, times are in milliseconds.
 */
static WORKING_AREA(waThread1, 128);
static msg_t Thread1(void *arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  while (TRUE) {
    palTogglePad(GPIOA, GPIOA_LED_1);
    chThdSleepMilliseconds(500);
    
  }
  return 0;
}

static WORKING_AREA(waThread2, 128);
static msg_t Thread2(void *arg) {
	static char smalldelay = 10;
	static uint16_t rot = 0;
    (void)arg;
	chRegSetThreadName("blinker2");
	while (TRUE) {
		if (! palReadPad(GPIOA, GPIOA_BUTTON_1)){
				smalldelay++;
				//gdispClear(Black);
				rot+=90;
				if (rot > 270) rot = 0;
				gdispSetOrientation(rot);
				//drawScreen();
				}
		palTogglePad(GPIOA, GPIOA_LED_2);
    chThdSleepMilliseconds(smalldelay);
    
  }
  return 0;
}


static void func_sleep(void){
  chSysLock();
  SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
  PWR->CR |= (PWR_CR_PDDS | PWR_CR_LPDS | PWR_CR_CSBF | PWR_CR_CWUF);
   __WFI();
}

static void cmd_sleep(BaseSequentialStream *chp, int argc, char *argv[]){
  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: sleep\r\n");
    return;
  }
  chprintf(chp, "Going to sleep.\r\n");

  chThdSleepMilliseconds(200);

  /* going to anabiosis */
  func_sleep();
}

/*
 *
 */
static void cmd_alarm(BaseSequentialStream *chp, int argc, char *argv[]){
  
  
  (void)argv;
  if (argc < 1) {
    goto ERROR;
  }

  if ((argc == 1) && (strcmp(argv[0], "get") == 0)){
    rtcGetAlarm(&RTCD1, 0, &alarmspec);
    chprintf(chp, "%D%s",alarmspec," - alarm in STM internal format\r\n");
    return;
  }

  if ((argc == 2) && (strcmp(argv[0], "set") == 0)){
    //i = atol(argv[1]);
   // alarmspec.tv_datetime = ((i / 10) & 7 << 4) | (i % 10) | RTC_ALRMAR_MSK4 |
    //                        RTC_ALRMAR_MSK3 | RTC_ALRMAR_MSK2;
		rtcGetAlarm(&RTCD1, 0, &alarmspec);
		alarmspec.tv_sec = alarmspec.tv_sec + 30;
    rtcSetAlarm(&RTCD1, 0, &alarmspec);
    return;
  }
  else{
    goto ERROR;
  }

ERROR:
  chprintf(chp, "Usage: alarm get\r\n");
  chprintf(chp, "       alarm set N\r\n");
  chprintf(chp, "where N is alarm time in seconds\r\n");
}

/*
 *
 */
static void cmd_date(BaseSequentialStream *chp, int argc, char *argv[]){
  struct tm timp;
	
  (void)argv;
	
  if (argc == 0) {
    goto ERROR;
  }

  if ((argc == 1) && (strcmp(argv[0], "get") == 0)){
    unix_time = rtcGetTimeUnixSec(&RTCD1);

  if (unix_time == -1){
      chprintf(chp, "incorrect time in RTC cell\r\n");
    }
    else{
      chprintf(chp, "%D%s",unix_time," - unix time\r\n");
      rtcGetTimeTm(&RTCD1, &timp);
      chprintf(chp, "%s%s",asctime(&timp)," - formatted time string\r\n");
    }
    return;
  }

  if ((argc == 2) && (strcmp(argv[0], "set") == 0)){
    unix_time = atol(argv[1]);
    if (unix_time > 0){
      rtcSetTimeUnixSec(&RTCD1, unix_time);
      return;
    }
    else{
      goto ERROR;
    }
  }
  else{
    goto ERROR;
  }

ERROR:
  chprintf(chp, "Usage: date get\r\n");
  chprintf(chp, "       date set N\r\n");
  chprintf(chp, "where N is time in seconds sins Unix epoch\r\n");
  chprintf(chp, "you can get current N value from unix console by the command\r\n");
  chprintf(chp, "%s", "date +\%s\r\n");
  return;
}

static SerialConfig ser_cfg = {
    115200,
    0,
    0,
    0,
};

static const ShellCommand commands[] = {
  {"alarm", cmd_alarm},
  {"date",  cmd_date},
  {"sleep", cmd_sleep},
  {NULL, NULL}
};

static const ShellConfig shell_cfg1 = {
  (BaseSequentialStream  *)&SD1,
  commands
};

/*
 * Maximum speed SPI configuration (18MHz, CPHA=0, CPOL=0, MSb first).
 */
static const SPIConfig hs_spicfg = {
  NULL,
  GPIOA,
  GPIOA_SPI1NSS,
  0
};
/*
 * Low speed SPI configuration (281.250kHz, CPHA=0, CPOL=0, MSb first).
 */
static const SPIConfig ls_spicfg = {
  NULL,
  GPIOA,
  GPIOA_SPI1NSS,
  SPI_CR1_MSTR | SPI_CR1_BR_2 | SPI_CR1_CPOL | SPI_CR1_CPHA |SPI_CR1_SSM
};

const char *tsCalibRead(uint16_t instance) {
  // This will perform a on-spot calibration
  // Unless you read and add the co-efficients here
  (void) instance;
  return NULL;
}

#define COLOR_SIZE	20
#define PEN_SIZE	20
#define OFFSET		3

#define COLOR_BOX(a)		(ev.x >= a && ev.x <= a + COLOR_SIZE)
#define PEN_BOX(a)			(ev.y >= a && ev.y <= a + COLOR_SIZE)
#define GET_COLOR(a)		(COLOR_BOX(a * COLOR_SIZE + OFFSET))
#define GET_PEN(a)			(PEN_BOX(a * 2 * PEN_SIZE + OFFSET))
#define DRAW_COLOR(a)		(a * COLOR_SIZE + OFFSET)
#define DRAW_PEN(a)			(a * 2 * PEN_SIZE + OFFSET)
#define DRAW_AREA(x, y)		(x >= PEN_SIZE + OFFSET + 3 && x <= gdispGetWidth() && \
							 y >= COLOR_SIZE + OFFSET + 3 && y <= gdispGetHeight())

void drawScreen(void) {
	char *msg = "uGFX";
	font_t		font1, font2;

	font1 = gdispOpenFont("DejaVuSans24*");
	font2 = gdispOpenFont("DejaVuSans12*");

	gdispClear(White);
	gdispDrawString(gdispGetWidth()-gdispGetStringWidth(msg, font1)-3, 3, msg, font1, Black);
	
	/* colors */
	gdispFillArea(0 * COLOR_SIZE + 3, 3, COLOR_SIZE, COLOR_SIZE, Black);	/* Black */
	gdispFillArea(1 * COLOR_SIZE + 3, 3, COLOR_SIZE, COLOR_SIZE, Red);		/* Red */
	gdispFillArea(2 * COLOR_SIZE + 3, 3, COLOR_SIZE, COLOR_SIZE, Yellow);	/* Yellow */
	gdispFillArea(3 * COLOR_SIZE + 3, 3, COLOR_SIZE, COLOR_SIZE, Green);	/* Green */
	gdispFillArea(4 * COLOR_SIZE + 3, 3, COLOR_SIZE, COLOR_SIZE, Blue);		/* Blue */
	gdispDrawBox (5 * COLOR_SIZE + 3, 3, COLOR_SIZE, COLOR_SIZE, Black);	/* White */

	/* pens */	
	gdispFillStringBox(OFFSET * 2, DRAW_PEN(1), PEN_SIZE, PEN_SIZE, "1", font2, White, Black, justifyCenter);
	gdispFillStringBox(OFFSET * 2, DRAW_PEN(2), PEN_SIZE, PEN_SIZE, "2", font2, White, Black, justifyCenter);
	gdispFillStringBox(OFFSET * 2, DRAW_PEN(3), PEN_SIZE, PEN_SIZE, "3", font2, White, Black, justifyCenter);
	gdispFillStringBox(OFFSET * 2, DRAW_PEN(4), PEN_SIZE, PEN_SIZE, "4", font2, White, Black, justifyCenter);
	gdispFillStringBox(OFFSET * 2, DRAW_PEN(5), PEN_SIZE, PEN_SIZE, "5", font2, White, Black, justifyCenter);
	
	gdispCloseFont(font1);
	gdispCloseFont(font2);
}

GEventMouse		ev;


/*
 * Application entry point.
 */
int main(void) {
	color_t color = Black;
	uint16_t pen = 0;
   char buffs[40]={'\0'};
   struct tm timp;
   static WORKING_AREA(waShell, 1024);
  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();
  /*
   * SPI1 I/O pins setup on PORTA
   */
  palSetPadMode(GPIOA, 5, PAL_MODE_STM32_ALTERNATE_PUSHPULL);     /* SCK. */
  palSetPadMode(GPIOA, 6, PAL_MODE_STM32_ALTERNATE_PUSHPULL);     /* MISO.*/
  palSetPadMode(GPIOA, 7, PAL_MODE_STM32_ALTERNATE_PUSHPULL);     /* MOSI.*/
  palSetPadMode(GPIOB, 6, PAL_MODE_OUTPUT_PUSHPULL); /*flash CS*/
  palSetPad(GPIOB, 6);
  
  spiStart(&SPID1, &ls_spicfg);       /* Setup transfer parameters.       */
   
   /*
   * Creates the blinker thread1.
   */
	chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);
	chThdCreateStatic(waThread2, sizeof(waThread2), NORMALPRIO, Thread2, NULL);
	
  /*
   * Activates the serial driver 2 using the driver default configuration.
   */
  sdStart(&SD1, &ser_cfg);
	/* Shell initialization.*/
  shellInit();
  
  shellCreateStatic(&shell_cfg1, waShell, sizeof(waShell), NORMALPRIO);
  
	

	/* initialize and clear the display */
	gfxInit();	
	 
	
    
     /* Calibrate the touchscreen */
	//ginputSetMouseCalibrationRoutines(0, NULL, tsCalibRead, FALSE);
	
	ginputGetMouse(0);
	gdispSetOrientation(GDISP_ROTATE_0);

	drawScreen();

	while (TRUE) {
		ginputGetMouseStatus(0, &ev);
		if (!(ev.current_buttons & GINPUT_MOUSE_BTN_LEFT))
			continue;

		/* inside color box ? */
		if(ev.y >= OFFSET && ev.y <= COLOR_SIZE) {
			     if(GET_COLOR(0)) 	color = Black;
			else if(GET_COLOR(1))	color = Red;
			else if(GET_COLOR(2))	color = Yellow;
			else if(GET_COLOR(3))	color = Green;
			else if(GET_COLOR(4))	color = Blue;
			else if(GET_COLOR(5))	color = White;
		
		/* inside pen box ? */
		} else if(ev.x >= OFFSET && ev.x <= PEN_SIZE) {
			     if(GET_PEN(1))		pen = 0;
			else if(GET_PEN(2))		pen = 1;
			else if(GET_PEN(3))		pen = 2;
			else if(GET_PEN(4))		pen = 3;
			else if(GET_PEN(5))		pen = 4;		

		/* inside drawing area ? */
		} else if(DRAW_AREA(ev.x, ev.y)) {
			if(pen == 0)
				gdispDrawPixel(ev.x, ev.y, color);
			else
				gdispFillCircle(ev.x, ev.y, pen, color);
		}
	}   

	
    
	gdispClear(Green);	
	while (TRUE) {
			//if (! palReadPad(GPIOA, GPIOA_BUTTON_2))
    			//	TestThread(&SD1);
			memset(buffs,'\0',40);
			unix_time = rtcGetTimeUnixSec(&RTCD1);
					
		//	gwinPrintf(GW3,"%d\r\n",unix_time);	
			rtcGetTimeTm(&RTCD1, &timp);
			chsprintf(buffs, "%s",asctime(&timp));
      		//gwinPrintf(GW1,buffs);	
			//gdispDrawString(0, 0, "buffs", font1, Red);
			
			//font = gdispOpenFont("UI2");
			
			//gdispDrawStringBox(0,gdispGetHeight()/2,gdispGetWidth(),20, buffs, font2, Green,justifyCenter);
			//chThdSleepMilliseconds(1000);
			//gdispControl(GDISP_CONTROL_ORIENTATION,GDISP_ROTATE_LANDSCAPE);
		//	gdispClear(Black);
  }

  
}
