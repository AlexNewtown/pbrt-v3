
/*
    pbrt source code is Copyright(c) 1998-2016
                        Matt Pharr, Greg Humphreys, and Wenzel Jakob.

    This file is part of pbrt.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

    - Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    - Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
    IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
    TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
    PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */


// core/light.cpp*
#include "light.h"
#include "scene.h"
#include "sampling.h"
#include "stats.h"
#include "paramset.h"

namespace pbrt {

STAT_COUNTER("Scene/Lights", numLights);
STAT_COUNTER("Scene/AreaLights", numAreaLights);

// Light Method Definitions
Light::Light(int flags, const Transform &LightToWorld,
             const MediumInterface &mediumInterface, int nSamples)
    : flags(flags),
      nSamples(std::max(1, nSamples)),
      mediumInterface(mediumInterface),
      LightToWorld(LightToWorld),
      WorldToLight(Inverse(LightToWorld)) 
{
    ++numLights;
}

Light::~Light() {}

bool VisibilityTester::Unoccluded(const Scene &scene) const 
{
    // 如果从 p0 到 p1 的光线未与场景物体相交, 则这两个点是可见的
    return !scene.IntersectP(p0.SpawnRayTo(p1));
}

Spectrum VisibilityTester::Tr(const Scene &scene, Sampler &sampler) const 
{
    Ray ray(p0.SpawnRayTo(p1));
    Spectrum Tr(1.f);

    while (true) 
    {
        SurfaceInteraction isect;
        bool hitSurface = scene.Intersect(ray, &isect);

        // Handle opaque surface along ray's path
        // 如果光线前进时碰到了不透明的表面(即便是半透明的, 也会发射折射), 则两点不可见
        if (hitSurface && isect.primitive->GetMaterial() != nullptr)
            return Spectrum(0.0f);

        // Update transmittance for current ray segment
        // surfaces with a nullptr material pointer should be ignored in ray
        // visibility tests, as those surfaces are only used to bound the extent of participating media
        if (ray.medium) Tr *= ray.medium->Tr(ray, sampler);

        // Generate next ray segment or return final transmittance
		// 如果没有再碰到任何(可见)表面, 则从 p0 开始的光线到达了 p1, 退出循环; 否则继续测试
        if (!hitSurface) 
            break;

        ray = isect.SpawnRayTo(p1);
    }
    return Tr;
}

Spectrum Light::Le(const RayDifferential &ray) const { return Spectrum(0.f); }

AreaLight::AreaLight(const Transform &LightToWorld, const MediumInterface &medium,
                     int nSamples)
    : Light((int)LightFlags::Area, LightToWorld, medium, nSamples) 
{
    ++numAreaLights;
}

}  // namespace pbrt
