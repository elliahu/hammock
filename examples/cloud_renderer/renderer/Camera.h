#pragma once
#include <hammock/hammock.h>

using namespace hammock;

/**
 * Simple perspective camera class for view and projection matrix computation
 */
class Camera final {
public:
    Camera(HmckVec3 position,float aspect, float32_t fov, float32_t znear = 0.01f, float32_t zfar = 100000.f, float yaw = 0.f, float pitch = 0.f, float roll = 0.f): position(position), fov(fov), znear(znear), yaw(yaw), pitch(pitch), roll(roll),
        zfar(zfar), aspect(aspect) {
    }

    HmckVec3 position;
    float32_t fov, znear, zfar, aspect;
    float32_t yaw{0.f}, pitch{0.f}, roll{0.f};

    struct FrustumDirections
    {
        HmckVec3 frustumA; // top-left dir
        HmckVec3 frustumB; // top-right dir
        HmckVec3 frustumC; // bottom-left dir
        HmckVec3 frustumD; // bottom-right dir
    };


    HmckMat4 getView() {
        // First, compute the camera's orientation vectors
        // Start with default vectors
        HmckVec3 worldUp = {0.0f, -1.0f, 0.0f};

        // Calculate forward direction based on yaw and pitch (ignoring roll for now)
        // This uses spherical coordinates to calculate the direction
        forward.X = cosf(yaw) * cosf(pitch);
        forward.Y = sinf(pitch);
        forward.Z = sinf(yaw) * cosf(pitch);
        forward = HmckNorm(forward);

        // Calculate right vector as cross product of forward and world up
        HmckVec3 right = HmckNorm(HmckCross(worldUp, forward));

        // Calculate camera up vector as cross product of right and forward
        up = HmckCross(forward, right);

        // Apply roll (rotation around forward axis) if needed
        if (roll != 0.0f) {
            // Create rotation matrix around forward axis
            HmckMat4 rollRotation = HmckRotate_RH(roll, forward);

            // Apply roll to up vector
            HmckVec4 upVec4 = {up.X, up.Y, up.Z, 0.0f};
            HmckVec4 rotatedUpVec4 = HmckMulM4V4(rollRotation, upVec4);
            up = {rotatedUpVec4.X, rotatedUpVec4.Y, rotatedUpVec4.Z};

            // Recalculate right vector to maintain orthogonality
            right = HmckCross(up, forward);
        }

        // Calculate target by adding forward to position
        target = HmckAdd(position, forward);

        // Create view matrix using LookAt function
        return HmckLookAt_RH(position, target, up);
    }

    HmckMat4 getProjection() const {
        //return Projection().orthographic(-50.0, 50.0, -50.0, 50.0, znear, zfar, false);

        HmckMat4 proj = HmckPerspective_RH_ZO(fov, aspect, znear, zfar);
        proj[1][1] *= -1;
        return proj;
    }

    HmckVec3 forwardDirection() const {return HmckNorm(forward);}
    HmckVec3 upDirection() const {return HmckNorm(up);}
    HmckVec3 rightDirection() const {return HmckCross(upDirection(), forwardDirection());}

    FrustumDirections getFrustumDirections() {
        HmckMat4 invViewProj = HmckInvGeneral(getView());

        HmckVec4 A0 = HmckMulM4V4(invViewProj, HmckVec4{-1, 1, 0.2f, 1});
        HmckVec4 A1 = HmckMulM4V4(invViewProj, HmckVec4{-1, 1, 0.5f, 1});

        HmckVec4 B0 = HmckMulM4V4(invViewProj, HmckVec4{1, 1, 0.2f, 1});
        HmckVec4 B1 = HmckMulM4V4(invViewProj, HmckVec4{1, 1, 0.5f, 1});

        HmckVec4 C0 = HmckMulM4V4(invViewProj, HmckVec4{-1, -1, 0.2f, 1});
        HmckVec4 C1 = HmckMulM4V4(invViewProj, HmckVec4{-1, -1, 0.5f, 1});

        HmckVec4 D0 = HmckMulM4V4(invViewProj, HmckVec4{1, -1, 0.2f, 1});
        HmckVec4 D1 = HmckMulM4V4(invViewProj, HmckVec4{1, -1, 0.5f, 1});

        FrustumDirections result;
        result.frustumA = HmckNorm(A1.XYZ / A1.W - A0.XYZ / A0.W);
        result.frustumB = HmckNorm(B1.XYZ / B1.W - B0.XYZ / B0.W);
        result.frustumC = HmckNorm(C1.XYZ / C1.W - C0.XYZ / C0.W);
        result.frustumD = HmckNorm(D1.XYZ / D1.W - D0.XYZ / D0.W);
        return result;
    }

private:
    HmckVec3 forward;
    HmckVec3 up;
    HmckVec3 target;
};
