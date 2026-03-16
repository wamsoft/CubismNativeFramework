/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "common.hlsl"
#include "FragShaderSrcColorBlend.hlsl"
#include "FragShaderSrcAlphaBlend.hlsl"

struct PSInput
{
    float2 v_texCoord : TEXCOORD0;
    float2 v_blendCoord : TEXCOORD1;
    float4 v_clipPos : TEXCOORD2;
};

float4 main(PSInput input) : SV_Target0
{
    float4 texColor = s_texture0.Sample(s_sampler0, input.v_texCoord);
    texColor.rgb = texColor.rgb * u_multiplyColor.rgb;
    texColor.rgb = (texColor.rgb + u_screenColor.rgb * texColor.a) - (texColor.rgb * u_screenColor.rgb);
    float4 col_formask = texColor * u_baseColor;

    float2 clipUv = input.v_clipPos.xy / input.v_clipPos.w;
    float4 clipMask = (1.0 - s_texture1.Sample(s_sampler1, clipUv)) * u_channelFlag;
    float maskVal = clipMask.r + clipMask.g + clipMask.b + clipMask.a;
    float4 colorSource = col_formask * maskVal;

    float4 colorDestination = ConvertPremultipliedToStraight(s_blendTexture.Sample(s_blendTextureSampler, input.v_blendCoord));
    return AlphaBlend(ColorBlend(colorSource.rgb, colorDestination.rgb), colorSource, colorDestination);
}
