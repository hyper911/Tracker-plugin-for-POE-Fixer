#pragma once

#include <string>
#include <filesystem>

// Load a PNG/JPEG/etc using WIC and create a D3D11 ShaderResourceView.
// d3dDevice is the host-provided ID3D11Device* (opaque void* here).
// On success returns a non-null ImTextureRef (void* usable by ImGui::Image())
// and fills outWidth/outHeight. Caller must not Release the returned pointer
// (the plugin should hold it and Release when done).
void* LoadTextureFromFileWIC(void* d3dDevice, const std::filesystem::path& path, int* outWidth, int* outHeight);

void ReleaseTexture(void* srv);
