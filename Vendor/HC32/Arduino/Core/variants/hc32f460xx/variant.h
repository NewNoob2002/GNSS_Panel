/*
 Copyright (c) 2015 Arduino LLC.  All right reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef BOARD_VARIANT_H_
#define BOARD_VARIANT_H_



//
// GPIO pin aliases (index into PIN_MAP array)
//

typedef enum
{
    PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7, PA8, PA9, PA10, PA11, PA12, PA13, PA14, PA15,
    PB0, PB1, PB2, PB3, PB4, PB5, PB6, PB7, PB8, PB9, PB10, PB11, PB12, PB13, PB14, PB15,
    PC0, PC1, PC2, PC3, PC4, PC5, PC6, PC7, PC8, PC9, PC10, PC11, PC12, PC13, PC14, PC15,
#ifdef GPIO_D
    PD0, PD1, PD2, PD3, PD4, PD5, PD6, PD7, PD8, PD9, PD10, PD11, PD12, PD13, PD14, PD15,
#endif
#ifdef GPIO_E
    PE0, PE1, PE2, PE3, PE4, PE5, PE6, PE7, PE8, PE9, PE10, PE11, PE12, PE13, PE14, PE15,
#endif
		PH0, PH1, PH2,
    PIN_MAX
} Pin_TypeDef;

//
// GPIO pin count (size of PIN_MAP array)
//
#define BOARD_NR_GPIO_PINS (PIN_MAX)


//
// USART gpio pins
//
#define VARIANT_USART1_TX_PIN PA4
#define VARIANT_USART1_RX_PIN PA11

#define VARIANT_USART2_TX_PIN PA10
#define VARIANT_USART2_RX_PIN PA15

#define VARIANT_USART3_TX_PIN PB12
#define VARIANT_USART3_RX_PIN PB13

#endif /* BOARD_VARIANT_H_ */
