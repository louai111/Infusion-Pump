#include <Keypad.h>
#include <avr/wdt.h>

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
    {'D', 'C', 'B', 'A'},
    {'#', '9', '6', '3'},
    {'0', '8', '5', '2'},
    {'*', '7', '4', '1'}
};
byte rowPins[ROWS] = {2, 3, 4, 5};
byte colPins[COLS] = {6, 7, 8, 9};
int buzzerPin = 12;

double weight;
float flowRate = 0;
float measuredFlowRate = 0;
int pump_pin = 10;       
int flowPin = 21;       
volatile int count = 0; 
int pump_speed = 128;   
unsigned long lastMillis = 0;
char mode;

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup() {
  pinMode(buzzerPin, OUTPUT);
  pinMode(pump_pin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  Serial.begin(9600);
  Serial.println("Pump and Flow Rate Measurement Initialized");
  modeSelection();
  if (mode == 'A') 
      drugSelection();
  else if (mode == '#') 
      manualSelection();
  else 
      Serial.println("Invalid mode selected.");

  pinMode(flowPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(flowPin), Flow, RISING);
}

void loop() {
  measureFlowRate();
  modifySpeed();
  checkAlarm();
  delay(500);
}

void checkAlarm() {
  if (measuredFlowRate < 12) { 
    digitalWrite(buzzerPin, HIGH); 
    Serial.println("CHECK FOR OCCLUSIONS");
  } else {
    digitalWrite(buzzerPin, LOW);
  }

  // if (flowRate - (flowRate * 0.5) > measuredFlowRate) {
  //   digitalWrite(buzzerPin, HIGH);
  // } else {
  //   digitalWrite(buzzerPin, LOW);
  // }
}

void modifySpeed() {
  if (measuredFlowRate < flowRate) pump_speed = min(255, pump_speed + 10);
    else if (measuredFlowRate > flowRate) pump_speed = max(0, pump_speed - 10);

  analogWrite(pump_pin, pump_speed);
}

void modeSelection() {
    Serial.println("Select Mode:");
    Serial.println("Press 'A' for Drug Library Mode");
    Serial.println("Press '#' for Manual Mode");
    char key = '\0';
    while (!key) {
        key = keypad.getKey();
        if (key == 'A' || key == '#') {
            mode = key;
            Serial.print("Mode Selected: ");
            Serial.println(mode);
            break;
        } else if (key == 'C') {
          reset();
        } else {
          key = false;
        }
    }
}

void manualSelection() {
    Serial.println("Manual Mode Selected.");
    Serial.println("Enter flow rate:  mL/min");
    Serial.println("Use 'D' to delete, '*' for a decimal point, and '#' to confirm.");
    
    String input = "";
    char key;
    while (true) {
        key = keypad.getKey();
        if (key) {
            if (key == 'D' && input.length() > 0) {
                input.remove(input.length() - 1);
                Serial.print("Current Input: ");
                Serial.println(input);
            } else if (key == '*') {
                input += '.';
                Serial.print("Current Input: ");
                Serial.println(input);
            } else if (key >= '0' && key <= '9') {
                input += key;
                Serial.print("Current Input: ");
                Serial.print(input);
                Serial.println(" mL/min");
            } else if (key == '#') {
                flowRate = input.toFloat();
                Serial.print("Flow Rate Set To: ");
                Serial.print(flowRate);
                Serial.println(" mL/min");
                break;
            } else if (key == 'C') reset();
        }
    }
}

void drugSelection() {
    Serial.println("Drug Library Mode Selected.");
    Serial.println("Enter patient weight in kg:");
    
    String weightInput = "";
    char key;
    while (true) {
        key = keypad.getKey();
        if (key) {
            if (key == 'D' && weightInput.length() > 0) {
                weightInput.remove(weightInput.length() - 1);
                Serial.print("Current Weight Input: ");
                Serial.println(weightInput);
            } else if (key >= '0' && key <= '9') {
                weightInput += key;
                Serial.print("Current Weight Input: ");
                Serial.println(weightInput);
            } else if (key == '#') {
                weight = weightInput.toDouble();
                Serial.print("Weight Set To: ");
                Serial.println(weight);
                break;
            } else if (key == 'C') reset();
        }
    }

    Serial.println("Select Drug:");
    Serial.println("Press '1' for Dopamine");
    Serial.println("Press '2' for Insulin");
    Serial.println("Press '3' for Fentanyl");
    char drugKey = '\0';

    // float dopamine_dosage = 10;
    // float insulin_dosage = 5;
    // float fentanyl_dosage = 5;

    // float dopamine_conc = 40;
    // float insulin_conc = 100;
    // float fentanyl_conc = 50;

    while (!drugKey) {
        drugKey = keypad.getKey();
        if (drugKey == '1' || drugKey == '2' || drugKey == '3') {
            Serial.print("Drug Selected: ");
            if (drugKey == '1') {
                Serial.println("Dopamine");
                // flowRate = dopamine_dosage * weight * 60 / dopamine_conc;
                flowRate = 105;
            } else if (drugKey == '2') {
                Serial.println("Insulin");
                // flowRate = insulin_dosage / insulin_conc;
                flowRate = 20;
            } else if (drugKey == '3') {
                Serial.println("Fentanyl");
                // flowRate = fentanyl_dosage / fentanyl_conc;
                flowRate = 30;
            }
            Serial.print("Flow Rate Set To: ");
            Serial.print(flowRate);
            Serial.println(" mL/min");

            break;
        }
        else if (drugKey == 'C') reset();
        else {
          drugKey = false;
        }
    }
}

void measureFlowRate() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastMillis >= 1000) {
    noInterrupts();  
    int pulses = count;
    count = 0; 
    interrupts();   
    
    measuredFlowRate = (pulses * 1000.0) * 1000 / 5880.0 / (currentMillis - lastMillis);

    Serial.print("Required Flow Rate: ");
    Serial.print(flowRate);
    Serial.println(" ml/min");

    Serial.print("------------ Measured Flow Rate: ");
    Serial.print(measuredFlowRate);
    Serial.println(" mL/min");

    lastMillis = currentMillis;
  }
}

void Flow() {
  count++;
}

void reset() {
  wdt_enable(WDTO_15MS);
  while (true); 
}