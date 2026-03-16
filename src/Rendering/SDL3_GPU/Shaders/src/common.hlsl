/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

// SDL3 GPU binding rules:
// Vertex:   t/s -> space0, b -> space1
// Fragment: t/s -> space2, b -> space3

#ifndef CSM_COMMON_HLSL
#define CSM_COMMON_HLSL

#if defined(VERTEX_SHADER)
cbuffer UBO : register(b0, space1)
{
    float4x4 u_matrix;
    float4x4 u_clipMatrix;
    float4 u_baseColor;
    float4 u_multiplyColor;
    float4 u_screenColor;
    float4 u_channelFlag;
};
#else
cbuffer UBO : register(b0, space3)
{
    float4x4 u_matrix;
    float4x4 u_clipMatrix;
    float4 u_baseColor;
    float4 u_multiplyColor;
    float4 u_screenColor;
    float4 u_channelFlag;
};

Texture2D s_texture0 : register(t0, space2);
SamplerState s_sampler0 : register(s0, space2);

Texture2D s_texture1 : register(t1, space2);
SamplerState s_sampler1 : register(s1, space2);

Texture2D s_blendTexture : register(t2, space2);
SamplerState s_blendTextureSampler : register(s2, space2);
#endif

#endif
