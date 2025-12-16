#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <SD.h>

#define TFT_CS     2
#define TFT_RST    4
#define TFT_DC     25
#define TFT_SCK    18
#define TFT_MOSI   23
#define SD_CS      5   // pin CS karty SD

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

void setup() {
    Serial.begin(115200);
    SPI.begin(TFT_SCK, -1, TFT_MOSI, TFT_CS);  
    tft.initR(INITR_BLACKTAB);
    tft.setRotation(2);
    tft.fillScreen(ST77XX_BLACK);

    if (!SD.begin(SD_CS)) {
        Serial.println("Błąd SD!");
        while (1);
    }
    Serial.println("SD OK");
}

void drawRaw(const char *filename) {
    File f = SD.open(filename);
    if (!f) {
        Serial.print("Nie mogę otworzyć pliku: ");
        Serial.println(filename);
        return;
    }

    uint16_t lineBuffer[128]; // bufor jednej linii
    for (int y = 0; y < 160; y++) {
        f.read((uint8_t*)lineBuffer, 128 * 2);
        // korekta kolejności bajtów
        for (int x = 0; x < 128; x++) {
            uint16_t c = lineBuffer[x];
            lineBuffer[x] = (c >> 8) | (c << 8);
        }
        tft.drawRGBBitmap(0, y, lineBuffer, 128, 1);
    }
    f.close();
}

void playAnimation(const char* folder, int frames) {
    char filename[64];
    Serial.println(folder);
    for (int i = 0; i <= frames; i++) {
        // budowanie nazwy: np. "/pije/pije_024.bmp"
        sprintf(filename, "/raw/%s/%s_%03d.raw", folder, folder, i);
        //tft.fillScreen(ST77XX_BLACK);
        drawRaw(filename);
        delay(10); // prędkość animacji
    }
}

void loop() {
    //playAnimation("pije", 52);

    playAnimation("pije", 50);
    playAnimation("odstawienie", 49);
    playAnimation("sluchanie", 51);
    playAnimation("mowi", 50);
    playAnimation("mowi", 50);
    playAnimation("przyklada", 50);
    //playAnimation("mowi", 51);
    // możesz dodać inne foldery
}