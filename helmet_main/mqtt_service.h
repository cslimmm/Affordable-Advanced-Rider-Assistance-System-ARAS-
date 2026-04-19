#ifndef MQTT_SERVICE_H
#define MQTT_SERVICE_H

#include <WiFi.h>
#include <PubSubClient.h>

// ===== MQTT config =====
extern PubSubClient client;


// ============================================================================
// MQTT TOPIC CONFIGURATION
// This file defines ALL MQTT topics used by the device.
// Change IDs here to switch customer / device / environment
// WITHOUT touching business logic code.
// ============================================================================

// ---------------------------------------------------------------------------
// Product / company namespace
// ---------------------------------------------------------------------------
#define PRODUCT_PREFIX   "mycompany/vehiclesafety"

// ---------------------------------------------------------------------------
// Customer identifier
// ---------------------------------------------------------------------------
#define CUSTOMER_ID      "demo_customer"

// ---------------------------------------------------------------------------
// Device identifier
// ---------------------------------------------------------------------------
#define DEVICE_ID        "vehicle_001"

// ---------------------------------------------------------------------------
// Base topic
// All MQTT topics for this device are derived from this base
// Format:
//   <PRODUCT>/<CUSTOMER>/<DEVICE>/...
// ---------------------------------------------------------------------------
#define BASE_TOPIC PRODUCT_PREFIX "/" CUSTOMER_ID "/" DEVICE_ID

// ============================================================================
// STATUS TOPICS
// Used to publish system and safety states
// ============================================================================

// ESP32 online / offline status (Last Will & Testament)
#define TOPIC_ESP_STATUS        BASE_TOPIC "/status/esp"

// Alcohol detection state: SAFE / ALCOHOL_DETECTED
#define TOPIC_ALCOHOL_STATUS    BASE_TOPIC "/status/alcohol"

// Front collision warning state
#define TOPIC_FRONT_COLLISION   BASE_TOPIC "/status/collision/front"

// Rear collision warning state
#define TOPIC_REAR_COLLISION    BASE_TOPIC "/status/collision/rear"

// Helmet Detect
#define TOPIC_HELMET_DETECT    BASE_TOPIC "/status/helmet"

// Emergency trigger state
#define TOPIC_EMERGENCY         BASE_TOPIC "/status/emergency"

// ============================================================================
// RAW SENSOR DATA TOPICS
// Used for monitoring, debugging, and UI sensor logs
// ============================================================================

// Raw alcohol sensor readings
#define TOPIC_ALCOHOL_RAW       BASE_TOPIC "/raw/alcohol"

// Raw front collision sensor data
#define TOPIC_FRONT_RAW         BASE_TOPIC "/raw/front"

// Raw rear collision sensor data
#define TOPIC_REAR_RAW          BASE_TOPIC "/raw/rear"

// Force sensor sensor data
#define TOPIC_HELMET_RAW          BASE_TOPIC "/raw/helmet"

// ============================================================================
// CONTROL TOPICS
// Used for commands sent FROM UI / server TO device
// ============================================================================

// DFPlayer volume control (payload: 0–30)
#define TOPIC_VOLUME_CONTROL    BASE_TOPIC "/control/volume"


// ===== Function declarations =====
void mqttInit(const char* server, int port);
void mqttLoop();
void mqttReconnect();
void mqttPublish(const char* topic, const char* msg);

#endif
