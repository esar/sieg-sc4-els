/*
   Copyright (C) 2023 Stephen Robinson
  
   This file is part of Sieg SC4 ELS
  
   Sieg SC4 ELS is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 2 of the License, or
   (at your option) any later version.
  
   Sieg SC4 ELS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this code (see the file names COPING).  
   If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdint.h>
#include <stm32f103x6.h>
#include <system_stm32f1xx.h>

#include "clock.h"
#include "spindle_encoder.h"
#include "servo.h"
#include "display.h"
#include "input.h"
#include "config.h"
#include "tables.h"


#define TRUE  1
#define FALSE 0

#define CHANGE_TIMEOUT   5000

#define UI_STATE_IDLE            0
#define UI_STATE_CHANGE_UNITS    1
#define UI_STATE_CHANGE_VALUE    2
#define UI_STATE_FAULT           3

#define UNITS_MAX    3
#define UNITS_MIN    0

#define FAULT_TOO_MANY_STEPS 1
#define FAULT_SERVO_ALARM    2


volatile static uint32_t steps_per_pulse = 0;
volatile static uint8_t reverse = 0;
volatile static uint8_t fault = 0;


void SysTick_Handler (void)
{
	static uint32_t servo_current = 0x80000000;
	static uint32_t encoder_current = 0x80000000;
	static uint32_t last_steps_per_pulse = -1;
	static uint8_t last_reverse = 0;
	static uint16_t last_encoder_pos = 0;
	static int16_t last_encoder_diff = 0;
	static volatile int32_t steps = 0;

	++ticks;

	// Update a 32 bit encoder position from the 16 bit counter
	uint16_t encoder_pos = spindle_encoder_get();
	int16_t encoder_diff = (int16_t)(encoder_pos - last_encoder_pos);
	// If direction is different to last then ignore small difference
	// until it builds up, that way we filter out jitter from the encoder
	if((encoder_diff ^ last_encoder_diff) < 0)
		if(encoder_diff > -10 && encoder_diff < 10)
			return;
	last_encoder_diff = encoder_diff;
	last_encoder_pos = encoder_pos;
	encoder_current += encoder_diff;

	// If the steps per pulse or direction has changed then reset the 
	// counters so that the servo doesn't suddenly need to be in a 
	// radically different position
	// If the current encoder position is below the threshold then
	// we've either wrapped around from high to low, or we're going to
	// wrap around from low to high imminently, either of which will
	// cause a huge jump in servo position, so reset
	if(steps_per_pulse != last_steps_per_pulse ||
	   reverse != last_reverse || 
	   encoder_current < 10000)
	{
		last_steps_per_pulse = steps_per_pulse;
		last_reverse = reverse;
		encoder_current = 0x80000000 + encoder_diff;
		servo_current = FROM_FIXED_MULT(((uint64_t)0x80000000<<16) * last_steps_per_pulse);
	}


	// Calculate the target servo position from the current encoder position
	uint32_t servo_target = FROM_FIXED_MULT(((uint64_t)encoder_current<<16) * last_steps_per_pulse);
	steps = (int32_t)(servo_target - servo_current);
	uint32_t abs_steps = steps < 0 ? 0 - steps : steps;

	// If too many steps for timer repeat register then we've fallen too far behind
	if(abs_steps > 255)
		fault = FAULT_TOO_MANY_STEPS;
	if(servo_alarm_get())
		fault = FAULT_SERVO_ALARM;
	if(!fault)
	{
		// If we have steps to make and the timer has finished sending
		// the last train of pulses, then set the direction and step count
		// and enable the pulse timer
		if(steps != 0 && servo_is_idle())
		{
			uint8_t reverse_direction = steps < 0;
			if(reverse)
				reverse_direction = !reverse_direction;
			servo_set_direction(reverse_direction);
			servo_step(abs_steps);
			servo_current += steps;
		}
	} else {
		servo_stop();
		last_steps_per_pulse = 0;   // trigger reset when fault is cleared
	}
}

void ui_update()
{
	static uint8_t uiState = UI_STATE_IDLE;
	static uint8_t activeUnits = DEFAULT_UNIT;
	static int16_t activeValue = DEFAULT_VALUE;
	static uint8_t activeReverse = 0;
	static int8_t changeUnits = 0;
	static int16_t changeValue = 0;
	static uint8_t changeReverse = 0;
	static uint32_t lastChangeTime = 0;

	if(fault)
	{
		uiState = UI_STATE_FAULT;
	}

	uint32_t now = get_ticks();
	uint8_t buttonClicks = input_button_get();
	if(buttonClicks > 0)
	{
		if(uiState == UI_STATE_IDLE)
		{
			if(buttonClicks > 1)
			{
				input_encoder_set(activeUnits);
				uiState = UI_STATE_CHANGE_UNITS;
			}
			else
			{
				if(activeReverse) {
					input_encoder_set(0 - activeValue - 1);
				} else {
					input_encoder_set(activeValue);
				}
				uiState = UI_STATE_CHANGE_VALUE;
			}
		}
		else
		{
			if(uiState == UI_STATE_CHANGE_UNITS)
			{
				activeUnits = changeUnits;
				input_encoder_set(activeValue);
				uiState = UI_STATE_CHANGE_VALUE;
			}
			else if(uiState == UI_STATE_CHANGE_VALUE)
			{
				activeValue = changeValue;
				activeReverse = changeReverse;
				uiState = UI_STATE_IDLE;
			}
			else if(uiState == UI_STATE_FAULT)
			{
				if(buttonClicks > 3) // quad-click to clear fault
				{
					uiState = UI_STATE_IDLE;
					fault = FALSE;
				}
			}
		}

		lastChangeTime = now;
	}

	uint8_t displayUnits = activeUnits;
	if(uiState == UI_STATE_CHANGE_UNITS)
	{
		int16_t newValue = input_encoder_get();
		if(newValue != changeUnits)
			lastChangeTime = now;
		changeUnits = newValue;
		if(changeUnits > UNITS_MAX)
		{
			changeUnits = UNITS_MAX;
			input_encoder_set(changeUnits);
		}
		if(changeUnits < UNITS_MIN)
		{
			changeUnits = UNITS_MIN;
			input_encoder_set(changeUnits);
		}
		displayUnits = changeUnits;
	}

	table_entry_t* table;
	uint8_t tableSize;
	if(displayUnits == 0)
	{
		table = table_mm_feed;
		tableSize = sizeof(table_mm_feed) / sizeof(*table_mm_feed);
	}
	else if(displayUnits == 1)
	{
		table = table_mm_thread;
		tableSize = sizeof(table_mm_thread) / sizeof(*table_mm_thread);
	}
	else if(displayUnits == 2)
	{
		table = table_inch_feed;
		tableSize = sizeof(table_inch_feed) / sizeof(*table_inch_feed);
	}
	else if(displayUnits == 3)
	{
		table = table_inch_thread;
		tableSize = sizeof(table_inch_thread) / sizeof(*table_inch_thread);
	}

	if(uiState == UI_STATE_CHANGE_VALUE)
	{
		int16_t newValue = input_encoder_get();
		if(newValue != changeValue)
			lastChangeTime = now;
		changeValue = newValue;
		if(changeValue < 0)
		{
			changeValue = 0 - changeValue - 1;
			changeReverse = 1;
		}
		else
		{
			changeReverse = 0;
		}

		if(changeValue >= tableSize)
		{
			changeValue = tableSize - 1;
			if(changeReverse)
			{
				input_encoder_set(0 - changeValue - 1);
			}
			else
			{
				input_encoder_set(changeValue);
			}
		}
	}

	if(now - lastChangeTime > CHANGE_TIMEOUT && uiState != UI_STATE_FAULT)
		uiState = UI_STATE_IDLE;

	
	uint8_t leds;
	uint8_t digit1;
	uint8_t digit10;
	uint8_t digit100;
	uint8_t digit1000;
	uint8_t flashBlank = ((now >> 7) & 3) == 3;
	if(uiState == UI_STATE_FAULT)
	{
		if(!flashBlank)
		{
			digit1 = MINUS;
			digit10 = fault;
			digit100 = ERROR;
			digit1000 = MINUS;
		}
		else
		{
			digit1 = digit10 = digit100 = digit1000 = BLANK;
		}
	}
	else if(uiState == UI_STATE_CHANGE_VALUE)
	{
		if(!flashBlank)
		{
			digit1 = table[changeValue].dig1;
			digit10 = table[changeValue].dig10;
			digit100 = table[changeValue].dig100;
			if(changeReverse)
				digit1000 = MINUS;
			else
				digit1000 = BLANK;
		}
		else
			digit1 = digit10 = digit100 = digit1000 = BLANK;
	}
	else
	{
		digit1 = table[activeValue].dig1;
		digit10 = table[activeValue].dig10;
		digit100 = table[activeValue].dig100;
		if(activeReverse)
			digit1000 = MINUS;
		else
			digit1000 = BLANK;
	}

	if(uiState == UI_STATE_CHANGE_UNITS)
	{
		if(flashBlank)
			leds = 0;
		else
			leds = 1 << (changeUnits + 1);
	}
	else
		leds = 1 << (activeUnits + 1);

	display_write(MAX7219_DIGIT0, digit1);
	display_write(MAX7219_DIGIT1, digit10);
	display_write(MAX7219_DIGIT2, digit100);
	display_write(MAX7219_DIGIT3, digit1000);
	display_write(MAX7219_DIGIT4, leds);

	steps_per_pulse = table[activeValue].steps_per_pulse;
	reverse = activeReverse;
}


void main (void)
{
	clock_init ();
	spindle_encoder_init();
	servo_init();
	display_init();
	input_init();

	delay_msec(500);

	// decode first 4
	display_write(MAX7219_DECODE_MODE, 0x0f);
	// set intensity
	display_write(MAX7219_INTENSITY, 2);
	// scan limit 5 digits
	display_write(MAX7219_SCAN_LIMIT, 4);
	// digit0 = 1
	display_write(MAX7219_DIGIT0, BLANK);
	display_write(MAX7219_DIGIT1, BLANK);
	display_write(MAX7219_DIGIT2, BLANK);
	display_write(MAX7219_DIGIT3, BLANK);
	display_write(MAX7219_DIGIT4, BLANK);
	// normal mode, not display test
	display_write(MAX7219_DISPLAY_TEST, 0);
	// enable
	display_write(MAX7219_SHUTDOWN, 1);


	while(TRUE)
	{
		ui_update();
		delay_msec(10);
	}
}

void _init()
{
}
