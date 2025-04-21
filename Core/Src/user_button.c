/*
 * user_button.c
 *
 *  Created on: May 27, 2024
 *      Author: noahmasten
 */

#include "user_button.h"

void UserButton_Init(void) {
	// turn on clock to GPIOC

	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

	// Configure User Button (PC13) for input
	GPIOC->MODER &= ~(GPIO_MODER_MODE13);		// Input mode
	GPIOC->PUPDR |= (2 << GPIO_PUPDR_PUPD13_Pos); // Pull down resistor
	SYSCFG->EXTICR[3] = (2 << SYSCFG_EXTICR4_EXTI13_Pos);	// Configure PC13 interrupt
	EXTI->IMR1 |= (EXTI_IMR1_IM13);		// enable interrupt in external interrupt controller
	EXTI->RTSR1 |= (EXTI_RTSR1_RT13);	// enable interrupt on rising edge

	// enable interrupts in NVIC
	NVIC_EnableIRQ(EXTI15_10_IRQn);
	__enable_irq();
}
