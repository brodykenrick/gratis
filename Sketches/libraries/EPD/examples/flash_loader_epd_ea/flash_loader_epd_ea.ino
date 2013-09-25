#define EMBEDDED_ARTISTS //!< Support for EA board @ http://www.embeddedartists.com/products/displays/lcd_27_epaper.php. Rev. B with SPI flash (on same SSEL) tested
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

// Simple demo with two functions:
//
// 1. Copy an included iname to a specified FLSH sector and display it
// 2. Display a list of already FLASHed images
//
// Note: only one function is available at a time.

// Brody Kenrick made the EA code back into the repaper code. Support for EA board @ http://www.embeddedartists.com/products/displays/lcd_27_epaper.php. Rev. B with SPI flash tested

#include <Arduino.h>
#include <inttypes.h>
#include <ctype.h>

// required libraries
#include <SPI.h>
#include <FLASH.h>
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



// set up images from screen size2
#if (SCREEN_SIZE == 144)
#define EPD_SIZE EPD_1_44
#define FILE_SUFFIX _1_44.xbm
#define NAME_SUFFIX _1_44_bits
#define SECTORS_USED 1

#elif (SCREEN_SIZE == 200)
#define EPD_SIZE EPD_2_0
#define FILE_SUFFIX _2_0.xbm
#define NAME_SUFFIX _2_0_bits
#define SECTORS_USED 1

#elif (SCREEN_SIZE == 270)
#define EPD_SIZE EPD_2_7
#define FILE_SUFFIX _2_7.xbm
#define NAME_SUFFIX _2_7_bits
#define SECTORS_USED 2

#else
#error "Unknown EPB size: Change the #define SCREEN_SIZE to a supported value"
#endif

// select image from:  text_image text-hello cat aphrodite venus saturn
// select a suitable sector, (size 270 will take two sectors)
#define IMAGE        cat
#define FLASH_SECTOR (0*SECTORS_USED)

// if the display list is defined it will take priority over the flashing
// (and FLASH code is disbled)
// define a list of {sector, milliseconds}
//#define DISPLAY_LIST {0, 5000}, {1*SECTORS_USED, 5000}, {2*SECTORS_USED, 5000}

// program version
#define FLASH_LOADER_VERSION "1"

// pre-processor convert to string
#define MAKE_STRING1(X) #X
#define MAKE_STRING(X) MAKE_STRING1(X)

// other pre-processor magic
// tiken joining and computing the string for #include
#define ID(X) X
#define MAKE_NAME1(X,Y) ID(X##Y)
#define MAKE_NAME(X,Y) MAKE_NAME1(X,Y)
#define MAKE_JOIN(X,Y) MAKE_STRING(MAKE_NAME(X,Y))

// calculate the include name and variable names
#define IMAGE_FILE MAKE_JOIN(IMAGE,FILE_SUFFIX)
#define IMAGE_BITS MAKE_NAME(IMAGE,NAME_SUFFIX)


// Add Images library to compiler path
#include <Images.h>  // this is just an empty file

// image
#if !defined(DISPLAY_LIST)
PROGMEM const
#define unsigned
#define char uint8_t
#include IMAGE_FILE
#undef char
#undef unsigned
#endif

// definition of I/O pins LaunchPad and Arduino are different

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
const int Pin_PANEL_ON = 2;  //a.k.a PWR_CTRL
const int Pin_BORDER = 3;
const int Pin_DISCHARGE = 4;
const int Pin_PWM = 5;
const int Pin_RESET = 6;
const int Pin_BUSY = 7;
const int Pin_EPD_CS = 8;
#ifndef EMBEDDED_ARTISTS
const int Pin_FLASH_CS = 9;
const int Pin_SW2 = 12;
#else
//Note for EA Rev. B the SN74LVC1G139 decoder/multiplexer (U3 on the board) is re-used for SSEL of the flash. Thus there is no separate flash SSEL.
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


// function prototypes
static void flash_info(void);
static void flash_read(void *buffer, uint32_t address, uint16_t length);

#if !defined(DISPLAY_LIST)
static void flash_program(uint16_t sector, const void *buffer, uint16_t length, bool buffer_in_progmem);
#endif

// define the E-Ink display
EPD_Class EPD(EPD_SIZE, Pin_PANEL_ON, Pin_BORDER, Pin_DISCHARGE, Pin_PWM, Pin_RESET, Pin_BUSY, Pin_EPD_CS);
#ifdef EMBEDDED_ARTISTS
LM75A_Class LM75A;
#endif /* EMBEDDED_ARTISTS */

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

	Serial.begin(115200);
#if !defined(__MSP430_CPU__)
	// wait for USB CDC serial port to connect.  Arduino Leonardo only
	while (!Serial) {
	}
#endif
	Serial.println();
	Serial.println();
	Serial.println("Flash loader version: " FLASH_LOADER_VERSION);
	Serial.println("Display: " MAKE_STRING(EPD_SIZE));
	Serial.println();

#ifndef EMBEDDED_ARTISTS
	FLASH.begin(Pin_FLASH_CS);
#else
        //Note for EA Rev. B the SN74LVC1G139 decoder/multiplexer (U3 on the board) is used for SSEL of the flash
        //SN74LVC1G139 @ http://www.ti.com/lit/ds/symlink/sn74lvc1g139.pdf
        //SSEL(Pin_EPD_CS):Dec-A[In]
        //PWR_CTRL(Pin_PANEL_ON):Dec-B[In]
        //Dec-Y0[Out]:SSEL_FLASH[Unexposed on board signal]
        //Dec-Y2[Out]:SSEL_DISP[Unexposed on board signal]

        //Pin_PANEL_ON:B || Pin_EPD_CS:A || SSEL_FLASH:Y0 || SSEL_DISP:Y2
        //   L                 L                 L               H
        //   L                 H                 H               H
        //   H                 L                 H               L
        //   H                 H                 H               H
        digitalWrite(Pin_PANEL_ON, LOW);
	FLASH.begin(Pin_EPD_CS);
#endif /* ! EMBEDDED_ARTISTS */

    flash_info();

#ifndef EMBEDDED_ARTISTS
	// configure temperature sensor
	S5813A.begin(Pin_TEMPERATURE);
#endif /* ! EMBEDDED_ARTISTS */

	// if necessary program the flash
#if !defined(DISPLAY_LIST)

        //First image
        flash_program(FLASH_SECTOR, IMAGE_BITS, sizeof(IMAGE_BITS), true);

#endif
}


typedef struct {
	int sector;
	int delay_ms;
} display_list_type[];

// list of {FLASH sector, milliseconds} to display
// if not defined then just display the current flash sector
static const display_list_type display_list = {
#if defined(DISPLAY_LIST)
	DISPLAY_LIST
#else
	{FLASH_SECTOR, 5000}
#endif
};

#define DISPLAY_ITEM_COUNT (sizeof(display_list)/sizeof(display_list[0]))

// state counter
static int state = 0;
static int display_index = 0;
#define INVALID_ADDRESS (0xffffffff)
static uint32_t old_address = INVALID_ADDRESS;

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


	int delay_counts = 5;

	switch(state) {
	default:
	case 0:         // clear the screen
		EPD.begin(); // power up the EPD panel
		EPD.setFactor(temperature); // adjust for current temperature
		EPD.clear();
		EPD.end();   // power down the EPD panel
		state = 1;
		break;

	case 1:         // next image
		uint32_t address = display_list[display_index].sector << 12;
		Serial.print("Address = 0x");
		Serial.println(address, HEX);
		delay_counts = display_list[display_index].delay_ms;
		EPD.begin();
#ifndef EMBEDDED_ARTISTS
	int t = S5813A.read();
#else /* EMBEDDED_ARTISTS */
        int t = LM75A.read();
#endif /* EMBEDDED_ARTISTS */
		EPD.setFactor(t);

		if (INVALID_ADDRESS != old_address)
                {
			EPD.frame_cb_repeat(old_address, flash_read, EPD_compensate);
			EPD.frame_cb_repeat(old_address, flash_read, EPD_white);
		}
		EPD.frame_cb_repeat(address, flash_read, EPD_inverse);
		EPD.frame_cb_repeat(address, flash_read, EPD_normal);

		EPD.end();
		// preserve address for next cycle
		old_address = address;
		// increment list index or reset to start on overflow
		if (++display_index >= DISPLAY_ITEM_COUNT) {
			display_index = 0;
		}
		break;

	}

	delay(delay_counts);
}


// display information about the FLASH chip
static void flash_info(void) {
	uint8_t maufacturer;
	uint16_t device;

	if (FLASH.available()) {
		Serial.println("FLASH chip detected OK");
	} else {
		Serial.println("unsupported FLASH chip");
	}

	FLASH.info(&maufacturer, &device);
	Serial.print("FLASH: manufacturer = 0x");
	Serial.print(maufacturer, HEX);
	Serial.print("  device = 0x");
	Serial.print(device, HEX);
	Serial.println();
}


// EPD display callback for reading the FLASH
static void flash_read(void *buffer, uint32_t address, uint16_t length)
{
#ifdef EMBEDDED_ARTISTS
  digitalWrite(Pin_PANEL_ON, LOW); //Change back to using SSEL for Flash
#endif /* EMBEDDED_ARTISTS */  

  FLASH.read(buffer, address, length);

#ifdef EMBEDDED_ARTISTS
  digitalWrite(Pin_PANEL_ON, HIGH); //Change back to using SSEL for EPD
#endif /* EMBEDDED_ARTISTS */  

}


#if !defined(DISPLAY_LIST)
// program image into FLASH
static void flash_program(uint16_t sector, const void *buffer, uint16_t length, bool buffer_in_progmem = false) {
	Serial.print("FLASH: program sector = ");
	Serial.println(sector, DEC);
	Serial.print("       from memory @ 0x");
	Serial.print((uint32_t)buffer, HEX);
	Serial.print("  total bytes: ");
	Serial.println(length, DEC);

	uint32_t address = sector << FLASH_SECTOR_SHIFT;

	// erase required sectors
	for (int i = 0; i < length; i += FLASH_SECTOR_SIZE, address += FLASH_SECTOR_SIZE) {
		Serial.print("FLASH: erase = 0x");
		Serial.println(address, HEX);
		FLASH.write_enable();
		FLASH.sector_erase(address);
	}

	// writable pages are FLASH_PAGE_SIZE bytes
	const uint8_t *p = (const uint8_t *)buffer;
	for (address = sector << FLASH_SECTOR_SHIFT; length >= FLASH_PAGE_SIZE;
	     length -= FLASH_PAGE_SIZE, address += FLASH_PAGE_SIZE, p += FLASH_PAGE_SIZE) {
		Serial.print("FLASH: write @ 0x");
		Serial.print(address, HEX);
		Serial.print("  from: 0x");
		Serial.print((uint32_t)p, HEX);
		Serial.print("  bytes: 0x");
		Serial.println(FLASH_PAGE_SIZE, HEX);

		FLASH.write_enable();
		FLASH.write(address, p, FLASH_PAGE_SIZE, buffer_in_progmem);
	}
	// write any remaining partial page
	if (length > 0) {
		Serial.print("FLASH: write @ 0x");
		Serial.print(address, HEX);
		Serial.print("  from: 0x");
		Serial.print((uint32_t)p, HEX);
		Serial.print("  bytes: 0x");
		Serial.println(length, HEX);
		FLASH.write_enable();
		FLASH.write(address, p, length);
	}

	// turn off write - just to be safe
	FLASH.write_disable();
       
}
#endif
