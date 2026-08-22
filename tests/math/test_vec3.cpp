#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "pulse_math.h"

int main() {
    HMM_Vec3 v3 = HMM_V3(1.0f, 2.0f, 3.0f);
    HMM_Vec3 v3b = HMM_V3(4.0f, 5.0f, 6.0f);
    HMM_Vec3 v3_sum = HMM_AddV3(v3, v3b);
    HMM_Vec3 v3_cross = HMM_Cross(v3, v3b);
    float v3_dot = HMM_DotV3(v3, v3b);
    float v3_len = HMM_LenV3(v3);
    HMM_Vec3 v3_norm = HMM_NormV3(v3);

    // sum(1,2,3) + (4,5,6) == (5,7,9)
    assert(v3_sum.X == 5.0f && v3_sum.Y == 7.0f && v3_sum.Z == 9.0f);
    // cross == (2*6-3*5, 3*4-1*6, 1*5-2*4) == (-3,6,-3)
    assert(v3_cross.X == -3.0f && v3_cross.Y == 6.0f && v3_cross.Z == -3.0f);
    // dot == 1*4+2*5+3*6 == 32
    assert(v3_dot == 32.0f);
    // len == sqrt(14)
    assert(fabsf(v3_len - sqrtf(14.0f)) < 1e-4f);
    // norm == (1,2,3)/sqrt(14)
    assert(fabsf(v3_norm.X - 1.0f / sqrtf(14.0f)) < 1e-4f);
    assert(fabsf(v3_norm.Y - 2.0f / sqrtf(14.0f)) < 1e-4f);
    assert(fabsf(v3_norm.Z - 3.0f / sqrtf(14.0f)) < 1e-4f);
    return 0;
}
