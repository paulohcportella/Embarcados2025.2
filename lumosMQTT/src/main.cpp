#include <WiFi.h>
#include <PubSubClient.h>

const int PIN_PIR = 27;   // entrada do sensor PIR
const int PIN_LED = 4;    // saída do LED
const int BRILHO_ALTO  = 255;
const int BRILHO_BAIXO = 60;

unsigned long ultimaDeteccao = 0;
const unsigned long tempoPresenca = 5000;   // LED forte por 5s após o último movimento

const int LEDC_CHANNEL = 0;
const int LEDC_FREQ    = 5000;
const int LEDC_BITS    = 8;

// -------- CONFIGURAÇÃO DO WI-FI E MQTT --------
const char* ssid = "uaifai-tiradentes";
const char* password = "bemvindoaocesar";
const char* mqtt_server = "172.26.69.165";  // IP do seu Mac onde o Mosquitto está rodando

WiFiClient espClient;
PubSubClient client(espClient);

// -------- FUNÇÕES DE CONEXÃO --------
void conectarWiFi() {
  Serial.print("Conectando ao Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi conectado!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

void conectarMQTT() {
  client.setServer(mqtt_server, 1883);
  while (!client.connected()) {
    Serial.print("Conectando ao broker MQTT...");
    if (client.connect("ESP32_PIR")) {
      Serial.println("conectado!");
    } else {
      Serial.print("falha, rc=");
      Serial.print(client.state());
      Serial.println(" — tentando novamente em 2s");
      delay(2000);
    }
  }
}

// -------- SETUP --------
void setup() {
  Serial.begin(115200);
  pinMode(PIN_PIR, INPUT);

  ledcSetup(LEDC_CHANNEL, LEDC_FREQ, LEDC_BITS);
  ledcAttachPin(PIN_LED, LEDC_CHANNEL);
  ledcWrite(LEDC_CHANNEL, BRILHO_BAIXO);

  conectarWiFi();
  conectarMQTT();

  Serial.println("Sistema iniciado. Aguardando estabilização do PIR (~20s)...");
  delay(20000);  // tempo de estabilização do PIR
  Serial.println("PIR pronto!");
}

// -------- LOOP PRINCIPAL --------
void loop() {
  if (!client.connected()) conectarMQTT();
  client.loop();

  int movimento = digitalRead(PIN_PIR);

  if (movimento == HIGH) {
    ultimaDeteccao = millis();
    ledcWrite(LEDC_CHANNEL, BRILHO_ALTO);
    unsigned long agora = millis();
    char buffer[20];
    sprintf(buffer, "%lu", agora);
    client.publish("casa/sala/movimento", buffer);
    Serial.println("Movimento detectado → LED forte");
  } else if (millis() - ultimaDeteccao > tempoPresenca) {
    ledcWrite(LEDC_CHANNEL, BRILHO_BAIXO);
    unsigned long agora = millis();
    char buffer[20];
    sprintf(buffer, "%lu", agora);
    client.publish("casa/sala/movimento", buffer);
    Serial.println("Sem movimento → LED fraco");
  }

  delay(300);
}