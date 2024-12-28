#include <Arduino_JSON.h>
#include <ESP8266WebServer.h>
#include <ESP8266WebServerSecure.h>
#include <ESP8266WebServerSecureAxTLS.h>
#include <ESP8266WebServerSecureBearSSL.h>
#include <DHT.h>
#include <DHT_U.h>
#include "thingProperties.h"

#define DHTPIN D4
#define DHTLED D5
#define DHTTYPE DHT22

#define LED_LNK D7

#define SEALEVELPRESSURE_HPA (1023.00)

#define NODE_ID "5483524859d4ffa8c17f89e1687a9f33933a49d236f3e17021871763e163a29f"

ESP8266WebServer server(80);

DHT dht (DHTPIN, DHTTYPE);

uint32_t delayMS;
unsigned long previousMillis = 0;
const long interval = 5000;

String IpDevice = "";
String ArdCloudStat = "";

void setup() {
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
  //
  pinMode(DHTLED, OUTPUT);
  // web server
  Serial.println("[SERVER] Initialize...");
  server.on("/healthCheck", HTTP_GET, checkSensor);
  server.on("/getInfo", HTTP_GET, getValuesFromWeb);
  server.begin();
}

void loop() {
  ArduinoCloud.update();
  // web server
  server.handleClient();
  //
  if(ArduinoCloud.connected()) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      digitalWrite(DHTLED, HIGH);
      getSensorValues();
      previousMillis = currentMillis;
      delay(50);
    }
    digitalWrite(DHTLED, LOW);
    ArdCloudStat = "OK";
  } else {
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

void getValuesFromWeb() {
  digitalWrite(LED_LNK, HIGH);
  delay(100);
  Serial.println("[SERVER] GET: /getInfo");
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  JSONVar JSONData;
  JSONData["temp"] = temperature;
  JSONData["humd"] = humidity;
  String result = JSON.stringify(JSONData);
  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "application/json", result);
  Serial.println("[SERVER] Request ready");
  digitalWrite(LED_LNK, LOW);
}

void checkSensor() {
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
  String result = JSON.stringify(JSONData);
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
