//https://beelogger.de/sensoren/temperatursensor-ds18b20/ für Pinning und Anregung
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <secrets.h>

#define LED_ERROR 23
#define LED_OK 22
#define ONE_WIRE_BUS 25
#define DTH22_pin 4
unsigned long timeInterval = 30000;
byte debug = 0;

// Definition der Zugangsdaten WiFi
WiFiClient myWiFiClient;

//Definition der Zugangsdaten MQTT
#define MQTT_CLIENTID "ESP32_Pool_Tempsensor" //Name muss eineindeutig auf dem MQTT-Broker sein!
#define MQTT_KEEPALIVE 60
#define MQTT_SOCKETTIMEOUT 25
#define MQTT_SERIAL_PUBLISH_STATUS "SmartHome/Pool/Technikraum/ESP32_Sensor_DHT22_DS18B20/status"
#define MQTT_SERIAL_RECEIVER_COMMAND "SmartHome/Pool/Technikraum/ESP32_Sensor_DHT22_DS18B20/command"
#define MQTT_SERIAL_PUBLISH_DS18B20 "SmartHome/Pool/Technikraum/ESP32_Sensor_DHT22_DS18B20/"
String mqttTopic;
String mqttJson;
String mqttPayload;
DeviceAddress myDS18B20Address;
String Adresse;
unsigned long MQTTReconnect = 0;
PubSubClient mqttClient(myWiFiClient);

// Anzahl der angeschlossenen DS18B20 - Sensoren
int DS18B20_Count = 0;

//DHT22
#define DHTTYPE DHT22
#define DHTPIN 4
DHT dht(DHTPIN,DHTTYPE);

//Initialisiere OneWire und Thermosensor(en)
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature myDS18B20(&oneWire);

void mqttConnect () {
  int i = 0;
  Serial.print("Verbindungsaubfau zu MQTT Server ");
  Serial.print(MQTT_SERVER);
  Serial.print(" Port ");  
  Serial.print(MQTT_PORT);
  Serial.print(" wird aufgebaut ");  
  while (!mqttClient.connected()) {
    Serial.print(".");
    if (mqttClient.connect(MQTT_CLIENTID, MQTT_USER, MQTT_PASSWORD, MQTT_SERIAL_PUBLISH_STATUS, 0, true, "false"))
    {
      mqttClient.publish(MQTT_SERIAL_PUBLISH_STATUS, "true", true);
      Serial.println("");
      Serial.println("MQTT verbunden!");
    } 
    else {
      if (++i > 20) {
        Serial.println("MQTT scheint nicht mehr erreichbar! Reboot!!");
        ESP.restart();
      }
      Serial.print("fehlgeschlagen rc=");
      Serial.print(mqttClient.state());
      Serial.println(" erneuter Versuch in 5 Sekunden.");
      delay(5000);      
    }    
  }
  mqttClient.subscribe(MQTT_SERIAL_RECEIVER_COMMAND);
}

void mqttCallback(char* topic, byte* message, unsigned int length) {
  String str;
  unsigned long mqttValue;
  String mqttMessage;
  String mqttTopicAC;
  byte tx_ac = 1;
  for (int i = 0; i < length; i++)
  {
    str += (char)message[i];
  }
  if (debug) {
    Serial.print("Nachricht aus dem Topic: ");
    Serial.print(topic);
    Serial.print(". Nachricht: ");
    Serial.println(str);
  }
  //Test-Botschaften  
  mqttTopicAC = MQTT_SERIAL_PUBLISH_DS18B20;
  mqttTopicAC += "ac";
  if (str.startsWith("Test")) {
    if (debug) Serial.println("check!");
    mqttClient.publish(mqttTopicAC.c_str(), "Test OK");
    tx_ac = 0;
  }
  //debug-Modfikation  
  if ((tx_ac) && (str.startsWith("debug=0"))) {
    debug = 0;
    mqttClient.publish(mqttTopicAC.c_str(), "debug=0 umgesetzt");
    tx_ac = 0;
  }
  if ((tx_ac) && (str.startsWith("debug=1"))) {
    debug = 1;
    mqttClient.publish(mqttTopicAC.c_str(), "debug=1 umgesetzt");
    tx_ac = 0;
  }
  if ((tx_ac) && (str.startsWith("time="))) {
    str.remove(0,5);
    mqttValue = str.toInt();
    if (mqttValue > 200) {
      timeInterval = mqttValue;    
      mqttMessage = "time=" + String(mqttValue) + " umgesetzt";
      mqttClient.publish(mqttTopicAC.c_str(), mqttMessage.c_str());
    } else {
      mqttClient.publish(mqttTopicAC.c_str(), "time < 200! nicht umgesetzt!");
    }
    tx_ac = 0;
  }
  if ((tx_ac) && (str.startsWith("restart"))) {
    mqttClient.publish(mqttTopicAC.c_str(), "reboot in einer Sekunde!");
    if (debug) Serial.println("für Restart: alles aus & restart in 1s!");
    vTaskDelay(1000);
    if (debug) Serial.println("führe Restart aus!");
    ESP.restart();
  }
}

void setup() {
  // Initialisierung und plausibilitaetschecks
  Serial.begin(115200);
  while (!Serial)
  Serial.println("Start Setup");
  pinMode(LED_ERROR, OUTPUT);
  digitalWrite(LED_ERROR, LOW);
  pinMode(LED_OK, OUTPUT);
  digitalWrite(LED_OK, HIGH);
  //WiFi-Setup
  int i = 0;
  Serial.print("Verbindungsaufbau zu ");
  Serial.print(ssid);
  WiFi.setHostname(HOSTNAME);
  WiFi.begin(ssid,password);
  while (WiFi.status() != WL_CONNECTED)
  {
    ++i;
    if (i > 240) {
      // Reboot nach 2min der Fehlversuche
      Serial.println("WLAN scheint nicht mehr erreichbar! Reboot!!");
      ESP.restart();
    }
    delay(500);
    Serial.print(".");    
  }
  Serial.println("");
  Serial.println("WiFi verbunden.");
  Serial.print("IP Adresse: ");
  Serial.print(WiFi.localIP());
  Serial.println("");
  //MQTT-Setup
  Serial.println("MQTT Server Initialisierung laeuft...");
  mqttClient.setServer(MQTT_SERVER,MQTT_PORT); 
  mqttClient.setCallback(mqttCallback);
  mqttClient.setKeepAlive(MQTT_KEEPALIVE);
  mqttClient.setSocketTimeout(MQTT_SOCKETTIMEOUT);
  mqttConnect();
  mqttTopic = MQTT_SERIAL_PUBLISH_DS18B20 + String("error");
  mqttPayload = String(String(MQTTReconnect) + ".: keine Fehler seit Reboot!");
  mqttClient.publish(mqttTopic.c_str(), mqttPayload.c_str());
  Serial.println("");
  //DS18B20-Setup
  Serial.println("Auslesen der DS18B20-Sensoren...");
  myDS18B20.begin();
  Serial.print("Anzahl gefundener 1-Wire-Geraete:  ");
  Serial.println(myDS18B20.getDeviceCount());
  DS18B20_Count = myDS18B20.getDS18Count();
  Serial.print("Anzahl gefundener DS18B20-Geraete: ");
  Serial.println(DS18B20_Count);
  if (DS18B20_Count == 0) {
    Serial.println("...kein DB18B20 angeschlossen... System angehalten!");
    digitalWrite(LED_OK, LOW);
    while (true) {
      //blinke bis zur Unendlichkeit...
      digitalWrite(LED_ERROR, HIGH);
      delay(250);
      digitalWrite(LED_ERROR, LOW);
      delay(250);
    }
  }
  Serial.print("Globale Aufloesung (Bit):        ");
  Serial.println(myDS18B20.getResolution());
  //DHT22-Sensor
  Serial.println("DHT22 initialisieren...");
  dht.begin();
  //OK-Blinker
  digitalWrite(LED_OK, HIGH);
  delay(250);
  digitalWrite(LED_OK, LOW);
  Serial.println("Normalbetrieb gestartet...");
}

void loop() {
  // put your main code here, to run repeatedly:
  if (!mqttClient.connected()) {
    if (debug) Serial.println("MQTT Server Verbindung verloren...");
    if (debug) Serial.print("Disconnect Errorcode: ");
    if (debug) Serial.println(mqttClient.state());  
    //Vorbereitung errorcode MQTT
    //https://pubsubclient.knolleary.net/api#state
    mqttTopic = MQTT_SERIAL_PUBLISH_DS18B20 + String("error");
    mqttPayload = String(String(++MQTTReconnect) + ". reconnect: ") + String("; MQTT disconnect rc=" + String(mqttClient.state()));
    //reconnect
    mqttConnect();
    //sende Fehlerstatus
    mqttClient.publish(mqttTopic.c_str(), mqttPayload.c_str());
  }
  mqttClient.loop();
  if (debug) Serial.print("\nAnfrage der Temperatursensoren... ");
  myDS18B20.requestTemperatures();  //Anfrage zum Auslesen der Temperaturen
  if (debug) Serial.println("fertig");
  for (int i = 0; i < DS18B20_Count; i++) {
    //print to Serial
    if (debug) { 
      Serial.print("DS18B20[");
      Serial.print(i);
      Serial.print("]: ");
      Serial.print(myDS18B20.getTempCByIndex(i));
      Serial.print(" *C (");
    }    
    myDS18B20.getAddress(myDS18B20Address,i);
    Adresse="";
    for (uint8_t j = 0; j < 8; j++)
    {
      Adresse += "0x";
      if (myDS18B20Address[j] < 0x10) Adresse += "0";
      Adresse += String(myDS18B20Address[j], HEX);
      if (j < 7) Adresse += ", ";
    }
    if (debug) Serial.println(Adresse + ")");
    //MQTT-Botschaften#
    //(Index + 1 in Ausgabe, da DHT22 = ID0)
    //JSON        
    mqttTopic = MQTT_SERIAL_PUBLISH_DS18B20 + String(i+1) + "/JSON";
    mqttJson = "{\"ID\":\"" + String(i) + "\"";
    mqttJson += ",\"Type\":\"" + String("DS18B20") + "\"";
    mqttJson += ",\"Temperatur\":\"" + String(myDS18B20.getTempCByIndex(i)) + "\"";
    mqttJson += ",\"Adresse\":\"(" + Adresse + ")\"}";
    if (debug) Serial.println("MQTT_JSON: " + mqttJson);
    mqttClient.publish(mqttTopic.c_str(), mqttJson.c_str());
    //Temperatur
    mqttTopic = MQTT_SERIAL_PUBLISH_DS18B20 + String(i+1) + "/Temperatur";
    mqttPayload = String(myDS18B20.getTempCByIndex(i));
    mqttClient.publish(mqttTopic.c_str(), mqttPayload.c_str());
    //ID
    mqttTopic = MQTT_SERIAL_PUBLISH_DS18B20 + String(i+1) + "/ID";
    mqttPayload = String(i);
    mqttClient.publish(mqttTopic.c_str(), mqttPayload.c_str());
    //Adresse
    mqttTopic = MQTT_SERIAL_PUBLISH_DS18B20 + String(i+1) + "/Adresse";
    mqttClient.publish(mqttTopic.c_str(), Adresse.c_str());
  }
  mqttClient.loop();
  // DHT22 Sensorauswertung
  if (debug) Serial.println("\nAnfrage des Feuchte- & Temperatursensors DHT22... ");
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float hic = 0;
  byte dhtOK=1;
  // Ueberprüfung, ob der DHT22 ausgelesen werden konnte.
  if (isnan(h) || isnan(t)) {
    if (debug) Serial.println("Abfrage des DHT22 fehlgeschlagen!");
    //setze Fehlerwerte für MQTT-Botschaft    
    h = -1.0;
    t = -99.0;
    hic = -99.0;   
    dhtOK = 0;
  }
  //https://de.wikipedia.org/wiki/Hitzeindex
  if (dhtOK) hic = dht.computeHeatIndex(t, h, false);
  if ((debug) && (dhtOK)) { 
    Serial.print("DHT22: Feuchtigkeit: ");
    Serial.print(h);
    Serial.print("%, Temperatur: ");
    Serial.print(t);
    Serial.print("*C, HeatIndex: ");
    Serial.print(hic);
    Serial.println("*C");
  }    
  //MQTT-Botschaften DHT22
  //JSON  
  mqttTopic = MQTT_SERIAL_PUBLISH_DS18B20 + String("0/JSON");
  mqttJson = "{\"ID\":\"0\"";
  mqttJson += ",\"Temperatur\":\"" + String(t) + "\"";
  mqttJson += ",\"Feuchtigkeit\":\"" + String(h) + "\"";
  mqttJson += ",\"HitzeIndex\":\"" + String(hic) + "\"}";
  if (debug) Serial.println("MQTT_JSON: " + mqttJson);
  mqttClient.publish(mqttTopic.c_str(), mqttJson.c_str());
  //Temperatur
  mqttTopic = MQTT_SERIAL_PUBLISH_DS18B20 + String("0/Temperatur");
  mqttPayload = String(t);
  mqttClient.publish(mqttTopic.c_str(), mqttPayload.c_str());
  //ID
  mqttTopic = MQTT_SERIAL_PUBLISH_DS18B20 + String("0/ID");
  mqttPayload = "0";
  mqttClient.publish(mqttTopic.c_str(), mqttPayload.c_str());
  //Type
  mqttTopic = MQTT_SERIAL_PUBLISH_DS18B20 + String("0/Type");
  mqttPayload = "DHT22";
  mqttClient.publish(mqttTopic.c_str(), mqttPayload.c_str());
  //Feuchtigkeit
  mqttTopic = MQTT_SERIAL_PUBLISH_DS18B20 + String("0/Feuchtigkeit");
  mqttPayload = String(h);
  mqttClient.publish(mqttTopic.c_str(),mqttPayload.c_str());
  //HeatIndex
  mqttTopic = MQTT_SERIAL_PUBLISH_DS18B20 + String("0/HitzeIndex");
  mqttPayload = String(hic);
  mqttClient.publish(mqttTopic.c_str(),mqttPayload.c_str());
  //Finish
  mqttClient.loop();
  digitalWrite(LED_OK, HIGH);
  delay(150);
  digitalWrite(LED_OK, LOW);
  delay(timeInterval - 150);
}
