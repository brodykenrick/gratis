#define EMBEDDED_ARTISTS
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
//Brody added support for external display sizes via the preprocessor definitions EPD_WIDTH and EPD_HEIGHT


#if !defined(EPD_GFX_H)
#define EPD_GFX_H 1

#include <Arduino.h>
#include <assert.h>

#include <EPD.h>
//Temperature sensor
#ifndef EMBEDDED_ARTISTS
#include <S5813A.h>
#else /* EMBEDDED_ARTISTS */
#include <Wire.h>
#include <LM75A.h>
#endif /* EMBEDDED_ARTISTS */

#include <Adafruit_GFX.h>

//NOTE: We always do a full clear (and not a transition from the old buffer) -- slower but less SRAM used.
#define HEIGHT_SHORTENED_PAGE (22) //<! Later make this some calculation in the constructor (based on passed in memory usage requests....)

class EPD_GFX : public Adafruit_GFX {

private:
	EPD_Class &EPD;
	//Temp sensor only needs a read function that returns in degrees Celsius
#ifndef EMBEDDED_ARTISTS
	S5813A_Class &TempSensor;
#else /* EMBEDDED_ARTISTS */
    LM75A_Class &TempSensor;
#endif /* EMBEDDED_ARTISTS */
	
    uint16_t pixel_width;  // must be a multiple of 8
    uint16_t pixel_height;
	uint16_t pixel_height_shortened;

	uint8_t         vertical_pages;
	uint8_t         vertical_page;

    //Buffers are only a subset of the total frame. We call this a page.
	uint8_t * new_image;

	EPD_GFX(EPD_Class&);  // disable copy constructor

public:

	enum {
		WHITE = 0,
		BLACK = 1
	};

	// constructor
	EPD_GFX(EPD_Class &epd, uint16_t pixel_width, uint16_t pixel_height,
#ifndef EMBEDDED_ARTISTS
	S5813A_Class &temp_sensor,
#else /* EMBEDDED_ARTISTS */
	LM75A_Class &temp_sensor,
#endif /* EMBEDDED_ARTISTS */
    uint16_t pixel_height_shortened = HEIGHT_SHORTENED_PAGE ):
		Adafruit_GFX(pixel_width, min(pixel_height,pixel_height_shortened)), //NOTE: The Adafruit_GFX lib is set to the minimal value
		EPD(epd), TempSensor(temp_sensor),
		pixel_width(pixel_width), pixel_height(pixel_height), pixel_height_shortened(pixel_height_shortened)
	{
        //Assumes divisor with no remainder....
        assert( (pixel_height%pixel_height_shortened) == 0);
		vertical_pages   = pixel_height/pixel_height_shortened;
		vertical_page = 0;

        //Buffers are only a subset of the total frame. We call this a page.
    	new_image = new uint8_t[(pixel_width/8 * pixel_height_shortened)];
    	assert( new_image );
	}

	void begin();
	void end();

	void clear_new_image();

    //NOTE: There is also a height() from Adafruit_GFX
    int16_t real_height(void) {
        return pixel_height;
    }

    void set_vertical_page(int vp) 
    {
        vertical_page = vp;
        clear_new_image();
    }

    uint16_t get_vertical_page_count() 
    {
        return vertical_pages;
    }

	// set a single pixel in new_image
	//NOTE: This is the trickery by which we get to limit the vertical size
	//This function does not write to the buffer if the pixel is not on the current page
	//VERY inefficient!
	void drawPixel(int16_t x, int16_t y, unsigned int colour)
	{
	    assert(y>=0); //BK: Not sure why it is allowed to be negative......
        if(
            ((uint16_t)y >= ( vertical_page    * this->pixel_height_shortened))
            &&
            ((uint16_t)y <  ((vertical_page+1) * this->pixel_height_shortened))
        )
        {
            y = (y % this->pixel_height_shortened); //Bring into 0..buffer_height size range
	
		    int bit = x & 0x07;
		    int byte = x / 8 + y * (pixel_width / 8);
		    int mask = 0x01 << bit;
		    if (BLACK == colour) {
			    this->new_image[byte] |= mask;
		    } else {
			    this->new_image[byte] &= ~mask;
		    }
		}
#if 0
        else
        {
        	Serial.print("drawPixel -- not on page! @ ");Serial.println(y);
        }
#endif
	}

	// Change old image to new image
	void display(boolean clear_first = true, boolean begin = false, boolean end = true);
	void clear();
    void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color,
      uint16_t bg, uint8_t size);

};

#endif

