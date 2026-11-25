#pragma once
// Minimal host stand-in for the library base class.
#include <stdio.h>
#include "Arduino.h"

struct AudioStatus {
    typedef void (*statusCBFn)(void*, int, const char*);
    statusCBFn fn = nullptr;
    void* data = nullptr;
    void st(int code, const char* text) { if (fn) fn(data, code, text); }
};

struct HostLogger {
    template <class... A> void printf_P(A...) {}
};
static HostLogger hostLogger;
static HostLogger* audioLogger = &hostLogger;

class AudioFileSource {
public:
    virtual ~AudioFileSource() {}
    virtual uint32_t read(void*, uint32_t) { return 0; }
    virtual uint32_t readNonBlock(void* d, uint32_t n) { return read(d, n); }
    virtual bool seek(int32_t, int) { return false; }
    virtual bool close() { return true; }
    virtual bool isOpen() { return true; }
    virtual uint32_t getSize() { return 0; }
    virtual uint32_t getPos() { return 0; }
    virtual bool loop() { return true; }
    bool RegisterStatusCB(AudioStatus::statusCBFn f, void* d) {
        cb.fn = f;
        cb.data = d;
        return true;
    }
protected:
    AudioStatus cb;
};
