#define BUZZER_PIN 17

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  Serial.println("--- PRUEBA 1 (US1): Verificación de Buzzer ---");
}

void loop() {
  Serial.println("Emitiendo pitido corto (Confirmación)...");
  tone(BUZZER_PIN, 2000, 150); // Tone de 2000 Hz por 150 ms
  delay(1000);

  Serial.println("Emitiendo pitido grave (Alerta)...");
  tone(BUZZER_PIN, 800, 300);  // Tone de 800 Hz por 300 ms
  delay(2000);
}