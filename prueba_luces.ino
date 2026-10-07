#include <Adafruit_NeoPixel.h>

#define LED_PIN  18
#define NUM_LEDS 12  // Cambia este número por la cantidad de LEDs de tu tira

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  Serial.println("--- PRUEBA 4 (US4): Verificación de Tira LED ---");

  strip.begin();
  strip.setBrightness(50); // Brillo al ~20%
  strip.show();
}

void loop() {
  Serial.println("Luces en ROJO");
  colorear(strip.Color(255, 0, 0));
  delay(1500);

  Serial.println("Luces en VERDE");
  colorear(strip.Color(0, 255, 0));
  delay(1500);

  Serial.println("Luces en AZUL");
  colorear(strip.Color(0, 0, 255));
  delay(1500);
}

void colorear(uint32_t color) {
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}