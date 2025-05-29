#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <SD.h>
#include <Audio.h>

const char* ssid = "NORA 24";
const char* password = "eloelo520";

#define SD_CS 5

Audio audio;

#define I2S_DOUT      27
#define I2S_BCLK      26
#define I2S_LRC       25

const char* host = "api.streamelements.com";
const int httpsPort = 443;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.print("Łączenie z ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nPołączono z Wi-Fi");
  
  if (!SD.begin(SD_CS)) {
    Serial.println("Błąd: Nie można zainicjalizować karty SD.");
    while(true) delay(1000);
  }

  if (!SD.exists("/mp3")) {
    SD.mkdir("/mp3");
  }

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(5);

  Serial.println("\nWpisz tekst do przeczytania i naciśnij Enter:");
}

void loop() {
  static String input = "";

  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (input.length() > 0) {
        if (audio.isRunning()) {
          Serial.println("Odtwarzanie trwa, poczekaj...");
        } else {
          Serial.print("Pobieram i odtwarzam tekst: ");
          Serial.println(input);
          if (downloadAndPlay(input)) {
            Serial.println("Odtwarzanie rozpoczęte.");
          } else {
            Serial.println("Błąd pobierania pliku.");
          }
        }
        input = "";
        Serial.println("\nWpisz tekst do przeczytania i naciśnij Enter:");
      }
    } else {
      input += c;
    }
  }

  audio.loop();
}

bool downloadAndPlay(const String& text) {
  //audio.stop();
  WiFiClientSecure client;
  client.setInsecure(); // NIE używaj w produkcji

  String url = "/kappa/v2/speech?voice=pl-PL-Wavenet-A&text=" + urlEncode(text);
  Serial.print("Tekst w download:" + text);
  if (!client.connect(host, httpsPort)) {
    Serial.println("Nie można połączyć się z serwerem.");
    return false;
  }

  // Wysłanie zapytania GET
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

  // Pomijanie nagłówków HTTP
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break;
  }

  // Otwórz plik do zapisu
  File file = SD.open("/mp3/temp.mp3", FILE_WRITE);
  if (!file) {
    Serial.println("Nie można otworzyć pliku do zapisu.");
    return false;
  }

  // Zapis danych mp3
  while (client.available()) {
    file.write(client.read());
  }
  file.close();

  client.stop();
  audio.connecttoFS(SD, "/mp3/temp.mp3");
  return true;
}

// Prosta funkcja URL-encode do bezpiecznego wysyłania tekstu
String urlEncode(const String &str) {
  String encoded = "";
  char c;
  char buf[4];
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum(c)) {
      encoded += c;
    } else if (c == ' ') {
      encoded += '+';
    } else {
      sprintf(buf, "%%%02X", (unsigned char)c);
      encoded += buf;
    }
  }
  return encoded;
}