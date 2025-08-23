#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

// --- Constants ---
const float gyroQuietThreshold = 0.05;       // Minimum gyro activity to consider "quiet"
const unsigned long quietTimeThreshold = 10 * 1 * 1000; // 10 minutes in milliseconds
const float angleWakeThreshold = 25.0;       // Minimum angle change to consider wake
const unsigned long wakeConfirmDuration = 5 * 1000;     // 30 seconds to confirm wake

// --- State Variables ---
float quietAngle = 0.0;
unsigned long lastMovementTime = 0;
unsigned long wakeStartTime = 0;
bool isSleeping = false;
bool wakeCandidate = false;

// --- Sensor Readings ---
float currentAngle = 0.0;
float gyroMagnitude = 0.0;

// --- For smoothing gyro readings ---
const int gyroBufferSize = 10;
float gyroBuffer[gyroBufferSize] = {0};
int gyroBufferIndex = 0;

// --- Setup ---
void setup() {
  Serial.begin(115200);
  Wire.begin();
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed");
    while (1);
  }
  Serial.println("MPU6050 initialized");

  lastMovementTime = millis(); // Evita detección inmediata de sueño
}

// --- Main Loop ---
void loop() {
  Serial.println("Loop start");
  readSensorData();
  detectSleepState();
  detectWakeTransition();
  Serial.println("Loop end\n");
  delay(1000); // Sampling every 100ms
}

// --- Read accelerometer and gyroscope, calculate angle and movement ---
void readSensorData() {
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // Normalize gyro values (as float)
  float gx_f = gx / 131.0;
  float gy_f = gy / 131.0;
  float gz_f = gz / 131.0;

  // Simple moving average for gyro magnitude
  gyroBuffer[gyroBufferIndex] = sqrt(gx_f*gx_f + gy_f*gy_f + gz_f*gz_f);
  gyroBufferIndex = (gyroBufferIndex + 1) % gyroBufferSize;

  float sum = 0;
  for (int i = 0; i < gyroBufferSize; i++) sum += gyroBuffer[i];
  gyroMagnitude = sum / gyroBufferSize;
  Serial.print("Gyro Magnitude: "); Serial.println(gyroMagnitude);

  // Calculate tilt angle (X-axis orientation)
  currentAngle = atan2(ax, sqrt(ay * ay + az * az)) * 180.0 / PI;
  Serial.print("Current Angle: "); Serial.println(currentAngle);
}

// --- Detect if user has been still long enough to assume sleep ---
void detectSleepState() {
  Serial.println("detectSleepState : Start");
  Serial.print("quietAngle :"); Serial.println(quietAngle);
  if (gyroMagnitude < gyroQuietThreshold) {
  Serial.println("gyroMagnitude < gyroQuietThreshold : true");
    if (millis() - lastMovementTime > quietTimeThreshold && !isSleeping) {
      quietAngle = currentAngle;
      isSleeping = true;
      Serial.println("Sleep detected. Quiet angle stored.");
    }
  } else {
    Serial.println("gyroMagnitude < gyroQuietThreshold : false");
    lastMovementTime = millis();
    if (isSleeping && !wakeCandidate) {
      // Movement detected while sleeping
      wakeCandidate = true;
      wakeStartTime = millis();
    }
  }
  Serial.println("detectSleepState : End");
}

// --- Detect if user has woken up based on movement + angle change ---
void detectWakeTransition() {
  Serial.println("detectWakeTransition : End");
  if (wakeCandidate) {
    Serial.println("wakeCandidate : true");
    float angleDelta = abs(currentAngle - quietAngle);
    bool angleChanged = angleDelta > angleWakeThreshold;
    bool moving = gyroMagnitude > gyroQuietThreshold;

    if (angleChanged && moving) {
      Serial.println("angleChanged && moving : true");
      if (millis() - wakeStartTime > wakeConfirmDuration) {
        isSleeping = false;
        wakeCandidate = false;
        Serial.println("Wake confirmed.");
      }
    } else if (!moving) {
      Serial.println("angleChanged && moving : false");
      // Si está quieto pero cambió ligeramente la postura
      wakeCandidate = false;
      Serial.println("Posture change detected, still sleeping.");
    }
  }
  Serial.println("wakeCandidate : false");
  Serial.println("detectWakeTransition : End");
}