
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

#if defined(_MSC_VER)
#define NOMINMAX
#pragma once
#endif

#ifndef PBRT_CORE_INTERACTION_H
#define PBRT_CORE_INTERACTION_H

// core/interaction.h*
#include "pbrt.h"
#include "geometry.h"
#include "transform.h"
#include "medium.h"
#include "material.h"

namespace pbrt {

// generating from a ray�Cshape intersection or from a ray passing through participating media
// interaction ��¼ ray �� surface/media ͨ���ཻ���Ժ����ɵľֲ������ϵĸ�����Ϣ, ���ں�����ɫ����, �μ��������ɵ�

// Interaction Declarations
struct Interaction {
    // Interaction Public Methods
    // A number of Interaction constructors are available; depending on what sort of interaction is being constructed and what sort of information about it is relevant
    Interaction() : time(0) {}
    Interaction(const Point3f &p, const Normal3f &n, const Vector3f &pError,
                const Vector3f &wo, Float time,
                const MediumInterface &mediumInterface)
        : p(p),
          time(time),
          pError(pError),
          wo(Normalize(wo)),
          n(n),
          mediumInterface(mediumInterface) {}

    bool IsSurfaceInteraction() const { return n != Normal3f(); }

    // �ڽ��� p ��, ���ݳ��䷽�� d ����һ���� p2/it �����µĹ���
    Ray SpawnRay(const Vector3f &d) const {
        Point3f o = OffsetRayOrigin(p, pError, n, d);
        return Ray(o, d, Infinity, time, GetMedium(d));
    }
    Ray SpawnRayTo(const Point3f &p2) const {
        Point3f origin = OffsetRayOrigin(p, pError, n, p2 - p);
        Vector3f d = p2 - p;
        return Ray(origin, d, 1 - ShadowEpsilon, time, GetMedium(d));
    }
    Ray SpawnRayTo(const Interaction &it) const {
        Point3f origin = OffsetRayOrigin(p, pError, n, it.p - p);
        Point3f target = OffsetRayOrigin(it.p, it.pError, it.n, origin - it.p);
        Vector3f d = target - origin;
        return Ray(origin, d, 1 - ShadowEpsilon, time, GetMedium(d));
    }

    Interaction(const Point3f &p, const Vector3f &wo, Float time,
                const MediumInterface &mediumInterface)
        : p(p), time(time), wo(wo), mediumInterface(mediumInterface) {}
    Interaction(const Point3f &p, Float time,
                const MediumInterface &mediumInterface)
        : p(p), time(time), mediumInterface(mediumInterface) {}

    bool IsMediumInteraction() const { return !IsSurfaceInteraction(); }

    const Medium *GetMedium(const Vector3f &w) const {
        return Dot(w, n) > 0 ? mediumInterface.outside : mediumInterface.inside;
    }
    const Medium *GetMedium() const {
        CHECK_EQ(mediumInterface.inside, mediumInterface.outside);
        return mediumInterface.inside;
    }

    // Interaction Public Data
    Point3f p;       // ������ռ��У������λ��
    Float time;
    // For interactions where the point  was computed by ray intersection, some floating-point error is generally present in the p value. 
    // pError gives a conservative bound on this error; it��s  for points in participating media. 
    // See Section 3.9 for more on pbrt��s approach to managing floating-point error and in particular Section 3.9.4 for how this bound is computed for various shapes.
    Vector3f pError; // �ۻ��ĸ������������

    // prev   light
    // -----  -----
    //   ^      ^
    //    \    /
    //  wo \  / wi
    //      \/
    //    ------
    //    isect
    Vector3f wo;     // ������ߵķ���(ֻ�� ray�Cshape intersection ʱ�Ż������������???)
    Normal3f n;      // ���㴦�ı��淨��
    MediumInterface mediumInterface; // ���������ڵ� medium
};

class MediumInteraction : public Interaction {
  public:
    // MediumInteraction Public Methods
    MediumInteraction() : phase(nullptr) {}
    MediumInteraction(const Point3f &p, const Vector3f &wo, Float time,
                      const Medium *medium, const PhaseFunction *phase)
        : Interaction(p, wo, time, medium), phase(phase) {}

    bool IsValid() const { return phase != nullptr; }

    // MediumInteraction Public Data
    const PhaseFunction *phase;
};

// The geometry of a particular point on a surface (often a position found by intersecting a ray against the surface) is represented by a SurfaceInteraction. 
// Having this abstraction lets most of the system work with points on surfaces without needing to consider the particular type of geometric shape the points lie on;
// SurfaceInteraction �����˺��� lighting, shading �Ȳ�������� shape ����ϵ
// SurfaceInteraction Declarations
class SurfaceInteraction : public Interaction {
  public:
    // SurfaceInteraction Public Methods
    SurfaceInteraction() {}
    SurfaceInteraction(const Point3f &p, const Vector3f &pError,
                       const Point2f &uv, const Vector3f &wo,
                       const Vector3f &dpdu, const Vector3f &dpdv,
                       const Normal3f &dndu, const Normal3f &dndv, Float time,
                       const Shape *sh,
                       int faceIndex = 0);

    // ���ò�ͬ�ڱ���ԭʼ���νṹ, ������ɫ�� shading �ṹ
    void SetShadingGeometry(const Vector3f &dpdu, const Vector3f &dpdv,
                            const Normal3f &dndu, const Normal3f &dndv,
                            bool orientationIsAuthoritative);

    // ���㽻���ϵ�ɢ�䷽��, ����ḳֵ�� this->bsdf, this->bssrdf ��, ���ں�������ɫ����
    void ComputeScatteringFunctions(
        const RayDifferential &ray, MemoryArena &arena,
        bool allowMultipleLobes = false,
        TransportMode mode = TransportMode::Radiance);

    // compute information about the projected size of the surface area around the intersection on the image plane 
    // for use in texture antialiasing
    void ComputeDifferentials(const RayDifferential &r) const;

    // ������㴦��һ���Է����ͼԪ��, ����� = �Է��� + �����, Le(light emit) ���������Է���Ĵ�С
    Spectrum Le(const Vector3f &w) const;

    // all of the shapes that pbrt supports do have at least a local parametric description - that for some range of $(u, v) $ values, points on the surface are given by some function $f$ such that $\mathrm{ p} = f(u, v) $
    // The parametric partial derivatives of the surface, $\partial \mathrm{p} / \partial u$ and $\partial \mathrm{p} / \partial v$, lie in the tangent plane but are not necessarily orthogonal.
    // The surface normal $\mathbf{n}$ is given by the cross product of $\partial \mathrm{p} / \partial u$ and $\partial \mathrm{p} / \partial v .$ 
    // The vectors $\partial \mathbf{n} / \partial u$ and $\partial \mathbf{n} / \partial v$ record the differential change in surface normal as we move $u$ and $v$ along the surface.
    // pbrt ʹ�õ����� shape ������һ�ֱ������������ʽ

    // SurfaceInteraction Public Data
    Point2f uv;				// ����������� UV ����, �����������, ���ɷ��ߵ�
	Vector3f dpdu, dpdv;	// ���� p �Ĳ���ƫ����(����ı仯��), λ�ڵ� p ����ƽ����(��������Ҫ����)
	Normal3f dndu, dndv;	// ���� n �Ĳ���ƫ����(���ߵı仯��)
    const Shape *shape = nullptr;

    // SurfaceInteraction stores a second instance of a surface normal and the various partial derivatives to represent possibly perturbed values of these quantities as can be generated by bump mapping or interpolated per-vertex normals with triangles.
	// �洢�ɰ�͹����������������𶥵㷨�߲�ֵ�õ�����ɫ���ߵ�ֵ
    struct {
        Normal3f n;	// ��ɫ����
        Vector3f dpdu, dpdv;
        Normal3f dndu, dndv;
    } shading;

    const Primitive *primitive = nullptr; // ���������� primitive, ������ȡ����Ĳ�����Ϣ

    BSDF *bsdf = nullptr;
    BSSRDF *bssrdf = nullptr;

    // ???
    mutable Vector3f dpdx, dpdy;
    mutable Float dudx = 0, dvdx = 0, dudy = 0, dvdy = 0;

    // Added after book publication. Shapes can optionally provide a face
    // index with an intersection point for use in Ptex texture lookups.
    // If Ptex isn't being used, then this value is ignored.
    int faceIndex = 0;
};

}  // namespace pbrt

#endif  // PBRT_CORE_INTERACTION_H
