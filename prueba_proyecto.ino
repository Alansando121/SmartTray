#include <Wire.h>
#include <Adafruit_PN532.h>
#include <Adafruit_NeoPixel.h>

#define IR_PIN     4
#define BUZZER_PIN 17
#define LED_PIN    18
#define SDA_PIN    21
#define SCL_PIN    22
#define NUM_LEDS   12

Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// ⚠️ REEMPLAZA ESTE ARREGLO CON EL UID QUE OBTUVISTE EN LA PRUEBA 3
uint8_t uidRegistrado[] = { 0xE1, 0x01, 0xF3, 0x68 }; 
uint8_t longitudUID = sizeof(uidRegistrado);

bool objetoEstaPresente = false;

void setLEDs(uint32_t color) {
  for(int i = 0; i < NUM_LEDS; i++) strip.setPixelColor(i, color);
  strip.show();
}

void setup() {
  Serial.begin(115200);
  pinMode(IR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  strip.begin();
  strip.setBrightness(50);
  setLEDs(strip.Color(255, 0, 0)); // Estado inicial: ROJO (Vacío)

  nfc.begin();
  if (!nfc.getFirmwareVersion()) {
    Serial.println("❌ ERROR: Revisa la conexión del PN532.");
    while (1);
  }
  nfc.SAMConfig();

  Serial.println("=== PRUEBA 5 (US5): Fusión de Sensores Lista ===");
}

void loop() {
  int lecturaIR = digitalRead(IR_PIN);

  // Si hay algo físicamente apoyado en la bandeja
  if (lecturaIR == LOW) {
    uint8_t uidLeido[7];
    uint8_t lenLeido;

    // Escanear etiqueta NFC
    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uidLeido, &lenLeido, 50)) {
      
      // Comprobar si el UID coincide con el registrado
      if (memcmp(uidLeido, uidRegistrado, lenLeido) == 0) {
        if (!objetoEstaPresente) {
          objetoEstaPresente = true;
          Serial.println("✅ Objeto CORRECTO detectado.");
          tone(BUZZER_PIN, 2000, 150); // Pitido de éxito
          setLEDs(strip.Color(0, 255, 0)); // VERDE
        }
      } else {
        Serial.println("⚠️ Objeto DESCONOCIDO (Tag no coincide).");
        setLEDs(strip.Color(255, 255, 0)); // AMARILLO
      }
    }
  } else {
    // Si la bandeja está vacía
    if (objetoEstaPresente) {
      objetoEstaPresente = false;
      Serial.println("🚨 Objeto RETIRADO.");
      tone(BUZZER_PIN, 800, 300); // Pitido de retiro
      setLEDs(strip.Color(255, 0, 0)); // ROJO
    }
  }
  delay(100);
}