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

#include "input.h"
#include "clock.h"


#define DEBOUNCE_TIME    50
#define CLICK_COUNT_TIME 250


void input_init()
{
	// Configure pins
	// PA0 = T2C1
	// PA1 = T2C2
	// PA2 = button 

	// Enable GPIOA
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

	// Set pins mode to alternate function
	GPIOA->CRL &= ~(GPIO_CRL_CNF0 | GPIO_CRL_MODE0 |
	                GPIO_CRL_CNF1 | GPIO_CRL_MODE1 |
	                GPIO_CRL_CNF2 | GPIO_CRL_MODE2);
	GPIOA->CRL |= GPIO_CRL_CNF0_1 | GPIO_CRL_CNF1_1; // T2C3/T2C4 = pulled input
	GPIOA->CRL |= GPIO_CRL_CNF0_1 | GPIO_CRL_CNF1_1 | GPIO_CRL_CNF2_1; // T2C3/T2C4/button = pulled input
	GPIOA->ODR |= GPIO_ODR_ODR0 | GPIO_ODR_ODR1 | GPIO_ODR_ODR2;       // pulled up
	
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

	// Set channel 1 as input from TI1
	TIM2->CCMR1 |= TIM_CCMR1_CC1S_0;
	// Set channel 2 as input from TI2
	TIM2->CCMR1 |= TIM_CCMR1_CC2S_0;
	// Set encoder mode, counting TI2 edges
	TIM2->SMCR  |= TIM_SMCR_SMS_0;
	// Start the timer
	TIM2->CR1   |= TIM_CR1_CEN;
}

uint8_t input_button_get()
{
	static uint8_t buttonClickCount = 0;
	static uint8_t buttonClickCounted = 0;
	static uint32_t buttonClickCountTime = 0;
	static uint8_t buttonState = 0;
	static uint32_t buttonTransitionTime = 0;

	uint8_t ret = 0;
	uint32_t now = get_ticks();
	uint8_t newButtonState = GPIOA->IDR & GPIO_IDR_IDR2;
	if(newButtonState != buttonState)
	{
		buttonState = newButtonState;
		buttonTransitionTime = now;
		buttonClickCounted = 0;
	}
	else
	{
		// If button is pressed and not bouncing then count the click
		if(buttonState && !buttonClickCounted && now - buttonTransitionTime > DEBOUNCE_TIME)
		{
			buttonClickCount += 1;
			buttonClickCounted = 1;
			buttonClickCountTime = now;
		}
	}

	// If the click count hasn't increased for a while then act on it
	if(buttonClickCount > 0 && now - buttonClickCountTime > CLICK_COUNT_TIME)
	{
		ret = buttonClickCount;
		buttonClickCount = 0;
	}

	return ret;
}

int16_t input_encoder_get()
{
	return (int16_t)TIM2->CNT;
}

void input_encoder_set(int16_t value)
{
	TIM2->CNT = value;
}

