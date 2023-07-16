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

#include "display.h"


void display_init()
{
	// Configure pins
	// PA4 = SPI1_NSS
	// PA5 = SPI1_SCLK
	// PA6 = SPI1_MISO
	// PA7 = SPI1_MOSI

	// Enable GPIOA
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;

	// Set pin mode to alternate function
	GPIOA->CRL &= ~(GPIO_CRL_CNF4 | GPIO_CRL_MODE4 |
	                GPIO_CRL_CNF5 | GPIO_CRL_MODE5 |
	                GPIO_CRL_CNF6 | GPIO_CRL_MODE6 |
	                GPIO_CRL_CNF7 | GPIO_CRL_MODE7);
	GPIOA->CRL |= GPIO_CRL_CNF5_1 | GPIO_CRL_MODE5_0;  // CLK = AF push/pull, 10MHz
	GPIOA->CRL |= GPIO_CRL_CNF6_0;                     // MISO = floating input
	GPIOA->CRL |= GPIO_CRL_CNF7_1 | GPIO_CRL_MODE7_0;  // MOSI = AF push/pull, 10MHz
	GPIOA->CRL |= GPIO_CRL_MODE4_0;                    // NSS = push/pull

	// Set NSS high
	GPIOA->ODR |= GPIO_ODR_ODR4;

        // Enable SPI1 Clock
        RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

        // Disable SPI1 for configuration and reset
        SPI1->CR1 &= ~SPI_CR1_SPE;
        RCC->APB2RSTR |= RCC_APB2RSTR_SPI1RST;
        RCC->APB2RSTR &= ~RCC_APB2RSTR_SPI1RST;

        // Set clock rate, polarity and phase
        SPI1->CR1 = (4 << SPI_CR1_BR_Pos) | SPI_CR1_CPOL | SPI_CR1_CPHA;

        // Set as master
        SPI1->CR1 |= SPI_CR1_MSTR;

        SPI1->CR2 |= SPI_CR2_SSOE;

}

void display_write(uint8_t reg, uint8_t value)
{
	uint8_t buf[2] = {reg, value};

	SPI1->SR &= ~SPI_SR_MODF;
	SPI1->CR1 |= SPI_CR1_SPE;
	SPI1->CR1 |= SPI_CR1_MSTR;

	// NSS low
	GPIOA->ODR &= ~GPIO_ODR_ODR4;

	for(uint8_t i = 0; i < sizeof(buf); ++i)
	{
		while(!(SPI1->SR & SPI_SR_TXE))
			;
		*(uint8_t*)&SPI1->DR = buf[i];
	}

	while(SPI1->SR & SPI_SR_BSY)
		;

	SPI1->CR1 &= ~SPI_CR1_SPE;

	// NSS high
	GPIOA->ODR |= GPIO_ODR_ODR4;
}


