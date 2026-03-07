#include <WiFi.h>
#include <WiFiUdp.h>
#include <WiFiManager.h>

const char* ssid     = "ALTO-HARD-LEO";
const char* password = "244466666";

IPAddress broadcastIP(255, 255, 255, 255);

const int udpLocalPort  = 8888;   // donde escucha el módulo
const int udpRemotePort = 9999;   // donde AOG escucha (y envía broadcast)

WiFiUDP udp;

#define NUM_RELAYS 3
const uint8_t relayPins[NUM_RELAYS] = {25, 26, 27};   // ← cambia según tus pines
uint8_t  relayLastState = 10;
#define PIN_AUTO    16
#define PIN_MASTER  17

bool autoMode       = true;     // true = AUTO, false = MANUAL
bool masterSections = true;     // Master switch global
bool lastAutoState   = HIGH;
bool lastMasterState = HIGH;

uint8_t sectionLo = 0;   // secciones 0..7
uint8_t sectionHi = 0;   // secciones 8..15 (no usas por ahora)

uint8_t onLo = 0, offLo = 0xFF;   // para informar switches estado a AOG
uint8_t onHi = 0, offHi = 0xFF;

uint32_t lastSend     = 0;
uint32_t lastHello    = 0;
uint8_t  watchdog     = 20;      // si > ~10-15 → desconectado

byte buffer[32];

// ────────────────────────────────────────────────
void checkButtons() {
  bool autoState   = digitalRead(PIN_AUTO);
  bool masterState = digitalRead(PIN_MASTER);

  if (autoState == LOW && lastAutoState == HIGH) {   // flanco descendente
    autoMode = !autoMode;
    Serial.println(autoMode ? "→ MODO AUTO" : "→ MODO MANUAL");
    delay(50); // anti rebote básico
  }

  if (masterState == LOW && lastMasterState == HIGH) {
    masterSections = !masterSections;
    Serial.println(masterSections ? "MASTER ON" : "MASTER OFF");
    delay(50);
  }

  lastAutoState   = autoState;
  lastMasterState = masterState;
}

void updateRelays() {
  uint8_t effectiveLo = sectionLo;

  if (!masterSections) {
    effectiveLo = 0;
  }
  // Solo usamos las primeras 3 secciones por ahora
  for (int i = 0; i < NUM_RELAYS; i++) {
    bool shouldOn = bitRead(effectiveLo, i);
    
    digitalWrite(relayPins[i], shouldOn ? HIGH : LOW);
  }
    if (relayLastState != sectionLo) {
      Serial.print("Relay "); 
      Serial.println(sectionLo, BIN);
      relayLastState = sectionLo;
    }

}

void sendHello() {
  byte hello[] = {0x80, 0x81, 0x7B, 0x7B, 5, 0,0,0,0,0, 71};
  byte ck = 0;
  for (int i = 2; i < sizeof(hello)-1; i++) ck += hello[i];
  hello[sizeof(hello)-1] = ck;

  udp.beginPacket(broadcastIP, udpRemotePort);
  udp.write(hello, sizeof(hello));
  udp.endPacket();
}

void sendScanReply() {
  byte reply[] = {0x80,0x81,0x7B,203,7, 0,0,0,0, 0,0,0,0, 0};
  reply[5] = WiFi.localIP()[0];
  reply[6] = WiFi.localIP()[1];
  reply[7] = WiFi.localIP()[2];
  reply[8] = WiFi.localIP()[3];
  reply[9] = WiFi.localIP()[0];
  reply[10]= WiFi.localIP()[1];
  reply[11]= WiFi.localIP()[2];
  reply[12]= 23;   // puerto ejemplo

  byte ck = 0;
  for (int i = 2; i < sizeof(reply)-1; i++) ck += reply[i];
  reply[sizeof(reply)-1] = ck;

  udp.beginPacket(broadcastIP, udpRemotePort);
  udp.write(reply, sizeof(reply));
  udp.endPacket();
}

void sendToAOG() {
  byte msg[14] = {0x80, 0x81, 0x7B, 0xEA, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  // mainByte: 1 = AUTO, 2 = MANUAL
  msg[5] = autoMode ? 1 : 2;

  // onLo / offLo: reportamos el estado del master como override
  uint8_t mask = (1 << NUM_RELAYS) - 1;  // 0b00000111 para 3 relés

  if (!masterSections) {
    // Master OFF → forzamos TODAS las secciones OFF (independientemente del modo)
    onLo  = 0x00;
    offLo = mask;          // bits en 1 = forzadas OFF
    onHi  = 0x00;
    offHi = 0xFF;          // si tuvieras más secciones
  } else {
    // Master ON → no hay override manual (AOG controla normalmente)
    onLo  = 0x00;
    offLo = 0x00;
    onHi  = 0x00;
    offHi = 0x00;
  }

  // Si estás en MANUAL y master ON → opcional: reportar todas ON
  if (!autoMode && masterSections) {
    onLo  = mask;
    offLo = 0x00;
  }

  msg[9]  = onLo;
  msg[10] = offLo;
  msg[11] = onHi;
  msg[12] = offHi;

  byte ck = 0;
  for (int i = 2; i < 13; i++) ck += msg[i];
  msg[13] = ck;

  udp.beginPacket(broadcastIP, udpRemotePort);
  udp.write(msg, 14);
  udp.endPacket();
}

void setup() {
  Serial.begin(115200);
  delay(100);

  for (int i = 0; i < NUM_RELAYS; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], LOW);   // apagados al inicio
  }

  pinMode(PIN_AUTO,   INPUT_PULLDOWN);
  pinMode(PIN_MASTER, INPUT_PULLDOWN);
  //Configuracion de AGO
  WiFiManager wm; 
  bool res;
   res = wm.autoConnect("SectionAGO");

    if(!res) {
        Serial.println("Failed to connect");
        ESP.restart();
    } 
    else {
        //if you get here you have connected to the WiFi    
        Serial.println("connected...yeey :)");
    }
 

  Serial.println("\nConectado! IP: " + WiFi.localIP().toString());
  
  broadcastIP = WiFi.localIP();
  broadcastIP[3] = 255;
  Serial.print("Broadcast: "); Serial.println(broadcastIP);

  udp.begin(udpLocalPort);
  Serial.println("UDP escuchando en puerto 8888");

  // Primer hello
  sendHello();
}

void loop() {
  checkButtons();

  // Recibir paquetes UDP
  int packetSize = udp.parsePacket();
  if (packetSize > 0 && packetSize < sizeof(buffer)) {
    int len = udp.read(buffer, sizeof(buffer));
    if (len >= 5 && buffer[0] == 0x80 && buffer[1] == 0x81 && buffer[2] == 0x7F) {
      
      uint8_t pgn = buffer[3];
      // Serial.print("PGN: "); Serial.println(pgn);

      if (pgn == 202) {           // scan request
        sendScanReply();
      }
      else if (pgn == 239) {      // machine data → comandos de secciones
        if (autoMode){//en modo auto solo lee lo que viene 
        // hydLift   = buffer[7];
        // tramline  = buffer[8];
        // geoStop   = buffer[9];
        sectionLo = buffer[11];

        sectionHi = buffer[12];

        watchdog = 0;             // reset watchdog
      //Serial.print("Secciones recibidas Lo: "); Serial.println(sectionLo, BIN);
        }
    }
      else if (pgn == 200) {      // hello from AgIO
        sendHello();
      }
    }
  }

  // Watchdog – si no llega nada en ~2 segundos → apagar todo
  watchdog++;
  if (watchdog > 400) {
    sectionLo = sectionHi = 0;
    masterSections = false;   // o mantener último estado, según prefieras
    Serial.println(watchdog);
  }

  // Enviar hello cada ~1 segundo
  if (millis() - lastHello > 1000) {
    sendHello();
    lastHello = millis();
  }


  // Enviar estado a AOG cada ~200 ms (como el código original)
  if (millis() - lastSend > 200) {
    sendToAOG();
    lastSend = millis();
  }

  // Aplicar relés
  updateRelays();
}