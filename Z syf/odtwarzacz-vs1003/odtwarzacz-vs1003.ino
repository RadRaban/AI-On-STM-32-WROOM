#include <SPI.h>
#include <SD.h>
#include <Adafruit_VS1053.h>

// Piny VS1053
#define VS1053_RESET  15
#define VS1053_CS     32
#define VS1053_DCS    33
#define CARDCS         5  // SD card CS
#define VS1053_DREQ    35
#define MOSI 23
#define MISO 19
#define SCK  18

Adafruit_VS1053_FilePlayer musicPlayer =
  Adafruit_VS1053_FilePlayer(MOSI, MISO, SCK, VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);


void setup() {
    Serial.begin(115200);
  if (!musicPlayer.begin()) {
    Serial.println("Nie znaleziono VS1053");
    while (1);
  }
  
  Serial.print("DREQ status: ");
Serial.println(digitalRead(VS1053_DREQ));
  Serial.println("VS1053 OK, test tonu...");
musicPlayer.sineTest(0x41, 1000); // 440 Hz
musicPlayer.sineTest(0x42, 1000); // 554 Hz
musicPlayer.sineTest(0x43, 1000); // 659 Hz
musicPlayer.sineTest(0x45, 1000); // 880 Hz
}

void loop() {
  
}

