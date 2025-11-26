#include <SPI.h>
#include <SD.h>
#include <VS1053.h>               // https://github.com/baldram/ESP_VS1053_Library
#include <ESP32_VS1053_Stream.h>
#include "venc44k2q05.h"

#define XCS   32    // Chip Select for SCI
#define XDCS  33    // Chip Select for SDI (data)
#define DREQ  25    // Data Request pin from VS1053
#define SD_CS 5     // Chip Select for SD card

ESP32_VS1053_Stream stream;
File pluginFile;
File outFile;
unsigned long startTime;
uint8_t state = 0;

void writeSCI(uint8_t addr, uint16_t data) {
  while (!digitalRead(DREQ));
  Serial.print("69");
  digitalWrite(XCS, LOW);
  SPI.transfer(0x02);
  SPI.transfer(addr);
  SPI.transfer16(data);
  digitalWrite(XCS, HIGH);
}

uint16_t readSCI(uint8_t addr) {
  while (!digitalRead(DREQ));
  digitalWrite(XCS, LOW);
  SPI.transfer(0x03);
  SPI.transfer(addr);
  uint16_t result = SPI.transfer16(0xFFFF);
  digitalWrite(XCS, HIGH);
  return result;
}

void waitForDREQ() {
  while (!digitalRead(DREQ));
}

void loadPluginFromArray(const uint16_t *plugin, size_t size) {
  for (size_t i = 0; i < size;) {
    Serial.print("69");
    uint16_t addr = plugin[i++];
    uint16_t n = plugin[i++];
    if (n & 0x8000U) {
      n &= 0x7FFF;
      uint16_t val = plugin[i++];
      while (n--) writeSCI(6, val); // SCI_WRAM
    } else {
      while (n--) {
        uint16_t val = plugin[i++];
        writeSCI(6, val); // SCI_WRAM
      }
    }
  }
}

void LoadUserCode(void) {
  int i = 0;

  while (i<sizeof(plugin)/sizeof(plugin[0])) {
    unsigned short addr, n, val;
    addr = plugin[i++];
    n = plugin[i++];
    if (n & 0x8000U) { /* RLE run, replicate n samples */
      n &= 0x7FFF;
      val = plugin[i++];
      while (n--) {
        writeSCI(addr, val);
      }
    } else {           /* Copy run, copy n samples */
      while (n--) {
        val = plugin[i++];
        writeSCI(addr, val);
      }
    }
  }
}


bool endRecording() {
  return millis() - startTime > 10000;
}

void setup() {
  Serial.begin(115200);
  Serial.println("1");
  SPI.begin();
  // Initialize the VS1053 decoder
    // if (!stream.startDecoder(XCS, XDCS, DREQ) || !stream.isChipConnected()) {
    //     Serial.println("Decoder not running - system halted");
    //     while (1) delay(100);
    // }
  pinMode(XCS, OUTPUT);
  pinMode(XDCS, OUTPUT);
  pinMode(DREQ, INPUT);
  digitalWrite(XCS, HIGH);
  digitalWrite(XDCS, HIGH);
  Serial.println("2");

  if (!SD.begin(SD_CS)) {
    Serial.println("SD init failed");
    while (1);
  }
  Serial.println("3");
  //loadPluginFromArray(plugin, pluginSize);
  LoadUserCode();
  Serial.println("Plugin loaded from header");
  pluginFile.close();

  writeSCI(10, 0x0034); // SCIR_AIADDR = 0x0034 (typowy adres startowy dla Ogg pluginu)


  outFile = SD.open("/record1.ogg", FILE_WRITE);
  if (!outFile) {
    Serial.println("File open failed");
    while (1);
  }


  writeSCI(0, 0x4804); // SM_SDINEW | SM_ADPCM
  writeSCI(12, 1024);  // Rec level
  writeSCI(13, 0);     // Max AGC
  writeSCI(14, 0);     // Misc
  writeSCI(10, 0x0034); // SCIR_AIADDR = 0x0034 (typowy adres startowy dla Ogg pluginu)

  startTime = millis();
  Serial.println("Recording started");
}

void loop() {
  if (state == 0 && endRecording()) {
    state = 1;
    writeSCI(14, 1); // Request stop
  }

  uint16_t wordsWaiting = readSCI(9); // HDAT1

  if (state == 1 && (readSCI(14) & (1 << 1))) {
    state = 2;
    wordsWaiting = readSCI(9);
  }

  while (wordsWaiting >= ((state < 2) ? 256 : 1)) {
    uint16_t wordsToRead = (wordsWaiting < 256) ? wordsWaiting : 256;

    wordsWaiting -= wordsToRead;

    if (state == 2 && wordsWaiting == 0) wordsToRead--;

    for (uint16_t i = 0; i < wordsToRead; i++) {
      uint16_t t = readSCI(8); // HDAT0
      outFile.write(t >> 8);
      outFile.write(t & 0xFF);
    }

    if (wordsToRead < 256) {
      uint16_t lastWord = readSCI(8);
      outFile.write(lastWord >> 8);
      readSCI(14);
      if (!(readSCI(14) & (1 << 2))) {
        outFile.write(lastWord & 0xFF);
      }
      state = 3;
      outFile.close();
      Serial.println("Recording finished");
      writeSCI(0, 0x4804); // Reset
      while (1);
    }
  }
}