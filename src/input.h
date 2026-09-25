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

    InputEvent getEvent();
    bool hasEvent() { return lastEvent != EVENT_NONE; }

    int8_t getEncoderDelta();

private:
    Bounce2::Button buttonPlay;
    Bounce2::Button buttonNext;
    Bounce2::Button buttonPrev;
    Bounce2::Button encoderClick;

    // EC11 quadrature decoder state.
    //
    // Both A/CLK and B/DT edges are observed. Only legal Gray-code
    // transitions count. Contact bounce that reverses direction cancels
    // itself instead of creating additional logical encoder movements.
    static volatile uint8_t encoderLastState;
    static volatile int8_t encoderTransitionAccumulator;
    static volatile int16_t encoderPendingSteps;

    static void IRAM_ATTR encoderISR();

    InputEvent lastEvent;
    uint32_t lastEventTime;
    uint32_t lastButtonPressTime;

    bool isLongPressing;
    static const uint32_t LONG_PRESS_TIME = 1500;
};

extern InputControl inputControl;

#endif // INPUT_H
