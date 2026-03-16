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
};

float4 main(PSInput input) : SV_Target0
{
    float4 texColor = s_texture0.Sample(s_sampler0, input.v_texCoord);
    texColor.rgb = texColor.rgb * u_multiplyColor.rgb;
    texColor.rgb = (texColor.rgb + u_screenColor.rgb * texColor.a) - (texColor.rgb * u_screenColor.rgb);
    float4 colorSource = ConvertPremultipliedToStraight(texColor * u_baseColor);
    float4 colorDestination = ConvertPremultipliedToStraight(s_blendTexture.Sample(s_blendTextureSampler, input.v_blendCoord));
    return AlphaBlend(ColorBlend(colorSource.rgb, colorDestination.rgb), colorSource, colorDestination);
}
