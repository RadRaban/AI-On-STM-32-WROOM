#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include "roz.h"


#define TFT_CS     5
#define TFT_RST    4
#define TFT_DC     2
#define TFT_SCK    18   // CLK
#define TFT_MOSI   23   // MOSI


Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);


void setup() {
    Serial.begin(115200);


    // Inicjalizacja SPI z ręcznie ustawionymi pinami
    SPI.begin(TFT_SCK, -1, TFT_MOSI, TFT_CS);  


    // Inicjalizacja wyświetlacza
    tft.initR(INITR_GREENTAB);
    tft.setRotation(0);


    // Test kolorów
    //tft.fillScreen(ST77XX_RED);
    //delay(1000);
    //tft.fillScreen(ST77XX_GREEN);
    //delay(1000);
    //tft.fillScreen(ST77XX_BLUE);


    // Wyświetlenie obrazu
    tft.drawRGBBitmap(0, 0, roz, 128, 160);
}


void loop() {
}
