#include "Wire.h"
#include "MPU6050.h"

MPU6050 mpu;

int16_t ax, ay, az;
int16_t gx, gy, gz;
int16_t lastAx, lastAy, lastAz;
const int threshold = 500; // Umbral para detectar movimiento

void setup() {
  Serial.begin(115200);
  Wire.begin();
  Serial.println("Iniciando MPU6050...");
  mpu.initialize();

  if (mpu.testConnection()) {
    Serial.println("MPU6050 conectado.");
  } else {
    Serial.println("Error: no se detecta el MPU6050.");
    while (1);
  }

  // Lectura inicial para comparar
  mpu.getAcceleration(&lastAx, &lastAy, &lastAz);
}

void loop() {
  mpu.getAcceleration(&ax, &ay, &az);

  // Comparar con la última lectura
  if (abs(ax - lastAx) > threshold ||
      abs(ay - lastAy) > threshold ||
      abs(az - lastAz) > threshold) {
    Serial.println("Movimiento detectado!");
  }

  // Guardar valores para la próxima comparación
  lastAx = ax;
  lastAy = ay;
  lastAz = az;

  delay(200); // Ajustar frecuencia de lectura
}
