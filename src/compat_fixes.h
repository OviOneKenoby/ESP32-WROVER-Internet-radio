#pragma once
// The ESP32-A2DP library (BluetoothA2DPOutput.h) calls a bare, unqualified
// min() and expects it to exist in the global namespace - this was true on
// classic Arduino AVR cores, which historically defined global min()/max()
// macros, but isn't guaranteed on this ESP32 core/toolchain combination.
// The library normally gets this from the optional "AudioTools" library
// (hence its "AudioTools library is not included first or installed"
// warning) or from a legacy ESP-IDF<5.0 code path - neither applies here,
// so we provide it ourselves, globally, via -include in platformio.ini.
//
// This header gets force-included into EVERY compiled file, including
// plain C files (e.g. glcdfont.c in Adafruit GFX) - <algorithm> and
// std::min/max are C++-only, so this must be a no-op when compiled as C.
#ifdef __cplusplus
#include <algorithm>
using std::max;
using std::min;
#endif
