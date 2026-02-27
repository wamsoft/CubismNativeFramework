// SDL3 GPU API requires specific descriptor set layout for SPIR-V:
//   Vertex samplers:        set = 0
//   Vertex uniform buffers: set = 1
//   Fragment samplers:        set = 2
//   Fragment uniform buffers: set = 3
// Compile with -DVERTEX_SHADER for .vert files

#ifdef VERTEX_SHADER
layout(set = 1, binding = 0) uniform UBO
{
    mat4 u_matrix;
    mat4 u_clipMatrix;
    vec4 u_baseColor;
    vec4 u_multiplyColor;
    vec4 u_screenColor;
    vec4 u_channelFlag;
}ubo;
// Vertex shaders do not use samplers
#else
layout(set = 3, binding = 0) uniform UBO
{
    mat4 u_matrix;
    mat4 u_clipMatrix;
    vec4 u_baseColor;
    vec4 u_multiplyColor;
    vec4 u_screenColor;
    vec4 u_channelFlag;
}ubo;

layout(set = 2, binding = 0) uniform sampler2D s_texture0;
layout(set = 2, binding = 1) uniform sampler2D s_texture1;
layout(set = 2, binding = 2) uniform sampler2D s_blendTexture;
#endif
