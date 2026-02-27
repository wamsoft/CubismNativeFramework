/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "CubismOffscreenRenderTarget_SDL3.hpp"
#include "CubismOffscreenManager_SDL3.hpp"

//------------ LIVE2D NAMESPACE ------------
namespace Live2D { namespace Cubism { namespace Framework { namespace Rendering {

CubismOffscreenRenderTarget_SDL3::CubismOffscreenRenderTarget_SDL3()
{
}

CubismOffscreenRenderTarget_SDL3::~CubismOffscreenRenderTarget_SDL3()
{
}

void CubismOffscreenRenderTarget_SDL3::SetOffscreenRenderTarget(
    SDL_GPUDevice* device,
    csmUint32 displayBufferWidth, csmUint32 displayBufferHeight,
    SDL_GPUTextureFormat colorFormat, SDL_GPUTextureFormat depthFormat
)
{
    if (GetUsingRenderTextureState())
    {

        if (_renderTarget->GetBufferWidth() != displayBufferWidth ||
            _renderTarget->GetBufferHeight() != displayBufferHeight)
        {
            _renderTarget->DestroyRenderTarget();
            _renderTarget->CreateRenderTarget(device, displayBufferWidth,
                displayBufferHeight, colorFormat, depthFormat);
        }
        return;
    }

    _renderTarget = CubismOffscreenManager_SDL3::GetInstance()->GetOffscreenRenderTarget(
        device, displayBufferWidth, displayBufferHeight, colorFormat, depthFormat);
}

csmBool CubismOffscreenRenderTarget_SDL3::GetUsingRenderTextureState() const
{
    return CubismOffscreenManager_SDL3::GetInstance()->GetUsingRenderTextureState(_renderTarget);
}

void CubismOffscreenRenderTarget_SDL3::StopUsingRenderTexture()
{
    CubismOffscreenManager_SDL3::GetInstance()->StopUsingRenderTexture(_renderTarget);
    _renderTarget = nullptr;
}

}}}}
