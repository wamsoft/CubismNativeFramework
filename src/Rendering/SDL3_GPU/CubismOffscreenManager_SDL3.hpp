/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once

#include "../ICubismOffscreenManager.hpp"
#include "CubismRenderTarget_SDL3.hpp"


//------------ LIVE2D NAMESPACE ------------
namespace Live2D { namespace Cubism { namespace Framework { namespace Rendering {

/**
 * @brief  オフスクリーン用のレンダーターゲットを管理するクラス
 */
class CubismOffscreenManager_SDL3 : public ICubismOffscreenManager<CubismRenderTarget_SDL3>
{

public:
    /**
    * @brief   クラスのインスタンス（シングルトン）を返す。
    *           インスタンスが生成されていない場合は内部でインスタンスを生成する。
    *
    * @return  クラスのインスタンス
    */
    static CubismOffscreenManager_SDL3* GetInstance();

    /**
    * @brief   クラスのインスタンス（シングルトン）を解放する。
    *
    */
    static void ReleaseInstance();

    /**
    * @brief 使用可能なレンダーターゲットの取得
    *
    *  @param[in]  device                 -> GPUデバイス
    *  @param[in]  displayBufferWidth     -> オフスクリーンの横幅
    *  @param[in]  displayBufferHeight    -> オフスクリーンの縦幅
    *  @param[in]  colorFormat            -> カラーフォーマット
    *  @param[in]  depthFormat            -> 深度フォーマット
    *
    *  @return 使用可能なレンダーターゲットを返す
    */
    CubismRenderTarget_SDL3* GetOffscreenRenderTarget(
        SDL_GPUDevice* device,
        csmUint32 displayBufferWidth, csmUint32 displayBufferHeight,
        SDL_GPUTextureFormat colorFormat, SDL_GPUTextureFormat depthFormat
    );

private:

    /**
     * @brief   コンストラクタ
     */
    CubismOffscreenManager_SDL3();

    /**
     * @brief   デストラクタ
     */
    ~CubismOffscreenManager_SDL3();

};

}}}}

//------------ LIVE2D NAMESPACE ------------
