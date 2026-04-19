#include "dfplayer_service.h"
#include <HardwareSerial.h>

// -----------------------------------------------------------------------------
// DFPlayer Mini UART pins
// ESP32 RX  <- DFPlayer TX
// ESP32 TX  -> DFPlayer RX (use 1K resistor)
// -----------------------------------------------------------------------------
#define DFPLAYER_RX D7
#define DFPLAYER_TX D6

// -----------------------------------------------------------------------------
// Timing control
// AUDIO_DURATION = how long each audio takes to finish (IMPORTANT)
// -----------------------------------------------------------------------------
#define AUDIO_DURATION 8000   // 8 seconds (adjust based on your mp3 length)
#define DF_WARNING_INTERVAL 5000

// Hardware serial port for DFPlayer communication
HardwareSerial dfSerial(1);

// DFPlayer object
DFRobotDFPlayerMini df;

// -----------------------------------------------------------------------------
// Global timing variable
;
// -----------------------------------------------------------------------------
static unsigned long lastDFPlayTime = 0;  // last time alcohol warning triggered
static unsigned long lastPlayTime = 0;    // ❗ FIX: track when audio actually starts


bool isPlaying = false; // whether DFPlayer is currently playing

// -----------------------------------------------------------------------------
// Queue system (store incoming audio requests)
// -----------------------------------------------------------------------------
#define MAX_QUEUE 20

int audioQueue[MAX_QUEUE];
int queueHead = 0;
int queueTail = 0;

// -----------------------------------------------------------------------------
// Initialize DFPlayer Mini
// - Set baud rate
// - Establish serial communication
// - Set default volume
// -----------------------------------------------------------------------------
void dfplayerInit() {
  dfSerial.begin(9600, SERIAL_8N1, DFPLAYER_RX, DFPLAYER_TX);
  delay(1000);

  // Keep trying until DFPlayer responds
  if (!df.begin(dfSerial, true, false)){
    Serial.println("❌ DFPlayer not detected");
    return;
  }

  Serial.println("✅ DFPlayer detected!");
  df.volume(20);  // Set default volume (0–30)
  
  df.playMp3Folder(4);
  delay(3000);
  
}

// -----------------------------------------------------------------------------
// Check if DFPlayer is currently playing (NOT very reliable)
// -----------------------------------------------------------------------------
bool isDFPlayerBusy() {
  return df.readState() == 1;
}

// ================== QUEUE DISPLAY ==================
void printQueue() {
  Serial.print("📥 Queue: ");

  if (queueHead == queueTail) {
    Serial.println("[empty]");
    return;
  }

  int i = queueHead;

  while (i != queueTail) {
    Serial.print(audioQueue[i]);
    Serial.print(" ");
    i = (i + 1) % MAX_QUEUE;
  }

  Serial.println();
}

// -----------------------------------------------------------------------------
// Add a track into the queue
// -----------------------------------------------------------------------------
void enqueue(int track) {

  int next = (queueTail + 1) % MAX_QUEUE;

  // If queue is full → drop the audio
  if (next == queueHead) {
    Serial.println("⚠ Queue full, drop audio");
    return;
  }

  audioQueue[queueTail] = track;
  queueTail = next;

  printQueue();  
}



// -----------------------------------------------------------------------------
// Get next track from queue
// -----------------------------------------------------------------------------
bool dequeue(int &track) {

  if (queueHead == queueTail) return false;

  track = audioQueue[queueHead];
  queueHead = (queueHead + 1) % MAX_QUEUE;
  return true;
}

// -----------------------------------------------------------------------------
// Called when MQTT / YOLO detects warning
// -----------------------------------------------------------------------------
void playWarning(int warning) {

  if (warning < 1 || warning > 20) {
    Serial.println("⚠ Invalid warning number");
    return;
  }

  Serial.print("▶ Playing warning audio: ");
  Serial.println(warning);

  // ❗ Do NOT play directly → push into queue
  enqueue(warning);
}

// -----------------------------------------------------------------------------
// Alcohol warning (repeat periodically)
// -----------------------------------------------------------------------------
void playAlcoholWarning() {

  unsigned long now = millis();

  // Only enqueue if enough time has passed
  if (now - lastDFPlayTime >= AUDIO_DURATION) {
    lastDFPlayTime = now;

    enqueue(1);  // track 1 = alcohol warning
  }
}

// -----------------------------------------------------------------------------
// Core audio engine (ONLY place that plays audio)
// -----------------------------------------------------------------------------
void processAudioQueue() {

  unsigned long now = millis();

  // ❗ If currently playing → wait until finished
  if (isPlaying && (now - lastPlayTime < AUDIO_DURATION)) {
    return;
  }

  int track;

  // If there is something in queue → play it
  if (dequeue(track)) {

    Serial.print("▶ Playing: ");
    Serial.println(track);

    df.playMp3Folder(track);
    lastPlayTime = now;
    isPlaying = true;

  } else {
    isPlaying = false;
  }
}

// -----------------------------------------------------------------------------
// Set DFPlayer volume (0 – 30)
// -----------------------------------------------------------------------------
void setDfVolume(int volume) {
  if (volume < 0) volume = 0;
  if (volume > 30) volume = 30;
  df.volume(volume);
}