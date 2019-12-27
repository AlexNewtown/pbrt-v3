
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

#ifndef PBRT_CORE_SAMPLER_H
#define PBRT_CORE_SAMPLER_H

// core/sampler.h*
#include "pbrt.h"
#include "geometry.h"
#include "rng.h"
#include <inttypes.h>

namespace pbrt {

// https://zhuanlan.zhihu.com/p/73943687

// The task of a Sampleris to **generate a sequence** of n-dimensional samples in $[0, 1)^n$ 
// ����ͼ���������Ҫ��ά�� n �ɾ���Ĺ��ߴ����㷨�������� �� (x, y, t, u, v, [(u0, u1), (u2, u3), (u4, u5), (u6, u7)])

// Sampler Declarations
class Sampler {
  public:
    // Sampler Interface
    virtual ~Sampler();
    // ÿ�����ص�Ĳ�������
    Sampler(int64_t samplesPerPixel);

    /*
        for (Point2i pixel : tileBounds)
        {
            sampler->StartPixel(pixel);
            // ������ sampler->StartThisSample(), ��Ըò����������µĲ�������

            do 
            {
                //Point2f p = a(sampler->Get2D()); // get x, y
                //Float t = c(sampler->Get1D()); // get t
                //...
                auto s = sampler->GetCameraSample(); // get x, y, t, u, v
            }
            while (sampler->StartNextSample());
        }
    */

    // ��ʼ��Ⱦ�������ǰ, �ȴ���������, �еĲ��������������Ϣ�������ɸ��õĲ�����
    virtual void StartPixel(const Point2i &p);
    // ֪ͨ�������Ե�ǰ���ص���һ����������в���
	virtual bool StartNextSample();

    // һЩ������(�� SPPM)������˳��ʹ������, ������Ҫ�ṩָ��ʹ��ĳ�������ķ���
    // virtual bool SetCurrentPixelSampleIndex(int64_t sampleIndex); 
	virtual bool SetSampleNumber(int64_t sampleNum);
    // int64_t GetCurrentSampleNumberIndex() const { return currentPixelSampleIndex_; }
	int64_t CurrentSampleNumber() const { return currentPixelSampleIndex; }


	// ʹ�� Get1D()��Get2D()��GetCameraSample() ��ö��������� 
	virtual Float   Get1D() = 0;
	virtual Point2f Get2D() = 0; //�൱�������������� Get1D, ���еĲ��������Խ�����ɸ��õķֲ�
	CameraSample GetCameraSample(const Point2i &pRaster);


    // ����Ⱦ֮ǰ�������в�����, �������� Get1D ������ʱ����
	// ��һ�Ժ���(Request/Get)����Ⱦ�����������ǽ�����صģ��տ�ʼ�Ӵ�ʱ��������. ���Ķ� DirectLightingIntegrator ��ʵ��ʱ, ���Բο��·���ע�������
    void Request1DArray(int n);
    void Request2DArray(int n);
    const Float *Get1DArray(int n);
    // ���� (x, y, t, u, v, [(u0, u1), (u2, u3), (u4, u5), (u6, u7)]) �Ĳ�������, Get2DArray ���ص��������Ų��ֵ�����
	const Point2f *Get2DArray(int n);

    // ����Ԥ�Ƶ�ÿ���ز�������, ������ѵĲ�������
    // ���� ZeroTwoSequenceSampler ʹ�� 2^n ���Ի����õ�����
    virtual int RoundCount(int n) const { return n; }

    // ��������ʹ�ö��߳̽��м���, ʹ�� Clone ��ÿ���߳�����һ�������Ĳ�����
    virtual std::unique_ptr<Sampler> Clone(int seed) = 0;

    std::string StateString() const {
      // pixel coord: ({0}, {1}), current pixel sample index: {2}
      return StringPrintf("(%d,%d), sample %" PRId64, currentPixel.x,
                          currentPixel.y, currentPixelSampleIndex);
    }

    // Sampler Public Data
    const int64_t samplesPerPixel;

  protected:
    // Sampler Protected Data
    Point2i currentPixel;
    int64_t currentPixelSampleIndex;

	/*
        sampleArray1D/2D ������, ʹ�ú�������Ⱦ���������һ��, �� Sampler �Ľӿ���Ƚ������ĵط�

        void DirectLightingIntegrator::Preprocess(...)
        {
            //...
            for (int i = 0; i < maxDepth; ++i)
            {
                for (size_t j = 0; j < scene.lights.size(); ++j)
                {
                    sampler.Request2DArray(nLightSamples[j]);
                    sampler.Request2DArray(nLightSamples[j]);
                
                    void Sampler::Request2DArray(int nLightSamples)
                    {
                        //...
                        samples2DArraySizes.push_back(nPixel);
                        sampleArray2D.push_back(std::vector<Point2f>(nLightSamples * samplesPerPixel));
                    }
                }
            }
        }

	    ÿ��������Ĵ�СΪ n * samplesPerPixel��samples1/2DArraySizes ��¼������� n �Ĵ�С, ����Ҫ�������� Get1DArray ʱ�Դ����������У��
    */
    std::vector<int> samples1DArraySizes, samples2DArraySizes;	
    std::vector<std::vector<Float>> sampleArray1D;
    std::vector<std::vector<Point2f>> sampleArray2D;

  private:
    /*
        void SamplerIntegrator::Render(const Scene &scene)
        {
            // ...
            for (Point2i pixel : tileBounds)
            {
                // ...
                tileSampler->StartPixel(pixel);	// currentPixelSampleIndex = 0; array1DOffset = array2DOffset = 0;

                do
                {
                    Spectrum L = Li(ray, scene, *tileSampler, arena);

                    Spectrum DirectLightingIntegrator::Li(...)
                    {
                        // ...
                        L += UniformSampleAllLights(isect, scene, arena, sampler, nLightSamples);

                        // �ݹ�׷�پ�����, ���õ� sampleArray2D[nLightSize * depth ... nLightSize * (depth + n)] �е�����
                    }

                    Spectrum UniformSampleAllLights(...)
                    {
                        //TODO: �鷳�ĵط�����ÿ�δ� sampleArray2D ȡ������һ������, ������һ��
                        for (size_t j = 0; j < scene.lights.size(); ++j)
                        {
                            const Point2f *Sampler::Get2DArray(int n)
                            {
                                 //...
                                 return &sampleArray2D[array2DOffset++][currentPixelSampleIndex * n];
                            }

                            int nLightSamples = nLightSamples[j];
                            const Point2f *uLightArray = sampler.Get2DArray(nLightSamples); // ��� uLightArray[nLightSamples]
                            const Point2f *uScatteringArray = sampler.Get2DArray(nLightSamples);

                            if (!uLightArray || !uScatteringArray)
                            {
                                // ...
                            }
                            else
                            {
                                // ...

                                for (int k = 0; k < nLightSamples; ++k)
                                {
                                    Ld += EstimateDirect(it, uScatteringArray[k], *light,
                                                         uLightArray[k], scene, sampler, arena, handleMedia);
                                }
                    
                                // ...
                            }
                        }
                    }

                    filmTile->AddSample(cameraSample.pFilm, L, rayWeight);
                }
                while (tileSampler->StartNextSample()); // ++currentPixelSampleIndex; array1DOffset = array2DOffset = 0;
            }
        }

	    array1D/2DOffset ��¼��ǰ������ sampleArray1/2D �е�����
    */
    size_t array1DOffset, array2DOffset;
};

// ��Ե������ص����������Ĳ�����
class PixelSampler : public Sampler {
  public:
    // PixelSampler Public Methods
    // ÿ���صĲ�������, ÿ����������Ҫ������ά��
    PixelSampler(int64_t samplesPerPixel, int nSampledDimensions);

    // ���ز�������������� StartPixel ������ samples1D �� samples2D �еĲ�����(���⻹�� sampleArray1D �� sampleArray2D)

    bool StartNextSample();
    bool SetSampleNumber(int64_t);
    Float Get1D();
    Point2f Get2D();

  protected:
    // PixelSampler Protected Data
    // ���� nSampledDimensions ��ǰ�������й� Get1D/2D ʹ�õ�����
    std::vector<std::vector<Float>> samples1D;
    std::vector<std::vector<Point2f>> samples2D;
    int current1DDimension = 0, current2DDimension = 0;

    // 1.�ڹ���ʱ���ɳ�ʼ������
    // 2.�����볬�� nSampledDimensions ��Χ������ʱ, ����ʹ�� rng ���ɵ� [0, 1) �ھ��ȷֲ�������
    RNG rng;
};

// �������ͼ��ƽ�����������Ĳ�����(�� Halton Sampler)
class GlobalSampler : public Sampler {
  public:
    // GlobalSampler Public Methods
    bool StartNextSample();
    void StartPixel(const Point2i &);
    bool SetSampleNumber(int64_t sampleNum);
    Float Get1D();
    Point2f Get2D();

    GlobalSampler(int64_t samplesPerPixel) : Sampler(samplesPerPixel) {}

    // GlobalSampler �����������ʵ���������ӿ�, GlobalSampler ���������ӿڵõ���������
    // �ο� P429/Table7.2 ���

    // ��� currentPixel �ĵ� sampleNum ��������ȫ������(Table7.2 �ĵ�һ��)
    virtual int64_t GetIndexForSample(int64_t sampleNum) const = 0;

    // �����һ�е�ȫ������, ���ص��������ص��ϵĲ���ֵ
    // ����������� (1.250000, 2.333333) �Ĳ���ֵ�� (0.250000, 0.333333), ���ݵڶ������� dimension ������ 0.250000 ���� 0.333333
    virtual Float SampleDimension(int64_t index, int dimension) const = 0;

  private:
    // GlobalSampler Private Data
    int dimension;
    int64_t intervalSampleIndex; // ȫ������

    // PBRT ����ǰ����ά�����ɲ��������������, ���԰����Ǳ����� (x, y, t, u, v), �������Ĳ��������� sampleArray1D/2D, ����ĸ� Get1D/2D ʹ��
    static const int arrayStartDim = 5; // (x, y, t, u, v, [(u0, u1), (u2, u3), (u4, u5), (u6, u7)])
    int arrayEndDim;
};

}  // namespace pbrt

#endif  // PBRT_CORE_SAMPLER_H
