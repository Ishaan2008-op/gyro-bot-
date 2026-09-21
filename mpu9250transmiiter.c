//the threshholds ae working perfectly fine 
//remaining to test the code is recieed on s3 or not i will type the s3 MAC add and then first i will see only esp code then i will see the motors tooo 
#include <WiFi.h>
#include <esp_now.h>
#include <MPU9250.h>
#include <Wire.h>

// ==================== MPU9250 SETUP ====================
MPU9250 mpu;

// ==================== THRESHOLDS FROM YOUR DATA ====================
#define FLAT_Z_MIN 0.98
#define FLAT_Z_MAX 1.06
#define FORWARD_Z_MIN 0.25
#define FORWARD_Z_MAX 0.55
#define BACKWARD_Z_MIN 0.15
#define BACKWARD_Z_MAX 0.75
#define AY_FORWARD_THRESHOLD 0.2
#define AY_BACKWARD_THRESHOLD -0.5


struct TiltState {
  int forward;
  int backward;
  int right;
  int left;
};

// ==================== RECEIVER MAC s3 isthe reciever  ====================
uint8_t receiverMAC[] = {0x30, 0x76, 0xf5, 0xF4, 0x26, 0x4C};  // Broadcast - works for any receiver

// ==================== DATA STRUCT (MATCH RECEIVER) ====================
typedef struct struct_message {
  int a;  // FORWARD
  int b;  // BACKWARD
  int c;  // RIGHT
  int d;  // LEFT
} struct_message;

struct_message outgoingData;

// ==================== TIMING ====================
unsigned long lastSendTime = 0;
const unsigned long SEND_INTERVAL = 100;  // 100ms = 10 Hz

// ==================== INITIALIZATION ====================

void initMPU9250() {
  Wire.begin(21, 22);  // SDA=21, SCL=22
  
  if (!mpu.setup(0x68)) {
    Serial.println("✗ MPU9250 NOT FOUND!");
    while(1) {
      delay(100);
    }
  }
  
  Serial.println("✓ MPU9250 found on 0x68");
  
  // Calibrate
  Serial.println("Calibrating... keep sensor still");
  mpu.calibrateAccelGyro();
  Serial.println("✓ MPU9250 ready");
}

void initESPNow() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  Serial.print("Transmitter MAC: ");
  Serial.println(WiFi.macAddress());
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("✗ ESP-NOW Init Failed");
    return;
  }
  
  // Register peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("✗ Failed to add peer");
    return;
  }
  
  Serial.println("✓ ESP-NOW ready");
}

// ==================== TILT DETECTION ====================


TiltState detectTilt(float ax, float ay, float az) {
  TiltState state = {0, 0, 0, 0};
  
  // PRIMARY: Forward/Backward Detection (Z-axis + AY)
  
  if (az >= FLAT_Z_MIN && az <= FLAT_Z_MAX) {
    if (ay > AY_BACKWARD_THRESHOLD && ay < AY_FORWARD_THRESHOLD) {
      return state;  // FLAT - no movement
    }
  }
  
  // Forward tilt
  if (az >= FORWARD_Z_MIN && az <= FORWARD_Z_MAX && ay > AY_FORWARD_THRESHOLD) {
    state.forward = 1;
    return state;
  }
  
  // Backward tilt
  if (ay < AY_BACKWARD_THRESHOLD && az < 0.75) {
    state.backward = 1;
    return state;
  }
  
  // SECONDARY: Left/Right Detection (only if no forward/back)
  
  if (state.forward == 0 && state.backward == 0) {
    if (ax > 0.3) {
      state.left = 1;
    } else if (ax < -0.3) {
      state.right = 1;
    }
  }
  
  return state;
}

// ==================== SETUP & LOOP ====================

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n\n========================================");
  Serial.println("  MPU9250 → BTS7960 TRANSMITTER");
  Serial.println("========================================\n");
  
  initMPU9250();
  initESPNow();
  
  Serial.println("\n✓ Ready - Tilt sensor to control robot\n");
}

void loop() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastSendTime >= SEND_INTERVAL) {
    lastSendTime = currentTime;
    
    // Update MPU9250
    if (mpu.update()) {
      
      // Read accelerometer
      float ax = mpu.getAccX();
      float ay = mpu.getAccY();
      float az = mpu.getAccZ();
      
      // Detect tilt
      TiltState tilt = detectTilt(ax, ay, az);
      
      // Build message (only ONE direction active at a time)
      outgoingData.a = tilt.forward;
      outgoingData.b = tilt.backward;
      outgoingData.c = tilt.right;
      outgoingData.d = tilt.left;
      
      // Send
      esp_now_send(receiverMAC, (uint8_t *)&outgoingData, sizeof(outgoingData));
      
      // Print debug
      Serial.print("AX:");
      Serial.print(ax, 2);
      Serial.print(" AY:");
      Serial.print(ay, 2);
      Serial.print(" AZ:");
      Serial.print(az, 2);
      Serial.print(" | ");
      
      if (tilt.forward) Serial.print("FORWARD ");
      if (tilt.backward) Serial.print("BACKWARD ");
      if (tilt.left) Serial.print("LEFT ");
      if (tilt.right) Serial.print("RIGHT ");
      if (!tilt.forward && !tilt.backward && !tilt.left && !tilt.right) Serial.print("STOP ");
      
      Serial.println();
    }
  }
}
