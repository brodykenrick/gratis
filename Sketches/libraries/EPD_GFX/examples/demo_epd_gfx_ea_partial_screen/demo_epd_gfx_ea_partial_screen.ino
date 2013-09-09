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


// graphic temperature display

// Operation from reset:
// * display version
// * display compiled-in display setting
// * display FLASH detected or not
// * display temperature (displayed before every image is changed)
// * clear screen
// * update display (temperature)
// * delay 60 seconds (flash LED)
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


// graphic handler
#ifndef EMBEDDED_ARTISTS
EPD_GFX G_EPD(EPD, S5813A);
#else /* EMBEDDED_ARTISTS */
EPD_GFX G_EPD(EPD, LM75A);
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

        Serial.print("availableMemory() = ");
        Serial.println(availableMemory());

	// set up graphics EPD library
	// and clear the screen
	G_EPD.begin();

        Serial.println( "Screen cleared." );
}

static int counter = 0;
// main loop
void loop() {
	int h = G_EPD.real_height();
	int hp = G_EPD.height();
	int w = G_EPD.width();
        int vertical_pages = G_EPD.vertical_pages;

	Serial.print("H=");
	Serial.print(h);
	Serial.print(" (with ");
	Serial.print(vertical_pages);
	Serial.print(" pages of ");
	Serial.print(hp);
	Serial.print(") : W=");
	Serial.println(w);
        Serial.flush();
        
        for(int i=0; i < vertical_pages; i++)
        {
          int starting_row_this_page = i * hp;
          Serial.print("Vertical Page ");Serial.println( i );Serial.flush();
          G_EPD.set_vertical_page(i);
          Serial.println("Drawing Rect.");Serial.flush();
          //Rectangle only on page 0
          //Optimise a little by only executing on the first page
          if (i == 0)
          {
            G_EPD.drawRect(counter%(w/2), 0, w/2, hp, EPD_GFX::BLACK);
          }
          
          //Rectangle in middle of screen across pages
          //We could limit to just the pages it is on -- but we can also just call for each page (at the cost of wasted drawPixels that do nothing useful)
          G_EPD.drawRect((counter + w/5)%(w/2), hp/2, w/3, h/2, EPD_GFX::BLACK);
        
          Serial.println("Drawing Line.");Serial.flush();
          //Vertical line down entire screen
          G_EPD.drawLine( w*3/4, 0, w*3/4, h-1, EPD_GFX::BLACK);   //This is inclusive so has dest pixels inside the border

          Serial.println("Text.");Serial.flush();
          //Write text on a few pages
          if(vertical_pages>=3)
          {
            if(( i == 0 ) || ( i == (vertical_pages-2) ) || ( i == (vertical_pages-1) ))
            {
              char temp[] = "Each";
              int x = 2;
              int y = 1 + starting_row_this_page; //Put on each page
              int char_size_multiplier = 2;
              for (int j = 0; j < sizeof(temp) - 1; ++j, x += (char_size_multiplier * (7)))
              {
                  Serial.print("Letter=");Serial.println( temp[j] );Serial.flush();
                  G_EPD.drawChar(x, y, temp[j], EPD_GFX::BLACK, EPD_GFX::WHITE, char_size_multiplier );
              }
            }
          }
  
          //Write text across pages
          {
            char temp[] = "Across";
            int x = w/16;
            int y = h/6;
            int char_size_multiplier = 6;
            for (int j = 0; j < sizeof(temp) - 1; ++j, x += (char_size_multiplier * (7)))
            {
                Serial.print("Letter=");Serial.println( temp[j] );Serial.flush();
                G_EPD.drawChar(x, y, temp[j], EPD_GFX::BLACK, EPD_GFX::WHITE, char_size_multiplier );
    	    }
          }
  
          Serial.println( "Display this page" );
          Serial.flush();
          // update the display
  	  G_EPD.display( );
        }
        counter++;        counter++;        counter++;        counter++;        counter++;
        
          Serial.println( "Finished loop" );
          Serial.flush();

	// flash LED for a number of seconds
	for (int x = 0; x < LOOP_DELAY_SECONDS * 10; ++x)
        {
		  digitalWrite(Pin_RED_LED, LED_ON);
		  delay(50);
		  digitalWrite(Pin_RED_LED, LED_OFF);
		  delay(50);
	}

	Serial.println("Clearing.");Serial.flush();        
      	G_EPD.clear( );
}

