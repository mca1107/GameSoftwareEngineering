#include "stdafx.h"
#include "AssetCache.h"
#include <windows.h>
#include <fstream>
#include <string>
#include <iostream>

namespace
{
constexpr std::uint32_t Magic = 0x53474348;
constexpr std::size_t MaxBytes = 8 * 1024 * 1024;

std::wstring Path(const wchar_t* name)
{
    wchar_t executable[32768] = {};
    DWORD length = GetModuleFileNameW(nullptr, executable, 32768);
    if (!length || length >= 32768)
    {
        return {};
    }
    std::wstring folder(executable, length);
    folder = folder.substr(0, folder.find_last_of(L"\\/")) + L"\\Cache";
    CreateDirectoryW(folder.c_str(), nullptr);
    return folder + L"\\" + name;
}

std::uint32_t Checksum(const unsigned char* data, std::size_t size)
{
    std::uint32_t hash = 2166136261u;
    for (std::size_t i = 0; i < size; ++i)
    {
        hash = (hash ^ data[i]) * 16777619u;
    }
    return hash;
}
}

bool AssetCache::Load(const wchar_t* name,
                      std::uint32_t version,
                      std::size_t expectedBytes,
                      std::vector<unsigned char>& bytes)
{
    bytes.clear();
    std::ifstream file(Path(name).c_str(), std::ios::binary);
    std::uint32_t header[4] = {};
    if (!file.read(reinterpret_cast<char*>(header), sizeof(header)) || header[0] != Magic ||
        header[1] != version || !header[2] || header[2] > MaxBytes ||
        (expectedBytes && header[2] != expectedBytes))
    {
        return false;
    }
    bytes.resize(header[2]);
    if (!file.read(reinterpret_cast<char*>(bytes.data()), bytes.size()) ||
        file.peek() != std::char_traits<char>::eof() ||
        Checksum(bytes.data(), bytes.size()) != header[3])
    {
        bytes.clear();
        return false;
    }
    std::wcout << L"Loaded model cache: " << name << L"\n";
    return true;
}

void AssetCache::Save(const wchar_t* name,
                      std::uint32_t version,
                      const void* data,
                      std::size_t size)
{
    if (!size || size > MaxBytes)
    {
        return;
    }
    std::wstring path = Path(name);
    if (path.empty())
    {
        return;
    }
    std::wstring temporary = path + L"." + std::to_wstring(GetCurrentProcessId()) + L".tmp";
    std::uint32_t header[] = {Magic,
                              version,
                              static_cast<std::uint32_t>(size),
                              Checksum(static_cast<const unsigned char*>(data), size)};
    std::ofstream file(temporary.c_str(), std::ios::binary | std::ios::trunc);
    file.write(reinterpret_cast<const char*>(header), sizeof(header));
    file.write(static_cast<const char*>(data), size);
    file.close();
    if (!file || !MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING))
    {
        DeleteFileW(temporary.c_str());
        std::wcerr << L"Cache unavailable; generated model stays in memory: " << name << L"\n";
    }
}
