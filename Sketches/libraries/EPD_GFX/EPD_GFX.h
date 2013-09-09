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

//Brody added support for embeded artists' display (temp sensor) via the preprocessor define EMBEDDED_ARTISTS


#if !defined(EPD_GFX_H)
#define EPD_GFX_H 1

#include <Arduino.h>
#include <EPD.h>
//Temperature sensor
#ifndef EMBEDDED_ARTISTS
#include <S5813A.h>
#else /* EMBEDDED_ARTISTS */
#include <Wire.h>
#include <LM75A.h>
#endif /* EMBEDDED_ARTISTS */
#include <Adafruit_GFX.h>


class EPD_GFX : public Adafruit_GFX {

private:
	EPD_Class &EPD;
#ifndef EMBEDDED_ARTISTS
	S5813A_Class &TempSensor;
#else /* EMBEDDED_ARTISTS */
    LM75A_Class &TempSensor;
#endif /* EMBEDDED_ARTISTS */
	

	static const int pixel_width = 200;  // must be a multiple of 8
	static const int pixel_height = 96;

	uint8_t old_image[pixel_width * pixel_height / 8];
	uint8_t new_image[pixel_width * pixel_height / 8];

	EPD_GFX(EPD_Class&);  // disable copy constructor

public:

	enum {
		WHITE = 0,
		BLACK = 1
	};

	// constructor
#ifndef EMBEDDED_ARTISTS
	EPD_GFX(EPD_Class &epd, S5813A_Class &temp_sensor) :
#else /* EMBEDDED_ARTISTS */
	EPD_GFX(EPD_Class &epd, LM75A_Class &temp_sensor) :
#endif /* EMBEDDED_ARTISTS */
		Adafruit_GFX(this->pixel_width, this->pixel_height),
		EPD(epd), TempSensor(temp_sensor)
	{
	}

	void begin();
	void end();

	// set a single pixel in new_image
	void drawPixel(int x, int y, unsigned int colour) {
		int bit = x & 0x07;
		int byte = x / 8 + y * (pixel_width / 8);
		int mask = 0x01 << bit;
		if (BLACK == colour) {
			this->new_image[byte] |= mask;
		} else {
			this->new_image[byte] &= ~mask;
		}
	}

	// change from old image to new image
	void display();

};


#endif
