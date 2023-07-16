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


volatile uint32_t ticks = 0;


void clock_init (void)
{
	// enable HSE
	RCC->CR |= 0x10001;
	while((RCC->CR & 0x20000) == 0)
		;
	// enable flash prefetch
	FLASH->ACR = 0x12;
	// config pll
	RCC->CFGR |= 0x1d0400; // pll=72MHz, APB1=36MHz, PHB=72MHz
	RCC->CR |= 0x1000000; //enable pll
	while((RCC->CR & 0x3000000) == 0)
		;
	// set sysclock as pll
	RCC->CFGR |= 2;
	while((RCC->CFGR & 0x8) == 0)
		;

	//Update SystemCoreClock variable according to Clock Register Values.
	SystemCoreClockUpdate();
	SysTick_Config(SystemCoreClock / 1000U); // seconds
	__enable_irq();
}


uint32_t get_ticks(void)
{
	return ticks;
}

void delay_msec(int millis) 
{
	uint32_t tmp = get_ticks();
	while ((get_ticks() - tmp) < millis)
		;
}
