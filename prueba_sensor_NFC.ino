#include <Wire.h>
#include <Adafruit_PN532.h>

// Definición de pines I2C para ESP32 Estándar
#define SDA_PIN 21
#define SCL_PIN 22

Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

void setup() {
  Serial.begin(115200);
  delay(1000); // Pausa para estabilizar la comunicación serie
  Serial.println("\n=============================================");
  Serial.println("   PRUEBA 3 (US3): Lectura NFC con PN532     ");
  Serial.println("=============================================");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("❌ ERROR: No se puede comunicar con el PN532.");
    Serial.println("   -> Revisa que SEL0 esté en 'H' y SEL1 en 'L'.");
    Serial.println("   -> Revisa que el módulo esté alimentado a 5V/VIN.");
    while (1); // Detener el programa si no hay conexión
  }

  // Mostrar versión del chip detectado
  Serial.print("✅ PN532 Encontrado. Firmware v"); 
  Serial.print((versiondata >> 16) & 0xFF, DEC); 
  Serial.print('.'); 
  Serial.println((versiondata >> 8) & 0xFF, DEC);

  // Configurar la placa para leer tarjetas RFID/NFC
  nfc.SAMConfig();

  Serial.println("\n>>> Pasa una tarjeta, llavero o sticker NFC sobre la antena...\n");
}

void loop() {
  boolean lecturaExitosa;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Arreglo donde se guarda el UID
  uint8_t uidLength;                        // Longitud del UID (4 u 7 bytes)

  // Busca una tarjeta ISO14443A durante máximo 100 ms
  lecturaExitosa = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100);

  if (lecturaExitosa) {
    Serial.println("━━━━━ 🏷️ ¡TAG NFC DETECTADO! ━━━━━");
    Serial.print("Longitud del UID: "); 
    Serial.print(uidLength, DEC); 
    Serial.println(" bytes");
    
    Serial.print("UID Hexadecimal:  ");
    for (uint8_t i = 0; i < uidLength; i++) {
      Serial.print(" 0x");
      if (uid[i] < 0x10) Serial.print("0");
      Serial.print(uid[i], HEX);
    }
    Serial.println("\n-----------------------------------\n");
    
    // Pausa para evitar lecturas repetidas continuas mientras sostienes el tag
    delay(1500); 
  }
}