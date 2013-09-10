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


// Graphic Temperature Display

// Operation from reset:
// * display version
// * display compiled-in display setting
// * display FLASH detected or not
// * display temperature (displayed before every image is changed)
// * clear screen
// * update display (temperature)
// * delay some seconds (flash LED)
// * back to update display

//Brody Kenrick modified this to support Arduino Uno (slower and less efficient displaying)


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
//Note: This include is affected by SCREEN_SIZE affected defines
#include <EPD_GFX.h>


// update delay in seconds
#define LOOP_DELAY_SECONDS 5


// current version number
#define THERMO_VERSION "3.BK.partial_screen"


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


#define HEIGHT_OF_SEGMENT (16) //<!Proportional to the memory used and inversely proportional to prcessing to display (and also has visual impact adds some delay). If your Arduino hangs at startup reduce this (BUT must be a factor of the screne size)...
// Graphic handler
//TODO: Move this into setup/loop so that we can create a visible error if we run out of memory
#ifndef EMBEDDED_ARTISTS
EPD_GFX G_EPD(EPD, EPD_WIDTH, EPD_HEIGHT, S5813A, HEIGHT_OF_SEGMENT);
#else /* EMBEDDED_ARTISTS */
EPD_GFX G_EPD(EPD, EPD_WIDTH, EPD_HEIGHT, LM75A, HEIGHT_OF_SEGMENT);
#endif /* EMBEDDED_ARTISTS */

// free RAM check for debugging. SRAM for ATmega328p = 2048Kb.
int availableMemory() {
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
	Serial.println();
	Serial.println("Thermo version: " THERMO_VERSION);
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

	// set up graphics EPD library
	// and clear the screen
	G_EPD.begin();

        Serial.println( "Screen cleared." );
}


// main loop
void loop() {
        long start_loop_ms = millis();
#ifndef EMBEDDED_ARTISTS
        int temperature = S5813A.read();
#else /* EMBEDDED_ARTISTS */
        int temperature = LM75A.read();
#endif /* EMBEDDED_ARTISTS */

	Serial.print("Temperature = ");
	Serial.print(temperature);
	Serial.println(" Celcius");
  
	int h        = G_EPD.real_height();
	int seg_h    = G_EPD.height();
	int w        = G_EPD.width();
        unsigned int segments = G_EPD.get_segment_count();

	Serial.print("Height=");
	Serial.print(h);
	Serial.print(" (with ");
	Serial.print(segments);
	Serial.print(" segments of ");
	Serial.print(seg_h);
	Serial.print(") : Width=");
	Serial.println(w);
        
        //Always cleared before we get here.
        Serial.println( "-----------------------------------------------" );

        for(unsigned int s=0; s < segments; s++)
        {
          Serial.print( "Segment " );Serial.print( s );
          Serial.print(" | ");
          G_EPD.set_current_segment(s);

          //Below is the original thermo code
          G_EPD.drawRect(1, 1, w - 2, h - 2, EPD_GFX::BLACK);
          G_EPD.drawRect(3, 3, w - 6, h - 6, EPD_GFX::BLACK);

          //Note -these are not proportionally to the screen size.
  	  G_EPD.fillTriangle(135,20, 186,40, 152,84, EPD_GFX::BLACK);
  	  G_EPD.fillTriangle(139,26, 180,44, 155,68, EPD_GFX::WHITE);

  	  char temp[sizeof("-999 C")];
  	  snprintf(temp, sizeof(temp), "%4d C", temperature);
  
          int x = 20;
          int y = 30;
          for (unsigned int i = 0; i < sizeof(temp) - 1; ++i, x += 14) {
              G_EPD.drawChar(x, y, temp[i], EPD_GFX::BLACK, EPD_GFX::WHITE, 2);
          }
  
          // small circle for degrees symbol
          G_EPD.drawCircle(20 + 4 * 14 + 6, 30, 4, EPD_GFX::BLACK);

  // 100 difference just to simplify things
  // so 1 pixel = 1 degree
  #define T_MIN (-10)
  #define T_MAX 80
  
          // clip
          if (temperature < T_MIN) {
  		temperature= T_MIN;
          } else if (temperature > T_MAX) {
  		temperature = T_MAX;
          }
  
          // temperature bar
          int bar_w = temperature - T_MIN;  // zero based
          int bar_h = 4;
          int bar_x0 = 24;
          int bar_y0 = 60;
  
          G_EPD.fillRect(bar_x0, bar_y0, T_MAX - T_MIN, bar_h, EPD_GFX::WHITE);
          G_EPD.fillRect(bar_x0, bar_y0, bar_w, bar_h, EPD_GFX::BLACK);
  
          // scale
          for (int t0 = T_MIN; t0 < T_MAX; t0 += 5) {
          	int t = t0 - T_MIN;
          	int tick = 8;
          	if (0 == t0) {
          		tick = 12;
          		G_EPD.drawCircle(bar_x0 + t, bar_y0 + 16, 3, EPD_GFX::BLACK);
          	} else if (0 == t0 % 10) {
          		tick = 10;
          	}
          	G_EPD.drawLine(bar_x0 + t, bar_y0 + tick, bar_x0 + t, bar_y0 + 6, EPD_GFX::BLACK);
          	G_EPD.drawLine(bar_x0 + t, bar_y0 + 6, bar_x0 + t + 5, bar_y0 + 6, EPD_GFX::BLACK);
          	G_EPD.drawLine(bar_x0 + t + 5, bar_y0 + 6, bar_x0 + t + 5, bar_y0 + 8, EPD_GFX::BLACK);
          }

          Serial.print( "Display segment " );Serial.println( s );
          // Update the display -- first and last segments of a loop are indicated
          G_EPD.display( false, s==0, s==(segments-1) );
        }

        Serial.println( "++++++++++++++++++++++++++++++++++++++++++++++++++" );

        Serial.print("Total display rendering in ms = ");
        Serial.println( millis() - start_loop_ms );
        
        Serial.println( "Delay with LED flashing." );

	// flash LED for a number of seconds
        for (int x = 0; x < LOOP_DELAY_SECONDS * 10; ++x)
        {
		  digitalWrite(Pin_RED_LED, LED_ON);
		  delay(50);
		  digitalWrite(Pin_RED_LED, LED_OFF);
		  delay(50);
	}

	Serial.println("Clearing.");
        G_EPD.clear( );
}

