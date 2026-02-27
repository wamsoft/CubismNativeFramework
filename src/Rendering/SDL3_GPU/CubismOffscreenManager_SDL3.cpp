/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "CubismOffscreenManager_SDL3.hpp"
#include "CubismFramework.hpp"
#include "Utils/CubismDebug.hpp"

//------------ LIVE2D NAMESPACE ------------
namespace Live2D { namespace Cubism { namespace Framework { namespace Rendering {

namespace {
    CubismOffscreenManager_SDL3* s_instance = nullptr;
}

CubismOffscreenManager_SDL3* CubismOffscreenManager_SDL3::GetInstance()
{
    if (s_instance == nullptr)
    {
        s_instance = new CubismOffscreenManager_SDL3();
    }

    return s_instance;
}

void CubismOffscreenManager_SDL3::ReleaseInstance()
{
    if (s_instance != nullptr)
    {
        delete s_instance;
        s_instance = nullptr;
    }
}

CubismRenderTarget_SDL3* CubismOffscreenManager_SDL3::GetOffscreenRenderTarget(
    SDL_GPUDevice* device,
    csmUint32 displayBufferWidth, csmUint32 displayBufferHeight,
    SDL_GPUTextureFormat colorFormat, SDL_GPUTextureFormat depthFormat
)
{
    // 使用数を更新
    UpdateRenderTargetCount();

    // 使われていないリソースコンテナがあればそれを返す
    CubismRenderTarget_SDL3* offscreenRenderTarget = GetUnusedOffscreenRenderTarget();
    if (offscreenRenderTarget != nullptr)
    {
        // サイズが違う場合は再作成する
        if (offscreenRenderTarget->GetBufferWidth() != displayBufferWidth ||
            offscreenRenderTarget->GetBufferHeight() != displayBufferHeight)
        {
            offscreenRenderTarget->DestroyRenderTarget();
            offscreenRenderTarget->CreateRenderTarget(device, displayBufferWidth,
                displayBufferHeight, colorFormat, depthFormat);
        }
        // 既存の未使用レンダーターゲットを返す
        return offscreenRenderTarget;
    }

    // 新規にレンダーターゲットを作成して登録する
    offscreenRenderTarget = CreateOffscreenRenderTarget();
    offscreenRenderTarget->CreateRenderTarget(device, displayBufferWidth,
        displayBufferHeight, colorFormat, depthFormat);

    return offscreenRenderTarget;
}

CubismOffscreenManager_SDL3::CubismOffscreenManager_SDL3()
{
}

CubismOffscreenManager_SDL3::~CubismOffscreenManager_SDL3()
{
}

}}}}
