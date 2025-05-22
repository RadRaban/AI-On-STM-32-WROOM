#include <SD.h>
#include <SPI.h>

#define ADC_PIN 34          // pin mikrofonu
#define SD_CS_PIN 5         // pin CS karty SD
#define SAMPLE_RATE 8000    // Hz
#define RECORD_SECONDS 5    // długość nagrania
#define FILENAME "/record.wav"

File audioFile;

void writeWavHeader(File file, int sampleRate, int dataSize) {
  int byteRate = sampleRate * 1; // 8-bit mono
  int blockAlign = 1;
  int bitsPerSample = 8;

  file.write((const uint8_t *)"RIFF", 4);

  uint32_t chunkSize = dataSize + 36;
  file.write((uint8_t *)&chunkSize, 4);

  file.write((const uint8_t *)"WAVE", 4);
  file.write((const uint8_t *)"fmt ", 4);

  uint32_t subchunk1Size = 16;
  uint16_t audioFormat = 1;
  uint16_t numChannels = 1;

  file.write((uint8_t *)&subchunk1Size, 4);
  file.write((uint8_t *)&audioFormat, 2);
  file.write((uint8_t *)&numChannels, 2);
  file.write((uint8_t *)&sampleRate, 4);
  file.write((uint8_t *)&byteRate, 4);
  file.write((uint8_t *)&blockAlign, 2);
  file.write((uint8_t *)&bitsPerSample, 2);

  file.write((const uint8_t *)"data", 4);
  file.write((uint8_t *)&dataSize, 4);
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(10); // 10-bit ADC (0–1023)

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Błąd SD!");
    while (1);
  }

  Serial.println("Nagrywanie...");
  audioFile = SD.open(FILENAME, FILE_WRITE);
  if (!audioFile) {
    Serial.println("Nie mogę otworzyć pliku!");
    while (1);
  }

  // zarezerwuj miejsce na nagłówek (44 bajty WAV)
  for (int i = 0; i < 44; i++) audioFile.write((byte)0);

  unsigned long start = millis();
  unsigned long duration = RECORD_SECONDS * 1000;
  unsigned long lastSampleTime = 0;
  int sampleInterval = 1000 / SAMPLE_RATE; // [ms]

  int bytesRecorded = 0;

  while (millis() - start < duration) {
    if (millis() - lastSampleTime >= sampleInterval) {
      int val = analogRead(ADC_PIN); // 0–1023
      uint8_t sample = map(val, 0, 1023, 0, 255);
      audioFile.write(sample);
      bytesRecorded++;
      lastSampleTime = millis();
    }
  }

  Serial.println("Zakończono nagrywanie");

  audioFile.seek(0);
  writeWavHeader(audioFile, SAMPLE_RATE, bytesRecorded);
  audioFile.close();

  Serial.println("Plik WAV zapisany");
}

void loop() {
  // nic
}
