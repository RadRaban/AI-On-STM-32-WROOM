#include <SPI.h>
#include <SD.h>
#include "plugin.h" // zawiera uint16_t plugin[]

#define XCS   32    // Chip Select for SCI
#define XDCS  33   // Chip Select for SDI (data)
#define DREQ  35    // Data Request pin from VS1053
#define SD_CS 5   // Chip Select for SD card

File audioFile;
unsigned long startTime;
uint32_t audioDataSize = 0;

void writeSCI(uint8_t addr, uint16_t data) {
  while (!digitalRead(DREQ)); // czekaj aż VS1053 gotowy
  digitalWrite(XCS, LOW);
  SPI.transfer(0x02); // Write command
  SPI.transfer(addr);
  SPI.transfer16(data);
  digitalWrite(XCS, HIGH);
}

void loadPlugin(const uint16_t *plugin, size_t size) {
  for (size_t i = 0; i < size;) {
    uint16_t addr = plugin[i++];
    uint16_t n = plugin[i++];
    if (n & 0x8000U) {
      n &= 0x7FFF;
      uint16_t val = plugin[i++];
      while (n--) writeSCI(addr, val);
    } else {
      while (n--) {
        uint16_t val = plugin[i++];
        writeSCI(addr, val);
      }
    }
  }
}

void writeWavHeader(File f) {
  // Nagłówek WAV dla mono, 8kHz, 4-bit IMA ADPCM
  uint32_t sampleRate = 8000;
  uint16_t bitsPerSample = 4;
  uint16_t channels = 1;
  uint32_t dataSize = 0xFFFFFFFF; // placeholder
  uint32_t byteRate = sampleRate * channels * bitsPerSample / 8;
  uint16_t blockAlign = 256;
  uint16_t extraSize = 0;

  // RIFF header
  f.write((const uint8_t *)"RIFF", 4);
  f.write((byte *)&dataSize, 4); // ChunkSize (placeholder)
  f.write((const uint8_t *)"WAVE", 4);

  // fmt chunk
  f.write((const uint8_t *)"fmt ", 4);
  uint32_t fmtSize = 20; // 16 + 2 (extraSize) + 2 (padding)
  f.write((byte *)&fmtSize, 4);
  uint16_t format = 0x11; // IMA ADPCM
  f.write((byte *)&format, 2);
  f.write((byte *)&channels, 2);
  f.write((byte *)&sampleRate, 4);
  f.write((byte *)&byteRate, 4);
  f.write((byte *)&blockAlign, 2);
  f.write((byte *)&bitsPerSample, 2);
  f.write((byte *)&extraSize, 2); // Extra bytes

  // data chunk
  f.write((const uint8_t *)"data", 4);
  f.write((byte *)&dataSize, 4); // Subchunk2Size (placeholder)
}


void stopRecording() {
  // Zaktualizuj rozmiar pliku w nagłówku WAV
  audioFile.seek(4);
  uint32_t fileSize = 36 + audioDataSize;
  audioFile.write((byte *)&fileSize, 4);

  // Zaktualizuj rozmiar sekcji "data"
  audioFile.seek(40);
  audioFile.write((byte *)&audioDataSize, 4);

  audioFile.close();
  Serial.println("Nagrywanie zakończone, plik zamknięty.");
}

void setup() {
  Serial.begin(115200);
  SPI.begin();

  pinMode(XCS, OUTPUT);
  pinMode(XDCS, OUTPUT);
  pinMode(DREQ, INPUT);
  digitalWrite(XCS, HIGH);
  digitalWrite(XDCS, HIGH);

  if (!SD.begin(SD_CS)) {
    Serial.println("Błąd SD");
    return;
  }

  audioFile = SD.open("/recording.raw", FILE_WRITE);
  if (!audioFile) {
    Serial.println("Nie można utworzyć pliku");
    return;
  }

  writeWavHeader(audioFile); // opcjonalnie

  Serial.println("Ładowanie pluginu...");
  loadPlugin(plugin, sizeof(plugin) / sizeof(plugin[0]));
  Serial.println("Plugin załadowany");

  // Rozpocznij nagrywanie
  writeSCI(0x0E, 0x0004); // SCI_MODE: SM_ADPCM
  writeSCI(0x0C, 0x0001); // SCI_AIADDR: start pluginu (adres może się różnić)

  startTime = millis();
}

void loop() {
  if (millis() - startTime > 10000) {
    stopRecording();
    while (true); // zatrzymaj program
  }

  if (digitalRead(DREQ) == HIGH) {
    digitalWrite(XDCS, LOW);
    uint8_t buffer[32];
    for (int i = 0; i < 32; i++) {
      buffer[i] = SPI.transfer(0x00);
    }
    digitalWrite(XDCS, HIGH);
    audioFile.write(buffer, 32);
    audioDataSize += 32;
  }
}
