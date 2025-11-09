#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// ===== Globais =====
const char* SSID     = "Wokwi-GUEST";
const char* PASSWORD = "";

const int PIN_LED    = 15;
const int PIN_BUZZER = 12;

WebServer server(80);

// Página HTML
const char* INDEX_HTML = R"HTML(
<!doctype html>
<html lang="pt-br">
<head>
  <meta charset="utf-8"/>
  <meta name="viewport" content="width=device-width,initial-scale=1"/>
  <title>Controle IoT - LED & Buzzer</title>
  <style>
    body{font-family:system-ui,-apple-system,Segoe UI,Roboto,Ubuntu;max-width:520px;margin:40px auto;padding:0 16px}
    h1{font-size:1.25rem}
    .card{border:1px solid #ddd;border-radius:12px;padding:16px}
    button{font-size:1rem;border:0;border-radius:10px;padding:12px 18px;cursor:pointer}
    .primary{background:#111;color:#fff}
    .row{display:flex;gap:12px;align-items:center}
    #status{margin-top:12px;color:#555}
  </style>
</head>
<body>
  <h1>Controle IoT - LED & Buzzer (ESP32)</h1>
  <div class="card">
    <p>Clique no botão para acionar o <b>buzzer</b> e fazer o <b>LED</b> piscar por ~1,5s.</p>
    <div class="row">
      <button class="primary" id="btn-alarme">Acionar Alarme</button>
      <span id="status">Pronto</span>
    </div>
  </div>

  <script>
    const btn = document.getElementById('btn-alarme');
    const statusEl = document.getElementById('status');

    async function acionar() {
      try {
        statusEl.textContent = 'Disparando...';
        const resp = await fetch('/alarme', {method: 'POST'});
        if (!resp.ok) throw new Error('Falha HTTP');
        const data = await resp.json();
        statusEl.textContent = data?.message ?? 'OK';
      } catch (e) {
        statusEl.textContent = 'Erro ao acionar';
      }
    }

    btn.addEventListener('click', acionar);
  </script>
</body>
</html>
)HTML";

// ===== Estado =====
bool     alarmActive = false;
uint32_t alarmUntil  = 0;
uint32_t ledToggleAt = 0;
bool     ledOn       = false;

// ===== Protótipos (necessário no .cpp) =====
void connectWiFi();
void handleRoot();
void handleAlarme();

// ===== Implementações =====
void connectWiFi() {
  Serial.println("Conectando ao WiFi…");
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
    Serial.print(".");
    delay(250);
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi conectado! IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Falha ao conectar (timeout).");
  }
}

void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
}

void handleAlarme() {
  alarmActive = true;
  alarmUntil  = millis() + 1500;
  ledToggleAt = 0;
  ledOn       = false;
  tone(PIN_BUZZER, 1000);
  server.send(200, "application/json",
              "{\"ok\":true,\"message\":\"Alarme acionado\"}");
}

void setup() {
  Serial.begin(115200);
  delay(150);
  Serial.println("Boot...");

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  pinMode(PIN_BUZZER, OUTPUT);
  noTone(PIN_BUZZER);

  connectWiFi();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/alarme", HTTP_POST, handleAlarme);
  server.onNotFound([](){ server.send(404, "text/plain", "404 - Not Found"); });
  server.begin();
  Serial.println("HTTP server iniciado");
}

void loop() {
  server.handleClient();
  uint32_t now = millis();

  if (alarmActive) {
    if (now >= ledToggleAt) {
      ledToggleAt = now + 120;
      ledOn = !ledOn;
      digitalWrite(PIN_LED, ledOn ? HIGH : LOW);
    }
    if (now >= alarmUntil) {
      alarmActive = false;
      digitalWrite(PIN_LED, LOW);
      noTone(PIN_BUZZER);
    }
  }
}
