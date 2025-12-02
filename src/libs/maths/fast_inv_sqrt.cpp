#include "fast_inv.sqrt.h"

float fast_inv_sqrt(float x)
{
    float xhalf = 0.5f * x;
    i32 i = *(i32*)&x;
    i = 0x5f3759df - (i >> 1);
    x = *(f32*)&i;
    x = x * (1.5f - xhalf * x * x);
    return x;
}
