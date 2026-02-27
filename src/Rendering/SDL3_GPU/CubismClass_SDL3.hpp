/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once
#include <SDL3/SDL_gpu.h>
#include "CubismFramework.hpp"
#include "Type/csmVector.hpp"

namespace Live2D { namespace Cubism { namespace Framework {

/**
 * @brief   バッファを扱うクラス
 */
class CubismBufferSDL3
{
public:
    /**
     * @brief   コンストラクタ
     */
    CubismBufferSDL3();

    /**
     * @brief   デストラクタ
     */
    ~CubismBufferSDL3();

    /**
     * @brief   バッファを作成する
     *
     * @param[in]  device          -> GPUデバイス
     * @param[in]  size            -> バッファサイズ
     * @param[in]  usageFlags      -> バッファの使用法を指定するビットマスク
     */
    void CreateBuffer(SDL_GPUDevice* device, csmUint32 size, SDL_GPUBufferUsageFlags usageFlags);

    /**
     * @brief   転送バッファを作成する
     *
     * @param[in]  device          -> GPUデバイス
     * @param[in]  size            -> バッファサイズ
     * @param[in]  usage           -> 転送バッファの使用法
     */
    void CreateTransferBuffer(SDL_GPUDevice* device, csmUint32 size, SDL_GPUTransferBufferUsage usage);

    /**
     * @brief   転送バッファをマップし、そのアドレスポインタを取得する
     *
     * @param[in]  device  -> GPUデバイス
     * @param[in]  cycle   -> サイクル
     *
     * @return マップされたメモリへのポインタ
     */
    void* MapTransferBuffer(SDL_GPUDevice* device, csmBool cycle);

    /**
     * @brief   転送バッファのマップを解除する
     *
     * @param[in]  device  -> GPUデバイス
     */
    void UnmapTransferBuffer(SDL_GPUDevice* device);

    /**
     * @brief   転送バッファからGPUバッファへコピーする
     *
     * @param[in]  copyPass        -> コピーパス
     * @param[in]  transferBuffer  -> 転送バッファ
     * @param[in]  size            -> コピーするサイズ
     */
    void UploadToBuffer(SDL_GPUCopyPass* copyPass, CubismBufferSDL3* transferBuffer, csmUint32 size);

    /**
     * @brief   リソースを破棄する
     *
     * @param[in]  device  -> GPUデバイス
     */
    void Destroy(SDL_GPUDevice* device);

    /**
     * @brief   GPUバッファを取得する
     *
     * @return GPUバッファ
     */
    SDL_GPUBuffer* GetBuffer() const { return _buffer; }

    /**
     * @brief   転送バッファを取得する
     *
     * @return 転送バッファ
     */
    SDL_GPUTransferBuffer* GetTransferBuffer() const { return _transferBuffer; }

    /**
     * @brief   バッファサイズを取得する
     *
     * @return バッファサイズ
     */
    csmUint32 GetSize() const { return _size; }

private:
    SDL_GPUBuffer* _buffer; ///< GPUバッファ
    SDL_GPUTransferBuffer* _transferBuffer; ///< 転送バッファ
    csmUint32 _size; ///< バッファサイズ
};

/**
 * @brief   イメージ（テクスチャ）を扱うクラス
 */
class CubismImageSDL3
{
public:
    /**
     * @brief   コンストラクタ
     */
    CubismImageSDL3();

    /**
     * @brief   デストラクタ
     */
    ~CubismImageSDL3();

    /**
     * @brief   テクスチャを作成する
     *
     * @param[in]  device          -> GPUデバイス
     * @param[in]  width           -> 横幅
     * @param[in]  height          -> 高さ
     * @param[in]  format          -> フォーマット
     * @param[in]  usageFlags      -> テクスチャの使用目的を指定するビットマスク
     * @param[in]  numLevels       -> ミップレベル数（省略時は1）
     */
    void CreateTexture(SDL_GPUDevice* device, csmInt32 width, csmInt32 height,
                       SDL_GPUTextureFormat format, SDL_GPUTextureUsageFlags usageFlags,
                       csmUint32 numLevels = 1);

    /**
     * @brief   サンプラーを作成する
     *
     * @param[in]  device          -> GPUデバイス
     * @param[in]  maxAnisotropy   -> 異方性の値の最大値
     */
    void CreateSampler(SDL_GPUDevice* device, csmFloat32 maxAnisotropy);

    /**
     * @brief   ミップマップを生成する
     *
     * テクスチャが複数のミップレベルを持っている場合、レベル0から残りのレベルを自動生成する。
     * この関数はパスの外で呼び出す必要がある。
     *
     * @param[in]  device  -> GPUデバイス
     */
    void GenerateMipmaps(SDL_GPUDevice* device);

    /**
     * @brief   ミップレベル数を取得する
     *
     * @return ミップレベル数
     */
    csmUint32 GetNumLevels() const { return _numLevels; }

    /**
     * @brief   テクスチャにデータをアップロードする
     *
     * @param[in]  device          -> GPUデバイス
     * @param[in]  data            -> アップロードするデータ
     * @param[in]  dataSize        -> データサイズ
     * @param[in]  width           -> 横幅
     * @param[in]  height          -> 高さ
     */
    void UploadTextureData(SDL_GPUDevice* device, const void* data, csmUint32 dataSize, csmInt32 width, csmInt32 height);

    /**
     * @brief   リソースを破棄する
     *
     * @param[in]  device  -> GPUデバイス
     */
    void Destroy(SDL_GPUDevice* device);

    /**
     * @brief   テクスチャを取得する
     *
     * @return テクスチャ
     */
    SDL_GPUTexture* GetTexture() const { return _texture; }

    /**
     * @brief   サンプラーを取得する
     *
     * @return サンプラー
     */
    SDL_GPUSampler* GetSampler() const { return _sampler; }

    /**
     * @brief   テクスチャの幅を取得する
     *
     * @return 幅
     */
    csmInt32 GetWidth() const { return _width; }

    /**
     * @brief   テクスチャの高さを取得する
     *
     * @return 高さ
     */
    csmInt32 GetHeight() const { return _height; }

    /**
     * @brief   テクスチャが有効かどうかを返す
     *
     * @return 有効ならtrue
     */
    csmBool IsValid() const { return _texture != nullptr; }

    /**
     * @brief   テクスチャフォーマットを取得する
     *
     * @return フォーマット
     */
    SDL_GPUTextureFormat GetFormat() const { return _format; }

private:
    SDL_GPUTexture* _texture; ///< テクスチャ
    SDL_GPUSampler* _sampler; ///< サンプラー
    csmInt32 _width; ///< 横幅
    csmInt32 _height; ///< 高さ
    csmUint32 _numLevels; ///< ミップレベル数
    SDL_GPUTextureFormat _format; ///< フォーマット
};

}}}

//------------ LIVE2D NAMESPACE ------------
