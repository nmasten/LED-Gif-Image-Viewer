/*
 * 32x32.h
 *
 *  Created on: Jun 1, 2024
 *      Author: noahmasten
 */

#ifndef INC_32X32_H_
#define INC_32X32_H_

#include "stm32l476xx.h"

#define SCAN_RATE (7)
#define MATRIX_HEIGHT 32
#define MATRIX_WIDTH 32
#define NUM_LEDS (MATRIX_HEIGHT * MATRIX_WIDTH)

typedef struct {
	float r;	// color R value, 0-255
	float g;	// color G value, 0-255
	float b;	// color B value, 0-255
} Color;

extern uint32_t *image_buffer;
extern uint32_t backbuffer[2][4*3*MATRIX_WIDTH];
extern volatile uint8_t backbuffer_index;
extern Color LED_Data[MATRIX_HEIGHT][MATRIX_WIDTH];

void set_pixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b);
void set_pixels_u32(uint16_t x, uint16_t y, uint32_t r, uint32_t g, uint32_t b);
void clear_led_matrix(void);

#endif /* INC_32X32_H_ */
