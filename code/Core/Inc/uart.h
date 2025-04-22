/*
 * uart.h
 *
 *  Created on: May 3, 2023
 *      Author: Lucas Caputi, Noah Masten
 */

#ifndef SRC_UART_H_
#include "stm32l476xx.h"

#define SRC_UART_H_

#define CLK_SPEED 32000000
#define BAUD_RATE 115200 // 115.2 kpbs
#define USART_DIV (CLK_SPEED / BAUD_RATE) // clock frequency divided by baud rate, rounded up

void UART_Init(void);
void UART_print(char* data);
void UART_ESC_Code(char *input_string);
void UART_print_char(int input_char);
void UART_Reset_Terminal(void);

#endif /* SRC_UART_H_ */
