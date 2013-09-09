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

#if !defined(EPD_H)
#define EPD_H 1

#include <Arduino.h>
#include <SPI.h>

#if defined(__MSP430_CPU__)
#define PROGMEM
#else
#include <avr/pgmspace.h>
#endif

#define EPD_PARTIAL_SCREEN_SRAM
// if more SRAM available (8 kBytes)
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega_2560__)
#define EPD_ENABLE_EXTRA_SRAM 1
#elif defined(EPD_PARTIAL_SCREEN_SRAM)
#define EPD_ENABLE_EXTRA_SRAM 1
#endif

typedef enum {
	EPD_1_44,        // 128 x 96
	EPD_2_0,         // 200 x 96
	EPD_2_7          // 264 x 176
} EPD_size;

typedef enum {           // Image pixel -> Display pixel
	EPD_compensate,  // B -> W, W -> B (Current Image)
	EPD_white,       // B -> N, W -> W (Current Image)
	EPD_inverse,     // B -> N, W -> B (New Image)
	EPD_normal       // B -> B, W -> W (New Image)
} EPD_stage;

typedef void EPD_reader(void *buffer, uint32_t address, uint16_t length);

class EPD_Class {
private:
	int EPD_Pin_EPD_CS;
	int EPD_Pin_PANEL_ON;
	int EPD_Pin_BORDER;
	int EPD_Pin_DISCHARGE;
	int EPD_Pin_PWM;
	int EPD_Pin_RESET;
	int EPD_Pin_BUSY;

	EPD_size size;
	uint16_t stage_time;
	uint16_t factored_stage_time;
	uint16_t lines_per_display;
	uint16_t dots_per_line;
	uint16_t bytes_per_line;
	uint16_t bytes_per_scan;
	PROGMEM const uint8_t *gate_source;
	uint16_t gate_source_length;
	PROGMEM const uint8_t *channel_select;
	uint16_t channel_select_length;

	bool filler;

	EPD_Class(const EPD_Class &f);  // prevent copy

public:
	// power up and power down the EPD panel
	void begin();
	void end();

	void setFactor(int temperature = 25) {
		this->factored_stage_time = this->stage_time * this->temperature_to_factor_10x(temperature) / 10;
	}

	// clear display (anything -> white)
	void clear(uint16_t first_line_no = 0, uint8_t line_count = 0) {
		this->frame_fixed_repeat(0xff, EPD_compensate, first_line_no, line_count);
		this->frame_fixed_repeat(0xff, EPD_white, first_line_no, line_count);
		this->frame_fixed_repeat(0xaa, EPD_inverse, first_line_no, line_count);
		this->frame_fixed_repeat(0xaa, EPD_normal, first_line_no, line_count);
	}

	// assuming a clear (white) screen output an image (PROGMEM data)
	void image(PROGMEM const uint8_t *image, uint16_t first_line_no = 0, uint8_t line_count = 0) {
		this->frame_fixed_repeat(0xaa, EPD_compensate, first_line_no, line_count);
		this->frame_fixed_repeat(0xaa, EPD_white, first_line_no, line_count);
		this->frame_data_repeat(image, EPD_inverse, first_line_no, line_count);
		this->frame_data_repeat(image, EPD_normal, first_line_no, line_count);
	}

	// change from old image to new image (PROGMEM data)
	void image(PROGMEM const uint8_t *old_image, PROGMEM const uint8_t *new_image, uint16_t first_line_no = 0, uint8_t line_count = 0) {
		this->frame_data_repeat(old_image, EPD_compensate, first_line_no, line_count);
		this->frame_data_repeat(old_image, EPD_white, first_line_no, line_count);
		this->frame_data_repeat(new_image, EPD_inverse, first_line_no, line_count);
		this->frame_data_repeat(new_image, EPD_normal, first_line_no, line_count);
	}

#if defined(EPD_ENABLE_EXTRA_SRAM)

	// assuming a clear (white) screen output an image (SRAM version)
	void image_sram(const uint8_t *image, uint16_t first_line_no = 0, uint8_t line_count = 0) {
		this->frame_fixed_repeat(0xaa, EPD_compensate, first_line_no, line_count);
		this->frame_fixed_repeat(0xaa, EPD_white, first_line_no, line_count);
		this->frame_sram_repeat(image, EPD_inverse, first_line_no, line_count);
		this->frame_sram_repeat(image, EPD_normal, first_line_no, line_count);
	}
	// change from old image to new image (SRAM version)
	void image_sram(const uint8_t *old_image, const uint8_t *new_image, uint16_t first_line_no = 0, uint8_t line_count = 0) {
		this->frame_sram_repeat(old_image, EPD_compensate, first_line_no, line_count);
		this->frame_sram_repeat(old_image, EPD_white, first_line_no, line_count);
		this->frame_sram_repeat(new_image, EPD_inverse, first_line_no, line_count);
		this->frame_sram_repeat(new_image, EPD_normal, first_line_no, line_count);
	}
	//stage is 0..3 representing the stages in the update process
	void image_sram_external_repeat(const uint8_t *old_image, const uint8_t *new_image, uint8_t stage, uint16_t first_line_no = 0, uint8_t line_count = 0) {
	    switch(stage)
	    {
    	    case EPD_compensate:
    		    this->frame_sram(old_image, EPD_compensate, first_line_no, line_count);
	    	    break;
            case EPD_white:
	        	this->frame_sram(old_image, EPD_white, first_line_no, line_count);
	    	    break;
	        case EPD_inverse:
        		this->frame_sram(new_image, EPD_inverse, first_line_no, line_count);
	    	    break;
	        case EPD_normal:
        		this->frame_sram(new_image, EPD_normal, first_line_no, line_count);
	    	    break;
		}
	}
#endif

	// Low level API calls
	// ===================

	// single frame refresh
	void frame_fixed(uint8_t fixed_value, EPD_stage stage, uint16_t first_line_no = 0, uint8_t line_count = 0);
	void frame_data(PROGMEM const uint8_t *new_image, EPD_stage stage, uint16_t first_line_no = 0, uint8_t line_count = 0);
#if defined(EPD_ENABLE_EXTRA_SRAM)
	void frame_sram(const uint8_t *new_image, EPD_stage stage, uint16_t first_line_no = 0, uint8_t line_count = 0);
#endif
	void frame_cb(uint32_t address, EPD_reader *reader, EPD_stage stage, uint16_t first_line_no = 0, uint8_t line_count = 0);


	// stage_time frame refresh
	void frame_fixed_repeat(uint8_t fixed_value, EPD_stage stage, uint16_t first_line_no = 0, uint8_t line_count = 0);
	void frame_data_repeat(PROGMEM const uint8_t *new_image, EPD_stage stage, uint16_t first_line_no = 0, uint8_t line_count = 0);
#if defined(EPD_ENABLE_EXTRA_SRAM)
	void frame_sram_repeat(const uint8_t *new_image, EPD_stage stage, uint16_t first_line_no = 0, uint8_t line_count = 0);
#endif
	void frame_cb_repeat(uint32_t address, EPD_reader *reader, EPD_stage stage, uint16_t first_line_no = 0, uint8_t line_count = 0);

	// convert temperature to compensation factor
	int temperature_to_factor_10x(int temperature);

	// single line display - very low-level
	// also has to handle AVR progmem
	void line(uint16_t line_no, const uint8_t *data, uint8_t fixed_value, bool read_progmem, EPD_stage stage);

	// inline static void attachInterrupt();
	// inline static void detachInterrupt();

	EPD_Class(EPD_size size,
		  int panel_on_pin,
		  int border_pin,
		  int discharge_pin,
		  int pwm_pin,
		  int reset_pin,
		  int busy_pin,
		  int chip_select_pin);

};

#endif


