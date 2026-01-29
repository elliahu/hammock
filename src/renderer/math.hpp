#pragma once
#include "HandmadeMath/HandmadeMath.h"


namespace hammock::math {

    using Vec2 = ::HMM_Vec2;
    using Vec3 = ::HMM_Vec3;
    using Vec4 = ::HMM_Vec4;
    using Mat2 = ::HMM_Mat2;
    using Mat3 = ::HMM_Mat3;
    using Mat4 = ::HMM_Mat4;



    Mat4 perspective(float FOV, float AspectRatio, float Near, float Far);
    Mat4 orthographic(float left, float right, float bottom, float top, float near, float far);
    Mat4 lookAt(Vec3 eye, Vec3 target, Vec3 up);
    Mat4 invLookAt(Mat4 lookAt);

}
