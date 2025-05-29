#include <SD.h>
#include <SPI.h>
#include <Audio.h> // z biblioteki ESP32-audioI2S


Audio audio;

#define I2S_DOUT      27
#define I2S_BCLK      26
#define I2S_LRC       25
// Lista ścieżek do plików
const char *playlist[] = {
  "/mp3/0001.mp3",
  "/mp3/0002.mp3",
  "/mp3/0003.wav",
  "/mp3/0004.mp3"
};

const int totalFiles = sizeof(playlist) / sizeof(playlist[0]);
int currentFile = 0;

void setup() {
  Serial.begin(115200);

  if (!SD.begin(5)) {
    Serial.println("Błąd inicjalizacji SD");
    return;
  }
  Serial.println("Poprawna inicjalizacja SD");

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(5); 

  // Odtwarzaj pierwszy plik
  Serial.print("Otwieram plik: ");
  Serial.println(playlist[currentFile]);
  audio.connecttoFS(SD, playlist[currentFile]);
}

void loop() {
  audio.loop();

  // Sprawdź, czy zakończono odtwarzanie
  if (!audio.isRunning()) {
    delay(500);  // krótka przerwa między plikami (opcjonalnie)

    currentFile = (currentFile + 1) % totalFiles;

    Serial.print("Otwieram plik: ");
    Serial.println(playlist[currentFile]);
    audio.connecttoFS(SD, playlist[currentFile]);
  }
}

