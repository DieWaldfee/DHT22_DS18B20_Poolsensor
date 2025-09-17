# DHT22_DS18B20_Poolsensor
Diese Projekt nutzt das ESP32 DEV Kit V4 Modul zur Ermittlung der Temperaturen in meinem Pool-Projekt.
Ausgewertet werden soviele DS18B20-Temperatursensoren, wie verfügbr und ein DHT1-Sensor für die Lufttempertur und dessen Feuchte.
Übermittel werden die erfassten Daten via MQTT an einen Broker - bei mir im ioBroker integriert und für die Poolsteuerung verwendet.

Die Platine ist für eine Lochrasterplatine entworfen - ist also einfach nachzubauen.


### Abhängigkeiten:
OneWire.h:_____________OneWire by Jim Studt, Tom Pollard, Robin James... v2.3.8 (über Arduino IDE)<br>
DallasTemperature.h:___DallasTemperature by Miles Burton v4.0.3 (über Arduino IDE)<br>
DHT:___________________DHT sensor library by Adafruit V1.4.6
WiFi.h:________________Arduino IDE<br>
WiFiClient.h:__________Arduino IDE<br>
PubSubClient.h:________PubSubClient by Nick O'Leary v2.8 (über Arduino IDE) [also tested with PubSubClient3 V3.2.1]<br>
Wire.h:________________by Jim Studt, Tom Pollard, Robiun James... v2.3.8 (über Arduino IDE)<br>
esp_task_wdt.h:________Espressif IDE<br>

### Board ESP32
esp32 by Espressif Systems v1.0.6 (über Arduino IDE)<br>
Getestet habe ich auch v2.0.5<br>
