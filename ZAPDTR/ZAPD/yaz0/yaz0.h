#pragma once

#include <vector>

uint32_t yaz0_decode(const uint8_t* src, uint8_t* dest, int32_t destsize);
std::vector<uint8_t> yaz0_encode(const uint8_t* src, int src_size);
void yaz0_decodeYarArchive(const uint8_t* src, uint8_t* dest, size_t decompSize);
