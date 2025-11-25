#include <SPI.h>
#include <SD.h>
#include <VS1053.h>               // https://github.com/baldram/ESP_VS1053_Library
#include <ESP32_VS1053_Stream.h>


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

uint16_t loadPlugin(File &f) {
  if (f.read() != 'P' || f.read() != '&' || f.read() != 'H') return 0xFFFF;

  while (f.available()) {
    uint8_t type = f.read();
    if (type >= 4) return 0xFFFF;

    static const uint16_t offsets[3] = {0x8000, 0x0000, 0x4000};
    uint16_t len = (f.read() << 8) | (f.read() & ~1);
    uint16_t addr = (f.read() << 8) | f.read();

    if (type == 3) {
  Serial.print("Found execute record at address: ");
  Serial.println(addr, HEX);
  return addr;
}

Serial.print("len = "); Serial.println(len);
Serial.print("addr = "); Serial.println(addr + offsets[type], HEX);

    writeSCI(7, addr + offsets[type]);

    while (len > 0) {
      int hi = f.read();
  int lo = f.read();
  if (hi < 0 || lo < 0) {
    Serial.println("Unexpected EOF while reading plugin data");
    return 0xFFFF;
  }
      uint16_t data = (f.read() << 8) | f.read();
      writeSCI(6, data);
      len -= 2;
    }
  }
  Serial.println("6");
  return 0xFFFF;
}

bool endRecording() {
  return millis() - startTime > 10000;
}

void setup() {
  Serial.begin(115200);
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

  if (!SD.begin(SD_CS)) {
    Serial.println("SD init failed");
    while (1);
  }

  pluginFile = SD.open("/venc44k2q05.img");
  if (!pluginFile) {
    Serial.println("Plugin not found");
    while (1);
  }

  

  uint16_t startAddr = loadPlugin(pluginFile);
  pluginFile.close();

  if (startAddr == 0xFFFF) {
    Serial.println("Plugin load failed");
    while (1);
  }

  outFile = SD.open("/record.ogg", FILE_WRITE);
  if (!outFile) {
    Serial.println("File open failed");
    while (1);
  }


  writeSCI(0, 0x4804); // SM_SDINEW | SM_ADPCM
  writeSCI(12, 1024);  // Rec level
  writeSCI(13, 0);     // Max AGC
  writeSCI(14, 0);     // Misc
  writeSCI(10, startAddr); // Start plugin

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
