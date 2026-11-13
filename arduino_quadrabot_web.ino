#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <WiFi.h>
#include <WebServer.h>

// --- WiFi Credentials ---
const char *ssid = "Quadrabot_WiFi";
const char *password = "12345678"; 

WebServer server(80);
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

// State variables for continuous movement
char currentCommand = 'S'; // 'S' for Stop/Center
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

// --- HTML Web Page (The D-Pad Interface) ---
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
  <title>Quadrabot Remote</title>
  <style>
    body { font-family: Arial; text-align: center; background-color: #121212; color: white; margin-top: 50px; user-select: none; }
    .dpad-container { display: grid; grid-template-columns: 100px 100px 100px; grid-template-rows: 100px 100px 100px; gap: 10px; justify-content: center; margin-top: 40px; }
    .btn { background-color: #1E88E5; color: white; border: none; border-radius: 15px; font-size: 20px; font-weight: bold; cursor: pointer; box-shadow: 0px 6px 0px #0D47A1; }
    .btn:active, .btn.active { background-color: #42A5F5; transform: translateY(6px); box-shadow: 0px 0px 0px #0D47A1; }
    .up { grid-column: 2; grid-row: 1; }
    .down { grid-column: 2; grid-row: 3; }
    .left { grid-column: 1; grid-row: 2; }
    .right { grid-column: 3; grid-row: 2; }
    .stop-btn { grid-column: 2; grid-row: 2; background-color: #E53935; box-shadow: 0px 6px 0px #b71c1c; border-radius: 50%; }
    .stop-btn:active { background-color: #ef5350; box-shadow: 0px 0px 0px #b71c1c; }
  </style>
</head>
<body>
  <h2>Quadrabot Remote</h2>
  <p>Press and hold to move. Release to stop.</p>
  
  <div class="dpad-container">
    <button class="btn up" 
      onmousedown="startCmd(event, 'F')" onmouseup="stopCmd(event)" onmouseleave="stopCmd(event)"
      ontouchstart="startCmd(event, 'F')" ontouchend="stopCmd(event)">FWD</button>
      
    <button class="btn left" 
      onmousedown="startCmd(event, 'L')" onmouseup="stopCmd(event)" onmouseleave="stopCmd(event)"
      ontouchstart="startCmd(event, 'L')" ontouchend="stopCmd(event)">LEFT</button>
      
    <button class="btn stop-btn" onclick="stopCmd(event)">STOP</button>
    
    <button class="btn right" 
      onmousedown="startCmd(event, 'R')" onmouseup="stopCmd(event)" onmouseleave="stopCmd(event)"
      ontouchstart="startCmd(event, 'R')" ontouchend="stopCmd(event)">RIGHT</button>
      
    <button class="btn down" 
      onmousedown="startCmd(event, 'B')" onmouseup="stopCmd(event)" onmouseleave="stopCmd(event)"
      ontouchstart="startCmd(event, 'B')" ontouchend="stopCmd(event)">BACK</button>
  </div>

  <script>
    let activeCmd = 'S';
    function startCmd(e, cmd) {
      e.preventDefault(); 
      if (activeCmd !== cmd) {
        activeCmd = cmd;
        fetch('/action?c=' + cmd); 
      }
    }
    function stopCmd(e) {
      e.preventDefault();
      if (activeCmd !== 'S') {
        activeCmd = 'S';
        fetch('/action?c=S'); 
      }
    }
  </script>
</body>
</html>
)rawliteral";

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
  delay(200);
  setServo(FL_KNEE, 90); setServo(FR_KNEE, 90);
  setServo(RL_KNEE, 90); setServo(RR_KNEE, 90);
}

// --- Web Server Handlers ---
void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleAction() {
  if (server.hasArg("c")) {
    currentCommand = server.arg("c").charAt(0);
    Serial.print("Web Command: "); Serial.println(currentCommand);
  }
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); 
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(50);
  
  centerAll();
  isCentered = true;

  WiFi.softAP(ssid, password);
  Serial.print("Connect to 'Quadrabot_WiFi'. IP: ");
  Serial.println(WiFi.softAPIP()); 

  server.on("/", handleRoot);
  server.on("/action", handleAction);
  server.begin();
}

// --- Low Power Gaits ---
void lowPowerWalk() {
  int speed = 250; 
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
  int speed = 250; 
  setServo(FL_KNEE, front_knee_up); setServo(RR_KNEE, rear_knee_up); delay(speed); 
  setServo(FL_HIP, front_hip_back); setServo(RR_HIP, rear_hip_back); delay(speed); 
  setServo(FR_HIP, front_hip_fwd);  setServo(RL_HIP, rear_hip_fwd);  delay(speed); 
  setServo(FL_KNEE, front_knee_down); setServo(RR_KNEE, rear_knee_down); delay(speed); 
  
  setServo(FR_KNEE, front_knee_up); setServo(RL_KNEE, rear_knee_up); delay(speed); 
  setServo(FR_HIP, front_hip_back); setServo(RL_HIP, rear_hip_back); delay(speed); 
  setServo(FL_HIP, front_hip_fwd);  setServo(RR_HIP, rear_hip_fwd);  delay(speed); 
  setServo(FR_KNEE, front_knee_down); setServo(RL_KNEE, rear_knee_down); delay(speed); 
}

// NEW: Turn Right (Left side FWD, Right side BACK)
void turnRight() {
  int speed = 250;
  // Pair A
  setServo(FL_KNEE, front_knee_up); setServo(RR_KNEE, rear_knee_up); delay(speed);
  setServo(FL_HIP, 130); setServo(RR_HIP, 130); delay(speed); 
  setServo(FR_HIP, 130); setServo(RL_HIP, 130); delay(speed); 
  setServo(FL_KNEE, front_knee_down); setServo(RR_KNEE, rear_knee_down); delay(speed);
  
  // Pair B
  setServo(FR_KNEE, front_knee_up); setServo(RL_KNEE, rear_knee_up); delay(speed);
  setServo(FR_HIP, 50); setServo(RL_HIP, 50); delay(speed); 
  setServo(FL_HIP, 50); setServo(RR_HIP, 50); delay(speed); 
  setServo(FR_KNEE, front_knee_down); setServo(RL_KNEE, rear_knee_down); delay(speed);
}

// NEW: Turn Left (Right side FWD, Left side BACK)
void turnLeft() {
  int speed = 250;
  // Pair A
  setServo(FL_KNEE, front_knee_up); setServo(RR_KNEE, rear_knee_up); delay(speed);
  setServo(FL_HIP, 50); setServo(RR_HIP, 50); delay(speed); 
  setServo(FR_HIP, 50); setServo(RL_HIP, 50); delay(speed); 
  setServo(FL_KNEE, front_knee_down); setServo(RR_KNEE, rear_knee_down); delay(speed);
  
  // Pair B
  setServo(FR_KNEE, front_knee_up); setServo(RL_KNEE, rear_knee_up); delay(speed);
  setServo(FR_HIP, 130); setServo(RL_HIP, 130); delay(speed); 
  setServo(FL_HIP, 130); setServo(RR_HIP, 130); delay(speed); 
  setServo(FR_KNEE, front_knee_down); setServo(RL_KNEE, rear_knee_down); delay(speed);
}

void loop() {
  server.handleClient(); // Keep the web server responsive

  // The continuous state machine
  if (currentCommand == 'F') { lowPowerWalk(); isCentered = false; }
  else if (currentCommand == 'B') { lowPowerWalkBackward(); isCentered = false; }
  else if (currentCommand == 'L') { turnLeft(); isCentered = false; }
  else if (currentCommand == 'R') { turnRight(); isCentered = false; }
  else if (currentCommand == 'S') {
    if (!isCentered) {
      centerAll();
      isCentered = true;
    }
  }
}