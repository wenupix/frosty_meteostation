#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Arduino_JSON.h>
#include "thingProperties.h"

#define DHTPIN 32
#define DHTTYPE DHT22
#define DHT_LED 33
#define LED_LNK 25
#define LED_NLK 27

#define NODE_ID "5483524859d4ffa8c17f89e1687a9f33933a49d236f3e17021871763e163a29f"

WebServer server(80);

DHT dht (DHTPIN, DHTTYPE);
//
uint32_t delayMS;
unsigned long previousMillis = 0;
const long interval = 5000;

String IpDevice = "";
String ArdCloudStat = "";

void setup() {
  // leds
  pinMode(DHT_LED, OUTPUT);
  pinMode(LED_LNK, OUTPUT);
  pinMode(LED_NLK, OUTPUT);
  digitalWrite(DHT_LED, HIGH);
  digitalWrite(LED_LNK, HIGH);
  digitalWrite(LED_NLK, HIGH);
  // Initialize serial and wait for port to open:
  Serial.begin(9600);
  // This delay gives the chance to wait for a Serial Monitor without blocking if none is found
  delay(1500); 
  // Defined in thingProperties.h
  initProperties();
  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  /////////////////////////////
  Serial.println("[DHT22] Inicializando...");
  dht.begin();
  Serial.println("[DHT22] Listo");
  Serial.print("[WIFI] Conectando a la red SSID: ");
  Serial.print(SECRET_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(SECRET_SSID, SECRET_OPTIONAL_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("[WIFI] Conectado a: ");
  Serial.println(SECRET_SSID);
  Serial.print("[WIFI] IP: ");
  Serial.println(WiFi.localIP());
  IpDevice = WiFi.localIP().toString();

  // server
  Serial.println("[SERVER] Inicializando...");
  server.on("/healthCheck", HTTP_GET, healthCheck);
  server.on("/getInfo", HTTP_GET, getInfo);
  server.begin();
  Serial.print("[SERVER] Listo.");
  digitalWrite(DHT_LED, LOW);
  digitalWrite(LED_LNK, LOW);
  digitalWrite(LED_NLK, LOW);
}

void loop() {
  ArduinoCloud.update();
  // web server
  server.handleClient();
  if(ArduinoCloud.connected()) {
    digitalWrite(LED_NLK, LOW);
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      digitalWrite(DHT_LED, HIGH);
      getSensorValues();
      previousMillis = currentMillis;
      delay(100);
    }
    digitalWrite(DHT_LED, LOW);
    ArdCloudStat = "OK";
  } else {
    digitalWrite(DHT_LED, LOW);
    digitalWrite(LED_NLK, HIGH);
    ArdCloudStat = "FAIL";
  }
}

void getSensorValues(){
  if (isnan(dht.readTemperature())) {
    Serial.println(F("[DHT22] Error reading temperature!"));
    temperature = 0;
  }
  else {
    Serial.print(F("Temperature: "));
    Serial.print(dht.readTemperature());
    Serial.print(F("°C | "));
    temperature = dht.readTemperature();
  }
  
  if (isnan(dht.readHumidity())) {
    Serial.println(F("[DHT22] Error reading humidity!"));
    humidity = 0;
  }
  else {
    Serial.print(F("Humidity: "));
    Serial.print(dht.readHumidity());
    Serial.println(F("%"));
    humidity = dht.readHumidity();
  }
}

void getInfo() {
  digitalWrite(LED_LNK, HIGH);
  delay(100);
  Serial.println("[SERVER] GET: /getInfo");
  JSONVar JSONData;
  JSONData["temp"] = temperature;
  JSONData["humd"] = humidity;
  String result = JSON.stringify(JSONData);
  //serializeJson(JSONData, data);
  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "application/json", result);
  Serial.println("[SERVER] Request ready");
  digitalWrite(LED_LNK, LOW);
}

void healthCheck() {
  digitalWrite(LED_LNK, HIGH);
  delay(100);
  Serial.println("[SERVER] GET: /healthCheck");
  String dhtStat;
  if(isnan(dht.readTemperature()) || isnan(dht.readHumidity())) {
    dhtStat = "FAIL";
  } else {
    dhtStat = "OK";
  }
  JSONVar JSONData;
  JSONData["node_id"] = NODE_ID;
  JSONData["NodeMCU"] = "OK";
  JSONData["LINK"] = IpDevice;
  JSONData["DHT22"] = dhtStat;
  JSONData["ArdCloudStat"] = ArdCloudStat;
  String result = JSON.stringify(JSONData);;
  server.send(200, "application/json", result);
  Serial.println("[SERVER] Request ready");
  digitalWrite(LED_LNK, LOW);
}


void onTemperatureChange()  {
  // Add your code here to act upon Temperature change
}
void onHumidityChange()  {
  // Add your code here to act upon Humidity change
}