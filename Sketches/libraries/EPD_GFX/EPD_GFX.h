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
//Brody added support for external display sizes in the constructor
//Brody added support for multi-segment screens (reducing SRAM bufer sizes)


#if !defined(EPD_GFX_H)
#define EPD_GFX_H 1

#include <Arduino.h>
// #include <assert.h>// AK: Teensy3.1 does not like "assert"

#include <EPD.h>

//#define EPD_GFX_HARDCODED_TEMP //!<Use a hardcoded temperature (provided to the constructor). Great to save on the PROGMEM/SRAM space of temp sensor (LM75A includes Wire/TWI).

//Temperature sensor
#if !defined(EPD_GFX_HARDCODED_TEMP)

#ifndef EMBEDDED_ARTISTS
#include <S5813A.h>
#else /* EMBEDDED_ARTISTS */
#include <Wire.h>
#include <LM75A.h>
#endif /* EMBEDDED_ARTISTS */

#endif //!defined(EPD_GFX_HARDCODED_TEMP)

#include <Adafruit_GFX.h>

//NOTE: We always do a full clear (and not a transition from the old buffer) -- slower but less SRAM used.
#define EPD_GFX_HEIGHT_SEGMENT_DEFAULT (8) //<! 8 is a factor of 176(2.7") and 96(other screens). TODO: Later make this some calculation in the constructor (based on passed in memory usage requests....)

#define EPD_GFX_CHAR_BASE_WIDTH  (5) //TODO: Bring out from "glcdfont.c" somehow??
#define EPD_GFX_CHAR_BASE_HEIGHT (7) //TODO: Bring out from "glcdfont.c" somehow??

#define EPD_GFX_CHAR_PADDED_WIDTH  (EPD_GFX_CHAR_BASE_WIDTH+1)
#define EPD_GFX_CHAR_PADDED_HEIGHT (EPD_GFX_CHAR_BASE_HEIGHT+1)

#define EPD_DRAWBITMAP_FAST_SUPPORT //!< Support a faster (more direct use of EPD hardware for) writing a bitmap. GFX has a drawBitmap that is just painfully slow.

class EPD_GFX : public Adafruit_GFX {

private:
	EPD_Class &EPD;
#if !defined(EPD_GFX_HARDCODED_TEMP)
	//Temp sensor only needs a read function that returns in degrees Celsius
#ifndef EMBEDDED_ARTISTS
	S5813A_Class &TempSensor;
#else /* EMBEDDED_ARTISTS */
    LM75A_Class &TempSensor;
#endif /* EMBEDDED_ARTISTS */

#else
  	uint8_t         temp_celsius;
#endif //!defined(EPD_GFX_HARDCODED_TEMP)
	
    uint16_t pixel_width;  // must be a multiple of 8
    uint16_t pixel_height;
	uint16_t pixel_height_segment; //must be a factor of pixel_height

	uint8_t         total_segments;
	uint8_t         current_segment;

    //Buffer for updating display
    //Note: This has removed the support of using a toggling buffer OLD/NEW as there is not enough SRAM for that.
	uint8_t * new_image;
	
	uint8_t get_temperature()
	{
#if defined(EPD_GFX_HARDCODED_TEMP)
	    return temp_celsius;
#else
        return this->TempSensor.read();
#endif //defined(EPD_GFX_HARDCODED_TEMP)
	}

	EPD_GFX(EPD_Class&);  // disable copy constructor

public:

	enum {
		WHITE = 0,
		BLACK = 1
	};

	// constructor
	EPD_GFX(EPD_Class &epd, uint16_t pixel_width, uint16_t pixel_height,
#if !defined(EPD_GFX_HARDCODED_TEMP)

#ifndef EMBEDDED_ARTISTS
	S5813A_Class &temp_sensor,
#else /* EMBEDDED_ARTISTS */
	LM75A_Class &temp_sensor,
#endif /* EMBEDDED_ARTISTS */

#else
    uint8_t         temp_celsius,
#endif //!defined(EPD_GFX_HARDCODED_TEMP)

    uint16_t pixel_height_segment = EPD_GFX_HEIGHT_SEGMENT_DEFAULT ):
		Adafruit_GFX(pixel_width, min(pixel_height,pixel_height_segment)), //NOTE: The Adafruit_GFX lib is set to the minimal value
		EPD(epd),
#if defined(EPD_GFX_HARDCODED_TEMP)
        temp_celsius(temp_celsius),
#else
		TempSensor(temp_sensor),
#endif //defined(EPD_GFX_HARDCODED_TEMP)
		pixel_width(pixel_width), pixel_height(pixel_height), pixel_height_segment(pixel_height_segment)
	{
        //Assumes divisor with no remainder....
//        assert( (pixel_height%pixel_height_segment) == 0);// AK: Teensy3.1 does not like "assert"
		total_segments   = pixel_height/pixel_height_segment;
		current_segment = 0;

        //Buffer is only a subset of the total frame. We call this a segment.
    	new_image = new uint8_t[  get_segment_buffer_size_bytes() ];
//    	assert( new_image ); // AK: Teensy3.1 does not like "assert"
	}
	
	uint16_t get_segment_buffer_size_bytes() 
    {
        return (pixel_width/8 * pixel_height_segment);
    }

	void begin();
	void end();

	void clear_new_image();

    //NOTE: There is also a height() from Adafruit_GFX
    int16_t real_height(void) {
        return pixel_height;
    }

    void set_current_segment(int segment) 
    {
        current_segment = segment;
        clear_new_image();
    }

    uint16_t get_segment_count() 
    {
        return total_segments;
    }

	// Set a single pixel in new_image
	//NOTE: This is the trickery by which we get to limit the vertical size and save SRAM
	//This function does not write to the buffer if the pixel is not on the current segment
	//However it gets called MANY times that are wasteful....
	//VERY inefficient!
	//The calling functions can be optimised though (drawChar has been -- others not yet)
	void drawPixel(int16_t x, int16_t y, uint16_t colour) // AK replaced "unsigned int" with "uint16_t" to work with Teensy3.1 
	{
//	    assert(y>=0); //BK: Not sure why it is allowed to be negative......// AK: Teensy3.1 does not like "assert"
        if(
            ((uint16_t)y >= ( current_segment    * this->pixel_height_segment))
            &&
            ((uint16_t)y <  ((current_segment+1) * this->pixel_height_segment))
        )
        {
            y = (y % this->pixel_height_segment); //Bring into 0..buffer_height size range
	
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
            //Useful for debugging and optimising
        	Serial.print("drawPixel -- not in segment! @ ");Serial.println(y);
        }
#endif
	}

	// Change old image to new image
	void display(boolean clear_first = true, boolean begin = false, boolean end = true);
	void clear();
    void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color,
      uint16_t bg, uint8_t size);


#if defined(EPD_DRAWBITMAP_FAST_SUPPORT)
    void drawBitmapFast(const uint8_t PROGMEM *bitmap, boolean subsampled_by_2);
#endif //defined(EPD_DRAWBITMAP_FAST_SUPPORT)

};

#endif

