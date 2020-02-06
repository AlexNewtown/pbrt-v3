
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


// core/sampling.cpp*
#include "sampling.h"
#include "geometry.h"
#include "shape.h"

namespace pbrt {

// Sampling Function Definitions
void StratifiedSample1D(Float *samp, int nSamples, RNG &rng, bool jitter) 
{
    Float invNSamples = (Float)1 / nSamples;

    for (int i = 0; i < nSamples; ++i) 
    {
        Float delta = jitter ? rng.UniformFloat() : 0.5f;
        // �������� 4 ��������, ���������� 0.5, 1.5, 2.5, 3.5
        samp[i] = std::min((i + delta) * invNSamples, OneMinusEpsilon);
    }
}

void StratifiedSample2D(Point2f *samp, int nx, int ny, RNG &rng, bool jitter) 
{
    Float dx = (Float)1 / nx, dy = (Float)1 / ny;

    for (int y = 0; y < ny; ++y) 
    {
        for (int x = 0; x < nx; ++x) 
        {
            Float jx = jitter ? rng.UniformFloat() : 0.5f;
            Float jy = jitter ? rng.UniformFloat() : 0.5f;

            samp->x = std::min((x + jx) * dx, OneMinusEpsilon);
            samp->y = std::min((y + jy) * dy, OneMinusEpsilon);

            ++samp;
        }
    }
}

// ���߽� N-Rooks �㷨�������� nDim Ϊ 2 ʱ(���ڶ�άƽ����), ��֤ÿ����ÿ��ֻ��һ�������㣬�������ɸ��õ�һά�ֲ�
// LatinHypercube(&sampleArray2D[i][j * count].x, count, 2, rng); -> ���Ե���������һ�� samples[nSamples][2] �Ķ�ά����
void LatinHypercube(Float *samples, int nSamples, int nDim, RNG &rng) 
{
    // Generate LHS samples along diagonal
    Float invNSamples = (Float)1 / nSamples;

    /*
        �ضԽ������ɲ�����, ��ͼ���Ÿ�������, r �� [0, 1) ��������
        (0 + r, 0 + r) / 9, (1 + r, 1 + r) / 9, (2 + r, 2 + r) / 9 ...

        Y
        ^
        |
        |                                 *
        |
        |                             *
        |
        |                        *
        |
        |                    *
        |
        |                *
        |
        |            *
        |
        |        *
        |
        |    *
        |
        |*
        O------------------------------------------------> X
        ��ʱÿ����ÿ��ֻ��һ��������
    */
    for (int i = 0; i < nSamples; ++i)
    {
        for (int j = 0; j < nDim; ++j) 
        {
            Float sj = (i + (rng.UniformFloat())) * invNSamples;

            samples[nDim * i + j] = std::min(sj, OneMinusEpsilon);
        }
    }

    // Permute LHS samples in each dimension
    /*
        Y
        ^
        |
        |    *
        |
        |*
        |
        |                        *
        |
        |                *
        |
        |                             *
        |
        |            *
        |
        |        *
        |
        |                    *
        |
        |                                 *
        O------------------------------------------------> X
        �Ƚ�����ͬ������ x ����, �ٽ��� y ����
        ��������˳���, ÿ����ÿ����Ȼֻ��һ��������
     */
    for (int i = 0; i < nDim; ++i) 
    {
        for (int j = 0; j < nSamples; ++j) 
        {
            int other = j + rng.UniformUInt32(nSamples - j);
            std::swap(samples[nDim * j + i], samples[nDim * other + i]);
        }
    }
}



// P760, Բ���ϵĽ���-�ܾ�����, ����ֻ�Ǹ�ʾ��, û��ʵ��ʹ��
Point2f RejectionSampleDisk(RNG &rng) 
{
    Point2f p;
    do 
    {
        p.x = 1 - 2 * rng.UniformFloat();
        p.y = 1 - 2 * rng.UniformFloat();
    } while (p.x * p.x + p.y * p.y > 1);
    
    return p;
}



// p774
Vector3f UniformSampleHemisphere(const Point2f &u) 
{
    Float z = u[0]; // [0, 1)

    Float r = std::sqrt(std::max((Float)0, (Float)1. - z * z));
    Float phi = 2 * Pi * u[1];

    return Vector3f(r * std::cos(phi), r * std::sin(phi), z);
}

Float UniformHemispherePdf() { return Inv2Pi; }



Vector3f UniformSampleSphere(const Point2f &u) 
{
    Float z = 1 - 2 * u[0]; // (-1, 1)

    Float r = std::sqrt(std::max((Float)0, (Float)1 - z * z));
    Float phi = 2 * Pi * u[1];

    return Vector3f(r * std::cos(phi), r * std::sin(phi), z);
}

Float UniformSpherePdf() { return Inv4Pi; }



// P776
// Figure13.10, ֱ�Ӷ�Բ�̲��þ��Ȳ���, ʵ�ʲ�������ǲ����ȵ�(�������ۼ���Բ�ĸ���)
// Figure13.11, �� radius ��ƽ��, ���Եõ����ֲַ��Ľ��, ���任���̴��ڱ���...
Point2f UniformSampleDisk(const Point2f &u) {
    Float r = std::sqrt(u[0]);
    Float theta = 2 * Pi * u[1];
    return Point2f(r * std::cos(theta), r * std::sin(theta));
}

// P778, Figure13.12, ��λ�����ε���λԲ��ͬ��ӳ��, ��һ�ָ��õľ��ȷֲ�
// https://blog.csdn.net/codeboycjy/article/details/6225886
// https://psgraphics.blogspot.com/2011/01/improved-code-for-concentric-map.html
Point2f ConcentricSampleDisk(const Point2f &u) {
    // Map uniform random numbers to $[-1,1]^2$
    // ��ӳ�䵽��λ��������
    Point2f uOffset = 2.f * u - Vector2f(1, 1);

    // Handle degeneracy at the origin
    if (uOffset.x == 0 && uOffset.y == 0) return Point2f(0, 0);

    // Apply concentric mapping to point
    Float theta, r;
    if (std::abs(uOffset.x) > std::abs(uOffset.y)) {
        r = uOffset.x;
        theta = PiOver4 * (uOffset.y / uOffset.x);
    } else {
        r = uOffset.y;
        theta = PiOver2 - PiOver4 * (uOffset.x / uOffset.y);
    }

    return r * Point2f(std::cos(theta), std::sin(theta));
}



Float UniformConePdf(Float cosThetaMax) {
    return 1 / (2 * Pi * (1 - cosThetaMax));
}

Vector3f UniformSampleCone(const Point2f &u, Float cosThetaMax) {
    Float cosTheta = ((Float)1 - u[0]) + u[0] * cosThetaMax;
    Float sinTheta = std::sqrt((Float)1 - cosTheta * cosTheta);
    Float phi = u[1] * 2 * Pi;
    return Vector3f(std::cos(phi) * sinTheta, std::sin(phi) * sinTheta,
                    cosTheta);
}

Vector3f UniformSampleCone(const Point2f &u, Float cosThetaMax,
                           const Vector3f &x, const Vector3f &y,
                           const Vector3f &z) {
    Float cosTheta = Lerp(u[0], cosThetaMax, 1.f);
    Float sinTheta = std::sqrt((Float)1. - cosTheta * cosTheta);
    Float phi = u[1] * 2 * Pi;
    return std::cos(phi) * sinTheta * x + std::sin(phi) * sinTheta * y +
           cosTheta * z;
}



// P782
Point2f UniformSampleTriangle(const Point2f &u) {
    Float su0 = std::sqrt(u[0]);
    return Point2f(1 - su0, u[1] * su0);
}



Distribution2D::Distribution2D(const Float *func, int nu, int nv) 
{
    pConditionalV.reserve(nv);
    for (int v = 0; v < nv; ++v) {
        // Compute conditional sampling distribution for $\tilde{v}$
        pConditionalV.emplace_back(new Distribution1D(&func[v * nu], nu));
    }

    // Compute marginal sampling distribution $p[\tilde{v}]$
    std::vector<Float> marginalFunc;
    marginalFunc.reserve(nv);
    for (int v = 0; v < nv; ++v)
        marginalFunc.push_back(pConditionalV[v]->funcInt);
    pMarginal.reset(new Distribution1D(&marginalFunc[0], nv));
}

}  // namespace pbrt
