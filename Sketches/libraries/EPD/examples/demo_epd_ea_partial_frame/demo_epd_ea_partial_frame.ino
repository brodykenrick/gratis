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

// This program is to illustrate the display operation as described in
// the datasheets.  The code is in a simple linear fashion and all the
// delays are set to maximum, but the SPI clock is set lower than its
// limit.  Therfore the display sequence will be much slower than
// normal and all of the individual display stages be clearly visible.

// Simple demo to toggle EPD between two images.
// Operation from reset:
// * display version
// * display compiled-in display setting
// * display FLASH detected or not
// * display temperature (displayed before every image is changed)
// * clear screen
// * delay 5 seconds (flash LED)
// * display text image
// * delay 5 seconds (flash LED)
// * display picture
// * delay 5 seconds (flash LED)
// * back to text display

// Embedded Artists has modified Pervasive Display Inc's demo application 
// to run on the 2.7 inch E-paper Display module (EA-LCD-009 

// Brody Kenrick consolidated the EA code back into the repaper code


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


// Change this for different display size
// supported sizes: 144 200 270
//#define SCREEN_SIZE 0
#define SCREEN_SIZE 270 //BK

// select two images from:  text_image text-hello cat aphrodite venus saturn
#define IMAGE_1  text_image
#define IMAGE_2  cat

// set up images from screen size2
#if (SCREEN_SIZE == 144)
#define EPD_SIZE EPD_1_44
#define FILE_SUFFIX _1_44.xbm
#define NAME_SUFFIX _1_44_bits
#define HEIGHT_SUFFIX _1_44_height
#define WIDTH_SUFFIX  _1_44_width

#elif (SCREEN_SIZE == 200)
#define EPD_SIZE EPD_2_0
#define FILE_SUFFIX _2_0.xbm
#define NAME_SUFFIX _2_0_bits
#define HEIGHT_SUFFIX _2_0_height
#define WIDTH_SUFFIX  _2_0_width

#elif (SCREEN_SIZE == 270)
#define EPD_SIZE EPD_2_7
#define FILE_SUFFIX _2_7.xbm
#define NAME_SUFFIX _2_7_bits
#define HEIGHT_SUFFIX _2_7_height
#define WIDTH_SUFFIX  _2_7_width

#else
#error "Unknown EPB size: Change the #define SCREEN_SIZE to a supported value"
#endif


#ifndef EMBEDDED_ARTISTS
// Error message for MSP430
#if (SCREEN_SIZE == 270) && defined(__MSP430_CPU__)
#error MSP430: not enough memory
#endif
#endif

// no futher changed below this point

// current version number
#define DEMO_VERSION "2.BK.partial"


// pre-processor convert to string
#define MAKE_STRING1(X) #X
#define MAKE_STRING(X) MAKE_STRING1(X)

// other pre-processor magic
// token joining and computing the string for #include
#define ID(X) X
#define MAKE_NAME1(X,Y) ID(X##Y)
#define MAKE_NAME(X,Y) MAKE_NAME1(X,Y)
#define MAKE_JOIN(X,Y) MAKE_STRING(MAKE_NAME(X,Y))

// calculate the include name and variable names
#define IMAGE_1_FILE MAKE_JOIN(IMAGE_1,FILE_SUFFIX)
#define IMAGE_1_BITS MAKE_NAME(IMAGE_1,NAME_SUFFIX)
#define IMAGE_1_HEIGHT MAKE_NAME(IMAGE_1,HEIGHT_SUFFIX)
#define IMAGE_1_WIDTH MAKE_NAME(IMAGE_1,WIDTH_SUFFIX)

#define IMAGE_2_FILE MAKE_JOIN(IMAGE_2,FILE_SUFFIX)
#define IMAGE_2_BITS MAKE_NAME(IMAGE_2,NAME_SUFFIX)
#define IMAGE_2_HEIGHT MAKE_NAME(IMAGE_2,HEIGHT_SUFFIX)
#define IMAGE_2_WIDTH MAKE_NAME(IMAGE_2,WIDTH_SUFFIX)


// Add Images library to compiler path
#include <Images.h>  // this is just an empty file

// images
PROGMEM const
#define unsigned
#define char uint8_t
#include IMAGE_1_FILE
#undef char
#undef unsigned

PROGMEM const
#define unsigned
#define char uint8_t
#include IMAGE_2_FILE
#undef char
#undef unsigned



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

// define the E-Ink display
EPD_Class EPD(EPD_SIZE, Pin_PANEL_ON, Pin_BORDER, Pin_DISCHARGE, Pin_PWM, Pin_RESET, Pin_BUSY, Pin_EPD_CS);
#ifdef EMBEDDED_ARTISTS
LM75A_Class LM75A;
#endif /* EMBEDDED_ARTISTS */

#define BITS_TO_BYTES(val_in_bits) (val_in_bits/8)

//Alternating vertical stripes -- One bit on one bit off
#define WHITE_STRIPES_SINGLE_LINE_2_7_CONTENTS 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,

#define PICTURE_HEIGHT_DIVISOR (8) //TODO: Swap this functionality around to be a line count
static unsigned char working_buffer_eighth_image_2_7_bits[ BITS_TO_BYTES( IMAGE_1_WIDTH ) * (IMAGE_1_HEIGHT/PICTURE_HEIGHT_DIVISOR) ] =
{
   WHITE_STRIPES_SINGLE_LINE_2_7_CONTENTS
   WHITE_STRIPES_SINGLE_LINE_2_7_CONTENTS
   WHITE_STRIPES_SINGLE_LINE_2_7_CONTENTS
   WHITE_STRIPES_SINGLE_LINE_2_7_CONTENTS
   WHITE_STRIPES_SINGLE_LINE_2_7_CONTENTS
   WHITE_STRIPES_SINGLE_LINE_2_7_CONTENTS
   WHITE_STRIPES_SINGLE_LINE_2_7_CONTENTS
   WHITE_STRIPES_SINGLE_LINE_2_7_CONTENTS
   WHITE_STRIPES_SINGLE_LINE_2_7_CONTENTS
   WHITE_STRIPES_SINGLE_LINE_2_7_CONTENTS
   WHITE_STRIPES_SINGLE_LINE_2_7_CONTENTS
   WHITE_STRIPES_SINGLE_LINE_2_7_CONTENTS
   WHITE_STRIPES_SINGLE_LINE_2_7_CONTENTS
   WHITE_STRIPES_SINGLE_LINE_2_7_CONTENTS
   WHITE_STRIPES_SINGLE_LINE_2_7_CONTENTS
   WHITE_STRIPES_SINGLE_LINE_2_7_CONTENTS
   WHITE_STRIPES_SINGLE_LINE_2_7_CONTENTS
   WHITE_STRIPES_SINGLE_LINE_2_7_CONTENTS
   WHITE_STRIPES_SINGLE_LINE_2_7_CONTENTS
   WHITE_STRIPES_SINGLE_LINE_2_7_CONTENTS
   WHITE_STRIPES_SINGLE_LINE_2_7_CONTENTS
   WHITE_STRIPES_SINGLE_LINE_2_7_CONTENTS
};

//Note: http://www.adafruit.com/blog/2008/04/17/free-up-some-arduino-sram/
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
#else /* EMBEDDED_ARTISTS */

  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV4);
#endif /* EMBEDDED_ARTISTS */

	Serial.begin(9600);
#if !defined(__MSP430_CPU__)
	// wait for USB CDC serial port to connect.  Arduino Leonardo only
	while (!Serial) {
	}
#endif
	Serial.println();
	Serial.println();
	Serial.println("Demo version: " DEMO_VERSION);
	Serial.println("Display: " MAKE_STRING(EPD_SIZE));
	Serial.println();

#ifndef EMBEDDED_ARTISTS
	FLASH.begin(Pin_FLASH_CS);
	if (FLASH.available()) {
		Serial.println("FLASH chip detected OK");
	} else {
		uint8_t maufacturer;
		uint16_t device;
		FLASH.info(&maufacturer, &device);
		Serial.print("unsupported FLASH chip: MFG: 0x");
		Serial.print(maufacturer, HEX);
		Serial.print("  device: 0x");
		Serial.print(device, HEX);
		Serial.println();
	}

	// configure temperature sensor
	S5813A.begin(Pin_TEMPERATURE);
#endif /* ! EMBEDDED_ARTISTS */

  //Checking the size of elements
  Serial.print( "sizeof(working_buffer_eighth_image_2_7_bits) = " );
  Serial.println( sizeof(working_buffer_eighth_image_2_7_bits) );
  
  Serial.print("availableMemory() = ");
  Serial.println(availableMemory());
}


static int state = 0;


// main loop
void loop() {
#ifndef EMBEDDED_ARTISTS
	int temperature = S5813A.read();
#else /* EMBEDDED_ARTISTS */
        int temperature = LM75A.read();
#endif /* EMBEDDED_ARTISTS */
	Serial.print("Temperature = ");
	Serial.print(temperature);
	Serial.println(" Celcius");

	EPD.begin(); // power up the EPD panel
	EPD.setFactor(temperature); // adjust for current temperature

	int delay_counts = 50;
	switch(state) {
	default:
	case 0:         // clear the screen
		EPD.clear();
		state = 1;
		delay_counts = 5;  // reduce delay so first image come up quickly
		break;

	case 1:         // clear -> text
		EPD.image(IMAGE_1_BITS);
		++state;
		break;

	case 2:         // text -> picture
		EPD.image(IMAGE_1_BITS, IMAGE_2_BITS);
		++state;
		break;

	case 3:        // picture -> text
		EPD.image(IMAGE_2_BITS, IMAGE_1_BITS);
		++state;
		break;

        case 4:         // clear the screen
          for(int i=0; i< PICTURE_HEIGHT_DIVISOR; i++)
          {
            //Clear part by part -- just to test
            EPD.clear( i*(IMAGE_1_HEIGHT/PICTURE_HEIGHT_DIVISOR), IMAGE_1_HEIGHT/PICTURE_HEIGHT_DIVISOR );
          }
          ++state;
          break;
      
        case 5:        // draw the white stripes from SRAM
          for(int i=0; i< PICTURE_HEIGHT_DIVISOR; i++)
          {
            //Asumes that screen is cleared (white) before hand
            EPD.image_sram( working_buffer_eighth_image_2_7_bits, i*(IMAGE_1_HEIGHT/PICTURE_HEIGHT_DIVISOR), IMAGE_1_HEIGHT/PICTURE_HEIGHT_DIVISOR );
          }
          state = 0;  // back to the start
          break;

	}
	EPD.end();   // power down the EPD panel

	// flash LED for 5 seconds
	for (int x = 0; x < delay_counts; ++x) {
		digitalWrite(Pin_RED_LED, LED_ON);
		delay(50);
		digitalWrite(Pin_RED_LED, LED_OFF);
		delay(50);
	}
}

