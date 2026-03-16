/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "CubismRenderer_SDL3.hpp"
#include "CubismOffscreenManager_SDL3.hpp"
#include "Math/CubismMatrix44.hpp"
#include "Type/csmVector.hpp"
#include "Model/CubismModel.hpp"
#include "Rendering/csmBlendMode.hpp"

//------------ LIVE2D NAMESPACE ------------
namespace Live2D { namespace Cubism { namespace Framework { namespace Rendering {
// 各種静的変数
namespace {
SDL_GPUDevice* s_device = nullptr;
csmUint32 s_bufferSetNum = 0;

// 描画対象情報
csmUint32 s_renderWidth = 0;
csmUint32 s_renderHeight = 0;
SDL_GPUTexture* s_renderTexture = nullptr;
SDL_GPUTextureFormat s_colorFormat = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
SDL_GPUTextureFormat s_depthFormat = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

// 外部から渡されるコマンドバッファ（メインレンダーループとの共有用）
SDL_GPUCommandBuffer* s_externalCommandBuffer = nullptr;

// シェーダーフォーマット情報（バックエンド検出結果）
SDL_GPUShaderFormat s_shaderFormat = SDL_GPU_SHADERFORMAT_SPIRV;
csmString s_shaderSubDir = "spv/";
csmString s_shaderExtension = ".spv";
csmString s_shaderEntryPoint = "main";

const ModelVertex modelRenderTargetVertexArray[] = {
    {{-1.0f, -1.0f}, {0.0f, 0.0f}},
    {{ 1.0f, -1.0f}, {1.0f, 0.0f}},
    {{ 1.0f,  1.0f}, {1.0f, 1.0f}},
    {{-1.0f,  1.0f}, {0.0f, 1.0f}}
};

const csmUint16 modelRenderTargetIndexArray[] = {
    0, 1, 2,
    2, 3, 0
};
}

SDL_GPUViewport GetViewport(csmFloat32 width, csmFloat32 height, csmFloat32 minDepth, csmFloat32 maxDepth)
{
    SDL_GPUViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.w = width;
    viewport.h = height;
    viewport.min_depth = minDepth;
    viewport.max_depth = maxDepth;
    return viewport;
}

SDL_Rect GetScissor(csmFloat32 offsetX, csmFloat32 offsetY, csmFloat32 width, csmFloat32 height)
{
    SDL_Rect rect{};
    rect.x = static_cast<int>(offsetX);
    rect.y = static_cast<int>(offsetY);
    rect.w = static_cast<int>(width);
    rect.h = static_cast<int>(height);
    return rect;
}


csmInt32 GetShaderNamesBegin(const csmBlendMode blendMode)
{
#define CSM_GET_SHADER_NAME(COLOR, ALPHA) ShaderNames_ ## COLOR ## ALPHA

#define CSM_SWITCH_ALPHA_BLEND(COLOR) {\
    switch (blendMode.GetAlphaBlendType()) \
    { \
    case Core::csmAlphaBlendType_Over: \
    default: \
        return CSM_GET_SHADER_NAME(COLOR, Over); \
    case Core::csmAlphaBlendType_Atop: \
        return CSM_GET_SHADER_NAME(COLOR, Atop); \
    case Core::csmAlphaBlendType_Out: \
        return CSM_GET_SHADER_NAME(COLOR, Out); \
    case Core::csmAlphaBlendType_ConjointOver: \
        return CSM_GET_SHADER_NAME(COLOR, ConjointOver); \
    case Core::csmAlphaBlendType_DisjointOver: \
        return CSM_GET_SHADER_NAME(COLOR, DisjointOver); \
    } \
}

    switch (blendMode.GetColorBlendType())
    {
    case Core::csmColorBlendType_Normal:
    default:
    {
        // Normal Over　のときは5.2以前の描画方法を利用する
        switch (blendMode.GetAlphaBlendType())
        {
        case Core::csmAlphaBlendType_Over:
        default:
            return ShaderNames_Normal;
        case Core::csmAlphaBlendType_Atop:
            return CSM_GET_SHADER_NAME(Normal, Atop);
        case Core::csmAlphaBlendType_Out:
            return CSM_GET_SHADER_NAME(Normal, Out);
        case Core::csmAlphaBlendType_ConjointOver:
            return CSM_GET_SHADER_NAME(Normal, ConjointOver);
        case Core::csmAlphaBlendType_DisjointOver:
            return CSM_GET_SHADER_NAME(Normal, DisjointOver);
        }
    }
    break;
    case Core::csmColorBlendType_AddCompatible:
        // AddCompatible は5.2以前の描画方法を利用する
        return ShaderNames_Add;
    case Core::csmColorBlendType_MultiplyCompatible:
        // MultCompatible は5.2以前の描画方法を利用する
        return ShaderNames_Mult;
    case Core::csmColorBlendType_Add:
        CSM_SWITCH_ALPHA_BLEND(Add);
        break;
    case Core::csmColorBlendType_AddGlow:
        CSM_SWITCH_ALPHA_BLEND(AddGlow);
        break;
    case Core::csmColorBlendType_Darken:
        CSM_SWITCH_ALPHA_BLEND(Darken);
        break;
    case Core::csmColorBlendType_Multiply:
        CSM_SWITCH_ALPHA_BLEND(Multiply);
        break;
    case Core::csmColorBlendType_ColorBurn:
        CSM_SWITCH_ALPHA_BLEND(ColorBurn);
        break;
    case Core::csmColorBlendType_LinearBurn:
        CSM_SWITCH_ALPHA_BLEND(LinearBurn);
        break;
    case Core::csmColorBlendType_Lighten:
        CSM_SWITCH_ALPHA_BLEND(Lighten);
        break;
    case Core::csmColorBlendType_Screen:
        CSM_SWITCH_ALPHA_BLEND(Screen);
        break;
    case Core::csmColorBlendType_ColorDodge:
        CSM_SWITCH_ALPHA_BLEND(ColorDodge);
        break;
    case Core::csmColorBlendType_Overlay:
        CSM_SWITCH_ALPHA_BLEND(Overlay);
        break;
    case Core::csmColorBlendType_SoftLight:
        CSM_SWITCH_ALPHA_BLEND(SoftLight);
        break;
    case Core::csmColorBlendType_HardLight:
        CSM_SWITCH_ALPHA_BLEND(HardLight);
        break;
    case Core::csmColorBlendType_LinearLight:
        CSM_SWITCH_ALPHA_BLEND(LinearLight);
        break;
    case Core::csmColorBlendType_Hue:
        CSM_SWITCH_ALPHA_BLEND(Hue);
        break;
    case Core::csmColorBlendType_Color:
        CSM_SWITCH_ALPHA_BLEND(Color);
        break;
    }

#undef CSM_SWITCH_ALPHA_BLEND
#undef CSM_GET_SHADER_NAME
}

/*********************************************************************************************************************
*                                      CubismClippingManager_SDL3
********************************************************************************************************************/

void CubismClippingManager_SDL3::SetupClippingContext(CubismModel& model, SDL_GPUCommandBuffer* commandBuffer,
                                                       CubismRenderer_SDL3* renderer, csmInt32 commandBufferCurrent,
                                                       CubismRenderer::DrawableObjectType drawableObjectType)
{
    // 全てのクリッピングを用意する
    // 同じクリップ（複数の場合はまとめて１つのクリップ）を使う場合は１度だけ設定する
    csmInt32 usingClipCount = 0;

    for (csmUint32 clipIndex = 0; clipIndex < _clippingContextListForMask.GetSize(); clipIndex++)
    {
        // １つのクリッピングマスクに関して
        CubismClippingContext_SDL3* cc = _clippingContextListForMask[clipIndex];

        // このクリップを利用する描画オブジェクト群全体を囲む矩形を計算
        CalcClippedTotalBounds(model, cc, drawableObjectType);

        if (cc->_isUsing)
        {
            usingClipCount++; //使用中としてカウント
        }
    }

    if (usingClipCount <= 0)
    {
        return;
    }

    // マスク作成処理
    // 後の計算のためにインデックスの最初をセット
    switch (drawableObjectType)
    {
    case CubismRenderer::DrawableObjectType_Drawable:
        _currentMaskBuffer = renderer->GetDrawableMaskBuffer(commandBufferCurrent, 0);
        break;
    case CubismRenderer::DrawableObjectType_Offscreen:
        _currentMaskBuffer = renderer->GetOffscreenMaskBuffer(commandBufferCurrent, 0);
        break;
    }

    if (_currentMaskBuffer == nullptr)
    {
        return;
    }

    // 1が無効（描かれない）領域、0が有効（描かれる）領域。（シェーダで Cd*Csで0に近い値をかけてマスクを作る。1をかけると何も起こらない）
    SDL_GPURenderPass* maskRenderPass = _currentMaskBuffer->BeginDraw(commandBuffer, 1.0f, 1.0f, 1.0f, 1.0f, true);
    if (maskRenderPass == nullptr)
    {
        return;
    }

    // 生成したFrameBufferと同じサイズでビューポートを設定
    SDL_GPUViewport viewport = GetViewport(
        static_cast<csmFloat32>(_clippingMaskBufferSize.X),
        static_cast<csmFloat32>(_clippingMaskBufferSize.Y),
        0.0f, 1.0f
    );
    SDL_SetGPUViewport(maskRenderPass, &viewport);
    SDL_Rect rect = GetScissor(
        0.0f, 0.0f,
        static_cast<csmFloat32>(_clippingMaskBufferSize.X),
        static_cast<csmFloat32>(_clippingMaskBufferSize.Y)
    );
    SDL_SetGPUScissor(maskRenderPass, &rect);

    // 各マスクのレイアウトを決定していく
    SetupLayoutBounds(usingClipCount);

    // サイズがレンダーテクスチャの枚数と合わない場合は合わせる
    if (_clearedMaskBufferFlags.GetSize() != _renderTextureCount)
    {
        _clearedMaskBufferFlags.Clear();

        for (csmInt32 i = 0; i < _renderTextureCount; ++i)
        {
            _clearedMaskBufferFlags.PushBack(false);
        }
    }
    else
    {
        // マスクのクリアフラグを毎フレーム開始時に初期化
        for (csmInt32 i = 0; i < _renderTextureCount; ++i)
        {
            _clearedMaskBufferFlags[i] = false;
        }
    }

    // 実際にマスクを生成する
    // 全てのマスクをどの様にレイアウトして描くかを決定し、ClipContext , ClippedDrawContext に記憶する
    for (csmUint32 clipIndex = 0; clipIndex < _clippingContextListForMask.GetSize(); clipIndex++)
    {
        // --- 実際に１つのマスクを描く ---
        CubismClippingContext_SDL3* clipContext = _clippingContextListForMask[clipIndex];
        csmRectF* allClippedDrawRect = clipContext->_allClippedDrawRect; //このマスクを使う、全ての描画オブジェクトの論理座標上の囲み矩形
        csmRectF* layoutBoundsOnTex01 = clipContext->_layoutBounds; //この中にマスクを収める
        const csmFloat32 MARGIN = 0.05f;
        CubismRenderTarget_SDL3* maskBuffer = NULL;
        // clipContextに設定したオフスクリーンサーフェイスをインデックスで取得
        switch (drawableObjectType)
        {
        case CubismRenderer::DrawableObjectType_Drawable:
            maskBuffer = renderer->GetDrawableMaskBuffer(commandBufferCurrent, clipContext->_bufferIndex);
            break;
        case CubismRenderer::DrawableObjectType_Offscreen:
            maskBuffer = renderer->GetOffscreenMaskBuffer(commandBufferCurrent, clipContext->_bufferIndex);
            break;
        }

        if (maskBuffer == nullptr)
        {
            continue;
        }

        // 現在のレンダーターゲットがclipContextのものと異なる場合
        if (_currentMaskBuffer != maskBuffer)
        {
            _currentMaskBuffer->EndDraw(maskRenderPass);
            _currentMaskBuffer = maskBuffer;
            // マスク用RenderTextureをactiveにセット
            maskRenderPass = _currentMaskBuffer->BeginDraw(commandBuffer, 1.0f, 1.0f, 1.0f, 1.0f, true);
            if (maskRenderPass == nullptr)
            {
                renderer->SetClippingContextBufferForMask(NULL);
                return;
            }
            SDL_SetGPUViewport(maskRenderPass, &viewport);
            SDL_SetGPUScissor(maskRenderPass, &rect);
        }

        // モデル座標上の矩形を、適宜マージンを付けて使う
        _tmpBoundsOnModel.SetRect(allClippedDrawRect);
        _tmpBoundsOnModel.Expand(allClippedDrawRect->Width * MARGIN, allClippedDrawRect->Height * MARGIN);
        //########## 本来は割り当てられた領域の全体を使わず必要最低限のサイズがよい
        // シェーダ用の計算式を求める。回転を考慮しない場合は以下のとおり
        // movePeriod' = movePeriod * scaleX + offX     [[ movePeriod' = (movePeriod - tmpBoundsOnModel.movePeriod)*scale + layoutBoundsOnTex01.movePeriod ]]
        csmFloat32 scaleX = layoutBoundsOnTex01->Width / _tmpBoundsOnModel.Width;
        csmFloat32 scaleY = layoutBoundsOnTex01->Height / _tmpBoundsOnModel.Height;

        // マスク生成時に使う行列を求める
        CreateMatrixForMask(false, layoutBoundsOnTex01, scaleX, scaleY);

        clipContext->_matrixForMask.SetMatrix(_tmpMatrixForMask.GetArray());
        clipContext->_matrixForDraw.SetMatrix(_tmpMatrixForDraw.GetArray());

        // 実際の描画を行う
        const csmInt32 clipDrawCount = clipContext->_clippingIdCount;
        for (csmInt32 i = 0; i < clipDrawCount; i++)
        {
            const csmInt32 clipDrawIndex = clipContext->_clippingIdList[i];

            // 頂点情報が更新されておらず、信頼性がない場合は描画をパスする
            if (!model.GetDrawableDynamicFlagVertexPositionsDidChange(clipDrawIndex))
            {
                continue;
            }

            renderer->IsCulling(model.GetDrawableCulling(clipDrawIndex) != 0);

            // レンダーパス開始時にマスクはクリアされているのでクリアする必要なし
            // 今回専用の変換を適用して描く
            // チャンネルも切り替える必要がある(A,R,G,B)
            renderer->SetClippingContextBufferForMask(clipContext);
            renderer->ExecuteDrawForMask(model, clipDrawIndex, maskRenderPass);
        }
    }
    // --- 後処理 ---
    _currentMaskBuffer->EndDraw(maskRenderPass);
    renderer->SetClippingContextBufferForMask(NULL);
}

/*********************************************************************************************************************
*                                      CubismClippingContext_SDL3
********************************************************************************************************************/

CubismClippingContext_SDL3::CubismClippingContext_SDL3(
    CubismClippingManager<CubismClippingContext_SDL3, CubismRenderTarget_SDL3>* manager, CubismModel& model,
    const csmInt32* clippingDrawableIndices, csmInt32 clipCount)
    : CubismClippingContext(clippingDrawableIndices, clipCount)
{
    _isUsing = false;
    _owner = manager;
}

CubismClippingContext_SDL3::~CubismClippingContext_SDL3()
{}

CubismClippingManager<CubismClippingContext_SDL3, CubismRenderTarget_SDL3>*
CubismClippingContext_SDL3::GetClippingManager()
{
    return _owner;
}

/*********************************************************************************************************************
*                                      CubismPipeline_SDL3
********************************************************************************************************************/

namespace {
const csmInt32 ShaderCount = ShaderNames_ShaderCount;
CubismPipeline_SDL3* s_pipelineManager = nullptr;
}

CubismPipeline_SDL3::CubismPipeline_SDL3()
{}

CubismPipeline_SDL3::~CubismPipeline_SDL3()
{
    ReleaseShaderProgram(s_device);
}

SDL_GPUShader* CubismPipeline_SDL3::PipelineResource::LoadShader(SDL_GPUDevice* device, csmString filename,
                                                                  SDL_GPUShaderStage stage,
                                                                  csmUint32 numSamplers, csmUint32 numUniformBuffers)
{
    csmLoadFileFunction fileLoader = CubismFramework::GetLoadFileFunction();
    csmReleaseBytesFunction bytesReleaser = CubismFramework::GetReleaseBytesFunction();

    if (!fileLoader)
    {
        CubismLogError("File loader is not set.");
        return nullptr;
    }

    if (!bytesReleaser)
    {
        CubismLogError("Byte releaser is not set.");
        return nullptr;
    }

    csmSizeInt fileSize;
    csmByte* fileData = fileLoader(filename.GetRawString(), &fileSize);

    if (!fileData)
    {
        CubismLogError("failed to open shader file: %s", filename.GetRawString());
        return nullptr;
    }

    SDL_GPUShaderCreateInfo shaderInfo = {};
    shaderInfo.code = reinterpret_cast<const Uint8*>(fileData);
    shaderInfo.code_size = static_cast<size_t>(fileSize);
    shaderInfo.entrypoint = s_shaderEntryPoint.GetRawString();
    shaderInfo.format = s_shaderFormat;
    shaderInfo.stage = stage;
    shaderInfo.num_samplers = numSamplers;
    shaderInfo.num_uniform_buffers = numUniformBuffers;
    shaderInfo.num_storage_buffers = 0;
    shaderInfo.num_storage_textures = 0;
    shaderInfo.props = 0;

    SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shaderInfo);

    // ファイル読み込みで確保したバイト列を解放
    bytesReleaser(fileData);

    if (shader == nullptr)
    {
        CubismLogError("Failed to create shader: %s", SDL_GetError());
    }
    else
    {
        CubismLogInfo("Create shader: %s", filename.GetRawString());
    }

    return shader;
}

csmBool CubismPipeline_SDL3::PipelineResource::CreateGraphicsPipeline(SDL_GPUDevice* device,
                                                                       csmString vertFileName, csmString fragFileName,
                                                                       SDL_GPUTextureFormat colorTargetFormat,
                                                                       csmUint32 shaderName,
                                                                       csmInt32 colorBlendMode, csmInt32 alphaBlendMode)
{
    // 頂点シェーダー: 1 uniform buffer, 0 samplers
    SDL_GPUShader* vertShader = LoadShader(device, vertFileName, SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);
    if (vertShader == nullptr)
    {
        return false;
    }
    else
    {
        CubismLogDebug("Load vertex shader: %s", vertFileName.GetRawString());
    }

    // フラグメントシェーダー: 0-1 uniform buffer, 1-3 samplers
    csmUint32 fragSamplers = (shaderName == ShaderNames_Copy) ? 1 : 3;
    csmUint32 fragUniforms = 1;
    SDL_GPUShader* fragShader = LoadShader(device, fragFileName, SDL_GPU_SHADERSTAGE_FRAGMENT, fragSamplers, fragUniforms);
    if (fragShader == nullptr)
    {
        SDL_ReleaseGPUShader(device, vertShader);
        return false;
    }
    else
    {
        CubismLogDebug("Load fragShader shader: %s", fragFileName.GetRawString());
    }

    // 頂点入力の設定
    SDL_GPUVertexBufferDescription vertexBufferDesc;
    ModelVertex::GetVertexBufferDescription(&vertexBufferDesc);

    SDL_GPUVertexAttribute attributeDescs[2];
    ModelVertex::GetVertexAttributes(attributeDescs);

    SDL_GPUVertexInputState vertexInputState = {};
    vertexInputState.vertex_buffer_descriptions = &vertexBufferDesc;
    vertexInputState.num_vertex_buffers = 1;
    vertexInputState.vertex_attributes = attributeDescs;
    vertexInputState.num_vertex_attributes = 2;

    // ラスタライザステートの設定
    SDL_GPURasterizerState rasterizerState = {};
    rasterizerState.fill_mode = SDL_GPU_FILLMODE_FILL;
    rasterizerState.cull_mode = SDL_GPU_CULLMODE_NONE;
    rasterizerState.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    rasterizerState.depth_bias_constant_factor = 0.0f;
    rasterizerState.depth_bias_clamp = 0.0f;
    rasterizerState.depth_bias_slope_factor = 0.0f;
    rasterizerState.enable_depth_bias = false;
    rasterizerState.enable_depth_clip = true;

    // 深度ステンシルステートの設定
    SDL_GPUDepthStencilState depthStencilState = {};
    depthStencilState.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
    depthStencilState.back_stencil_state.fail_op = SDL_GPU_STENCILOP_KEEP;
    depthStencilState.back_stencil_state.pass_op = SDL_GPU_STENCILOP_KEEP;
    depthStencilState.back_stencil_state.depth_fail_op = SDL_GPU_STENCILOP_KEEP;
    depthStencilState.back_stencil_state.compare_op = SDL_GPU_COMPAREOP_ALWAYS;
    depthStencilState.front_stencil_state = depthStencilState.back_stencil_state;
    depthStencilState.compare_mask = 0xFF;
    depthStencilState.write_mask = 0xFF;
    depthStencilState.enable_depth_test = true;
    depthStencilState.enable_depth_write = true;
    depthStencilState.enable_stencil_test = false;

    // マルチサンプルステートの設定
    SDL_GPUMultisampleState multisampleState = {};
    multisampleState.sample_count = SDL_GPU_SAMPLECOUNT_1;
    multisampleState.sample_mask = 0;
    multisampleState.enable_mask = false;

    // プリミティブタイプ
    SDL_GPUPrimitiveType primitiveType = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

    // ブレンドステートの設定
    auto createBlendState = [](SDL_GPUBlendFactor srcColor, SDL_GPUBlendFactor dstColor,
                               SDL_GPUBlendFactor srcAlpha, SDL_GPUBlendFactor dstAlpha) -> SDL_GPUColorTargetBlendState
    {
        SDL_GPUColorTargetBlendState blendState = {};
        blendState.src_color_blendfactor = srcColor;
        blendState.dst_color_blendfactor = dstColor;
        blendState.color_blend_op = SDL_GPU_BLENDOP_ADD;
        blendState.src_alpha_blendfactor = srcAlpha;
        blendState.dst_alpha_blendfactor = dstAlpha;
        blendState.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
        blendState.color_write_mask = SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G |
                                      SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A;
        blendState.enable_blend = true;
        blendState.enable_color_write_mask = false;
        return blendState;
    };

    if (shaderName == ShaderNames_Copy || (colorBlendMode != ColorBlendMode_None && alphaBlendMode != AlphaBlendMode_None))
    {
        // 5.3以降
        _pipeline.Resize(1);

        SDL_GPUColorTargetBlendState blendState;
        if (shaderName == ShaderNames_Copy)
        {
            blendState = createBlendState(SDL_GPU_BLENDFACTOR_ONE, SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                                          SDL_GPU_BLENDFACTOR_ONE, SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA);
        }
        else
        {
            blendState = createBlendState(SDL_GPU_BLENDFACTOR_ONE, SDL_GPU_BLENDFACTOR_ZERO,
                                          SDL_GPU_BLENDFACTOR_ONE, SDL_GPU_BLENDFACTOR_ZERO);
        }

        SDL_GPUColorTargetDescription colorTargetDesc = {};
        colorTargetDesc.format = colorTargetFormat;
        colorTargetDesc.blend_state = blendState;

        SDL_GPUGraphicsPipelineTargetInfo targetInfo = {};
        targetInfo.color_target_descriptions = &colorTargetDesc;
        targetInfo.num_color_targets = 1;
        targetInfo.depth_stencil_format = s_depthFormat;
        targetInfo.has_depth_stencil_target = true;

        SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.vertex_shader = vertShader;
        pipelineInfo.fragment_shader = fragShader;
        pipelineInfo.vertex_input_state = vertexInputState;
        pipelineInfo.primitive_type = primitiveType;
        pipelineInfo.rasterizer_state = rasterizerState;
        pipelineInfo.multisample_state = multisampleState;
        pipelineInfo.depth_stencil_state = depthStencilState;
        pipelineInfo.target_info = targetInfo;
        pipelineInfo.props = 0;

        _pipeline[0] = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);
        if (_pipeline[0] == nullptr)
        {
            CubismLogError("Failed to create graphics pipeline: %s", SDL_GetError());
        }
    }
    else
    {
        // 5.2以前 (Normal, Add, Mult, Mask用)
        _pipeline.Resize(4);

        // ブレンドモード別のブレンドステート
        SDL_GPUColorTargetBlendState blendStates[4];
        // 通常
        blendStates[Blend_Normal] = createBlendState(SDL_GPU_BLENDFACTOR_ONE, SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                                                     SDL_GPU_BLENDFACTOR_ONE, SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA);
        // 加算
        blendStates[Blend_Add] = createBlendState(SDL_GPU_BLENDFACTOR_ONE, SDL_GPU_BLENDFACTOR_ONE,
                                                  SDL_GPU_BLENDFACTOR_ZERO, SDL_GPU_BLENDFACTOR_ONE);
        // 乗算
        blendStates[Blend_Mult] = createBlendState(SDL_GPU_BLENDFACTOR_DST_COLOR, SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                                                   SDL_GPU_BLENDFACTOR_ZERO, SDL_GPU_BLENDFACTOR_ONE);
        // マスク
        blendStates[Blend_Mask] = createBlendState(SDL_GPU_BLENDFACTOR_ZERO, SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_COLOR,
                                                   SDL_GPU_BLENDFACTOR_ZERO, SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA);

        for (csmInt32 i = 0; i < 4; ++i)
        {
            SDL_GPUColorTargetDescription colorTargetDesc = {};
            colorTargetDesc.format = colorTargetFormat;
            colorTargetDesc.blend_state = blendStates[i];

            SDL_GPUGraphicsPipelineTargetInfo targetInfo = {};
            targetInfo.color_target_descriptions = &colorTargetDesc;
            targetInfo.num_color_targets = 1;
            targetInfo.depth_stencil_format = s_depthFormat;
            targetInfo.has_depth_stencil_target = true;

            SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
            pipelineInfo.vertex_shader = vertShader;
            pipelineInfo.fragment_shader = fragShader;
            pipelineInfo.vertex_input_state = vertexInputState;
            pipelineInfo.primitive_type = primitiveType;
            pipelineInfo.rasterizer_state = rasterizerState;
            pipelineInfo.multisample_state = multisampleState;
            pipelineInfo.depth_stencil_state = depthStencilState;
            pipelineInfo.target_info = targetInfo;
            pipelineInfo.props = 0;

            _pipeline[i] = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);
            if (_pipeline[i] == nullptr)
            {
                CubismLogError("Failed to create graphics pipeline [%d]: %s", i, SDL_GetError());
            }
        }
    }

    SDL_ReleaseGPUShader(device, vertShader);
    SDL_ReleaseGPUShader(device, fragShader);

    return true;
}

void CubismPipeline_SDL3::PipelineResource::Release(SDL_GPUDevice* device)
{
    for (csmInt32 i = 0; i < _pipeline.GetSize(); i++)
    {
        if (_pipeline[i] != nullptr)
        {
            SDL_ReleaseGPUGraphicsPipeline(device, _pipeline[i]);
            _pipeline[i] = nullptr;
        }
    }
    _pipeline.Clear();
}

void CubismPipeline_SDL3::CreatePipelines(SDL_GPUDevice* device, SDL_GPUTextureFormat colorTargetFormat)
{
    //パイプラインを作成済みの場合は生成する必要なし
    if (_pipelineResource.GetSize() != 0)
    {
        return;
    }

    // バックエンドに応じたシェーダーパスヘルパー
    csmString shaderDir = csmString("FrameworkShaders/") + s_shaderSubDir;
    csmString ext = s_shaderExtension;

    _pipelineResource.Resize(ShaderCount);
    for (csmInt32 i = 0; i < ShaderCount; i++)
    {
        // Add/Mult スロットは Normal と同じリソースを共有するため、ここでは確保しない
        if ((i >= ShaderNames_Add && i <= ShaderNames_AddMaskedInvertedPremultipliedAlpha) ||
            (i >= ShaderNames_Mult && i <= ShaderNames_MultMaskedInvertedPremultipliedAlpha))
        {
            _pipelineResource[i] = nullptr;
        }
        else
        {
            _pipelineResource[i] = CSM_NEW PipelineResource();
        }
    }

    CreatePipelineResource(device, ShaderNames_Copy, shaderDir + "VertShaderSrcCopy" + ext, shaderDir + "FragShaderSrcCopy" + ext, colorTargetFormat);
    CreatePipelineResource(device, ShaderNames_SetupMask, shaderDir + "VertShaderSrcSetupMask" + ext, shaderDir + "FragShaderSrcSetupMask" + ext, colorTargetFormat);

    // 通常
    CreatePipelineResource(device, ShaderNames_Normal, shaderDir + "VertShaderSrc" + ext, shaderDir + "FragShaderSrc" + ext, colorTargetFormat);
    CreatePipelineResource(device, ShaderNames_NormalMasked, shaderDir + "VertShaderSrcMasked" + ext, shaderDir + "FragShaderSrcMask" + ext, colorTargetFormat);
    CreatePipelineResource(device, ShaderNames_NormalMaskedInverted, shaderDir + "VertShaderSrcMasked" + ext, shaderDir + "FragShaderSrcMaskInverted" + ext, colorTargetFormat);
    CreatePipelineResource(device, ShaderNames_NormalPremultipliedAlpha, shaderDir + "VertShaderSrc" + ext, shaderDir + "FragShaderSrcPremultipliedAlpha" + ext, colorTargetFormat);
    CreatePipelineResource(device, ShaderNames_NormalMaskedPremultipliedAlpha, shaderDir + "VertShaderSrcMasked" + ext, shaderDir + "FragShaderSrcMaskPremultipliedAlpha" + ext, colorTargetFormat);
    CreatePipelineResource(device, ShaderNames_NormalMaskedInvertedPremultipliedAlpha, shaderDir + "VertShaderSrcMasked" + ext, shaderDir + "FragShaderSrcMaskInvertedPremultipliedAlpha" + ext, colorTargetFormat);

    // 加算（通常と同じリソースを共有）
    _pipelineResource[ShaderNames_Add] = _pipelineResource[ShaderNames_Normal];
    _pipelineResource[ShaderNames_AddMasked] = _pipelineResource[ShaderNames_NormalMasked];
    _pipelineResource[ShaderNames_AddMaskedInverted] = _pipelineResource[ShaderNames_NormalMaskedInverted];
    _pipelineResource[ShaderNames_AddPremultipliedAlpha] = _pipelineResource[ShaderNames_NormalPremultipliedAlpha];
    _pipelineResource[ShaderNames_AddMaskedPremultipliedAlpha] = _pipelineResource[ShaderNames_NormalMaskedPremultipliedAlpha];
    _pipelineResource[ShaderNames_AddMaskedInvertedPremultipliedAlpha] = _pipelineResource[ShaderNames_NormalMaskedInvertedPremultipliedAlpha];

    // 乗算（通常と同じリソースを共有）
    _pipelineResource[ShaderNames_Mult] = _pipelineResource[ShaderNames_Normal];
    _pipelineResource[ShaderNames_MultMasked] = _pipelineResource[ShaderNames_NormalMasked];
    _pipelineResource[ShaderNames_MultMaskedInverted] = _pipelineResource[ShaderNames_NormalMaskedInverted];
    _pipelineResource[ShaderNames_MultPremultipliedAlpha] = _pipelineResource[ShaderNames_NormalPremultipliedAlpha];
    _pipelineResource[ShaderNames_MultMaskedPremultipliedAlpha] = _pipelineResource[ShaderNames_NormalMaskedPremultipliedAlpha];
    _pipelineResource[ShaderNames_MultMaskedInvertedPremultipliedAlpha] = _pipelineResource[ShaderNames_NormalMaskedInvertedPremultipliedAlpha];

    // ブレンドモードの組み合わせ分作成
    {
        csmUint32 offset = ShaderNames_NormalAtop;
        for (csmInt32 i = 0; i <= Core::csmColorBlendType_Color; ++i)
        {
            if (i == Core::csmColorBlendType_AddCompatible || i == Core::csmColorBlendType_MultiplyCompatible)
            {
                continue;
            }

            // Normal Overはシェーダを作る必要がないため 1 から始める
            const csmInt32 start = (i == 0 ? 1 : 0);
            for (csmInt32 j = start; j <= Core::csmAlphaBlendType_DisjointOver; ++j)
            {
                csmString colorBlendModeName = csmBlendMode::ColorBlendModeToString(i);
                csmString alphaBlendModeName = csmBlendMode::AlphaBlendModeToString(j);

                csmString baseFragShaderFileName = "FragShaderSrcBlend";
                csmString fragShaderFileName = shaderDir + baseFragShaderFileName + colorBlendModeName + alphaBlendModeName + ext;
                CreatePipelineResource(device, offset++, shaderDir + "VertShaderSrcBlend" + ext, fragShaderFileName, colorTargetFormat, i, j);

                baseFragShaderFileName = "FragShaderSrcMaskBlend";
                fragShaderFileName = shaderDir + baseFragShaderFileName + colorBlendModeName + alphaBlendModeName + ext;
                CreatePipelineResource(device, offset++, shaderDir + "VertShaderSrcMaskedBlend" + ext, fragShaderFileName, colorTargetFormat, i, j);

                baseFragShaderFileName = "FragShaderSrcMaskInvertedBlend";
                fragShaderFileName = shaderDir + baseFragShaderFileName + colorBlendModeName + alphaBlendModeName + ext;
                CreatePipelineResource(device, offset++, shaderDir + "VertShaderSrcMaskedBlend" + ext, fragShaderFileName, colorTargetFormat, i, j);

                baseFragShaderFileName = "FragShaderSrcPremultipliedAlphaBlend";
                fragShaderFileName = shaderDir + baseFragShaderFileName + colorBlendModeName + alphaBlendModeName + ext;
                CreatePipelineResource(device, offset++, shaderDir + "VertShaderSrcBlend" + ext, fragShaderFileName, colorTargetFormat, i, j);

                baseFragShaderFileName = "FragShaderSrcMaskPremultipliedAlphaBlend";
                fragShaderFileName = shaderDir + baseFragShaderFileName + colorBlendModeName + alphaBlendModeName + ext;
                CreatePipelineResource(device, offset++, shaderDir + "VertShaderSrcMaskedBlend" + ext, fragShaderFileName, colorTargetFormat, i, j);

                baseFragShaderFileName = "FragShaderSrcMaskInvertedPremultipliedAlphaBlend";
                fragShaderFileName = shaderDir + baseFragShaderFileName + colorBlendModeName + alphaBlendModeName + ext;
                CreatePipelineResource(device, offset++, shaderDir + "VertShaderSrcMaskedBlend" + ext, fragShaderFileName, colorTargetFormat, i, j);
            }
        }
    }
}

void CubismPipeline_SDL3::CreatePipelineResource(SDL_GPUDevice* device,
                                                  csmUint32 const& shaderName,
                                                  csmString const& vertShaderFileName,
                                                  csmString const& fragShaderFileName,
                                                  SDL_GPUTextureFormat const& colorTargetFormat,
                                                  csmInt32 const& colorBlendMode, csmInt32 const& alphaBlendMode)
{
    if (!_pipelineResource[shaderName]->CreateGraphicsPipeline(device, vertShaderFileName, fragShaderFileName,
                                                                colorTargetFormat, shaderName, colorBlendMode, alphaBlendMode))
    {
        CSM_DELETE(_pipelineResource[shaderName]);
        _pipelineResource[shaderName] = nullptr;
        CubismLogError("failed to create shader %d", shaderName);
    }
}

CubismPipeline_SDL3* CubismPipeline_SDL3::GetInstance()
{
    if (s_pipelineManager == nullptr)
    {
        s_pipelineManager = CSM_NEW CubismPipeline_SDL3();
    }
    return s_pipelineManager;
}

void CubismPipeline_SDL3::ReleaseShaderProgram(SDL_GPUDevice* device)
{
    for (csmInt32 i = 0; i < _pipelineResource.GetSize(); i++)
    {
        if (i >= ShaderNames_Add && i <= ShaderNames_MultMaskedInvertedPremultipliedAlpha)
        {
            // 加算と乗算は通常と同じリソースを参照しており2重解放になってしまうためスキップ
            continue;
        }

        if (_pipelineResource[i] != nullptr)
        {
            _pipelineResource[i]->Release(device);
            CSM_DELETE(_pipelineResource[i]);
            _pipelineResource[i] = nullptr;
        }
    }
}

/*********************************************************************************************************************
*                                       CubismRenderer_SDL3
********************************************************************************************************************/

CubismRenderer* CubismRenderer::Create(csmUint32 width, csmUint32 height)
{
    return CSM_NEW CubismRenderer_SDL3(width, height);
}

void CubismRenderer::StaticRelease()
{
    CubismRenderer_SDL3::DoStaticRelease();
}

CubismRenderer_SDL3::CubismRenderer_SDL3(csmUint32 width, csmUint32 height)
    : CubismRenderer(width, height)
    , _drawableClippingManager(nullptr)
    , _clippingContextBufferForMask(nullptr)
    , _clippingContextBufferForDraw(nullptr)
    , _offscreenClippingManager(nullptr)
    , _clippingContextBufferForOffscreen(nullptr)
    , _currentOffscreen(nullptr)
    , _currentRenderTarget(nullptr)
    , _clearColor()
    , _commandBufferCurrent(0)
    , _isClearedModelRenderTarget(false)
{
    _clearColor.r = 0.0f;
    _clearColor.g = 0.0f;
    _clearColor.b = 0.0f;
    _clearColor.a = 0.0f;
}

CubismRenderer_SDL3::~CubismRenderer_SDL3()
{
    CSM_DELETE_SELF(CubismClippingManager_SDL3, _drawableClippingManager);
    CSM_DELETE_SELF(CubismClippingManager_SDL3, _offscreenClippingManager);

    // オフスクリーンを作成していたのなら開放
    for (csmInt32 i = 0; i < _modelRenderTargets.GetSize(); i++)
    {
        if (_modelRenderTargets[i].IsValid())
        {
            _modelRenderTargets[i].DestroyRenderTarget();
        }
    }
    _modelRenderTargets.Clear();

    for (csmUint32 buffer = 0; buffer < _drawableMaskBuffers.GetSize(); buffer++)
    {
        for (csmInt32 i = 0; i < _drawableMaskBuffers[buffer].GetSize(); i++)
        {
            if (_drawableMaskBuffers[buffer][i].IsValid())
            {
                _drawableMaskBuffers[buffer][i].DestroyRenderTarget();
            }
        }
        _drawableMaskBuffers[buffer].Clear();
    }
    _drawableMaskBuffers.Clear();

    // オフスクリーン用マスクバッファを開放
    for (csmUint32 buffer = 0; buffer < _offscreenMaskBuffers.GetSize(); buffer++)
    {
        for (csmInt32 i = 0; i < _offscreenMaskBuffers[buffer].GetSize(); i++)
        {
            if (_offscreenMaskBuffers[buffer][i].IsValid())
            {
                _offscreenMaskBuffers[buffer][i].DestroyRenderTarget();
            }
        }
        _offscreenMaskBuffers[buffer].Clear();
    }
    _offscreenMaskBuffers.Clear();

    _depthImage.Destroy(s_device);
    _blendReadImage.Destroy(s_device);

    // その他バッファ開放
    for (csmUint32 buffer = 0; buffer < s_bufferSetNum; buffer++)
    {
        for (csmUint32 drawAssign = 0; drawAssign < _vertexBuffers[buffer].GetSize(); drawAssign++)
        {
            _vertexBuffers[buffer][drawAssign].Destroy(s_device);
            _stagingBuffers[buffer][drawAssign].Destroy(s_device);
            _indexBuffers[buffer][drawAssign].Destroy(s_device);
        }

        for (csmUint32 offscreenAssign = 0; offscreenAssign < _offscreenVertexBuffers[buffer].GetSize(); ++offscreenAssign)
        {
            _offscreenVertexBuffers[buffer][offscreenAssign].Destroy(s_device);
            _offscreenStagingBuffers[buffer][offscreenAssign].Destroy(s_device);
            _offscreenIndexBuffers[buffer][offscreenAssign].Destroy(s_device);
        }

        _copyVertexBuffer[buffer].Destroy(s_device);
        _copyStagingBuffer[buffer].Destroy(s_device);
        _copyIndexBuffer[buffer].Destroy(s_device);
    }
}

void CubismRenderer_SDL3::DoStaticRelease()
{
    if (s_pipelineManager)
    {
        CSM_DELETE_SELF(CubismPipeline_SDL3, s_pipelineManager);
        s_pipelineManager = nullptr;
    }
}

void CubismRenderer_SDL3::InitializeConstantSettings(SDL_GPUDevice* device, csmUint32 swapchainImageCount,
                                                      csmUint32 width, csmUint32 height,
                                                      SDL_GPUTextureFormat colorTargetFormat,
                                                      SDL_GPUTextureFormat depthTargetFormat)
{
    s_device = device;
    s_bufferSetNum = swapchainImageCount;
    s_renderWidth = width;
    s_renderHeight = height;
    s_colorFormat = colorTargetFormat;
    s_depthFormat = depthTargetFormat;

    // デバイスがサポートするシェーダーフォーマットを検出してロード情報を設定
    SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device);
    if (formats & SDL_GPU_SHADERFORMAT_SPIRV)
    {
        s_shaderFormat = SDL_GPU_SHADERFORMAT_SPIRV;
        s_shaderSubDir = "spv/";
        s_shaderExtension = ".spv";
        s_shaderEntryPoint = "main";
    }
    else if (formats & SDL_GPU_SHADERFORMAT_DXIL)
    {
        s_shaderFormat = SDL_GPU_SHADERFORMAT_DXIL;
        s_shaderSubDir = "dxil/";
        s_shaderExtension = ".dxil";
        s_shaderEntryPoint = "main";
    }
    else if (formats & SDL_GPU_SHADERFORMAT_MSL)
    {
        s_shaderFormat = SDL_GPU_SHADERFORMAT_MSL;
        s_shaderSubDir = "msl/";
        s_shaderExtension = ".msl";
        s_shaderEntryPoint = "main0";
    }
    CubismLogInfo("Shader format: subdir=%s ext=%s entrypoint=%s",
                  s_shaderSubDir.GetRawString(), s_shaderExtension.GetRawString(),
                  s_shaderEntryPoint.GetRawString());
}

void CubismRenderer_SDL3::SetRenderTarget(SDL_GPUTexture* texture, SDL_GPUTextureFormat format,
                                           csmUint32 width, csmUint32 height)
{
    s_renderTexture = texture;
    s_colorFormat = format;
    s_renderWidth = width;
    s_renderHeight = height;
}

void CubismRenderer_SDL3::SetImageExtent(csmUint32 width, csmUint32 height)
{
    s_renderWidth = width;
    s_renderHeight = height;
}

void CubismRenderer_SDL3::SetExternalCommandBuffer(SDL_GPUCommandBuffer* commandBuffer)
{
    s_externalCommandBuffer = commandBuffer;
}

void CubismRenderer_SDL3::CreateVertexBuffer()
{
    const csmInt32 drawableCount = GetModel()->GetDrawableCount();
    const csmInt32 offscreenCount = GetModel()->GetOffscreenCount();
    _stagingBuffers.Resize(s_bufferSetNum);
    _vertexBuffers.Resize(s_bufferSetNum);

    _offscreenStagingBuffers.Resize(s_bufferSetNum);
    _offscreenVertexBuffers.Resize(s_bufferSetNum);

    _copyStagingBuffer.Resize(s_bufferSetNum);
    _copyVertexBuffer.Resize(s_bufferSetNum);

    for (csmUint32 buffer = 0; buffer < s_bufferSetNum; buffer++)
    {
        // Drawable用
        _stagingBuffers[buffer].Resize(drawableCount);
        _vertexBuffers[buffer].Resize(drawableCount);
        for (csmInt32 drawAssign = 0; drawAssign < drawableCount; drawAssign++)
        {
            const csmInt32 vcount = GetModel()->GetDrawableVertexCount(drawAssign);
            if (vcount != 0)
            {
                csmUint32 bufferSize = sizeof(ModelVertex) * vcount;

                _stagingBuffers[buffer][drawAssign].CreateTransferBuffer(s_device, bufferSize, SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD);
                _vertexBuffers[buffer][drawAssign].CreateBuffer(s_device, bufferSize, SDL_GPU_BUFFERUSAGE_VERTEX);
            }
        }

        // オフスクリーン用
        _offscreenStagingBuffers[buffer].Resize(offscreenCount);
        _offscreenVertexBuffers[buffer].Resize(offscreenCount);

        for (csmInt32 offscreenAssign = 0; offscreenAssign < offscreenCount; ++offscreenAssign)
        {
            csmUint32 bufferSize = sizeof(modelRenderTargetVertexArray);

            _offscreenStagingBuffers[buffer][offscreenAssign].CreateTransferBuffer(s_device, bufferSize, SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD);
            _offscreenVertexBuffers[buffer][offscreenAssign].CreateBuffer(s_device, bufferSize, SDL_GPU_BUFFERUSAGE_VERTEX);
        }

        // コピー用
        csmUint32 copyVertexBufferSize = sizeof(modelRenderTargetVertexArray);
        _copyStagingBuffer[buffer].CreateTransferBuffer(s_device, copyVertexBufferSize, SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD);
        _copyVertexBuffer[buffer].CreateBuffer(s_device, copyVertexBufferSize, SDL_GPU_BUFFERUSAGE_VERTEX);
    }
}

void CubismRenderer_SDL3::CreateIndexBuffer()
{
    const csmInt32 drawableCount = GetModel()->GetDrawableCount();
    const csmInt32 offscreenCount = GetModel()->GetOffscreenCount();
    _indexBuffers.Resize(s_bufferSetNum);
    _offscreenIndexBuffers.Resize(s_bufferSetNum);
    _copyIndexBuffer.Resize(s_bufferSetNum);

    // 全バッファセットのインデックスバッファアップロードを1つのコマンドバッファにバッチ化
    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(s_device);
    if (commandBuffer == nullptr)
    {
        CubismLogError("Failed to acquire command buffer for index buffer upload: %s", SDL_GetError());
        return;
    }

    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
    if (copyPass == nullptr)
    {
        CubismLogError("Failed to begin copy pass for index buffer upload: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(commandBuffer);
        return;
    }

    // サブミット後に破棄するステージングバッファを一時保持
    csmVector<CubismBufferSDL3> stagingBuffers;

    for (csmUint32 buffer = 0; buffer < s_bufferSetNum; buffer++)
    {
        // Drawable用
        _indexBuffers[buffer].Resize(drawableCount);
        for (csmInt32 drawAssign = 0; drawAssign < drawableCount; drawAssign++)
        {
            const csmInt32 icount = GetModel()->GetDrawableVertexIndexCount(drawAssign);
            if (icount != 0)
            {
                csmUint32 bufferSize = sizeof(uint16_t) * icount;
                const csmUint16* indices = GetModel()->GetDrawableVertexIndices(drawAssign);

                CubismBufferSDL3 stagingBuffer;
                stagingBuffer.CreateTransferBuffer(s_device, bufferSize, SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD);
                void* mappedData = stagingBuffer.MapTransferBuffer(s_device, false);
                memcpy(mappedData, indices, bufferSize);
                stagingBuffer.UnmapTransferBuffer(s_device);

                _indexBuffers[buffer][drawAssign].CreateBuffer(s_device, bufferSize, SDL_GPU_BUFFERUSAGE_INDEX);
                _indexBuffers[buffer][drawAssign].UploadToBuffer(copyPass, &stagingBuffer, bufferSize);

                stagingBuffers.PushBack(stagingBuffer);
            }
        }

        // オフスクリーン用
        _offscreenIndexBuffers[buffer].Resize(offscreenCount);
        for (csmInt32 offscreenAssign = 0; offscreenAssign < offscreenCount; ++offscreenAssign)
        {
            csmUint32 bufferSize = sizeof(modelRenderTargetIndexArray);

            CubismBufferSDL3 stagingBuffer;
            stagingBuffer.CreateTransferBuffer(s_device, bufferSize, SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD);
            void* mappedData = stagingBuffer.MapTransferBuffer(s_device, false);
            memcpy(mappedData, modelRenderTargetIndexArray, bufferSize);
            stagingBuffer.UnmapTransferBuffer(s_device);

            _offscreenIndexBuffers[buffer][offscreenAssign].CreateBuffer(s_device, bufferSize, SDL_GPU_BUFFERUSAGE_INDEX);
            _offscreenIndexBuffers[buffer][offscreenAssign].UploadToBuffer(copyPass, &stagingBuffer, bufferSize);

            stagingBuffers.PushBack(stagingBuffer);
        }

        // コピー用
        {
            csmUint32 copyIndexBufferSize = sizeof(modelRenderTargetIndexArray);

            CubismBufferSDL3 stagingBuffer;
            stagingBuffer.CreateTransferBuffer(s_device, copyIndexBufferSize, SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD);
            void* mappedData = stagingBuffer.MapTransferBuffer(s_device, false);
            memcpy(mappedData, modelRenderTargetIndexArray, copyIndexBufferSize);
            stagingBuffer.UnmapTransferBuffer(s_device);

            _copyIndexBuffer[buffer].CreateBuffer(s_device, copyIndexBufferSize, SDL_GPU_BUFFERUSAGE_INDEX);
            _copyIndexBuffer[buffer].UploadToBuffer(copyPass, &stagingBuffer, copyIndexBufferSize);

            stagingBuffers.PushBack(stagingBuffer);
        }
    }

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);

    // サブミット後にステージングバッファを解放
    for (csmInt32 i = 0; i < stagingBuffers.GetSize(); i++)
    {
        stagingBuffers[i].Destroy(s_device);
    }
}

void CubismRenderer_SDL3::CreateDepthBuffer()
{
    _depthImage.CreateTexture(s_device, static_cast<csmInt32>(s_renderWidth), static_cast<csmInt32>(s_renderHeight),
                              s_depthFormat, SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET);
}

void CubismRenderer_SDL3::InitializeRenderer()
{
    CreateVertexBuffer();
    CreateIndexBuffer();
    CreateDepthBuffer();
    CubismPipeline_SDL3::GetInstance()->CreatePipelines(s_device, s_colorFormat);
}

void CubismRenderer_SDL3::Initialize(CubismModel* model)
{
    Initialize(model, 1);
}

void CubismRenderer_SDL3::Initialize(CubismModel* model, csmInt32 maskBufferCount)
{
    if (s_device == nullptr)
    {
        CubismLogError("Device has not been set.");
        CSM_ASSERT(0);
        return;
    }

    CubismRenderer::Initialize(model, maskBufferCount); // 親クラスの処理を呼ぶ

    // 1未満は1に補正する
    if (maskBufferCount < 1)
    {
        maskBufferCount = 1;
        CubismLogWarning(
            "The number of render textures must be an integer greater than or equal to 1. Set the number of render textures to 1.");
    }

    // オフスクリーン作成
    _modelRenderTargets.Clear();
    csmInt32 createSize = 1;
    for (csmInt32 i = 0; i < createSize; i++)
    {
        CubismRenderTarget_SDL3 modelRenderTarget;
        modelRenderTarget.CreateRenderTarget(s_device, _modelRenderTargetWidth, _modelRenderTargetHeight, s_colorFormat, s_depthFormat);
        _modelRenderTargets.PushBack(modelRenderTarget);
    }

    if (model->IsUsingMasking())
    {
        //モデルがマスクを使用している時のみにする
        _drawableClippingManager = CSM_NEW CubismClippingManager_SDL3();
        _drawableClippingManager->Initialize(
            *model,
            maskBufferCount,
            DrawableObjectType_Drawable
        );

        // クリッピングマスクのバッファを作成
        const CubismVector2 clippingMaskBufferSize = _drawableClippingManager->GetClippingMaskBufferSize();
        const csmInt32 renderTextureCount = _drawableClippingManager->GetRenderTextureCount();
        _drawableMaskBuffers.Resize(s_bufferSetNum);
        for (csmUint32 buffer = 0; buffer < s_bufferSetNum; buffer++)
        {
            _drawableMaskBuffers[buffer].Resize(renderTextureCount);
            for (csmInt32 i = 0; i < renderTextureCount; ++i)
            {
                _drawableMaskBuffers[buffer][i].CreateRenderTarget(s_device,
                    static_cast<csmUint32>(clippingMaskBufferSize.X),
                    static_cast<csmUint32>(clippingMaskBufferSize.Y),
                    s_colorFormat, s_depthFormat);
            }
        }
    }

    // オフスクリーン用のクリッピングマネージャの初期化
    const csmInt32 offscreenCount = model->GetOffscreenCount();
    if (offscreenCount > 0 && model->IsUsingMaskingForOffscreen())
    {
        _offscreenClippingManager = CSM_NEW CubismClippingManager_SDL3();
        _offscreenClippingManager->Initialize(
            *model,
            maskBufferCount,
            DrawableObjectType_Offscreen
        );

        // オフスクリーン用マスクバッファの作成
        const CubismVector2 clippingMaskBufferSize = _offscreenClippingManager->GetClippingMaskBufferSize();
        const csmInt32 renderTextureCount = _offscreenClippingManager->GetRenderTextureCount();
        _offscreenMaskBuffers.Resize(s_bufferSetNum);
        for (csmUint32 buffer = 0; buffer < s_bufferSetNum; buffer++)
        {
            _offscreenMaskBuffers[buffer].Resize(renderTextureCount);
            for (csmInt32 i = 0; i < renderTextureCount; ++i)
            {
                _offscreenMaskBuffers[buffer][i].CreateRenderTarget(s_device,
                    static_cast<csmUint32>(clippingMaskBufferSize.X),
                    static_cast<csmUint32>(clippingMaskBufferSize.Y),
                    s_colorFormat, s_depthFormat);
            }
        }
    }

    // オフスクリーンの初期化
    // オフスクリーン数が0の場合は何もしない
    if (offscreenCount > 0)
    {
        _offscreenList.Resize(offscreenCount);
        for (csmInt32 i = 0; i < offscreenCount; ++i)
        {
            _offscreenList[i].SetOffscreenIndex(i);
            // オフスクリーンのサイズはモデルのレンダターゲットサイズを使用
            // 注: 実際のレンダーターゲットは描画時にSetOffscreenRenderTargetで設定される
        }

        // 全てのオフスクリーンを登録し終わってから行う
        SetupParentOffscreens(model, offscreenCount);
    }

    _commandBufferCurrent = 0;

    InitializeRenderer();
}

void CubismRenderer_SDL3::SetupParentOffscreens(const CubismModel* model, csmInt32 offscreenCount)
{
    // オフスクリーンの親をセットアップ
    CubismOffscreenRenderTarget_SDL3* parentOffscreen;
    for (csmInt32 offscreenIndex = 0; offscreenIndex < offscreenCount; ++offscreenIndex)
    {
        parentOffscreen = nullptr;
        const csmInt32 ownerIndex = model->GetOffscreenOwnerIndices()[offscreenIndex];
        csmInt32 parentIndex = model->GetPartParentPartIndex(ownerIndex);

        // 親のオフスクリーンを探す
        while (parentIndex != CubismModel::CubismNoIndex_Parent)
        {
            for (csmInt32 i = 0; i < offscreenCount; ++i)
            {
                if (model->GetOffscreenOwnerIndices()[_offscreenList[i].GetOffscreenIndex()] != parentIndex)
                {
                    continue; // オフスクリーンのインデックスが親と一致しなければスキップ
                }

                parentOffscreen = &_offscreenList[i];
                break;
            }

            if (parentOffscreen != nullptr)
            {
                break;
            }

            parentIndex = model->GetPartParentPartIndex(parentIndex);
        }

        // 親のオフスクリーンを設定
        _offscreenList[offscreenIndex].SetParentPartOffscreen(parentOffscreen);
    }
}

void CubismRenderer_SDL3::CopyToBuffer(csmInt32 drawAssign, const csmInt32 vcount,
                                        const csmFloat32* varray, const csmFloat32* uvarray,
                                        SDL_GPUCopyPass* copyPass)
{
    if (vcount == 0)
    {
        return;
    }

    CubismBufferSDL3* stagingBuffer = &_stagingBuffers[_commandBufferCurrent][drawAssign];
    CubismBufferSDL3* vertexBuffer = &_vertexBuffers[_commandBufferCurrent][drawAssign];

    // ステージングバッファにデータをマップ
    ModelVertex* mappedData = static_cast<ModelVertex*>(stagingBuffer->MapTransferBuffer(s_device, false));
    if (mappedData)
    {
        for (csmInt32 i = 0; i < vcount; ++i)
        {
            mappedData[i].pos.X = varray[i * 2];
            mappedData[i].pos.Y = varray[i * 2 + 1];
            mappedData[i].texCoord.X = uvarray[i * 2];
            mappedData[i].texCoord.Y = uvarray[i * 2 + 1];
        }
        stagingBuffer->UnmapTransferBuffer(s_device);
    }

    // ステージングバッファから頂点バッファへコピー
    csmUint32 bufferSize = sizeof(ModelVertex) * vcount;
    vertexBuffer->UploadToBuffer(copyPass, stagingBuffer, bufferSize);
}

void CubismRenderer_SDL3::UpdateMatrix(csmFloat32 mat4[16], CubismMatrix44 cubismMat)
{
    for (csmInt32 i = 0; i < 16; ++i)
    {
        mat4[i] = cubismMat.GetArray()[i];
    }
}

void CubismRenderer_SDL3::UpdateColor(csmFloat32 vec4[4], csmFloat32 r, csmFloat32 g, csmFloat32 b, csmFloat32 a)
{
    vec4[0] = r;
    vec4[1] = g;
    vec4[2] = b;
    vec4[3] = a;
}

void CubismRenderer_SDL3::SetColorUniformBuffer(ModelUBO& ubo, const CubismTextureColor& baseColor,
                                                 const CubismTextureColor& multiplyColor, const CubismTextureColor& screenColor)
{
    UpdateColor(ubo.baseColor, baseColor.R, baseColor.G, baseColor.B, baseColor.A);
    UpdateColor(ubo.multiplyColor, multiplyColor.R, multiplyColor.G, multiplyColor.B, multiplyColor.A);
    UpdateColor(ubo.screenColor, screenColor.R, screenColor.G, screenColor.B, screenColor.A);
}

void CubismRenderer_SDL3::BindTexture(CubismImageSDL3& image)
{
    _textures.PushBack(image);
}

void CubismRenderer_SDL3::SetClippingMaskBufferSize(csmFloat32 width, csmFloat32 height)
{
    if (_drawableClippingManager == nullptr)
    {
        return;
    }

    // インスタンス破棄前にレンダーテクスチャの数を保存
    const csmInt32 renderTextureCount = _drawableClippingManager->GetRenderTextureCount();

    // FrameBufferのサイズを変更するためにインスタンスを破棄・再作成する
    CSM_DELETE_SELF(CubismClippingManager_SDL3, _drawableClippingManager);

    _drawableClippingManager = CSM_NEW CubismClippingManager_SDL3();

    _drawableClippingManager->SetClippingMaskBufferSize(width, height);

    _drawableClippingManager->Initialize(
        *GetModel(),
        renderTextureCount,
        DrawableObjectType_Drawable
    );
}

CubismVector2 CubismRenderer_SDL3::GetClippingMaskBufferSize() const
{
    if (_drawableClippingManager != nullptr)
    {
        return _drawableClippingManager->GetClippingMaskBufferSize();
    }
    return CubismVector2();
}

CubismRenderTarget_SDL3* CubismRenderer_SDL3::GetModelRenderTarget()
{
    if (_modelRenderTargets.GetSize() > 0)
    {
        return &_modelRenderTargets[0];
    }
    return nullptr;
}

CubismRenderTarget_SDL3* CubismRenderer_SDL3::GetDrawableMaskBuffer(csmUint32 backbufferNum, csmInt32 offscreenIndex)
{
    if (backbufferNum < _drawableMaskBuffers.GetSize() &&
        offscreenIndex < _drawableMaskBuffers[backbufferNum].GetSize())
    {
        return &_drawableMaskBuffers[backbufferNum][offscreenIndex];
    }
    return nullptr;
}

void CubismRenderer_SDL3::SetClippingContextBufferForMask(CubismClippingContext_SDL3* clip)
{
    _clippingContextBufferForMask = clip;
}

CubismClippingContext_SDL3* CubismRenderer_SDL3::GetClippingContextBufferForMask() const
{
    return _clippingContextBufferForMask;
}

void CubismRenderer_SDL3::SetClippingContextBufferForDraw(CubismClippingContext_SDL3* clip)
{
    _clippingContextBufferForDraw = clip;
}

CubismClippingContext_SDL3* CubismRenderer_SDL3::GetClippingContextBufferForDrawable() const
{
    return _clippingContextBufferForDraw;
}

void CubismRenderer_SDL3::SetClippingContextBufferForOffscreen(CubismClippingContext_SDL3* clip)
{
    _clippingContextBufferForOffscreen = clip;
}

CubismClippingContext_SDL3* CubismRenderer_SDL3::GetClippingContextBufferForOffscreen() const
{
    return _clippingContextBufferForOffscreen;
}

CubismRenderTarget_SDL3* CubismRenderer_SDL3::GetOffscreenMaskBuffer(csmUint32 backBufferNum, csmInt32 offscreenIndex)
{
    if (backBufferNum < _offscreenMaskBuffers.GetSize() &&
        offscreenIndex < _offscreenMaskBuffers[backBufferNum].GetSize())
    {
        return &_offscreenMaskBuffers[backBufferNum][offscreenIndex];
    }
    return nullptr;
}

SDL_GPURenderPass* CubismRenderer_SDL3::BeginRendering(SDL_GPUCommandBuffer* commandBuffer, csmBool isResume)
{
    SDL_GPUColorTargetInfo colorTargetInfo = {};
    colorTargetInfo.texture = s_renderTexture;
    colorTargetInfo.clear_color = _clearColor;
    colorTargetInfo.load_op = isResume ? SDL_GPU_LOADOP_LOAD : SDL_GPU_LOADOP_CLEAR;
    colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPUDepthStencilTargetInfo depthTargetInfo = {};
    depthTargetInfo.texture = _depthImage.GetTexture();
    depthTargetInfo.clear_depth = 1.0f;
    // 深度は常にクリアする（Vulkanレンダラー準拠）。
    depthTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    depthTargetInfo.store_op = SDL_GPU_STOREOP_DONT_CARE;
    depthTargetInfo.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
    depthTargetInfo.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
    depthTargetInfo.cycle = false;
    depthTargetInfo.clear_stencil = 0;

    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1,
                                                            _depthImage.IsValid() ? &depthTargetInfo : nullptr);
    if (renderPass == nullptr)
    {
        CubismLogError("Failed to begin render pass: %s", SDL_GetError());
    }
    return renderPass;
}

void CubismRenderer_SDL3::EndRendering(SDL_GPURenderPass* renderPass)
{
    if (renderPass != nullptr)
    {
        SDL_EndGPURenderPass(renderPass);
    }
}

SDL_GPURenderPass* CubismRenderer_SDL3::BeginRenderTarget(SDL_GPUCommandBuffer* commandBuffer,
                                                           csmBool isResume)
{
    if (GetModel()->IsBlendModeEnabled())
    {
        BeforeDrawModelRenderTarget();
        // ブレンドモード有効時はモデルレンダーターゲットに描画
        // Vulkanレンダラーに合わせて、BeginDrawの後にフラグをセットする。
        // これにより初回（_isClearedModelRenderTarget=false）では
        // isClear=!false=trueとなりカラーバッファが正しくクリアされる。
        SDL_GPURenderPass* renderPass = _modelRenderTargets[0].BeginDraw(commandBuffer, 0.0f, 0.0f, 0.0f, 0.0f, !_isClearedModelRenderTarget);
        _isClearedModelRenderTarget = true;
        return renderPass;
    }
    else
    {
        return BeginRendering(commandBuffer, isResume);
    }
}

void CubismRenderer_SDL3::EndRenderTarget(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass)
{
    if (GetModel()->IsBlendModeEnabled())
    {
        // モデルレンダーターゲットのレンダーパスを終了
        if (_currentRenderTarget != nullptr)
        {
            _currentRenderTarget->EndDraw(renderPass);
        }
        else
        {
            EndRendering(renderPass);
        }
        // メインターゲットへコピーバック
        AfterDrawModelRenderTarget();
        // CopyRenderTargetで実際のコピーを実行
        const CubismRenderTarget_SDL3* srcTarget = (_currentOffscreen != nullptr)
            ? _currentOffscreen->GetRenderTarget()
            : &_modelRenderTargets[0];
        CopyRenderTarget(srcTarget, commandBuffer);
        _currentRenderTarget = nullptr;
    }
    else
    {
        EndRendering(renderPass);
    }
}

void CubismRenderer_SDL3::DoDrawModel()
{
    _isClearedModelRenderTarget = false;

    // 外部コマンドバッファが設定されている場合はそれを使用する
    // （メインレンダーループのコマンドバッファを共有し、スワップチェインテクスチャのレイアウト追跡を正しく行う）
    const csmBool useExternalCmdBuf = (s_externalCommandBuffer != nullptr);
    SDL_GPUCommandBuffer* commandBuffer = useExternalCmdBuf ? s_externalCommandBuffer : SDL_AcquireGPUCommandBuffer(s_device);
    if (commandBuffer == nullptr)
    {
        CubismLogError("Failed to acquire command buffer: %s", SDL_GetError());
        return;
    }

    // コピーパスを開始してバッファを更新
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
    if (copyPass == nullptr)
    {
        CubismLogError("Failed to begin copy pass in DoDrawModel: %s", SDL_GetError());
        if (!useExternalCmdBuf)
        {
            SDL_CancelGPUCommandBuffer(commandBuffer);
        }
        return;
    }

    //------------ クリッピングマスク・バッファ前処理方式の場合 ------------
    if (_drawableClippingManager != nullptr)
    {
        // サイズが違う場合はここで作成しなおし
        for (csmInt32 i = 0; i < _drawableClippingManager->GetRenderTextureCount(); ++i)
        {
            if (_drawableMaskBuffers[_commandBufferCurrent][i].GetBufferWidth() != static_cast<csmUint32>(
                _drawableClippingManager->GetClippingMaskBufferSize().X) ||
                _drawableMaskBuffers[_commandBufferCurrent][i].GetBufferHeight() != static_cast<csmUint32>(
                    _drawableClippingManager->GetClippingMaskBufferSize().Y))
            {
                _drawableMaskBuffers[_commandBufferCurrent][i].CreateRenderTarget(
                    s_device,
                    static_cast<csmUint32>(_drawableClippingManager->GetClippingMaskBufferSize().X),
                    static_cast<csmUint32>(_drawableClippingManager->GetClippingMaskBufferSize().Y),
                    s_colorFormat, s_depthFormat
                );
            }
        }
        if (IsUsingHighPrecisionMask())
        {
            _drawableClippingManager->SetupMatrixForHighPrecision(*GetModel(), false, DrawableObjectType_Drawable);
        }
    }

    // オフスクリーン用クリッピングマネージャーの処理
    if (_offscreenClippingManager != nullptr)
    {
        // サイズが違う場合はここで作成しなおし
        for (csmInt32 i = 0; i < _offscreenClippingManager->GetRenderTextureCount(); ++i)
        {
            if (_offscreenMaskBuffers[_commandBufferCurrent][i].GetBufferWidth() != static_cast<csmUint32>(
                _offscreenClippingManager->GetClippingMaskBufferSize().X) ||
                _offscreenMaskBuffers[_commandBufferCurrent][i].GetBufferHeight() != static_cast<csmUint32>(
                    _offscreenClippingManager->GetClippingMaskBufferSize().Y))
            {
                _offscreenMaskBuffers[_commandBufferCurrent][i].CreateRenderTarget(
                    s_device,
                    static_cast<csmUint32>(_offscreenClippingManager->GetClippingMaskBufferSize().X),
                    static_cast<csmUint32>(_offscreenClippingManager->GetClippingMaskBufferSize().Y),
                    s_colorFormat, s_depthFormat
                );
            }
        }
        if (IsUsingHighPrecisionMask())
        {
            _offscreenClippingManager->SetupMatrixForHighPrecision(*GetModel(), false, DrawableObjectType_Offscreen, GetMvpMatrix());
        }
    }

    // 全てのDrawableの頂点データをコピーパスでアップロード
    {
        const csmInt32 drawableCount = GetModel()->GetDrawableCount();
        for (csmInt32 i = 0; i < drawableCount; i++)
        {
            const csmInt32 vcount = GetModel()->GetDrawableVertexCount(i);
            if (vcount > 0 && static_cast<csmInt32>(_vertexBuffers[_commandBufferCurrent].GetSize()) > i
                && _vertexBuffers[_commandBufferCurrent][i].GetBuffer() != nullptr)
            {
                CopyToBuffer(i, vcount,
                             const_cast<csmFloat32*>(GetModel()->GetDrawableVertices(i)),
                             reinterpret_cast<csmFloat32*>(const_cast<Core::csmVector2*>(GetModel()->GetDrawableVertexUvs(i))),
                             copyPass);
            }
        }

        // オフスクリーン用頂点データのアップロード
        const csmInt32 offscreenCount = GetModel()->GetOffscreenCount();
        for (csmInt32 i = 0; i < offscreenCount; i++)
        {
            if (static_cast<csmInt32>(_offscreenStagingBuffers[_commandBufferCurrent].GetSize()) > i
                && _offscreenStagingBuffers[_commandBufferCurrent][i].GetTransferBuffer() != nullptr
                && _offscreenVertexBuffers[_commandBufferCurrent][i].GetBuffer() != nullptr)
            {
                CubismBufferSDL3* stagingBuf = &_offscreenStagingBuffers[_commandBufferCurrent][i];
                CubismBufferSDL3* vertexBuf = &_offscreenVertexBuffers[_commandBufferCurrent][i];
                ModelVertex* mapped = static_cast<ModelVertex*>(stagingBuf->MapTransferBuffer(s_device, false));
                if (mapped)
                {
                    memcpy(mapped, modelRenderTargetVertexArray, sizeof(modelRenderTargetVertexArray));
                    stagingBuf->UnmapTransferBuffer(s_device);
                    vertexBuf->UploadToBuffer(copyPass, stagingBuf, sizeof(modelRenderTargetVertexArray));
                }
            }
        }

        // コピー用頂点データのアップロード
        if (_copyStagingBuffer[_commandBufferCurrent].GetTransferBuffer() != nullptr
            && _copyVertexBuffer[_commandBufferCurrent].GetBuffer() != nullptr)
        {
            CubismBufferSDL3* stagingBuf = &_copyStagingBuffer[_commandBufferCurrent];
            CubismBufferSDL3* vertexBuf = &_copyVertexBuffer[_commandBufferCurrent];
            ModelVertex* mapped = static_cast<ModelVertex*>(stagingBuf->MapTransferBuffer(s_device, false));
            if (mapped)
            {
                memcpy(mapped, modelRenderTargetVertexArray, sizeof(modelRenderTargetVertexArray));
                stagingBuf->UnmapTransferBuffer(s_device);
                vertexBuf->UploadToBuffer(copyPass, stagingBuf, sizeof(modelRenderTargetVertexArray));
            }
        }
    }

    SDL_EndGPUCopyPass(copyPass);

    if (useExternalCmdBuf)
    {
        // 外部コマンドバッファの場合はサブミットせず、そのまま描画ループへ
        // （コピーパスとレンダーパスは同一コマンドバッファ上で順次実行される）
        DrawObjectLoop(commandBuffer);
    }
    else
    {
        // 内部コマンドバッファの場合はコピーパス用をサブミットし、描画用に新規取得
        SDL_SubmitGPUCommandBuffer(commandBuffer);
        commandBuffer = SDL_AcquireGPUCommandBuffer(s_device);
        if (commandBuffer == nullptr)
        {
            CubismLogError("Failed to acquire command buffer: %s", SDL_GetError());
            return;
        }
        DrawObjectLoop(commandBuffer);
    }
}

void CubismRenderer_SDL3::PostDraw()
{
    _commandBufferCurrent = (_commandBufferCurrent + 1) % s_bufferSetNum;
}

void CubismRenderer_SDL3::BeforeDrawModelRenderTarget()
{
    if (!GetModel()->IsBlendModeEnabled())
    {
        return;
    }

    // オフスクリーンのバッファのサイズが違う場合は作り直し
    for (csmInt32 i = 0; i < _modelRenderTargets.GetSize(); ++i)
    {
        if (_modelRenderTargets[i].GetBufferWidth() != _modelRenderTargetWidth ||
            _modelRenderTargets[i].GetBufferHeight() != _modelRenderTargetHeight)
        {
            _modelRenderTargets[i].DestroyRenderTarget();
            _modelRenderTargets[i].CreateRenderTarget(s_device, _modelRenderTargetWidth, _modelRenderTargetHeight,
                                                       s_colorFormat, s_depthFormat);
        }
    }

    // 別バッファに描画開始
    _currentRenderTarget = &_modelRenderTargets[0];

    // ブレンド読み取り用テクスチャの確保（レンダーターゲットと同サイズ）
    EnsureBlendReadImage();
}

void CubismRenderer_SDL3::AfterDrawModelRenderTarget()
{
    if (!GetModel()->IsBlendModeEnabled())
    {
        return;
    }

    // 描画先をリセット
    _currentRenderTarget = nullptr;
}

void CubismRenderer_SDL3::EnsureBlendReadImage()
{
    const csmUint32 w = _modelRenderTargetWidth;
    const csmUint32 h = _modelRenderTargetHeight;
    if (_blendReadImage.IsValid() &&
        static_cast<csmUint32>(_blendReadImage.GetWidth()) == w &&
        static_cast<csmUint32>(_blendReadImage.GetHeight()) == h)
    {
        return; // 既に正しいサイズで作成済み
    }

    _blendReadImage.Destroy(s_device);
    _blendReadImage.CreateTexture(s_device, static_cast<csmInt32>(w), static_cast<csmInt32>(h),
                                  s_colorFormat,
                                  SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER);
    _blendReadImage.CreateSampler(s_device, 1.0f);
}

void CubismRenderer_SDL3::BlitBlendSnapshot(SDL_GPUCommandBuffer* commandBuffer, CubismRenderTarget_SDL3* srcTarget)
{
    if (commandBuffer == nullptr || srcTarget == nullptr || !_blendReadImage.IsValid())
    {
        return;
    }

    SDL_GPUBlitInfo blitInfo = {};
    blitInfo.source.texture = srcTarget->GetTexture();
    blitInfo.source.w = srcTarget->GetBufferWidth();
    blitInfo.source.h = srcTarget->GetBufferHeight();
    blitInfo.destination.texture = _blendReadImage.GetTexture();
    blitInfo.destination.w = static_cast<Uint32>(_blendReadImage.GetWidth());
    blitInfo.destination.h = static_cast<Uint32>(_blendReadImage.GetHeight());
    blitInfo.load_op = SDL_GPU_LOADOP_DONT_CARE;
    blitInfo.filter = SDL_GPU_FILTER_NEAREST;

    SDL_BlitGPUTexture(commandBuffer, &blitInfo);
}

void CubismRenderer_SDL3::ExecuteDrawForDrawable(const CubismModel& model, const csmInt32 index, SDL_GPURenderPass* renderPass)
{
    if (renderPass == nullptr)
    {
        return;
    }

    // パイプラインレイアウト設定用のインデックスを取得
    csmUint32 blendIndex = 0;
    csmUint32 shaderIndex = 0;
    const csmBool masked = GetClippingContextBufferForDrawable() != nullptr;
    const csmBool invertedMask = model.GetDrawableInvertedMask(index);
    const csmBool isPremultipliedAlpha = IsPremultipliedAlpha();
    const csmInt32 offset = (masked ? (invertedMask ? 2 : 1) : 0) + (isPremultipliedAlpha ? 3 : 0);
    const csmInt32 shaderNameBegin = GetShaderNamesBegin(model.GetDrawableBlendModeType(index));

    switch (shaderNameBegin)
    {
    default:
        // 5.3以降
        shaderIndex = shaderNameBegin + offset;
        blendIndex = 0;
        break;
    case ShaderNames_Normal:
        shaderIndex = ShaderNames_Normal + offset;
        blendIndex = CompatibleBlend::Blend_Normal;
        break;
    case ShaderNames_Add:
        shaderIndex = ShaderNames_Add + offset;
        blendIndex = CompatibleBlend::Blend_Add;
        break;
    case ShaderNames_Mult:
        shaderIndex = ShaderNames_Mult + offset;
        blendIndex = CompatibleBlend::Blend_Mult;
        break;
    }

    SDL_GPUGraphicsPipeline* pipeline = CubismPipeline_SDL3::GetInstance()->GetPipeline(shaderIndex, blendIndex);
    if (pipeline == nullptr)
    {
        return;
    }

    ModelUBO ubo;

    if (masked)
    {
        // クリッピング用行列の設定
        UpdateMatrix(ubo.clipMatrix, GetClippingContextBufferForDrawable()->_matrixForDraw);
        // カラーチャンネルの設定
        SetColorChannel(ubo, GetClippingContextBufferForDrawable());
    }

    // MVP行列の設定
    UpdateMatrix(ubo.projectionMatrix, GetMvpMatrix());

    // 色定数バッファの設定
    CubismTextureColor baseColor;
    if (model.IsBlendModeEnabled())
    {
        csmFloat32 drawableOpacity = model.GetDrawableOpacity(index);
        baseColor.A = drawableOpacity;
        if (isPremultipliedAlpha)
        {
            baseColor.R = drawableOpacity;
            baseColor.G = drawableOpacity;
            baseColor.B = drawableOpacity;
        }
    }
    else
    {
        baseColor = GetModelColorWithOpacity(model.GetDrawableOpacity(index));
    }
    CubismTextureColor multiplyColor = model.GetMultiplyColor(index);
    CubismTextureColor screenColor = model.GetScreenColor(index);
    SetColorUniformBuffer(ubo, baseColor, multiplyColor, screenColor);

    // パイプラインのバインド
    SDL_BindGPUGraphicsPipeline(renderPass, pipeline);

    // 頂点バッファとインデックスバッファのバインド
    BindVertexAndIndexBuffers(index, renderPass, DrawableObjectType_Drawable);

    // テクスチャインデックス取得
    csmInt32 textureIndex = model.GetDrawableTextureIndex(index);

    // テクスチャのバインド（パイプラインは3つのフラグメントサンプラーを宣言しているため、常に3つバインドする）
    if (textureIndex >= 0 && textureIndex < static_cast<csmInt32>(_textures.GetSize()))
    {
        SDL_GPUTextureSamplerBinding textureSamplerBindings[3] = {};

        // s_texture0: メインテクスチャ
        textureSamplerBindings[0].texture = _textures[textureIndex].GetTexture();
        textureSamplerBindings[0].sampler = _textures[textureIndex].GetSampler();

        // s_texture1: マスクテクスチャ
        if (masked && GetClippingContextBufferForDrawable() != nullptr)
        {
            CubismRenderTarget_SDL3* maskBuffer = GetDrawableMaskBuffer(_commandBufferCurrent,
                                                                         GetClippingContextBufferForDrawable()->_bufferIndex);
            if (maskBuffer != nullptr)
            {
                textureSamplerBindings[1].texture = maskBuffer->GetTexture();
                textureSamplerBindings[1].sampler = maskBuffer->GetTextureSampler();
            }
        }
        else
        {
             // 未使用時はダミーとしてメインテクスチャをバインド
            textureSamplerBindings[1] = textureSamplerBindings[0];
        }

        // s_blendTexture: ブレンドテクスチャ
        // ブレンドモード有効時は _blendReadImage
        // （レンダーターゲットのスナップショット）をバインドする。
        // レンダーターゲット自体をバインドするとレイアウト競合
        // （COLOR_ATTACHMENT vs SHADER_READ_ONLY）が発生するため、
        // 事前にBlitBlendSnapshotで別テクスチャにコピーしたものを使用する。
        if (model.IsBlendModeEnabled() && _blendReadImage.IsValid())
        {
            textureSamplerBindings[2].texture = _blendReadImage.GetTexture();
            textureSamplerBindings[2].sampler = _blendReadImage.GetSampler();
        }
        else
        {
            // 未使用時はダミーとしてメインテクスチャをバインド
            textureSamplerBindings[2] = textureSamplerBindings[0];
        }

        SDL_BindGPUFragmentSamplers(renderPass, 0, textureSamplerBindings, 3);
    }

    // ユニフォームバッファをプッシュ
    SDL_PushGPUVertexUniformData(_activeCommandBuffer, 0, &ubo, sizeof(ModelUBO));
    SDL_PushGPUFragmentUniformData(_activeCommandBuffer, 0, &ubo, sizeof(ModelUBO));

    // 描画
    SDL_DrawGPUIndexedPrimitives(renderPass, model.GetDrawableVertexIndexCount(index), 1, 0, 0, 0);
}

void CubismRenderer_SDL3::ExecuteDrawForMask(const CubismModel& model, const csmInt32 index, SDL_GPURenderPass* renderPass)
{
    if (renderPass == nullptr)
    {
        return;
    }

    csmUint32 shaderIndex = ShaderNames_SetupMask;
    csmUint32 blendIndex = Blend_Mask;

    SDL_GPUGraphicsPipeline* pipeline = CubismPipeline_SDL3::GetInstance()->GetPipeline(shaderIndex, blendIndex);
    if (pipeline == nullptr)
    {
        return;
    }

    ModelUBO ubo;

    // クリッピング用行列の設定
    UpdateMatrix(ubo.clipMatrix, GetClippingContextBufferForMask()->_matrixForMask);

    // カラーチャンネルの設定
    SetColorChannel(ubo, GetClippingContextBufferForMask());

    // MVP行列の設定
    UpdateMatrix(ubo.projectionMatrix, GetMvpMatrix());

    // 色定数バッファの設定
    csmRectF* rect = GetClippingContextBufferForMask()->_layoutBounds;
    CubismTextureColor baseColor = {rect->X * 2.0f - 1.0f, rect->Y * 2.0f - 1.0f,
                                    rect->GetRight() * 2.0f - 1.0f, rect->GetBottom() * 2.0f - 1.0f};
    CubismTextureColor multiplyColor = model.GetMultiplyColor(index);
    CubismTextureColor screenColor = model.GetScreenColor(index);
    SetColorUniformBuffer(ubo, baseColor, multiplyColor, screenColor);

    // パイプラインのバインド
    SDL_BindGPUGraphicsPipeline(renderPass, pipeline);

    // 頂点バッファとインデックスバッファのバインド
    BindVertexAndIndexBuffers(index, renderPass, DrawableObjectType_Drawable);

    // テクスチャインデックス取得
    csmInt32 textureIndex = model.GetDrawableTextureIndex(index);

    // テクスチャのバインド（パイプラインは3つのフラグメントサンプラーを宣言しているため、常に3つバインドする）
    if (textureIndex >= 0 && textureIndex < static_cast<csmInt32>(_textures.GetSize()))
    {
        SDL_GPUTextureSamplerBinding textureSamplerBindings[3] = {};

        // s_texture0: メインテクスチャ
        textureSamplerBindings[0].texture = _textures[textureIndex].GetTexture();
        textureSamplerBindings[0].sampler = _textures[textureIndex].GetSampler();

        // s_texture1, s_blendTexture: 未使用なのでダミーとしてメインテクスチャをバインド
        textureSamplerBindings[1] = textureSamplerBindings[0];
        textureSamplerBindings[2] = textureSamplerBindings[0];

        SDL_BindGPUFragmentSamplers(renderPass, 0, textureSamplerBindings, 3);
    }

    // ユニフォームバッファをプッシュ
    SDL_PushGPUVertexUniformData(_activeCommandBuffer, 0, &ubo, sizeof(ModelUBO));
    SDL_PushGPUFragmentUniformData(_activeCommandBuffer, 0, &ubo, sizeof(ModelUBO));

    // 描画
    SDL_DrawGPUIndexedPrimitives(renderPass, model.GetDrawableVertexIndexCount(index), 1, 0, 0, 0);
}

void CubismRenderer_SDL3::ExecuteDrawForOffscreen(const CubismModel& model, CubismOffscreenRenderTarget_SDL3* offscreen, SDL_GPURenderPass* renderPass)
{
    if (renderPass == nullptr || offscreen == nullptr)
    {
        return;
    }

    csmInt32 offscreenIndex = offscreen->GetOffscreenIndex();

    // パイプラインレイアウト設定用のインデックスを取得
    csmUint32 blendIndex = 0;
    csmUint32 shaderIndex = 0;
    const csmBool masked = GetClippingContextBufferForOffscreen() != nullptr;
    const csmBool invertedMask = model.GetOffscreenInvertedMask(offscreenIndex);
    const csmInt32 usePremultipliedAlpha = 3; // オフスクリーンはPremultipliedAlphaを使用
    const csmInt32 offset = (masked ? (invertedMask ? 2 : 1) : 0) + usePremultipliedAlpha;
    const csmInt32 shaderNameBegin = GetShaderNamesBegin(model.GetOffscreenBlendModeType(offscreenIndex));

    switch (shaderNameBegin)
    {
    default:
        shaderIndex = shaderNameBegin + offset;
        blendIndex = 0;
        break;
    case ShaderNames_Normal:
        shaderIndex = ShaderNames_Normal + offset;
        blendIndex = CompatibleBlend::Blend_Normal;
        break;
    case ShaderNames_Add:
        shaderIndex = ShaderNames_Add + offset;
        blendIndex = CompatibleBlend::Blend_Add;
        break;
    case ShaderNames_Mult:
        shaderIndex = ShaderNames_Mult + offset;
        blendIndex = CompatibleBlend::Blend_Mult;
        break;
    }

    SDL_GPUGraphicsPipeline* pipeline = CubismPipeline_SDL3::GetInstance()->GetPipeline(shaderIndex, blendIndex);
    if (pipeline == nullptr)
    {
        return;
    }

    ModelUBO ubo;

    if (masked)
    {
        UpdateMatrix(ubo.clipMatrix, GetClippingContextBufferForOffscreen()->_matrixForDraw);
        SetColorChannel(ubo, GetClippingContextBufferForOffscreen());
    }

    // MVP行列の設定（オフスクリーンは単位行列）
    CubismMatrix44 mvpMatrix;
    mvpMatrix.LoadIdentity();
    UpdateMatrix(ubo.projectionMatrix, mvpMatrix);

    // 色定数バッファの設定
    csmFloat32 offscreenOpacity = model.GetOffscreenOpacity(offscreenIndex);
    CubismTextureColor baseColor = CubismTextureColor(offscreenOpacity, offscreenOpacity, offscreenOpacity, offscreenOpacity);
    CubismTextureColor multiplyColor = model.GetMultiplyColorOffscreen(offscreenIndex);
    CubismTextureColor screenColor = model.GetScreenColorOffscreen(offscreenIndex);
    SetColorUniformBuffer(ubo, baseColor, multiplyColor, screenColor);

    // パイプラインのバインド
    SDL_BindGPUGraphicsPipeline(renderPass, pipeline);

    // 頂点バッファとインデックスバッファのバインド
    BindVertexAndIndexBuffers(offscreenIndex, renderPass, DrawableObjectType_Offscreen);

    // テクスチャのバインド（パイプラインは3つのフラグメントサンプラーを宣言しているため、常に3つバインドする）
    SDL_GPUTextureSamplerBinding textureSamplerBindings[3] = {};

    const CubismRenderTarget_SDL3* renderTarget = offscreen->GetRenderTarget();
    if (renderTarget != nullptr)
    {
        // s_texture0: メインテクスチャ
        textureSamplerBindings[0].texture = renderTarget->GetTexture();
        textureSamplerBindings[0].sampler = renderTarget->GetTextureSampler();

        // s_texture1: マスクテクスチャ
        if (masked && GetClippingContextBufferForOffscreen() != nullptr)
        {
            CubismRenderTarget_SDL3* maskBuffer = GetOffscreenMaskBuffer(_commandBufferCurrent,
                                                                          GetClippingContextBufferForOffscreen()->_bufferIndex);
            if (maskBuffer != nullptr)
            {
                textureSamplerBindings[1].texture = maskBuffer->GetTexture();
                textureSamplerBindings[1].sampler = maskBuffer->GetTextureSampler();
            }
        }
        else
        {
             // 未使用時はダミーとしてメインテクスチャをバインド
            textureSamplerBindings[1] = textureSamplerBindings[0];
        }

        // s_blendTexture: ブレンドテクスチャ
        // ブレンドモード有効時は _blendReadImage（レンダーターゲットのスナップショット）をバインドする。
        // レンダーターゲット自体をバインドするとレイアウト競合が発生するため、
        // 事前にBlitBlendSnapshotで別テクスチャにコピーしたものを使用する。
        if (model.IsBlendModeEnabled() && _blendReadImage.IsValid())
        {
            textureSamplerBindings[2].texture = _blendReadImage.GetTexture();
            textureSamplerBindings[2].sampler = _blendReadImage.GetSampler();
        }
        else
        {
             // 未使用時はダミーとしてメインテクスチャをバインド
            textureSamplerBindings[2] = textureSamplerBindings[0];
        }

        SDL_BindGPUFragmentSamplers(renderPass, 0, textureSamplerBindings, 3);
    }

    // ユニフォームバッファをプッシュ
    SDL_PushGPUVertexUniformData(_activeCommandBuffer, 0, &ubo, sizeof(ModelUBO));
    SDL_PushGPUFragmentUniformData(_activeCommandBuffer, 0, &ubo, sizeof(ModelUBO));

    // 描画
    SDL_DrawGPUIndexedPrimitives(renderPass, sizeof(modelRenderTargetIndexArray) / sizeof(csmUint16), 1, 0, 0, 0);
}

void CubismRenderer_SDL3::ExecuteDrawForRenderTarget(const CubismRenderTarget_SDL3* srcBuffer, SDL_GPURenderPass* renderPass)
{
    if (renderPass == nullptr || srcBuffer == nullptr)
    {
        return;
    }

    csmFloat32 uboBaseColor[4];
    CubismTextureColor baseColor = GetModelColor();
    baseColor.R *= baseColor.A;
    baseColor.G *= baseColor.A;
    baseColor.B *= baseColor.A;
    UpdateColor(uboBaseColor, baseColor.R, baseColor.G, baseColor.B, baseColor.A);

    // パイプラインのバインド
    SDL_GPUGraphicsPipeline* pipeline = CubismPipeline_SDL3::GetInstance()->GetPipeline(ShaderNames_Copy, 0);
    if (pipeline == nullptr)
    {
        return;
    }
    SDL_BindGPUGraphicsPipeline(renderPass, pipeline);

    // 頂点バッファとインデックスバッファのバインド
    SDL_GPUBufferBinding vertexBinding = {};
    vertexBinding.buffer = _copyVertexBuffer[_commandBufferCurrent].GetBuffer();
    vertexBinding.offset = 0;
    SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBinding, 1);

    SDL_GPUBufferBinding indexBinding = {};
    indexBinding.buffer = _copyIndexBuffer[_commandBufferCurrent].GetBuffer();
    indexBinding.offset = 0;
    SDL_BindGPUIndexBuffer(renderPass, &indexBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    // テクスチャのバインド
    SDL_GPUTextureSamplerBinding textureSamplerBinding = {};
    textureSamplerBinding.texture = srcBuffer->GetTexture();
    textureSamplerBinding.sampler = srcBuffer->GetTextureSampler();
    SDL_BindGPUFragmentSamplers(renderPass, 0, &textureSamplerBinding, 1);

    // ビューポート設定
    SDL_GPUViewport viewport = GetViewport(static_cast<csmFloat32>(s_renderWidth),
                                           static_cast<csmFloat32>(s_renderHeight), 0.0f, 1.0f);
    SDL_SetGPUViewport(renderPass, &viewport);

    // シザー設定
    SDL_Rect scissor = GetScissor(0.0f, 0.0f, static_cast<csmFloat32>(s_renderWidth),
                                  static_cast<csmFloat32>(s_renderHeight));
    SDL_SetGPUScissor(renderPass, &scissor);

    // ユニフォームバッファをプッシュ
    SDL_PushGPUFragmentUniformData(_activeCommandBuffer, 0, &uboBaseColor, sizeof(csmFloat32) * 4);

    // 描画
    SDL_DrawGPUIndexedPrimitives(renderPass, sizeof(modelRenderTargetIndexArray) / sizeof(csmUint16), 1, 0, 0, 0);
}

void CubismRenderer_SDL3::DrawMeshSDL3(const CubismModel& model, const csmInt32 index,
                                        SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass)
{
    if (s_device == nullptr)
    {
        return;
    }

    // コマンドバッファをメンバ変数に保持（SDL_PushGPU*UniformData用）
    _activeCommandBuffer = commandBuffer;

    if (model.GetDrawableVertexIndexCount(index) == 0)
    {
        return;
    }

    if (model.GetDrawableOpacity(index) <= 0.0f && GetClippingContextBufferForMask() == nullptr)
    {
        return;
    }

    csmInt32 textureIndex = model.GetDrawableTextureIndices(index);
    if (textureIndex < 0 || textureIndex >= static_cast<csmInt32>(_textures.GetSize()))
    {
        return;
    }

    if (_textures[textureIndex].GetSampler() == nullptr || _textures[textureIndex].GetTexture() == nullptr)
    {
        return;
    }

    if (GetClippingContextBufferForMask() != nullptr) // マスク生成時
    {
        ExecuteDrawForMask(model, index, renderPass);
    }
    else
    {
        ExecuteDrawForDrawable(model, index, renderPass);
    }

    SetClippingContextBufferForDraw(nullptr);
    SetClippingContextBufferForMask(nullptr);
}

void CubismRenderer_SDL3::BindVertexAndIndexBuffers(const csmInt32 index, SDL_GPURenderPass* renderPass, DrawableObjectType drawableObjectType)
{
    SDL_GPUBuffer* vertexBuffer = nullptr;
    SDL_GPUBuffer* indexBuffer = nullptr;

    switch (drawableObjectType)
    {
    case DrawableObjectType_Drawable:
        if (index < _vertexBuffers[_commandBufferCurrent].GetSize())
        {
            vertexBuffer = _vertexBuffers[_commandBufferCurrent][index].GetBuffer();
            indexBuffer = _indexBuffers[_commandBufferCurrent][index].GetBuffer();
        }
        break;
    case DrawableObjectType_Offscreen:
        if (index < _offscreenVertexBuffers[_commandBufferCurrent].GetSize())
        {
            vertexBuffer = _offscreenVertexBuffers[_commandBufferCurrent][index].GetBuffer();
            indexBuffer = _offscreenIndexBuffers[_commandBufferCurrent][index].GetBuffer();
        }
        break;
    }

    if (vertexBuffer != nullptr)
    {
        SDL_GPUBufferBinding vertexBinding = {};
        vertexBinding.buffer = vertexBuffer;
        vertexBinding.offset = 0;
        SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBinding, 1);
    }

    if (indexBuffer != nullptr)
    {
        SDL_GPUBufferBinding indexBinding = {};
        indexBinding.buffer = indexBuffer;
        indexBinding.offset = 0;
        SDL_BindGPUIndexBuffer(renderPass, &indexBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    }
}

void CubismRenderer_SDL3::SetColorChannel(ModelUBO& ubo, CubismClippingContext_SDL3* contextBuffer)
{
    if (contextBuffer != nullptr)
    {
        const csmInt32 channelIndex = contextBuffer->_layoutChannelIndex;
        CubismTextureColor* channelColor = contextBuffer->GetClippingManager()->GetChannelFlagAsColor(channelIndex);
        UpdateColor(ubo.channelFlag, channelColor->R, channelColor->G, channelColor->B, channelColor->A);
    }
    else
    {
        UpdateColor(ubo.channelFlag, 1.0f, 0.0f, 0.0f, 0.0f);
    }
}

void CubismRenderer_SDL3::CopyRenderTarget(const CubismRenderTarget_SDL3* src, SDL_GPUCommandBuffer* commandBuffer)
{
    if (src == nullptr || commandBuffer == nullptr)
    {
        return;
    }

    _activeCommandBuffer = commandBuffer;
    SDL_GPURenderPass* renderPass = BeginRendering(commandBuffer, true);
    if (renderPass == nullptr)
    {
        CubismLogError("CopyRenderTarget: Failed to begin render pass: %s", SDL_GetError());
        return;
    }

    ExecuteDrawForRenderTarget(src, renderPass);

    EndRendering(renderPass);
}

void CubismRenderer_SDL3::DrawObjectLoop(SDL_GPUCommandBuffer* commandBuffer)
{
    // 深度バッファのサイズを更新する
    if (_depthImage.GetWidth() != static_cast<csmInt32>(s_renderWidth) ||
        _depthImage.GetHeight() != static_cast<csmInt32>(s_renderHeight))
    {
        _depthImage.Destroy(s_device);
        CreateDepthBuffer();
    }

    const csmInt32 drawableCount = GetModel()->GetDrawableCount();
    const csmInt32 offscreenCount = GetModel()->GetOffscreenCount();
    const csmInt32 totalCount = drawableCount + offscreenCount;
    const csmInt32* renderOrder = GetModel()->GetRenderOrders();

    _currentOffscreen = nullptr;
    _currentRenderTarget = nullptr;

    // 描画オブジェクトのリサイズ
    if (_sortedObjectsIndexList.GetSize() != static_cast<csmUint32>(totalCount))
    {
        _sortedObjectsIndexList.Resize(totalCount, 0);
        _sortedObjectsTypeList.Resize(totalCount, DrawableObjectType_Drawable);
    }

    // インデックスを描画順でソート
    for (csmInt32 i = 0; i < totalCount; ++i)
    {
        const csmInt32 order = renderOrder[i];

        if (i < drawableCount)
        {
            _sortedObjectsIndexList[order] = i;
            _sortedObjectsTypeList[order] = DrawableObjectType_Drawable;
        }
        else if (i < totalCount)
        {
            _sortedObjectsIndexList[order] = i - drawableCount;
            _sortedObjectsTypeList[order] = DrawableObjectType_Offscreen;
        }
    }

    // コマンドバッファを保持（SetupClippingContext内のExecuteDrawForMaskでも使用される）
    _activeCommandBuffer = commandBuffer;

    // 通常方式のクリッピングマスクのセットアップ（レンダーパス開始前に実施）
    if (!IsUsingHighPrecisionMask())
    {
        if (_drawableClippingManager != nullptr)
        {
            _drawableClippingManager->SetupClippingContext(*GetModel(), commandBuffer, this, _commandBufferCurrent, DrawableObjectType_Drawable);
        }
        if (_offscreenClippingManager != nullptr)
        {
            _offscreenClippingManager->SetupClippingContext(*GetModel(), commandBuffer, this, _commandBufferCurrent, DrawableObjectType_Offscreen);
        }
    }

    // BeginRenderTargetでブレンドモードに応じたレンダーパスを開始
    // 外部コマンドバッファ使用時は既にスプライト等が描画済みなのでLOADOP_LOADを使用
    const csmBool isResume = (s_externalCommandBuffer != nullptr);
    SDL_GPURenderPass* currentRenderPass = BeginRenderTarget(commandBuffer, isResume);
    if (currentRenderPass == nullptr)
    {
        CubismLogError("Failed to begin render pass: %s", SDL_GetError());
        if (s_externalCommandBuffer == nullptr)
        {
            SDL_SubmitGPUCommandBuffer(commandBuffer);
        }
        return;
    }

    // ビューポート設定
    SDL_GPUViewport viewport = GetViewport(static_cast<csmFloat32>(s_renderWidth),
                                           static_cast<csmFloat32>(s_renderHeight), 0.0f, 1.0f);
    SDL_SetGPUViewport(currentRenderPass, &viewport);

    // シザー設定
    SDL_Rect scissor = GetScissor(0.0f, 0.0f, static_cast<csmFloat32>(s_renderWidth),
                                  static_cast<csmFloat32>(s_renderHeight));
    SDL_SetGPUScissor(currentRenderPass, &scissor);

    // 描画
    for (csmInt32 i = 0; i < totalCount; ++i)
    {
        const csmInt32 objectIndex = _sortedObjectsIndexList[i];
        const csmInt32 objectType = _sortedObjectsTypeList[i];

        RenderObject(objectIndex, objectType, commandBuffer, currentRenderPass);
    }

    // オフスクリーンが残っている場合は親オフスクリーンへの伝搬を行う
    while (_currentOffscreen != nullptr)
    {
        SubmitDrawToParentOffscreen(_currentOffscreen->GetOffscreenIndex(), DrawableObjectType_Offscreen,
                                    commandBuffer, currentRenderPass);
    }

    // EndRenderTargetでレンダーパス終了（ブレンドモード時はコピーバックも行う）
    EndRenderTarget(commandBuffer, currentRenderPass);

    // 外部コマンドバッファの場合はサブミットしない（メインレンダーループがサブミットする）
    if (s_externalCommandBuffer == nullptr)
    {
        SDL_SubmitGPUCommandBuffer(commandBuffer);
    }

    PostDraw();
}

void CubismRenderer_SDL3::RenderObject(csmInt32 objectIndex, csmInt32 objectType,
                                        SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass*& renderPass)
{
    switch (objectType)
    {
    case DrawableObjectType_Drawable:
        DrawDrawable(objectIndex, commandBuffer, renderPass);
        break;
    case DrawableObjectType_Offscreen:
        AddOffscreen(objectIndex, commandBuffer, renderPass);
        break;
    default:
        CubismLogError("Unknown drawable type: %d", objectType);
        break;
    }
}

void CubismRenderer_SDL3::DrawDrawable(csmInt32 drawableIndex, SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass*& renderPass)
{
    // Drawableが表示状態でなければ処理をパスする
    if (!GetModel()->GetDrawableDynamicFlagIsVisible(drawableIndex))
    {
        return;
    }

    SubmitDrawToParentOffscreen(drawableIndex, DrawableObjectType_Drawable, commandBuffer, renderPass);

    // クリッピングマスク
    CubismClippingContext_SDL3* clipContext = (_drawableClippingManager != nullptr)
        ? (*_drawableClippingManager->GetClippingContextListForDraw())[drawableIndex]
        : nullptr;

    // ブレンドモードのDrawableかどうか判定する。
    // ブレンドシェーダー（FragShaderSrcBlend系）はs_blendTextureを読み取るため、
    // 描画前にレンダーターゲットのスナップショットが必要。
    csmBool needsBlendSnapshot = false;
    if (GetModel()->IsBlendModeEnabled())
    {
        const csmInt32 shaderNameBegin = GetShaderNamesBegin(GetModel()->GetDrawableBlendModeType(drawableIndex));
        needsBlendSnapshot = (shaderNameBegin != ShaderNames_Normal &&
                              shaderNameBegin != ShaderNames_Add &&
                              shaderNameBegin != ShaderNames_Mult);
    }

    if (clipContext != nullptr && IsUsingHighPrecisionMask())
    {
        if (clipContext->_isUsing)
        {
            CubismRenderTarget_SDL3* currentHighPrecisionMaskColorBuffer =
                &_drawableMaskBuffers[_commandBufferCurrent][clipContext->_bufferIndex];

            // 現在のレンダーパスを終了して新しいマスク用レンダーパスを開始
            if (_currentRenderTarget != nullptr)
            {
                _currentRenderTarget->EndDraw(renderPass);
            }
            else
            {
                SDL_EndGPURenderPass(renderPass);
            }

            // CubismRenderTarget_SDL3::BeginDraw を使用してdepth stencilを含むrender passを作成する。
            // パイプラインは has_depth_stencil_target=true で作成されているため、
            // render passもdepth stencilを持つ必要がある（VUID-vkCmdDrawIndexed-renderPass-02684対策）。
            SDL_GPURenderPass* maskRenderPass = currentHighPrecisionMaskColorBuffer->BeginDraw(commandBuffer, 1.0f, 1.0f, 1.0f, 1.0f, true);
            if (maskRenderPass != nullptr)
            {
                // ビューポート設定
                SDL_GPUViewport viewport = GetViewport(
                    static_cast<csmFloat32>(_drawableClippingManager->GetClippingMaskBufferSize().X),
                    static_cast<csmFloat32>(_drawableClippingManager->GetClippingMaskBufferSize().Y),
                    0.0f, 1.0f);
                SDL_SetGPUViewport(maskRenderPass, &viewport);

                SDL_Rect scissor = GetScissor(
                    0.0f, 0.0f,
                    static_cast<csmFloat32>(_drawableClippingManager->GetClippingMaskBufferSize().X),
                    static_cast<csmFloat32>(_drawableClippingManager->GetClippingMaskBufferSize().Y));
                SDL_SetGPUScissor(maskRenderPass, &scissor);

                const csmInt32 clipDrawCount = clipContext->_clippingIdCount;
                for (csmInt32 ctx = 0; ctx < clipDrawCount; ctx++)
                {
                    const csmInt32 clipDrawIndex = clipContext->_clippingIdList[ctx];

                    if (!GetModel()->GetDrawableDynamicFlagVertexPositionsDidChange(clipDrawIndex))
                    {
                        continue;
                    }

                    IsCulling(GetModel()->GetDrawableCulling(clipDrawIndex) != 0);
                    SetClippingContextBufferForMask(clipContext);
                    DrawMeshSDL3(*GetModel(), clipDrawIndex, commandBuffer, maskRenderPass);
                }

                currentHighPrecisionMaskColorBuffer->EndDraw(maskRenderPass);
            }

            SetClippingContextBufferForMask(nullptr);

            // マスク用レンダーパス終了後、レンダーパスが非アクティブな状態。
            // ブレンドシェーダー用のスナップショットをここで取得する（追加のレンダーパスブレイク不要）。
            if (needsBlendSnapshot && _blendReadImage.IsValid())
            {
                CubismRenderTarget_SDL3* blendSrc = (_currentRenderTarget != nullptr)
                    ? _currentRenderTarget : GetModelRenderTarget();
                BlitBlendSnapshot(commandBuffer, blendSrc);
                needsBlendSnapshot = false; // 処理済み
            }

            // 元のレンダーパスを再開
            // Vulkanレンダラーに合わせて、ブレンドモード有効時は _currentRenderTarget（モデルレンダーターゲット）、
            // 無効時は s_renderTexture（スワップチェーン）に復帰する。
            if (_currentRenderTarget != nullptr)
            {
                // ブレンドモード有効時: モデルレンダーターゲットまたはオフスクリーンレンダーターゲットに復帰
                renderPass = _currentRenderTarget->BeginDraw(commandBuffer, 0.0f, 0.0f, 0.0f, 0.0f, false);
            }
            else
            {
                // 通常時: スワップチェーンに復帰
                renderPass = BeginRendering(commandBuffer, true);
            }

            // ビューポートを戻す
            const csmFloat32 restoreWidth = (_currentRenderTarget != nullptr)
                ? static_cast<csmFloat32>(_modelRenderTargetWidth)
                : static_cast<csmFloat32>(s_renderWidth);
            const csmFloat32 restoreHeight = (_currentRenderTarget != nullptr)
                ? static_cast<csmFloat32>(_modelRenderTargetHeight)
                : static_cast<csmFloat32>(s_renderHeight);

            SDL_GPUViewport viewport = GetViewport(
                restoreWidth,
                restoreHeight,
                0.0f, 1.0f);
            SDL_SetGPUViewport(renderPass, &viewport);

            SDL_Rect scissor = GetScissor(
                0.0f, 0.0f,
                restoreWidth,
                restoreHeight);
            SDL_SetGPUScissor(renderPass, &scissor);
        }
    }

    // マスク無しのブレンドDrawableの場合、レンダーパスを一旦終了してスナップショットを取得する。
    if (needsBlendSnapshot && _blendReadImage.IsValid())
    {
        // 現在のレンダーパスを終了
        if (_currentRenderTarget != nullptr)
        {
            _currentRenderTarget->EndDraw(renderPass);
        }
        else
        {
            SDL_EndGPURenderPass(renderPass);
        }

        // レンダーターゲットのスナップショットを _blendReadImage にコピー
        CubismRenderTarget_SDL3* blendSrc = (_currentRenderTarget != nullptr)
            ? _currentRenderTarget : GetModelRenderTarget();
        BlitBlendSnapshot(commandBuffer, blendSrc);

        // レンダーパスを再開（LOAD_OPで既存内容を保持）
        if (_currentRenderTarget != nullptr)
        {
            renderPass = _currentRenderTarget->BeginDraw(commandBuffer, 0.0f, 0.0f, 0.0f, 0.0f, false);
        }
        else
        {
            renderPass = BeginRendering(commandBuffer, true);
        }

        // ビューポートを戻す
        const csmFloat32 restoreWidth = (_currentRenderTarget != nullptr)
            ? static_cast<csmFloat32>(_modelRenderTargetWidth)
            : static_cast<csmFloat32>(s_renderWidth);
        const csmFloat32 restoreHeight = (_currentRenderTarget != nullptr)
            ? static_cast<csmFloat32>(_modelRenderTargetHeight)
            : static_cast<csmFloat32>(s_renderHeight);

        SDL_GPUViewport viewport = GetViewport(restoreWidth, restoreHeight, 0.0f, 1.0f);
        SDL_SetGPUViewport(renderPass, &viewport);

        SDL_Rect scissor = GetScissor(0.0f, 0.0f, restoreWidth, restoreHeight);
        SDL_SetGPUScissor(renderPass, &scissor);
    }

    // クリッピングマスクをセットする
    SetClippingContextBufferForDraw(clipContext);
    IsCulling(GetModel()->GetDrawableCulling(drawableIndex) != 0);
    DrawMeshSDL3(*GetModel(), drawableIndex, commandBuffer, renderPass);
}

void CubismRenderer_SDL3::SubmitDrawToParentOffscreen(csmInt32 objectIndex, DrawableObjectType objectType,
                                                       SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass*& renderPass)
{
    if (_currentOffscreen == nullptr ||
        objectIndex == CubismModel::CubismNoIndex_Offscreen)
    {
        return;
    }

    csmInt32 currentOwnerIndex = GetModel()->GetOffscreenOwnerIndices()[_currentOffscreen->GetOffscreenIndex()];

    if (currentOwnerIndex == CubismModel::CubismNoIndex_Offscreen)
    {
        return;
    }

    csmInt32 targetParentIndex = CubismModel::CubismNoIndex_Parent;
    switch (objectType)
    {
    case DrawableObjectType_Drawable:
        targetParentIndex = GetModel()->GetDrawableParentPartIndex(objectIndex);
        break;
    case DrawableObjectType_Offscreen:
        targetParentIndex = GetModel()->GetPartParentPartIndex(GetModel()->GetOffscreenOwnerIndices()[objectIndex]);
        break;
    default:
        return;
    }

    // 階層を辿って現在のオフスクリーンのオーナーのパーツがいたら処理を終了する
    while (targetParentIndex != CubismModel::CubismNoIndex_Parent)
    {
        if (targetParentIndex == currentOwnerIndex)
        {
            return;
        }
        targetParentIndex = GetModel()->GetPartParentPartIndex(targetParentIndex);
    }

    // 現オフスクリーンの描画対象は全て描画完了しているので現オフスクリーンを描画する
    DrawOffscreen(_currentOffscreen, commandBuffer, renderPass);

    // さらに親のオフスクリーンに伝搬可能なら伝搬する
    SubmitDrawToParentOffscreen(objectIndex, objectType, commandBuffer, renderPass);
}

void CubismRenderer_SDL3::AddOffscreen(csmInt32 offscreenIndex, SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass*& renderPass)
{
    // 以前のオフスクリーンレンダリングターゲットを親に伝搬する処理を追加する
    if (_currentOffscreen != nullptr && _currentOffscreen->GetOffscreenIndex() != offscreenIndex)
    {
        csmBool isParent = false;
        csmInt32 ownerIndex = GetModel()->GetOffscreenOwnerIndices()[offscreenIndex];
        csmInt32 parentIndex = GetModel()->GetPartParentPartIndex(ownerIndex);

        csmInt32 currentOffscreenIndex = _currentOffscreen->GetOffscreenIndex();
        csmInt32 currentOffscreenOwnerIndex = GetModel()->GetOffscreenOwnerIndices()[currentOffscreenIndex];

        while (parentIndex != CubismModel::CubismNoIndex_Parent)
        {
            if (parentIndex == currentOffscreenOwnerIndex)
            {
                isParent = true;
                break;
            }
            parentIndex = GetModel()->GetPartParentPartIndex(parentIndex);
        }

        if (!isParent)
        {
            // 現在のオフスクリーンレンダリングターゲットがあるなら、親に伝搬する
            SubmitDrawToParentOffscreen(offscreenIndex, DrawableObjectType_Offscreen,
                                        commandBuffer, renderPass);
        }
    }

    if (offscreenIndex >= static_cast<csmInt32>(_offscreenList.GetSize()))
    {
        return;
    }

    CubismOffscreenRenderTarget_SDL3* offscreen = &_offscreenList[offscreenIndex];
    offscreen->SetOffscreenRenderTarget(s_device, _modelRenderTargetWidth, _modelRenderTargetHeight,
                                         s_colorFormat, s_depthFormat);

    // 以前のオフスクリーンレンダリングターゲットを取得
    CubismOffscreenRenderTarget_SDL3* oldOffscreen = offscreen->GetParentPartOffscreen();
    offscreen->SetOldOffscreen(oldOffscreen);

    // 現在のオフスクリーンレンダリングターゲットを設定
    // ※ _currentRenderTarget を上書きする前に保存しておき、EndDraw は前の RT に対して呼ぶ。
    //   上書き後に EndDraw すると前の RT の _isRendering がリセットされず、
    //   後続の BeginDraw が「already in rendering state」で失敗する。
    CubismRenderTarget_SDL3* prevRenderTarget = _currentRenderTarget;
    _currentOffscreen = offscreen;
    _currentRenderTarget = offscreen->GetRenderTarget();

    // 以前のレンダーパスを終了してオフスクリーンレンダーパスを開始
    if (prevRenderTarget != nullptr)
    {
        prevRenderTarget->EndDraw(renderPass);
    }
    else
    {
        SDL_EndGPURenderPass(renderPass);
    }
    renderPass = offscreen->GetRenderTarget()->BeginDraw(commandBuffer, 0.0f, 0.0f, 0.0f, 0.0f, true);
    if (renderPass == nullptr)
    {
        CubismLogError("AddOffscreen: Failed to begin offscreen render pass");
        return;
    }

    // ビューポートとシザーをオフスクリーンサイズに設定
    SDL_GPUViewport offscreenViewport = GetViewport(
        static_cast<csmFloat32>(_modelRenderTargetWidth),
        static_cast<csmFloat32>(_modelRenderTargetHeight), 0.0f, 1.0f);
    SDL_SetGPUViewport(renderPass, &offscreenViewport);

    SDL_Rect offscreenScissor = GetScissor(0.0f, 0.0f,
        static_cast<csmFloat32>(_modelRenderTargetWidth),
        static_cast<csmFloat32>(_modelRenderTargetHeight));
    SDL_SetGPUScissor(renderPass, &offscreenScissor);
}

void CubismRenderer_SDL3::DrawOffscreen(CubismOffscreenRenderTarget_SDL3* currentOffscreen,
                                         SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass*& renderPass)
{
    if (currentOffscreen == nullptr)
    {
        return;
    }

    // 現在のオフスクリーンレンダーパスを終了
    CubismRenderTarget_SDL3* renderTarget = currentOffscreen->GetRenderTarget();
    if (renderTarget != nullptr)
    {
        renderTarget->EndDraw(renderPass);
    }
    else
    {
        SDL_EndGPURenderPass(renderPass);
    }
    renderPass = nullptr;

    CubismOffscreenRenderTarget_SDL3* parentOffscreen = currentOffscreen->GetOldOffscreen();
    _currentOffscreen = parentOffscreen;

    // クリッピングマスク
    const csmInt32 offscreenIndex = currentOffscreen->GetOffscreenIndex();
    CubismClippingContext_SDL3* clipContext = (_offscreenClippingManager != nullptr)
        ? (*_offscreenClippingManager->GetClippingContextListForOffscreen())[offscreenIndex]
        : nullptr;

    // 高精細マスク時はオフスクリーン用マスクをここで生成する
    if (clipContext != nullptr && IsUsingHighPrecisionMask() && clipContext->_isUsing)
    {
        CubismRenderTarget_SDL3* currentHighPrecisionMaskColorBuffer = &_offscreenMaskBuffers[_commandBufferCurrent][clipContext->_bufferIndex];
        SDL_GPURenderPass* maskRenderPass = currentHighPrecisionMaskColorBuffer->BeginDraw(commandBuffer, 1.0f, 1.0f, 1.0f, 1.0f, true);
        if (maskRenderPass != nullptr)
        {
            SDL_GPUViewport viewport = GetViewport(
                static_cast<csmFloat32>(_offscreenClippingManager->GetClippingMaskBufferSize().X),
                static_cast<csmFloat32>(_offscreenClippingManager->GetClippingMaskBufferSize().Y),
                0.0f, 1.0f);
            SDL_SetGPUViewport(maskRenderPass, &viewport);

            SDL_Rect scissor = GetScissor(
                0.0f, 0.0f,
                static_cast<csmFloat32>(_offscreenClippingManager->GetClippingMaskBufferSize().X),
                static_cast<csmFloat32>(_offscreenClippingManager->GetClippingMaskBufferSize().Y));
            SDL_SetGPUScissor(maskRenderPass, &scissor);

            const csmInt32 clipDrawCount = clipContext->_clippingIdCount;
            for (csmInt32 i = 0; i < clipDrawCount; ++i)
            {
                const csmInt32 clipDrawIndex = clipContext->_clippingIdList[i];

                if (!GetModel()->GetDrawableDynamicFlagVertexPositionsDidChange(clipDrawIndex))
                {
                    continue;
                }

                IsCulling(GetModel()->GetDrawableCulling(clipDrawIndex) != 0);
                SetClippingContextBufferForMask(clipContext);
                DrawMeshSDL3(*GetModel(), clipDrawIndex, commandBuffer, maskRenderPass);
            }

            currentHighPrecisionMaskColorBuffer->EndDraw(maskRenderPass);
            SetClippingContextBufferForMask(nullptr);
        }
    }

    // ブレンドシェーダーを使用するオフスクリーンの場合、親レンダーターゲットのスナップショットを取得する。
    // この時点ではレンダーパスが非アクティブなので、追加のレンダーパスブレイクなしでBlitできる。
    if (GetModel()->IsBlendModeEnabled() && _blendReadImage.IsValid())
    {
        const csmInt32 shaderNameBegin = GetShaderNamesBegin(GetModel()->GetOffscreenBlendModeType(offscreenIndex));
        if (shaderNameBegin != ShaderNames_Normal &&
            shaderNameBegin != ShaderNames_Add &&
            shaderNameBegin != ShaderNames_Mult)
        {
            CubismRenderTarget_SDL3* parentTarget = nullptr;
            if (parentOffscreen != nullptr)
            {
                parentTarget = parentOffscreen->GetRenderTarget();
            }
            else
            {
                parentTarget = &_modelRenderTargets[0];
            }
            BlitBlendSnapshot(commandBuffer, parentTarget);
        }
    }

    // 親のレンダーパスを開始
    if (parentOffscreen != nullptr)
    {
        // 親のオフスクリーンレンダリングターゲットに描画
        _currentRenderTarget = parentOffscreen->GetRenderTarget();
        renderPass = parentOffscreen->GetRenderTarget()->BeginDraw(commandBuffer, 0.0f, 0.0f, 0.0f, 0.0f, false);

        SDL_GPUViewport viewport = GetViewport(
            static_cast<csmFloat32>(_modelRenderTargetWidth),
            static_cast<csmFloat32>(_modelRenderTargetHeight), 0.0f, 1.0f);
        SDL_SetGPUViewport(renderPass, &viewport);

        SDL_Rect scissor = GetScissor(0.0f, 0.0f,
            static_cast<csmFloat32>(_modelRenderTargetWidth),
            static_cast<csmFloat32>(_modelRenderTargetHeight));
        SDL_SetGPUScissor(renderPass, &scissor);
    }
    else
    {
        // 親がないので、モデル描画用レンダーターゲットに復帰
        // Vulkanレンダラーに合わせて、ブレンドモード有効時は _modelRenderTargets[0] に復帰し、
        // 無効時は s_renderTexture（スワップチェーン）に復帰する。
        if (GetModel()->IsBlendModeEnabled())
        {
            // ブレンドモード有効時: モデルレンダーターゲットに復帰
            _currentRenderTarget = &_modelRenderTargets[0];
            renderPass = _modelRenderTargets[0].BeginDraw(commandBuffer, 0.0f, 0.0f, 0.0f, 0.0f, false);

            SDL_GPUViewport viewport = GetViewport(
                static_cast<csmFloat32>(_modelRenderTargetWidth),
                static_cast<csmFloat32>(_modelRenderTargetHeight), 0.0f, 1.0f);
            SDL_SetGPUViewport(renderPass, &viewport);

            SDL_Rect scissor = GetScissor(0.0f, 0.0f,
                static_cast<csmFloat32>(_modelRenderTargetWidth),
                static_cast<csmFloat32>(_modelRenderTargetHeight));
            SDL_SetGPUScissor(renderPass, &scissor);
        }
        else
        {
            // 通常時: スワップチェーンに復帰
            _currentRenderTarget = nullptr;

            renderPass = BeginRendering(commandBuffer, true);

            SDL_GPUViewport viewport = GetViewport(
                static_cast<csmFloat32>(s_renderWidth),
                static_cast<csmFloat32>(s_renderHeight), 0.0f, 1.0f);
            SDL_SetGPUViewport(renderPass, &viewport);

            SDL_Rect scissor = GetScissor(0.0f, 0.0f,
                static_cast<csmFloat32>(s_renderWidth),
                static_cast<csmFloat32>(s_renderHeight));
            SDL_SetGPUScissor(renderPass, &scissor);
        }
    }

    if (renderPass == nullptr)
    {
        CubismLogError("DrawOffscreen: Failed to begin render pass");
        _currentOffscreen = parentOffscreen;
        return;
    }

    // クリッピングコンテキストを設定してオフスクリーンテクスチャを描画
    SetClippingContextBufferForOffscreen(clipContext);

    ExecuteDrawForOffscreen(*GetModel(), currentOffscreen, renderPass);

    // 後処理
    currentOffscreen->StopUsingRenderTexture();
    SetClippingContextBufferForOffscreen(nullptr);

    // 現在のオフスクリーンを親に戻す
    _currentOffscreen = parentOffscreen;
}

}}}}
