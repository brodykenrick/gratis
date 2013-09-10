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
	int temperature = this->TempSensor.read();

	// erase display
	this->EPD.begin();
	this->EPD.setFactor(temperature);
	this->EPD.clear();
	this->EPD.end();


	// clear buffers to white
	clear_new_image();
}


void EPD_GFX::display(boolean clear_first, boolean begin, boolean end) {
	// Erase old (optionally), display new
	// Optionally begins and ends the EPD/SPI.
	// There are delays in thos functions that only need to be pre/post use of the display

	int temperature = this->TempSensor.read();

	if(begin)
	{
    	this->EPD.begin();
    	this->EPD.setFactor(temperature);
	}

    if(clear_first)
    {
        this->EPD.clear(current_segment * this->pixel_height_segment, this->pixel_height_segment);
    }
    //NOTE: Although the expectation is that pixel_height_segment is going to be in an uint8_t keep an eye on this...
    assert( this->pixel_height_segment <= 255);
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
//The ONLY changes are to remove the check on being inside the vertical (y) segment
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
     ((x + 6 * size - 1) < 0)    // Clip left
    )
    return;
#endif

  for (int8_t i=0; i<6; i++ ) {
    uint8_t line;
    if (i == 5) 
      line = 0x0;
    else 
      line = pgm_read_byte(font+(c*5)+i);
    for (int8_t j = 0; j<8; j++) {
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

