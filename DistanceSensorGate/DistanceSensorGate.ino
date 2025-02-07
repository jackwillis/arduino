/*
 * Simplified Gate Controller with Ultrasonic Sensor, Piezo Buzzer, and One LED.
 *
 * Behavior:
 * - The LED fades in (brightens) when the gate is triggered (an object is close) and fades out when released.
 * - The piezo buzzer beeps with a high-pitched tone when the gate activates and a low-pitched tone when it deactivates.
 * - A finite state machine (FSM) handles the gate logic.
 * - Serial output is reduced using a frame counter to improve efficiency.
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
const float GATE_TRIGGER_DISTANCE     = 100.0;  // cm
const int   GATE_TRIGGER_TIME         = 250;    // ms
const float GATE_RELEASE_DISTANCE     = 200.0;  // cm
const int   GATE_RELEASE_TIME         = 1000;   // ms

// ----- LED Fade Settings -----
const int LED_FADE_STEP = 5;   // Step size for LED brightness change per loop iteration (affects fade duration)

// ----- Frame Counter -----
const int FRAME_INTERVAL = 20; // Only print to Serial every 20 frames
int frameCounter = 0;

// ----- Global Variables -----
float distance = MAX_DISTANCE;
float smoothedDistance = MAX_DISTANCE;

enum GateState { IDLE, TRIGGERING, TRIGGERED, RELEASING };
GateState gateState = IDLE;
unsigned long stateStartTime = 0;

int ledBrightness = 0;
GateState prevGateState = IDLE;

// ----- Setup -----
void setup() {
  Serial.begin(9600);
  pinMode(TRIG_PIN,   OUTPUT);
  pinMode(ECHO_PIN,   INPUT);
  pinMode(PIEZO_PIN,  OUTPUT);
  pinMode(LED_PIN,    OUTPUT);
  float init = readUltrasonic();
  smoothedDistance = constrain(init, MIN_DISTANCE, MAX_DISTANCE);
}

// ----- Main Loop -----
void loop() {
  distance = readUltrasonic();
  smoothedDistance = smoothDistance(distance);
  updateGateState();
  updateLED();
  beepOnGateStateChange();
  
  if (++frameCounter >= FRAME_INTERVAL || gateState != prevGateState) {
    Serial.print("Raw: "); Serial.print(distance);
    Serial.print(" cm, Smoothed: "); Serial.print(smoothedDistance);
    Serial.print(" cm, State: ");
    printGateState();
    Serial.println();
    frameCounter = 0;
  }
  
  delay(10);
}

// ----- Function: Read Ultrasonic Sensor -----
float readUltrasonic() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long dur = pulseIn(ECHO_PIN, HIGH, 30000);
  // Time constant taken from Arduino documentation
  return (dur == 0) ? MAX_DISTANCE : (dur * 0.0343) / 2.0;
}

// ----- Function: Smooth Distance (EMA) -----
float smoothDistance(float newDist) {
  return DISTANCE_ALPHA * newDist + (1 - DISTANCE_ALPHA) * smoothedDistance;
}

// ----- Function: Update Gate State (FSM) -----
void updateGateState() {
  unsigned long now = millis();
  prevGateState = gateState;

  switch (gateState) {
    case IDLE:
      if (smoothedDistance < GATE_TRIGGER_DISTANCE) {
        gateState = TRIGGERING;
        stateStartTime = now;
      }
      break;
    case TRIGGERING:
      if (smoothedDistance < GATE_TRIGGER_DISTANCE) {
        if (now - stateStartTime >= GATE_TRIGGER_TIME) gateState = TRIGGERED;
      } else gateState = IDLE;
      break;
    case TRIGGERED:
      if (smoothedDistance > GATE_RELEASE_DISTANCE) {
        gateState = RELEASING;
        stateStartTime = now;
      }
      break;
    case RELEASING:
      if (smoothedDistance > GATE_RELEASE_DISTANCE) {
        if (now - stateStartTime >= GATE_RELEASE_TIME) gateState = IDLE;
      } else gateState = TRIGGERED;
      break;
  }
}

// ----- Function: Update LED -----
void updateLED() {
  int target = (gateState == TRIGGERED || gateState == RELEASING) ? 255 : 0;
  int ledBrightnessChange = (ledBrightness < target) ? LED_FADE_STEP : -LED_FADE_STEP;
  ledBrightness += ledBrightnessChange;
  ledBrightness = constrain(ledBrightness, 0, 255);
  analogWrite(LED_PIN, ledBrightness);
}

// ----- Beep Functions -----
void beepTriggered() {
  tone(PIEZO_PIN, 1000, 200);
}
void beepReleased() {
  tone(PIEZO_PIN, 500, 200);
}

// ----- Function: Beep on Gate State Change -----
void beepOnGateStateChange() {
  if (gateState == prevGateState) {
    return;
  }

  switch (gateState) {
    case TRIGGERED:
      if (prevGateState != RELEASING) {
        beepTriggered();
      }
      break;

    case IDLE:
      if (prevGateState == TRIGGERED || prevGateState == RELEASING) {
        beepReleased();
      }
      break;
  }
}


// ----- Function: Print Gate State -----
void printGateState() {
  switch (gateState) {
    case IDLE: Serial.print("IDLE"); break;
    case TRIGGERING: Serial.print("TRIGGERING"); break;
    case TRIGGERED: Serial.print("TRIGGERED"); break;
    case RELEASING: Serial.print("RELEASING"); break;
  }
}
