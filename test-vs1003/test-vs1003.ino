#include <VS1053.h>
#include <SPI.h>

#define VS1053_CS   32
#define VS1053_DCS  33
#define VS1053_DREQ 35

VS1053 player(VS1053_CS, VS1053_DCS, VS1053_DREQ);

void setup() {
  Serial.begin(115200);
  SPI.begin();
  player.begin();
  player.setVolume(95); // Maksymalna głośność

  Serial.println("VS1053 test rozpoczęty");
  if (player.getChipVersion() == 4) {
    Serial.println("VS1053 wykryty!");
  } else {
    Serial.println("Nie wykryto VS1053");
  }
}

void loop() {
  // Możesz tu dodać odtwarzanie próbki MP3
}
