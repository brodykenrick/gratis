#define EMBEDDED_ARTISTS
// -*- mode: c++ -*-
// Copyright 2013 Pervasive Displays, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at:
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
// express or implied.  See the License for the specific language
// governing permissions and limitations under the License.


// Brody Kenrick
// Quick demo of updating of whole display using partial screen segments

// Operation from reset:
// * display version
// * display compiled-in display setting
// * clear screen
// * update display -- loops over some display code
// * delay X seconds (flash LED)
// * back to update display
#include <assert.h>

#include <inttypes.h>
#include <ctype.h>

// required libraries
#include <SPI.h>
#ifndef EMBEDDED_ARTISTS
#include <FLASH.h>
#endif /* EMBEDDED_ARTISTS */
#include <EPD.h>
//Temperature sensor
#ifndef EMBEDDED_ARTISTS
#include <S5813A.h>
#else /* EMBEDDED_ARTISTS */
#include <Wire.h>
#include <LM75A.h>
#endif /* EMBEDDED_ARTISTS */
#include <Adafruit_GFX.h>


// Change this for different display size
// supported sizes: 144 200 270
//#define SCREEN_SIZE 0
#define SCREEN_SIZE 270 //BK

#if (SCREEN_SIZE == 144)
#define EPD_SIZE EPD_1_44
#error "Not supported yet. Change/extend...."
#define EPD_WIDTH (-1)
#define EPD_HEIGHT (-1)

#elif (SCREEN_SIZE == 200)
#define EPD_SIZE EPD_2_0
#error "Not supported yet. Change/extend...."
#define EPD_WIDTH (-1)
#define EPD_HEIGHT (-1)

#elif (SCREEN_SIZE == 270)
#define EPD_SIZE EPD_2_7
#define EPD_WIDTH (264)
#define EPD_HEIGHT (176)

#else
#error "Unknown EPB size: Change the #define SCREEN_SIZE to a supported value"
#endif

//Note: This include is affected by EMBEDDED_ARTISTS define
#include <EPD_GFX.h>

//! Height in pixels of each segment in the EPD buffer. Proportional to the memory used and inversely proportional to processing to display. Also has visual impact and adds some delay in rendering.
//!If your Arduino hangs at startup reduce this (BUT must be a factor of the screen size -- you can start with 1).
#define HEIGHT_OF_SEGMENT (16)

#define PROFILE
#if defined(PROFILE)
#include "Stopwatch.h"
#endif //defined(PROFILE)

// Update delay in seconds
#define LOOP_DELAY_SECONDS (10)

#define VERBOSE

// current version number
#define DEMO_VERSION "1"


#if defined(__MSP430_CPU__)

// TI LaunchPad IO layout
const int Pin_TEMPERATURE = A4;
const int Pin_PANEL_ON = P2_3;
const int Pin_BORDER = P2_5;
const int Pin_DISCHARGE = P2_4;
const int Pin_PWM = P2_1;
const int Pin_RESET = P2_2;
const int Pin_BUSY = P2_0;
const int Pin_EPD_CS = P2_6;
const int Pin_FLASH_CS = P2_7;
const int Pin_SW2 = P1_3;
const int Pin_RED_LED = P1_0;

#else

// Arduino IO layout
#ifndef EMBEDDED_ARTISTS
const int Pin_TEMPERATURE = A0;
#endif /* ! EMBEDDED_ARTISTS */
const int Pin_PANEL_ON = 2;
const int Pin_BORDER = 3;
const int Pin_DISCHARGE = 4;
const int Pin_PWM = 5;
const int Pin_RESET = 6;
const int Pin_BUSY = 7;
const int Pin_EPD_CS = 8;
#ifndef EMBEDDED_ARTISTS
const int Pin_FLASH_CS = 9;
const int Pin_SW2 = 12;
#endif /* ! EMBEDDED_ARTISTS */
const int Pin_RED_LED = 13;

#endif


// LED anode through resistor to I/O pin
// LED cathode to Ground
#define LED_ON  HIGH
#define LED_OFF LOW


// pre-processor convert to string
#define MAKE_STRING1(X) #X
#define MAKE_STRING(X) MAKE_STRING1(X)


// define the E-Ink display
EPD_Class EPD(EPD_SIZE, Pin_PANEL_ON, Pin_BORDER, Pin_DISCHARGE, Pin_PWM, Pin_RESET, Pin_BUSY, Pin_EPD_CS);
#ifdef EMBEDDED_ARTISTS
LM75A_Class LM75A;
#endif /* EMBEDDED_ARTISTS */


// Graphic handler
//TODO: Move this into setup/loop so that we can create a visible error if we run out of memory
#ifndef EMBEDDED_ARTISTS
EPD_GFX G_EPD(EPD, EPD_WIDTH, EPD_HEIGHT, S5813A, HEIGHT_OF_SEGMENT);
#else /* EMBEDDED_ARTISTS */
EPD_GFX G_EPD(EPD, EPD_WIDTH, EPD_HEIGHT, LM75A, HEIGHT_OF_SEGMENT);
#endif /* EMBEDDED_ARTISTS */

// free RAM check for debugging. SRAM for ATmega328p = 2048Kb.
int availableMemory()
{
    // Use 1024 with ATmega168
    int size = 2048;
    byte *buf;
    while ((buf = (byte *) malloc(--size)) == NULL);
        free(buf);
    return size;
}

// I/O setup
void setup() {
	pinMode(Pin_RED_LED, OUTPUT);
#ifndef EMBEDDED_ARTISTS
	pinMode(Pin_SW2, INPUT);
	pinMode(Pin_TEMPERATURE, INPUT);
#endif /* ! EMBEDDED_ARTISTS */
	pinMode(Pin_PWM, OUTPUT);
	pinMode(Pin_BUSY, INPUT);
	pinMode(Pin_RESET, OUTPUT);
	pinMode(Pin_PANEL_ON, OUTPUT);
	pinMode(Pin_DISCHARGE, OUTPUT);
	pinMode(Pin_BORDER, OUTPUT);
	pinMode(Pin_EPD_CS, OUTPUT);
#ifndef EMBEDDED_ARTISTS
	pinMode(Pin_FLASH_CS, OUTPUT);
#endif /* ! EMBEDDED_ARTISTS */

	digitalWrite(Pin_RED_LED, LOW);
	digitalWrite(Pin_PWM, LOW);
	digitalWrite(Pin_RESET, LOW);
	digitalWrite(Pin_PANEL_ON, LOW);
	digitalWrite(Pin_DISCHARGE, LOW);
	digitalWrite(Pin_BORDER, LOW);
	digitalWrite(Pin_EPD_CS, LOW);
#ifndef EMBEDDED_ARTISTS
	digitalWrite(Pin_FLASH_CS, HIGH);
#endif /* EMBEDDED_ARTISTS */
	Serial.begin(9600);
#if !defined(__MSP430_CPU__)
	// wait for USB CDC serial port to connect.  Arduino Leonardo only
	while (!Serial) {
	}
#endif
	Serial.println();
	Serial.println("EPD_GFX Partial Screen Demo.");
	Serial.println("Version: " DEMO_VERSION);
	Serial.println("Display: " MAKE_STRING(EPD_SIZE));
	Serial.println();

#ifndef EMBEDDED_ARTISTS
	FLASH.begin(Pin_FLASH_CS);
	if (FLASH.available()) {
		Serial.println("FLASH chip detected OK");
	} else {
		Serial.println("unsupported FLASH chip");
	}

	// configure temperature sensor
	S5813A.begin(Pin_TEMPERATURE);
#endif /* ! EMBEDDED_ARTISTS */

#if defined(VERBOSE)
	// get the current temperature
#ifndef EMBEDDED_ARTISTS
	int temperature = S5813A.read();
#else /* EMBEDDED_ARTISTS */
        int temperature = LM75A.read();
#endif /* EMBEDDED_ARTISTS */
	Serial.print("Temperature = ");
	Serial.print(temperature);
	Serial.println(" Celcius");

        Serial.print("Memory (SRAM) available = ");
        Serial.print(availableMemory());
        Serial.println(" bytes.");
        
        Serial.print("Memory (SRAM) allocated in display = ");
        Serial.print( G_EPD.get_segment_buffer_size_bytes() );
        Serial.println(" bytes.");
#endif //defined(VERBOSE)

	// set up graphics EPD library
	// and clear the screen
	G_EPD.begin();
#if defined(VERBOSE)
        Serial.println( "Screen cleared." );
#endif //defined(VERBOSE)
}

#if defined(PROFILE)
CStopwatch stopwatch_loop_overall("LoopOverall");
CStopwatch stopwatch_loop_excl_delay_clear("LoopExclDelayClear");
CStopwatch stopwatch_loop_display_only("LoopDisplayOnly");
CStopwatch stopwatch_loop_draw_only("LoopDrawOnly");
#endif //defined(PROFILE)

static int counter = 0;
// main loop
void loop() {
#if defined(PROFILE)
        stopwatch_loop_overall.Start();
        stopwatch_loop_excl_delay_clear.Start();
#endif //defined(PROFILE)
        
	int h        = G_EPD.real_height();
	int seg_h    = G_EPD.height();
	int w        = G_EPD.width();
        unsigned int segments = G_EPD.get_segment_count();

#if defined(VERBOSE)
	Serial.print("Height=");
	Serial.print(h);
	Serial.print(" (with ");
	Serial.print(segments);
	Serial.print(" segments of ");
	Serial.print(seg_h);
	Serial.print(") : Width=");
	Serial.println(w);
#endif //defined(VERBOSE)        
        
        //Always cleared before we get here.
#if defined(VERBOSE)
        Serial.println( "-----------------------------------------------" );
#endif //defined(VERBOSE)

        for(unsigned int s=0; s < segments; s++)
        {
#if defined(PROFILE)
          stopwatch_loop_draw_only.Start();
#endif //defined(PROFILE)
          int starting_row_this_segment = s * seg_h;
#if defined(VERBOSE)
          Serial.print( "Segment " );Serial.print( s );
          Serial.print(" | ");
#endif //defined(VERBOSE)

          G_EPD.set_current_segment(s);
#if defined(VERBOSE)
          Serial.print("Drawing:");
          Serial.print(" Rect.");
#endif //defined(VERBOSE)
          //Rectangle only on segment 0. Make it slide across the screen on each loop
          //Optimise a little by only executing on the first segment
          if (s == 0)
          {
            G_EPD.drawRect(counter%(w/2), 0, w/2, seg_h, EPD_GFX::BLACK);
          }
          
          //Rectangle in middle of screen across segments
          //We could limit to just the segments it is on -- but we can also just call for each segment (at the cost of wasted drawPixels that do nothing useful)
          G_EPD.drawRect((counter + w/5)%(w/2), seg_h/2, w/3, h/2, EPD_GFX::BLACK);
#if defined(VERBOSE)
          Serial.print(" Line.");
#endif //defined(VERBOSE)
          //Vertical line down entire screen
          G_EPD.drawLine( w*3/4, 0, w*3/4, h-1, EPD_GFX::BLACK);   //This is inclusive so has dest pixels inside the border

#if defined(VERBOSE)
          Serial.print(" Text=");
#endif //defined(VERBOSE)
          //Write text on a some segments (if big enough....)
          if(( s == 0 ) ||
            ((segments>=3) && (( s == (segments-2) ) || ( s == (segments-1) ))) ||
            ((segments>=5) && (( s == (segments-3) ) || ( s == (segments-4) )))
          )
          {
            if(seg_h >= EPD_GFX_CHAR_BASE_HEIGHT)
            {
              char temp[] = "Some";
#if defined(VERBOSE)
              Serial.print(temp);
              Serial.print(".");
#endif //defined(VERBOSE)
              unsigned int x = 2;
              unsigned int y = 1 + starting_row_this_segment; //Put on each segment
              unsigned int char_size_multiplier = (seg_h/EPD_GFX_CHAR_BASE_HEIGHT); //Make as big as we can
              for (unsigned int j = 0; j < sizeof(temp) - 1; ++j, x += (char_size_multiplier * (EPD_GFX_CHAR_BASE_WIDTH + 1)))
              {
                  //Serial.print("Letter=");Serial.println( temp[j] );
                  G_EPD.drawChar(x, y, temp[j], EPD_GFX::BLACK, EPD_GFX::WHITE, char_size_multiplier );
              }
            }
          }

#if defined(VERBOSE)
          Serial.print(" Text=");
#endif //defined(VERBOSE)

          //Write text across segments
          {
            char temp[] = "Across";
#if defined(VERBOSE)
            Serial.print(temp);
            Serial.print(".");
#endif //defined(VERBOSE)
            unsigned int x = w/16;
            unsigned int y = h/6;
            unsigned int char_size_multiplier = 6;
            for (unsigned int j = 0; j < sizeof(temp) - 1; ++j, x += (char_size_multiplier * (EPD_GFX_CHAR_BASE_WIDTH + 1)))
            {
                //Serial.print("Letter=");Serial.println( temp[j] );
                G_EPD.drawChar(x, y, temp[j], EPD_GFX::BLACK, EPD_GFX::WHITE, char_size_multiplier );
    	    }
          }
#if defined(VERBOSE)
          Serial.println();
#endif //defined(VERBOSE)

#if defined(PROFILE)
          stopwatch_loop_draw_only.Stop();
#endif //defined(PROFILE)

#if defined(PROFILE)
          stopwatch_loop_display_only.Start();
#endif //defined(PROFILE)

#if defined(VERBOSE)
          Serial.print( "Display segment " );Serial.println( s );
#endif //defined(VERBOSE)
          // Update the display -- first and last segments of a loop are indicated
          G_EPD.display( false, s==0, s==(segments-1) );
#if defined(PROFILE)
          stopwatch_loop_display_only.Stop();
#endif //defined(PROFILE)
          
        }
        counter+=5;

#if defined(PROFILE)
          stopwatch_loop_excl_delay_clear.Stop();
#endif //defined(PROFILE)

#if defined(VERBOSE)
        Serial.println( "++++++++++++++++++++++++++++++++++++++++++++++++++" );
        Serial.println( "Delay with LED flashing." );
#endif //defined(VERBOSE)


	// flash LED for a number of seconds
	for (int x = 0; x < LOOP_DELAY_SECONDS * 10; ++x)
        {
		  digitalWrite(Pin_RED_LED, LED_ON);
		  delay(50);
		  digitalWrite(Pin_RED_LED, LED_OFF);
		  delay(50);
	}
#if defined(VERBOSE)
	Serial.println("Clearing.");
#endif //defined(VERBOSE)

      	G_EPD.clear( );
#if defined(PROFILE)
        stopwatch_loop_overall.Stop();
#endif //defined(PROFILE)
#if defined(PROFILE)
        stopwatch_loop_overall.SerialPrint();
        stopwatch_loop_excl_delay_clear.SerialPrint();
        stopwatch_loop_draw_only.SerialPrint();
        stopwatch_loop_display_only.SerialPrint();
#endif //defined(PROFILE)
}


#if defined(NDEBUG)
// Assert -- Handle diagnostic informations given by assertion and abort program execution.
void __assert(const char *__func, const char *__file, int __lineno, const char *__sexp)
{
	// Report helpful information via Serial.
	Serial.print("ASSERT:");
	Serial.print(__func);
	Serial.print(":");
	Serial.print(__file);
	Serial.print(":");
	Serial.print(__lineno, DEC);
	Serial.print(":");
	Serial.print(__sexp);
	Serial.println(".");
	Serial.print("RAM = ");
	Serial.print( FreeRam() );
	Serial.println(" bytes .");

	Serial.flush();

	delay(100);

	// abort program execution.
	abort();
}
#endif /*defined(NDEBUG)*/
