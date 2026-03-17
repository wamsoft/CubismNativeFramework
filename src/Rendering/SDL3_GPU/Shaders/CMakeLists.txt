set(normal_shader_files
   ${CMAKE_CURRENT_SOURCE_DIR}/src/FragShaderSrc.frag.hlsl
   ${CMAKE_CURRENT_SOURCE_DIR}/src/FragShaderSrcCopy.frag.hlsl
   ${CMAKE_CURRENT_SOURCE_DIR}/src/FragShaderSrcMask.frag.hlsl
   ${CMAKE_CURRENT_SOURCE_DIR}/src/FragShaderSrcMaskInverted.frag.hlsl
   ${CMAKE_CURRENT_SOURCE_DIR}/src/FragShaderSrcMaskInvertedPremultipliedAlpha.frag.hlsl
   ${CMAKE_CURRENT_SOURCE_DIR}/src/FragShaderSrcMaskPremultipliedAlpha.frag.hlsl
   ${CMAKE_CURRENT_SOURCE_DIR}/src/FragShaderSrcPremultipliedAlpha.frag.hlsl
   ${CMAKE_CURRENT_SOURCE_DIR}/src/FragShaderSrcSetupMask.frag.hlsl
   ${CMAKE_CURRENT_SOURCE_DIR}/src/VertShaderSrc.vert.hlsl
   ${CMAKE_CURRENT_SOURCE_DIR}/src/VertShaderSrcCopy.vert.hlsl
   ${CMAKE_CURRENT_SOURCE_DIR}/src/VertShaderSrcMasked.vert.hlsl
   ${CMAKE_CURRENT_SOURCE_DIR}/src/VertShaderSrcSetupMask.vert.hlsl
   ${CMAKE_CURRENT_SOURCE_DIR}/src/VertShaderSrcBlend.vert.hlsl
   ${CMAKE_CURRENT_SOURCE_DIR}/src/VertShaderSrcMaskedBlend.vert.hlsl
)

set(blend_fragment_shader_files
   ${CMAKE_CURRENT_SOURCE_DIR}/src/FragShaderSrcBlend.frag.hlsl
   ${CMAKE_CURRENT_SOURCE_DIR}/src/FragShaderSrcMaskBlend.frag.hlsl
   ${CMAKE_CURRENT_SOURCE_DIR}/src/FragShaderSrcMaskInvertedBlend.frag.hlsl
   ${CMAKE_CURRENT_SOURCE_DIR}/src/FragShaderSrcMaskInvertedPremultipliedAlphaBlend.frag.hlsl
   ${CMAKE_CURRENT_SOURCE_DIR}/src/FragShaderSrcMaskPremultipliedAlphaBlend.frag.hlsl
   ${CMAKE_CURRENT_SOURCE_DIR}/src/FragShaderSrcPremultipliedAlphaBlend.frag.hlsl
)

set(blend_helper_files
   ${CMAKE_CURRENT_SOURCE_DIR}/src/FragShaderSrcColorBlend.hlsl
   ${CMAKE_CURRENT_SOURCE_DIR}/src/FragShaderSrcAlphaBlend.hlsl
)

set(shared_shader_files
   ${CMAKE_CURRENT_SOURCE_DIR}/src/common.hlsl
)

set(shader_files
   ${normal_shader_files}
   ${blend_fragment_shader_files}
   ${blend_helper_files}
   ${shared_shader_files}
)

set(color_blend_modes
    Normal
    Add
    AddGlow
    Darken
    Multiply
    ColorBurn
    LinearBurn
    Lighten
    Screen
    ColorDodge
    Overlay
    SoftLight
    HardLight
    LinearLight
    Hue
    Color
)

set(Color_Blend_Mode_Normal 0)
set(Color_Blend_Mode_Add 1)
set(Color_Blend_Mode_AddGlow 2)
set(Color_Blend_Mode_Darken 3)
set(Color_Blend_Mode_Multiply 4)
set(Color_Blend_Mode_ColorBurn 5)
set(Color_Blend_Mode_LinearBurn 6)
set(Color_Blend_Mode_Lighten 7)
set(Color_Blend_Mode_Screen 8)
set(Color_Blend_Mode_ColorDodge 9)
set(Color_Blend_Mode_Overlay 10)
set(Color_Blend_Mode_SoftLight 11)
set(Color_Blend_Mode_HardLight 12)
set(Color_Blend_Mode_LinearLight 13)
set(Color_Blend_Mode_Hue 14)
set(Color_Blend_Mode_Color 15)

set(alpha_blend_modes
    Over
    Atop
    Out
    ConjointOver
    DisjointOver
)

set(Alpha_Blend_Mode_Over 0)
set(Alpha_Blend_Mode_Atop 1)
set(Alpha_Blend_Mode_Out 2)
set(Alpha_Blend_Mode_ConjointOver 3)
set(Alpha_Blend_Mode_DisjointOver 4)

if(NOT DEFINED SHADERCROSS_COMMAND)
  if(DEFINED ENV{SDL_SHADERCROSS_PATH})
    set(SHADERCROSS_COMMAND "$ENV{SDL_SHADERCROSS_PATH}/shadercross")
  else()
    message(FATAL_ERROR "SDL_SHADERCROSS_PATH environment variable is not set and SHADERCROSS_COMMAND is not defined.")
  endif()
endif()

set(output_base_dir ${CMAKE_CURRENT_BINARY_DIR}/compiledShaders)
set(spv_dir ${output_base_dir}/spv)
set(dxil_dir ${output_base_dir}/dxil)
set(msl_dir ${output_base_dir}/msl)
set(shader_include_dir ${CMAKE_CURRENT_SOURCE_DIR}/src)

foreach(shader ${normal_shader_files})
  get_filename_component(file_name ${shader} NAME)
  get_filename_component(full_path ${shader} ABSOLUTE)
  string(REGEX REPLACE "\\.vert\\.hlsl$|\\.frag\\.hlsl$" "" base_name ${file_name})
  set_source_files_properties(${shader} PROPERTIES HEADER_FILE_ONLY TRUE)

  if(file_name MATCHES "\\.vert\\.hlsl$")
    set(SHADER_STAGE "vertex")
  else()
    set(SHADER_STAGE "fragment")
  endif()

  set(spv_file ${spv_dir}/${base_name}.spv)
  set(dxil_file ${dxil_dir}/${base_name}.dxil)
  set(msl_file ${msl_dir}/${base_name}.msl)

  add_custom_command(
    OUTPUT ${spv_file}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${spv_dir}
    COMMAND ${SHADERCROSS_COMMAND} ${full_path} -s HLSL -d SPIRV -t ${SHADER_STAGE} -e main -I ${shader_include_dir} -o ${spv_file}
    DEPENDS ${full_path} ${shared_shader_files} ${SHADERCROSS_TARGET_DEPENDS}
  )

  add_custom_command(
    OUTPUT ${dxil_file}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${dxil_dir}
    COMMAND ${SHADERCROSS_COMMAND} ${full_path} -s HLSL -d DXIL -t ${SHADER_STAGE} -e main -I ${shader_include_dir} -o ${dxil_file}
    DEPENDS ${full_path} ${shared_shader_files} ${SHADERCROSS_TARGET_DEPENDS}
  )

  add_custom_command(
    OUTPUT ${msl_file}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${msl_dir}
    COMMAND ${SHADERCROSS_COMMAND} ${full_path} -s HLSL -d MSL -t ${SHADER_STAGE} -e main -I ${shader_include_dir} -o ${msl_file}
    DEPENDS ${full_path} ${shared_shader_files} ${SHADERCROSS_TARGET_DEPENDS}
  )

  list(APPEND compiled_shaders_framework_spv ${spv_file})
  list(APPEND compiled_shaders_framework_dxil ${dxil_file})
  list(APPEND compiled_shaders_framework_msl ${msl_file})
endforeach()

foreach(shader ${blend_fragment_shader_files})
  get_filename_component(file_name ${shader} NAME)
  get_filename_component(full_path ${shader} ABSOLUTE)
  set_source_files_properties(${shader} PROPERTIES HEADER_FILE_ONLY TRUE)

  foreach(color_blend_mode ${color_blend_modes})
    foreach(alpha_blend_mode ${alpha_blend_modes})
      if((color_blend_mode MATCHES "^Normal$") AND (alpha_blend_mode MATCHES "^Over$"))
        continue()
      endif()

      string(REGEX REPLACE "\\.frag\\.hlsl$" "" base_name ${file_name})
      set(output_name ${base_name}${color_blend_mode}${alpha_blend_mode})
      set(define_color_blend_mode "CSM_COLOR_BLEND_MODE=${Color_Blend_Mode_${color_blend_mode}}")
      set(define_alpha_blend_mode "CSM_ALPHA_BLEND_MODE=${Alpha_Blend_Mode_${alpha_blend_mode}}")

      set(spv_file ${spv_dir}/${output_name}.spv)
      set(dxil_file ${dxil_dir}/${output_name}.dxil)
      set(msl_file ${msl_dir}/${output_name}.msl)

      add_custom_command(
        OUTPUT ${spv_file}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${spv_dir}
        COMMAND ${SHADERCROSS_COMMAND} ${full_path} -s HLSL -d SPIRV -t fragment -e main -I ${shader_include_dir} -D${define_color_blend_mode} -D${define_alpha_blend_mode} -o ${spv_file}
        DEPENDS ${full_path} ${shared_shader_files} ${blend_helper_files} ${SHADERCROSS_TARGET_DEPENDS}
      )

      add_custom_command(
        OUTPUT ${dxil_file}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${dxil_dir}
        COMMAND ${SHADERCROSS_COMMAND} ${full_path} -s HLSL -d DXIL -t fragment -e main -I ${shader_include_dir} -D${define_color_blend_mode} -D${define_alpha_blend_mode} -o ${dxil_file}
        DEPENDS ${full_path} ${shared_shader_files} ${blend_helper_files} ${SHADERCROSS_TARGET_DEPENDS}
      )

      add_custom_command(
        OUTPUT ${msl_file}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${msl_dir}
        COMMAND ${SHADERCROSS_COMMAND} ${full_path} -s HLSL -d MSL -t fragment -e main -I ${shader_include_dir} -D${define_color_blend_mode} -D${define_alpha_blend_mode} -o ${msl_file}
        DEPENDS ${full_path} ${shared_shader_files} ${blend_helper_files} ${SHADERCROSS_TARGET_DEPENDS}
      )

      list(APPEND compiled_shaders_framework_spv ${spv_file})
      list(APPEND compiled_shaders_framework_dxil ${dxil_file})
      list(APPEND compiled_shaders_framework_msl ${msl_file})
    endforeach()
  endforeach()
endforeach()

source_group("shaders" FILES ${shader_files})

# フォーマット別のカスタムターゲット
# 利用者側で必要なフォーマットだけ add_dependencies() で選択する
add_custom_target(FrameworkShaders_SPIRV
    DEPENDS ${compiled_shaders_framework_spv}
    SOURCES ${shader_files}
)

add_custom_target(FrameworkShaders_DXIL
    DEPENDS ${compiled_shaders_framework_dxil}
    SOURCES ${shader_files}
)

add_custom_target(FrameworkShaders_MSL
    DEPENDS ${compiled_shaders_framework_msl}
    SOURCES ${shader_files}
)

# 全フォーマットをまとめてビルドするための便利ターゲット
add_custom_target(FrameworkShaders
    DEPENDS FrameworkShaders_SPIRV FrameworkShaders_DXIL FrameworkShaders_MSL
)
