#define IR_PIN 4

void setup() {
  Serial.begin(115200);
  pinMode(IR_PIN, INPUT);
  Serial.println("--- PRUEBA 2 (US2): Verificación de Sensor IR ---");
}

void loop() {
  int estadoIR = digitalRead(IR_PIN);

  if (estadoIR == LOW) {
    Serial.println("🟢 OBJETO DETECTADO (Sensor tapado)");
  } else {
    Serial.println("⚪ ZONA VACÍA (Sensor libre)");
  }
  delay(300);
}