module;

#include <cassert>
#include <cmath>
#include <limits>

export module hammock.renderer.camera;

import hammock.renderer.math;




namespace hammock::renderer {
    export class Projection {
    public:
        Vec3 upNegY() { return Vec3{0.f, -1.f, 0.f}; }
        Vec3 upPosY() { return Vec3{0.f, 1.f, 0.f}; }

        Mat4 perspective(float fovy, float aspect, float zNear, float zFar, bool flip = true) {
            assert(std::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
            Mat4 projection = perspective(fovy, aspect, zNear, zFar);
            if (flip) {
                projection[1][1] *= -1;
            }
            return projection;
        }

        Mat4 orthographic(float left, float right, float bottom, float top, float znear, float zfar, bool flip = true) {
            Mat4 projection = ortographic(left, right, bottom, top, znear, zfar);
            if (flip) {
                projection[1][1] *= -1;
            }
            return projection;
        }

        Mat4 view(Vec3 eye, Vec3 target, Vec3 up) {
            return lookAt(eye, target, up);
        }

        Mat4 inverseView(Vec3 eye, Vec3 target, Vec3 up) {
            return invLookAt(lookAt(eye, target, up));
        }
    };
}
