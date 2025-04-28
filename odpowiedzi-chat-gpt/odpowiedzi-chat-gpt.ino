#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Wprowadź dane do Wi-Fi
const char* ssid = "NORA 24";
const char* password = "eloelo520";

// Klucz API OpenAI (lub inny)
const char* openai_api_key = "";  // <- tutaj wklejasz swój klucz

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Łączenie z Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nPołączono z Wi-Fi!");
}

void loop() {
  if (Serial.available() > 0) {
    String userInput = Serial.readStringUntil('\n');
    userInput.trim();

    if (userInput.length() > 0) {
      String response = askChat(userInput);
      Serial.println("Odpowiedź AI:");
      Serial.println(response);
    }
  }
}

String askChat(String prompt) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    // Zmieniamy URL na OpenAI ChatGPT API
    http.begin("https://api.openai.com/v1/chat/completions");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + String(openai_api_key));  // Klucz API
    
    // Przygotowanie zapytania JSON dla OpenAI ChatGPT
    StaticJsonDocument<512> doc;
    doc["model"] = "gpt-3.5-turbo";  // Wybór modelu ChatGPT (np. gpt-3.5-turbo, gpt-4)
    JsonArray messages = doc.createNestedArray("messages");
    JsonObject message = messages.createNestedObject();
    message["role"] = "user";
    message["content"] = prompt;  // Pytanie od użytkownika
    
    String requestBody;
    serializeJson(doc, requestBody);
    
    int httpResponseCode = http.POST(requestBody);

    if (httpResponseCode > 0) {
      String payload = http.getString();
      Serial.println("RAW odpowiedź serwera:");
      Serial.println(payload);

      StaticJsonDocument<2048> responseDoc;
      DeserializationError error = deserializeJson(responseDoc, payload);

      if (!error) {
        // Odbieramy odpowiedź od ChatGPT
        String reply = responseDoc["choices"][0]["message"]["content"].as<String>();
        http.end();
        return reply;
      } else {
        http.end();
        return "Błąd dekodowania JSON.";
      }
    } else {
      http.end();
      return "Błąd połączenia HTTP: " + String(httpResponseCode);
    }
  }
  return "Brak połączenia Wi-Fi.";
}
