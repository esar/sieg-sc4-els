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

#include "config.h"
#include "spindle_encoder.h"


void spindle_encoder_init()
{
	// Configure pins
	// PB4 = T3C1
	// PB5 = T3C2

	// Enable GPIOB
	RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
	RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;

	AFIO->MAPR &= ~AFIO_MAPR_TIM3_REMAP;
	AFIO->MAPR |= AFIO_MAPR_TIM3_REMAP_1;

	// Set pins to alternate function
	GPIOB->CRL &= ~(GPIO_CRL_CNF4 | GPIO_CRL_MODE4 |
	                GPIO_CRL_CNF5 | GPIO_CRL_MODE5);
	GPIOB->CRL |= GPIO_CRL_CNF4_1 | GPIO_CRL_CNF5_1; // T3C1/T3C2 = pulled input
	GPIOB->ODR |= GPIO_ODR_ODR4 | GPIO_ODR_ODR5;     // T3C1/T3C2 = pulled up

	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

	// Set channel 1 as input from TI1
	TIM3->CCMR1 |= TIM_CCMR1_CC1S_0;
	// Set channel 2 as input from TI2
	TIM3->CCMR1 |= TIM_CCMR1_CC2S_0;
#ifdef REVERSE_DIRECTION
	// Switch polarity of one of the inputs
	TIM3->CCER |= TIM_CCER_CC1P;
#endif
	// Set encoder mode, counting both edges
	TIM3->SMCR  |= TIM_SMCR_SMS_0 | TIM_SMCR_SMS_1;
	// Start the timer
	TIM3->CR1   |= TIM_CR1_CEN;
}

uint16_t spindle_encoder_get()
{
	//static uint16_t pos = 0;
	//pos += 100;
	//return pos;
	return TIM3->CNT;
}

