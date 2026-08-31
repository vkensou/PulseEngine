#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "pulse_math.h"

// HandmadeMath has no HMM_LenQ; compute the quaternion length directly.
static float quat_len(HMM_Quat q) {
    return sqrtf(q.X * q.X + q.Y * q.Y + q.Z * q.Z + q.W * q.W);
}

int main() {
    // Unit quaternion: normalizing must be the identity.
    HMM_Quat q = HMM_Q(1.0f, 0.0f, 0.0f, 0.0f);
    HMM_Quat q_norm = HMM_NormQ(q);

    assert(fabsf(q_norm.X - 1.0f) < 1e-5f);
    assert(fabsf(q_norm.Y) < 1e-5f);
    assert(fabsf(q_norm.Z) < 1e-5f);
    assert(fabsf(q_norm.W) < 1e-5f);

    // Non-unit quaternion: normalize then length must be 1,
    // and the direction (component ratios) must be kept.
    HMM_Quat q2 = HMM_Q(1.0f, 2.0f, 3.0f, 4.0f);
    float q2_len = quat_len(q2);
    assert(fabsf(q2_len - sqrtf(30.0f)) < 1e-4f);
    HMM_Quat q2_norm = HMM_NormQ(q2);
    assert(fabsf(quat_len(q2_norm) - 1.0f) < 1e-4f);
    assert(fabsf(q2_norm.X / q2.X - 1.0f / q2_len) < 1e-4f);

    // Axis-angle rotation, 90 degrees around Z.
    HMM_Quat rot = HMM_QFromAxisAngle_RH(HMM_V3(0, 0, 1), HMM_AngleDeg(90));
    assert(fabsf(rot.Z - 0.70710678f) < 1e-4f);
    assert(fabsf(rot.W - 0.70710678f) < 1e-4f);
    return 0;
}
