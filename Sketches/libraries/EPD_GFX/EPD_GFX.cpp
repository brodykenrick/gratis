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
#include <limits.h>
#include <EPD.h>
#include "EPD_GFX.h"


void EPD_GFX::begin() {
    clear();
}

void EPD_GFX::clear_new_image() {
   	memset(this->new_image, 0, pixel_width/8 * pixel_height_segment);
}

void EPD_GFX::clear() {

	// erase display
	this->EPD.begin();
	this->EPD.setFactor( get_temperature() );
	this->EPD.clear();
	this->EPD.end();


	// clear buffers to white
	clear_new_image();
}


void EPD_GFX::display(boolean clear_first, boolean begin, boolean end) {
	// Erase old (optionally), display new
	// Optionally begins and ends the EPD/SPI.
	// There are delays in thos functions that only need to be pre/post use of the display

	if(begin)
	{
    	this->EPD.begin();
    	this->EPD.setFactor( get_temperature() );
	}

    if(clear_first)
    {
        this->EPD.clear(current_segment * this->pixel_height_segment, this->pixel_height_segment);
    }
    //NOTE: Although the expectation is that pixel_height_segment is going to be in an uint8_t keep an eye on this...
//    assert( this->pixel_height_segment <= 255); // AK: Teensy3.1 does not like "assert"
	this->EPD.image_sram(this->new_image, current_segment * this->pixel_height_segment,
	                    (uint8_t)this->pixel_height_segment);

	if(end)
	{
    	this->EPD.end();
	}
}

//Font from Adafruit_GFX libary
#include "glcdfont.c"

// Draw a character
//Override this function (so we don't need to modify the Adafruit library).
//The changes are to:
// * modify the check on being inside the vertical (y) segment
//  to allow for chars that overlap segments (and also to optimise a
//  quick return if the char won't be in a segment at all)
void EPD_GFX::drawChar(int16_t x, int16_t y, unsigned char c,
			    uint16_t color, uint16_t bg, uint8_t size) {
#if 0 //Original code
//  if((x >= _width)            || // Clip right
//     (y >= _height)           || // Clip bottom
//     ((x + 6 * size - 1) < 0) || // Clip left
//     ((y + 8 * size - 1) < 0))   // Clip top
//    return;
#else
  if((x >= _width)            || // Clip right
     ((x + EPD_GFX_CHAR_PADDED_WIDTH * size - 1) < 0)    // Clip left
    )
    return;
#endif

#if 1
  //Check if we are doing any drawing in this segment
  //NOTE: This is an optimisation (only for parialt segments) to save wasted cyclces getting all the way to drawPixel for many non-existent pixels for this segment.
  const unsigned int y_end_char = y + size * (EPD_GFX_CHAR_PADDED_HEIGHT);
  const unsigned int segment_start_row = (current_segment  )* pixel_height_segment;
  const unsigned int segment_end_row   = (current_segment+1)* pixel_height_segment;
  boolean draw_char = true;
  if(y > segment_end_row)
  {
	  //Char starts on a later segment
	  draw_char = false;
  }
  if(y_end_char < segment_start_row)
  {
	  //Char finished on an earlier segment
	  draw_char = false;
  }
  if(!draw_char)
  {
    return;
  }
#endif

  //Unchanged below (except magic numbers replaced)
  for (int8_t i=0; i<EPD_GFX_CHAR_PADDED_WIDTH; i++ ) {
    uint8_t line;
    if (i == EPD_GFX_CHAR_BASE_WIDTH) 
      line = 0x0;
    else 
      line = pgm_read_byte(font+(c*EPD_GFX_CHAR_BASE_WIDTH)+i);
    for (int8_t j = 0; j<EPD_GFX_CHAR_PADDED_HEIGHT; j++) {
      if (line & 0x1) {
        if (size == 1) // default size
          drawPixel(x+i, y+j, color);
        else {  // big size
          fillRect(x+(i*size), y+(j*size), size, size, color);
        } 
      } else if (bg != color) {
        if (size == 1) // default size
          drawPixel(x+i, y+j, bg);
        else {  // big size
          fillRect(x+i*size, y+j*size, size, size, bg);
        }
      }
      line >>= 1;
    }
  }
}

#if defined(EPD_DRAWBITMAP_FAST_SUPPORT)
void EPD_GFX::drawBitmapFast(const uint8_t PROGMEM *bitmap, boolean subsampled_by_2) {

    this->EPD.begin();
	this->EPD.setFactor( get_temperature() );
	this->EPD.clear();
	this->EPD.image( bitmap, 0, this->pixel_height, subsampled_by_2 );
	this->EPD.end();
}

#endif //defined(EPD_DRAWBITMAP_FAST_SUPPORT)

