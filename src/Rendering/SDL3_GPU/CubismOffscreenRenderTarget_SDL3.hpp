/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once

#include "../ICubismOffscreenRenderTarget.hpp"
#include "CubismRenderTarget_SDL3.hpp"

//------------ LIVE2D NAMESPACE ------------
namespace Live2D { namespace Cubism { namespace Framework { namespace Rendering {

/**
 * @brief  オフスクリーン用のレンダーターゲットを管理するクラス
 */
class CubismOffscreenRenderTarget_SDL3 : public ICubismOffscreenRenderTarget<CubismOffscreenRenderTarget_SDL3, CubismRenderTarget_SDL3>
{
public:
    /**
     * @brief   コンストラクタ
     */
    CubismOffscreenRenderTarget_SDL3();

    /**
     * @brief   デストラクタ
     */
    ~CubismOffscreenRenderTarget_SDL3();

    /**
     * @brief オフスクリーン描画用レンダーターゲットを設定する
     *
     *  @param[in]  device                 -> GPUデバイス
     *  @param[in]  displayBufferWidth     -> オフスクリーンの横幅
     *  @param[in]  displayBufferHeight    -> オフスクリーンの縦幅
     *  @param[in]  colorFormat            -> カラーフォーマット
     *  @param[in]  depthFormat            -> 深度フォーマット
     */
    void SetOffscreenRenderTarget(
        SDL_GPUDevice* device,
        csmUint32 displayBufferWidth, csmUint32 displayBufferHeight,
        SDL_GPUTextureFormat colorFormat, SDL_GPUTextureFormat depthFormat
    );

    /**
     * @brief レンダーターゲットの使用状態を取得する。
     *
     * @return 使用中はtrue、未使用の場合はfalseを返す。
     */
    csmBool GetUsingRenderTextureState() const;

    /**
     * @brief オフスクリーン描画用レンダーターゲットの使用を終了する。
     */
    void StopUsingRenderTexture();

};

}}}}

//------------ LIVE2D NAMESPACE ------------
