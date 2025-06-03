#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_VS1053.h>
#include <ArduinoJson.h>

// KONFIGURACJA WI-FI
const char* ssid = "NORA 24";
const char* password = "eloelo520";
const char* host = "translate.google.com";
//const char* host = "api.streamelements.com";
const int httpsPort = 443;
const int tokeny = 80;
const char* clarinToken = "Bearer Zc5hXjQNGnRy64T4wF9QQz9HnNG5KBtYzZNnvK_lQOedR_vm";
const char* clarinURL = "https://services.clarin-pl.eu/api/v1/oapi/chat/completions";
// Piny VS1053
#define VS1053_RESET  15
#define VS1053_CS     32
#define VS1053_DCS    33
#define CARDCS         5  // SD card CS
#define VS1053_DREQ    22

bool fetchWithRetry(const String& text, const char* filename, int maxAttempts = 3, int minSize = 2000);

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

  musicPlayer.setVolume(50, 50);
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);


  // if (nagrajAudio("/mp3/test.ogg")) {
  //   Serial.println("Plik nagrany poprawnie.");
  // } else {
  //   Serial.println("Problem z nagraniem pliku.");
  // }
}

void loop() {
  //musicPlayer.begin();
  //musicPlayer.setVolume(10, 10);
  //musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);

  Serial.println("Podaj tekst do przeczytania: ");
  String userText = readSerialInput();
  Serial.print("Wczytano: ");
  Serial.println(userText);
  String odpowiedz = wyslijZapytanie(userText);
  Serial.print("Wczytano: ");
  Serial.println(odpowiedz);

  if(fetchWithRetry(odpowiedz, "/mp3/speach.mp3")){
    musicPlayer.playFullFile("/mp3/0001.mp3");
    while(musicPlayer.playingMusic){
      delay(100);
    }
    musicPlayer.playFullFile("/mp3/speach.mp3");
    while(musicPlayer.playingMusic){
      delay(100);
    }
    musicPlayer.playFullFile("/mp3/speach.mp3");
    while(musicPlayer.playingMusic){
      delay(100);
    }
    musicPlayer.playFullFile("/mp3/0001.mp3");
    while(musicPlayer.playingMusic){
      delay(100);
    }
  }
  delay(500);
}

bool fetchWithRetry(const String& text, const char* filename, int maxAttempts, int minSize) {
  for (int attempt = 1; attempt <= maxAttempts; ++attempt) {
    Serial.printf("Próba %d pobrania pliku...\n", attempt);
    if (!fetchSpeechToFile(text, filename)) {
      Serial.println("Pobieranie się nie powiodło.");
      continue; // spróbuj jeszcze raz
    }

    File file = SD.open(filename);
    if (!file) {
      Serial.println("Nie można otworzyć pliku do weryfikacji.");
      continue;
    }

    size_t fileSize = file.size();
    file.close();

    if (fileSize >= minSize) {
      Serial.printf("Plik pobrany poprawnie (%d bajtów).\n", fileSize);
      return true;
    } else {
      Serial.printf("Plik za mały (%d bajtów), ponawiam...\n", fileSize);
      SD.remove(filename); // usuń uszkodzony plik
      delay(500); // krótka przerwa przed kolejną próbą
    }
  }

  Serial.println("Wszystkie próby pobrania pliku nieudane.");
  return false;
}

// ==== FUNKCJA DO TTS ====
bool fetchSpeechToFile(const String& text, const char* filename) {
  //audio.stop();
  WiFiClientSecure client;
  client.setInsecure(); // NIE używaj w produkcji

  //String url = "/kappa/v2/speech?voice=pl-PL-Wavenet-A&text=" + urlEncode(text);
  String url = "/translate_tts?ie=UTF-8&q=" + urlEncode(text) + "&tl=pl&client=tw-ob";
  Serial.print("Tekst w download:" + text);
  if (!client.connect(host, httpsPort)) {
    Serial.println("Nie można połączyć się z serwerem.");
    return false;
  }

  // Wysłanie zapytania GET
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: Mozilla/5.0\r\n" +
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
String urlEncode(const String& str) {
  String encoded = "";
  for (size_t i = 0; i < str.length(); i++) {
    unsigned char c = str[i];
    if ((c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9')) {
      encoded += (char)c;
    } else if (c == ' ') {
      encoded += '+';
    } else {
      encoded += '%';
      if (c < 16) encoded += '0';
      encoded += String(c, HEX);
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

String wyslijZapytanie(String inputText) {
  if (WiFi.status() != WL_CONNECTED) return "";

  HTTPClient http;
  http.begin(clarinURL);
  http.addHeader("Authorization", clarinToken);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");

  StaticJsonDocument<1024> requestJson;
  requestJson["model"] = "bielik";
  requestJson["max_tokens"] = tokeny;
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