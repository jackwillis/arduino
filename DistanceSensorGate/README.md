# Distance Sensor Gate with Arduino

## Overview
This project creates an interactive **distance-based gate** using an **ultrasonic sensor (HC-SR04)**, a **blue LED**, and a **piezo buzzer**. 

- As a person or object approaches, the **LED gradually fades in**.
- When they move away, the **LED fades out**.
- A **buzzer beeps when the gate opens and closes**.
- The system runs on a **finite state machine (FSM)** to ensure smooth transitions.

This can be used for **interactive installations**, **shadow boxes**, or **proximity-based lighting effects**.

---

## How It Works
1. The **ultrasonic sensor** measures distance.
2. If the object is **close enough for a set time**, the **gate activates**:
   - **LED fades in** (PWM effect)
   - **Buzzer plays an entry tone**
3. When the object **moves away past a second threshold**, the **gate deactivates**:
   - **LED fades out**
   - **Buzzer plays an exit tone**
4. The system **smooths sensor readings** to prevent flickering.

---

## Components
| Component          | Function |
|-------------------|----------|
| **Arduino Uno**   | Controls the system |
| **HC-SR04**       | Measures distance |
| **Blue LED**      | Indicates gate status |
| **220Ω Resistor** | Limits LED current |
| **Piezo Buzzer**  | Provides audio feedback |
| **Jumper Wires**  | Connects components |
| **Breadboard**    | For prototyping |

---

## Circuit Diagram
### **Breadboard Layout**
![Breadboard Layout](DistanceSensorGate_bb.png)

### **Schematic**
![Schematic](DistanceSensorGate_schem.png)

---

## Wiring Guide
### **Ultrasonic Sensor (HC-SR04)**
- **VCC** → 5V  
- **GND** → GND  
- **TRIG** → Pin **10**  
- **ECHO** → Pin **11**  

### **LED**
- **Anode (+)** → **220Ω Resistor** → Pin **9**  
- **Cathode (-)** → GND  

### **Piezo Buzzer**
- **Positive (+)** → Pin **8**  
- **Negative (-)** → GND  

---

## Operation
1. **Upload the provided code** to the Arduino.
2. Place an object **within ~100 cm** → The **LED fades in** and the **buzzer beeps**.
3. Stay in range for **250ms** to activate the gate.
4. Move **beyond 200 cm** for **1 second** → The **LED fades out** and the **buzzer signals exit**.

---

## Adjusting the Behavior
- **Change `GATE_TRIGGER_DISTANCE`** *(default: 100 cm)* to adjust the activation range.
- **Modify `GATE_RELEASE_DISTANCE`** *(default: 200 cm)* to change when the gate turns off.
- **Adjust `LED_FADE_STEP`** *(default: 5)* for a slower or faster fade.
- **Customize `beepTriggered()` and `beepReleased()`** for different entry/exit sounds.

---

## Troubleshooting
- **Nothing is working?** Double-check wiring, especially **power (5V, GND)** connections.
- **Sensor acting strangely?** Ensure it's **not facing reflective surfaces** or **too far from objects**.
- **LED not lighting up?** Check if it's wired correctly and the resistor is in series.
- **Buzzer not making sound?** Make sure it's wired with the **positive lead to Pin 8**.

---

## Next Steps
- Integrate into a **shadow box or two-way mirror** for hidden reveals.
- Add **multiple sensors** for more interactive zones.
- Control **other outputs** (e.g., motors, relays) for expanded installations.
