/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "CubismRenderTarget_SDL3.hpp"
#include "Utils/CubismDebug.hpp"

//------------ LIVE2D NAMESPACE ------------
namespace Live2D { namespace Cubism { namespace Framework { namespace Rendering {

CubismRenderTarget_SDL3::CubismRenderTarget_SDL3()
    : _device(nullptr)
    , _bufferWidth(0)
    , _bufferHeight(0)
    , _colorImage(nullptr)
    , _depthImage(nullptr)
    , _isRendering(false)
    , _colorFormat(SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM)
{
}

SDL_GPURenderPass* CubismRenderTarget_SDL3::BeginDraw(SDL_GPUCommandBuffer* commandBuffer,
                                                       csmFloat32 r, csmFloat32 g, csmFloat32 b, csmFloat32 a,
                                                       csmBool isClear)
{
    if (!IsValid())
    {
        CubismLogError("Render target is not valid.");
        return nullptr;
    }

    if (_isRendering)
    {
        CubismLogWarning("Render target is already in rendering state.");
        return nullptr;
    }

    // カラーターゲットの設定
    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture = _colorImage->GetTexture();
    colorTarget.mip_level = 0;
    colorTarget.layer_or_depth_plane = 0;
    colorTarget.clear_color.r = r;
    colorTarget.clear_color.g = g;
    colorTarget.clear_color.b = b;
    colorTarget.clear_color.a = a;
    colorTarget.load_op = isClear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;
    colorTarget.resolve_texture = nullptr;
    colorTarget.resolve_mip_level = 0;
    colorTarget.resolve_layer = 0;
    colorTarget.cycle = false;
    colorTarget.cycle_resolve_texture = false;

    // 深度ターゲットの設定
    // Vulkanレンダラーに合わせて深度バッファは常にクリアする。
    // isClearはカラーにのみ影響し、深度は常に1.0にクリアする。
    // これにより、オフスクリーン描画後のメインターゲット復帰時に
    // 前のDrawableの深度値が残ってクワッドが深度テストに失敗する問題を防ぐ。
    SDL_GPUDepthStencilTargetInfo depthTarget = {};
    if (_depthImage != nullptr && _depthImage->IsValid())
    {
        depthTarget.texture = _depthImage->GetTexture();
        depthTarget.clear_depth = 1.0f;
        depthTarget.load_op = SDL_GPU_LOADOP_CLEAR;
        depthTarget.store_op = SDL_GPU_STOREOP_DONT_CARE;
        depthTarget.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
        depthTarget.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
        depthTarget.cycle = false;
        depthTarget.clear_stencil = 0;
    }

    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(
        commandBuffer,
        &colorTarget, 1,
        (_depthImage != nullptr && _depthImage->IsValid()) ? &depthTarget : nullptr
    );

    if (renderPass == nullptr)
    {
        CubismLogError("Failed to begin GPU render pass: %s", SDL_GetError());
        return nullptr;
    }

    _isRendering = true;
    return renderPass;
}

void CubismRenderTarget_SDL3::EndDraw(SDL_GPURenderPass* renderPass)
{
    if (renderPass == nullptr)
    {
        CubismLogWarning("Invalid render pass.");
        return;
    }

    SDL_EndGPURenderPass(renderPass);
    _isRendering = false;
}

void CubismRenderTarget_SDL3::CreateRenderTarget(
    SDL_GPUDevice* device,
    csmUint32 displayBufferWidth, csmUint32 displayBufferHeight,
    SDL_GPUTextureFormat colorFormat, SDL_GPUTextureFormat depthFormat
)
{
    if (device == nullptr)
    {
        CubismLogError("Device is null.");
        return;
    }

    // 既存のリソースがあれば破棄
    DestroyRenderTarget();

    _device = device;
    _bufferWidth = displayBufferWidth;
    _bufferHeight = displayBufferHeight;
    _colorFormat = colorFormat;

    // カラーバッファの作成
    _colorImage = CSM_NEW CubismImageSDL3();
    _colorImage->CreateTexture(
        device,
        static_cast<csmInt32>(displayBufferWidth),
        static_cast<csmInt32>(displayBufferHeight),
        colorFormat,
        SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER
    );

    if (!_colorImage->IsValid())
    {
        CubismLogError("Failed to create color image for render target.");
        CSM_DELETE(_colorImage);
        _colorImage = nullptr;
        return;
    }

    // サンプラーの作成
    _colorImage->CreateSampler(device, 1.0f);

    // 深度バッファの作成（深度フォーマットが指定されている場合）
    if (depthFormat != SDL_GPU_TEXTUREFORMAT_INVALID)
    {
        _depthImage = CSM_NEW CubismImageSDL3();
        _depthImage->CreateTexture(
            device,
            static_cast<csmInt32>(displayBufferWidth),
            static_cast<csmInt32>(displayBufferHeight),
            depthFormat,
            SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET
        );

        if (!_depthImage->IsValid())
        {
            CubismLogWarning("Failed to create depth image for render target.");
            CSM_DELETE(_depthImage);
            _depthImage = nullptr;
        }
    }
}

void CubismRenderTarget_SDL3::DestroyRenderTarget()
{
    if (_device == nullptr)
    {
        return;
    }

    if (_colorImage != nullptr)
    {
        _colorImage->Destroy(_device);
        CSM_DELETE(_colorImage);
        _colorImage = nullptr;
    }

    if (_depthImage != nullptr)
    {
        _depthImage->Destroy(_device);
        CSM_DELETE(_depthImage);
        _depthImage = nullptr;
    }

    _device = nullptr;
    _bufferWidth = 0;
    _bufferHeight = 0;
    _isRendering = false;
}

SDL_GPUTexture* CubismRenderTarget_SDL3::GetTexture() const
{
    return _colorImage != nullptr ? _colorImage->GetTexture() : nullptr;
}

SDL_GPUSampler* CubismRenderTarget_SDL3::GetTextureSampler() const
{
    return _colorImage != nullptr ? _colorImage->GetSampler() : nullptr;
}

csmUint32 CubismRenderTarget_SDL3::GetBufferWidth() const
{
    return _bufferWidth;
}

csmUint32 CubismRenderTarget_SDL3::GetBufferHeight() const
{
    return _bufferHeight;
}

csmBool CubismRenderTarget_SDL3::IsValid() const
{
    return _colorImage != nullptr && _colorImage->IsValid();
}

csmBool CubismRenderTarget_SDL3::IsRendering() const
{
    return _isRendering;
}

SDL_GPUTextureFormat CubismRenderTarget_SDL3::GetColorFormat() const
{
    return _colorFormat;
}

}}}}
