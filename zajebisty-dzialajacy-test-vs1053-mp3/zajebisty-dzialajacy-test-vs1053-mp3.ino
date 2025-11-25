#include <Arduino.h>
#include <SD.h>
#include <VS1053.h>               // https://github.com/baldram/ESP_VS1053_Library
#include <ESP32_VS1053_Stream.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
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

ESP32_VS1053_Stream stream;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

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

    // Start SPI bus
    //SPI.setHwCs(true);
    //SPI.begin(SPI_CLK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);

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

    //INICJALIZACJA WYŚWIETLACZA
    tft.initR(INITR_BLACKTAB);
    tft.setRotation(2);
    tft.fillScreen(ST77XX_BLACK);
    delay(100);
    tft.drawRGBBitmap(0, 0, roz, 128, 160);

    Serial.println("VS1053 running - starting SD playback");
    tft.drawRGBBitmap(0, 0, roz, 128, 160);
                    delay(300);
                    tft.drawRGBBitmap(0, 0, roz_1, 128, 160);
                    delay(300);
                    tft.drawRGBBitmap(0, 0, roz_2, 128, 160);
                    delay(300);
                    tft.drawRGBBitmap(0, 0, roz_3, 128, 160);
    // Start playback from an SD file
    stream.connecttofile(SD, "/mp3/nagranie.mp3");

    File f = SD.open("/mp3/nagranie.mp3");
if (f) {
  Serial.println("✅ Plik istnieje, rozmiar: " + String(f.size()));
  f.close();
} else {
  Serial.println("❌ Plik nie istnieje!");
}
    if (!stream.isRunning()) {
        Serial.println("No file running - system halted");
        while (1) delay(100);
    }

    Serial.print("Codec: ");
    Serial.println(stream.currentCodec());
}

void loop() {
    stream.loop();
    delay(5);
}

void audio_eof_stream(const char* info) {
    Serial.printf("End of file: %s\n", info);
    tft.drawRGBBitmap(0, 0, roz_3, 128, 160);
        delay(300);
        tft.drawRGBBitmap(0, 0, roz_2, 128, 160);
        delay(300);
        tft.drawRGBBitmap(0, 0, roz_1, 128, 160);
        delay(300);
        tft.drawRGBBitmap(0, 0, roz, 128, 160);
}