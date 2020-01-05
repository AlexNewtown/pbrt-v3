
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

#ifndef PBRT_CORE_FILM_H
#define PBRT_CORE_FILM_H

// core/film.h*
#include "pbrt.h"
#include "geometry.h"
#include "spectrum.h"
#include "filter.h"
#include "stats.h"
#include "parallel.h"

namespace pbrt {

// FilmTilePixel Declarations
struct FilmTilePixel
{
    Spectrum contribSum = 0.f;
    Float filterWeightSum = 0.f;
};

/*
    (0,0)_ _ _ _ _ _ _x ����
        |\            |
        |   \         |
        |     \       |    film ������ϵ
        |       \     |
        |         \   |
        |_ _ _ _ _ _ \|
      y ����          (width, height)
*/

// Film Declarations
class Film
{
  public:
    // Film Public Methods
    Film(const Point2i &resolution, const Bounds2f &cropWindow,
         std::unique_ptr<Filter> filter, Float diagonal,
         const std::string &filename, Float scale,
         Float maxSampleLuminance = Infinity);

    Bounds2i GetSampleBounds() const;
    Bounds2f GetPhysicalExtent() const;

    std::unique_ptr<FilmTile> GetFilmTile(const Bounds2i &sampleBounds);
    void MergeFilmTile(std::unique_ptr<FilmTile> tile);

    void SetImage(const Spectrum *img) const;
    void AddSplat(const Point2f &p, Spectrum v);
    void WriteImage(Float splatScale = 1);
    void Clear();

    // Film Public Data
    const Point2i fullResolution;
    const Float diagonal;
    std::unique_ptr<Filter> filter;
    const std::string filename;
    Bounds2i croppedPixelBounds;

  private:
    // Film Private Data
    struct Pixel
    {
        Pixel() { xyz[0] = xyz[1] = xyz[2] = filterWeightSum = 0; }
        Float xyz[3];
        Float filterWeightSum;
        AtomicFloat splatXYZ[3]; // Atomic
        Float pad; // ռλ
    };
    std::unique_ptr<Pixel[]> pixels;

    // Ԥ������˲���Ȩ��, P486
    // The error introduced by not evaluating the filter at each sample��s precise location isn��t noticeable in practice.
    static PBRT_CONSTEXPR int filterTableWidth = 16;
    Float filterTable[filterTableWidth * filterTableWidth];

    std::mutex mutex;
    const Float scale;
    const Float maxSampleLuminance;

    // Film Private Methods
    // ���ָ�������ϵ����ص�
    Pixel &GetPixel(const Point2i &p)
    {
        CHECK(InsideExclusive(p, croppedPixelBounds));

        int width = croppedPixelBounds.pMax.x - croppedPixelBounds.pMin.x;
        int offset = (p.x - croppedPixelBounds.pMin.x) +
                     (p.y - croppedPixelBounds.pMin.y) * width;

        return pixels[offset];
    }
};

// ÿ���߳���Ⱦ�Ľ�����ȴ��뵽һ�� FilmTile ��, ���߳���Ⱦ����ʱ����� FilmTile �ϲ��� Film ��
class FilmTile
{
  public:
    // FilmTile Public Methods
    // return std::unique_ptr<FilmTile>(
    //     new FilmTile(tilePixelBounds, filter->radius, filterTable, filterTableWidth, maxSampleLuminance));
    FilmTile(const Bounds2i &pixelBounds, const Vector2f &filterRadius,
             const Float *filterTable, int filterTableSize,
             Float maxSampleLuminance)
        : pixelBounds(pixelBounds),
          filterRadius(filterRadius),
          invFilterRadius(1 / filterRadius.x, 1 / filterRadius.y),
          filterTable(filterTable),
          filterTableSize(filterTableSize),
          maxSampleLuminance(maxSampleLuminance)
    {
        pixels = std::vector<FilmTilePixel>(std::max(0, pixelBounds.Area()));
    }

    // filmTile->AddSample(cameraSample.pFilm, L, rayWeight);
    void AddSample(const Point2f &pFilm, Spectrum L,
                   Float sampleWeight = 1.)
    {
        ProfilePhase _(Prof::AddFilmSample);

        if (L.y() > maxSampleLuminance)
            L *= maxSampleLuminance / L.y();

        // Compute sample's raster bounds
        // ��������ת��Ϊ��ɢ���� pFilmDiscrete, ����Ϊ���ģ�������������㹱�׵ķ�Χ
        // [ceil(x - 2.5), floor(x + 1.5) + 1)
        // �� '2 < pFilm.x && pFilm.x <= 2.5 && 2 < pFilm.y && pFilm.y <=2.5' �������ϵ��Ҹ����ӿ���
        // (2.5, 2.5) -> [0, 0] => (5, 5)
        // û������, ʵ�����ǽ� [2.5, 2.5] �Ĳ���ֵ���׸� [0.5, 0.5] �� [4.5, 4.5] �����ص�
        Point2f pFilmDiscrete = pFilm - Vector2f(0.5f, 0.5f);
        Point2i p0 = (Point2i)Ceil(pFilmDiscrete - filterRadius);
        Point2i p1 = (Point2i)Floor(pFilmDiscrete + filterRadius) + Point2i(1, 1);
        p0 = Max(p0, pixelBounds.pMin);
        p1 = Min(p1, pixelBounds.pMax);

        // Loop over filter support and add sample to pixel arrays

        // Precompute $x$ and $y$ filter table offsets
		// �ο� P492, �����Ǹ��Ż�, Ԥ�ȼ����������ص���Ȩ�ر��е�����
        int *ifx = ALLOCA(int, p1.x - p0.x);
        for (int x = p0.x; x < p1.x; ++x) 
        {   
            // min(floor(abs(0 - 2) * 1/2 * 16), 15) = 15
            // min(floor(abs(1 - 2.1) * 1/2 * 16), 15) = 8
            Float fx = std::abs((x - pFilmDiscrete.x) * invFilterRadius.x * filterTableSize);
            ifx[x - p0.x] = std::min((int)std::floor(fx), filterTableSize - 1);
        }
        int *ify = ALLOCA(int, p1.y - p0.y);
        for (int y = p0.y; y < p1.y; ++y) 
        {
            Float fy = std::abs((y - pFilmDiscrete.y) * invFilterRadius.y * filterTableSize);
            ify[y - p0.y] = std::min((int)std::floor(fy), filterTableSize - 1);
        }

        for (int y = p0.y; y < p1.y; ++y)
        {
            for (int x = p0.x; x < p1.x; ++x)
            {
                // Evaluate filter value at $(x,y)$ pixel
                int offset = ify[y - p0.y] * filterTableSize + ifx[x - p0.x];
                Float filterWeight = filterTable[offset];

                // Update pixel values with filtered sample contribution

                // �ο� P473 ʽ 7.12, ���������˲����̷ֱ������Ӻͷ�ĸ������� Film ����ͼ���ʱ��Ž��г�������
                FilmTilePixel &pixel = GetPixel(Point2i(x, y));
                pixel.contribSum += L * sampleWeight * filterWeight;
                pixel.filterWeightSum += filterWeight;
            }
        }
    }

    FilmTilePixel &GetPixel(const Point2i &p)
    {
        CHECK(InsideExclusive(p, pixelBounds));
        int width = pixelBounds.pMax.x - pixelBounds.pMin.x;
        int offset =
            (p.x - pixelBounds.pMin.x) + (p.y - pixelBounds.pMin.y) * width;
        return pixels[offset];
    }

    const FilmTilePixel &GetPixel(const Point2i &p) const
    {
        CHECK(InsideExclusive(p, pixelBounds));
        int width = pixelBounds.pMax.x - pixelBounds.pMin.x;
        int offset =
            (p.x - pixelBounds.pMin.x) + (p.y - pixelBounds.pMin.y) * width;
        return pixels[offset];
    }

    Bounds2i GetPixelBounds() const { return pixelBounds; }

  private:
    // FilmTile Private Data
    const Bounds2i pixelBounds;

    const Vector2f filterRadius, invFilterRadius;

    const Float *filterTable;
    const int filterTableSize;

    std::vector<FilmTilePixel> pixels;

    const Float maxSampleLuminance;

    friend class Film;
};

Film *CreateFilm(const ParamSet &params, std::unique_ptr<Filter> filter);

}  // namespace pbrt

#endif  // PBRT_CORE_FILM_H
