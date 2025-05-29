#include <FS.h>
#include <SD.h>

// GPIO25 = DAC1
#define DAC_PIN 25

// Ścieżka do pliku
const char* wavFilePath = "/mp3/0002.wav";

void setup() {
  Serial.begin(115200);

  // Inicjalizacja SD
  if (!SD.begin()) {
    Serial.println("Błąd inicjalizacji SD");
    while (1);
  }

  File file = SD.open(wavFilePath);
  if (!file) {
    Serial.println("Nie można otworzyć pliku");
    while (1);
  }

  Serial.println("Plik otwarty, pomijanie nagłówka WAV...");

  // Pomijanie nagłówka WAV — zwykle 44 bajty
  file.seek(44);

  Serial.println("Odtwarzanie...");

  while (file.available()) {
    uint8_t sample = file.read();

    // Wzmocnienie "programowe"
    int amplified = (sample - 128) * 2 + 128;  // 2x głośność, przesunięcie do środka
    amplified = constrain(amplified, 0, 255);  // ograniczenie do DAC zakresu

    dacWrite(DAC_PIN, amplified);
    delayMicroseconds(125);  // 8kHz
  }

  file.close();
  Serial.println("Gotowe.");
}

void loop() {
  // Nic więcej nie robimy
}
