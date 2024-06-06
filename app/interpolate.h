#include <stddef.h>
#include <stdint.h>

/**
 * This function takes a pointer to an array of pixels from the input image
 * along with some other meta data. It applies grayscale conversion and finally
 * a blur to the "image" and saves it in the result pointer. After all the
 * result pointer has the new interpolated image.
 * @param img Pointer to the input image
 * @param width Width
 * @param height Height
 * @param a First coefficient for the grayscale conversion (floating point)
 * @param b Second coefficient for the grayscale conversion (floating point)
 * @param c Third coefficient for the grayscale conversion (floating point)
 * @param scale_factor Scaling factor
 * @param tmp Provisional results
 * @param result Result of the conversion
 */
void interpolate(const uint8_t *img, size_t width, size_t height, float a,
                 float b, float c, size_t scale_factor, uint8_t *tmp,
                 uint8_t *result);

/**
 * This function takes a pointer to an array of pixels from the input image
 * along with some other meta data. It applies grayscale conversion and finally
 * a blur to the "image" and saves it in the result pointer. After all the
 * result pointer has the new interpolated image.
 * @note This is an alternative approach to the naive version
 * @param img Pointer to the input image
 * @param width Width
 * @param height Height
 * @param a First coefficient for the grayscale conversion (floating point)
 * @param b Second coefficient for the grayscale conversion (floating point)
 * @param c Third coefficient for the grayscale conversion (floating point)
 * @param scale_factor Scaling factor
 * @param tmp Provisional results
 * @param result Result of the conversion
 */
void interpolate_V1(const uint8_t *img, size_t width, size_t height, float a,
                    float b, float c, size_t scale_factor, uint8_t *tmp,
                    uint8_t *result);
