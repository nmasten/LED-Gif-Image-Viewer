/*
 * 32x32.c
 *
 *  Created on: Jun 1, 2024
 *      Author: noahmasten
 */

#include "32x32.h"

/**
  * @brief  Writes RGB values to a single pixel, using the specified x and y coordinates
  * Source: https://github.com/cpmetz/32x32-instructable/blob/master/main.patch
  */
void set_pixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b)
{
	uint16_t offset = y * 4 * 3;
	offset += x >> 3;
	// shift by the remainder times 4
	uint8_t shift = (x & 0x7u) << 2;
	uint32_t mask = ~(0xFu << shift);
	uint32_t pr = ((uint32_t)(r) & 0xFu) << shift;
	uint32_t pg = ((uint32_t)(g) & 0xFu) << shift;
	uint32_t pb = ((uint32_t)(b) & 0xFu) << shift;

	image_buffer[offset + 0] = (image_buffer[offset + 0] & mask) | pb;
	image_buffer[offset + 4] = (image_buffer[offset + 4] & mask) | pg;
	image_buffer[offset + 8] = (image_buffer[offset + 8] & mask) | pr;
}

/**
  * @brief  Writes RGB values to a pixel using 32-bit color
  * Source: https://github.com/cpmetz/32x32-instructable/blob/master/main.patch
  */
void set_pixels_u32(uint16_t x, uint16_t y, uint32_t r, uint32_t g, uint32_t b)
{
	uint16_t offset = y * 4 * 3;
	offset += x >> 3;
	image_buffer[offset + 0] = r;
	image_buffer[offset + 4] = g;
	image_buffer[offset + 8] = b;
}

/**
  * @brief Writes RGB values (0,0,0) to all pixels, clearing the matrix
  */
void clear_led_matrix(void) {
	for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
		for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
			image_buffer = backbuffer[0];
			set_pixel(x,y, 0, 0, 0);
			image_buffer = backbuffer[1];
			set_pixel(x,y, 0, 0, 0);
		}
	}
}
