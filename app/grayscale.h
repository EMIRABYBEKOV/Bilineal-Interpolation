#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/**
 * @brief This function implements the naive grayscale method,
 * function takes ppm format as input and processes it, saving pixels in array for pgm format as output
 * @param arr_of_img Array of image pixels
 * @param width, @param height Image dimensions
 * @param a, @param b, @param blength Coefficients for converting to grayscale,
 * if all three are equal to zero, default values will be used
 */
void grayscale(const uint8_t *arr_of_img, uint8_t *res_img, size_t width, size_t height, float a, float b, float c);