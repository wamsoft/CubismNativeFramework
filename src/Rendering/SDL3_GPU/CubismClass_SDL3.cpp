/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "CubismClass_SDL3.hpp"
#include "CubismFramework.hpp"
#include "Utils/CubismDebug.hpp"

//------------ LIVE2D NAMESPACE ------------
namespace Live2D { namespace Cubism { namespace Framework {

//==================== CubismBufferSDL3 ====================

CubismBufferSDL3::CubismBufferSDL3()
    : _buffer(nullptr)
    , _transferBuffer(nullptr)
    , _size(0)
{
}

CubismBufferSDL3::~CubismBufferSDL3()
{
    CSM_ASSERT(_buffer == nullptr);
    CSM_ASSERT(_transferBuffer == nullptr);
}

void CubismBufferSDL3::CreateBuffer(SDL_GPUDevice* device, csmUint32 size, SDL_GPUBufferUsageFlags usageFlags)
{
    if (_buffer != nullptr)
    {
        CubismLogWarning("Buffer already created. Destroy existing buffer first.");
        return;
    }

    _size = size;

    SDL_GPUBufferCreateInfo bufferInfo = {};
    bufferInfo.usage = usageFlags;
    bufferInfo.size = size;
    bufferInfo.props = 0;

    _buffer = SDL_CreateGPUBuffer(device, &bufferInfo);

    if (_buffer == nullptr)
    {
        CubismLogError("Failed to create GPU buffer: %s", SDL_GetError());
    }
}

void CubismBufferSDL3::CreateTransferBuffer(SDL_GPUDevice* device, csmUint32 size, SDL_GPUTransferBufferUsage usage)
{
    if (_transferBuffer != nullptr)
    {
        CubismLogWarning("Transfer buffer already created. Destroy existing buffer first.");
        return;
    }

    _size = size;

    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = usage;
    transferInfo.size = size;
    transferInfo.props = 0;

    _transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferInfo);

    if (_transferBuffer == nullptr)
    {
        CubismLogError("Failed to create transfer buffer: %s", SDL_GetError());
    }
}

void* CubismBufferSDL3::MapTransferBuffer(SDL_GPUDevice* device, csmBool cycle)
{
    if (_transferBuffer == nullptr)
    {
        CubismLogError("Transfer buffer not created.");
        return nullptr;
    }

    void* mappedMemory = SDL_MapGPUTransferBuffer(device, _transferBuffer, cycle);

    if (mappedMemory == nullptr)
    {
        CubismLogError("Failed to map transfer buffer: %s", SDL_GetError());
    }

    return mappedMemory;
}

void CubismBufferSDL3::UnmapTransferBuffer(SDL_GPUDevice* device)
{
    if (_transferBuffer == nullptr)
    {
        CubismLogError("Transfer buffer not created.");
        return;
    }

    SDL_UnmapGPUTransferBuffer(device, _transferBuffer);
}

void CubismBufferSDL3::UploadToBuffer(SDL_GPUCopyPass* copyPass, CubismBufferSDL3* transferBuffer, csmUint32 size)
{
    if (_buffer == nullptr)
    {
        CubismLogError("Destination GPU buffer not created.");
        return;
    }

    if (transferBuffer == nullptr || transferBuffer->GetTransferBuffer() == nullptr)
    {
        CubismLogError("Source transfer buffer not created or invalid.");
        return;
    }

    SDL_GPUTransferBufferLocation source = {};
    source.transfer_buffer = transferBuffer->GetTransferBuffer();
    source.offset = 0;

    SDL_GPUBufferRegion destination = {};
    destination.buffer = _buffer;
    destination.offset = 0;
    destination.size = size;

    SDL_UploadToGPUBuffer(copyPass, &source, &destination, false);
}

void CubismBufferSDL3::Destroy(SDL_GPUDevice* device)
{
    if (_buffer != nullptr)
    {
        SDL_ReleaseGPUBuffer(device, _buffer);
        _buffer = nullptr;
    }

    if (_transferBuffer != nullptr)
    {
        SDL_ReleaseGPUTransferBuffer(device, _transferBuffer);
        _transferBuffer = nullptr;
    }

    _size = 0;
}

//==================== CubismImageSDL3 ====================

CubismImageSDL3::CubismImageSDL3()
    : _texture(nullptr)
    , _sampler(nullptr)
    , _width(0)
    , _height(0)
    , _numLevels(1)
    , _format(SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM)
{
}

CubismImageSDL3::~CubismImageSDL3()
{
    CSM_ASSERT(_texture == nullptr);
    CSM_ASSERT(_sampler == nullptr);
}

void CubismImageSDL3::CreateTexture(SDL_GPUDevice* device, csmInt32 width, csmInt32 height,
                                    SDL_GPUTextureFormat format, SDL_GPUTextureUsageFlags usageFlags,
                                    csmUint32 numLevels)
{
    if (_texture != nullptr)
    {
        CubismLogWarning("Texture already created. Destroy existing texture first.");
        return;
    }

    _width = width;
    _height = height;
    _format = format;
    _numLevels = (numLevels > 0) ? numLevels : 1;

    SDL_GPUTextureCreateInfo textureInfo = {};
    textureInfo.type = SDL_GPU_TEXTURETYPE_2D;
    textureInfo.format = format;
    textureInfo.usage = usageFlags;
    textureInfo.width = static_cast<Uint32>(width);
    textureInfo.height = static_cast<Uint32>(height);
    textureInfo.layer_count_or_depth = 1;
    textureInfo.num_levels = _numLevels;
    textureInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
    textureInfo.props = 0;

    _texture = SDL_CreateGPUTexture(device, &textureInfo);

    if (_texture == nullptr)
    {
        CubismLogError("Failed to create GPU texture: %s", SDL_GetError());
    }
}

void CubismImageSDL3::CreateSampler(SDL_GPUDevice* device, csmFloat32 maxAnisotropy)
{
    if (_sampler != nullptr)
    {
        CubismLogWarning("Sampler already created. Destroy existing sampler first.");
        return;
    }

    SDL_GPUSamplerCreateInfo samplerInfo = {};
    samplerInfo.min_filter = SDL_GPU_FILTER_LINEAR;
    samplerInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
    samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerInfo.mip_lod_bias = 0.0f;
    samplerInfo.max_anisotropy = maxAnisotropy;
    samplerInfo.compare_op = SDL_GPU_COMPAREOP_NEVER;
    samplerInfo.min_lod = 0.0f;
    samplerInfo.max_lod = static_cast<float>(_numLevels);
    samplerInfo.enable_anisotropy = maxAnisotropy > 1.0f ? true : false;
    samplerInfo.enable_compare = false;
    samplerInfo.props = 0;

    _sampler = SDL_CreateGPUSampler(device, &samplerInfo);

    if (_sampler == nullptr)
    {
        CubismLogError("Failed to create GPU sampler: %s", SDL_GetError());
    }
}

void CubismImageSDL3::UploadTextureData(SDL_GPUDevice* device, const void* data, csmUint32 dataSize, csmInt32 width, csmInt32 height)
{
    if (_texture == nullptr)
    {
        CubismLogError("Texture not created.");
        return;
    }

    if (data == nullptr || dataSize == 0)
    {
        CubismLogError("Invalid data for texture upload.");
        return;
    }

    // 転送バッファを作成
    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = dataSize;
    transferInfo.props = 0;

    SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
    if (transferBuffer == nullptr)
    {
        CubismLogError("Failed to create transfer buffer for texture upload: %s", SDL_GetError());
        return;
    }

    // 転送バッファにデータをコピー
    void* mappedData = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
    if (mappedData == nullptr)
    {
        CubismLogError("Failed to map transfer buffer for texture upload: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
        return;
    }

    memcpy(mappedData, data, dataSize);
    SDL_UnmapGPUTransferBuffer(device, transferBuffer);

    // コマンドバッファを取得してコピーパスでアップロード
    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
    if (commandBuffer == nullptr)
    {
        CubismLogError("Failed to acquire command buffer for texture upload: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
        return;
    }

    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
    if (copyPass == nullptr)
    {
        CubismLogError("Failed to begin copy pass for texture upload: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(commandBuffer);
        SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
        return;
    }

    SDL_GPUTextureTransferInfo source = {};
    source.transfer_buffer = transferBuffer;
    source.offset = 0;
    source.pixels_per_row = static_cast<Uint32>(width);
    source.rows_per_layer = static_cast<Uint32>(height);

    SDL_GPUTextureRegion destination = {};
    destination.texture = _texture;
    destination.mip_level = 0;
    destination.layer = 0;
    destination.x = 0;
    destination.y = 0;
    destination.z = 0;
    destination.w = static_cast<Uint32>(width);
    destination.h = static_cast<Uint32>(height);
    destination.d = 1;

    SDL_UploadToGPUTexture(copyPass, &source, &destination, false);

    SDL_EndGPUCopyPass(copyPass);

    // ミップマップが複数レベルある場合は自動生成する
    if (_numLevels > 1)
    {
        SDL_GenerateMipmapsForGPUTexture(commandBuffer, _texture);
    }

    SDL_SubmitGPUCommandBuffer(commandBuffer);

    // 転送バッファを解放
    SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
}

void CubismImageSDL3::GenerateMipmaps(SDL_GPUDevice* device)
{
    if (_texture == nullptr || _numLevels <= 1)
    {
        return;
    }

    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
    if (commandBuffer == nullptr)
    {
        CubismLogError("Failed to acquire command buffer for mipmap generation: %s", SDL_GetError());
        return;
    }

    SDL_GenerateMipmapsForGPUTexture(commandBuffer, _texture);

    SDL_SubmitGPUCommandBuffer(commandBuffer);
}

void CubismImageSDL3::Destroy(SDL_GPUDevice* device)
{
    if (_sampler != nullptr)
    {
        SDL_ReleaseGPUSampler(device, _sampler);
        _sampler = nullptr;
    }

    if (_texture != nullptr)
    {
        SDL_ReleaseGPUTexture(device, _texture);
        _texture = nullptr;
    }

    _width = 0;
    _height = 0;
    _numLevels = 1;
}

}}}
