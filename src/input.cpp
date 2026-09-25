#include "input.h"
#include "config.h"
#include <limits.h>

// Global input control
InputControl inputControl;

// ============================================
// EC11 encoder shared state
// ============================================

volatile uint8_t InputControl::encoderLastState = 0;
volatile int8_t InputControl::encoderTransitionAccumulator = 0;
volatile int16_t InputControl::encoderPendingSteps = 0;

// ============================================
// EC11 quadrature decoder
// ============================================
//
// State encoding:
//
// bit 1 = CLK / S1
// bit 0 = DT  / S2
//
// Legal quadrature sequences:
//
// Direction A:
//   00 -> 10 -> 11 -> 01 -> 00
//
// Direction B:
//   00 -> 01 -> 11 -> 10 -> 00
//
// Every legal one-bit transition contributes +/-1.
//
// Mechanical contact bounce normally generates a transition followed
// immediately by its reverse. Because those have opposite signs, they
// cancel instead of creating extra movements.
//
// The EC11 module used in TUNER//01 produces four valid quadrature`r`n// transitions per mechanical detent, therefore`r`n// ENCODER_STEPS_PER_DETENT is 4.
//
// There is deliberately NO fixed microsecond debounce delay here.
// A time-based filter can discard legitimate quadrature edges and is
// not required when the Gray-code sequence itself is validated.
//
void IRAM_ATTR InputControl::encoderISR() {
    const uint8_t currentState =
        (digitalRead(ENCODER_CLK_PIN) ? 0x02 : 0x00) |
        (digitalRead(ENCODER_DT_PIN)  ? 0x01 : 0x00);

    const uint8_t previousState = encoderLastState;

    if (currentState == previousState) {
        return;
    }

    encoderLastState = currentState;

    int8_t delta = 0;

    switch ((previousState << 2) | currentState) {

        // 00 -> 10
        // 10 -> 11
        // 11 -> 01
        // 01 -> 00
        case 0x02:
        case 0x0B:
        case 0x0D:
        case 0x04:
            delta = +1;
            break;

        // 00 -> 01
        // 01 -> 11
        // 11 -> 10
        // 10 -> 00
        case 0x01:
        case 0x07:
        case 0x0E:
        case 0x08:
            delta = -1;
            break;

        default:
            // Invalid transition: both bits changed at once.
            //
            // This can be caused by missed edges or severe contact noise.
            // Do not convert it into movement. Discard the partial
            // sequence and resynchronize from the newly observed state.
            encoderTransitionAccumulator = 0;
            return;
    }

    encoderTransitionAccumulator += delta;

    if (encoderTransitionAccumulator >= ENCODER_STEPS_PER_DETENT) {
        if (encoderPendingSteps < INT16_MAX) {
            encoderPendingSteps++;
        }

        encoderTransitionAccumulator -= ENCODER_STEPS_PER_DETENT;
    }
    else if (encoderTransitionAccumulator <= -ENCODER_STEPS_PER_DETENT) {
        if (encoderPendingSteps > INT16_MIN) {
            encoderPendingSteps--;
        }

        encoderTransitionAccumulator += ENCODER_STEPS_PER_DETENT;
    }
}

// ============================================
// Constructor
// ============================================

InputControl::InputControl()
    : lastEvent(EVENT_NONE),
      lastEventTime(0),
      lastButtonPressTime(0),
      isLongPressing(false) {
}

InputControl::~InputControl() {
    detachInterrupt(digitalPinToInterrupt(ENCODER_CLK_PIN));
    detachInterrupt(digitalPinToInterrupt(ENCODER_DT_PIN));
}

// ============================================
// Initialization
// ============================================

bool InputControl::init() {

    // Buttons
    pinMode(BUTTON_PLAY_PIN, INPUT_PULLUP);
    pinMode(BUTTON_NEXT_PIN, INPUT_PULLUP);
    pinMode(BUTTON_PREV_PIN, INPUT_PULLUP);

    // EC11 encoder
    //
    // GPIO27 and GPIO32 both support internal pull-ups.
    // The encoder module is powered from 3.3 V.
    pinMode(ENCODER_CLK_PIN, INPUT_PULLUP);
    pinMode(ENCODER_DT_PIN, INPUT_PULLUP);
    pinMode(ENCODER_SW_PIN, INPUT_PULLUP);

    // Bounce2 buttons
    buttonPlay.attach(BUTTON_PLAY_PIN, INPUT_PULLUP);
    buttonPlay.interval(BUTTON_DEBOUNCE_MS);

    buttonNext.attach(BUTTON_NEXT_PIN, INPUT_PULLUP);
    buttonNext.interval(BUTTON_DEBOUNCE_MS);

    buttonPrev.attach(BUTTON_PREV_PIN, INPUT_PULLUP);
    buttonPrev.interval(BUTTON_DEBOUNCE_MS);

    encoderClick.attach(ENCODER_SW_PIN, INPUT_PULLUP);
    encoderClick.interval(BUTTON_DEBOUNCE_MS);

    // Synchronize the quadrature state machine with the encoder's
    // actual electrical state before enabling interrupts.
    encoderLastState =
        (digitalRead(ENCODER_CLK_PIN) ? 0x02 : 0x00) |
        (digitalRead(ENCODER_DT_PIN)  ? 0x01 : 0x00);

    encoderTransitionAccumulator = 0;
    encoderPendingSteps = 0;

    // Observe both quadrature channels.
    attachInterrupt(
        digitalPinToInterrupt(ENCODER_CLK_PIN),
        encoderISR,
        CHANGE
    );

    attachInterrupt(
        digitalPinToInterrupt(ENCODER_DT_PIN),
        encoderISR,
        CHANGE
    );

    Serial.printf(
        "[INPUT] Initialized: Buttons + EC11 Encoder (state=%u)\n",
        encoderLastState
    );

    return true;
}

// ============================================
// Update
// ============================================

void InputControl::update() {

    buttonPlay.update();
    buttonNext.update();
    buttonPrev.update();
    encoderClick.update();

    const uint32_t now = millis();

    if (now - lastEventTime > 200) {
        lastEvent = EVENT_NONE;
    }

    // ========================================
    // Buttons
    // ========================================

    if (buttonPlay.fell()) {
        lastEvent = EVENT_PLAY_PAUSE;
        lastEventTime = now;
        Serial.println("[INPUT] Play/Pause pressed");
    }

    if (buttonNext.fell()) {
        lastEvent = EVENT_NEXT;
        lastEventTime = now;
        Serial.println("[INPUT] Next pressed");
    }

    if (buttonPrev.fell()) {
        lastEvent = EVENT_PREV;
        lastEventTime = now;
        Serial.println("[INPUT] Prev pressed");
    }

    if (encoderClick.fell()) {
        lastEvent = EVENT_ENCODER_CLICK;
        lastEventTime = now;
        Serial.println("[INPUT] Encoder clicked");
    }

    // ========================================
    // Encoder
    // ========================================

    int8_t encoderStep = 0;

    noInterrupts();

    if (encoderPendingSteps > 0) {
        encoderPendingSteps--;
        encoderStep = +1;
    }
    else if (encoderPendingSteps < 0) {
        encoderPendingSteps++;
        encoderStep = -1;
    }

    interrupts();

    if (encoderStep > 0) {
        lastEvent = EVENT_ENCODER_UP;
        lastEventTime = now;

        Serial.println("[INPUT] Encoder UP");
    }
    else if (encoderStep < 0) {
        lastEvent = EVENT_ENCODER_DOWN;
        lastEventTime = now;

        Serial.println("[INPUT] Encoder DOWN");
    }

    // ========================================
    // Play button long press
    // ========================================

    if (buttonPlay.isPressed() && !isLongPressing) {

        const uint32_t pressDuration =
            buttonPlay.currentDuration();

        if (pressDuration > LONG_PRESS_TIME) {
            lastEvent = EVENT_LONG_PRESS;
            lastEventTime = now;
            isLongPressing = true;

            Serial.println("[INPUT] Long press detected");
        }
    }
    else if (!buttonPlay.isPressed()) {
        isLongPressing = false;
    }
}

// ============================================
// Event Retrieval
// ============================================

InputEvent InputControl::getEvent() {
    const InputEvent event = lastEvent;
    lastEvent = EVENT_NONE;
    return event;
}

// ============================================
// Encoder State
// ============================================

int8_t InputControl::getEncoderDelta() {
    return 0;
}

