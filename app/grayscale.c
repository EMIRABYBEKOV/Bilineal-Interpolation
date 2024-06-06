#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "grayscale.h"

void grayscale(const uint8_t *arr_of_img, uint8_t *res_img, size_t width, size_t height, float a, float b, float c) {
    //if Coefficients are not set
    if(a == 0 && b == 0 && c == 0){
        a = 0.299;
        b = 0.587;
        c = 0.114;
    }
    //Convert every pixel to grayscale
    float divisor = a + b + c;
    for (size_t i = 0; i < width * height; ++i) {
        uint8_t gs_value = (arr_of_img[i * 3] * a + arr_of_img[i * 3 + 1] * b + arr_of_img[i * 3 + 2] * c) / divisor;
        // Assign the grayscale value to all channels
        res_img[i] = gs_value;
    }
}

