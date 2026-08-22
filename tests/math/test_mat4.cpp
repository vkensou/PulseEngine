#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "pulse_math.h"

int main() {
    // Identity matrix: 1 on the diagonal, 0 elsewhere.
    HMM_Mat4 m4 = HMM_M4D(1.0f);
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            if (c == r) {
                assert(m4.Elements[c][r] == 1.0f);
            } else {
                assert(m4.Elements[c][r] == 0.0f);
            }
        }
    }

    // Translation matrix: column-major, translation lives in column 3.
    HMM_Mat4 m4_trans = HMM_Translate(HMM_V3(1.0f, 2.0f, 3.0f));
    assert(m4_trans.Elements[3][0] == 1.0f);
    assert(m4_trans.Elements[3][1] == 2.0f);
    assert(m4_trans.Elements[3][2] == 3.0f);
    assert(m4_trans.Elements[3][3] == 1.0f);
    assert(m4_trans.Elements[0][0] == 1.0f);
    assert(m4_trans.Elements[1][1] == 1.0f);
    assert(m4_trans.Elements[2][2] == 1.0f);

    // Perspective (RH, no): the W row must get -Z for RH_NO convention.
    HMM_Mat4 m4_proj = HMM_Perspective_RH_NO(90.0f, 16.0f / 9.0f, 0.1f, 100.0f);
    assert(m4_proj.Elements[2][3] == -1.0f);
    assert(m4_proj.Elements[3][3] == 0.0f);
    assert(m4_proj.Elements[2][2] != 0.0f);
    assert(m4_proj.Elements[0][0] != 0.0f);
    assert(m4_proj.Elements[1][1] != 0.0f);
    return 0;
}
