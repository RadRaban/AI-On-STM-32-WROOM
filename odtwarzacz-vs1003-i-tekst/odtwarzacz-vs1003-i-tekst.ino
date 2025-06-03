#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_VS1053.h>


// KONFIGURACJA WI-FI
const char* ssid = "NORA 24";
const char* password = "eloelo520";
const char* host = "api.streamelements.com";
const int httpsPort = 443;
// Piny VS1053
#define VS1053_RESET  15
#define VS1053_CS     32
#define VS1053_DCS    33
#define CARDCS         5  // SD card CS
#define VS1053_DREQ    22

Adafruit_VS1053_FilePlayer musicPlayer =
  Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);

void setup() {
  Serial.begin(115200);
  delay(500);
if (!musicPlayer.begin()) {
    Serial.println("Nie znaleziono VS1053");
    while (1);
  }
  // Połączenie z Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Łączenie z WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nPołączono z WiFi!");

  if (!musicPlayer.begin()) {
    Serial.println("Nie znaleziono VS1053");
    while (1);
  }

  if (!SD.begin(CARDCS)) {
    Serial.println("Błąd SD!");
    while (1);
  }

  musicPlayer.setVolume(10, 10);
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);


  // if (nagrajAudio("/mp3/test.ogg")) {
  //   Serial.println("Plik nagrany poprawnie.");
  // } else {
  //   Serial.println("Problem z nagraniem pliku.");
  // }
}

void loop() {
  Serial.println("Podaj tekst do przeczytania: ");
  String userText = readSerialInput();
  Serial.print("Wczytano: ");
  Serial.println(userText);

  if(fetchSpeechToFile(userText, "/mp3/speach.mp3")){
    musicPlayer.playFullFile("/mp3/speach.mp3");
    while(musicPlayer.playingMusic){
      delay(100);
    }
  }
}

// ==== FUNKCJA DO TTS ====
bool fetchSpeechToFile(const String& text, const char* filename) {
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
  File file = SD.open(filename, FILE_WRITE);
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

  Serial.println("Pobieranie zakończone");
  return true;
}

String readSerialInput() {
  String input = "";
  while (true) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') {
        if (input.length() > 0) {
          return input;  // zwracamy wpisany tekst
        }
      } else {
        input += c;
      }
    }
  }
}

// ===== FUNKCJA URL-ENCODE =====
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

bool nagrajAudio(const String& filename) {

  // Rozpocznij nagrywanie (void, nie zwraca wartości)
  musicPlayer.startRecordOgg(filename.c_str());

  Serial.println("Nagrywanie trwa 10 sekund...");
  unsigned long start = millis();
  while (millis() - start < 10000) {
    delay(100);
  }

  // Zakończ nagrywanie (void)
  musicPlayer.stopRecordOgg();

  Serial.println("Nagrywanie zakończone i zapisane.");

  return true;
}
