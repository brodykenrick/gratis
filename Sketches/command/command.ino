// Support for EA board
// http://www.embeddedartists.com/products/displays/lcd_27_epaper.php
// Rev. C with SPI flash (on same SSEL) tested
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
#define EPD_SIZE EPD_2_7

#define COMMAND_VERSION "2"


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

static uint16_t xbm_count;
static bool xbm_parser(uint8_t *b);


static uint8_t Serial_getc();
static uint16_t Serial_gethex(bool echo);
static void Serial_puthex(uint32_t n, int bits);
static void Serial_puthex_byte(uint8_t n);
static void Serial_puthex_word(uint16_t n);
static void Serial_puthex_double(uint32_t n);
static void Serial_hex_dump(uint32_t address, const void *buffer, uint16_t length);


// define the E-Ink display
EPD_Class EPD(EPD_SIZE, Pin_PANEL_ON, Pin_BORDER, Pin_DISCHARGE, Pin_PWM, Pin_RESET, Pin_BUSY, Pin_EPD_CS);

#ifdef EMBEDDED_ARTISTS
LM75A_Class LM75A;
#endif /* EMBEDDED_ARTISTS */


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
void selectFlash() {
  digitalWrite(Pin_PANEL_ON, LOW);
}

void selectEPD() {
  digitalWrite(Pin_PANEL_ON, HIGH); //Change back to using SSEL for EPD
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

	Serial.begin(115200);
#if !defined(__MSP430_CPU__)
	// wait for USB CDC serial port to connect.  Arduino Leonardo only
	while (!Serial) {
	}
#endif
	Serial.println();
	Serial.println();
	Serial.println("Command version: " COMMAND_VERSION);
	Serial.println("Display: " MAKE_STRING(EPD_SIZE));
	Serial.println();

#ifndef EMBEDDED_ARTISTS
	FLASH.begin(Pin_FLASH_CS);
#else
        selectFlash();
	FLASH.begin(Pin_EPD_CS);
#endif /* ! EMBEDDED_ARTISTS */

	flash_info();

#ifndef EMBEDDED_ARTISTS
	// configure temperature sensor
	S5813A.begin(Pin_TEMPERATURE);
#endif /* ! EMBEDDED_ARTISTS */
}

void startEPD() {
        selectEPD();
        EPD.begin();
#ifndef EMBEDDED_ARTISTS
        int temperature = S5813A.read();
#else /* EMBEDDED_ARTISTS */
        int temperature = LM75A.read();
#endif /* EMBEDDED_ARTISTS */
        EPD.setFactor(temperature);
}


// main loop
void loop() {
	Serial.println();
	Serial.print("Command: ");
	uint8_t c = Serial_getc();
	if (!isprint(c)) {
		return;
	}
	Serial.write(c);
	switch (c) {
	case 'h':
	case 'H':
	case '?':
		Serial.println();
		Serial.println("h          - this command");
		Serial.println("d<ss> <ll> - dump sector in hex, ll * 16 bytes");
		Serial.println("e<ss>      - erase sector to 0xff");
		Serial.println("u<ss>      - upload XBM to sector");
		Serial.println("i<ss>      - display an image on white screen");
		Serial.println("r<ss>      - revert an image back to white");
		Serial.println("l          - search for non-empty sectors");
		Serial.println("w          - clear screen to white");
		Serial.println("f          - dump FLASH identification");
		Serial.println("t          - show temperature");
		break;
	case 'd':
	{
                selectFlash();
		uint16_t sector = Serial_gethex(true);
		Serial.print(' ');
		uint16_t count = Serial_gethex(true);
		if (0 == count) {
			count = 1;
		}
		Serial.println();
		Serial.print("sector: ");
		Serial_puthex_byte(sector);
		Serial.println();
		uint32_t address = sector;
		address <<= 12;
		for (uint16_t i = 0; i < count; ++i) {
			uint8_t buffer[16];
			FLASH.read(buffer, address, sizeof(buffer));
			Serial_hex_dump(address, buffer, sizeof(buffer));
			address += sizeof(buffer);
		}
		break;
	}

	case 'e':
	{
                selectFlash();
		uint32_t sector = Serial_gethex(true);
		FLASH.write_enable();
		FLASH.sector_erase(sector << 12);
		FLASH.write_disable();
		break;
	}

	case 'u':
	{
                selectFlash();
		uint32_t address = Serial_gethex(true);
		address <<= 12;
		xbm_count = 0;
		Serial.println();
		Serial.println("start upload...");

		for (;;) {
			uint8_t buffer[16];
			uint16_t count = 0;
			for (int i = 0; i < sizeof(buffer); ++i, ++count) {
				if (!xbm_parser(&buffer[i])) {
					break;
				}
			}
			FLASH.write_enable();
			FLASH.write(address, buffer, count);
			address += count;
			if (sizeof(buffer) != count) {
				break;
			}
		}

		FLASH.write_disable();

		Serial.println();
		Serial.print(" read = ");
		Serial_puthex_word(xbm_count);
		if (128 * 96 / 8 == xbm_count) {
			Serial.print(" 128x96 1.44");
		} else if (200 * 96 / 8 == xbm_count) {
			Serial.print(" 200x96 2.0");
		} else if (264L * 176 / 8 == xbm_count) {
			Serial.print(" 264x176 2.7");
		} else {
			Serial.print(" invalid image");
		}
		Serial.println();
		break;
	}

	case 'i':
	{
		uint32_t address = Serial_gethex(true);
		address <<= 12;
                startEPD();
		EPD.frame_cb_repeat(address, flash_read, EPD_inverse);
		EPD.frame_cb_repeat(address, flash_read, EPD_normal);
		EPD.end();
		break;
	}

	case 'r':
	{
		uint32_t address = Serial_gethex(true);
		address <<= 12;
                startEPD();
		EPD.frame_cb_repeat(address, flash_read, EPD_compensate);
		EPD.frame_cb_repeat(address, flash_read, EPD_white);
		EPD.end();
		break;
	}

	case 'l':
	{
                selectFlash();
		Serial.println();
		uint16_t per_line = 0;
		for (uint16_t sector = 0; sector <= 0xff; ++sector) {
			for (uint16_t i = 0; i < 4096; i += 16) {
				uint8_t buffer[16];
				uint32_t address = sector;
				address <<= 12;
				address += i;
				FLASH.read(buffer, address, sizeof(buffer));
				bool flag = false;
				for (uint16_t j = 0; j < sizeof(buffer); ++j) {
					if (0xff != buffer[j]) {
						flag = true;
						break;
					}
				}
				if (flag) {
					Serial_puthex_byte(sector);
					if (++per_line >= 16) {
						Serial.println();
						per_line = 0;
					} else {
						Serial.print(' ');
					}
					break;
				}
			}

		}
		if (0 != per_line) {
			Serial.println();
		}
		break;
	}

	case 'f':
	{
		selectFlash();
                Serial.println();
		flash_info();
		break;
	}

	case 'w':
	{
		startEPD();
		EPD.clear();
		EPD.end();
		break;
	}

	default:
		Serial.println();
		Serial.println("error");

	}
}


static void flash_info(void) {
	uint8_t maufacturer;
	uint16_t device;

	if (FLASH.available()) {
		Serial.println("FLASH chip detected OK");
	} else {
		Serial.println("unsupported FLASH chip");
	}

	FLASH.info(&maufacturer, &device);
	Serial.print("FLASH: manufacturer = ");
	Serial_puthex_byte(maufacturer);
	Serial.print(" device = ");
	Serial_puthex_word(device);
	Serial.println();
}


static void flash_read(void *buffer, uint32_t address, uint16_t length) {
        selectFlash();
	FLASH.read(buffer, address, length);
        selectEPD();
}


static bool xbm_parser(uint8_t *b) {
	for (;;) {
		uint8_t c = Serial_getc();
		if ('0' == c && 'x' == Serial_getc()) {
			*b = Serial_gethex(false);
			++xbm_count;
			if (0 == (xbm_count & 0x07)) {
				digitalWrite(Pin_RED_LED, !digitalRead(Pin_RED_LED));
			}
			return true;
		} else if (';' == c) {
			break;
		}
	}
	digitalWrite(Pin_RED_LED, LED_OFF);
	return false;
}


// miscellaneous serial interface routines

static uint8_t Serial_getc() {
	while (0 == Serial.available()) {
	}
	return Serial.read();
}


static uint16_t Serial_gethex(bool echo) {
	uint16_t acc = 0;
	for (;;) {

		uint8_t c = Serial_getc();

		if (c >= '0' && c <= '9') {
			c -= '0';
		} else if (c >= 'a' && c <= 'f') {
			c -= 'a' - 10;
		} else if (c >= 'A' && c <= 'F') {
			c -= 'A' - 10;
		} else {
			return acc;
		}
		if (echo) {
			Serial_puthex(c, 4);
		}
		acc <<= 4;
		acc += c;
	}
}


static void Serial_puthex(uint32_t n, int bits) {
	for (int i = bits - 4; i >= 0; i -= 4) {
		char nibble = ((n >> i) & 0x0f) + '0';
		if (nibble > '9') {
			nibble += 'a' - '9' - 1;
		}
		Serial.print(nibble);
	}
}


static void Serial_puthex_byte(uint8_t n) {
	Serial_puthex(n, 8);
}


static void Serial_puthex_word(uint16_t n) {
	Serial_puthex(n, 16);
}


static void Serial_puthex_double(uint32_t n) {
	Serial_puthex(n, 32);
}


static void Serial_hex_dump(uint32_t address, const void *buffer, uint16_t length) {
	const uint8_t *p = (const uint8_t *)buffer;
	while (0 != length) {
		Serial_puthex_double(address);
		Serial.print(": ");
		for (int i = 0; i < 16; ++i, ++address) {
			Serial_puthex_byte(*p++);
			Serial.print(' ');
			if (7 == i) {
				Serial.print(' ');
			}
			if (--length == 0) {
				break;
			}
		}
		Serial.println();
	}
}
