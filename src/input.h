#ifndef INPUT_H
#define INPUT_H

#include <Arduino.h>
#include <Bounce2.h>

enum ButtonType {
    BUTTON_PLAY_PAUSE,
    BUTTON_NEXT,
    BUTTON_PREV,
    BUTTON_ENCODER_CLICK
};

enum InputEvent {
    EVENT_NONE,
    EVENT_PLAY_PAUSE,
    EVENT_NEXT,
    EVENT_PREV,
    EVENT_ENCODER_UP,
    EVENT_ENCODER_DOWN,
    EVENT_ENCODER_CLICK,
    EVENT_LONG_PRESS
};

class InputControl {
public:
    InputControl();
    ~InputControl();
    
    bool init();
    void update();
    
    // Get last event
    InputEvent getEvent();
    bool hasEvent() { return lastEvent != EVENT_NONE; }
    
    // Encoder state
    int8_t getEncoderDelta();
    
private:
    // Button instances
    Bounce2::Button buttonPlay;
    Bounce2::Button buttonNext;
    Bounce2::Button buttonPrev;
    Bounce2::Button encoderClick;
    
    // Rotary encoder - static because the ISR (also static) needs to
    // modify this state without an object instance. Previously this was
    // split between a class member (read by update(), never written) and
    // a separate same-named file-scope global (written by a free-function
    // ISR, never read by update()) - two different variables with the same
    // name, silently disconnected from each other. Consolidated into one.
    //
    // Switched from a dual-pin (CLK+DT both CHANGE-triggered), full 4-state
    // Gray code decoder to the simpler single-edge approach below: this is
    // a well-known failure mode of the dual-pin approach - many common
    // rotary encoder modules produce 2+ electrical transitions per single
    // mechanical detent click, and triggering on CHANGE of both pins
    // multiplies that further, producing multiple registered movements per
    // physical click (reported directly: "one click = two movements").
    // Triggering on only CLK's FALLING edge and reading DT synchronously
    // at that moment is the standard, far more reliable pattern for this
    // class of encoder.
    //
    // Declared here (no initializer - that's C++17-only "inline static"
    // syntax, which triggered a compiler warning suggesting the active
    // build wasn't reliably using C++17 despite platformio.ini requesting
    // it). Actual definition + initialization is in input.cpp instead,
    // which works correctly under any C++ standard.
    static volatile int16_t encoderPosition;
    static volatile uint32_t encoderLastInterruptMicros;
    static void IRAM_ATTR encoderISR();
    
    // Event handling
    InputEvent lastEvent;
    uint32_t lastEventTime;
    uint32_t lastButtonPressTime;
    
    // Long press detection
    bool isLongPressing;
    static const uint32_t LONG_PRESS_TIME = 1500; // ms
};

extern InputControl inputControl;

#endif // INPUT_H
