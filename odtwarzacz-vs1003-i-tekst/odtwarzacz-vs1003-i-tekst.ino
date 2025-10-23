#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_VS1053.h>


// KONFIGURACJA WI-FI
const char* ssid = "NORA 24";
const char* password = "eloelo520";
const char* host = "api.streamelements.com";
const int httpsPort = 443;
// Piny VS1053
//in mic out glośnik jak podpięcie według schematu z tymi dolnymi tak jak jest
#define VS1053_RESET  15
#define VS1053_CS     32
#define VS1053_DCS    33
#define CARDCS         5  // SD card CS
#define VS1053_DREQ    35
#define MOSI 23
#define MISO 19
#define SCK  18
#define CARD         5

Adafruit_VS1053_FilePlayer musicPlayer =
  Adafruit_VS1053_FilePlayer(MOSI, MISO, SCK, VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);

void setup() {
  Serial.begin(115200);
  delay(500);
  // Połączenie z Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Łączenie z WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nPołączono z WiFi!");

  if (!musicPlayer.begin()) {
    Serial.println("Nie znaleziono VS1053");
    while (1);
  }
  musicPlayer.setVolume(10, 10);
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);
  testGlosnika ();
 delay(1000);
 //SPIClass spiSD(HSPI);  // użyj HSPI
 //spiSD.begin(14, 12, 13, CARD);
 if (!SD.begin(CARD)) {
   Serial.println("Błąd SD!");
   while (1);
 }



  // if (nagrajAudio("/mp3/test.ogg")) {
  //   Serial.println("Plik nagrany poprawnie.");
  // } else {
  //   Serial.println("Problem z nagraniem pliku.");
  // }
}

void loop() {
  Serial.println("Podaj tekst do przeczytania: ");
  testGlosnika ();
  String userText = readSerialInput();
  Serial.print("Wczytano: ");
  Serial.println(userText);

  if(fetchSpeechToFile(userText, "/mp3/speach.mp3")){
    musicPlayer.playFullFile("/mp3/speach.mp3");
    while(musicPlayer.playingMusic){
      delay(100);
    }
  }
}

void testGlosnika () {
  Serial.println("🔊 Generuję pipnięcie...");
  musicPlayer.sineTest(0x44, 1000);  // 0x44 = ~1kHz, 500ms
  Serial.println("✅ Pipnięcie zakończone.");
}

// ==== FUNKCJA DO TTS ====
bool fetchSpeechToFile(const String& text, const String& filename) {
  WiFiClientSecure client;
  client.setInsecure();
  musicPlayer.stopPlaying();  // zatrzymaj VS1053
  delay(100);

  String safeText = text;
  if (safeText.length() > 300) {
    Serial.println("⚠️ Tekst za długi, skracam.");
    safeText = safeText.substring(0, 300);
  }

  String url = "/kappa/v2/speech?voice=pl-PL-Wavenet-A&text=" + urlEncode(safeText);
  Serial.println("🔽 Pobieram MP3 dla: " + safeText);

  if (!client.connect(host, httpsPort)) {
    Serial.println("❌ Brak połączenia z serwerem.");
    return false;
  }

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break;
  }

  if (SD.exists(filename.c_str())) {
    SD.remove(filename.c_str());
  }

  File file = SD.open(filename.c_str(), FILE_WRITE);
  if (!file) {
    Serial.println("❌ Nie można otworzyć pliku do zapisu.");
    return false;
  }

  uint8_t buffer[128];
  unsigned long start = millis();
  while (client.available()) {
    size_t len = client.read(buffer, sizeof(buffer));
    if (len > 0) {
      file.write(buffer, len);
    }
    if (millis() - start > 5000) {
      Serial.println("⚠️ Timeout pobierania.");
      break;
    }
  }

  file.close();
  client.stop();
  Serial.println("✅ Pobieranie zakończone.");
  return true;
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
