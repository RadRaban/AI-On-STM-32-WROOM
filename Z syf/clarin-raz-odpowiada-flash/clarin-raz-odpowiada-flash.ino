#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPIMemory.h>
#include <Audio.h>
#include "time.h"

// === KONFIGURACJA ===
const char* ssid = "NORA 24";
const char* password = "eloelo520";

#define SD_CS 5
#define I2S_DOUT 27
#define I2S_BCLK 26
#define I2S_LRC 25

const char* clarinToken = "Bearer Zc5hXjQNGnRy64T4wF9QQz9HnNG5KBtYzZNnvK_lQOedR_vm";
const char* clarinURL = "https://services.clarin-pl.eu/api/v1/oapi/chat/completions";

const char* host = "api.streamelements.com";
const int httpsPort = 443;

// === AUDIO ===
Audio audio;
SPIFlash flash(5); // CS dla W25Q64

WiFiClientSecure client;

// === FUNKCJE POMOCNICZE ===
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

void playFromFlash(uint32_t startAddr, uint32_t size) {
  uint32_t addr = startAddr;

  while (size--) {
    byte b = flash.readByte(addr++);
    musicPlayer.playData(&b, 1);  // odtwarzanie bajt po bajcie
    delay(1);  // zależnie od szybkości — możesz dostosować
  }
}

bool downloadAndPlay(const String& text) {
  client.stop();
  client.setInsecure();
  client.setTimeout(10000);

  String url = "/kappa/v2/speech?voice=pl-PL-Wavenet-A&text=" + urlEncode(text);

  if (!client.connect(host, httpsPort)) {
    Serial.println("❌ Nie można połączyć się z serwerem.");
    return false;
  }

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

  // Czekaj aż serwer zacznie odpowiedź (nagłówki)
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break;
  }

  // Kasuj sektor przed zapisem
  const uint32_t flashAddress = 0x000000;
  flash.eraseSector(flashAddress);
  Serial.println("🧹 Flash sektor wyczyszczony.");

  // Zapis danych z klienta do pamięci Flash
  uint32_t addr = flashAddress;
  while (client.connected() || client.available()) {
    if (client.available()) {
      byte b = client.read();
      flash.writeByte(addr++, b);
    }
  }

  client.stop();  // Upewnij się że połączenie jest zakończone
  Serial.println("✅ MP3 zapisany do pamięci Flash.");

  // Odtwarzanie z pamięci Flash
  playFromFlash(flashAddress, addr - flashAddress);  // rozmiar = ilość zapisanych bajtów
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
  requestJson["max_tokens"] = 10;
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

// === SETUP I LOOP ===
void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Łączenie z Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Połączono z Wi-Fi.");

  if (!flash.begin()) {
    Serial.println("Nie udało się zainicjalizować pamięci Flash.");
  }

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(5);

  Serial.println("\n🗣️ Wpisz wiadomość do Bielika i naciśnij ENTER:");

}

bool czyOdtwarzanieTrwa = false;

void loop() {
  audio.loop();

  // Sprawdź, czy skończyło się odtwarzanie
  if (czyOdtwarzanieTrwa && !audio.isRunning()) {
    czyOdtwarzanieTrwa = false;
    Serial.println("✅ Audio zakończone.");
    Serial.println("🗣️ Wpisz kolejne pytanie:");
  }

  // Jeśli nie trwa odtwarzanie – przyjmuj nowe pytania
  if (!czyOdtwarzanieTrwa && Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.length() > 0) {
      Serial.println("📤 Wysyłam do Bielika: " + input);
      String odpowiedz = wyslijZapytanie(input);

      if (odpowiedz != "") {
        czyOdtwarzanieTrwa = true;
        if (!downloadAndPlay(odpowiedz)) {
          czyOdtwarzanieTrwa = false;
          Serial.println("🗣️ Wpisz kolejne pytanie:");
        }
      } else {
        Serial.println("🗣️ Wpisz kolejne pytanie:");
      }
    }
  }
}

