#include <WiFi.h>
#include <HTTPClient.h>
#include <SD.h>
#include <Audio.h>

const char* ssid = "NORA 24";
const char* password = "eloelo520";

// Ustawienia SD
#define SD_CS 5  // Podmień, jeśli masz inny pin CS

Audio audio;

#define I2S_DOUT      27
#define I2S_BCLK      26
#define I2S_LRC       25

const char* host = "api.streamelements.com";
const int httpsPort = 443;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Połączenie z Wi-Fi
  Serial.print("Łączenie z ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nPołączono z Wi-Fi");

  // Inicjalizacja karty SD
  if (!SD.begin(SD_CS)) {
    Serial.println("Błąd: Nie można zainicjalizować karty SD.");
    return;
  }

  if (!SD.exists("/mp3")) {
  Serial.println("Tworzę folder /mp3/");
  SD.mkdir("/mp3");
  }

  Serial.println("Karta SD gotowa.");

  // Połączenie HTTPS
  WiFiClientSecure client;
  client.setInsecure();  // NIE używaj w produkcji

  Serial.print("Łączenie z ");
  Serial.println(host);

  if (!client.connect(host, httpsPort)) {
    Serial.println("Błąd połączenia!");
    return;
  }

  String text = "I nawet kiedy będę sam Nie zmienię się, to nie mój świat "; // <- tekst do przeczytania
  String url = "/kappa/v2/speech?voice=pl-PL-Wavenet-A&text=" + text;

  // Żądanie HTTP GET
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

  // Czekanie na odpowiedź i pomijanie nagłówków HTTP
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break;
  }

  // Zapis pliku MP3 na SD
  File file = SD.open("/mp3/0001.mp3", FILE_WRITE);
  if (!file) {
    Serial.println("Nie można otworzyć pliku do zapisu.");
    return;
  }

  Serial.println("Pobieranie i zapisywanie...");
  int bytes = 0;
  while (client.available()) {
    char c = client.read();
    file.write(c);
    bytes++;
  }

  file.close();
  Serial.print("Zapisano 0001.mp3 na SD. Bajtów: ");
  Serial.println(bytes);
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(5); 

  // Odtwarzaj pierwszy plik
  Serial.println ("Otwieram plik: 0001.mp3");

  audio.connecttoFS(SD, "/mp3/0001.mp3");

}

void loop() {
  audio.loop();
}
