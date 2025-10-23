#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <Adafruit_VS1053.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "roz.h"
#include "roz_1.h"
#include "roz_2.h"
#include "roz_3.h"
#include "BluetoothSerial.h"
//#include <FlashStorage.h>

// KONFIGURACJA WI-FI
const char* ssid = "NORA 24";
const char* password = "eloelo520";
const char* host = "api.streamelements.com";
const int httpsPort = 443;
const int tokeny = 100;
const char* clarinToken = "Bearer Zc5hXjQNGnRy64T4wF9QQz9HnNG5KBtYzZNnvK_lQOedR_vm";
const char* clarinURL = "https://services.clarin-pl.eu/api/v1/oapi/chat/completions";

// Piny VS1053
#define VS1053_RESET  15
#define VS1053_CS     12
#define VS1053_DCS    21
#define CARDCS         5  // SD card CS
#define VS1053_DREQ    22

#define TFT_CS     2
#define TFT_RST    4
#define TFT_DC     25
#define TFT_SCK    18   // CLK
#define TFT_MOSI   23   // MOSI

#define FLASH_CS 5   // GPIO dla CS
#define FLASH_SCK 18
#define FLASH_MOSI 23
#define FLASH_MISO 19

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

Adafruit_VS1053_FilePlayer musicPlayer =
  Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);

BluetoothSerial SerialBT;
// Sprawdzenie PSRAM
void setup() {
  Serial.begin(115200);

  SPI.begin(FLASH_SCK, FLASH_MISO, FLASH_MOSI, FLASH_CS);  

  SerialBT.begin("ESP32-Terminal"); // Nazwa Bluetooth ESP32
Serial.println("Bluetooth aktywowany! Czekam na połączenie...");

  // Inicjalizacja wyświetlacza
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(2);
  tft.fillScreen(ST77XX_BLACK);
  delay(100);

  // Wyświetlenie obrazu
  tft.drawRGBBitmap(0, 0, roz, 128, 160);


  if (ESP.getPsramSize() > 0) {
    Serial.println("PSRAM dostępny!");
  } else {
    Serial.println("Brak PSRAM, zmiana pamięci RAM może nie być możliwa.");
  }

  // Połączenie z Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Łączenie z WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nPołączono z WiFi!");
}

// Pobieranie pliku do pamięci RAM zamiast SD
bool fetchSpeechToRAM(const String& text, uint8_t** buffer, size_t* dataSize) {
  WiFiClientSecure client;
  client.setInsecure();

  String url = "/kappa/v2/speech?voice=pl-PL-Wavenet-A&text=" + urlEncode(text);
  if (!client.connect(host, httpsPort)) {
    Serial.println("Nie można połączyć się z serwerem.");
    return false;
  }

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: Mozilla/5.0\r\n" +
               "Connection: close\r\n\r\n");

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break;
  }

  *dataSize = client.available();
  *buffer = (uint8_t*)ps_malloc(*dataSize);
  if (!(*buffer)) {
    Serial.println("Nie udało się przydzielić pamięci!");
    return false;
  }

  size_t index = 0;
  while (client.available()) {
    (*buffer)[index++] = client.read();
  }

  client.stop();
  Serial.printf("Dane pobrane do RAM (%d bajtów)\n", *dataSize);
  return true;
}

// Odtwarzanie pliku z pamięci RAM
void playFromRAM(uint8_t* buffer, size_t dataSize) {
  musicPlayer.playData(buffer, dataSize);
}

void loop() {
  String userText = "";
  
  if (SerialBT.available()) { // Sprawdzenie czy są dostępne dane
    while (SerialBT.available()) {
      char c = SerialBT.read();
      if (c == '\n') break; // Koniec wpisywania, wysyłamy zapytanie
      userText += c;
    }

    // Sprawdzenie, czy tekst nie jest pusty

      Serial.print("Wczytano: ");
      Serial.println(userText);

      String odpowiedz = wyslijZapytanie(userText);
      tft.drawRGBBitmap(0, 0, roz, 128, 160);
      delay(300);
      tft.drawRGBBitmap(0, 0, roz_1, 128, 160);
      delay(300);
      tft.drawRGBBitmap(0, 0, roz_2, 128, 160);
      delay(300);
      tft.drawRGBBitmap(0, 0, roz_3, 128, 160);
      
      Serial.print("Odebrano: ");
      Serial.println(odpowiedz);
      SerialBT.print("Odebrano: ");
      SerialBT.println(odpowiedz);
      tft.drawRGBBitmap(0, 0, roz_3, 128, 160);
      delay(300);
      tft.drawRGBBitmap(0, 0, roz_2, 128, 160);
      delay(300);
      tft.drawRGBBitmap(0, 0, roz_1, 128, 160);
      delay(300);
      tft.drawRGBBitmap(0, 0, roz, 128, 160);
  }

  
  // uint8_t* audioBuffer;
  // size_t dataSize;
  // if (fetchSpeechToRAM(userText, &audioBuffer, &dataSize)) {
  //   playFromRAM(audioBuffer, dataSize);
  //   free(audioBuffer);
  // }

  delay(500);
  
}


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

String readSerialBTInput() {
  String input = "";
  while (true) {
    if (SerialBT.available()) {
      char c = SerialBT.read();
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

String wyslijZapytanie(String inputText) {
  if (WiFi.status() != WL_CONNECTED) return "";

  HTTPClient http;
  http.begin(clarinURL);
  http.addHeader("Authorization", clarinToken);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
  http.setTimeout(10000); // 10 sekund na odpowiedź
  http.setReuse(false);
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
