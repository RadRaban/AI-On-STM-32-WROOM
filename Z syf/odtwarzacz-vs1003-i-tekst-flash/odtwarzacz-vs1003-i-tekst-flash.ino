#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <SPIMemory.h>
#include <Adafruit_VS1053.h>


// KONFIGURACJA WI-FI
const char* ssid = "NORA 24";
const char* password = "eloelo520";
const char* host = "api.streamelements.com";
const int httpsPort = 443;
// Piny VS1053
#define VS1053_RESET  15
#define VS1053_CS     32
#define VS1053_DCS    33
#define CARDCS         -1  // SD card CS
#define VS1053_DREQ    35
#define MOSI 23
#define MISO 19
#define SCK  18
#define FLASH_CS 16

Adafruit_VS1053_FilePlayer musicPlayer =
  Adafruit_VS1053_FilePlayer(MOSI, MISO, SCK, VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);


//---------------------------------------------------zmień ten pin na jakis inny bo wywala
SPIClass hspi(HSPI);
SPIFlash flash(FLASH_CS, &hspi); // CS dla W25Q64
const uint32_t flashAddress = 0x000000;
uint32_t mp3Size = 0;

void setup() {
  Serial.begin(115200);
  hspi.begin(14, 12, 13);
  //musicPlayer.useSPI(hspi);
  delay(500);
//testGlosnika ();
  testFlashWriteRead();
  // Połączenie z Wi-Fi
  WiFi.begin(ssid, password);
  testFlashWriteRead();
  Serial.print("Łączenie z WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nPołączono z WiFi!");
testFlashWriteRead();
if (!musicPlayer.begin()) {
    Serial.println("Nie znaleziono VS1053");
    while (1);
  }
  musicPlayer.setVolume(10, 10);
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);
  testGlosnika ();

testFlashWriteRead();
  // if (nagrajAudio("/mp3/test.ogg")) {
  //   Serial.println("Plik nagrany poprawnie.");
  // } else {
  //   Serial.println("Problem z nagraniem pliku.");
  // }
}

void loop() {
  
  Serial.println("Podaj tekst do przeczytania: ");
  String userText = readSerialInput();
  Serial.print("Wczytano: ");
  Serial.println(userText);
testGlosnika ();
  if (fetchSpeechToFlash(userText)) {
    dumpFlashPreview();
    playFromFlash();
  } else {
    Serial.println("Błąd podczas pobierania mowy.");
  }
}


void testGlosnika () {
  Serial.println("🔊 Generuję pipnięcie...");
  musicPlayer.sineTest(0x44, 1000);  // 0x44 = ~1kHz, 500ms
  Serial.println("✅ Pipnięcie zakończone.");
}

void playFromFlash() {
  const uint32_t chunkSize = 32;
  uint8_t buffer[chunkSize];
  uint32_t addr = flashAddress;
  uint32_t size = mp3Size; // faktyczny rozmiar MP3

  while (size > 0) {
    uint32_t toRead = (size > chunkSize) ? chunkSize : size;
    for (uint32_t i = 0; i < toRead; i++) {
      buffer[i] = flash.readByte(addr++);
    }
    musicPlayer.playData(buffer, toRead);
    size -= toRead;
    delay(1);
  }

  musicPlayer.stopPlaying();
  Serial.println("Odtwarzanie zakończone.");
}

// ==== FUNKCJA DO TTS ====
bool fetchSpeechToFlash(const String& text) {
  HTTPClient http;
  String url = "https://api.streamelements.com/kappa/v2/speech?voice=pl-PL-Wavenet-A&text=" + urlEncode(text);
  http.begin(url);
  http.addHeader("Accept", "audio/mpeg");
  int httpCode = http.GET();

  if (httpCode != 200) {
    Serial.print("❌ Błąd HTTP: ");
    Serial.println(httpCode);
    http.end();
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();
  Serial.println("🔍 Nagłówek MP3:");
  for (int i = 0; i < 16; i++) {
    byte b;
    flash.readByte(flashAddress + i, b);
    Serial.printf("%02X ", b);
  }
Serial.println();
Serial.println("🔍 ASCII odpowiedzi:");
for (int i = 0; i < 128; i++) {
  byte b;
  flash.readByte(flashAddress + i, b);
  Serial.print((b >= 32 && b <= 126) ? (char)b : '.');
}
Serial.println();
  const size_t bufferSize = 256;
  byte buffer[bufferSize];
  size_t index = 0;
  uint32_t addr = flashAddress;
  const uint32_t sectorSize = 4096;
  uint32_t nextSectorErase = flashAddress & ~(sectorSize - 1);

  Serial.println("💾 Rozpoczynam zapis MP3 do Flash...");
  uint32_t totalWritten = 0;

  while (http.connected() && stream->available()) {
    int b = stream->read();
    if (b >= 0) {
      buffer[index++] = (byte)b;

      if (index == bufferSize) {
        while (addr + bufferSize > nextSectorErase + sectorSize) {
          flash.eraseSector(nextSectorErase);
          nextSectorErase += sectorSize;
        }
        flash.writeByteArray(addr, buffer, bufferSize);
        addr += bufferSize;
        totalWritten += bufferSize;
        index = 0;
      }
    }
  }

  if (index > 0) {
    while (addr + index > nextSectorErase + sectorSize) {
      flash.eraseSector(nextSectorErase);
      nextSectorErase += sectorSize;
    }
    flash.writeByteArray(addr, buffer, index);
    addr += index;
    totalWritten += index;
  }

  http.end();
  mp3Size = totalWritten;

  Serial.print("✅ Zapis zakończony. Rozmiar MP3: ");
  Serial.println(mp3Size);

  return true;
}




void testFlashWriteRead() {
  if (flash.begin()) {
  const uint32_t binAddress = 0x002000;
  const size_t binSize = 64;
  byte binData[binSize];
  byte binRead[binSize];

  for (size_t i = 0; i < binSize; i++) {
    binData[i] = i;
  }

  Serial.println("Kasowanie sektora binarnego...");
  flash.eraseSector(binAddress);

  Serial.println("Zapis binarny przez writeByteArray...");
  flash.writeByteArray(binAddress, binData, binSize);

  Serial.println("Odczyt binarny...");
  flash.readByteArray(binAddress, binRead, binSize);

  Serial.println("Porównanie bajtów:");
  bool match = true;
  for (size_t i = 0; i < binSize; i++) {
    Serial.print(binRead[i], HEX);
    Serial.print(" ");
    if (binRead[i] != binData[i]) {
      Serial.print("❌ Błąd na bajcie ");
      Serial.print(i);
      Serial.print(": zapisano ");
      Serial.print(binData[i], HEX);
      Serial.print(", odczytano ");
      Serial.println(binRead[i], HEX);
      match = false;
    }
  }
  Serial.println();
  if (match) {
    Serial.println("✅ Zapis binarny działa!");
  } else {
    Serial.println("❌ Zapis binarny nie działa.");
  }
  }
}





void dumpFlashPreview() {
  Serial.println("Podgląd danych w Flash:");
  for (int i = 0; i < 64; i++) {
    byte b = flash.readByte(flashAddress + i);
    Serial.printf("%02X ", b);
  }
  Serial.println();
}

String readSerialInput() {
  String input = "";
  while (true) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') {
        if (input.length() > 0) {
          return input;  // zwracamy wpisany tekst
        }
      } else {
        input += c;
      }
    }
  }
}

// ===== FUNKCJA URL-ENCODE =====
String urlEncode(const String &str) {
  String encoded = "";
  char c;
  char buf[4];
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum(c)) {
      encoded += c;
    } else if (c == ' ') {
      encoded += '+';
    } else {
      sprintf(buf, "%%%02X", (unsigned char)c);
      encoded += buf;
    }
  }
  return encoded;
}

bool nagrajAudio(const String& filename) {

  // Rozpocznij nagrywanie (void, nie zwraca wartości)
  musicPlayer.startRecordOgg(filename.c_str());

  Serial.println("Nagrywanie trwa 10 sekund...");
  unsigned long start = millis();
  while (millis() - start < 10000) {
    delay(100);
  }

  // Zakończ nagrywanie (void)
  musicPlayer.stopRecordOgg();

  Serial.println("Nagrywanie zakończone i zapisane.");

  return true;
}
