#include "input.h"
#include "config.h"

// Global input control
InputControl inputControl;

// Static member definitions - see input.h for why these aren't declared
// with C++17 "inline static" syntax instead.
volatile int16_t InputControl::encoderPosition = 0;
volatile uint32_t InputControl::encoderLastInterruptMicros = 0;

// ISR for rotary encoder - this is InputControl::encoderISR, matching the
// static member declared in input.h. attachInterrupt() in init() below
// resolves the unqualified name "encoderISR" to this class member (member
// lookup takes priority inside a member function body), so this is the
// definition that actually needs to exist - a same-named free function
// here would silently never be called.
//
// Triggered only on ENCODER_CLK_PIN's falling edge (see init() below) -
// direction is read synchronously by comparing DT's level against CLK's
// at that moment, the standard technique for this class of module.
void IRAM_ATTR InputControl::encoderISR() {
    // Minimum interval between accepted clicks - filters contact bounce
    // without being tight enough to risk dropping a fast legitimate turn.
    // NOTE: this is a mitigation, not a fix, for a floating/unpulled CLK
    // pin (see config.h ENCODER_CLK_PIN comment) - it can reduce how often
    // noise gets registered as a click, but it cannot fully stop a
    // genuinely floating pin from eventually tripping the threshold. If
    // you're seeing continuous unprompted scrolling, add the pull-up
    // resistor (or move to a different GPIO) rather than just raising this
    // number further.
    uint32_t nowMicros = micros();
    if (nowMicros - encoderLastInterruptMicros < 5000) {
        return;
    }
    encoderLastInterruptMicros = nowMicros;
    
    if (digitalRead(ENCODER_DT_PIN) != digitalRead(ENCODER_CLK_PIN)) {
        encoderPosition++;
    } else {
        encoderPosition--;
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
    // Detach interrupt
    detachInterrupt(digitalPinToInterrupt(ENCODER_CLK_PIN));
}

// ============================================
// Initialization
// ============================================
bool InputControl::init() {
    // Initialize button pins
    pinMode(BUTTON_PLAY_PIN, INPUT_PULLUP);
    pinMode(BUTTON_NEXT_PIN, INPUT_PULLUP);
    pinMode(BUTTON_PREV_PIN, INPUT_PULLUP);
    
    // Initialize encoder pins
    pinMode(ENCODER_CLK_PIN, INPUT_PULLUP);
    pinMode(ENCODER_DT_PIN, INPUT_PULLUP);
    pinMode(ENCODER_SW_PIN, INPUT_PULLUP);
    
    // Configure Bounce2 buttons
    buttonPlay.attach(BUTTON_PLAY_PIN, INPUT_PULLUP);
    buttonPlay.interval(BUTTON_DEBOUNCE_MS);
    
    buttonNext.attach(BUTTON_NEXT_PIN, INPUT_PULLUP);
    buttonNext.interval(BUTTON_DEBOUNCE_MS);
    
    buttonPrev.attach(BUTTON_PREV_PIN, INPUT_PULLUP);
    buttonPrev.interval(BUTTON_DEBOUNCE_MS);
    
    encoderClick.attach(ENCODER_SW_PIN, INPUT_PULLUP);
    encoderClick.interval(BUTTON_DEBOUNCE_MS);
    
    // Attach encoder ISR - only on CLK's falling edge. Previously this
    // attached CHANGE-triggered interrupts on BOTH CLK and DT, which is
    // what caused multiple registered movements per single physical click.
    attachInterrupt(digitalPinToInterrupt(ENCODER_CLK_PIN), encoderISR, FALLING);
    
    Serial.println("[INPUT] Initialized: Buttons and Encoder");
    return true;
}

// ============================================
// Update - Call regularly (10-20ms interval)
// ============================================
void InputControl::update() {
    // Update button states
    buttonPlay.update();
    buttonNext.update();
    buttonPrev.update();
    encoderClick.update();
    
    uint32_t now = millis();
    
    // Clear old events
    if (now - lastEventTime > 200) {
        lastEvent = EVENT_NONE;
    }
    
    // Check for button press events
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
    
    // Check for encoder rotation
    if (encoderPosition >= ENCODER_STEPS_PER_DETENT) {
        lastEvent = EVENT_ENCODER_UP;
        Serial.printf("[INPUT] Encoder UP (raw=%d)\n", encoderPosition);
        lastEventTime = now;
        encoderPosition -= ENCODER_STEPS_PER_DETENT;
    } else if (encoderPosition <= -ENCODER_STEPS_PER_DETENT) {
        lastEvent = EVENT_ENCODER_DOWN;
        Serial.printf("[INPUT] Encoder DOWN (raw=%d)\n", encoderPosition);
        lastEventTime = now;
        encoderPosition += ENCODER_STEPS_PER_DETENT;
    }
    
    // Check for long press
    if (buttonPlay.isPressed() && !isLongPressing) {
        uint32_t pressDuration = buttonPlay.currentDuration();
        if (pressDuration > LONG_PRESS_TIME) {
            lastEvent = EVENT_LONG_PRESS;
            lastEventTime = now;
            isLongPressing = true;
            Serial.println("[INPUT] Long press detected");
        }
    } else if (!buttonPlay.isPressed()) {
        isLongPressing = false;
    }
}

// ============================================
// Event Retrieval
// ============================================
InputEvent InputControl::getEvent() {
    InputEvent event = lastEvent;
    lastEvent = EVENT_NONE;
    return event;
}

// ============================================
// Encoder State
// ============================================
int8_t InputControl::getEncoderDelta() {
    // Can be used to get precise encoder position if needed
    // Currently using direct ISR approach
    return 0;
}
