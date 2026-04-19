#include "force_sensor.h"
#include "mqtt_service.h"

// ------------------ Pin ------------------
const uint8_t FSR_PIN = A1; // FSR sensor analog input

// ------------------ Sampling ------------------
const uint16_t SAMPLE_MS = 20; // Sampling interval (20 ms)


// ------------------ Threshold ------------------
int HELMET_OFFSET = 0;   // Offset above baseline to detect helmet

// ------------------ Threshold ------------------
// Based on real data:
// No helmet = 0~20
// Helmet ON = 1300~4095
const int ON_THRESHOLD  = 300;
const int OFF_THRESHOLD = 80;
// ------------------ Hysteresis ------------------
const int HYSTERESIS = 5;  // prevent flicker

// ------------------ Debounce ------------------
const uint16_t ON_DEBOUNCE_MS  = 300; // Delay to confirm helmet ON
const uint16_t OFF_DEBOUNCE_MS = 300; // Delay to confirm helmet OFF

// ------------------ State ------------------
bool helmetOn = false;  // Current helmet status

// ------------------ Internal ------------------
uint32_t lastSample  = 0;
uint32_t lastPublish = 0;
uint32_t onSince     = 0;
uint32_t offSince    = 0;

// --------------------------------------------------
int readFSR() {
  return analogRead(FSR_PIN);  // Read raw ADC value
}

// --------------------------------------------------
void ForceSensorInit() {

  analogReadResolution(12);  // Set ADC resolution (0–4095)

}

// --------------------------------------------------
void ForceSensorLoop() {

  uint32_t now = millis();

  // Control sampling rate
  if (now - lastSample < SAMPLE_MS) return;
  lastSample = now;

  // ---------- Read + EMA filter ----------
  int raw = readFSR();  // Raw sensor reading
  
  // ==================================================
  // Helmet Detection
  // ==================================================
  if (!helmetOn) {

    // Helmet Put On
    if (raw > ON_THRESHOLD) {

      if (onSince == 0) onSince = now;

      if (now - onSince >= ON_DEBOUNCE_MS) {
        helmetOn = true;
        onSince = 0;

        mqttPublish(TOPIC_HELMET_DETECT, "HELMET DETECTED");
      }

    } else {
      onSince = 0;
    }

  } else {

    // Helmet Removed
    if (raw < OFF_THRESHOLD) {

      if (offSince == 0) offSince = now;

      if (now - offSince >= OFF_DEBOUNCE_MS) {
        helmetOn = false;
        offSince = 0;

        mqttPublish(TOPIC_HELMET_DETECT, "HELMET NOT DETECTED");
      }

    } else {
      offSince = 0;
    }
  }


  // ---------- Publish raw data (every 500ms) ----------
  if (now - lastPublish > 500) {
    lastPublish = now;

    char msg[80];
    snprintf(
      msg,
      sizeof(msg),
      "raw:%d,helmet:%d",
      raw,
      helmetOn ? 1 : 0
    );

  mqttPublish(TOPIC_HELMET_RAW, msg);  // Send debug data

  //Serial.println(msg);
  }
}