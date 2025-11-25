#include <SPI.h>
#include <SD.h>

#define VS1053_CS     32
#define VS1053_DCS    33
#define VS1053_DREQ   35
#define VS1053_RESET  15
#define SDREADER_CS    5

#define REC_BUFFER_SIZE 512
File audioFile;
bool isRecording = false;
unsigned long recordingStartTime = 0;
uint8_t recBuf[REC_BUFFER_SIZE];

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(VS1053_DREQ, INPUT);
  pinMode(VS1053_CS, OUTPUT);
  pinMode(VS1053_DCS, OUTPUT);
  pinMode(VS1053_RESET, OUTPUT);
  digitalWrite(VS1053_CS, HIGH);
  digitalWrite(VS1053_DCS, HIGH);
  digitalWrite(VS1053_RESET, HIGH);

  SPI.begin();

  if (!SD.begin(SDREADER_CS)) {
    Serial.println("SD init failed!");
    while (1);
  }

  Serial.println("Ready. Send 'r' to start recording.");
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "r" && !isRecording) {
      startRecording();
    }
  }

  if (isRecording && millis() - recordingStartTime >= 10000) {
    stopRecording();
  }

  if (isRecording && digitalRead(VS1053_DREQ)) {
    uint16_t words = readSCI(0x1E); // RECWORDS
    if (words >= 256) {
      for (int i = 0; i < 256; i++) {
        uint16_t w = readSCI(0x1F); // RECDATA
        recBuf[2*i] = w >> 8;
        recBuf[2*i+1] = w & 0xFF;
      }
      audioFile.write(recBuf, 512);
    }
  }
}

void startRecording() {
  Serial.println("Starting Ogg recording...");
  isRecording = true;
  recordingStartTime = millis();

  resetVS1053();

  writeSCI(0x00, readSCI(0x00) | 0x0004); // SM_RESET
  delay(10);

  writeMem(0xC01A, 0x0002); // Disable interrupts
  loadPlugin(encoderPlugin, sizeof(encoderPlugin)/2);

  writeSCI(0x00, readSCI(0x00) | 0x0020 | 0x0004); // SM_LINE1 | SM_ADPCM
  writeSCI(0x0C, 1024); // RECGAIN
  writeSCI(0x0F, 0);    // AICTRL3
  writeSCI(0x0A, 0x0034); // AIADDR = 0x34 (start encoder)

  writeSCI(0x0F, 0x8080); // Reset VU meter

  audioFile = SD.open("/recording.ogg", FILE_WRITE);
}

void stopRecording() {
  Serial.println("Stopping recording...");
  isRecording = false;

  writeSCI(0x0F, readSCI(0x0F) | 0x0001); // Set AICTRL3 bit 0 to stop encoder

  delay(100);
  while (!(readSCI(0x0F) & 0x0002)) {
    delay(10); // Wait until encoder signals completion
  }

  int wordsLeft = readSCI(0x1E);
  while (wordsLeft--) {
    uint16_t w = readSCI(0x1F);
    audioFile.write(w >> 8);
    audioFile.write(w & 0xFF);
  }

  audioFile.close();
  Serial.println("Recording saved.");
}

void resetVS1053() {
  digitalWrite(VS1053_RESET, LOW);
  delay(100);
  digitalWrite(VS1053_RESET, HIGH);
  while (!digitalRead(VS1053_DREQ)) delay(1);
}

void writeSCI(uint8_t addr, uint16_t data) {
  while (!digitalRead(VS1053_DREQ));
  digitalWrite(VS1053_CS, LOW);
  SPI.transfer(0x02);
  SPI.transfer(addr);
  SPI.transfer16(data);
  digitalWrite(VS1053_CS, HIGH);
}

uint16_t readSCI(uint8_t addr) {
  while (!digitalRead(VS1053_DREQ));
  digitalWrite(VS1053_CS, LOW);
  SPI.transfer(0x03);
  SPI.transfer(addr);
  uint16_t result = SPI.transfer16(0xFFFF);
  digitalWrite(VS1053_CS, HIGH);
  return result;
}

void writeMem(uint16_t addr, uint16_t data) {
  writeSCI(0x0B, addr); // WRAMADDR
  writeSCI(0x0C, data); // WRAM
}

void loadPlugin(const uint16_t *plugin, uint16_t len) {
  for (int i = 0; i < len;) {
    uint16_t addr = plugin[i++];
    uint16_t n = plugin[i++];
    if (n & 0x8000U) {
      n &= 0x7FFF;
      uint16_t val = plugin[i++];
      while (n--) writeSCI(addr, val);
    } else {
      while (n--) writeSCI(addr, plugin[i++]);
    }
  }
}

// Dummy plugin for compilation — replace with actual encoderPlugin[]
const uint16_t encoderPlugin[] = {
  // Wstaw tutaj zawartość venc44k2q05.plg jako tablicę uint16_t
};
