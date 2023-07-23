#ifndef UTILITY_COLORS_H_
#define UTILITY_COLORS_H_
#include <stdint.h>

typedef struct rgb
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} rgb_t;

typedef struct hsl
{
    uint8_t hue;
    uint8_t saturation;
    uint8_t lightness;
} hsl_t;

#endif