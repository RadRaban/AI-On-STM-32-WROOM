#include <SPI.h>
#include <SPIMemory.h>

SPIFlash flash(5); // GPIO5 jako CS



void setup() {
  Serial.begin(115200);
  testBinaryWrite();
  testBinaryWrite();
}

void loop() {
  // nic
}


void testBinaryWrite() {
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