/*
 * Simplified Gate Controller with Ultrasonic Sensor, Piezo Buzzer, and One LED.
 *
 * Behavior:
 * - The LED fades in (brightens) when the gate is triggered (an object is close) and fades out when released.
 * - The piezo buzzer beeps with a high-pitched tone when the gate activates and a low-pitched tone when it deactivates.
 * - A finite state machine (FSM) handles the gate logic.
 */

// ----- Pin Assignments -----
const int TRIG_PIN  = 10;   // Ultrasonic sensor trigger
const int ECHO_PIN  = 11;   // Ultrasonic sensor echo
const int PIEZO_PIN = 8;    // Piezo buzzer
const int LED_PIN   = 9;    // Single LED for gate status

// ----- Distance & Smoothing Parameters -----
const float MIN_DISTANCE   = 2.0;    // cm (minimum valid reading)
const float MAX_DISTANCE   = 400.0;  // cm (max or no response)
const float DISTANCE_ALPHA = 0.25;   // Smoothing factor for the exponential moving average (EMA)

// ----- Gate (FSM) Settings -----
// To trigger the gate: the object must be within GATE_TRIGGER_DISTANCE for at least GATE_TRIGGER_TIME.
// To release the gate: the object must be beyond GATE_RELEASE_DISTANCE for at least GATE_RELEASE_TIME.
const float GATE_TRIGGER_DISTANCE     = 100.0;  // cm
const int   GATE_TRIGGER_TIME         = 250;    // ms
const float GATE_RELEASE_DISTANCE     = 200.0;  // cm
const int   GATE_RELEASE_TIME         = 1000;   // ms

// ----- LED Fade Settings -----
const int LED_FADE_STEP = 5;   // Step size for LED brightness change per loop iteration (affects fade duration)

// ----- Global Variables -----
float distance = MAX_DISTANCE;         // Latest raw distance reading (cm)
float smoothedDistance = MAX_DISTANCE; // Smoothed distance value

// Define the finite state machine for the gate.
enum GateState { 
  IDLE,         // No object present or object is not close enough
  TRIGGERING,   // Object detected in trigger zone; waiting for trigger time
  TRIGGERED,    // Gate is active
  RELEASING     // Object is moving away; waiting for release time
};
GateState gateState = IDLE;
unsigned long stateStartTime = 0;  // Timestamp when the current state began

int ledBrightness = 0;  // Current brightness level for the LED (0 to 255)
GateState prevGateState = IDLE;  // Used to detect state transitions for beeping

// ----- Setup -----
void setup() {
  Serial.begin(115200);
  
  // Initialize sensor and output pins.
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(PIEZO_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  
  // Seed the smoothing filter with an initial sensor reading.
  float init = readUltrasonic();
  smoothedDistance = constrain(init, MIN_DISTANCE, MAX_DISTANCE);
}

// ----- Main Loop -----
void loop() {
  // 1. Read the ultrasonic sensor and apply EMA smoothing.
  distance = readUltrasonic();
  smoothedDistance = smoothDistance(distance);
  
  // 2. Update the gate state (FSM) based on the smoothed distance.
  updateGateState();
  
  // 3. Update the LED brightness with a smooth fade transition.
  updateLED();
  
  // 4. Check for state changes and beep accordingly.
  beepOnGateStateChange();
  
  // 5. Debug: print distance and gate state.
  Serial.print("Raw: ");
  Serial.print(distance);
  Serial.print(" cm, Smoothed: ");
  Serial.print(smoothedDistance);
  Serial.print(" cm, State: ");
  printGateState();
  Serial.println();
  
  delay(10);  // Small delay for loop stability
}

// ----- Function: Read Ultrasonic Sensor -----
// Sends a pulse and returns the measured distance in cm.
float readUltrasonic() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long dur = pulseIn(ECHO_PIN, HIGH, 30000);  // Timeout after 30ms
  const int speedOfSound = 0.0343; // from Arduino documentation
  return (dur == 0) ? MAX_DISTANCE : (dur * speedOfSound) / 2.0;
}

// ----- Function: Smooth Distance (EMA) -----
// Applies an exponential moving average to reduce sensor noise.
float smoothDistance(float newDist) {
  return DISTANCE_ALPHA * newDist + (1 - DISTANCE_ALPHA) * smoothedDistance;
}

// ----- Function: Update Gate State (FSM) -----
// Manages state transitions based on smoothed distance and elapsed time.
void updateGateState() {
  unsigned long now = millis();
  prevGateState = gateState;  // Save the previous state for beeping

  switch (gateState) {
    case IDLE:
      // Transition to TRIGGERING if object comes within trigger distance.
      if (smoothedDistance < GATE_TRIGGER_DISTANCE) {
        gateState = TRIGGERING;
        stateStartTime = now;
      }
      break;
      
    case TRIGGERING:
      // Remain in TRIGGERING if still within trigger distance.
      if (smoothedDistance < GATE_TRIGGER_DISTANCE) {
        if (now - stateStartTime >= GATE_TRIGGER_TIME) {
          gateState = TRIGGERED;  // Activate gate after required time.
        }
      } else {
        // If object leaves before time is up, return to IDLE.
        gateState = IDLE;
      }
      break;
      
    case TRIGGERED:
      // If object moves beyond release distance, begin releasing.
      if (smoothedDistance > GATE_RELEASE_DISTANCE) {
        gateState = RELEASING;
        stateStartTime = now;
      }
      break;
      
    case RELEASING:
      // If still beyond release distance long enough, return to IDLE.
      if (smoothedDistance > GATE_RELEASE_DISTANCE) {
        if (now - stateStartTime >= GATE_RELEASE_TIME) {
          gateState = IDLE;
        }
      } else {
        // If object comes back, revert to TRIGGERED.
        gateState = TRIGGERED;
      }
      break;
  }
}

// ----- Function: Update LED -----
// Adjusts the LED brightness gradually to create a fade effect.
// LED_FADE_STEP defines the incremental step per loop (i.e., how fast the LED fades).
void updateLED() {
  // Target brightness is 255 when gate is triggered, 0 otherwise.
  int target = (gateState == TRIGGERED) ? 255 : 0;
  
  if (ledBrightness < target) {
    ledBrightness += LED_FADE_STEP;
  } else if (ledBrightness > target) {
    ledBrightness -= LED_FADE_STEP;
  }
  
  // Ensure ledBrightness remains within valid PWM range [0, 255].
  ledBrightness = constrain(ledBrightness, 0, 255);
  
  analogWrite(LED_PIN, ledBrightness);
}

// ----- Beep Functions -----
// Beep when the gate is activated.
void beepTriggered() {
  tone(PIEZO_PIN, 1000, 200);  // 1000 Hz tone for 200ms
}

// Beep when the gate is released.
void beepReleased() {
  tone(PIEZO_PIN, 500, 200);   // 500 Hz tone for 200ms
}

// ----- Function: Beep on Gate State Change -----
// Checks for a change in the gate state and calls the corresponding beep function.
void beepOnGateStateChange() {
  if (gateState != prevGateState) {
    if (gateState == TRIGGERED && prevGateState != RELEASING) {
      beepTriggered();
    } else if (gateState == IDLE && (prevGateState == TRIGGERED || prevGateState == RELEASING)) {
      beepReleased();
    }
  }
}

// ----- Function: Print Gate State -----
// Prints the current gate state to the Serial Monitor for debugging.
void printGateState() {
  switch (gateState) {
    case IDLE: Serial.print("IDLE"); break;
    case TRIGGERING: Serial.print("TRIGGERING"); break;
    case TRIGGERED: Serial.print("TRIGGERED"); break;
    case RELEASING: Serial.print("RELEASING"); break;
  }
}
