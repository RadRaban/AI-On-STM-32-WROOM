#include <Arduino.h>
#include <SD.h>
#include <VS1053.h>  // https://github.com/baldram/ESP_VS1053_Library
#include <ESP32_VS1053_Stream.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>


#define SPI_CLK_PIN 18
#define SPI_MISO_PIN 19
#define SPI_MOSI_PIN 23

#define VS1053_CS 21
#define VS1053_DCS 17
#define VS1053_DREQ 16
#define SDREADER_CS 5

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
    Serial.println("SD card not mounted - system halted");
    while (1) delay(100);
  }

  Serial.println("SD card mounted - starting decoder...");

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
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      inputText.trim();
      if (inputText.length() > 0) {
        Serial.println("📥 Wpisales tekst: " + inputText);
        String odpowiedz = wyslijZapytanie(inputText);
        Serial.println("📥 Clarin odpowiedzial: " + odpowiedz);
        isPlaying = true;

        // Pobierz i odtwórz
        if (fetchSpeechFromElevenLabs(odpowiedz, "/mp3/speach3.mp3")) {
          cleanMP3File("/mp3/speach3.mp3", "/mp3/speach3_clean.mp3", 5);
          stream.connecttofile(SD, "/mp3/speach3_clean.mp3");
        }

        inputText = "";
      }
    } else {
      inputText += c;
    }
  }
  if (!stream.isRunning() && isPlaying) {
    Serial.println("✅ Gotowe. Wpisz kolejny tekst:");
    isPlaying = false;
  }

  delay(5);
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
  requestJson["max_tokens"] = 80;
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