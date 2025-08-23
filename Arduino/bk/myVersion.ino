#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

// constants
const unsigned long oneSecond = 1000;
const unsigned long oneMinute = 60 * oneSecond;
const unsigned long sleepTimeConfirmDuration = 10 * oneMinute; // 10 minutes in milliseconds
const unsigned long awakeTimeConfirmDuration = 30 * oneSecond;     // 30 seconds to confirm wake

// Variables globales para el filtro
float roll = 0, pitch = 0;   // Ángulos filtrados
unsigned long currentTime = 0;  // Para calcular dt

// -- General variables -- 
int16_t moveX, moveY, moveZ;
int16_t speedX, speedY, speedZ;
int16_t tempRaw;
float moveX_g, moveY_g, moveZ_g;
float speedX_dps, speedY_dps, speedZ_dps;
float rollAcc, pitchAcc;
float tempC;

unsigned long isFallinSleep = 0;
unsigned long isAwaking = 0;

// -- State Variables --
float currentSleepAngleXY = 0.0;
float currentSleepAngleXZ = 0.0;
float currentAwakeAngleXY = 0.0;
float currentAwakeAngleXZ = 0.0;
unsigned long lastSleepTime = 0;
unsigned long lastAwakeTime = 0;
bool isAwake = true;
bool isSleep = false;


void setup() {
  Serial.begin(115200);
  delmoveY(1000);

  Wire.begin();
  
  Serial.println("Iniciando MPU6050...");
  mpu.initialize();

  if (mpu.testConnection()) {
    Serial.println("MPU6050 conectado correctamente.");
  } else {
    Serial.println("Error: No se pudo conectar al MPU6050.");
    while (1);
  }

  lastTime = millis();
}

void loop(){  
  Serial.println("Loop start");
  detectState();
  Serial.println("Loop end\n");
  delay(1000); // Sampling every 100ms
}

void detectState(){

  if(isAwake){
    detectSleep()
  }

  if(isSleep){
    detectAwake()
  }
}

void detectSleep(){

  GetDataFromSensor();

  float currentAngleXY = pitchAcc;
  float currentAngleXZ = rollAcc;

  if(currentAngleXY/currentAwakeAngleXY > -2 && currentAngleXY/currentAwakeAngleXY < 2 && currentAngleXZ/currentAwakeAngleXY > -2 && currentAngleXZ/currentAwakeAngleXY < 2){
    if(isFallinSleep >= sleepTimeConfirmDuration){      
      isAwake = false;
      isSleep = true;
    }else{
      isFallinSleep += currentTime;
    }
  }else{
    currentAwakeAngleXY = currentAngleXY;
    currentAwakeAngleXY = currentAngleXZ;
    isFallinSleep = currentTime;
  }
}

void detectAwake(){
  GetDataFromSensor();

  float currentAngleXY = pitchAcc;
  float currentAngleXZ = rollAcc;

  if(currentSleepAngleXY + 10 > currentAngleXY || currentSleepAngleXY - 10 < currentAngleXY || currentSleepAngleXZ + 10 > currentAngleXZ || currentSleepAngleXZ - 10 < currentAngleXZ){
    if(isAwaking >= awakeTimeConfirmDuration){        
      isAwake = true;
      isSleep = false;
    }else{
      isAwaking += currentTime;
    }else{
      currentAwakeAngleXY = currentAngleXY;
      currentAwakeAngleXY = currentAngleXZ;
      isAwaking = currentTime;
    }
  }
}

void GetDataFromSensor(){
  currentTime = millis();
  mpu.getMotion6(&moveX, &moveY, &moveZ, &speedX, &speedY, &speedZ);
  tempRaw = mpu.getTemperature();

  moveX_g = moveX / 16384.0;
  moveY_g = moveY / 16384.0;
  moveZ_g = moveZ / 16384.0;

  speedX_dps = speedX / 131.0;
  speedY_dps = speedY / 131.0;
  speedZ_dps = speedZ / 131.0;

  rollAcc  = atan2(moveY_g, moveZ_g) * RAD_TO_DEG;
  pitchAcc = atan2(-moveX_g, sqrt(moveY_g * moveY_g + moveZ_g * moveZ_g)) * RAD_TO_DEG;

  tempC = (tempRaw / 340.0) + 36.53;
}
