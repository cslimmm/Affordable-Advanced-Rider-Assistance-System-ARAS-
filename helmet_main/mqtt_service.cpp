#include "mqtt_service.h"
#include "dfplayer_service.h"


// -----------------------------------------------------------------------------
// Create a WiFi client object (handles TCP/IP over WiFi)
// This is the network layer used by the MQTT client
// -----------------------------------------------------------------------------
WiFiClient espClient;

// Create an MQTT client and attach the WiFi client to it
// MQTT will use espClient to communicate with the broker
PubSubClient client(espClient);

// -----------------------------------------------------------------------------
// MQTT client ID (must be UNIQUE for each device)
// If two devices use the same ID, one will be disconnected
// -----------------------------------------------------------------------------
static const char* mqttClientId = "esp32c3-test-001";

// -----------------------------------------------------------------------------
// MQTT message callback
// This function is automatically called when a subscribed message is received
//
// topic   : MQTT topic name
// payload : message data (byte array, NOT null-terminated string)
// length  : length of payload
// -----------------------------------------------------------------------------
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Convert payload to integer
  char msg[10];
  memcpy(msg, payload, length);
  msg[length] = '\0';

  int value = atoi(msg); //atoi() --> ASCII-to-integer

  // Volume control
  if (strcmp(topic, TOPIC_VOLUME_CONTROL) == 0) { //strcmp() compares two strings, returns 0 if there are equal

    value = constrain(value, 0, 30);

    setDfVolume(value);

    Serial.print("🔊 Volume set to: ");
    Serial.println(value);
  }

    // Collision / warning detection
  else if (strcmp(topic, TOPIC_FRONT_COLLISION) == 0) {

    Serial.print("Warning detected: ");
    Serial.println(value);

    // play corresponding audio file
    playWarning(value);
  }
}

// -----------------------------------------------------------------------------
// Initialize MQTT settings
// This should be called once in setup()
//
// server : MQTT broker address (IP or domain)
// port   : MQTT broker port (usually 1883)
// -----------------------------------------------------------------------------
void mqttInit(const char* server, int port) {
  // Set MQTT broker address and port
  client.setServer(server, port);

  // Register message callback function
  client.setCallback(mqttCallback);

  // Try to connect immediately
  mqttReconnect();
}

// -----------------------------------------------------------------------------
// Handle MQTT connection and automatic reconnection
// Non-blocking: tries to reconnect every 5 seconds if disconnected
// -----------------------------------------------------------------------------
void mqttReconnect() {
  static unsigned long lastTry = 0;

  // ❗ DO NOT try MQTT if WiFi is down
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  
  if (client.connected()) return;
  if (millis() - lastTry < 5000) return;

  lastTry = millis();
  Serial.print("🔄 MQTT reconnecting...");

  // 🔴 connect() with Last Will
  if (client.connect(
        mqttClientId,
        TOPIC_ESP_STATUS,   // will topic
        0,                    // QoS
        true,                 // retained
        "OFFLINE"             // will message
      )) {

    Serial.println("connected");

    client.subscribe(TOPIC_VOLUME_CONTROL);
    client.subscribe(TOPIC_FRONT_COLLISION);

    // 🔴 Publish ONLINE as retained
    client.publish(
      TOPIC_ESP_STATUS,
      "ONLINE",
      true
    );

  } else {
    Serial.print("failed rc=");
    Serial.println(client.state());
  }
}

// -----------------------------------------------------------------------------
// MQTT loop handler
// Must be called repeatedly inside the main loop()
// Keeps MQTT connection alive and processes incoming messages
// -----------------------------------------------------------------------------

void mqttLoop() {
  // ---------- MQTT ----------
  mqttReconnect();
  client.loop();
}

// -----------------------------------------------------------------------------
// Publish MQTT message
//
// topic : MQTT topic
// msg   : message content (string)
// -----------------------------------------------------------------------------
void mqttPublish(const char* topic, const char* msg) {
  // Only publish if MQTT is connected
  if (client.connected()) {
    client.publish(topic, msg);
  }
}
