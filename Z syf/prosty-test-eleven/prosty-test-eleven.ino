#include <WiFi.h>
#include <HTTPClient.h>
#include <SD.h>
#include <SPI.h>

#define SDREADER_CS 5

const char* ssid = "NORA 24";
const char* password = "eloelo520";
const char* apiKey = "sk_b83551ad26185d7b41ac2d604356a0a2dcbf542fbeea4903";
const char* voiceID = "lehrjHysCyPSvjt0uSy6"; // przykładowy voice ID

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nPołączono z Wi-Fi");

  if (!SD.begin(SDREADER_CS)) {
    Serial.println("Błąd inicjalizacji SD");
    return;
  }

  sendTextToSpeech("Witaj świecie, to działa!");
}

void loop() {}

void sendTextToSpeech(String text) {
  HTTPClient http;
  String url = "https://api.elevenlabs.io/v1/text-to-speech/" + String(voiceID);
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("xi-api-key", apiKey);

  String payload = "{\"text\":\"" + text + "\",\"model_id\":\"eleven_monolingual_v1\"}";

  int httpCode = http.POST(payload);
  if (httpCode == 200) {
    WiFiClient* stream = http.getStreamPtr();
    File file = SD.open("/voice.mp3", FILE_WRITE);
    if (!file) {
      Serial.println("Nie można otworzyć pliku do zapisu");
      return;
    }

    uint8_t buffer[128];
    while (http.connected()) {
      size_t size = stream->available();
      if (size) {
        int len = stream->readBytes(buffer, sizeof(buffer));
        file.write(buffer, len);
      }
      delay(1);
    }

    file.close();
    Serial.println("Plik zapisany jako /voice.mp3");
  } else {
    Serial.printf("Błąd HTTP: %d\n", httpCode);
    Serial.println(http.getString());
  }

  http.end();
}