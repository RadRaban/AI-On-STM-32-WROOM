#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <SD.h>
#include <SPI.h>

const char* ssid     = "NORA 24";
const char* password = "eloelo520";

const char* apiKey   = "sk_11edb2a04df7e2cd1f6d2124f683004ebfc52ad0b5073088";
const char* sttUrl   = "https://api.elevenlabs.io/v1/speech-to-text";

#define SD_CS 5

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.print("Łączenie z WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" OK");

  if (!SD.begin(SD_CS)) {
    Serial.println("Błąd SD!");
    while (1);
  }
  Serial.println("SD OK");

  fetchTextFromElevenLabs("/recording.wav");
}

void loop() {}

bool fetchTextFromElevenLabs(const String& filename) {
  WiFiClientSecure client;
  client.setInsecure();

  String url = "/v1/speech-to-text";
  String host = "api.elevenlabs.io";
  const int httpsPort = 443;

  Serial.println("🔼 Wysyłam WAV do ElevenLabs: " + filename);

  if (!client.connect(host.c_str(), httpsPort)) {
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

  // Nagłówki HTTP
  client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "xi-api-key: " + apiKey + "\r\n" +
               "Content-Type: multipart/form-data; boundary=" + boundary + "\r\n" +
               "Content-Length: " + String(contentLength) + "\r\n" +
               "Connection: close\r\n\r\n");

  // Początek body
  client.print(bodyStart);

  // Wysyłanie pliku WAV
  uint8_t buffer[512];
  while (audioFile.available()) {
    size_t len = audioFile.read(buffer, sizeof(buffer));
    if (len > 0) client.write(buffer, len);
  }
  audioFile.close();

  // Koniec body
  client.print(bodyEnd);

  // Odbiór odpowiedzi
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


