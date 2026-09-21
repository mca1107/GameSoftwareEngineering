#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace AssetCache
{
bool Load(const wchar_t* name,
          std::uint32_t version,
          std::size_t expectedBytes,
          std::vector<unsigned char>& bytes);
void Save(const wchar_t* name, std::uint32_t version, const void* data, std::size_t size);
}
