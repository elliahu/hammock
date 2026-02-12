#include "math.hpp"

#include "HandmadeMath/HandmadeMath.h"

namespace hammock::math {

    Mat4 perspective(float FOV, float AspectRatio, float Near, float Far) {
        return ::HMM_Perspective_RH_ZO(FOV, AspectRatio, Near, Far);
    }

    Mat4 orthographic(float left, float right, float bottom, float top, float near, float far) {
        return ::HMM_Orthographic_RH_ZO(left, right, bottom, top, near, far);
    }

    Mat4 lookAt(Vec3 eye, Vec3 target, Vec3 up) { return ::HMM_LookAt_RH(eye, target, up); }

    Mat4 invLookAt(Mat4 lookAt) { return ::HMM_InvLookAt(lookAt); }
    Mat4 translate(Vec3 position) { return HMM_Translate(position); }
    Mat4 scale(Vec3 scale) { return HMM_Scale(scale); }
    Mat4 rotate(Quat quat) { return HMM_QToM4(quat); }
}  // namespace hammock::math