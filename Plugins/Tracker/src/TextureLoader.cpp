#include "TextureLoader.h"
#include <d3d11.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <memory>

using Microsoft::WRL::ComPtr;

void* LoadTextureFromFileWIC(void* d3dDevice, const std::filesystem::path& path, int* outWidth, int* outHeight) {
    if (!d3dDevice) return nullptr;
    ID3D11Device* device = static_cast<ID3D11Device*>(d3dDevice);
    if (!device) return nullptr;

    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool coInited = SUCCEEDED(hr);

    ComPtr<IWICImagingFactory> factory;
    hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
    if (FAILED(hr) || !factory) {
        if (coInited) CoUninitialize();
        return nullptr;
    }

    ComPtr<IWICBitmapDecoder> decoder;
    // IWICImagingFactory::CreateDecoderFromFilename expects a wide string (LPCWSTR)
    hr = factory->CreateDecoderFromFilename(path.wstring().c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
    if (FAILED(hr) || !decoder) {
        if (coInited) CoUninitialize();
        return nullptr;
    }

    ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr) || !frame) { if (coInited) CoUninitialize(); return nullptr; }

    ComPtr<IWICFormatConverter> converter;
    hr = factory->CreateFormatConverter(&converter);
    if (FAILED(hr) || !converter) { if (coInited) CoUninitialize(); return nullptr; }

    hr = converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) { if (coInited) CoUninitialize(); return nullptr; }

    UINT width = 0, height = 0;
    hr = converter->GetSize(&width, &height);
    if (FAILED(hr) || width == 0 || height == 0) { if (coInited) CoUninitialize(); return nullptr; }

    const UINT stride = width * 4;
    std::unique_ptr<BYTE[]> pixels(new (std::nothrow) BYTE[height * stride]);
    if (!pixels) { if (coInited) CoUninitialize(); return nullptr; }

    hr = converter->CopyPixels(nullptr, stride, height * stride, pixels.get());
    if (FAILED(hr)) { if (coInited) CoUninitialize(); return nullptr; }

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // matches WIC 32bpp BGRA
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA initData{};
    initData.pSysMem = pixels.get();
    initData.SysMemPitch = stride;

    ComPtr<ID3D11Texture2D> tex;
    hr = device->CreateTexture2D(&desc, &initData, &tex);
    if (FAILED(hr) || !tex) { if (coInited) CoUninitialize(); return nullptr; }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    ID3D11ShaderResourceView* srv = nullptr;
    hr = device->CreateShaderResourceView(tex.Get(), &srvDesc, &srv);
    if (FAILED(hr) || !srv) { if (coInited) CoUninitialize(); return nullptr; }

    if (outWidth) *outWidth = static_cast<int>(width);
    if (outHeight) *outHeight = static_cast<int>(height);

    if (coInited) CoUninitialize();
    return static_cast<void*>(srv);
}

void ReleaseTexture(void* srv) {
    if (!srv) return;
    ID3D11ShaderResourceView* view = static_cast<ID3D11ShaderResourceView*>(srv);
    view->Release();
}
