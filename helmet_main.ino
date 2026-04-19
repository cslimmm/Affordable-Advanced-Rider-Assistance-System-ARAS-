#include <WiFi.h>

#include "wifi_service.h"
#include "mqtt_service.h"
#include "alcohol_sensor.h"
#include "dfplayer_service.h"
#include "force_sensor.h"

// MQTT
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

void setup() {
  Serial.begin(115200);
 // delay(2000);

  dfplayerInit();
  Serial.println("Waiting Force Sensor");

  ForceSensorInit();
 // delay(500);
  Serial.println("Waiting alcohol");
  alcoholInit();
  //delay(500);


  wifiInit();
  mqttInit(mqtt_server, mqtt_port);


}

static bool alarmPlaying = false;

void loop() {
  wifiLoop();
  mqttLoop();
  ForceSensorLoop();
  alcoholLoop();


  // enqueue alcohol warning (DON'T play directly)
  if (alcoholIsDetected()) {
    playAlcoholWarning();
  }

  // ---------- Audio system ----------
  processAudioQueue();   // ONLY place to play sound
}

