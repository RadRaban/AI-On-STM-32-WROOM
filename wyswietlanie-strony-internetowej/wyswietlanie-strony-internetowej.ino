#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "NORA 24";
const char* password = "eloelo520";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Łączenie z WiFi...");
  }
  Serial.println("Połączono!");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("https://chatgpt.com");  // <-- tutaj dajesz swoją stronę
    int httpCode = http.GET();

    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println(payload);  // Wypisuje kod HTML na Serial Monitor
    } else {
      Serial.println("Błąd połączenia");
    }
    http.end();
  } else {
    Serial.println("Brak połączenia WiFi");
  }

  delay(5000);  // <-- opóźnienie 5 sekund przed kolejnym pobraniem
}
