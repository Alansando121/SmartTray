#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <WiFiManager.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <Adafruit_PN532.h>
#include <Adafruit_NeoPixel.h>

// --- Mapeo de Pines ESP32-S3 ---
#define SDA_PIN       8   // SDA nativo ESP32-S3
#define SCL_PIN       9   // SCL nativo ESP32-S3
#define BUZZER_PIN    17  // VCC del Buzzer de 2 pines
#define LED_PIN       19  // Tira de LEDs WS2812B
#define NUM_LEDS      9   // 3 LEDs por cada posición (3 x 3)

// Pines de los 3 Sensores IR
const int irPines[3] = {4, 5, 18};

Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
WebServer server(80);

// --- Estructura para administrar cada Ranura/Objeto ---
struct ObjetoBandeja {
  int id;
  String nombre;
  String uidHex;
  bool registrado;
  int estado; // 0 = RETIRADO, 1 = EN BANDEJA, 4 = ALERTA
};

ObjetoBandeja bandeja[3] = {
  {1, "Ranura 1 (Vacía)", "", false, 0},
  {2, "Ranura 2 (Vacía)", "", false, 0},
  {3, "Ranura 3 (Vacía)", "", false, 0}
};

// Variables globales de escaneo NFC
String ultimoUIDHex = "";
bool hayTagSobreNFC = false;
String bitacora = "";

// --- Control Buzzer 2 Pines (Directo HIGH = Suena) ---
void encenderBuzzer() { digitalWrite(BUZZER_PIN, HIGH); }
void apagarBuzzer() { digitalWrite(BUZZER_PIN, LOW); }
void emitirBip(int duracionMs) {
  encenderBuzzer();
  delay(duracionMs);
  apagarBuzzer();
}

void agregarABitacora(String evento) {
  bitacora = "• " + evento + "<br>" + bitacora;
  Serial.println("📝 BITÁCORA: " + evento);
}

void setLEDsZona(int zona, uint32_t color) {
  int inicio = zona * 3;
  for (int i = inicio; i < inicio + 3; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}

String uidToString(uint8_t* uid, uint8_t len) {
  String str = "";
  for (uint8_t i = 0; i < len; i++) {
    if (uid[i] < 0x10) str += "0";
    str += String(uid[i], HEX);
  }
  str.toUpperCase();
  return str;
}

// --- Operaciones JSON en LittleFS ---
void guardarConfiguracionJSON() {
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.createNestedArray("objetos");

  for (int i = 0; i < 3; i++) {
    JsonObject obj = array.createNestedObject();
    obj["id"] = bandeja[i].id;
    obj["nombre"] = bandeja[i].nombre;
    obj["uid"] = bandeja[i].uidHex;
    obj["registrado"] = bandeja[i].registrado;
  }

  File configFile = LittleFS.open("/config.json", "w");
  if (configFile) {
    serializeJson(doc, configFile);
    configFile.close();
    Serial.println("💾 Configuración guardada en LittleFS.");
  }
}

void cargarConfiguracionJSON() {
  if (!LittleFS.exists("/config.json")) return;

  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) return;

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();

  if (!error) {
    JsonArray array = doc["objetos"];
    for (int i = 0; i < 3 && i < array.size(); i++) {
      bandeja[i].nombre = array[i]["nombre"].as<String>();
      bandeja[i].uidHex = array[i]["uid"].as<String>();
      bandeja[i].registrado = array[i]["registrado"].as<bool>();
    }
    Serial.println("📂 Configuración multi-objeto cargada de LittleFS.");
  }
}

// --- Interfaz Web HTML / CSS / JS ---
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>SmartExit S3 - Multi-Bandeja 3X</title>
  <style>
    body { font-family: 'Segoe UI', sans-serif; background: #121212; color: #fff; text-align: center; padding: 15px; margin: 0; }
    .card { background: #1e1e1e; display: inline-block; padding: 20px; border-radius: 20px; box-shadow: 0 8px 24px rgba(0,0,0,0.5); width: 95%; max-width: 450px; }
    h1 { font-size: 1.5rem; color: #00e676; margin-bottom: 5px; }
    
    .grid-objetos { display: flex; flex-direction: column; gap: 12px; margin: 15px 0; }
    .item-box { background: #2a2a2a; border-radius: 12px; padding: 12px; text-align: left; position: relative; }
    .item-title { font-weight: bold; font-size: 1rem; color: #00e676; }
    .item-status { font-size: 0.85rem; margin-top: 4px; font-weight: bold; }
    .status-0 { color: #2979ff; }
    .status-1 { color: #00e676; }
    .status-4 { color: #ff1744; animation: blink 1s infinite; }
    @keyframes blink { 0% { opacity: 1; } 50% { opacity: 0.3; } 100% { opacity: 1; } }

    .scanner-box { background: #333; padding: 12px; border-radius: 12px; margin-bottom: 15px; }
    input[type=text] { width: 60%; padding: 8px; border-radius: 6px; border: 1px solid #555; background: #1e1e1e; color: #fff; }
    select { padding: 8px; border-radius: 6px; background: #1e1e1e; color: #fff; border: 1px solid #555; }
    .btn { background: #00e676; color: #121212; font-weight: bold; border: none; padding: 8px 12px; border-radius: 6px; cursor: pointer; margin-top: 8px; }
    .btn-danger { background: #ff1744; color: #fff; padding: 4px 8px; font-size: 0.75rem; border-radius: 4px; border: none; cursor: pointer; float: right; }

    .bitacora-box { background: #2a2a2a; border-radius: 12px; padding: 12px; font-size: 0.8rem; color: #ccc; text-align: left; max-height: 120px; overflow-y: auto; margin-top: 15px; }
  </style>
</head>
<body>
  <div class="card">
    <h1>SmartExit Tray (ESP32-S3) 📥</h1>
    <p style="color:#888; font-size:0.8rem;">Monitoreo de 3 Objetos Independientes</p>

    <div class="scanner-box">
      <div style="font-size:0.85rem; color:#aaa;">🔍 Tag sobre NFC: <b id="uid-nfc" style="color:#fff;">Ninguno</b></div>
      <div style="margin-top:8px;">
        <select id="selectSlot">
          <option value="0">Ranura 1</option>
          <option value="1">Ranura 2</option>
          <option value="2">Ranura 3</option>
        </select>
        <input type="text" id="nombreInput" placeholder="Nombre (ej: Cartera)">
      </div>
      <button class="btn" onclick="guardarObjeto()">💾 Guardar / Modificar Objeto</button>
    </div>

    <div class="grid-objetos" id="gridRanuras"></div>

    <div class="bitacora-box">
      <strong>📜 Bitácora de Eventos:</strong>
      <div id="logs" style="margin-top:5px;">Sin movimientos.</div>
    </div>
  </div>

  <script>
    setInterval(function() {
      fetch('/data').then(r => r.json()).then(d => {
        document.getElementById('logs').innerHTML = d.bitacora || "Sin registros.";
        document.getElementById('uid-nfc').innerHTML = d.uidDetectado || "Esperando lectura...";

        var htmlGrid = "";
        d.objetos.forEach((obj, index) => {
          var estadoTxt = "🔵 RETIRADO (EN USO)";
          var estadoClase = "status-0";
          if (obj.estado === 1) { estadoTxt = "🟢 EN BANDEJA"; estadoClase = "status-1"; }
          if (obj.estado === 4) { estadoTxt = "🚨 ¡ALERTA! MOVIMIENTO NO REGISTRADO"; estadoClase = "status-4"; }

          htmlGrid += `
            <div class="item-box">
              <button class="btn-danger" onclick="eliminarObjeto(${index})">🗑️ Borrar</button>
              <div class="item-title">Ranura ${index + 1}: ${obj.nombre}</div>
              <div style="font-size:0.75rem; color:#aaa;">UID: ${obj.uid || "Sin registrar"}</div>
              <div class="item-status ${estadoClase}">${estadoTxt}</div>
            </div>
          `;
        });
        document.getElementById('gridRanuras').innerHTML = htmlGrid;
      });
    }, 500);

    function guardarObjeto() {
      var slot = document.getElementById('selectSlot').value;
      var nom = document.getElementById('nombreInput').value;
      if (!nom) { alert("Escribe un nombre para el objeto."); return; }
      
      fetch(`/guardar?slot=${slot}&nombre=${encodeURIComponent(nom)}`)
        .then(r => r.text()).then(m => alert(m));
    }

    function eliminarObjeto(slot) {
      if (confirm(`¿Deseas borrar la configuración de la Ranura ${slot + 1}?`)) {
        fetch(`/eliminar?slot=${slot}`).then(r => r.text()).then(m => alert(m));
      }
    }
  </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Configurar Pines IR
  for (int i = 0; i < 3; i++) pinMode(irPines[i], INPUT);
  
  // Configurar Buzzer de 2 pines
  pinMode(BUZZER_PIN, OUTPUT);
  apagarBuzzer();

  if (LittleFS.begin(true)) cargarConfiguracionJSON();

  Wire.begin(SDA_PIN, SCL_PIN);
  nfc.begin();
  if (nfc.getFirmwareVersion()) nfc.SAMConfig();

  strip.begin();
  strip.setBrightness(50);

  WiFiManager wm;
  wm.setCaptivePortalEnable(true);
  wm.setConfigPortalTimeout(120);

  if (!wm.autoConnect("SmartExit-Setup")) ESP.restart();

  Serial.print("✅ ESP32-S3 IP Local: http://");
  Serial.println(WiFi.localIP());

  server.on("/", []() { server.send(200, "text/html", index_html); });

  server.on("/data", []() {
    DynamicJsonDocument doc(1024);
    doc["uidDetectado"] = hayTagSobreNFC ? ultimoUIDHex : "Sin lectura";
    doc["bitacora"] = bitacora;

    JsonArray arr = doc.createNestedArray("objetos");
    for (int i = 0; i < 3; i++) {
      JsonObject o = arr.createNestedObject();
      o["nombre"] = bandeja[i].nombre;
      o["uid"] = bandeja[i].uidHex;
      o["estado"] = bandeja[i].estado;
    }

    String jsonStr;
    serializeJson(doc, jsonStr);
    server.send(200, "application/json", jsonStr);
  });

  server.on("/guardar", []() {
    int slot = server.arg("slot").toInt();
    String nombre = server.arg("nombre");

    if (!hayTagSobreNFC) {
      server.send(200, "text/plain", "⚠️ Coloca primero el tag NFC sobre el lector antes de guardar.");
      return;
    }

    for (int i = 0; i < 3; i++) {
      if (i != slot && bandeja[i].registrado && bandeja[i].uidHex == ultimoUIDHex) {
        server.send(200, "text/plain", "❌ ERROR: Este tag NFC ya pertenece a '" + bandeja[i].nombre + "'. Usa un tag diferente.");
        return;
      }
    }

    bandeja[slot].nombre = nombre;
    bandeja[slot].uidHex = ultimoUIDHex;
    bandeja[slot].registrado = true;
    guardarConfiguracionJSON();

    emitirBip(200);
    agregarABitacora("Ranura " + String(slot + 1) + " registrada: " + nombre + " (" + ultimoUIDHex + ")");
    server.send(200, "text/plain", "✅ Objeto '" + nombre + "' asignado a la Ranura " + String(slot + 1));
  });

  server.on("/eliminar", []() {
    int slot = server.arg("slot").toInt();
    String nombreAnterior = bandeja[slot].nombre;
    
    bandeja[slot].nombre = "Ranura " + String(slot + 1) + " (Vacía)";
    bandeja[slot].uidHex = "";
    bandeja[slot].registrado = false;
    bandeja[slot].estado = 0;
    
    guardarConfiguracionJSON();
    agregarABitacora("Ranura " + String(slot + 1) + " liberada (" + nombreAnterior + ")");
    server.send(200, "text/plain", "🗑️ Ranura " + String(slot + 1) + " liberada.");
  });

  server.begin();
  emitirBip(150);
  agregarABitacora("Sistema listo en ESP32-S3 (Buzzer 2 Pines).");
}

void loop() {
  server.handleClient();

  uint8_t uidLeido[7];
  uint8_t lenLeido;
  bool tagPresente = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uidLeido, &lenLeido, 40);

  if (tagPresente) {
    hayTagSobreNFC = true;
    ultimoUIDHex = uidToString(uidLeido, lenLeido);
  } else {
    hayTagSobreNFC = false;
  }

  for (int i = 0; i < 3; i++) {
    int lecturaIR = digitalRead(irPines[i]);

    if (bandeja[i].registrado) {
      if (lecturaIR == LOW) {
        if (bandeja[i].estado == 0) {
          bandeja[i].estado = 1;
          emitirBip(100);
          agregarABitacora(bandeja[i].nombre + " colocado en Ranura " + String(i + 1));
        }
        setLEDsZona(i, strip.Color(0, 255, 0));
      } else {
        if (bandeja[i].estado == 1) {
          bandeja[i].estado = 4;
          agregarABitacora("🚨 ALERTA: " + bandeja[i].nombre + " retirado de Ranura " + String(i + 1) + " sin validación!");
        }

        if (bandeja[i].estado == 4) {
          setLEDsZona(i, strip.Color(255, 0, 0));
        } else {
          setLEDsZona(i, strip.Color(0, 0, 255));
        }
      }

      if (bandeja[i].estado == 4 && hayTagSobreNFC && ultimoUIDHex == bandeja[i].uidHex) {
        bandeja[i].estado = 0;
        emitirBip(250);
        agregarABitacora("Alerta resuelta para " + bandeja[i].nombre + " vía NFC.");
      }
    } else {
      setLEDsZona(i, strip.Color(50, 50, 50));
    }
  }

  delay(50);
}