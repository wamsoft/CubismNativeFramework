/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "common.hlsl"

struct PSInput
{
    float2 v_texCoord : TEXCOORD0;
    float4 v_myPos : TEXCOORD1;
};

float4 main(PSInput input) : SV_Target0
{
    float isInside = step(u_baseColor.x, input.v_myPos.x / input.v_myPos.w)
        * step(u_baseColor.y, input.v_myPos.y / input.v_myPos.w)
        * step(input.v_myPos.x / input.v_myPos.w, u_baseColor.z)
        * step(input.v_myPos.y / input.v_myPos.w, u_baseColor.w);

    return u_channelFlag * s_texture0.Sample(s_sampler0, input.v_texCoord).a * isInside;
}
