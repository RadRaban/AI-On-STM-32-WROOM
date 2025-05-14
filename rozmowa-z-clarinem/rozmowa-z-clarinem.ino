#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "NORA 24";
const char* password = "eloelo520";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nPołączono z WiFi!");
  Serial.println("Wpisz wiadomość do modelu Bielik i naciśnij ENTER:");
}

void loop() {
  if (Serial.available()) {
    String userInput = Serial.readStringUntil('\n');
    userInput.trim(); // usuń spacje i nową linię

    if (userInput.length() == 0) return;

    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin("https://services.clarin-pl.eu/api/v1/oapi/chat/completions");
      http.addHeader("Authorization", "Bearer Zc5hXjQNGnRy64T4wF9QQz9HnNG5KBtYzZNnvK_lQOedR_vm");
      http.addHeader("Content-Type", "application/json");
      http.addHeader("Accept", "application/json");
      http.setTimeout(10000); // 10 sekund na odpowiedź
      http.setReuse(false);

      // Budujemy JSON bezpiecznie
      StaticJsonDocument<1024> requestJson; // można zwiększyć jeśli potrzebne
      requestJson["model"] = "bielik";
      requestJson["max_tokens"] = 100; // lub np. 150
      JsonArray messages = requestJson.createNestedArray("messages");
      JsonObject msg = messages.createNestedObject();
      msg["role"] = "user";
      msg["content"] = userInput;

      String requestBody;
      serializeJson(requestJson, requestBody);  // zamienia JSON na String

      int httpResponseCode = http.POST(requestBody);

      if (httpResponseCode > 0) {
        String response = http.getString();

        // Większy bufor dla dużych odpowiedzi
        DynamicJsonDocument doc(6144); 
        DeserializationError error = deserializeJson(doc, response);

        if (!error) {
          const char* message = doc["choices"][0]["message"]["content"];
          Serial.println("\n🤖 Odpowiedź modelu:");
          Serial.println(message);
          Serial.println("\n🔁 Wpisz kolejne pytanie:");
        } else {
          Serial.println("❌ Błąd parsowania JSON-a!");
        }

      } else {
        Serial.print("❌ Błąd POST: ");
        Serial.println(httpResponseCode);
      }

      http.end();
    } else {
      Serial.println("❌ Brak połączenia WiFi");
    }
  }
}

