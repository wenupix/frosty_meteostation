#include <AsyncEventSource.h>
#include <AsyncJson.h>
#include <AsyncWebSocket.h>
#include <AsyncWebSynchronization.h>
#include <ESPAsyncWebServer.h>
#include <StringArray.h>
#include <WebAuthentication.h>
#include <WebHandlerImpl.h>
#include <WebResponseImpl.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Arduino_JSON.h>
#include <Adafruit_BME280.h>
#include "thingProperties.h"

#define LED_LNK 27
#define LED_NLK 26
#define BZZ_PIN 14

#define NODE_ID "6e6df21e46170e3778aa001693d5cf18b801ab85efe91c1bc570a4f6751510ed"

WebServer server(80);
// inicializacion del modulo dht y bme
Adafruit_BME280 bme;

// presion atmosferica a nivel del mar
#define SEALEVELPRESSURE_HPA (1013.00)

uint32_t delayMS;
unsigned long previousMillis = 0;
const long interval = 5000;

int altd; // variable para altitud
bool dataDebug = true; // mostrar valores en consola serial

// variables para healthcheck
String IpDevice = "";
String ArdCloudStat = "";

void setup() {
  buzzer_start();
  // leds: panel de control
  pinMode(LED_LNK, OUTPUT);
  pinMode(LED_NLK, OUTPUT);
  digitalWrite(LED_LNK, HIGH);
  digitalWrite(LED_NLK, HIGH);
  // Inicializa la conexion serial
  Serial.begin(9600);
  // delay para dar tiempo al dispositivo a conectarse al cloud
  delay(1500);
  // Defined in thingProperties.h
  initProperties();
  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  /////////////////////////////
  // preparando conexion con la wifi
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
  // bme280: inicializando sensor bme
  Serial.println("[BME280] Initialize...");
  bool status;
  status = bme.begin(0x76);
  if(!status) {
    Serial.println("[BME280] Error in module. Check wiring");
    while(1);
  }
  Serial.println("[BME280] Ready");
  // server: inicializando servidor http
  Serial.println("[SERVER] Inicializando...");
  server.on("/healthCheck", HTTP_GET, healthCheck);
  server.on("/getInfo", HTTP_GET, getInfo);
  server.begin();
  Serial.print("[SERVER] Listo.");
  digitalWrite(LED_LNK, LOW);
  digitalWrite(LED_NLK, LOW);
  Serial.println("[ESP8266] READY");
  buzzer_ready();
}

void loop() {
  ArduinoCloud.update();
  // web server
  server.handleClient();
  // si el dispositivo está conectado a la nube arduino
  if(ArduinoCloud.connected()) {
    digitalWrite(LED_NLK, LOW); // led de actividad: off
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      digitalWrite(LED_LNK, HIGH);
      getSensorValues(); // obtiene valores del sensor
      previousMillis = currentMillis;
      delay(100);
    }
    digitalWrite(LED_LNK, LOW);
    ArdCloudStat = "OK"; // estado de la conexion al cloud
  } else { // problemas con la conexion
    digitalWrite(LED_LNK, LOW);
    digitalWrite(LED_NLK, HIGH);
    ArdCloudStat = "FAIL";
  }
}

// obtiene valores del sensor BME
// son almacenados en variables globales de arduino cloud
// luego, son leidos por el dashboard
void getSensorValues() {
  temp = roundf( bme.readTemperature() * 10) / 10 ;
  humd = roundf( bme.readHumidity() * 10) / 10 ;
  pres = roundf( (bme.readPressure() / 100.0F) * 10 ) / 10;
  altd = bme.readAltitude(SEALEVELPRESSURE_HPA);
  if(dataDebug) {
    Serial.print("[SENSORS_STAT] ");
    Serial.print("TEMP: ");
    Serial.print(temp);
    Serial.print(" °C | HUMD: ");
    Serial.print(humd);
    Serial.print(" % | PRES: ");
    Serial.print(pres);
    Serial.print(" hPa | ALTD: ");
    Serial.print(altd);
    Serial.print("m");
    Serial.println();
  }
}

//
// las siguientes funciones son los request que recibe
// el servidor web y responde con un mensaje en formato JSON
//

// funcion para el request con los datos del sensor
void getInfo() {
  digitalWrite(LED_LNK, HIGH);
  delay(100);
  Serial.println("[SERVER] GET: /getInfo");
  JSONVar JSONData;
  JSONData["temp"] = temp;
  JSONData["humd"] = humd;
  JSONData["pres"] = pres;
  JSONData["altd"] = altd;
  String result = JSON.stringify(JSONData);
  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "application/json", result);
  Serial.println("[SERVER] Request ready");
  digitalWrite(LED_LNK, LOW);
}

// funcion para el request del healthcheck
void healthCheck() {
  digitalWrite(LED_LNK, HIGH);
  delay(100);
  Serial.println("[SERVER] GET: /healthCheck");
  String bmeStat;
  if(isnan(bme.readTemperature()) || isnan(bme.readHumidity()) || isnan(bme.readPressure()) || isnan(bme.readAltitude(SEALEVELPRESSURE_HPA))) {
    bmeStat = "FAIL";
  } else {
    bmeStat = "OK";
  }
  JSONVar JSONData;
  JSONData["node_id"] = NODE_ID;
  JSONData["NodeMCU"] = "OK";
  JSONData["LINK"] = IpDevice;
  JSONData["BME280"] = bmeStat;
  JSONData["ArdCloudStat"] = ArdCloudStat;
  String result = JSON.stringify(JSONData);;
  server.send(200, "application/json", result);
  Serial.println("[SERVER] Request ready");
  digitalWrite(LED_LNK, LOW);
}

//
// funciones para generar un tono en el buzzer
//
// generador tono - inicio mcu
void buzzer_start() {
  // 1
  tone(BZZ_PIN, 440);
  delay(100);
  noTone(BZZ_PIN);
  delay(10);
  // 2
  tone(BZZ_PIN, 880);
  delay(100);
  noTone(BZZ_PIN);
  delay(10);
  // 3
  tone(BZZ_PIN, 1760);
  delay(100);
  noTone(BZZ_PIN);
  delay(10);
}

// generador tono - comando recibido
void buzzer_ready() {
  // 1
  tone(BZZ_PIN, 1000);
  delay(250);
  noTone(BZZ_PIN);
  delay(50);
}

//
// codigo generado automaticamente por arduino cloud
//
void onTempChange()  {
  // Add your code here to act upon Temperature change
}
void onHumdChange()  {
  // Add your code here to act upon Humidity change
}
void onPresChange()  {
  // Add your code here to act upon Humidity change
}