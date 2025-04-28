#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Wprowadź dane do Wi-Fi
const char* ssid = "NORA 24";
const char* password = "eloelo520";

// Klucz API OpenAI (lub inny)
const char* openai_api_key = "";  // <- tutaj wklejasz swój klucz API

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Łączenie z Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nPołączono z Wi-Fi!");

  // Możemy spróbować wysłać ping, żeby upewnić się, że internet działa
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Połączono z Wi-Fi!");
  } else {
    Serial.println("Brak połączenia Wi-Fi.");
  }
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
    
    //http.begin("https://api-inference.huggingface.co/models/meta-llama/Llama-3.3-70B-Instruct"); //need pro nawet z kreditsami
    //http.begin("https://api-inference.huggingface.co/models/openchat/openchat_3.5"); //14GB
    //http.begin("https://api-inference.huggingface.co/models/deepseek-ai/DeepSeek-R1-Distill-Qwen-32B"); //nawet rozumie i odpowiada co 3 zdanie ale po polsku mu nie idzie
    //http.begin("https://api-inference.huggingface.co/models/openai-community/gpt2"); //http bład dekodowania
    //http.begin("https://api-inference.huggingface.co/models/openai-community/openai-gpt"); //bład dekodowania
    //http.begin("https://api-inference.huggingface.co/models/distilgpt2"); // odlatuje w kosmos i zaczyna cytować powieści np o gazowaniu żydów a ja tylko powiedziałem "hi my friend"
    //http.begin("https://api-inference.huggingface.co/models/eryk-mazus/polka-1.1b-chat");//blad dekodowania
    //http.begin("https://api-inference.huggingface.co/models/speakleash/Bielik-11B-v2.3-Instruct"); //22GB
    //http.begin("https://api-inference.huggingface.co/models/facebook/mbart-large-50-many-to-many-mmt"); //http11
    //http.begin("https://api-inference.huggingface.co/models/CohereForAI/c4ai-command-r-v01"); //69GB
    http.begin("https://api-inference.huggingface.co/models/mistralai/Mistral-7B-Instruct-v0.3"); //odpowiada po polsku w miarę ale no znowu kredity mi się skonczyły trzeba kolejne multikonto zalożyć
    //http.begin("https://api-inference.huggingface.co/models/CohereForAI/c4ai-command-r-plus-08-2024"); //nie sprawdzałem jeszcze
    //http.begin("https://api-inference.huggingface.co/models/"); //
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + String(openai_api_key));  // Klucz API z Hugging Face
    
    // Ustawienie timeoutu
    http.setTimeout(15000);  // Ustaw timeout na 15 sekund
    
    StaticJsonDocument<512> doc;
    doc["inputs"] = prompt;
    
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
        // Wydobycie odpowiedzi z JSON
        String reply = responseDoc[0]["generated_text"].as<String>();
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
