#pragma once

#include <cstdint>

constexpr uint32_t MEMORY_SIZE = 256 * 1024;
constexpr uint32_t ADDRESS_LEN = 18;

constexpr uint32_t CACHE_TAG_LEN = 8;
constexpr uint32_t CACHE_INDEX_LEN = 5;
constexpr uint32_t CACHE_OFFSET_LEN = ADDRESS_LEN - CACHE_TAG_LEN - CACHE_INDEX_LEN;

constexpr uint32_t CACHE_LINE_SIZE = 1u << CACHE_OFFSET_LEN;
constexpr uint32_t CACHE_SET_COUNT = 32;
constexpr uint32_t CACHE_WAY = 4;
constexpr uint32_t CACHE_LINE_COUNT = CACHE_SET_COUNT * CACHE_WAY;
constexpr uint32_t CACHE_SIZE = CACHE_LINE_COUNT * CACHE_LINE_SIZE;
