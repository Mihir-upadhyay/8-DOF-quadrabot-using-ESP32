#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <WiFi.h>
#include <esp_now.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

// --- Physical Calibration Constants ---
#define SERVO_MIN  150
#define SERVO_MAX  600

// Channels
#define FL_HIP 0  
#define FL_KNEE 1
#define FR_HIP 2  
#define FR_KNEE 3
#define RL_HIP 4  
#define RL_KNEE 5
#define RR_HIP 6  
#define RR_KNEE 7

// State variables
char currentCommand = 'S'; 
bool isCentered = false;

// --- Explicit Calibration Angles ---
int front_knee_up = 120;
int front_knee_down = 90;
int front_hip_fwd = 130;
int front_hip_back = 50;

int rear_knee_up = 60;    
int rear_knee_down = 90;
int rear_hip_fwd = 50;    
int rear_hip_back = 130;  

// Structure to receive data (MUST MATCH SENDER)
typedef struct struct_message {
  char command;
} struct_message;

struct_message incomingData;

// --- Callback when data is received ---
// --- Callback when data is received ---
// We changed the second parameter name to 'data' so it doesn't conflict
void OnDataRecv(const uint8_t * mac, const uint8_t *data, int len) {
  
  // Copy the raw bytes from 'data' into our 'incomingData' structure
  memcpy(&incomingData, data, sizeof(incomingData));
  
  // Now we can safely read the command!
  currentCommand = incomingData.command;
  
  Serial.print("Received Command: ");
  Serial.println(currentCommand);
}

// --- Master Servo Logic ---
void setServo(int ch, int angle) {
  int finalAngle = angle;
  if (ch == FR_HIP || ch == FR_KNEE || ch == RR_HIP || ch == RR_KNEE) {
    finalAngle = 180 - finalAngle;
  }
  finalAngle = constrain(finalAngle, 0, 180);
  pwm.setPWM(ch, 0, map(finalAngle, 0, 180, SERVO_MIN, SERVO_MAX));
}

void centerAll() {
  Serial.println("Stopping & Centering...");
  setServo(FL_HIP, 90); setServo(FR_HIP, 90);
  setServo(RL_HIP, 90); setServo(RR_HIP, 90);
  delay(150);
  setServo(FL_KNEE, 90); setServo(FR_KNEE, 90);
  setServo(RL_KNEE, 90); setServo(RR_KNEE, 90);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); 
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(50);
  
  centerAll();
  isCentered = true;

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(); // Disconnect from any saved networks

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Register the receive callback
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
  Serial.println("Quadrabot ready and waiting for Joystick signal...");
}

// --- Gaits ---
void lowPowerWalk() {
  int speed = 200; 
  setServo(FL_KNEE, front_knee_up); setServo(RR_KNEE, rear_knee_up); delay(speed);
  setServo(FL_HIP, front_hip_fwd);  setServo(RR_HIP, rear_hip_fwd); delay(speed);
  setServo(FR_HIP, front_hip_back); setServo(RL_HIP, rear_hip_back); delay(speed);
  setServo(FL_KNEE, front_knee_down); setServo(RR_KNEE, rear_knee_down); delay(speed);
  
  setServo(FR_KNEE, front_knee_up); setServo(RL_KNEE, rear_knee_up); delay(speed);
  setServo(FR_HIP, front_hip_fwd);  setServo(RL_HIP, rear_hip_fwd); delay(speed);
  setServo(FL_HIP, front_hip_back); setServo(RR_HIP, rear_hip_back); delay(speed);
  setServo(FR_KNEE, front_knee_down); setServo(RL_KNEE, rear_knee_down); delay(speed);
}

void lowPowerWalkBackward() {
  int speed = 200; 
  setServo(FL_KNEE, front_knee_up); setServo(RR_KNEE, rear_knee_up); delay(speed); 
  setServo(FL_HIP, front_hip_back); setServo(RR_HIP, rear_hip_back); delay(speed); 
  setServo(FR_HIP, front_hip_fwd);  setServo(RL_HIP, rear_hip_fwd);  delay(speed); 
  setServo(FL_KNEE, front_knee_down); setServo(RR_KNEE, rear_knee_down); delay(speed); 
  
  setServo(FR_KNEE, front_knee_up); setServo(RL_KNEE, rear_knee_up); delay(speed); 
  setServo(FR_HIP, front_hip_back); setServo(RL_HIP, rear_hip_back); delay(speed); 
  setServo(FL_HIP, front_hip_fwd);  setServo(RR_HIP, rear_hip_fwd);  delay(speed); 
  setServo(FR_KNEE, front_knee_down); setServo(RL_KNEE, rear_knee_down); delay(speed); 
}

void turnRight() {
  int speed = 200;
  setServo(FL_KNEE, front_knee_up); setServo(RR_KNEE, rear_knee_up); delay(speed);
  setServo(FL_HIP, 130); setServo(RR_HIP, 130); delay(speed); 
  setServo(FR_HIP, 130); setServo(RL_HIP, 130); delay(speed); 
  setServo(FL_KNEE, front_knee_down); setServo(RR_KNEE, rear_knee_down); delay(speed);
  
  setServo(FR_KNEE, front_knee_up); setServo(RL_KNEE, rear_knee_up); delay(speed);
  setServo(FR_HIP, 50); setServo(RL_HIP, 50); delay(speed); 
  setServo(FL_HIP, 50); setServo(RR_HIP, 50); delay(speed); 
  setServo(FR_KNEE, front_knee_down); setServo(RL_KNEE, rear_knee_down); delay(speed);
}

void turnLeft() {
  int speed = 200;
  setServo(FL_KNEE, front_knee_up); setServo(RR_KNEE, rear_knee_up); delay(speed);
  setServo(FL_HIP, 50); setServo(RR_HIP, 50); delay(speed); 
  setServo(FR_HIP, 50); setServo(RL_HIP, 50); delay(speed); 
  setServo(FL_KNEE, front_knee_down); setServo(RR_KNEE, rear_knee_down); delay(speed);
  
  setServo(FR_KNEE, front_knee_up); setServo(RL_KNEE, rear_knee_up); delay(speed);
  setServo(FR_HIP, 130); setServo(RL_HIP, 130); delay(speed); 
  setServo(FL_HIP, 130); setServo(RR_HIP, 130); delay(speed); 
  setServo(FR_KNEE, front_knee_down); setServo(RL_KNEE, rear_knee_down); delay(speed);
}

void loop() {
  // Execute the current command in a continuous loop
  if (currentCommand == 'F') { 
    lowPowerWalk(); 
    isCentered = false; 
  }
  else if (currentCommand == 'B') { 
    lowPowerWalkBackward(); 
    isCentered = false; 
  }
  else if (currentCommand == 'R') { 
    turnRight(); 
    isCentered = false; 
  }
  else if (currentCommand == 'L') { 
    turnLeft(); 
    isCentered = false; 
  }
  else if (currentCommand == 'S') {
    if (!isCentered) {
      centerAll();
      isCentered = true;
    }
  }
}