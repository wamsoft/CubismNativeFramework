/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once
#include "../CubismRenderer.hpp"
#include "../CubismClippingManager.hpp"
#include <SDL3/SDL_gpu.h>
#include "CubismFramework.hpp"
#include "CubismRenderTarget_SDL3.hpp"
#include "CubismOffscreenRenderTarget_SDL3.hpp"
#include "CubismClass_SDL3.hpp"
#include "Type/csmVector.hpp"
#include "Type/csmMap.hpp"
#include "Math/CubismVector2.hpp"

//------------ LIVE2D NAMESPACE ------------
namespace Live2D { namespace Cubism { namespace Framework { namespace Rendering {

/**
 * @brief   viewportのサイズを設定する
 *
 * @param[in]  width    -> 横幅
 * @param[in]  height   -> 縦幅
 * @param[in]  minDepth -> 最小深度
 * @param[in]  maxDepth -> 最大深度
 */
SDL_GPUViewport GetViewport(csmFloat32 width, csmFloat32 height, csmFloat32 minDepth, csmFloat32 maxDepth);

/**
 * @brief   scissorのサイズを設定する
 *
 * @param[in]  offsetX  -> x方向のオフセット
 * @param[in]  offsetY  -> y方向のオフセット
 * @param[in]  width    -> 横幅
 * @param[in]  height   -> 縦幅
 */
SDL_Rect GetScissor(csmFloat32 offsetX, csmFloat32 offsetY, csmFloat32 width, csmFloat32 height);

/*********************************************************************************************************************
*                                      CubismRenderer_SDL3
********************************************************************************************************************/
//  前方宣言
class CubismRenderer_SDL3;
class CubismClippingContext_SDL3;

/**
 * @brief  クリッピングマスクの処理を実行するクラス
 */
class CubismClippingManager_SDL3 : public CubismClippingManager<
            CubismClippingContext_SDL3, CubismRenderTarget_SDL3>
{
public:
    /**
     * @brief   クリッピングコンテキストを作成する。モデル描画時に実行する。
     *
     * @param[in]   model          ->  モデルのインスタンス
     * @param[in]   commandBuffer  ->  コマンドバッファ
     * @param[in]   renderer       ->  レンダラのインスタンス
     * @param[in]   commandBufferCurrent       -> スワップチェインに使用中のバッファインデックス
     * @param[in]   drawableObjectType  ->  描画オブジェクトのタイプ
     */
    void SetupClippingContext(CubismModel& model, SDL_GPUCommandBuffer* commandBuffer,
                              CubismRenderer_SDL3* renderer, csmInt32 commandBufferCurrent,
                              CubismRenderer::DrawableObjectType drawableObjectType);
};

/**
 * @brief   クリッピングマスクのコンテキスト
 */
class CubismClippingContext_SDL3 : public CubismClippingContext
{
    friend class CubismClippingManager_SDL3;
    friend class CubismRenderer_SDL3;

public:
    /**
     * @brief   引数付きコンストラクタ
     *
     */
    CubismClippingContext_SDL3(
        CubismClippingManager<CubismClippingContext_SDL3, CubismRenderTarget_SDL3>* manager, CubismModel& model,
        const csmInt32* clippingDrawableIndices, csmInt32 clipCount);

    /**
     * @brief   デストラクタ
     */
    virtual ~CubismClippingContext_SDL3();

    /**
     * @brief   このマスクを管理するマネージャのインスタンスを取得する。
     *
     * @return  クリッピングマネージャのインスタンス
     */
    CubismClippingManager<CubismClippingContext_SDL3, CubismRenderTarget_SDL3>* GetClippingManager();

    CubismClippingManager<CubismClippingContext_SDL3, CubismRenderTarget_SDL3>* _owner;
    ///< このマスクを管理しているマネージャのインスタンス
};

enum ShaderNames
{
#define CSM_CREATE_BLEND_OVERLAP_SHADER_NAMES(NAME) \
    CSM_CREATE_SHADER_NAMES(NAME ## Over), \
    CSM_CREATE_SHADER_NAMES(NAME ## Atop), \
    CSM_CREATE_SHADER_NAMES(NAME ## Out), \
    CSM_CREATE_SHADER_NAMES(NAME ## ConjointOver), \
    CSM_CREATE_SHADER_NAMES(NAME ## DisjointOver)

#define CSM_CREATE_SHADER_NAMES(NAME) \
    ShaderNames_ ## NAME, \
    ShaderNames_ ## NAME ## Masked, \
    ShaderNames_ ## NAME ## MaskedInverted, \
    ShaderNames_ ## NAME ## PremultipliedAlpha, \
    ShaderNames_ ## NAME ## MaskedPremultipliedAlpha, \
    ShaderNames_ ## NAME ## MaskedInvertedPremultipliedAlpha

    // Copy
    ShaderNames_Copy,

    // SetupMask
    ShaderNames_SetupMask,

    // Normal
    CSM_CREATE_SHADER_NAMES(Normal),
    // Add
    CSM_CREATE_SHADER_NAMES(Add),
    // Mult
    CSM_CREATE_SHADER_NAMES(Mult),

    //Normal
    CSM_CREATE_SHADER_NAMES(NormalAtop),
    CSM_CREATE_SHADER_NAMES(NormalOut),
    CSM_CREATE_SHADER_NAMES(NormalConjointOver),
    CSM_CREATE_SHADER_NAMES(NormalDisjointOver),
    // 加算
    CSM_CREATE_BLEND_OVERLAP_SHADER_NAMES(Add),
    // 加算(発光)
    CSM_CREATE_BLEND_OVERLAP_SHADER_NAMES(AddGlow),
    // 比較(暗)
    CSM_CREATE_BLEND_OVERLAP_SHADER_NAMES(Darken),
    // 乗算
    CSM_CREATE_BLEND_OVERLAP_SHADER_NAMES(Multiply),
    // 焼き込みカラー
    CSM_CREATE_BLEND_OVERLAP_SHADER_NAMES(ColorBurn),
    // 焼き込み(リニア)
    CSM_CREATE_BLEND_OVERLAP_SHADER_NAMES(LinearBurn),
    // 比較(明)
    CSM_CREATE_BLEND_OVERLAP_SHADER_NAMES(Lighten),
    // スクリーン
    CSM_CREATE_BLEND_OVERLAP_SHADER_NAMES(Screen),
    // 覆い焼きカラー
    CSM_CREATE_BLEND_OVERLAP_SHADER_NAMES(ColorDodge),
    // オーバーレイ
    CSM_CREATE_BLEND_OVERLAP_SHADER_NAMES(Overlay),
    // ソフトライト
    CSM_CREATE_BLEND_OVERLAP_SHADER_NAMES(SoftLight),
    // ハードライト
    CSM_CREATE_BLEND_OVERLAP_SHADER_NAMES(HardLight),
    // リニアライト
    CSM_CREATE_BLEND_OVERLAP_SHADER_NAMES(LinearLight),
    // 色相
    CSM_CREATE_BLEND_OVERLAP_SHADER_NAMES(Hue),
    // カラー
    CSM_CREATE_BLEND_OVERLAP_SHADER_NAMES(Color),

    // ブレンドモードの組み合わせ数 = 加算(5.2以前) + 乗算(5.2以前) + (通常 + 加算 + 加算(発光) + 比較(暗) + 乗算 + 焼き込みカラー + 焼き込み(リニア) + 比較(明) + スクリーン + 覆い焼きカラー + オーバーレイ + ソフトライト + ハードライト + リニアライト + 色相 + カラー) * (over + atop + out + conjoint over + disjoint over)
    // シェーダの数 = コピー用 + マスク生成用 + (通常 + 加算 + 乗算 + ブレンドモードの組み合わせ数) * (マスク無 + マスク有 + マスク有反転 + マスク無の乗算済アルファ対応版 + マスク有の乗算済アルファ対応版 + マスク有反転の乗算済アルファ対応版)
    ShaderNames_ShaderCount,

#undef CSM_CREATE_BLEND_OVERLAP_SHADER_NAMES
#undef CSM_CREATE_SHADER_NAMES
};

enum CompatibleBlend
{
    Blend_Normal,
    Blend_Add,
    Blend_Mult,
    Blend_Mask
};

enum ColorBlendMode
{
    ColorBlendMode_None = -1,
    ColorBlendMode_Normal,
    ColorBlendMode_Add,
    ColorBlendMode_AddGlow,
    ColorBlendMode_Darken,
    ColorBlendMode_Multiply,
    ColorBlendMode_ColorBurn,
    ColorBlendMode_LinearBurn,
    ColorBlendMode_Lighten,
    ColorBlendMode_Screen,
    ColorBlendMode_ColorDodge,
    ColorBlendMode_Overlay,
    ColorBlendMode_SoftLight,
    ColorBlendMode_HardLight,
    ColorBlendMode_LinearLight,
    ColorBlendMode_Hue,
    ColorBlendMode_Color,
    ColorBlendMode_Count,
};

enum AlphaBlendMode
{
    AlphaBlendMode_None = -1,
    AlphaBlendMode_Over,
    AlphaBlendMode_Atop,
    AlphaBlendMode_Out,
    AlphaBlendMode_ConjointOver,
    AlphaBlendMode_DisjointOver,
    AlphaBlendMode_Count,
};

/**
 * @brief   どのシェーダーを利用するかを取得する
 */
csmInt32 GetShaderNamesBegin(const csmBlendMode blendMode);

/**
 * @brief   頂点情報を保持する構造体
 *
 */
struct ModelVertex
{
    CubismVector2 pos; // Position
    CubismVector2 texCoord; // UVs

    /**
     * @brief   頂点バッファの属性記述を取得する
     *
     * @param[out]  attributeDescriptions -> 属性記述の配列
     */
    static void GetVertexAttributes(SDL_GPUVertexAttribute attributeDescriptions[2])
    {
        // Position attribute
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].buffer_slot = 0;
        attributeDescriptions[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        attributeDescriptions[0].offset = offsetof(ModelVertex, pos);

        // TexCoord attribute
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].buffer_slot = 0;
        attributeDescriptions[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        attributeDescriptions[1].offset = offsetof(ModelVertex, texCoord);
    }

    /**
     * @brief   頂点バッファの記述を取得する
     *
     * @param[out]  bufferDescription -> バッファ記述
     */
    static void GetVertexBufferDescription(SDL_GPUVertexBufferDescription* bufferDescription)
    {
        bufferDescription->slot = 0;
        bufferDescription->pitch = sizeof(ModelVertex);
        bufferDescription->input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
        bufferDescription->instance_step_rate = 0;
    }
};

/**
 * @brief  シェーダー関連のプログラムを生成・破棄するクラス<br>
 *         シングルトンなクラスであり、CubismPipeline_SDL3::GetInstance()からアクセスする。
 */
class CubismPipeline_SDL3
{
public:
    /**
     * @brief   コンストラクタ
     */
    CubismPipeline_SDL3();

    /**
     * @brief   デストラクタ
     */
    ~CubismPipeline_SDL3();

    struct PipelineResource
    {
        /**
         * @brief   シェーダーを読み込む
         *
         * @param[in]   device        ->  GPUデバイス
         * @param[in]   filename      ->  ファイル名
         * @param[in]   stage         ->  シェーダーステージ
         * @param[in]   numSamplers   ->  サンプラー数
         * @param[in]   numUniformBuffers -> ユニフォームバッファ数
         *
         * @return  シェーダーオブジェクト
         */
        SDL_GPUShader* LoadShader(SDL_GPUDevice* device, csmString filename, SDL_GPUShaderStage stage,
                                  csmUint32 numSamplers, csmUint32 numUniformBuffers);

        /**
         * @brief   パイプラインを作成する
         *
         * @param[in]   device              ->  GPUデバイス
         * @param[in]   vertFileName        ->  Vertexシェーダーのファイル
         * @param[in]   fragFileName        ->  Fragmentシェーダーのファイル
         * @param[in]   colorTargetFormat   ->  カラーターゲットフォーマット
         * @param[in]   shaderName          ->  シェーダー名
         * @param[in]   colorBlendMode      ->  ブレンドカラーモード
         * @param[in]   alphaBlendMode      ->  オーバーラップカラーモード
         *
         * @return     シェーダーの作成に成功: true 失敗: false
         */
        csmBool CreateGraphicsPipeline(SDL_GPUDevice* device, csmString vertFileName, csmString fragFileName,
                                       SDL_GPUTextureFormat colorTargetFormat,
                                       csmUint32 shaderName,
                                       csmInt32 colorBlendMode = ColorBlendMode_None,
                                       csmInt32 alphaBlendMode = AlphaBlendMode_None);

        /**
         * @brief   リソースを解放する
         *
         * @param[in]   device -> GPUデバイス
         */
        void Release(SDL_GPUDevice* device);

        /**
         * @brief   パイプラインを取得する
         *
         * @param[in]   index -> インデックス
         *
         * @return  パイプライン
         */
        SDL_GPUGraphicsPipeline* GetPipeline(csmInt32 index)
        {
            return _pipeline[index];
        }

    private:
        csmVector<SDL_GPUGraphicsPipeline*> _pipeline; ///< normal, add, multi, maskそれぞれのパイプライン
    };

    /**
     * @brief   シェーダーごとにPipelineResourceのインスタンスを作成する
     *
     * @param[in]   device              ->  GPUデバイス
     * @param[in]   colorTargetFormat   ->  カラーターゲットフォーマット
     */
    void CreatePipelines(SDL_GPUDevice* device, SDL_GPUTextureFormat colorTargetFormat);

    /**
     * @brief   PipelineResourceのインスタンスを作成する
     *
     * @param[in]   device              ->  GPUデバイス
     * @param[in]   shaderName          ->  シェーダー名
     * @param[in]   vertShaderFileName  ->  Vertexシェーダーファイル名
     * @param[in]   fragShaderFileName  ->  Fragmentシェーダーファイル名
     * @param[in]   colorTargetFormat   ->  カラーターゲットフォーマット
     * @param[in]   colorBlendMode      ->  ブレンドカラーモード
     * @param[in]   alphaBlendMode      ->  オーバーラップカラーモード
     */
    void CreatePipelineResource(SDL_GPUDevice* device,
                                csmUint32 const& shaderName,
                                csmString const& vertShaderFileName,
                                csmString const& fragShaderFileName,
                                SDL_GPUTextureFormat const& colorTargetFormat,
                                csmInt32 const& colorBlendMode = ColorBlendMode_None,
                                csmInt32 const& alphaBlendMode = AlphaBlendMode_None);

    /**
     * @brief   指定したシェーダーのグラフィックスパイプラインを取得する
     *
     * @param[in]   shaderIndex         ->  シェーダインデックス
     * @param[in]   blendIndex          ->  ブレンドモードのインデックス
     *
     * @return  指定したシェーダーのグラフィックスパイプライン
     */
    SDL_GPUGraphicsPipeline* GetPipeline(csmInt32 shaderIndex, csmInt32 blendIndex)
    {
        if (_pipelineResource[shaderIndex] == NULL)
        {
            return NULL;
        }
        return _pipelineResource[shaderIndex]->GetPipeline(blendIndex);
    }

    /**
     * @brief   インスタンスを取得する（シングルトン）
     *
     * @return  インスタンスのポインタ
     */
    static CubismPipeline_SDL3* GetInstance();

    /**
     * @brief   リソースを開放する
     *
     * @param[in]   device -> GPUデバイス
     */
    void ReleaseShaderProgram(SDL_GPUDevice* device);

private:
    csmVector<PipelineResource*> _pipelineResource;
};

/**
 * @brief   SDL3用の描画命令を実装したクラス
 *
 */
class CubismRenderer_SDL3 : public CubismRenderer
{
    friend class CubismClippingManager_SDL3;
    friend class CubismRenderer;

    /**
     * @brief   モデル用ユニフォームバッファオブジェクトの中身を保持する構造体
     */
    struct ModelUBO
    {
        csmFloat32 projectionMatrix[16]; ///< シェーダープログラムに渡すデータ(ProjectionMatrix)
        csmFloat32 clipMatrix[16]; ///< シェーダープログラムに渡すデータ(ClipMatrix)
        csmFloat32 baseColor[4]; ///< シェーダープログラムに渡すデータ(BaseColor)
        csmFloat32 multiplyColor[4]; ///< シェーダープログラムに渡すデータ(MultiplyColor)
        csmFloat32 screenColor[4]; ///< シェーダープログラムに渡すデータ(ScreenColor)
        csmFloat32 channelFlag[4]; ///< シェーダープログラムに渡すデータ(ChannelFlag)
    };

protected:
    /**
     * @brief   コンストラクタ
     */
    CubismRenderer_SDL3(csmUint32 width, csmUint32 height);

    /**
     * @brief   デストラクタ
     */
    ~CubismRenderer_SDL3() override;

    /**
     * @brief   レンダラが保持する静的なリソースを解放する。
     */
    static void DoStaticRelease();

public:

    /**
     * @brief    レンダラを作成するための各種設定
     *
     *           モデルを読み込む前に一度だけ呼び出す
     *
     * @param[in]   device              -> GPUデバイス
     * @param[in]   swapchainImageCount -> スワップチェーンのイメージ数
     * @param[in]   width               -> 描画解像度の幅
     * @param[in]   height              -> 描画解像度の高さ
     * @param[in]   colorTargetFormat   -> 描画対象のフォーマット
    * @param[in]   depthTargetFormat   -> 深度対象のフォーマット
     */
    static void InitializeConstantSettings(SDL_GPUDevice* device,
                                           csmUint32 swapchainImageCount,
                                           csmUint32 width, csmUint32 height,
                                   SDL_GPUTextureFormat colorTargetFormat,
                                   SDL_GPUTextureFormat depthTargetFormat);


    /**
     * @brief    レンダリング対象の指定
     *
     * @param[in]   texture        -> テクスチャ
     * @param[in]   format         -> フォーマット
     * @param[in]   width          -> 幅
     * @param[in]   height         -> 高さ
     */
    static void SetRenderTarget(SDL_GPUTexture* texture, SDL_GPUTextureFormat format,
                                csmUint32 width, csmUint32 height);

    /**
     * @brief   描画対象の解像度を設定する
     *
     * @param[in]  width   -> 幅
     * @param[in]  height  -> 高さ
     */
    static void SetImageExtent(csmUint32 width, csmUint32 height);

    /**
     * @brief   外部コマンドバッファを設定する
     *          メインレンダーループのコマンドバッファを共有し、スワップチェインテクスチャの
     *          レイアウト追跡を正しく行うために使用する。
     *          nullptrを設定すると内部でコマンドバッファを取得するモードに戻る。
     *
     * @param[in]  commandBuffer   -> 外部コマンドバッファ（またはnullptr）
     */
    static void SetExternalCommandBuffer(SDL_GPUCommandBuffer* commandBuffer);

    /**
     * @brief   空の頂点バッファを作成する。
     */
    void CreateVertexBuffer();

    /**
     * @brief   インデックスバッファを作成する。
     */
    void CreateIndexBuffer();

    /**
     * @brief  深度バッファを作成する。
     */
    void CreateDepthBuffer();

    /**
     * @brief   レンダラーを初期化する。
     */
    void InitializeRenderer();

    /**
     * @brief    レンダラの初期化処理を実行する<br>
     *           引数に渡したモデルからレンダラの初期化処理に必要な情報を取り出すことができる
     *
     * @param[in]  model -> モデルのインスタンス
     */
    void Initialize(Framework::CubismModel* model) override;

    /**
     * @brief    レンダラの初期化処理を実行する<br>
     *           引数に渡したモデルからレンダラの初期化処理に必要な情報を取り出すことができる
     *
     * @param[in]  model -> モデルのインスタンス
     * @param[in]  maskBufferCount -> マスクバッファの数
     */
    void Initialize(Framework::CubismModel* model, csmInt32 maskBufferCount) override;

    /**
     * @bref オフスクリーンの親を探して設定する
     *
     * @param model -> モデルのインスタンス
     * @param offscreenCount -> オフスクリーンの数
     */
    void SetupParentOffscreens(const CubismModel* model, csmInt32 offscreenCount);

    /**
     * @brief   頂点バッファを更新する。
     * @param[in]   drawAssign    -> 描画インデックス
     * @param[in]   vcount        -> 頂点数
     * @param[in]   varray        -> 頂点配列
     * @param[in]   uvarray       -> uv配列
     * @param[in]   copyPass      -> コピーパス
     */
    void CopyToBuffer(csmInt32 drawAssign, const csmInt32 vcount, const csmFloat32* varray, const csmFloat32* uvarray,
                      SDL_GPUCopyPass* copyPass);

    /**
     * @brief   行列を更新する
     *
     * @param[in]   mat4       -> 更新する行列
     * @param[in]   cubismMat  -> 新しい値
     */
    static void UpdateMatrix(csmFloat32 mat4[16], CubismMatrix44 cubismMat);

    /**
     * @brief   カラーベクトルを更新する
     *
     * @param[in]   vec4       -> 更新するカラーベクトル
     * @param[in]   r, g, b, a -> 新しい値
     */
    void UpdateColor(csmFloat32 vec4[4], csmFloat32 r, csmFloat32 g, csmFloat32 b, csmFloat32 a);

    /**
     * @brief   メッシュ描画を実行する
     *
     * @param[in]   model                 ->  描画対象のモデル
     * @param[in]   index                 ->  描画オブジェクトのインデックス
     * @param[in]   renderPass            ->  レンダーパス
     */
    void ExecuteDrawForDrawable(const CubismModel& model, const csmInt32 index, SDL_GPURenderPass* renderPass);

    /**
     * @brief   マスク描画を実行する
     *
     * @param[in]   model                 ->  描画対象のモデル
     * @param[in]   index                 ->  描画オブジェクトのインデックス
     * @param[in]   renderPass            ->  レンダーパス
     */
    void ExecuteDrawForMask(const CubismModel& model, const csmInt32 index, SDL_GPURenderPass* renderPass);

    /**
     * @brief   オフスクリーン描画を実行する
     *
     * @param[in]   model                 ->  描画対象のモデル
     * @param[in]   offscreen             ->  描画対象
     * @param[in]   renderPass            ->  レンダーパス
     */
    void ExecuteDrawForOffscreen(const CubismModel& model, CubismOffscreenRenderTarget_SDL3* offscreen, SDL_GPURenderPass* renderPass);

    /**
     * @brief   オフスクリーンからのコピーを実行する
     *
     * @param[in]   srcBuffer             ->  コピー元
     * @param[in]   renderPass            ->  レンダーパス
     */
    void ExecuteDrawForRenderTarget(const CubismRenderTarget_SDL3* srcBuffer, SDL_GPURenderPass* renderPass);

    /**
     * @brief   [オーバーライド]<br>
     *           描画オブジェクト（アートメッシュ）を描画する。<br>
     * @param[in]   model           ->  描画対象のモデル
     * @param[in]   index           ->  描画メッシュのインデックス
     * @param[in]   commandBuffer   ->  コマンドバッファ
     * @param[in]   renderPass      ->  レンダーパス
     */
    void DrawMeshSDL3(const CubismModel& model, const csmInt32 index,
                      SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass);

    /**
     * @brief   レンダリング開始
     *
     * @param[in]   commandBuffer       ->  コマンドバッファ
     * @param[in]   isResume            ->  レンダリング再開かのフラグ
     *
     * @return  レンダーパス
     */
    SDL_GPURenderPass* BeginRendering(SDL_GPUCommandBuffer* commandBuffer, csmBool isResume);

    /**
     * @brief   レンダリング終了
     *
     * @param[in]   renderPass       ->  レンダーパス
     */
    void EndRendering(SDL_GPURenderPass* renderPass);

    /**
     * @brief   MOCバージョンによって異なる対象へのレンダリング開始
     *          ブレンドモードが有効な場合はモデルレンダーターゲットに描画する。
     *
     * @param[in]   commandBuffer       ->  コマンドバッファ
     * @param[in]   isResume            ->  レンダリング再開かのフラグ
     *
     * @return  レンダーパス
     */
    SDL_GPURenderPass* BeginRenderTarget(SDL_GPUCommandBuffer* commandBuffer,
                                         csmBool isResume);

    /**
     * @brief   MOCバージョンによって異なる対象へのレンダリング終了
     *          ブレンドモードが有効な場合はコピーバックを行う。
     *
     * @param[in]   commandBuffer   ->  コマンドバッファ
     * @param[in]   renderPass      ->  レンダーパス
     */
    void EndRenderTarget(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass);

    /**
     * @brief   モデルを描画する実際の処理
     *
     */
    void DoDrawModel() override;

    /**
     * @brief   描画完了後の追加処理。<br>
     *          マルチバッファリングに必要な処理を実装している。
     */
    void PostDraw();

    /**
     * @brief   モデル描画直前のステートを保持する
     */
    void SaveProfile() override {}

    /**
     * @brief   モデル描画直前のステートを復帰する
     */
    void RestoreProfile() override {}

    /**
     * @brief   モデル描画直前のオフスクリーン設定
     */
    void BeforeDrawModelRenderTarget() override;

    /**
     * @brief   モデル描画後のオフスクリーン設定
     */
    void AfterDrawModelRenderTarget() override;

    /**
     * @brief   マスクテクスチャに描画するクリッピングコンテキストをセットする。
     *
     * @param[in]  clip ->  クリッピングコンテキスト
     */
    void SetClippingContextBufferForMask(CubismClippingContext_SDL3* clip);

    /**
     * @brief   マスクテクスチャに描画するクリッピングコンテキストを取得する。
     *
     * @return  マスクテクスチャに描画するクリッピングコンテキスト
     */
    CubismClippingContext_SDL3* GetClippingContextBufferForMask() const;

    /**
     * @brief   画面上に描画するクリッピングコンテキストをセットする。
     *
     * @param[in]  clip ->  クリッピングコンテキスト
     */
    void SetClippingContextBufferForDraw(CubismClippingContext_SDL3* clip);

    /**
     * @brief   画面上に描画するクリッピングコンテキストを取得する。
     *
     * @return  画面上に描画するクリッピングコンテキスト
     */
    CubismClippingContext_SDL3* GetClippingContextBufferForDrawable() const;

    /**
     * @brief   テクスチャをバインドする
     *
     * @param[in]   image         ->  テクスチャとサンプラーを保持しているインスタンス
     */
    void BindTexture(CubismImageSDL3& image);

    /**
     * @brief  クリッピングマスクバッファのサイズを設定する
     *
     * @param[in]  width  -> クリッピングマスクバッファの横幅
     * @param[in]  height -> クリッピングマスクバッファの立幅
     *
     */
    void SetClippingMaskBufferSize(csmFloat32 width, csmFloat32 height);

    /**
     * @brief  クリッピングマスクバッファのサイズを取得する
    *
     * @return クリッピングマスクバッファのサイズ
     *
     */
    CubismVector2 GetClippingMaskBufferSize() const;

    /**
     * @brief  モデル全体を描画する先のフレームバッファを取得する
     *
     * @return モデル全体を描画する先のフレームバッファへの参照
     *
     */
    CubismRenderTarget_SDL3* GetModelRenderTarget();

    /**
     * @brief  クリッピングマスクのバッファを取得する
     *
     * @param[in] backbufferNum  -> バックバッファの番号
     * @param[in] offscreenIndex -> オフスクリーンのインデックス
     *
     * @return クリッピングマスクのバッファへのポインタ
     *
     */
    CubismRenderTarget_SDL3* GetDrawableMaskBuffer(csmUint32 backbufferNum, csmInt32 offscreenIndex);

private:

    /**
     * @brief  色定数バッファを設定する
     *
     * @param[in]   ubo                   ->  ユニフォームバッファ
     * @param[in]   baseColor             ->  ベースカラー
     * @param[in]   multiplyColor         ->  乗算カラー
     * @param[in]   screenColor           ->  スクリーンカラー
     */
    void SetColorUniformBuffer(ModelUBO& ubo, const CubismTextureColor& baseColor,
                               const CubismTextureColor& multiplyColor, const CubismTextureColor& screenColor);

    /**
     * @brief  頂点バッファとインデックスバッファをバインドする
     *
     * @param[in]   index                   ->  描画メッシュのインデックス
     * @param[in]   renderPass              ->  レンダーパス
     * @param[in]   drawableObjectType      ->  描画オブジェクトの種類
     */
    void BindVertexAndIndexBuffers(const csmInt32 index, SDL_GPURenderPass* renderPass, DrawableObjectType drawableObjectType);

    /**
     * @brief  描画に使用するカラーチャンネルを設定
     *
     * @param[in]   ubo              ->  ユニフォームバッファ
     * @param[in]   contextBuffer    ->  描画コンテキスト
     */
    void SetColorChannel(ModelUBO& ubo, CubismClippingContext_SDL3* contextBuffer);

    /**
     * @brief  コピー用のシェーダーでオフスクリーンのテクスチャをメインターゲットにコピーする
     *         内部でレンダーパスを開始・終了する。
     *
     * @param[in]   src                -> コピー元
     * @param[in]   commandBuffer      -> コマンドバッファ
     */
    void CopyRenderTarget(const CubismRenderTarget_SDL3* src, SDL_GPUCommandBuffer* commandBuffer);

    /**
     * @brief  描画オブジェクトのループ処理
     *
     * @param[in]   commandBuffer         -> コマンドバッファ
     */
    void DrawObjectLoop(SDL_GPUCommandBuffer* commandBuffer);

    /**
     * @brief  描画オブジェクトの描画
     *
     * @param[in]   objectIndex           -> オブジェクトのインデックス
     * @param[in]   objectType            -> オブジェクトの種類
     * @param[in]   commandBuffer         -> コマンドバッファ
     * @param[in]   renderPass            -> レンダーパス
     */
    void RenderObject(csmInt32 objectIndex, csmInt32 objectType,
                      SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass*& renderPass);

    /**
     * @brief  Drawableの描画
     *
     * @param[in]   drawableIndex         -> Drawableのインデックス
     * @param[in]   commandBuffer         -> コマンドバッファ
     * @param[in]   renderPass            -> レンダーパス
     */
    void DrawDrawable(csmInt32 drawableIndex, SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass*& renderPass);

    /**
     * @brief  親オフスクリーンへの描画伝搬
     *
     * @param[in]   objectIndex           -> オブジェクトのインデックス
     * @param[in]   objectType            -> オブジェクトの種類
     * @param[in]   commandBuffer         -> コマンドバッファ
     * @param[in]   renderPass            -> レンダーパス
     */
    void SubmitDrawToParentOffscreen(csmInt32 objectIndex, DrawableObjectType objectType,
                                     SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass*& renderPass);

    /**
     * @brief  オフスクリーンの追加
     *
     * @param[in]   offscreenIndex        -> オフスクリーンのインデックス
     * @param[in]   commandBuffer         -> コマンドバッファ
     * @param[in]   renderPass            -> レンダーパス
     */
    void AddOffscreen(csmInt32 offscreenIndex, SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass*& renderPass);

    /**
     * @brief  オフスクリーンの描画
     *
     * @param[in]   currentOffscreen      -> 現在のオフスクリーン
     * @param[in]   commandBuffer         -> コマンドバッファ
     * @param[in]   renderPass            -> レンダーパス
     */
    void DrawOffscreen(CubismOffscreenRenderTarget_SDL3* currentOffscreen,
                       SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass*& renderPass);

    /**
     * @brief  オフスクリーンの描画用クリッピングコンテキストをセットする
     *
     * @param[in]   clip                  -> クリッピングコンテキスト
     */
    void SetClippingContextBufferForOffscreen(CubismClippingContext_SDL3* clip);

    /**
     * @brief   オフスクリーン描画用クリッピングコンテキストを取得する。
     *
     * @return  オフスクリーン描画用クリッピングコンテキスト
     */
    CubismClippingContext_SDL3* GetClippingContextBufferForOffscreen() const;

    /**
     * @brief  オフスクリーン用マスクバッファを取得する
     *
     * @param[in]   backBufferNum         -> バックバッファの番号
     * @param[in]   offscreenIndex        -> オフスクリーンのインデックス
     *
     * @return      オフスクリーン用マスクバッファ
     */
    CubismRenderTarget_SDL3* GetOffscreenMaskBuffer(csmUint32 backBufferNum, csmInt32 offscreenIndex);

    /**
     * @brief   ブレンドテクスチャのスナップショットをブリット（レンダーパス非アクティブ時に呼ぶ）
     *
     * @param[in]   commandBuffer  -> コマンドバッファ
     * @param[in]   srcTarget      -> ブリット元のレンダーターゲット
     */
    void BlitBlendSnapshot(SDL_GPUCommandBuffer* commandBuffer, CubismRenderTarget_SDL3* srcTarget);

    /**
     * @brief   ブレンド読み取り用イメージの作成・サイズ更新
     */
    void EnsureBlendReadImage();

    CubismClippingContext_SDL3* _clippingContextBufferForOffscreen; ///< オフスクリーン用クリッピングコンテキスト
    CubismOffscreenRenderTarget_SDL3* _currentOffscreen; ///< 現在のオフスクリーン
    CubismRenderTarget_SDL3* _currentRenderTarget; ///< 現在のレンダーターゲット

    CubismRenderer_SDL3(const CubismRenderer_SDL3&);
    CubismRenderer_SDL3& operator=(const CubismRenderer_SDL3&);

    CubismClippingManager_SDL3* _drawableClippingManager; ///< クリッピングマスク管理オブジェクト
    CubismClippingContext_SDL3* _clippingContextBufferForMask; ///< マスクテクスチャに描画するためのクリッピングコンテキスト
    CubismClippingContext_SDL3* _clippingContextBufferForDraw; ///< 画面上描画するためのクリッピングコンテキスト

    csmVector<csmInt32> _sortedObjectsIndexList;                ///< 描画オブジェクトのインデックスを描画順に並べたリスト
    csmVector<DrawableObjectType> _sortedObjectsTypeList;       ///< 描画オブジェクトの種別を描画順に並べたリスト

    CubismClippingManager_SDL3* _offscreenClippingManager; ///< オフスクリーン用クリッピングマスク管理オブジェクト
    csmVector<CubismOffscreenRenderTarget_SDL3> _offscreenList;     ///< モデルのオフスクリーン

    csmUint32 _commandBufferCurrent; ///< スワップチェーン用に使用中のバッファインデックス

    csmVector<csmVector<CubismRenderTarget_SDL3>> _drawableMaskBuffers; ///< Drawableのマスク描画用のフレームバッファ
    csmVector<csmVector<CubismRenderTarget_SDL3>> _offscreenMaskBuffers; ///< オフスクリーン機能マスク描画用のフレームバッファ
    csmVector<CubismRenderTarget_SDL3> _modelRenderTargets; ///< モデル全体を描画する先のフレームバッファ
    csmVector<csmVector<CubismBufferSDL3>> _vertexBuffers; ///< 頂点バッファ
    csmVector<csmVector<CubismBufferSDL3>> _stagingBuffers; ///< 頂点バッファを更新する際に使うステージングバッファ
    csmVector<csmVector<CubismBufferSDL3>> _indexBuffers; ///< インデックスバッファ

    csmVector<csmVector<CubismBufferSDL3>> _offscreenVertexBuffers; ///< オフスクリーン用頂点バッファ
    csmVector<csmVector<CubismBufferSDL3>> _offscreenStagingBuffers; ///< オフスクリーン用頂点バッファを更新する際に使うステージングバッファ
    csmVector<csmVector<CubismBufferSDL3>> _offscreenIndexBuffers; ///< オフスクリーン用インデックスバッファ

    csmVector<CubismBufferSDL3> _copyVertexBuffer;
    csmVector<CubismBufferSDL3> _copyStagingBuffer;
    csmVector<CubismBufferSDL3> _copyIndexBuffer;

    csmVector<CubismImageSDL3> _textures; ///< モデルが使うテクスチャ
    CubismImageSDL3 _depthImage; ///< オフスクリーンの色情報を保持する深度画像
    CubismImageSDL3 _blendReadImage; ///< ブレンドテクスチャ読み取り用のスナップショットイメージ（フィードバックループ回避）
    SDL_FColor _clearColor; ///< クリアカラー
    SDL_GPUCommandBuffer* _activeCommandBuffer; ///< 現在アクティブなコマンドバッファ

    csmBool _isClearedModelRenderTarget; ///< レンダーターゲットをクリアしたか
};
}}}}

//------------ LIVE2D NAMESPACE ------------
