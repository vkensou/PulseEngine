#include <stdio.h>
#include "pulse_math.h"

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    // Test HMM_Vec3
    HMM_Vec3 v3 = HMM_V3(1.0f, 2.0f, 3.0f);
    HMM_Vec3 v3b = HMM_V3(4.0f, 5.0f, 6.0f);
    HMM_Vec3 v3_sum = HMM_AddV3(v3, v3b);
    HMM_Vec3 v3_cross = HMM_Cross(v3, v3b);
    float v3_dot = HMM_DotV3(v3, v3b);
    float v3_len = HMM_LenV3(v3);
    HMM_Vec3 v3_norm = HMM_NormV3(v3);

    printf("HMM_Vec3: (%.1f, %.1f, %.1f)\n", v3.X, v3.Y, v3.Z);
    printf("  sum: (%.1f, %.1f, %.1f)\n", v3_sum.X, v3_sum.Y, v3_sum.Z);
    printf("  cross: (%.1f, %.1f, %.1f)\n", v3_cross.X, v3_cross.Y, v3_cross.Z);
    printf("  dot: %.1f\n", v3_dot);
    printf("  len: %.1f\n", v3_len);
    printf("  norm: (%.4f, %.4f, %.4f)\n", v3_norm.X, v3_norm.Y, v3_norm.Z);

    // Test HMM_Quat
    HMM_Quat q = HMM_Q(1.0f, 0.0f, 0.0f, 0.0f);
    HMM_Quat q_norm = HMM_NormQ(q);
    printf("HMM_Quat: (%.1f, %.1f, %.1f, %.1f)\n", q.X, q.Y, q.Z, q.W);
    printf("  norm: (%.1f, %.1f, %.1f, %.1f)\n", q_norm.X, q_norm.Y, q_norm.Z, q_norm.W);

    // Test HMM_Mat4
    HMM_Mat4 m4 = HMM_M4D(1.0f);
    HMM_Mat4 m4_trans = HMM_Translate(HMM_V3(1.0f, 2.0f, 3.0f));
    HMM_Mat4 m4_proj = HMM_Perspective_RH_NO(90.0f, 16.0f / 9.0f, 0.1f, 100.0f);
    (void)m4_trans;
    (void)m4_proj;

    printf("HMM_Mat4 identity diagonal: (%.1f, %.1f, %.1f, %.1f)\n",
           m4.Elements[0][0], m4.Elements[1][1],
           m4.Elements[2][2], m4.Elements[3][3]);

    // Verify v3_sum is (5, 7, 9)
    bool pass = (v3_sum.X == 5.0f && v3_sum.Y == 7.0f && v3_sum.Z == 9.0f);
    printf("\n=== %s ===\n", pass ? "PASS" : "FAIL");

    return pass ? 0 : 1;
}
