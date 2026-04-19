#include <WiFi.h>
#include <WiFiManager.h>
#include "wifi_service.h"
#include "dfplayer_service.h"

// -----------------------------------------------------------------------------
// Pin configuration
// -----------------------------------------------------------------------------
#define WIFI_RESET_PIN D3        // BOOT button (GPIO9) – long press to reset WiFi
#define LED_PIN        D2        // Status LED (GPIO2)

// -----------------------------------------------------------------------------
// Timing parameters
// -----------------------------------------------------------------------------
#define LONG_PRESS_MS     2000    // Long press duration to reset WiFi (ms)

// -----------------------------------------------------------------------------
// Internal state variables
// -----------------------------------------------------------------------------
static unsigned long buttonPressStart = 0;
static bool resetTriggered = false;

static unsigned long ledTimer = 0;
static bool ledState = false;

// -----------------------------------------------------------------------------
// WiFi audio state
// -----------------------------------------------------------------------------
static bool lastWiFiState = false;
static unsigned long wifiWaitingTimer = 0;

static unsigned long wifiReconnectTimer = 0;

// -----------------------------------------------------------------------------
// LED blink helper
// Internal state variables
// -----------------------------------------------------------------------------
void ledBlink(unsigned long interval) {
  if (millis() - ledTimer >= interval) {
    ledTimer = millis();
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
  }
}

// -----------------------------------------------------------------------------
// Attempt to connect to WiFi using WiFiManager
// If connection fails, the device will reboot
// -----------------------------------------------------------------------------
void tryConnectWiFi() {
  Serial.println("📡 Trying WiFi...");

  // Clear previous WiFi connection state
  WiFi.disconnect(true); 
  delay(200);

  WiFiManager wm; // Create WiFiManager object
  wm.setConnectTimeout(10);        // Max 10s to connect to AP
  wm.setConfigPortalTimeout(20);    // Exit config portal after 20s
  wm.setDebugOutput(true);

  bool res = wm.autoConnect("SmartHelmet_Setup");

  if (!res) {
    Serial.println("❌ WiFi connect failed → reboot");
    ESP.restart();                 // Force reboot to retry
  }
}

// -----------------------------------------------------------------------------
// Initialize WiFi (call once in setup)
// -----------------------------------------------------------------------------
void wifiInit() {

  pinMode(WIFI_RESET_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN,   HIGH);

  WiFi.mode(WIFI_STA); // Station mode
  Serial.println("🚀 WiFi init");
  
  // Try WiFi connection immediately at boot
  tryConnectWiFi();
}

// -----------------------------------------------------------------------------
// Handle long press button to reset saved WiFi credentials
// -----------------------------------------------------------------------------
void handleWiFiResetButton() {
  if (digitalRead(WIFI_RESET_PIN) == LOW) {

    if (buttonPressStart == 0) {
      buttonPressStart = millis();
    }

    if (!resetTriggered &&
        millis() - buttonPressStart > LONG_PRESS_MS) {

      resetTriggered = true;
      Serial.println("🔁 Reset WiFi settings");

      WiFiManager wm;
      wm.resetSettings();
      delay(500);
      ESP.restart();
    }

  } else {
    buttonPressStart = 0;
    resetTriggered = false;
  }
}

// -----------------------------------------------------------------------------
// Detect WiFi connection state change and trigger audio once
// -----------------------------------------------------------------------------
void handleWiFiState(bool currentState) {

  if (currentState != lastWiFiState) {

    if (currentState) {
      Serial.println("✅ WiFi Connected");
      enqueue(5);
      wifiWaitingTimer = 0;

    } else {
      Serial.println("❌ WiFi Not Connected");
      enqueue(4);
      wifiWaitingTimer = millis();
    }

    lastWiFiState = currentState;
  }
}

// -----------------------------------------------------------------------------
// Control LED based on WiFi status
// -----------------------------------------------------------------------------
void handleWiFiLED(bool currentState) {

  if (currentState) {
    digitalWrite(LED_PIN, LOW); // ON
  } else {
    ledBlink(500);
  }
}

// -----------------------------------------------------------------------------
// Repeat "waiting" audio periodically when WiFi is not connected
// -----------------------------------------------------------------------------
void handleWaitingAudio(bool currentState) {

  if (!currentState && millis() - wifiWaitingTimer > 8000) {
    enqueue(4);
    wifiWaitingTimer = millis();
  }
}

// -----------------------------------------------------------------------------
// Attempt to reconnect WiFi periodically when disconnected
// -----------------------------------------------------------------------------
void handleWiFiReconnect(bool currentState) {

  if (!currentState && millis() - wifiReconnectTimer > 10000) {
    Serial.println("🔄 Attempting WiFi reconnect...");

    WiFi.disconnect();
    WiFi.reconnect();

    wifiReconnectTimer = millis();
  }
}

// -----------------------------------------------------------------------------
// WiFi background handler (non-blocking)
// - Handle WiFi reset button
// - Update LED status
// - Retry WiFi connection periodically
// -----------------------------------------------------------------------------
void wifiLoop() {

  handleWiFiResetButton(); 

  bool currentState = (WiFi.status() == WL_CONNECTED);

  handleWiFiState(currentState);
  handleWiFiLED(currentState);
  handleWaitingAudio(currentState);
  handleWiFiReconnect(currentState);
}
