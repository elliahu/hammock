module;

#include "HandmadeMath.h"

export module hammock.renderer.math;

export namespace hammock::renderer::math {

    using Vec2 = ::HMM_Vec2;
    using Vec3 = ::HMM_Vec3;
    using Vec4 = ::HMM_Vec4;
    using Mat2 = ::HMM_Mat2;
    using Mat3 = ::HMM_Mat3;
    using Mat4 = ::HMM_Mat4;



    inline Mat4 perspective(float FOV, float AspectRatio, float Near, float Far) {
        return ::HMM_Perspective_RH_ZO(FOV, AspectRatio, Near, Far);
    }

    inline Mat4 orthographic(float left, float right, float bottom, float top, float near, float far) {
        return ::HMM_Orthographic_RH_ZO(left, right, bottom, top, near, far);
    }

    inline Mat4 lookAt(Vec3 eye, Vec3 target, Vec3 up) {
        return ::HMM_LookAt_RH(eye, target, up);
    }

    inline Mat4 invLookAt(Mat4 lookAt) {
        return ::HMM_InvLookAt(lookAt);
    }


}
