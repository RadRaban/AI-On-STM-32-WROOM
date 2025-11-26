#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "driver/i2s.h"

// ---------------- USER CONFIG ----------------
#define SD_CS       5
#define BUTTON_PIN  14
#define SAMPLE_RATE 16000
#define BITS_PER_SAMPLE 16
#define CHANNELS    1

// INMP441 connections
#define I2S_WS   33
#define I2S_SD   32
#define I2S_SCK  26

const char* ssid     = "NORA 24";
const char* password = "eloelo520";

const char* apiKey   = "sk_11edb2a04df7e2cd1f6d2124f683004ebfc52ad0b5073088";  // wstaw swój klucz
const char* host     = "api.elevenlabs.io";
const int httpsPort  = 443;

// ---------------- GLOBALS ----------------
File audioFile;
bool recording = false;
uint8_t i2sBuffer[1024];
int16_t sampleBuffer[512];
int bufIndex = 0;
uint32_t samplesWritten = 0;

// ---------------- WAV HELPERS ----------------
void writeWavHeader(File &f, uint32_t sampleRate, uint16_t bitsPerSample, uint16_t channels) {
  byte header[44];
  int byteRate = sampleRate * channels * bitsPerSample / 8;
  int blockAlign = channels * bitsPerSample / 8;

  memcpy(header, "RIFF", 4);
  *(uint32_t*)(header + 4) = 0;
  memcpy(header + 8, "WAVEfmt ", 8);
  *(uint32_t*)(header + 16) = 16;
  *(uint16_t*)(header + 20) = 1;
  *(uint16_t*)(header + 22) = channels;
  *(uint32_t*)(header + 24) = sampleRate;
  *(uint32_t*)(header + 28) = byteRate;
  *(uint16_t*)(header + 32) = blockAlign;
  *(uint16_t*)(header + 34) = bitsPerSample;
  memcpy(header + 36, "data", 4);
  *(uint32_t*)(header + 40) = 0;

  f.write(header, 44);
}

void finalizeWav(File &f, uint32_t samplesWritten) {
  uint32_t dataSize = samplesWritten * sizeof(int16_t);
  uint32_t chunkSize = dataSize + 36;

  f.seek(4);
  f.write((uint8_t*)&chunkSize, 4);
  f.seek(40);
  f.write((uint8_t*)&dataSize, 4);
}

// ---------------- FILE NAMING ----------------
String makeFilename() {
  return String("/recording.wav");
}

// ---------------- I2S CONFIG ----------------
void i2s_init() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 256,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
  i2s_zero_dma_buffer(I2S_NUM_0);
}

// ---------------- STT FUNCTION ----------------
bool fetchTextFromElevenLabs(const String& filename) {
  WiFiClientSecure client;
  client.setInsecure();

  if (!client.connect(host, httpsPort)) {
    Serial.println("❌ Brak połączenia z ElevenLabs");
    return false;
  }

  File audioFile = SD.open(filename.c_str(), FILE_READ);
  if (!audioFile) {
    Serial.println("❌ Nie można otworzyć pliku WAV");
    return false;
  }

  String boundary = "----ESP32Boundary";
  String bodyStart = "--" + boundary + "\r\n" +
                     "Content-Disposition: form-data; name=\"model_id\"\r\n\r\n" +
                     "scribe_v2\r\n" +
                     "--" + boundary + "\r\n" +
                     "Content-Disposition: form-data; name=\"file\"; filename=\"recording.wav\"\r\n" +
                     "Content-Type: audio/wav\r\n\r\n";
  String bodyEnd = "\r\n--" + boundary + "--\r\n";

  size_t contentLength = bodyStart.length() + audioFile.size() + bodyEnd.length();

  client.print(String("POST /v1/speech-to-text HTTP/1.1\r\n") +
               "Host: " + host + "\r\n" +
               "xi-api-key: " + apiKey + "\r\n" +
               "Content-Type: multipart/form-data; boundary=" + boundary + "\r\n" +
               "Content-Length: " + String(contentLength) + "\r\n" +
               "Connection: close\r\n\r\n");

  client.print(bodyStart);

  uint8_t buffer[512];
  while (audioFile.available()) {
    size_t len = audioFile.read(buffer, sizeof(buffer));
    if (len > 0) client.write(buffer, len);
  }
  audioFile.close();

  client.print(bodyEnd);

  String response;
  while (client.connected() || client.available()) {
    if (client.available()) {
      response += client.readString();
    }
  }
  client.stop();

  // Serial.println("📥 Odpowiedź API:");
  // Serial.println(response);

  int idx = response.indexOf("\"text\"");
  if (idx >= 0) {
    int start = response.indexOf(":", idx) + 2;
    int end = response.indexOf("\"", start);
    String transcript = response.substring(start, end);
    Serial.println("✅ Transkrypcja: " + transcript);
  } else {
    Serial.println("⚠️ Nie znaleziono pola 'text' w odpowiedzi");
  }

  return true;
}

// ---------------- RECORDING ----------------
void startRecording() {
  samplesWritten = 0;
  bufIndex = 0;

  String filename = makeFilename();
  audioFile = SD.open(filename.c_str(), FILE_WRITE);
  if (!audioFile) {
    Serial.println("File open failed!");
    return;
  }
  writeWavHeader(audioFile, SAMPLE_RATE, BITS_PER_SAMPLE, CHANNELS);
  recording = true;

  Serial.println("🎙️ Recording started...");
}

void stopRecording() {
  if (!recording) return;

  if (bufIndex > 0) {
    audioFile.write((uint8_t*)sampleBuffer, bufIndex * sizeof(int16_t));
    samplesWritten += bufIndex;
    bufIndex = 0;
  }

  finalizeWav(audioFile, samplesWritten);
  audioFile.close();
  recording = false;

  Serial.println("💾 Recording stopped and saved.");

  // ✅ od razu transkrypcja
  fetchTextFromElevenLabs("/recording.wav");
}

// ---------------- BUTTON ----------------
void handleButton() {
  static bool lastState = HIGH;
  bool currentState = digitalRead(BUTTON_PIN);

  if (lastState == HIGH && currentState == LOW) {
    if (!recording) startRecording();
    else stopRecording();
    delay(200);
  }
  lastState = currentState;
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" CONNECTED");

  if (!SD.begin(SD_CS)) {
    Serial.println("SD init failed!");
    while (1);
  }
  Serial.println("SD OK");

  i2s_init();
}

// ---------------- LOOP ----------------
void loop() {
  handleButton();

  if (recording) {
    size_t bytesRead;
    i2s_read(I2S_NUM_0, (void*)i2sBuffer, sizeof(i2sBuffer), &bytesRead, portMAX_DELAY);

    // Każda próbka z I2S ma 32 bity; konwertujemy na 16-bit
    int samples = bytesRead / 4;
    int32_t* raw = (int32_t*)i2sBuffer;

    for (int i = 0; i < samples; i++) {
      int16_t s = (raw[i] >> 14);  // skalowanie/konwersja 32->16 bit
      sampleBuffer[bufIndex++] = s;

      if (bufIndex >= 512) {
        audioFile.write((uint8_t*)sampleBuffer, sizeof(sampleBuffer));
        samplesWritten += 512;
        bufIndex = 0;
      }
    }
  }
}