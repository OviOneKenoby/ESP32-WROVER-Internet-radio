#pragma once
// Builds a JSON string literal (including the surrounding quotes) for ANY input
// bytes. Every control character below 0x20 and 0x7F is written as \u00XX, quote
// and backslash are escaped, and a byte that is not part of a valid UTF-8
// character is written as U+FFFD, so the result is always valid strict JSON.
// Output is appended to a String-like object (operator+= (char) / (const char*)).
// Pure and header-only so the host tests can call it.
#include "browse_text.h"
#include <cstddef>
#include <cstdint>

namespace json_text {

template <class Out>
inline void appendQuoted(Out& out, const char* text, size_t size) {
    static const char hex[] = "0123456789abcdef";
    const uint8_t* in = reinterpret_cast<const uint8_t*>(text);
    out += '"';
    for (size_t at = 0; in && at < size;) {
        const uint8_t c = in[at];
        if (c == '"' || c == '\\') {
            out += '\\';
            out += char(c);
            ++at;
        } else if (c < 0x20 || c == 0x7f) {
            out += "\\u00";
            out += hex[c >> 4];
            out += hex[c & 15];
            ++at;
        } else if (c < 0x80) {
            out += char(c);
            ++at;
        } else {
            const size_t width = browse_text::validWidth(in, size, at);
            if (!width) {
                out += "\\ufffd";
                ++at;
            } else {
                for (size_t i = 0; i < width; ++i) out += char(in[at + i]);
                at += width;
            }
        }
    }
    out += '"';
}

template <class Out>
inline void appendQuoted(Out& out, const char* text) {
    size_t size = 0;
    while (text && text[size]) ++size;
    appendQuoted(out, text, size);
}

}  // namespace json_text
