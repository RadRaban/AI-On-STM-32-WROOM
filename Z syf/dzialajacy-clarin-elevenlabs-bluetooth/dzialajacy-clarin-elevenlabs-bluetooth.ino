#include <Arduino.h>
#include <SD.h>
#include <VS1053.h>
#include <ESP32_VS1053_Stream.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
//#include <BluetoothSerial.h>
#include "roz.h"
#include "roz_1.h"
#include "roz_2.h"
#include "roz_3.h"

#define SPI_CLK_PIN 18
#define SPI_MISO_PIN 19
#define SPI_MOSI_PIN 23

#define VS1053_CS 32
#define VS1053_DCS 33
#define VS1053_DREQ 35
#define SDREADER_CS 5

#define TFT_CS     2
#define TFT_RST    4
#define TFT_DC     16
#define TFT_SCK    18   // CLK
#define TFT_MOSI   23   // MOSI

const char* ssid = "NORA 24";
const char* password = "eloelo520";

const char* host = "api.elevenlabs.io";
const int httpsPort = 443;
const char* apiKey = "sk_7997562cf808ec71caf6ab5ff71dcc52ba790be36f628077";
const char* voiceID = "lehrjHysCyPSvjt0uSy6";
const char* clarinToken = "Bearer Zc5hXjQNGnRy64T4wF9QQz9HnNG5KBtYzZNnvK_lQOedR_vm";
const char* clarinURL = "https://services.clarin-pl.eu/api/v1/oapi/chat/completions";

ESP32_VS1053_Stream stream;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
//BluetoothSerial SerialBT;

String inputText = "";
String btInputText = "";
String odpowiedz = "";
bool isPlaying = false;

void setup() {
    Serial.begin(115200);

    while (!Serial)
        delay(10);
    // Start SPI bus
    //SPI.setHwCs(true);
    SPI.begin(SPI_CLK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);

    // Initialize the VS1053 decoder
    if (!stream.startDecoder(VS1053_CS, VS1053_DCS, VS1053_DREQ) || !stream.isChipConnected()) {
        Serial.println("Decoder not running - system halted");
        while (1) delay(100);
    }

    // Mount SD card
    if (!mountSDcard()) {
        Serial.println("SD card not mounted - system halted");
        while (1) delay(100);
    }
    // DOKŁADNIE TAKA SAMA KOLEJNOŚĆ JAK W ORYGINALE
    WiFi.begin(ssid, password);
    Serial.print("Łączenie z WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n✅ Połączono z WiFi!");

    //INICJALIZACJA WYŚWIETLACZA
    tft.initR(INITR_BLACKTAB);
    tft.setRotation(2);
    tft.fillScreen(ST77XX_BLACK);
    delay(100);
    tft.drawRGBBitmap(0, 0, roz, 128, 160);
    // Bluetooth NA SAMYM KOŃCU - po wszystkim
    // Serial.println("🔵 Uruchamiam Bluetooth...");
    // if (!SerialBT.begin("ESP32_TTS")) {
    //     Serial.println("❌ Bluetooth ERROR");
    // } else {
    //     Serial.println("✅ Bluetooth OK jako 'ESP32_TTS'");
    // }

    

    Serial.println("SD card mounted - starting decoder...");

    

    

}

void loop() {
    stream.loop();

    // USB Serial - oryginalna obsługa
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n') {
            inputText.trim();
            if (inputText.length() > 0) {
                Serial.println("📥 [USB] Tekst: " + inputText);
                
                // SerialBT.flush();         // Opróżnij bufor transmisji
                // SerialBT.disconnect();    // Rozłącz aktywne połączenie
                // delay(100);               // Krótka pauza na zakończenie procesu
                // SerialBT.end(); 
                
                String odpowiedz = wyslijZapytanie(inputText);
                Serial.println("📥 Clarin: " + odpowiedz);
                String oczyszczony = cleanText(odpowiedz);
                isPlaying = true;

                if (fetchSpeechFromElevenLabs(oczyszczony, "/mp3/speach2.mp3")) {
                    cleanMP3File("/mp3/speach2.mp3", "/mp3/speach2_clean.mp3", 5);
                    tft.drawRGBBitmap(0, 0, roz, 128, 160);
                    delay(300);
                    tft.drawRGBBitmap(0, 0, roz_1, 128, 160);
                    delay(300);
                    tft.drawRGBBitmap(0, 0, roz_2, 128, 160);
                    delay(300);
                    tft.drawRGBBitmap(0, 0, roz_3, 128, 160);
                    stream.connecttofile(SD, "/mp3/speach2_clean.mp3");
                }

                inputText = "";
            }
        } else {
            inputText += c;
        }
    }

    // Bluetooth Serial
    // while (SerialBT.available()) {
    //     char c = SerialBT.read();
    //     if (c == '\n' || c == '\r') {
    //         if (btInputText.length() > 0) {
    //             Serial.println("📱 [BT] Tekst: " + btInputText);
                
    //             // SerialBT.flush();         // Opróżnij bufor transmisji
    //             // SerialBT.disconnect();    // Rozłącz aktywne połączenie
    //             // delay(100);               // Krótka pauza na zakończenie procesu
    //             // SerialBT.end(); 
                
    //             SerialBT.println("📥 Przetwarzam...");
                
    //             String odpowiedz = wyslijZapytanie(btInputText);
    //             Serial.println("📥 Clarin: " + odpowiedz);
    //             SerialBT.println("🤖 " + odpowiedz);
    //             isPlaying = true;

    //             if (fetchSpeechFromElevenLabs(odpowiedz, "/mp3/speach2.mp3")) {
    //                 cleanMP3File("/mp3/speach2.mp3", "/mp3/speach2_clean.mp3", 5);
    //                 stream.connecttofile(SD, "/mp3/speach2_clean.mp3");
    //             } else {
    //                 SerialBT.println("❌ Błąd TTS");
    //             }

    //             btInputText = "";
    //         }
    //     } else {
    //         btInputText += c;
    //     }
    // }

    if (!stream.isRunning() && isPlaying) {
        //SerialBT.begin("ESP32_TTS");
        tft.drawRGBBitmap(0, 0, roz_3, 128, 160);
        delay(300);
        tft.drawRGBBitmap(0, 0, roz_2, 128, 160);
        delay(300);
        tft.drawRGBBitmap(0, 0, roz_1, 128, 160);
        delay(300);
        tft.drawRGBBitmap(0, 0, roz, 128, 160);
        Serial.println("✅ Gotowe. Wpisz kolejny tekst:");
        //SerialBT.println("✅ Gotowe");
        isPlaying = false;
    }
    
    delay(5);
}

String cleanText(String input) {
  String output = "";

  for (size_t i = 0; i < input.length(); i++) {
    char c = input.charAt(i);

    // Pomijamy entery
    if (c == '\n' || c == '\r') continue;

    // Sprawdzamy, czy znak jest częścią UTF-8 emotki
    // Emotki zaczynają się od bajtów >= 0xF0
    if ((uint8_t)c >= 0xF0) {
      // Pomijamy całą sekwencję emotki (zwykle 4 bajty)
      i += 3; // zakładamy 4-bajtowe emotki
      continue;
    }

    output += c;
  }

  return output;
}

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

    client.print(String("POST ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "xi-api-key: " + apiKey + "\r\n" +
                 "Content-Type: application/json\r\n" +
                 "Content-Length: " + payload.length() + "\r\n" +
                 "Connection: close\r\n\r\n" +
                 payload);

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
                startTime = millis();
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