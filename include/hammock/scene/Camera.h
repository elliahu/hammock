#pragma once

#include "hammock/core/HandmadeMath.h"

namespace Hammock {
    class Projection {
    public:
        HmckVec3 upNegY() { return HmckVec3{0.f, -1.f, 0.f}; }
        HmckVec3 upPosY() { return HmckVec3{0.f, 1.f, 0.f}; }

        HmckMat4 perspective(float fovy, float aspect, float zNear, float zFar, bool flip = true);

        HmckMat4 view(HmckVec3 eye, HmckVec3 target, HmckVec3 up);

        HmckMat4 inverseView(HmckVec3 eye, HmckVec3 target, HmckVec3 up);
    };
}
