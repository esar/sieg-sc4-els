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

#include "servo.h"


void servo_init()
{
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

	// PA8 = step = T1C1 = push/pull output (10MHz)
	GPIOA->CRH &= ~(GPIO_CRH_CNF8 | GPIO_CRH_MODE8);
	GPIOA->CRH |= GPIO_CRH_CNF8_1 | GPIO_CRH_MODE8_0;
	
	// PA9 = direction = gpio push/pull output (10MHz)
	GPIOA->CRH &= ~(GPIO_CRH_CNF9 | GPIO_CRH_MODE9);
	GPIOA->CRH |= GPIO_CRH_MODE9_0;

	// PA10 = alarm = floating input
	GPIOA->CRH &= ~(GPIO_CRH_CNF10 | GPIO_CRH_MODE10);
	GPIOA->CRH |= GPIO_CRH_MODE10_0;
	
	// Enable TIM1
	RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

	// Cnfigure timer 1 for 50% duty cycle, one pulse output, with repeat count
	TIM1->CR1 &= ~TIM_CR1_CKD;      // no clock division
	TIM1->ARR = 8;                  // auto reload = 8
	TIM1->CCR1 = 4;                 // compare = 4 (50%)
	TIM1->PSC = (uint16_t)((SystemCoreClock / 1000000) - 1);    // prescaler
	TIM1->RCR = 1;                  // repeat count = 1
	TIM1->EGR = TIM_EGR_UG;         // reinit counter
	TIM1->SMCR = 0;                 // slave mode disabled
	TIM1->CR1 |= TIM_CR1_OPM;       // one pulse mode
	TIM1->CCMR1 &= (uint16_t)~TIM_CCMR1_OC1M;
	TIM1->CCMR1 |= TIM_CCMR1_OC1M_0 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2;  // PWM mode 2
	TIM1->CCER &= (uint16_t)~TIM_CCER_CC1P;  // output active high
	TIM1->CCER |= TIM_CCER_CC1E;    // channel 1 output enable
	TIM1->BDTR |= TIM_BDTR_MOE;     // main output enable
	TIM1->CR1 |= TIM_CR1_CEN;       // enable

	GPIOA->BSRR |= GPIO_BSRR_BS9;
}

uint8_t servo_is_idle()
{
	return (TIM1->CR1 & TIM_CR1_CEN) == 0;
}

void servo_set_direction(uint8_t reverse)
{	
	GPIOA->BSRR |= reverse ? GPIO_BSRR_BS9 : GPIO_BSRR_BR9;
}

void servo_step(uint8_t steps)
{
	TIM1->RCR = steps - 1;
	TIM1->CR1 |= TIM_CR1_CEN;
}

void servo_stop()
{
	TIM1->CR1 &= ~TIM_CR1_CEN;  // stop servo pulses
}

uint8_t servo_alarm_get()
{
	static uint8_t value = 0;
	static uint8_t last_value = 0;
	static uint8_t stable_count = 0;

	uint8_t new_value = GPIOA->IDR & GPIO_IDR_IDR10;
	if(new_value != last_value)
	{
		stable_count = 0;
		last_value = new_value;
	}
	if(stable_count > 10)
		value = last_value;
	else
		stable_count += 1;

	return value;	
}
