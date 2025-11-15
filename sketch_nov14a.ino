#include <WiFiNINA.h>
#include <Arduino_MKRIoTCarrier.h>
#include "secrets.h"            // contient: const char* tsApiKey = "..."
#include <MQTTPubSubClient.h>

// --- Adafruit IO ---
#define IO_USERNAME  "cedric3iL"

// Feeds MQTT (adapte "humidity"/"pressure" si tu as mis d'autres noms de feeds)
#define IO_FEED_TEMP_MQTT  "cedric3iL/feeds/temperature"
#define IO_FEED_HUM_MQTT   "cedric3iL/feeds/humidity"
#define IO_FEED_PRESS_MQTT "cedric3iL/feeds/pressure"

MKRIoTCarrier carrier;

// --- Wi-Fi ---
char ssid[] = "Houston";
char pass[] = "cedric10";

int status = WL_IDLE_STATUS;
IPAddress ipLocal;                 // IP locale de la carte

// --- ThingSpeak ---
WiFiClient tsClient;                    // client TCP pour ThingSpeak
const char* tsServer = "api.thingspeak.com";
unsigned long lastTS = 0;
const unsigned long TS_PERIOD_MS = 20000;   // 20 s

// --- MQTT Adafruit IO ---
WiFiClient mqttWiFiClient;      // client TCP pour Adafruit IO
MQTTPubSubClient mqttClient;    // client MQTT

// --- LEDs ---
bool ledsOn = false;
unsigned long lastToggle = 0;
const unsigned long PERIOD_MS = 500;

// --- Capteurs / affichage ---
const float ALTITUDE_M = 120.0;
float fTemp, fHum, fPress_hPa, fPressSL_hPa;

unsigned long lastUi = 0;
const unsigned long UI_UPDATE_MS = 500;

// ------------------------------------------------------
// Remet les 5 couleurs des LEDs
// ------------------------------------------------------
void writeColors() {
  carrier.leds.setPixelColor(0, carrier.leds.Color(255, 0, 0));
  carrier.leds.setPixelColor(1, carrier.leds.Color(0, 255, 0));
  carrier.leds.setPixelColor(2, carrier.leds.Color(0, 0, 255));
  carrier.leds.setPixelColor(3, carrier.leds.Color(255, 255, 0));
  carrier.leds.setPixelColor(4, carrier.leds.Color(255, 0, 255));
}

// ------------------------------------------------------
// Envoi HTTP vers ThingSpeak
// field1 = temp, field2 = pression, field3 = humidité
// ------------------------------------------------------
bool httpRequest(float tempC, float press_hPa, float humPct) {
  String data = String("field1=") + String(tempC, 2) +
                "&field2=" + String(press_hPa, 2) +
                "&field3=" + String(humPct, 1);

  // Connexion TCP vers ThingSpeak (port 80)
  if (!tsClient.connect(tsServer, 80)) {
    Serial.println("ThingSpeak: connect() a echoue");
    return false;
  }

  // En-têtes HTTP
  tsClient.println("POST /update HTTP/1.1");
  tsClient.println("Host: api.thingspeak.com");
  tsClient.println("Connection: close");
  tsClient.println("User-Agent: ArduinoWiFi/1.1");
  tsClient.print  ("X-THINGSPEAKAPIKEY: ");
  tsClient.println(tsApiKey);
  tsClient.println("Content-Type: application/x-www-form-urlencoded");
  tsClient.print  ("Content-Length: ");
  tsClient.println(data.length());
  tsClient.println();              // ligne vide

  // Corps de la requête
  tsClient.print(data);

  // Lecture rapide de la réponse
  unsigned long t0 = millis();
  String lastLine;
  while (tsClient.connected() && millis() - t0 < 3000) {
    while (tsClient.available()) {
      lastLine = tsClient.readStringUntil('\n');
      // Serial.println(lastLine);
    }
  }
  tsClient.stop();

  // Si la dernière ligne est un entier > 0, update OK
  long id = lastLine.toInt();
  if (id > 0) {
    Serial.print("ThingSpeak: update id=");
    Serial.println(id);
    return true;
  } else {
    Serial.println("ThingSpeak: reponse non valide / update KO");
    return false;
  }
}

// ------------------------------------------------------
// Affichage MAC
// ------------------------------------------------------
void printMacAddress(byte mac[]) {
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) Serial.print("0");
    Serial.print(mac[i], HEX);
    if (i > 0) Serial.print(":");
  }
  Serial.println();
}

// ------------------------------------------------------
// Affichage infos réseau
// ------------------------------------------------------
void printCurrentNet() {
  Serial.print("SSID : ");  Serial.println(WiFi.SSID());
  byte bssid[6]; WiFi.BSSID(bssid);
  Serial.print("BSSID : "); printMacAddress(bssid);
  Serial.print("Encryption Type : "); Serial.println(WiFi.encryptionType(), HEX);
  Serial.print("Signal Strength (RSSI) : "); Serial.print(WiFi.RSSI()); Serial.println(" dBm\n");
}

// ------------------------------------------------------
// SETUP
// ------------------------------------------------------
void setup() {
  carrier.begin();

  // Buzzer 1 s
  carrier.Buzzer.sound(500);
  delay(1000);
  carrier.Buzzer.noSound();

  // Série
  Serial.begin(9600);
  while (!Serial) { }
  Serial.println("Initialisation Wi-Fi...");

  // Écran "Connexion..."
  carrier.display.fillScreen(ST77XX_RED);
  carrier.display.setTextColor(ST77XX_WHITE);
  carrier.display.setTextSize(2);
  carrier.display.setCursor(40, 110);
  carrier.display.print("Connexion...");

  // Vérifs module WiFi
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("ERREUR : Module WiFi non detecte !");
    while (true) {}
  }
  if (WiFi.firmwareVersion() < String("1.0.0")) {
    Serial.println("Avertissement : mettez a jour le firmware NINA !");
  }

  // Connexion WiFi
  while (status != WL_CONNECTED) {
    Serial.print("Tentative de connexion au reseau : ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(10000);
  }

  // Connecté : IP + infos
  ipLocal = WiFi.localIP();
  Serial.println("\nConnexion Wi-Fi reussie !");
  Serial.print("Adresse IP : ");
  Serial.println(ipLocal);
  printCurrentNet();

  // --- CONNEXION TCP VERS BROKER MQTT (Adafruit IO) ---
  Serial.println("Connexion TCP au broker MQTT (Adafruit IO)...");
  if (!mqttWiFiClient.connect("io.adafruit.com", 1883)) {
    Serial.println("ERREUR : connexion TCP au broker echouee !");
    while (true) {
      // blocage comme demande dans l'énoncé
    }
  }
  Serial.println("TCP OK vers io.adafruit.com:1883");

  // --- INITIALISATION CLIENT MQTT ---
  mqttClient.begin(mqttWiFiClient);

  Serial.println("Connexion MQTT a Adafruit IO...");
  // Boucle tant que la connexion MQTT n'est pas réalisée
  while (!mqttClient.connect("MKR-IoT-Device", IO_USERNAME, IO_KEY)) {
    Serial.println("Echec connexion MQTT, nouvel essai dans 1s...");
    delay(1000);
  }
  Serial.println("Broker connecté");

  // Écran de bienvenue + IP
  carrier.display.fillScreen(ST77XX_RED);
  carrier.display.setTextColor(ST77XX_WHITE);
  carrier.display.setTextSize(2);
  carrier.display.setCursor(40, 90);
  carrier.display.print("Bienvenue !");
  carrier.display.setCursor(20, 130);
  carrier.display.print("IP: ");
  carrier.display.print(ipLocal);
  delay(3000);

  // --- TEST DE PING INTERNET ---
  char site[] = "www.google.com";
  int rtt = WiFi.ping(site);

  if (rtt >= 0) {
    Serial.print("Ping OK vers ");
    Serial.print(site);
    Serial.print(" : ");
    Serial.print(rtt);
    Serial.println(" ms");

    carrier.display.fillScreen(ST77XX_GREEN);
    carrier.display.setTextColor(ST77XX_BLACK);
    carrier.display.setTextSize(2);
    carrier.display.setCursor(20, 100);
    carrier.display.print("Ping Google OK ");
    carrier.display.print(rtt);
    carrier.display.print("ms");
  } else {
    Serial.print("Ping echoue vers ");
    Serial.println(site);

    carrier.display.fillScreen(ST77XX_RED);
    carrier.display.setTextColor(ST77XX_WHITE);
    carrier.display.setTextSize(2);
    carrier.display.setCursor(20, 100);
    carrier.display.print("Ping echoue!");
  }
  delay(3000);
  // --- FIN PING ---

  // Prépare écran pour valeurs
  carrier.display.fillScreen(ST77XX_BLACK);
  carrier.display.setTextColor(ST77XX_WHITE);
  carrier.display.setTextSize(2);

  // LEDs : démarrage
  writeColors();
  carrier.leds.show();
  ledsOn = true;
  lastToggle = millis();
}

// ------------------------------------------------------
// LOOP
// ------------------------------------------------------
void loop() {
  unsigned long now = millis();

  // --- Clignotement LEDs ---
  if (now - lastToggle >= PERIOD_MS) {
    lastToggle = now;
    if (ledsOn) {
      carrier.leds.clear();
      carrier.leds.show();
    } else {
      writeColors();
      carrier.leds.show();
    }
    ledsOn = !ledsOn;
  }

  // --- Mise à jour capteurs + affichage ---
  if (now - lastUi >= UI_UPDATE_MS) {
    lastUi = now;

    fTemp       = carrier.Env.readTemperature();
    fHum        = carrier.Env.readHumidity();
    fPress_hPa  = carrier.Pressure.readPressure();
    fPressSL_hPa = fPress_hPa + (ALTITUDE_M / 8.0);  // correction altitude

    carrier.display.fillScreen(ST77XX_BLACK);

    carrier.display.setCursor(20, 60);
    carrier.display.print("Temp : ");
    carrier.display.print(fTemp, 2);
    carrier.display.print(" \xB0C");

    carrier.display.setCursor(20, 100);
    carrier.display.print("Hum  : ");
    carrier.display.print(fHum, 1);
    carrier.display.print(" %");

    carrier.display.setCursor(20, 140);
    carrier.display.print("Press: ");
    carrier.display.print(fPressSL_hPa, 1);
    carrier.display.print(" hPa");
  }

  // --- Envoi périodique vers ThingSpeak + MQTT Adafruit ---
  if (now - lastTS >= TS_PERIOD_MS) {
    lastTS = now;

    // 1) HTTP vers ThingSpeak (temp, pression, humidité)
    bool okTS = httpRequest(fTemp, fPressSL_hPa, fHum);
    if (okTS) {
      Serial.println("POST ThingSpeak: OK");
    } else {
      Serial.println("POST ThingSpeak: ECHEC");
    }

    // 2) MQTT vers Adafruit IO (Temp, Hum, Press)
    String tempPayload  = String(fTemp, 2);
    String humPayload   = String(fHum, 1);
    String pressPayload = String(fPressSL_hPa, 1);

    bool okMQTT_T = mqttClient.publish(IO_FEED_TEMP_MQTT,  tempPayload);
    bool okMQTT_H = mqttClient.publish(IO_FEED_HUM_MQTT,   humPayload);
    bool okMQTT_P = mqttClient.publish(IO_FEED_PRESS_MQTT, pressPayload);

    Serial.print("MQTT Temp -> ");
    Serial.print(IO_FEED_TEMP_MQTT);
    Serial.print(" = ");
    Serial.println(tempPayload);

    Serial.print("MQTT Hum  -> ");
    Serial.print(IO_FEED_HUM_MQTT);
    Serial.print(" = ");
    Serial.println(humPayload);

    Serial.print("MQTT Press-> ");
    Serial.print(IO_FEED_PRESS_MQTT);
    Serial.print(" = ");
    Serial.println(pressPayload);

    if (!(okMQTT_T && okMQTT_H && okMQTT_P)) {
      Serial.println("Au moins un publish MQTT a echoue.");
    }
  }

  // Mets à jour le client MQTT (gestion interne)
  mqttClient.update();
}
