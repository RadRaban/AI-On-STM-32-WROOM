#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <VS1053.h>  // https://github.com/baldram/ESP_VS1053_Library
#include <ESP32_VS1053_Stream.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "driver/i2s.h"


#define SPI_CLK_PIN 18
#define SPI_MISO_PIN 19
#define SPI_MOSI_PIN 23

#define VS1053_CS 21
#define VS1053_DCS 17
#define VS1053_DREQ 16
#define SDREADER_CS 5

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
#define TOKENS 150

const char* ssid = "NORA 24";
const char* password = "eloelo520";

const char* host = "api.elevenlabs.io";
const int httpsPort = 443;
const char* apiKey = "sk_11edb2a04df7e2cd1f6d2124f683004ebfc52ad0b5073088";  // Twój klucz API
const char* voiceID = "lehrjHysCyPSvjt0uSy6";                                // lub Twój własny
const char* clarinToken = "Bearer Zc5hXjQNGnRy64T4wF9QQz9HnNG5KBtYzZNnvK_lQOedR_vm";
const char* clarinURL = "https://services.clarin-pl.eu/api/v1/oapi/chat/completions";
//21m00Tcm4TlvDq8ikWAM klasyk
//21m00Tcm4TlvDq8ikWAM
// lehrjHysCyPSvjt0uSy6 niby poolski
ESP32_VS1053_Stream stream;

String inputText = "";
String odpowiedz = "";
bool isPlaying = false;

// ---------------- GLOBALS ----------------
File audioFile;
bool recording = false;
uint8_t i2sBuffer[1024];
int16_t sampleBuffer[512];
int bufIndex = 0;
uint32_t samplesWritten = 0;


bool mountSDcard() {
  if (!SD.begin(SDREADER_CS)) {
    Serial.println("Card mount failed");
    return false;
  }

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return false;
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  return true;
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  while (!Serial)
    delay(10);

  Serial.println("\n\nVS1053 SD Card Playback Example\n");

  WiFi.begin(ssid, password);
  Serial.print("Łączenie z WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Połączono z WiFi!");

  // Start SPI bus
  // SPI.setHwCs(true);
  // SPI.begin(SPI_CLK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);

  // Mount SD card
  if (!mountSDcard()) {
    Serial.println("SD init failed!");
    while (1) delay(100);
  }
  Serial.println("SD OK");

  Serial.println("SD card mounted - starting decoder...");

  i2s_init();

  // Initialize the VS1053 decoder
  if (!stream.startDecoder(VS1053_CS, VS1053_DCS, VS1053_DREQ) || !stream.isChipConnected()) {
    Serial.println("Decoder not running - system halted");
    while (1) delay(100);
  }

  // Start playback from an SD file
    stream.connecttofile(SD, "/mp3/speach3_clean.mp3");

    File f = SD.open("/mp3/speach3_clean.mp3");
if (f) {
  Serial.println("✅ Plik istnieje, rozmiar: " + String(f.size()));
  f.close();
} else {
  Serial.println("❌ Plik nie istnieje!");
}
    if (!stream.isRunning()) {
        Serial.println("No file running - system halted");
        //while (1) delay(100);
    }

    Serial.print("Codec: ");
    Serial.println(stream.currentCodec());
}

void loop() {
  stream.loop();
  //while (Serial.available()) {
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
  //}
  if (!stream.isRunning() && isPlaying) {
    Serial.println("✅ Gotowe. Wpisz kolejny tekst:");
    isPlaying = false;
  }

  delay(5);
}



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

String cleanText(String input) {
  String output = "";

  for (size_t i = 0; i < input.length(); i++) {
    char c = input.charAt(i);

    // Pomijamy entery
    if (c == '\n' || c == '\r') continue;

    // Pomijamy cudzysłowy
    if (c == '"' || c == '\'') continue;

    // Pomijamy emotki (UTF-8 4‑bajtowe)
    if ((uint8_t)c >= 0xF0) {
      i += 3; 
      continue;
    }

    output += c;
  }

  return output;
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
  String transcript ="";
  int idx = response.indexOf("\"text\"");
  if (idx >= 0) {
    int start = response.indexOf(":", idx) + 2;
    int end = response.indexOf("\"", start);
    transcript = response.substring(start, end);
    Serial.println("✅ Transkrypcja: " + transcript);
  } else {
    Serial.println("⚠️ Nie znaleziono pola 'text' w odpowiedzi");
  }
  Serial.println("📥 Wpisales tekst: " + transcript);
        String odpowiedz = wyslijZapytanie(transcript);
        String czystaOdpowiedz = cleanText(odpowiedz);
        Serial.println("📥 Clarin odpowiedzial: " + czystaOdpowiedz);
        isPlaying = true;

        // Pobierz i odtwórz
        if (fetchSpeechFromElevenLabs(czystaOdpowiedz, "/mp3/speach3.mp3")) {
          cleanMP3File("/mp3/speach3.mp3", "/mp3/speach3_clean.mp3", 5);
          stream.connecttofile(SD, "/mp3/speach3_clean.mp3");
        }

        inputText = "";

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


String wyslijZapytanie(String inputText) {
  if (WiFi.status() != WL_CONNECTED) return "";

  HTTPClient http;
  http.begin(clarinURL);
  http.addHeader("Authorization", clarinToken);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");

  StaticJsonDocument<1024> requestJson;
  requestJson["model"] = "bielik";
  requestJson["max_tokens"] = TOKENS;
  JsonArray messages = requestJson.createNestedArray("messages");
  JsonObject msg = messages.createNestedObject();
  msg["role"] = "user";
  msg["content"] = inputText;

  String requestBody;
  serializeJson(requestJson, requestBody);

  int responseCode = http.POST(requestBody);

  if (responseCode > 0) {
    String response = http.getString();
    DynamicJsonDocument doc(6144);
    DeserializationError error = deserializeJson(doc, response);
    if (!error) {
      return doc["choices"][0]["message"]["content"].as<String>();
    } else {
      Serial.println("❌ Błąd parsowania JSON-a!");
    }
  } else {
    Serial.print("❌ Błąd HTTP: ");
    Serial.println(responseCode);
  }

  http.end();
  return "";
}

bool fetchSpeechFromElevenLabs(const String& text, const String& filename) {
  WiFiClientSecure client;
  client.setInsecure();

  if (SD.exists(filename.c_str())) {
    SD.remove(filename.c_str());
    Serial.println("🧹 Usunięto stary plik: " + filename);
  }

  String url = "/v1/text-to-speech/" + String(voiceID) + "/stream?output_format=mp3_44100_128";
  String payload = "{\"text\":\"" + text + "\",\"model_id\":\"eleven_multilingual_v2\"}";

  Serial.println("🔽 Pobieram MP3 z ElevenLabs dla: " + text);

  if (!client.connect(host, httpsPort)) {
    Serial.println("❌ Brak połączenia z ElevenLabs");
    return false;
  }

  client.print(String("POST ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "xi-api-key: " + apiKey + "\r\n" + "Content-Type: application/json\r\n" + "Content-Length: " + payload.length() + "\r\n" + "Connection: close\r\n\r\n" + payload);

  // Pomijanie nagłówków
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break;
  }

  File file = SD.open(filename.c_str(), FILE_WRITE);
  if (!file) {
    Serial.println("❌ Nie można zapisać pliku na SD");
    return false;
  }

  uint8_t buffer[512];
  unsigned long startTime = millis();
  const unsigned long timeout = 10000;

  while (client.connected() || client.available()) {
    if (client.available()) {
      size_t len = client.read(buffer, sizeof(buffer));
      if (len > 0) {
        file.write(buffer, len);
        startTime = millis();  // reset timeout
      }
    } else {
      if (millis() - startTime > timeout) {
        Serial.println("⚠️ Timeout podczas pobierania MP3");
        break;
      }
      delay(10);
    }
  }

  file.close();
  client.stop();

  File check = SD.open(filename.c_str());
  if (check) {
    Serial.println("📦 Rozmiar zapisany: " + String(check.size()));
    check.close();
    // if (check.size() < 1024) {
    //   Serial.println("⚠️ Plik zbyt mały — prawdopodobnie niekompletny");
    //   //return false;
    // }
  }

  Serial.println("✅ MP3 zapisany jako: " + filename);
  return true;
}


void audio_eof_stream(const char* info) {
  Serial.printf("End of file: %s\n", info);
}

bool cleanMP3File(const String& sourcePath, const String& destPath, size_t skipBytes) {
  File source = SD.open(sourcePath, FILE_READ);
  if (!source) {
    Serial.println("❌ Nie można otworzyć pliku źródłowego");
    return false;
  }

  File dest = SD.open(destPath, FILE_WRITE);
  if (!dest) {
    Serial.println("❌ Nie można utworzyć pliku docelowego");
    source.close();
    return false;
  }

  // Pomijanie bajtów
  source.seek(skipBytes);

  uint8_t buffer[512];
  while (source.available()) {
    size_t len = source.read(buffer, sizeof(buffer));
    dest.write(buffer, len);
  }

  source.close();
  dest.close();
  Serial.println("✅ Plik oczyszczony zapisany jako: " + String(destPath));
  return true;
}