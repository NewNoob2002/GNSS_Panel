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

// 引脚表逐项迁自 Arduino/Core/variants/hc32f460xx/variant.cpp。
#include "pinmap.h"

//
// Pin Map
//

extern const pin_info_t PIN_MAP[BOARD_NR_GPIO_PINS] = {
	// GPIO 位号与端口（编号顺序保持不变）。
	// ---  PAx  ---
	{0,  GPIO_PORT_A},	// PA0
	{1,  GPIO_PORT_A},	// PA1
	{2,  GPIO_PORT_A},	// PA2
	{3,  GPIO_PORT_A},	// PA3
	{4,  GPIO_PORT_A},	// PA4
	{5,  GPIO_PORT_A},	// PA5
	{6,  GPIO_PORT_A},	// PA6
	{7,  GPIO_PORT_A},	// PA7
	{8,  GPIO_PORT_A},		// PA8
	{9,  GPIO_PORT_A},		// PA9
	{10, GPIO_PORT_A},		// PA10
	{11, GPIO_PORT_A},		// PA11
	{12, GPIO_PORT_A},		// PA12
	{13, GPIO_PORT_A},		// PA13
	{14, GPIO_PORT_A},		// PA14
	{15, GPIO_PORT_A},		// PA15
												//
												// ---  PBx  ---
												//
	{0,  GPIO_PORT_B},	// PB0
	{1,  GPIO_PORT_B},	// PB1
	{2,  GPIO_PORT_B},		// PB2
	{3,  GPIO_PORT_B},		// PB3
	{4,  GPIO_PORT_B},		// PB4
	{5,  GPIO_PORT_B},		// PB5
	{6,  GPIO_PORT_B},		// PB6
	{7,  GPIO_PORT_B},		// PB7
	{8,  GPIO_PORT_B},		// PB8
	{9,  GPIO_PORT_B},		// PB9
	{10, GPIO_PORT_B},		// PB10
	{11, GPIO_PORT_B},			// PB11
	{12, GPIO_PORT_B},		// PB12
	{13, GPIO_PORT_B},		// PB13
	{14, GPIO_PORT_B},		// PB14
	{15, GPIO_PORT_B},		// PB15
												//
												// ---  PCx  ---
												//
	{0,  GPIO_PORT_C}, // PC0
	{1,  GPIO_PORT_C}, // PC1
	{2,  GPIO_PORT_C},	// PC2
	{3,  GPIO_PORT_C},	// PC3
	{4,  GPIO_PORT_C},	// PC4
	{5,  GPIO_PORT_C},	// PC5
	{6,  GPIO_PORT_C},		// PC6
	{7,  GPIO_PORT_C},		// PC7
	{8,  GPIO_PORT_C},		// PC8
	{9,  GPIO_PORT_C},		// PC9
	{10, GPIO_PORT_C},		// PC10
	{11, GPIO_PORT_C},		// PC11
	{12, GPIO_PORT_C},		// PC12
	{13, GPIO_PORT_C},		// PC13
	{14, GPIO_PORT_C},		// PC14 XTAL32_OUT
	{15, GPIO_PORT_C},		// PC15 XTAL32_IN
												//
												// ---  PDx  ---
												//
	#ifdef GPIO_D
	{0,  GPIO_PORT_D},		// PD0
	{1,  GPIO_PORT_D},		// PD1
	{2,  GPIO_PORT_D},		// PD2
	{3,  GPIO_PORT_D},		// PD3
	{4,  GPIO_PORT_D},		// PD4
	{5,  GPIO_PORT_D},			// PD5
	{6,  GPIO_PORT_D},			// PD6
	{7,  GPIO_PORT_D},			// PD7
	{8,  GPIO_PORT_D},		// PD8
	{9,  GPIO_PORT_D},		// PD9
	{10, GPIO_PORT_D},		// PD10
	{11, GPIO_PORT_D},		// PD11
	{12, GPIO_PORT_D},		// PD12
	{13, GPIO_PORT_D},		// PD13
	{14, GPIO_PORT_D},		// PD14
	{15, GPIO_PORT_D},		// PD15
	#endif
												//
												// ---  PEx  ---
												//
	#ifdef GPIO_E
	{0,  GPIO_PORT_E},			// PE0
	{1,  GPIO_PORT_E},			// PE1
	{2,  GPIO_PORT_E},		// PE2
	{3,  GPIO_PORT_E},		// PE3
	{4,  GPIO_PORT_E},		// PE4
	{5,  GPIO_PORT_E},		// PE5
	{6,  GPIO_PORT_E},			// PE6
	{7,  GPIO_PORT_E},			// PE7
	{8,  GPIO_PORT_E},		// PE8
	{9,  GPIO_PORT_E},		// PE9
	{10, GPIO_PORT_E},		// PE10
	{11, GPIO_PORT_E},		// PE11
	{12, GPIO_PORT_E},		// PE12
	{13, GPIO_PORT_E},		// PE13
	{14, GPIO_PORT_E},		// PE14
	{15, GPIO_PORT_E},		// PE15
	#endif
												//
												// ---  PHx  ---
												//
	{0,  GPIO_PORT_H},		// PH0   XTAL_IN
	{1,  GPIO_PORT_H},		// PH1   XTAL_OUT
	{2,  GPIO_PORT_H},		// PH2
};
