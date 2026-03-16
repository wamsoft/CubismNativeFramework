/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#define VERTEX_SHADER
#include "common.hlsl"

struct VSInput
{
    float4 a_position : TEXCOORD0;
    float2 a_texCoord : TEXCOORD1;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 v_texCoord : TEXCOORD0;
    float2 v_blendCoord : TEXCOORD1;
    float4 v_clipPos : TEXCOORD2;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    float4 pos = mul(u_matrix, input.a_position);
    output.position = pos;
    output.v_clipPos = mul(u_clipMatrix, input.a_position);
    output.v_clipPos.y = 1.0 - output.v_clipPos.y;

    output.v_texCoord = input.a_texCoord;
    output.v_texCoord.y = 1.0 - output.v_texCoord.y;

    float2 ndcPos = pos.xy / pos.w;
    output.v_blendCoord = ndcPos * 0.5 + 0.5;
    output.v_blendCoord.y = 1.0 - output.v_blendCoord.y;
    return output;
}
