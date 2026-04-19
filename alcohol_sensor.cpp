#include "alcohol_sensor.h"
#include "mqtt_service.h"

// ===== Hardware config =====
#define MQ3_PIN 2   // MQ-3 analog output pin connected to A0


// ===== Parameters =====
// MQ-3 warm-up time (5-10 minutes)
static const int warmupSeconds = 30; // 300 
// Number of ADC samples per reading (noise smoothing)
static const int sampleCount = 30;

// Absolute alcohol trigger threshold
static const float alcoholThreshold = 1760.0;

// ===================== Runtime State =====================
static bool alcoholWarmupDone = false; // Warm-up status
static bool firstCheckAfterWarmup = false;

static bool isAlcoholDetected = false;
static bool lastAlcoholState = false;

static unsigned long warmupStartTime = 0;


// ===================== Read MQ-3 (averaged) =====================
float readMQ3Average() {
  long total = 0;

  for (int i = 0; i < sampleCount; i++) {
    total += analogRead(MQ3_PIN);
    delay(20); // small delay for ADC stability
  }
  return total / (float)sampleCount;
}

// ====================== Initialization ======================
void alcoholInit() {

  // ----- Start sensor warm-up -----
  warmupStartTime = millis();
  alcoholWarmupDone = false;
  firstCheckAfterWarmup = false;

  analogReadResolution(12);        //  resolution
  analogSetAttenuation(ADC_11db);  //  voltage range

  mqttPublish("TOPIC_ALCOHOL_STATUS", "WARMING_UP");
  Serial.println("MQ3 warming up...");
}

// ===================== Warm-up handler =====================
bool AlcoholSensorWarmUp() {

  if (!alcoholWarmupDone) {

    unsigned long elapsed = millis() - warmupStartTime;

    // Report remaining time every 1 second
    static unsigned long lastReport = 0;
    if (millis() - lastReport > 1000) {

      char warmupMsg[50];
      int remaining =
          max(0, warmupSeconds - (int)(elapsed / 1000));

      snprintf(
        warmupMsg,
        sizeof(warmupMsg),
        "WARMING UP (%ds)",
        remaining
      );

      mqttPublish(TOPIC_ALCOHOL_STATUS, warmupMsg);
      Serial.println(warmupMsg);

      lastReport = millis();
    }

    // warm-up finished
    if (elapsed >= warmupSeconds * 1000UL) {
      alcoholWarmupDone = true;
      firstCheckAfterWarmup = true;   // ⭐ trigger first detection
      Serial.println("✅ Warm-up finished");
    }

    return false; // ⛔ still warming up
  }

  return true; // ✅ warm-up done
}

// ===================== Alcohol Loop =====================
void alcoholLoop() {

  // ⛔ Block detection until warm-up done
  if (!AlcoholSensorWarmUp()) {
    return;  // ⛔ warm-up not done，exit loop
  }

  // ===== read sensor once =====
  float sensorValue = readMQ3Average();
  bool detected = (sensorValue > alcoholThreshold);

  // ===== Build log message =====
  char logMsg[100];
  snprintf(
    logMsg,
    sizeof(logMsg),
    "\nSensor: %.2f ",
    sensorValue
  );

  // ===== MQTT raw log =====
  mqttPublish(TOPIC_ALCOHOL_RAW, logMsg);

  // ===== Serial Monitor =====
  Serial.print("Sensor: "); Serial.println(sensorValue);

  // ===== first detection after warm-up =====
  if (firstCheckAfterWarmup) {
    firstCheckAfterWarmup = false;

    if (detected) {
      mqttPublish(TOPIC_ALCOHOL_STATUS, "ALCOHOL_DETECTED");
      Serial.println("📡 MQTT: ALCOHOL_DETECTED (after warm-up)");
    } else {
      mqttPublish(TOPIC_ALCOHOL_STATUS, "SAFE");
      Serial.println("📡 MQTT: SAFE (after warm-up)");
    }

    lastAlcoholState = detected;
    return;
  }

  // ===== normal detection =====
  isAlcoholDetected = detected;

  // ---------- STATE CHANGE: SAFE -> ALCOHOL ----------
  if (isAlcoholDetected) {

    if (!lastAlcoholState) {
      mqttPublish(TOPIC_ALCOHOL_STATUS, "ALCOHOL DETECTED");
      Serial.println("📡 MQTT: ALCOHOL_DETECTED");
    }

  }else{
      // ---------- STATE CHANGE: ALCOHOL -> SAFE ----------
      if (!isAlcoholDetected && lastAlcoholState) {
        mqttPublish(TOPIC_ALCOHOL_STATUS, "SAFE");
        Serial.println("📡 MQTT: SAFE");
      }
    }
  
  lastAlcoholState = isAlcoholDetected;
}

// ===================== ★ Expose state to main =====================
bool alcoholIsDetected() {
  return isAlcoholDetected;
}
