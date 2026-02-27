/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once
#include <SDL3/SDL_gpu.h>
#include "CubismFramework.hpp"
#include "CubismClass_SDL3.hpp"

//------------ LIVE2D NAMESPACE ------------
namespace Live2D { namespace Cubism { namespace Framework { namespace Rendering {

/**
 * @brief  オフスクリーン描画用構造体
 */
class CubismRenderTarget_SDL3
{
public:
    CubismRenderTarget_SDL3();

    /**
     * @brief   レンダリングターゲットへの描画開始
     *
     * @param[in]  commandBuffer     -> コマンドバッファ
     * @param[in]   r                -> 赤(0.0~1.0)
     * @param[in]   g                -> 緑(0.0~1.0)
     * @param[in]   b                -> 青(0.0~1.0)
     * @param[in]   a                -> α(0.0~1.0)
     * @param[in]   isClear          -> レンダーターゲットをクリアするか
     *
     * @return  レンダーパス
     */
    SDL_GPURenderPass* BeginDraw(SDL_GPUCommandBuffer* commandBuffer,
                                 csmFloat32 r, csmFloat32 g, csmFloat32 b, csmFloat32 a,
                                 csmBool isClear);

    /**
     * @brief   描画終了
     *
     *  @param[in]  renderPass -> レンダーパス
     */
    void EndDraw(SDL_GPURenderPass* renderPass);

    /**
     *  @brief  CubismRenderTargetを作成する。
     *
     *  @param[in]  device                 -> GPUデバイス
     *  @param[in]  displayBufferWidth     -> オフスクリーンの横幅
     *  @param[in]  displayBufferHeight    -> オフスクリーンの縦幅
     *  @param[in]  colorFormat            -> カラーフォーマット
     *  @param[in]  depthFormat            -> 深度フォーマット
     */
    void CreateRenderTarget(
        SDL_GPUDevice* device,
        csmUint32 displayBufferWidth, csmUint32 displayBufferHeight,
        SDL_GPUTextureFormat colorFormat, SDL_GPUTextureFormat depthFormat
    );

    /**
     * @brief   CubismRenderTargetの削除
     *
     * @param[in]  device -> GPUデバイス
     */
    void DestroyRenderTarget();

    /**
     * @brief   テクスチャへのアクセッサ
     */
    SDL_GPUTexture* GetTexture() const;

    /**
     * @brief   テクスチャサンプラーへのアクセッサ
     */
    SDL_GPUSampler* GetTextureSampler() const;

    /**
     * @brief   バッファ幅を返す
     */
    csmUint32 GetBufferWidth() const;

    /**
     * @brief   バッファ高さを返す
     */
    csmUint32 GetBufferHeight() const;

    /**
     * @brief   現在有効かどうかを返す
     */
    csmBool IsValid() const;

    /**
     * @brief   レンダリングパスがアクティブかどうかを返す
     */
    csmBool IsRendering() const;

    /**
     * @brief   カラーフォーマットを取得する
     */
    SDL_GPUTextureFormat GetColorFormat() const;

private:
    SDL_GPUDevice* _device; ///< GPUデバイス
    csmUint32 _bufferWidth; ///< オフスクリーンの横幅
    csmUint32 _bufferHeight; ///< オフスクリーンの縦幅
    CubismImageSDL3* _colorImage; ///< カラーバッファ
    CubismImageSDL3* _depthImage; ///< 深度バッファ
    csmBool _isRendering; ///< レンダリングパスがアクティブかどうか
    SDL_GPUTextureFormat _colorFormat; ///< カラーフォーマット
};

}}}}

//------------ LIVE2D NAMESPACE ------------
