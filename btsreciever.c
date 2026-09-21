#include <WiFi.h>
#include <esp_now.h>

// ==================== MOTOR PINS ====================
// ===== LEFT Driver (BTS7960 #1) =====
#define LEFT_R_PWM 5
#define LEFT_L_PWM 18

// ===== RIGHT Driver (BTS7960 #2) =====
#define RIGHT_R_PWM 12
#define RIGHT_L_PWM 13

#define MOTOR_SPEED 255  // EN pins already HIGH on BTS

// ==================== DATA STRUCT (MUST MATCH TRANSMITTER) ====================
typedef struct struct_message {
  int a;  // FORWARD
  int b;  // BACKWARD
  int c;  // RIGHT
  int d;  // LEFT
} struct_message;

struct_message incomingData;

// ==================== MOTOR CONTROL ====================
void stopMotors() {
  analogWrite(LEFT_R_PWM, 0);
  analogWrite(LEFT_L_PWM, 0);
  analogWrite(RIGHT_R_PWM, 0);
  analogWrite(RIGHT_L_PWM, 0);
}

void moveForward() {
  analogWrite(LEFT_R_PWM, MOTOR_SPEED);
  analogWrite(LEFT_L_PWM, 0);
  analogWrite(RIGHT_R_PWM, MOTOR_SPEED);
  analogWrite(RIGHT_L_PWM, 0);
}

void moveBackward() {
  analogWrite(LEFT_R_PWM, 0);
  analogWrite(LEFT_L_PWM, MOTOR_SPEED);
  analogWrite(RIGHT_R_PWM, 0);
  analogWrite(RIGHT_L_PWM, MOTOR_SPEED);
}

void turnRight() {
  analogWrite(LEFT_R_PWM, MOTOR_SPEED);
  analogWrite(LEFT_L_PWM, 0);
  analogWrite(RIGHT_R_PWM, 0);
  analogWrite(RIGHT_L_PWM, MOTOR_SPEED);
}

void turnLeft() {
  analogWrite(LEFT_R_PWM, 0);
  analogWrite(LEFT_L_PWM, MOTOR_SPEED);
  analogWrite(RIGHT_R_PWM, MOTOR_SPEED);
  analogWrite(RIGHT_L_PWM, 0);
}

// ==================== CALLBACK FUNCTION ====================
void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingBuffer, int len) {
  const uint8_t *mac = info->src_addr;   // sender MAC now comes from the info struct

  memcpy(&incomingData, incomingBuffer, sizeof(incomingData));

  Serial.print("Received from MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.print(mac[i], HEX);
    if (i < 5) Serial.print(":");
  }
  Serial.println();

  Serial.print("Data → A:");
  Serial.print(incomingData.a);
  Serial.print(" B:");
  Serial.print(incomingData.b);
  Serial.print(" C:");
  Serial.print(incomingData.c);
  Serial.print(" D:");
  Serial.println(incomingData.d);

  if (incomingData.a) Serial.println("  → FORWARD DETECTED ✓");
  if (incomingData.b) Serial.println("  → BACKWARD DETECTED ✓");
  if (incomingData.c) Serial.println("  → RIGHT DETECTED ✓");
  if (incomingData.d) Serial.println("  → LEFT DETECTED ✓");
  if (!incomingData.a && !incomingData.b && !incomingData.c && !incomingData.d) {
    Serial.println("  → STOP");
  }
  Serial.println();

  // ---- Drive motors based on received command ----
  if (incomingData.a) {
    moveForward();
  } else if (incomingData.b) {
    moveBackward();
  } else if (incomingData.c) {
    turnRight();
  } else if (incomingData.d) {
    turnLeft();
  } else {
    stopMotors();
  }
}

// ==================== INITIALIZATION ====================
void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n\n========================================");
  Serial.println("  ESP-NOW RECEIVER (TEST MODE)");
  Serial.println("========================================\n");

  // Setup PWM pins (EN pins already HIGH on BTS)
  pinMode(LEFT_R_PWM, OUTPUT);
  pinMode(LEFT_L_PWM, OUTPUT);
  pinMode(RIGHT_R_PWM, OUTPUT);
  pinMode(RIGHT_L_PWM, OUTPUT);
  stopMotors();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  Serial.print("Receiver MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("✗ ESP-NOW Init Failed");
    return;
  }

  esp_now_register_recv_cb(onDataRecv);
  Serial.println("✓ ESP-NOW ready - waiting for data...\n");
}

void loop() {
  delay(1000);
  // Data reception happens in the callback automatically
}