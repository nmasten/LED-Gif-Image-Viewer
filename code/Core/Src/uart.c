/*
 * uart.c
 *
 *  Created on: May 3, 2023
 *      Author: Lucas Caputi, Noah Masten
 */

#include "uart.h"
#include <stdio.h>
#include <string.h>

/* CONFIGURES PINS PA2 (TX) and PA3 (RX) for USART */
void UART_Init(void) {

	/* enable clock for GPIOA and USART2 */
		RCC->AHB2ENR 		|=  (RCC_AHB2ENR_GPIOAEN);
		RCC->APB1ENR1		|= (RCC_APB1ENR1_USART2EN);

		/* GPIO config (PA2->Tx and PA3->Rx) */
		GPIOA->AFR[0]   	&= ~(GPIO_AFRL_AFSEL2_Msk | GPIO_AFRL_AFSEL3_Msk);	// clear AFR
		GPIOA->AFR[0]		|=  ( (0x7UL << GPIO_AFRL_AFSEL2_Pos) |				// set PA2, PA3 to AF7
								  (0x7UL << GPIO_AFRL_AFSEL3_Pos) );
		GPIOA->MODER 		&= ~(GPIO_MODER_MODE2 | GPIO_MODER_MODE3); 			// clear mode2 and mode3
		GPIOA->MODER 		|=  (GPIO_MODER_MODE2_1 | GPIO_MODER_MODE3_1);		// set to alternate function
		GPIOA->OTYPER 		&= ~(GPIO_OTYPER_OT2 | GPIO_OTYPER_OT3);			// set OTYPE2 and OTYPE3 to push-pull
		GPIOA->OSPEEDR 		|=  (GPIO_OSPEEDR_OSPEED2 | GPIO_OSPEEDR_OSPEED3);	// set OSPEED2 and OPSEED3 to high speed
		GPIOA->PUPDR 		&= ~(GPIO_PUPDR_PUPD2 | GPIO_PUPDR_PUPD3);			// set PUPD2 and PUPD3 to no pull-up/pull-down

		/* USART2 config */
		USART2->CR1 &= ~(USART_CR1_M);										// set word length to 8 bits
		USART2->CR1 |=  (USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE); 	// transmit enable, read enable, RXNE interrupt enable
		USART2->CR2 &= ~(USART_CR2_MSBFIRST | USART_CR2_STOP);		 		// LSB first, 1 stop bit
		USART2->BRR  =  (USART_DIV);										// baud rate configuration
		USART2->CR1 |=  (USART_CR1_UE);										// enable USART2

		/* enable interrupts in NVIC */
		NVIC->ISER[1] = (1 << (USART2_IRQn & 0x1F));

		/* enable interrupts globally */
		__enable_irq();

}

void UART_print(char *input_string) {
	for(int i = 0; i < strlen(input_string); i++) {
		USART2->TDR = input_string[i];			// transmit character
		while(!(USART2->ISR & USART_ISR_TC));	// wait until transmission is complete
	}
}

/**
  * @brief Transmits escape + input_string
  * @retval None
  */
void UART_ESC_Code(char *input_string) {
	USART2->TDR = 0x1B;						// transmit escape
	while(!(USART2->ISR & USART_ISR_TC));	// wait until transmission is complete

	for(int i = 0; i < strlen(input_string); i++) {
		USART2->TDR = input_string[i];			// transmit character
		while(!(USART2->ISR & USART_ISR_TC));	// wait until transmission is complete
	}
}

/**
  * @brief Transmits single character
  * @retval None
  */
void UART_print_char(int input_char) {
	USART2->TDR = input_char; // transmit character
	while(!(USART2->ISR & USART_ISR_TC)); // wait until transmission is complete
}

/**
  * @brief Clears terminal and resets cursor to top left
  * @retval None
  */
void UART_Reset_Terminal(void) {
	UART_ESC_Code("[2J");	// Clear screen
	UART_ESC_Code("[H");	// Move cursor to top left corner
}
