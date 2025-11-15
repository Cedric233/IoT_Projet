#include <WiFiNINA.h>
#include <Arduino_MKRIoTCarrier.h>
MKRIoTCarrier carrier;

// --- Wi-Fi ---
char ssid[] = "Houston";
char pass[] = "cedric10";

int status = WL_IDLE_STATUS;
IPAddress ipLocal;                 // variable globale IP

// --- LEDs ---
bool ledsOn = false;
unsigned long lastToggle = 0;
const unsigned long PERIOD_MS = 500;

// --- Capteurs / affichage ---
const float ALTITUDE_M = 120.0;
float fTemp, fHum, fPress_hPa, fPressSL_hPa;

unsigned long lastUi = 0;
const unsigned long UI_UPDATE_MS = 500;

void writeColors() {
  carrier.leds.setPixelColor(0, carrier.leds.Color(255, 0, 0));
  carrier.leds.setPixelColor(1, carrier.leds.Color(0, 255, 0));
  carrier.leds.setPixelColor(2, carrier.leds.Color(0, 0, 255));
  carrier.leds.setPixelColor(3, carrier.leds.Color(255, 255, 0));
  carrier.leds.setPixelColor(4, carrier.leds.Color(255, 0, 255));
}

void printMacAddress(byte mac[]) {
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) Serial.print("0");
    Serial.print(mac[i], HEX);
    if (i > 0) Serial.print(":");
  }
  Serial.println();
}

void printCurrentNet() {
  Serial.print("SSID : ");  Serial.println(WiFi.SSID());
  byte bssid[6]; WiFi.BSSID(bssid);
  Serial.print("BSSID : "); printMacAddress(bssid);
  Serial.print("Encryption Type : "); Serial.println(WiFi.encryptionType(), HEX);
  Serial.print("Signal Strength (RSSI) : "); Serial.print(WiFi.RSSI()); Serial.println(" dBm\n");
}

void setup() {
  carrier.begin();

  // Buzzer 1 s
  carrier.Buzzer.sound(500);
  delay(1000);
  carrier.Buzzer.noSound();

  // Démarrage Série
  Serial.begin(9600);
  while (!Serial) { }
  Serial.println("Initialisation Wi-Fi...");

  // Écran "Connexion..."
  carrier.display.fillScreen(ST77XX_RED);
  carrier.display.setTextColor(ST77XX_WHITE);
  carrier.display.setTextSize(2);
  carrier.display.setCursor(40, 110);
  carrier.display.print("Connexion...");

  // Vérifs module
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("ERREUR : Module WiFi non detecte !");
    while (true) {}
  }
  if (WiFi.firmwareVersion() < String("1.0.0")) {
    Serial.println("Avertissement : mettez a jour le firmware NINA !");
  }

  // Connexion
  while (status != WL_CONNECTED) {
    Serial.print("Tentative de connexion au reseau : ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(10000);
  }

  // --- CONNECTE : récupère et affiche l'IP ---
  ipLocal = WiFi.localIP();
  Serial.println("\nConnexion Wi-Fi reussie !");
  Serial.print("Adresse IP : ");
  Serial.println(ipLocal);
  printCurrentNet();

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

  //  --- TEST DE PING INTERNET ---
  char site[] = "www.google.com";
  int rtt = WiFi.ping(site);  // envoie un ping

  if (rtt >= 0) {
    Serial.print("Ping OK vers ");
    Serial.print(site);
    Serial.print(" : ");
    Serial.print(rtt);
    Serial.println(" ms");

    // Affiche le succès sur l'écran
    carrier.display.fillScreen(ST77XX_GREEN);
    carrier.display.setTextColor(ST77XX_BLACK);
    carrier.display.setTextSize(2);
    carrier.display.setCursor(20, 100);
    carrier.display.print("Ping Google OK");
    carrier.display.print(rtt);
    carrier.display.print("ms");
  } else {
    Serial.print("Ping echoue vers ");
    Serial.println(site);

    // Affiche l'échec sur l'écran
    carrier.display.fillScreen(ST77XX_RED);
    carrier.display.setTextColor(ST77XX_WHITE);
    carrier.display.setTextSize(2);
    carrier.display.setCursor(20, 100);
    carrier.display.print("Ping echoue!");
  }
  delay(3000);
  // ✅ --- FIN TEST DE PING ---

  // Prépare l'écran pour les valeurs
  carrier.display.fillScreen(ST77XX_BLACK);
  carrier.display.setTextColor(ST77XX_WHITE);
  carrier.display.setTextSize(2);

  // LEDs : démarrage
  writeColors();
  carrier.leds.show();
  ledsOn = true;
  lastToggle = millis();
}

void loop() {
  unsigned long now = millis();

  // Clignotement LEDs
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

  // Maj capteurs + écran
  if (now - lastUi >= UI_UPDATE_MS) {
    lastUi = now;

    fTemp       = carrier.Env.readTemperature();
    fHum        = carrier.Env.readHumidity();
    fPress_hPa  = carrier.Pressure.readPressure();
    fPressSL_hPa = fPress_hPa + (ALTITUDE_M / 8.0);

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
}
