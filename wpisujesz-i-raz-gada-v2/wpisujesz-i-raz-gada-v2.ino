#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <SD.h>
#include <Audio.h>

const char* ssid = "NORA 24";
const char* password = "eloelo520";

#define SD_CS 5  // lub Twój CS, np. D5

#define I2S_DOUT 27
#define I2S_BCLK 26
#define I2S_LRC  25

Audio audio;
WiFiClientSecure client;

const char* host = "api.streamelements.com";
const int httpsPort = 443;

int fileIndex = 1;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Łączenie z WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Połączono!");

  if (!SD.begin(SD_CS)) {
    Serial.println("Nie można zainicjować SD!");
    while (1);
  }

  if (!SD.exists("/mp3")) {
    SD.mkdir("/mp3");
  }

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(8);

  Serial.println("Wpisz tekst do przeczytania:");
}

void loop() {
  static String input = "";

  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (input.length() > 0) {
        Serial.println("Pobieranie...");
        if (downloadMP3(input)) {
          Serial.println("Odtwarzanie...");
        } else {
          Serial.println("Błąd pobierania.");
        }
        input = "";
        Serial.println("Wpisz kolejny tekst:");
      }
    } else {
      input += c;
    }
  }

  audio.loop();
}

bool downloadMP3(const String& text) {
  client.stop();
  client.setInsecure();

  String url = "/kappa/v2/speech?voice=pl-PL-Wavenet-A&text=" + urlEncode(text);
  if (!client.connect(host, httpsPort)) {
    Serial.println("Błąd połączenia z serwerem.");
    return false;
  }

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break;
  }

  String filename = "/mp3/" + zeroPad(fileIndex++, 4) + ".mp3";
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("Błąd otwarcia pliku do zapisu.");
    return false;
  }

  while (client.available()) {
    file.write(client.read());
  }
  file.close();
  client.stop();

  if (SD.exists(filename)) {
    audio.connecttoFS(SD, filename.c_str());
    return true;
  }
  return false;
}

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

String zeroPad(int num, int width) {
  String out = String(num);
  while (out.length() < width) {
    out = "0" + out;
  }
  return out;
}
