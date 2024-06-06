#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <immintrin.h>
#include "interpolate.h"
#include "grayscale.c"


/**
 * Еhis function is an implementation of a form of bilineal interpolation
 * @param scale_factor Scaling factor
 * @param x Pixel location along the x-axis.
 * @param y Pixel location along the y-axis.
 * @param q00 Top left corner pixel
 * @param qs0 Top right corner pixel
 * @param q0s Bottom left corner pixel
 * @param qss Bottom right corner Pixel
 */
unsigned char matrix_formula(size_t s, size_t x, size_t y, uint8_t q00, uint8_t qs0, uint8_t q0s, uint8_t qss){

    size_t s_2 = s * s;
    size_t res = (((s - y) * q00 + y * q0s)*(s - x) + x * ((s - y) * qs0 + y * qss)) / s_2;

    return (uint8_t)res;
}

/**
 * Еhis function is an implementation of a multiplication of two __m128i
 * @param a first 4 int
 * @param b second 4 int
 */
__m128i _mm_mul_epi32_gra(__m128i a, __m128i b) {
    
    int ia[4] __attribute__ ((aligned (16)));
    int ib[4] __attribute__ ((aligned (16)));
    _mm_store_si128((__m128i *)ia, a);
    _mm_store_si128((__m128i *)ib, b);

    int res[4] __attribute__ ((aligned (16)));
    res[0] = ia[0] * ib[0];
    res[1] = ia[1] * ib[1];
    res[2] = ia[2] * ib[2];
    res[3] = ia[3] * ib[3];

    return _mm_load_si128((__m128i *)res);
}

__m128i _mm_div_epi32_gra(__m128i a, __m128i b) {
    int ia[4] __attribute__ ((aligned (16)));
    int ib[4] __attribute__ ((aligned (16)));
    _mm_store_si128((__m128i *)ia, a);
    _mm_store_si128((__m128i *)ib, b);

    int res[4] __attribute__ ((aligned (16)));
    res[0] = ia[0] / ib[0];
    res[1] = ia[1] / ib[1];
    res[2] = ia[2] / ib[2];
    res[3] = ia[3] / ib[3];

    return _mm_load_si128((__m128i *)res);
}

/**
 * This function is an implementation of a form of bilineal interpolation using 128 bit registers.
 */
__m128i matrix_formula_V1(__m128i nil, __m128i s, __m128i s_2, __m128i x, __m128i y, uint8_t q00, uint8_t qs0, uint8_t q0s, uint8_t qss){
    // size_t res = (((s - y) * q00 + y * q0s)*(s - x) + x * ((s - y) * qs0 + y * qss)) / s_2;
    __m128i smenoy = _mm_sub_epi32(s, y);
    __m128i smenox = _mm_sub_epi32(s, x);

    // First half
    __m128i res1 = nil;
    for (uint8_t i = 0; i < q00; i++) {
        res1 = _mm_add_epi32(res1, smenoy);
    }
    __m128i res2 = nil;
    for (uint8_t i = 0; i < q0s; i++) {
        res2 = _mm_add_epi32(res2, y);
    }
    __m128i res3 = _mm_add_epi32(res1, res2);

    // Second half
    __m128i res4 = nil;
    for (uint8_t i = 0; i < qss; i++) {
        res4 = _mm_add_epi32(res4, y);
    }
    __m128i res5 = nil;
    for (uint8_t i = 0; i < qs0; i++) {
        res5 = _mm_add_epi32(res5, smenoy);
    }
    __m128i res6 = _mm_add_epi32(res4, res5);

    // Last part
    __m128i res7 = _mm_mul_epi32_gra(res3, smenox);
    __m128i res8 = _mm_mul_epi32_gra(res6, x);
    __m128i res9 = _mm_add_epi32(res7, res8);

    return _mm_div_epi32_gra(res9, s_2);
}

/**
 * Еhis function is for finding all possible values for spaces out of 4 pixel
 * @param scale_factor Scaling factor
 * @param width Width of image
 * @param height Height of image
 * @param index Pixel index in the image array.
 * @param q00 Top left corner pixel
 * @param qs0 Top right corner pixel
 * @param q0s Bottom left corner pixel
 * @param qss Bottom right corner Pixel
 * @param res_img Array of image with saved result after calculation
 * @param y_index Pixel location of top right pixel along the y-axis in initial array.
 * @param x_index Pixel location of top right pixel along the x-axis in initial array.
 */
void interpolate_small(size_t scale_factor, size_t width, size_t height, size_t index, uint8_t q00, uint8_t qs0, uint8_t q0s, uint8_t qss, uint8_t *res_img,
                       size_t y_index, size_t x_index){

    //Тew image characteristics
    size_t new_width = width * scale_factor;

    //Step for moving to next correct pixel index
    size_t step = new_width - scale_factor + 1;


    //Index of first pixel index in CENTER of 4 Pixels
    size_t index2 = index + new_width + 1;
    //Loop for calculation center space pixels
    for (size_t y = 1; y < scale_factor; y++){
        for (size_t x = 1; x < scale_factor; x++){
            res_img[index2] = matrix_formula(scale_factor, x, y, q00, qs0, q0s, qss);
            index2 += 1;
        }
        index2 += step;
    }


    //Loop for calculation UPPER space pixels, between Q(0,0) and Q(s,0)
    size_t counter_x = 1;
    index2 = index + scale_factor;
    for (size_t x = index + 1; x < index2; x++){
        //If has edgepixels
        if (y_index > 0){
            res_img[x] = (res_img[x] + matrix_formula(scale_factor, counter_x, 0, q00, qs0, q0s, qss)) / 2;
        }else{
            res_img[x] = matrix_formula(scale_factor, counter_x, 0, q00, qs0, q0s, qss);
        }
        counter_x += 1;
    }

    //Loop for calculation UNDER space pixels, between Q(0,s) and Q(s,s)
    counter_x = 1;
    index2 = index + 1 + new_width * scale_factor;
    size_t end_x = index2 + scale_factor - 1;
    for (size_t x = index2; x < end_x; x++){
        res_img[x] = matrix_formula(scale_factor, counter_x, scale_factor, q00, qs0, q0s, qss);
        counter_x += 1;
    }


    //Finding correct index of pixel in result array
    size_t counter_y = index + new_width + scale_factor;
    //Loop for calculation RIGHT space pixels, between Q(s,0) and Q(s,s)
    for (size_t y = 1; y < scale_factor; y++){
        res_img[counter_y] = matrix_formula(scale_factor, scale_factor, y, q00, qs0, q0s, qss);
        counter_y += new_width;
    }


    //Finding correct index of pixel in result array
    counter_y = index + new_width;
    //Loop for calculation LEFT space pixels, between Q(0,0) and Q(0,s)
    for (size_t y = 1; y < scale_factor; y++){
        //If has edgepixels
        if (x_index > 0){
            res_img[counter_y] = (res_img[counter_y] + matrix_formula(scale_factor, 0, y, q00, qs0, q0s, qss)) / 2;
        }else{
            res_img[counter_y] = matrix_formula(scale_factor, 0, y, q00, qs0, q0s, qss);
        }
        counter_y += new_width;
    }


    //Below is the code for the last edge pixels. If the rightmost 4 pixels are the last ones from the right.
    //The space pixels will be counted with corner pixels where Q(0,0) = Q(s,s) and Q(0,s) = Q(s,s)
    if (x_index == (width - 2)){
        //Finding corrrect index of pixel in result array
        index2 = index + new_width + 1 + scale_factor;
        //Loop for calculation CENTER space pixels
        for (size_t y = 1; y < scale_factor; y++){
            for (size_t x = 1; x < scale_factor; x++){
                res_img[index2] = matrix_formula(scale_factor, x, y, qs0, qs0, qss, qss);
                index2 += 1;
            }
            index2 += step;
        }


        //Loop for calculation UPPER space pixels, between Q(0,0) and Q(s,0)
        size_t counter_x = 1;
        index2 = index + scale_factor + 1;
        end_x = index + scale_factor + scale_factor;
        for (size_t x = index2; x < end_x; x++){
            //If has edgepixels
            if ((y_index > 0)){
                res_img[x] = (res_img[x] + matrix_formula(scale_factor, counter_x, 0, qs0, qs0, qss, qss)) / 2;
            }else{
                res_img[x] = matrix_formula(scale_factor, counter_x, 0, qs0, qs0, qss, qss);
            }
            counter_x += 1;
        }


        //Loop for calculation UNDER space pixels, between Q(0,s) and Q(s,s)
        counter_x = 1;
        index2 = index + scale_factor + 1 + new_width * scale_factor;
        end_x = index2 + scale_factor - 1;
        for (size_t x = index2; x < end_x; x++){
            res_img[x] = matrix_formula(scale_factor, counter_x, scale_factor, qs0, qs0, qss, qss);
            counter_x += 1;
        }

        
        //Finding corrrect index of pixel in result array
        size_t counter_y = index + new_width + scale_factor;
        //Loop for calculation LEFT space pixels, between Q(0,0) and Q(0,s)
        for (size_t y = 1; y < scale_factor; y++){
            res_img[counter_y] = matrix_formula(scale_factor, scale_factor, y, qs0, qs0, qss, qss);
            counter_y += new_width;
        }
    }



    //Below is the code for the last edge pixels. If the rightmost 4 pixels are the last ones from the down.
    //The space pixels will be counted with corner pixels where Q(0,0) = Q(0,s) and Q(s,0) = Q(s,s)
    if (y_index == (height - 2)){
        //Finding corrrect index of pixel in result array
        index2 = index + new_width * (scale_factor + 1) + 1;
        //Loop for calculation CENTER space pixels
        for (size_t y = 1; y < scale_factor; y++){
            for (size_t x = 1; x < scale_factor; x++){
                res_img[index2] = matrix_formula(scale_factor, x, y, q0s, q0s, qss, qss);
                index2 += 1;
            }
            index2 += step;
        }


        //Loop for calculation UPPER space pixels, between Q(0,0) and Q(s,0)
        size_t counter_x = 1;
        index2 = index + new_width * scale_factor + 1;
        end_x = index + new_width * scale_factor + scale_factor;
        for (size_t x = index2; x < end_x; x++){
            res_img[x] = matrix_formula(scale_factor, counter_x, scale_factor, q0s, q0s, qss, qss);
            counter_x += 1;
        }


        //Finding corrrect index of pixel in result array
        size_t counter_y = index + (scale_factor + 1) * new_width;
        //Loop for calculation LEFT space pixels, between Q(0,0) and Q(0,s)
        for (size_t y = 1; y < scale_factor; y++){
            //If has edgepixels
            if (x_index > 0){
                res_img[counter_y] = (res_img[counter_y] + matrix_formula(scale_factor, 0, y, q0s, q0s, qss, qss)) / 2;
            }else{
                res_img[counter_y] = matrix_formula(scale_factor, 0, y, q0s, q0s, qss, qss);
            }
            counter_y += new_width;
        }


        //Finding corrrect index of pixel in result array
        counter_y = index + (scale_factor + 1) * new_width + scale_factor;
        //Loop for calculation RIGHT space pixels, between Q(s,0) and Q(s,s)
        for (size_t y = 1; y < scale_factor; y++){
            res_img[counter_y] = matrix_formula(scale_factor, scale_factor, y, q0s, q0s, qss, qss);
            counter_y += new_width;
        }
    }


    //Below is the code for the last edge pixels. If the rightmost 4 pixels are the last ones from the down and from the right.
    //The space pixels will be counted with corner pixels where Q(0,0) = Q(s,s), Q(s,0) = Q(s,s), Q(0,s) = Q(s,s)
    if ((y_index == (height - 2)) && (x_index == (width - 2))){
        //Finding corrrect index of pixel in result array
        index2 = index + new_width * (scale_factor + 1) + 1 + scale_factor;
        //Loop for calculation CENTER space pixels
        for (size_t y = 1; y < scale_factor; y++){
            for (size_t x = 1; x < scale_factor; x++){
                res_img[index2] = matrix_formula(scale_factor, x, y, qss, qss, qss, qss);
                index2 += 1;
            }
            index2 += step;
        }
    }
}

/**
 * This function is for finding all possible values for spaces out of 4 pixel using 128bit registers
 * @param scale_factor Scaling factor
 * @param width Width of image
 * @param height Height of image
 * @param index Pixel index in the image array.
 * @param q00 Top left corner pixel
 * @param qs0 Top right corner pixel
 * @param q0s Bottom left corner pixel
 * @param qss Bottom right corner Pixel
 * @param res_img Array of image with saved result after calculation
 * @param y_index Pixel location of top right pixel along the y-axis in initial array.
 * @param x_index Pixel location of top right pixel along the x-axis in initial array.
 */
void interpolate_small_V1(__m128i nil, __m128i s, __m128i s_2, size_t scale_factor, size_t width, size_t height, size_t index, uint8_t q00, uint8_t qs0, uint8_t q0s, uint8_t qss, uint8_t *res_img,
                       size_t y_index, size_t x_index){
    //Тew image characteristics
    size_t new_width = width * scale_factor;

    //Step for moving to next correct pixel index
    size_t step = new_width - scale_factor + 1;

    //Index of first pixel index in CENTER of 4 Pixels
    size_t index2 = index + new_width + 1;
    //Loop for calculation center space pixels
    for (size_t y = 1; y < scale_factor; y++){
        int ys[4] = {(int)y, (int)y, (int)y, (int)y};
        __m128i yi = _mm_load_si128((__m128i *)ys);
        for (size_t x = 1; x < scale_factor; x += 1){
            if (x + 3 >= scale_factor) {
                res_img[index2] = matrix_formula(scale_factor, x, y, q00, qs0, q0s, qss);
                index2 += 1;
                continue;
            }

            int xs[4] = {(int)x, (int)(x + 1), (int)(x + 2), (int)(x + 3)};
            __m128i xi = _mm_load_si128((__m128i *)xs);
            __m128i r = matrix_formula_V1(nil, s, s_2, xi, yi, q00, qs0, q0s, qss);

            int n[4];
            _mm_store_si128((__m128i *)n, r);

            res_img[index2] = n[0];
            res_img[index2 + 1] = n[1];
            res_img[index2 + 2] = n[2];
            res_img[index2 + 3] = n[3];

            index2 += 4;
            x += 3;
        }
        index2 += step;
    }


    //Loop for calculation UPPER space pixels, between Q(0,0) and Q(s,0)
    size_t counter_x = 1;
    index2 = index + scale_factor;
    for (size_t x = index + 1; x < index2; x += 1){
        if (x + 3 >= index2) {
            //If has edgepixels
            if (y_index > 0){
                res_img[x] = (res_img[x] + matrix_formula(scale_factor, counter_x, 0, q00, qs0, q0s, qss)) / 2;
            }else{
                res_img[x] = matrix_formula(scale_factor, counter_x, 0, q00, qs0, q0s, qss);
            }
            counter_x += 1;
            continue;
        }
        int xs[4] = {(int)counter_x, (int)(counter_x + 1), (int)(counter_x + 2), (int)(counter_x + 3)};
        __m128i xi = _mm_load_si128((__m128i *)xs);
        __m128i r = matrix_formula_V1(nil, s, s_2, xi, nil, q00, qs0, q0s, qss);

        int n[4];
        _mm_store_si128((__m128i *)n, r);

        //If has edgepixels
        if (y_index > 0){
            res_img[x] = (res_img[x] + n[0]) / 2;
            res_img[x + 1] = (res_img[x + 1] + n[1]) / 2;
            res_img[x + 2] = (res_img[x + 2] + n[2]) / 2;
            res_img[x + 3] = (res_img[x + 3] + n[3]) / 2;
        }else{
            res_img[x] = n[0];
            res_img[x + 1] = n[1];
            res_img[x + 2] = n[2];
            res_img[x + 3] = n[3];
        }
        counter_x += 4;
        x += 3;
    }

    //Loop for calculation UNDER space pixels, between Q(0,s) and Q(s,s)
    counter_x = 1;
    index2 = index + 1 + new_width * scale_factor;
    size_t end_x = index2 + scale_factor - 1;
    for (size_t x = index2; x < end_x; x += 1){
        if (x + 3 >= end_x) {
            res_img[x] = matrix_formula(scale_factor, counter_x, scale_factor, q00, qs0, q0s, qss);
            counter_x += 1;
            continue;
        }
        int xs[4] = {(int)counter_x, (int)(counter_x + 1), (int)(counter_x + 2), (int)(counter_x + 3)};
        __m128i xi = _mm_load_si128((__m128i *)xs);
        __m128i r = matrix_formula_V1(nil, s, s_2, xi, s, q00, qs0, q0s, qss);

        int n[4];
        _mm_store_si128((__m128i *)n, r);

        res_img[x] = n[0];
        res_img[x + 1] = n[1];
        res_img[x + 2] = n[2];
        res_img[x + 3] = n[3];
        counter_x += 4;
        x += 3;
    }


    //Finding correct index of pixel in result array
    size_t counter_y = index + new_width + scale_factor;
    //Loop for calculation RIGHT space pixels, between Q(s,0) and Q(s,s)
    for (size_t y = 1; y < scale_factor; y += 1){
        if (y + 3 >= scale_factor) {
            res_img[counter_y] = matrix_formula(scale_factor, scale_factor, y, q00, qs0, q0s, qss);
            counter_y += new_width;
            continue;
        }
        int ys[4] = {(int)y, (int)(y + 1), (int)(y + 2), (int)(y + 3)};
        __m128i yi = _mm_load_si128((__m128i *)ys);
        __m128i r = matrix_formula_V1(nil, s, s_2, s, yi, q00, qs0, q0s, qss);

        int n[4];
        _mm_store_si128((__m128i *)n, r);

        res_img[counter_y] = n[0];
        res_img[counter_y + new_width] = n[1];
        res_img[counter_y + new_width * 2] = n[2];
        res_img[counter_y + new_width * 3] = n[3];
        counter_y += new_width * 4;
        y += 3;
    }


    //Finding correct index of pixel in result array
    counter_y = index + new_width;
    //Loop for calculation LEFT space pixels, between Q(0,0) and Q(0,s)
    for (size_t y = 1; y < scale_factor; y += 1){
        if (y + 3 >= scale_factor) {
            //If has edgepixels
            if (x_index > 0){
                res_img[counter_y] = (res_img[counter_y] + matrix_formula(scale_factor, 0, y, q00, qs0, q0s, qss)) / 2;
            }else{
                res_img[counter_y] = matrix_formula(scale_factor, 0, y, q00, qs0, q0s, qss);
            }
            counter_y += new_width;
            continue;
        }
        int ys[4] = {(int)y, (int)(y + 1), (int)(y + 2), (int)(y + 3)};
        __m128i yi = _mm_load_si128((__m128i *)ys);
        __m128i r = matrix_formula_V1(nil, s, s_2, nil, yi, q00, qs0, q0s, qss);

        int n[4];
        _mm_store_si128((__m128i *)n, r);

        //If has edgepixels
        if (y_index > 0){
            res_img[counter_y] = (res_img[counter_y] + n[0]) / 2;
            res_img[counter_y + 1] = (res_img[counter_y + 1] + n[1]) / 2;
            res_img[counter_y + 2] = (res_img[counter_y + 2] + n[2]) / 2;
            res_img[counter_y + 3] = (res_img[counter_y + 3] + n[3]) / 2;
        }else{
            res_img[counter_y] = n[0];
            res_img[counter_y + 1] = n[1];
            res_img[counter_y + 2] = n[2];
            res_img[counter_y + 3] = n[3];
        }
        counter_y += new_width * 4;
        y += 3;
    }

    //Below is the code for the last edge pixels. If the rightmost 4 pixels are the last ones from the right.
    //The space pixels will be counted with corner pixels where Q(0,0) = Q(s,s) and Q(0,s) = Q(s,s)
    if (x_index == (width - 2)){
        //Finding corrrect index of pixel in result array
        index2 = index + new_width + 1 + scale_factor;
        //Loop for calculation CENTER space pixels
        for (size_t y = 1; y < scale_factor; y++){
            int ys[4] = {(int)y, (int)y, (int)y, (int)y};
            __m128i yi = _mm_load_si128((__m128i *)ys);
            for (size_t x = 1; x < scale_factor; x += 1){
                if (x + 3 >= scale_factor) {
                    res_img[index2] = matrix_formula(scale_factor, x, y, qs0, qs0, qss, qss);
                    index2 += 1;
                    continue;
                }
                int xs[4] = {(int)x, (int)(x + 1), (int)(x + 2), (int)(x + 3)};
                __m128i xi = _mm_load_si128((__m128i *)xs);
                __m128i r = matrix_formula_V1(nil, s, s_2, xi, yi, qs0, qs0, qss, qss);

                int n[4];
                _mm_store_si128((__m128i *)n, r);

                res_img[index2] = n[0];
                res_img[index2 + 1] = n[1];
                res_img[index2 + 2] = n[2];
                res_img[index2 + 3] = n[3];

                index2 += 4;
                x += 3;
            }
            index2 += step;
        }


        //Loop for calculation UPPER space pixels, between Q(0,0) and Q(s,0)
        size_t counter_x = 1;
        index2 = index + scale_factor + 1;
        end_x = index + scale_factor + scale_factor;
        for (size_t x = index2; x < end_x; x += 1){
            if (x + 3 >= end_x) {
                //If has edgepixels
                if (y_index > 0){
                    res_img[x] = (res_img[x] + matrix_formula(scale_factor, counter_x, 0, qs0, qs0, qss, qss)) / 2;
                }else{
                    res_img[x] = matrix_formula(scale_factor, counter_x, 0, qs0, qs0, qss, qss);
                }
                counter_x += 1;
                continue;
            }
            int xs[4] = {(int)counter_x, (int)(counter_x + 1), (int)(counter_x + 2), (int)(counter_x + 3)};
            __m128i xi = _mm_load_si128((__m128i *)xs);
            __m128i r = matrix_formula_V1(nil, s, s_2, xi, nil, qs0, qs0, qss, qss);

            int n[4];
            _mm_store_si128((__m128i *)n, r);

            //If has edgepixels
            if (y_index > 0){
                res_img[x] = (res_img[x] + n[0]) / 2;
                res_img[x + 1] = (res_img[x + 1] + n[1]) / 2;
                res_img[x + 2] = (res_img[x + 2] + n[2]) / 2;
                res_img[x + 3] = (res_img[x + 3] + n[3]) / 2;
            }else{
                res_img[x] = n[0];
                res_img[x + 1] = n[1];
                res_img[x + 2] = n[2];
                res_img[x + 3] = n[3];
            }
            counter_x += 4;
            x += 3;
        }


        //Loop for calculation UNDER space pixels, between Q(0,s) and Q(s,s)
        counter_x = 1;
        index2 = index + scale_factor + 1 + new_width * scale_factor;
        end_x = index2 + scale_factor - 1;
        for (size_t x = index2; x < end_x; x += 1){
            //If has edgepixels
            if (x + 3 >= end_x) {
                res_img[x] = matrix_formula(scale_factor, counter_x, scale_factor, qs0, qs0, qss, qss);
                counter_x += 1;
                continue;
            }
            int xs[4] = {(int)counter_x, (int)(counter_x + 1), (int)(counter_x + 2), (int)(counter_x + 3)};
            __m128i xi = _mm_load_si128((__m128i *)xs);
            __m128i r = matrix_formula_V1(nil, s, s_2, xi, s, qs0, qs0, qss, qss);

            int n[4];
            _mm_store_si128((__m128i *)n, r);

            res_img[x] = n[0];
            res_img[x + 1] = n[1];
            res_img[x + 2] = n[2];
            res_img[x + 3] = n[3];
            counter_x += 4;
            x += 3;
        }

        
        //Finding corrrect index of pixel in result array
        size_t counter_y = index + new_width + scale_factor;
        //Loop for calculation LEFT space pixels, between Q(0,0) and Q(0,s)
        for (size_t y = 1; y < scale_factor; y += 1){
            if (y + 3 >= scale_factor) {
                res_img[counter_y] = matrix_formula(scale_factor, scale_factor, y, qs0, qs0, qss, qss);
                counter_y += new_width;
                continue;
            }
            int ys[4] = {(int)y, (int)(y + 1), (int)(y + 2), (int)(y + 3)};
            __m128i yi = _mm_load_si128((__m128i *)ys);
            __m128i r = matrix_formula_V1(nil, s, s_2, s, yi, qs0, qs0, qss, qss);

            int n[4];
            _mm_store_si128((__m128i *)n, r);

            res_img[counter_y] = n[0];
            res_img[counter_y + new_width] = n[1];
            res_img[counter_y + (new_width * 2)] = n[2];
            res_img[counter_y + (new_width * 3)] = n[3];
            counter_y += new_width * 4;
            y += 3;
        }
    }



    //Below is the code for the last edge pixels. If the rightmost 4 pixels are the last ones from the down.
    //The space pixels will be counted with corner pixels where Q(0,0) = Q(0,s) and Q(s,0) = Q(s,s)
    if (y_index == (height - 2)){
        //Finding corrrect index of pixel in result array
        index2 = index + new_width * (scale_factor + 1) + 1;
        //Loop for calculation CENTER space pixels
        for (size_t y = 1; y < scale_factor; y++){
            int ys[4] = {(int)y, (int)y, (int)y, (int)y};
            __m128i yi = _mm_load_si128((__m128i *)ys);
            for (size_t x = 1; x < scale_factor; x += 1){
                if (x + 3 >= scale_factor) {
                    res_img[index2] = matrix_formula(scale_factor, x, y, q0s, q0s, qss, qss);
                    index2 += 1;
                    continue;
                }
                int xs[4] = {(int)x, (int)(x + 1), (int)(x + 2), (int)(x + 3)};
                __m128i xi = _mm_load_si128((__m128i *)xs);
                __m128i r = matrix_formula_V1(nil, s, s_2, xi, yi, q0s, q0s, qss, qss);

                int n[4];
                _mm_store_si128((__m128i *)n, r);

                res_img[index2] = n[0];
                res_img[index2 + 1] = n[1];
                res_img[index2 + 2] = n[2];
                res_img[index2 + 3] = n[3];

                index2 += 4;
                x += 3;
            }
            index2 += step;
        }


        //Loop for calculation UPPER space pixels, between Q(0,0) and Q(s,0)
        size_t counter_x = 1;
        index2 = index + new_width * scale_factor + 1;
        end_x = index + new_width * scale_factor + scale_factor;
        for (size_t x = index2; x < end_x; x += 1){
            if (x + 3 >= end_x) {
                res_img[x] = matrix_formula(scale_factor, counter_x, scale_factor, q0s, q0s, qss, qss);
                counter_x += 1;
                continue;
            }
            int xs[4] = {(int)counter_x, (int)(counter_x + 1), (int)(counter_x + 2), (int)(counter_x + 3)};
            __m128i xi = _mm_load_si128((__m128i *)xs);
            __m128i r = matrix_formula_V1(nil, s, s_2, xi, s, q0s, q0s, qss, qss);

            int n[4];
            _mm_store_si128((__m128i *)n, r);

            res_img[x] = n[0];
            res_img[x + 1] = n[1];
            res_img[x + 2] = n[2];
            res_img[x + 3] = n[3];
            counter_x += 4;
            x += 3;
        }


        //Finding corrrect index of pixel in result array
        size_t counter_y = index + (scale_factor + 1) * new_width;
        //Loop for calculation LEFT space pixels, between Q(0,0) and Q(0,s)
        for (size_t y = 1; y < scale_factor; y += 1){
            if (y + 3 >= scale_factor) {
                //If has edgepixels
                if (x_index > 0){
                    res_img[counter_y] = (res_img[counter_y] + matrix_formula(scale_factor, 0, y, q0s, q0s, qss, qss)) / 2;
                }else{
                    res_img[counter_y] = matrix_formula(scale_factor, 0, y, q0s, q0s, qss, qss);
                }
                counter_y += new_width;
                continue;
            }
            int ys[4] = {(int)y, (int)(y + 1), (int)(y + 2), (int)(y + 3)};
            __m128i yi = _mm_load_si128((__m128i *)ys);
            __m128i r = matrix_formula_V1(nil, s, s_2, nil, yi, q0s, q0s, qss, qss);

            int n[4];
            _mm_store_si128((__m128i *)n, r);

            //If has edgepixels
            if (y_index > 0){
                res_img[counter_y] = (res_img[counter_y] + n[0]) / 2;
                res_img[counter_y + 1] = (res_img[counter_y + 1] + n[1]) / 2;
                res_img[counter_y + 2] = (res_img[counter_y + 2] + n[2]) / 2;
                res_img[counter_y + 3] = (res_img[counter_y + 3] + n[3]) / 2;
            }else{
                res_img[counter_y] = n[0];
                res_img[counter_y + 1] = n[1];
                res_img[counter_y + 2] = n[2];
                res_img[counter_y + 3] = n[3];
            }
            counter_y += new_width * 4;
            y += 3;
        }


        //Finding corrrect index of pixel in result array
        counter_y = index + (scale_factor + 1) * new_width + scale_factor;
        //Loop for calculation RIGHT space pixels, between Q(s,0) and Q(s,s)
        for (size_t y = 1; y < scale_factor; y += 1){
            if (y + 3 >= scale_factor) {
                res_img[counter_y] = matrix_formula(scale_factor, scale_factor, y, q0s, q0s, qss, qss);
                counter_y += new_width;
                continue;
            }
            int ys[4] = {(int)y, (int)(y + 1), (int)(y + 2), (int)(y + 3)};
            __m128i yi = _mm_load_si128((__m128i *)ys);
            __m128i r = matrix_formula_V1(nil, s, s_2, s, yi, q0s, q0s, qss, qss);

            int n[4];
            _mm_store_si128((__m128i *)n, r);

            res_img[counter_y] = n[0];
            res_img[counter_y + new_width] = n[1];
            res_img[counter_y + (new_width * 2)] = n[2];
            res_img[counter_y + (new_width * 3)] = n[3];
            counter_y += new_width * 4;
            y += 3;
        }
    }


    //Below is the code for the last edge pixels. If the rightmost 4 pixels are the last ones from the down and from the right.
    //The space pixels will be counted with corner pixels where Q(0,0) = Q(s,s), Q(s,0) = Q(s,s), Q(0,s) = Q(s,s)
    if ((y_index == (height - 2)) && (x_index == (width - 2))){
        //Finding corrrect index of pixel in result array
        index2 = index + new_width * (scale_factor + 1) + 1 + scale_factor;
        //Loop for calculation CENTER space pixels
        for (size_t y = 1; y < scale_factor; y++){
            int ys[4] = {(int)y, (int)y, (int)y, (int)y};
            __m128i yi = _mm_load_si128((__m128i *)ys);
            for (size_t x = 1; x < scale_factor; x += 1){
                if (x + 3 >= scale_factor) {
                    res_img[index2] = matrix_formula(scale_factor, x, y, qss, qss, qss, qss);
                    index2 += 1;
                    continue;
                }
                int xs[4] = {(int)x, (int)(x + 1), (int)(x + 2), (int)(x + 3)};
                __m128i xi = _mm_load_si128((__m128i *)xs);
                __m128i r = matrix_formula_V1(nil, s, s_2, xi, yi, qss, qss, qss, qss);

                int n[4];
                _mm_store_si128((__m128i *)n, r);

                res_img[index2] = n[0];
                res_img[index2 + 1] = n[1];
                res_img[index2 + 2] = n[2];
                res_img[index2 + 3] = n[3];

                index2 += 4;
                x += 3;
            }
            index2 += step;
        }
    }
}

void interpolate(const uint8_t *img, size_t width, size_t height, float a, float b, float c, size_t scale_factor, uint8_t *tmp, uint8_t *result){

    //First turn the image to grayscale, the result is saved in tmp array
    grayscale(img, tmp, width, height, a, b, c);

    //Тew image characteristics
    size_t new_width = width * scale_factor;
    size_t new_height = height * scale_factor;

    //The existing pixels are moved to their new positions. The resulting gaps are marked by black pixels:
    size_t counter = 0;
    size_t ss = new_height * new_width;
    size_t step = scale_factor * new_width;
    for (size_t i = 0; i < ss; i += step){
        for (size_t j = 0; j < width; ++j){
            result[i + scale_factor * j] = tmp[counter];
            counter += 1;
        }
    }

    //Index of top right corner pixel
    size_t index = 0;
    //Loop to go throw every possible corner pixels
    for (size_t i = 0; i < height - 1; i++){
        for (size_t j = 0; j < width - 1; j++){
            size_t init = i * width;
            uint8_t q00 = tmp[init + j];
            uint8_t qs0 = tmp[init + j + 1];
            uint8_t q0s = tmp[init + j + width];
            uint8_t qss = tmp[init + j + width + 1];

            interpolate_small(scale_factor, width, height, index, q00, qs0, q0s, qss, result, i, j);

            index += scale_factor;
        }
        index += new_width * (scale_factor - 1) + scale_factor;
    }
}

void interpolate_V1(const uint8_t *img,
                    size_t width,
                    size_t height,
                    float a,
                    float b,
                    float c,
                    size_t scale_factor,
                    uint8_t *tmp,
                    uint8_t *result){
    //First turn the image to grayscale, the result is saved in tmp array
    grayscale(img, tmp, width, height, a, b, c);

    //Тew image characteristics
    size_t new_width = width * scale_factor;
    size_t new_height = height * scale_factor;

    //The existing pixels are moved to their new positions. The resulting gaps are marked by black pixels:
    size_t counter = 0;
    size_t ss = new_height * new_width;
    size_t step = scale_factor * new_width;
    for (size_t i = 0; i < ss; i += step){
        for (size_t j = 0; j < width; ++j){
            result[i + scale_factor * j] = tmp[counter];
            counter += 1;
        }
    }

    // Init 128bit registers
    int scalef2 = (int)(scale_factor * scale_factor);
    int nil[4] = {0, 0, 0, 0};
    int sf[4] = {(int)scale_factor, (int)scale_factor, (int)scale_factor, (int)scale_factor};
    int sf2[4] = {scalef2, scalef2, scalef2, scalef2};
    __m128i null = _mm_load_si128((__m128i *)nil);
    __m128i s = _mm_load_si128((__m128i *)sf);
    __m128i s_2 = _mm_load_si128((__m128i *)sf2);

    //Index of top right corner pixel
    size_t index = 0;
    //Loop to go throw every possible corner pixels
    for (size_t i = 0; i < height - 1; i++){
        size_t init = i * width;
        for (size_t j = 0; j < width - 1; j++){
            uint8_t q00 = tmp[init + j];
            uint8_t qs0 = tmp[init + j + 1];
            uint8_t q0s = tmp[init + j + width];
            uint8_t qss = tmp[init + j + width + 1];

            interpolate_small_V1(null, s, s_2, scale_factor, width, height, index, q00, qs0, q0s, qss, result, i, j);

            index += scale_factor;
        }
        index += new_width * (scale_factor - 1) + scale_factor;
    }
}



int main(){

    FILE *input_file = fopen("./input_data/smile3.ppm", "rb");

    if (!input_file)
    {
        perror("Error by opening");
        return 1;
    }

    FILE *output_file = fopen("output_img.pgm", "wb");
    if (!output_file)
    {
        perror("Error by creating");
        fclose(input_file);
        return 1;
    }

    uint8_t format[3];
    size_t width, height, max_color;
    size_t s = 10;

    fscanf(input_file, "%2s", format);
    fscanf(input_file, "%zd %zd %zd", &width, &height, &max_color);

    fgetc(input_file);

    fprintf(output_file, "P2\n%lu %lu\n%d\n", s * width, s * height, 255);

    unsigned char *arr_of_img = (uint8_t *)malloc(width * height * 3);
    uint8_t *res_img_gray = (uint8_t *)malloc(width * height);
    uint8_t *res_img = (uint8_t *)malloc((s * width) * (s * height));

    fread(arr_of_img, sizeof(uint8_t), width * height * 3, input_file);

    const uint8_t *img = (const uint8_t *)arr_of_img;

    interpolate(arr_of_img, width, height, 0, 0, 0, s, res_img_gray, res_img);

    for (size_t i = 0; i < (s * width) * (s * height); ++i)
    {
        fprintf(output_file, "%d ", res_img[i]);
    }

    fclose(input_file);
    fclose(output_file);

    free(arr_of_img);
    free(res_img_gray);
    free(res_img);

    return 0;
}