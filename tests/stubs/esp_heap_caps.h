#pragma once

#include <cstddef>
#include <cstdint>

static constexpr uint32_t MALLOC_CAP_8BIT = 1U << 0;
static constexpr uint32_t MALLOC_CAP_INTERNAL = 1U << 1;
static constexpr uint32_t MALLOC_CAP_SPIRAM = 1U << 2;

extern "C" void* heap_caps_malloc(size_t size, uint32_t caps);
extern "C" void* heap_caps_realloc(void* pointer, size_t size, uint32_t caps);
extern "C" void heap_caps_free(void* pointer);
