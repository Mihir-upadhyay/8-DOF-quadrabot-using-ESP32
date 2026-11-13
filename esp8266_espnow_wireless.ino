#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads; 

// --- YOUR EXACT ESP32 MAC ADDRESS ---
uint8_t broadcastAddress[] = {0xA8, 0x42, 0xE3, 0xBA, 0xC1, 0xA4};

typedef struct struct_message {
  char command; 
} struct_message;

struct_message myData;
char lastCommand = 'S'; 

// NodeMCU D3 is GPIO 0
#define JOY_BTN 0  

void setup() {
  Serial.begin(115200);
  pinMode(JOY_BTN, INPUT_PULLUP);

  Wire.begin(4, 5); 
  if (!ads.begin()) {
    Serial.println("Failed to initialize ADS1115.");
    while (1); 
  }
  Serial.println("ADS1115 Ready.");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(); 

  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
}

void loop() {
  int xValue = ads.readADC_SingleEnded(0); 
  int yValue = ads.readADC_SingleEnded(1); 
  bool btnPressed = (digitalRead(JOY_BTN) == LOW);
  
  char currentCmd = 'S'; // Default to Stop/Center

  // --- RAW VALUE DEBUGGER ---
  // Uncomment the next line if you want to see the exact numbers your joystick outputs!
  // Serial.print("Raw X: "); Serial.print(xValue); Serial.print(" | Raw Y: "); Serial.println(yValue);

  // --- WIDENED DEADZONES ---
  // Max value is usually ~17500. Center is usually ~8500.
  // We pushed the thresholds to the absolute edges so it only triggers on a hard push!
  int highThreshold = 15000; 
  int lowThreshold = 2500;   

  if (btnPressed) {
    currentCmd = 'S'; // Button overrides to Stop
  } 
  else if (yValue > highThreshold) {
    currentCmd = 'F'; 
  } 
  else if (yValue < lowThreshold) {
    currentCmd = 'B'; 
  }
  else if (xValue > highThreshold) {
    currentCmd = 'R'; 
  } 
  else if (xValue < lowThreshold) {
    currentCmd = 'L'; 
  }
  // If no thresholds are crossed (it's in the middle deadzone), currentCmd remains 'S'.

  // Only broadcast if the state actually changes
  if (currentCmd != lastCommand) {
    myData.command = currentCmd;
    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    
    Serial.print("Sending Command: ");
    Serial.println(currentCmd);
    lastCommand = currentCmd;
  }

  delay(50); 
}