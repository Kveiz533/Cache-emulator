#pragma once 

#include <cstddef>


constexpr size_t MEMORY_SIZE = 262144; // дано 
constexpr size_t CACHE_SIZE = 4096; // CACHE_LINE_COUNT * CACHE_LINE_SIZE
constexpr size_t CACHE_LINE_SIZE = 32; // 2^OFFSET_LEN
constexpr size_t CACHE_LINE_COUNT = 128; // CACHE_SET_COUNT * CACHE_WAY
constexpr size_t CACHE_WAY = 4; // дано
constexpr size_t CACHE_SET_COUNT = 32; // дано
constexpr size_t ADDRESS_LEN = 18; // log2(MEMORY_SIZE)
constexpr size_t CACHE_TAG_LEN = 8; // дано
constexpr size_t CACHE_INDEX_LEN = 5; // log2(CACHE_SET_COUNT)
constexpr size_t CACHE_OFFSET_LEN = 5; // ADDRESS_LEN - TAG_LEN - INDEX_LEN

