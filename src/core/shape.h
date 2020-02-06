
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

#ifndef PBRT_CORE_SHAPE_H
#define PBRT_CORE_SHAPE_H

/*
pbrt hides details about primitives behind a two-level abstraction. 
The Shape class provides access to the raw geometric properties of the primitive, such as its surface area and bounding box, and provides a ray intersection routine. 
The Primitive class encapsulates additional nongeometric information about the primitive, such as its material properties. The rest of the renderer then deals only with the abstract Primitive interface. This chapter will focus on the geometry-only Shape class; the Primitive interface is a key topic of Chapter 4.
���� Shape ֻ����˳����ж���ļ��νṹ, GeometricPrimitive ������� Shape, Material, AreaLight

All shapes are defined in object coordinate space; for example, all spheres are defined in a coordinate system where the center of the sphere is at the origin.
���� shape ���������ڶ�������ռ���(��� sphere �������趨�����ĵ�λ��, ���������ڶ��������е�ԭ��λ��)



*/

// core/shape.h*
#include "pbrt.h"
#include "geometry.h"
#include "interaction.h"
#include "memory.h"
#include "transform.h"

namespace pbrt {

// Shape Declarations
class Shape {
  public:
    // Shape Interface
    Shape(const Transform *ObjectToWorld, const Transform *WorldToObject,
          bool reverseOrientation);
    virtual ~Shape();

	// �ֱ𷵻ض�������ϵ����������ϵ�µİ�Χ��
	// ʹ�ö���İ�Χ�н�����ǰ�ཻ����, ���Խ�ʡ�ཻ����Ŀ���
    virtual Bounds3f ObjectBound() const = 0;
    virtual Bounds3f WorldBound() const;

	// �ж� ray �Ƿ��� shape �ཻ, ���ཻ��������㽻���ϵ�΢�ּ�������
	// ϸ�ڼ� Section 3.1.3 Intersection Tests
  // tHit: ���� isect.hit �� ray �ľ���, ��������ǰ��ֹ�ཻ����
    virtual bool Intersect(const Ray &ray, Float *tHit,
                           SurfaceInteraction *isect,
                           bool testAlphaTexture = true) const = 0;
	// ֻ�ж� ray �Ƿ��� shape �ཻ
    virtual bool IntersectP(const Ray &ray,
                            bool testAlphaTexture = true) const {
        return Intersect(ray, nullptr, nullptr, testAlphaTexture);
    }

    virtual Float Area() const = 0;



    // AreaLight ������ Shape, Sample �� Pdf ��Ҫ�ڲ��� AreaLight ʱʹ��
    // P837, Sample a point on the surface of the shape and return the PDF with
    // respect to area on the surface.
    virtual Interaction Sample(const Point2f &u, Float *pdf) const = 0;
    // Shapes almost always sample uniformly by area on their surface
    virtual Float Pdf(const Interaction &) const { return 1 / Area(); }

    // P837, Sample a point on the shape given a reference point ref and
    // return the PDF with respect to solid angle from ref
    virtual Interaction Sample(const Interaction &ref, const Point2f &u,
                               Float *pdf) const;
    virtual Float Pdf(const Interaction &ref, const Vector3f &wi) const;

    // Returns the solid angle subtended by the shape w.r.t. the reference
    // point p, given in world space. Some shapes compute this value in
    // closed-form, while the default implementation uses Monte Carlo
    // integration; the nSamples parameter determines how many samples are
    // used in this case.
    virtual Float SolidAngle(const Point3f &p, int nSamples = 512) const;



    // Shape Public Data
	// �� ray-intersection test ʱ��Ҫ�� ray �任����������ϵ��, ����...ʱ��Ҫ�� shape �任����������ϵ��, shape ͬʱ�������������任
	// because multiple shapes in the scene will frequently have the same transformation applied to them, pbrt keeps a pool of Transforms so that they can be reused and passes pointers to the shared Transforms to the shapes.
	// As such, the Shape destructor does not delete its Transform pointers, leaving the Transform management code to manage that memory instead.
    const Transform *ObjectToWorld, *WorldToObject;

	// Shapes also take a Boolean parameter, reverseOrientation, that indicates whether their surface normal directions should be reversed from the default. 
	// This capability is useful because the orientation of the surface normal is used to determine which side of a shape is ��outside.��
	// For example, shapes that emit illumination are emissive only on the side the surface normal lies on. 
	// The value of this parameter is managed via the ReverseOrientation statement in pbrt input files.
    const bool reverseOrientation; // �Ƿ�ת���߳���, ����˵���� shape ���ⲿ
    const bool transformSwapsHandedness; // ObjectToWorld �任�Ƿ�ı�������ϵ������(���� -> ����)
};

}  // namespace pbrt

#endif  // PBRT_CORE_SHAPE_H
