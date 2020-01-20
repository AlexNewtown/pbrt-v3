
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

#ifndef PBRT_SAMPLERS_MAXMIN_H
#define PBRT_SAMPLERS_MAXMIN_H

// samplers/maxmin.h*
#include "sampler.h"
#include "lowdiscrepancy.h"

namespace pbrt {

// 在 ZeroTwoSequenceSampler 的基础上, 确保每个样本间保持一定距离

// MaxMinDistSampler Declarations
class MaxMinDistSampler : public PixelSampler {
  public:
    // MaxMinDistSampler Public Methods
    void StartPixel(const Point2i &);
    std::unique_ptr<Sampler> Clone(int seed);
    int RoundCount(int count) const { return RoundUpPow2(count); }

    MaxMinDistSampler(int64_t samplesPerPixel, int nSampledDimensions)
        : PixelSampler  // 这样子用 lambda 表达式并不方便啊
          (
              [](int64_t spp) 
              {
                  int Cindex = Log2Int(spp);
                  if (Cindex >= sizeof(CMaxMinDist) / sizeof(CMaxMinDist[0])) // if(Cindex >= 17), 目前就 17 个符合 MaxMin 分布的矩阵, 多了不支持???
                  {
                      Warning(
                          "No more than %d samples per pixel are supported "
                          "with "
                          "MaxMinDistSampler. Rounding down.",
                          (1 << int(sizeof(CMaxMinDist) /
                                    sizeof(CMaxMinDist[0]))) -
                              1);
                      spp = (1 << (sizeof(CMaxMinDist) /
                                   sizeof(CMaxMinDist[0]))) -
                            1;
                  }
                  if (!IsPowerOf2(spp)) 
                  {
                      spp = RoundUpPow2(spp);
                      Warning(
                          "Non power-of-two sample count rounded up to %" PRId64
                          " for MaxMinDistSampler.",
                          spp);
                  }
                  return spp;
              }(samplesPerPixel), // param1
              nSampledDimensions  // param2
          ) 
    {
        int Cindex = Log2Int(samplesPerPixel);

        CHECK(Cindex >= 0 &&
              Cindex < (sizeof(CMaxMinDist) / sizeof(CMaxMinDist[0])));
        CPixel = CMaxMinDist[Cindex];
    }

  private:
    // MaxMinDistSampler Private Data
    const uint32_t *CPixel;
};

MaxMinDistSampler *CreateMaxMinDistSampler(const ParamSet &params);

}  // namespace pbrt

#endif  // PBRT_SAMPLERS_MAXMIN_H
