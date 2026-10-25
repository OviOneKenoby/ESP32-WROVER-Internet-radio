#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

class TestSerial {
public:
    void println(const char*) {}

    template <typename... Args>
    void printf(const char*, Args...) {}
};

extern TestSerial Serial;
