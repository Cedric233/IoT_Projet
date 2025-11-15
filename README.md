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

## 🌡️ Dernière température mesurée
![Dernière température](https://api.thingspeak.com/channels/3165332/field/1/last.png)

## 💧 Dernière humidité mesurée
![Dernière humidité](https://api.thingspeak.com/channels/3165332/field/3/last.png)

## 🧭 Dernière pression atmosphérique
![Dernière pression](https://api.thingspeak.com/channels/3165332/field/2/last.png)


## 📈 Évolution des 24 dernières heures
![Graphique des 24h](https://api.thingspeak.com/channels/3165332/charts/1?bgcolor=%23ffffff&color=%23d62020&dynamic=true&results=100&title=Température+sur+24h&type=line)

