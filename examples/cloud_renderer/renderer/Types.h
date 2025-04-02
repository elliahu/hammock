#pragma once
#include <hammock/hammock.h>

#define CWD(path) "../../../examples/cloud_renderer/" path
#define ASSET_PATH(asset) CWD("assets/" asset)
#define COMPILED_SHADER_PATH(shader) CWD("spv/" shader ".spv")
#define GROUPS_COUNT(res,size) ((res+size-1)/size)

struct GeometryPushConstantData {
    HmckMat4 modelViewProjection;
    HmckVec4 lightDirection;
    HmckVec4 lightColor;
    HmckVec4 ambientLightColor = {0.05f, 0.05f,0.05f,0.0f};
};

// Data for cloud pass passed as uniform buffer
struct CloudsUniformBufferData {
    HmckMat4 invView;
    HmckMat4 invProj;
    HmckMat4 view;
    HmckMat4 proj;
    HmckVec4 cameraPosition;
    HmckVec4 lightColor{1.0f, 1.0f, 1.0f, 5.0f}; // W is strength
    HmckVec4 lightDirection{0.0f, 1.0f, 0.f, 0.0f};
    HmckVec4 skyColorZenith{59.0 / 255.0, 110.0 / 255.0, 219.0 / 255.0};
    HmckVec4 skyColorHorizon{169.0 / 255.0, 175.0 / 255.0, 188.0 / 255.0};
    HmckVec4 windDirection;
    float resX;
    float resY;
    float fov;
    float znear;
    float zfar;
    float time = 0.0f;
    float timeOfDay = 0.45f;
    int frameIndexMod16;
};

// Data for cloud pass passed as push constant block
struct CloudsPushConstantData {
    float anvilBias = 0.0f;
    float globalDensity = 0.3f;
    float globalCoverage = 0.0;
    float baseMultiplier = 0.8f;
    float detailMultiplier = 0.75;
    float cloudSpeed = 0.f;
    float baseScale = 50.f;
    float detailScale = 50.0f;
    float curliness = 2.0f;
    float absorption = 0.0042f;
    float eccentricity = 0.22f;
    float intensity = .95f;
    float spread = 1.0f;
    float ambientStrength = 0.05;
    int DEBUG_epicView = 0;
    int DEBUG_cheapSampleDistance = 100000;
    int DEBUG_maxSamples = 96;
    int DEBUG_maxLightSamples = 4;
    int DEBUG_earlyTermination = 0;
    int DEBUG_lateTermination = 0;
    int DEBUG_longStepMulti = 2;
};

// Atmospheric pass data
struct AtmosphereUniformBufferData {
    HmckVec4 ScatterRayleigh{5.802f, 13.558f, 33.1f, 0.f};;
    HmckVec4 AbsorbOzone{0.65f, 1.881f, 0.085f, 0.f};
    HmckVec4 eye;
    HmckVec4 sunDirection;
    float HDensityRayleigh = 8.f;
    float ScatterMie = 3.996f;
    float AsymmetryMie = 0.8f;
    float AbsorbMie = 4.4f;
    float HDensityMie = 1.2f;
    float OzoneCenterHeight = 25.f;
    float OzoneThickness = 30;
    float PlanetRadius = 6378;
    float AtmosphereRadius = 6460;

    [[nodiscard]] AtmosphereUniformBufferData toStdUnit() const {
        AtmosphereUniformBufferData ret = *this;
        ret.ScatterRayleigh = 1e-6f * ret.ScatterRayleigh;
        ret.HDensityRayleigh = 1e3f * ret.HDensityRayleigh;
        ret.ScatterMie = 1e-6f * ret.ScatterMie;
        ret.AbsorbMie = 1e-6f * ret.AbsorbMie;
        ret.HDensityMie = 1e3f * ret.HDensityMie;
        ret.AbsorbOzone = 1e-6f * ret.AbsorbOzone;
        ret.OzoneCenterHeight = 1e3f * ret.OzoneCenterHeight;
        ret.OzoneThickness = 1e3f * ret.OzoneThickness;
        ret.PlanetRadius = 1e3f * ret.PlanetRadius;
        ret.AtmosphereRadius = 1e3f * ret.AtmosphereRadius;
        return ret;
    }

    HmckVec3 getSigmaS(float h) const {
        HmckVec3 rayleigh = ScatterRayleigh.XYZ * std::exp(-h / HDensityRayleigh);
        HmckVec3 mie = {ScatterMie * std::exp(-h / HDensityMie)};
        return rayleigh + mie;
    }

    HmckVec3 getSigmaT(float h) const {
        HmckVec3 rayleigh = ScatterRayleigh.XYZ * std::exp(-h / HDensityRayleigh);
        float mie = (ScatterMie + AbsorbMie) * std::exp(-h / HDensityMie);
        float ozoneDensity = (std::max)(
            0.0f, 1 - 0.5f * std::abs(h - OzoneCenterHeight) / OzoneThickness);
        HmckVec3 ozone = AbsorbOzone.XYZ * ozoneDensity;

        return HmckAdd(HmckAdd(rayleigh, HmckVec3{mie,mie,mie}), ozone);
    }

    HmckVec3 evalPhaseFunction(float h, float u) const {
        HmckVec3 sRayleigh = ScatterRayleigh.XYZ * std::exp(-h / HDensityRayleigh);
        HmckVec3 sMie = {ScatterMie * std::exp(-h / HDensityMie)};
        HmckVec3 s = sRayleigh + sMie;

        float g = AsymmetryMie, g2 = g * g, u2 = u * u;
        float pRayleigh = 3 / (16 * HmckPI) * (1 + u2);

        float m = 1 + g2 - 2 * g * u;
        float pMie = 3 / (8 * HmckPI) * (1 - g2) * (1 + u2)
                     / ((2 + g2) * m * std::sqrt(m));

        HmckVec3 result;
        for (int i = 0; i < 3; ++i) {
            if (s[i] > 0)
                result[i] = (pRayleigh * sRayleigh[i] + pMie * sMie[i]) / s[i];
        }

        return result;
    }
};

// Data for post process pass passed as push constant block
struct PostProcessingPushConstantData {
    HmckVec4 colorTint{1.0f, 1.0f, 1.0f, 0.0f};
    float contrast = 1.0; // Default: 1.0, Range: 0.5 to 2.0
    float brightness = 0.0; // Default: 0.0, Range: -1.0 to 1.0
    float vignetteStrength = 2.f; // Default 2.0, Range: 0.0 to 3.0
    float vignetteSoftness = 1.0f; // Default: 0.5, Range: 0.0 to 2.0
    float grainAmount = 0.035f; // Default: 0.0, Range: 0.0 to 0.1
    float time = 0.0f;
};