#include <Wire.h>
#include <MPU6050.h>
#include <HTTPClient.h>
#include <WiFi.h>

MPU6050 mpu;

const char *ssid = "SG24";
const char *password = "M4tr1z_R0j4*";
const char *endpoint = "https://timbre-telegram-worker.tempusorione.workers.dev";
const char *message = "Despiertate! Movimiento detectado.";

bool needSentMessage = false;

// -- Configuración --
const unsigned long SAMPLE_INTERVAL = 1000;
const unsigned long SLEEP_CONFIRM_TIME = 10 * 1 * 1000; // 10 minutos
const unsigned long WAKE_CONFIRM_TIME = 15 * 1000;      // 30 segundos
const float ANGLE_THRESHOLD = 10.0;                     // grados de cambio

// -- Estado --
bool isAwake = true;
unsigned long lastSampleTime = 0;
unsigned long sleepTimer = 0;
unsigned long wakeTimer = 0;
float refPitch = 0.0;
float refRoll = 0.0;

void setup()
{
  Serial.begin(115200);
  Wire.begin();
  mpu.initialize();

  if (!mpu.testConnection())
  {
    Serial.println("Error: No se pudo conectar al MPU6050.");
    while (1)
      ;
  }

  Serial.println("MPU6050 conectado.");
  delay(1000);
  updateReferenceAngles(); // Inicializa ángulos de referencia
}

void loop()
{
  unsigned long now = millis();
  if (now - lastSampleTime >= SAMPLE_INTERVAL)
  {
    lastSampleTime = now;
    analizeState();
    updateReferenceAngles();
  }
}

void analizeState()
{
  float pitch, roll;
  readAngles(pitch, roll);

  if (isAwake)
  {
    if (isStable(pitch, roll))
    {
      sleepTimer += SAMPLE_INTERVAL;
      if (sleepTimer >= SLEEP_CONFIRM_TIME)
      {
        isAwake = false;
        needSentMessage = true;
        wakeTimer = 0;
        refPitch = pitch;
        refRoll = roll;
        Serial.println("🛌 Estado: Dormido");
      }
    }
    else
    {
      sleepTimer = 0;
      refPitch = pitch;
      refRoll = roll;
    }
  }
  else
  {
    if (isMoved(pitch, roll))
    {
      wakeTimer += SAMPLE_INTERVAL;
      //Serial.println("Estado: se movio durante -> " + String(wakeTimer / 1000) + "s");

      if (wakeTimer >= WAKE_CONFIRM_TIME && needSentMessage)
      {
        isAwake = true;
        sleepTimer = 0;
        refPitch = pitch;
        refRoll = roll;
        Serial.println("🌞 Estado: Despierto");

        sendWakeMessage();
      }
    }
    else
    {
      //Serial.println("Estado: se quedo quieto luego de -> " + String(wakeTimer / 1000) + "s");
      if (wakeTimer > 0)
      {
        wakeTimer -= SAMPLE_INTERVAL / 3;
      }
      else
      {
        wakeTimer = 0;
      }
    }
  }
}

bool isStable(float pitch, float roll)
{
  return abs(pitch - refPitch) < ANGLE_THRESHOLD && abs(roll - refRoll) < ANGLE_THRESHOLD;
}

bool isMoved(float pitch, float roll)
{
  return abs(pitch - refPitch) > ANGLE_THRESHOLD || abs(roll - refRoll) > ANGLE_THRESHOLD;
}

void readAngles(float &pitch, float &roll)
{
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  float ax_g = ax / 16384.0;
  float ay_g = ay / 16384.0;
  float az_g = az / 16384.0;

  roll = atan2(ay_g, az_g) * RAD_TO_DEG;
  pitch = atan2(-ax_g, sqrt(ay_g * ay_g + az_g * az_g)) * RAD_TO_DEG;
}

void updateReferenceAngles()
{
  float pitch, roll;
  readAngles(pitch, roll);
  refPitch = pitch;
  refRoll = roll;
}

bool connectWiFi(const char *ssid, const char *password, unsigned long timeoutMs = 15000)
{
  Serial.println("🌐 Activando WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < timeoutMs)
  {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\n✅ WiFi conectado");
    return true;
  }
  else
  {
    Serial.println("\n❌ Error conectando al WiFi");
    return false;
  }
}

bool sendHttpMessage(const String &endpoint, const String &payload)
{
  HTTPClient http;
  http.begin(endpoint);
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(payload);
  Serial.print("📬 Código de respuesta: ");
  Serial.println(httpResponseCode);

  if (httpResponseCode > 0)
  {
    String response = http.getString();
    Serial.println("🧾 Respuesta: " + response);

    // Aceptamos cualquier 20x como válido
    if (httpResponseCode >= 200 && httpResponseCode < 300)
    {
      http.end();
      needSentMessage = false;
      return true;
    }
  }

  http.end();
  return false;
}

void sendWakeMessage()
{
  if (!connectWiFi(ssid, password))
  {
    return; // si no conecta en el tiempo dado, salir
  }

  String payload = "{\"chat_id\":\"741537983\",\"mensaje\":\"" + String(message) + "\"}";

  // Reintentos hasta éxito
  while (!sendHttpMessage(endpoint, payload))
  {
    Serial.println("⚠️ Fallo en el envío, reintentando en 2s...");
    delay(2000);
  }

  Serial.println("✅ Mensaje enviado con éxito");

  // si después querés apagar el WiFi:
  Serial.println("🔌 Apagando WiFi...");
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}
