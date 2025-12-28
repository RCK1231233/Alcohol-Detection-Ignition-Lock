#include <LiquidCrystal.h>

// Initialize the LCD with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 10, 9, 8, 7);

const int sensor = A0;      // MQ-3 alcohol sensor connected to Analog A0
const int led = 13;         // Warning LED connected to Digital 13
const int relay = 5;        // Relay module connected to Digital 5
const int touchPin = 2;     // Touch sensor connected to Digital 2

float previousMgL = -1;     // Variable to track BAC changes
bool engineStarted = false; // Variable to track engine ignition status

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  
  // Initial startup message
  lcd.print("Alcohol Detector");
  delay(2000);

  pinMode(sensor, INPUT);
  pinMode(led, OUTPUT);
  pinMode(relay, OUTPUT);
  pinMode(touchPin, INPUT);

  // Ensure relay is off initially (active-low logic common in relay modules)
  digitalWrite(relay, LOW); 
  lcd.clear();
}

// Function to scroll long messages on the second line of the LCD
void scrollMessage(const String& message) {
  int messageLength = message.length();
  int lcdColumns = 16;

  for (int position = 0; position < messageLength; position++) {
    lcd.setCursor(0, 1);
    lcd.print("                "); // Clear only the second line
    lcd.setCursor(0, 1);
    lcd.print(message.substring(position, position + lcdColumns));
    delay(500);  // Adjust for scrolling speed
  }
}

void loop() {
  // If the engine has already started, keep the relay high and stop re-testing
  if (engineStarted) {
    return;  
  }

  // Check if touch sensor is activated to begin the breathalyzer test
  if (digitalRead(touchPin) == HIGH) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Testing Alcohol");

    unsigned long testStartTime = millis();
    float adcValue = 0;
    int sampleCount = 0;

    // Collect and average sensor data for 5 seconds for accuracy
    while (millis() - testStartTime < 5000) {
      adcValue += analogRead(sensor);
      sampleCount++;
      delay(10);
    }

    // Convert raw ADC value to Voltage and then to Blood Alcohol Concentration (BAC)
    float avgAdcValue = adcValue / sampleCount;
    float v = avgAdcValue * (5.0 / 1024.0);
    float mgL = 0.67 * v; // Conversion factor based on project calibration

    // Log data to Serial Monitor
    Serial.print("BAC: ");
    Serial.print(mgL);
    Serial.println(" g/L");

    // Display BAC on the top row of the LCD
    lcd.setCursor(0, 0);
    lcd.print("BAC: ");
    lcd.print(mgL, 4);
    lcd.print(" g/L     ");

    // Threshold check (1.5 g/L limit)
    if (mgL > 1.5) {
      // High Alcohol detected: Lock ignition and show warning
      Serial.println("Engage Relay Protection");
      digitalWrite(led, HIGH);     // Alert LED ON
      digitalWrite(relay, LOW);    // Relay OFF (Engine Disabled)
      scrollMessage("HIGH ALCOHOL ENGINE DISABLED");
    } else {
      // Safe BAC detected: Prompt user to start the vehicle
      lcd.setCursor(0, 1);
      lcd.print("Tap to Start Eng.");
      Serial.println("Normal BAC, Tap to Start Engine");

      digitalWrite(led, LOW);      // Alert LED OFF
      digitalWrite(relay, LOW);    // Keep relay off until second tap

      // Wait for a second touch to confirm ignition
      while (digitalRead(touchPin) == LOW) {
        // Idle until user taps the sensor again
      }

      // Engage Relay to start the engine
      lcd.setCursor(0, 1);
      lcd.print("Engine Started   ");
      digitalWrite(relay, HIGH);   
      engineStarted = true;        
    }
    delay(1000); 
  } else {
    // Default standby screen
    lcd.setCursor(0, 0);
    lcd.print("Touch to Start  ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    digitalWrite(led, LOW);
    digitalWrite(relay, LOW); 
    previousMgL = -1;          
  }
}
