#include <Arduino.h>
#include <SD.h>
#include <VS1053.h>               // https://github.com/baldram/ESP_VS1053_Library
#include <ESP32_VS1053_Stream.h>

#define SPI_CLK_PIN 18
#define SPI_MISO_PIN 19
#define SPI_MOSI_PIN 23

#define VS1053_CS 32
#define VS1053_DCS 33
#define VS1053_DREQ 35
#define SDREADER_CS 5

#define SCI_MODE      0x00
#define SCI_AICTRL0   0x0C
#define SCI_AICTRL1   0x0D
#define SCI_AICTRL2   0x0E
#define SCI_AICTRL3   0x0F
#define SCI_HDAT0     0x08
#define SCI_HDAT1     0x09
#define SM_RESET      0x04
#define SM_ADPCM      0x10
#define SM_LINE1 1
#define RM_53_FORMAT_IMA_ADPCM  0x0000
#define RM_53_FORMAT_PCM        0x0004
#define RM_53_ADC_MODE_LEFT     0x0002

ESP32_VS1053_Stream stream;
VS1053 vs1053(VS1053_CS, VS1053_DCS, VS1053_DREQ);



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
    vs1053.begin();
    Serial.println("\n\nVS1053 SD Card Playback Example\n");

    // Start SPI bus
    SPI.setHwCs(true);
    SPI.begin(SPI_CLK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);

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

    Serial.println("VS1053 running - starting SD playback");
    recordToSD_ADPCM("/mp3/speach1_clean.adpcm", 3000);
    wrapADPCMwithWAVHeader("/mp3/speach1_clean.adpcm", "/mp3/speach1_clean.wav",3000);
    stream.connecttofile(SD, "/mp3/speach1_clean.wav");

    File f = SD.open("/mp3/speach1_clean.wav");
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

void initVS1053Recording() {
  vs1053.softReset();  // pełny reset układu

  // Ustaw tryb ADPCM bez SM_LINE1
  uint16_t mode = SM_RESET | SM_ADPCM;
  vs1053.writeRegister(SCI_AICTRL3, RM_53_FORMAT_PCM | RM_53_ADC_MODE_LEFT);


  // Ustaw parametry nagrywania
  vs1053.writeRegister(SCI_AICTRL0, 8000); // sample rate
  vs1053.writeRegister(SCI_AICTRL1, 0);    // format: 0 = ADPCM
  vs1053.writeRegister(SCI_AICTRL2, 0);    // gain
  vs1053.writeRegister(SCI_AICTRL3, 0x0002); // ADC mode: MONO (left channel)

  // Diagnostyka
  uint16_t modeRead = readVS1053Register(SCI_MODE);
  Serial.printf("SCI_MODE: 0x%04X\n", modeRead);
  if (modeRead & SM_LINE1) {
    Serial.println("⚠️ SM_LINE1 aktywny — nagrywasz z LINE-IN zamiast mikrofonu!");
  } else {
    Serial.println("✅ Nagrywanie z MIC-IN aktywne");
  }
}

void wrapADPCMwithWAVHeader(const char* adpcmPath, const char* wavPath, uint32_t sampleRate) {
  File adpcmFile = SD.open(adpcmPath, FILE_READ);
  if (!adpcmFile) {
    Serial.println("❌ Nie można otworzyć pliku ADPCM");
    return;
  }

  File wavFile = SD.open(wavPath, FILE_WRITE);
  if (!wavFile) {
    Serial.println("❌ Nie można utworzyć pliku WAV");
    adpcmFile.close();
    return;
  }

  uint32_t dataSize = adpcmFile.size();
  uint32_t fileSize = dataSize + 44 - 8;

  // Nagłówek WAV (ADPCM IMA)
  wavFile.write((uint8_t*)"RIFF", 4);
  wavFile.write((uint8_t*)&fileSize, 4);
  wavFile.write((uint8_t*)"WAVE", 4);
  wavFile.write((uint8_t*)"fmt ", 4);

  uint32_t fmtChunkSize = 20;
  wavFile.write((uint8_t*)&fmtChunkSize, 4);

  uint16_t audioFormat = 0x11; // IMA ADPCM
  uint16_t numChannels = 1;
  uint16_t blockAlign = 256;
  uint16_t bitsPerSample = 4;
  uint16_t samplesPerBlock = 505;

  wavFile.write((uint8_t*)&audioFormat, 2);
  wavFile.write((uint8_t*)&numChannels, 2);
  wavFile.write((uint8_t*)&sampleRate, 4);

  uint32_t byteRate = sampleRate * blockAlign / samplesPerBlock;
  wavFile.write((uint8_t*)&byteRate, 4);
  wavFile.write((uint8_t*)&blockAlign, 2);
  wavFile.write((uint8_t*)&bitsPerSample, 2);
  wavFile.write((uint8_t*)&samplesPerBlock, 2);

  wavFile.write((uint8_t*)"data", 4);
  wavFile.write((uint8_t*)&dataSize, 4);

  // Skopiuj dane ADPCM
  uint8_t buffer[512];
  while (adpcmFile.available()) {
    size_t len = adpcmFile.read(buffer, sizeof(buffer));
    wavFile.write(buffer, len);
  }

  adpcmFile.close();
  wavFile.close();
  Serial.println("✅ Utworzono plik WAV z nagłówkiem: " + String(wavPath));
}

uint16_t readVS1053Register(uint8_t reg) {
  digitalWrite(VS1053_CS, LOW);
  SPI.transfer(0x03);       // SCI_READ opcode
  SPI.transfer(reg);        // rejestr do odczytu
  uint16_t result = SPI.transfer(0xFF) << 8;
  result |= SPI.transfer(0xFF);
  digitalWrite(VS1053_CS, HIGH);
  return result;
}

void recordToSD_ADPCM(const char* filename, unsigned long duration_ms) {
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("❌ Nie można otworzyć pliku do zapisu");
    return;
  }

  // Reset VS1053
  vs1053.softReset();

  // Ustaw tryb ADPCM

  // Ustaw parametry nagrywania
  initVS1053Recording();

  Serial.println("🎙️ Rozpoczynam nagrywanie...");

  unsigned long start = millis();
  while (millis() - start < duration_ms) {
    if (digitalRead(VS1053_DREQ) == HIGH) {
      uint16_t hdat0 = readVS1053Register(SCI_HDAT0);
      uint16_t hdat1 = readVS1053Register(SCI_HDAT1);
      Serial.printf("HDAT0: %04X, HDAT1: %04X\n", hdat0, hdat1);
      file.write((uint8_t*)&hdat0, 2);
      file.write((uint8_t*)&hdat1, 2);
    }
  }

  file.close();
  Serial.println("✅ Nagrywanie zakończone: " + String(filename));
}

void audio_eof_stream(const char* info) {
    Serial.printf("End of file: %s\n", info);
}