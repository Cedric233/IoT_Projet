# IoT Projet – Arduino MKR IoT Carrier

##  Description
Ce projet IoT utilise la carte **Arduino MKR WiFi 1010** et le **MKR IoT Carrier (Oplà Kit)** pour mesurer :
-  la **température**
-  l’**humidité**
-  la **pression atmosphérique**

Les données sont :
- affichées sur l’écran du Carrier,
- envoyées au service **ThingSpeak** pour visualisation en ligne,
- et la carte vérifie sa connexion Internet via un **ping vers Google**.

##  Matériel utilisé
- Arduino MKR WiFi 1010  
- MKR IoT Carrier  
- Capteur environnemental intégré (temp/hum/press)  
- Buzzer, LEDs RGB, écran TFT  

##  Fonctionnalités
- Connexion Wi-Fi (via `WiFiNINA`)  
- Test de connexion Internet (`WiFi.ping`)  
- Envoi vers ThingSpeak via `WiFiClient`  
- Affichage dynamique sur écran TFT  

##  Configuration
Dans le code :
```cpp
char ssid[] = "TonSSID";
char pass[] = "TonMotDePasse";
const char* TS_API_KEY = "TaCleAPI";
