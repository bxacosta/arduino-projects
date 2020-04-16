#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <FS.h>

ESP8266WebServer server(80);

// VERBOSE OUTPUT
//#define SERIAL_VERBOSE

// SOFT AP SETTINGS
#define AP_NAME "reader-esp" + String(ESP.getChipId(), HEX)
#define AP_PASSWORD "jedai.1234"

// FILE NAME CONSTANT
#define CONFIG_FILE "/config.json"
#define STA_FILE "/sta.json"

// STRUCT DEFINITIONS
struct Params {
  String id;
  String idS;
  String base;
  String ctrl;
  String mov;
  unsigned long report;
  unsigned long read;
};

// GLOBAL VARIABLES
unsigned long lastCtrlRequest = 0;
unsigned long lastMovRequest = 0;
bool haveConfig = false;
Params params;


struct network {
  String mac;
  String ip;
  String gateway;
  String subnet;
  String hostname;
  String status;
  String ssid;
  String psk;
  String bssid;
  String rssid;
};

////////////////////////// SERVER METHODS //////////////////////////////////
void handleRoot() {
  server.send(200, "text/html", "<h1>You are connected</h1>");
}

void handleConfig() {
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "Body not received");
    return;
  }

  String body = server.arg("plain");

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, body);
  if (error) {
    server.send(500, "text/plain", "Deserialization error: " + String(error.c_str()));
    return;
  }

  JsonObject obj = doc.as<JsonObject>();
  if (!saveJsonFile(CONFIG_FILE, obj)) {
    server.send(500, "text/plain", "Error to write config file");
    return;
  }

  server.send(200, "text/plain", "Config saved");
}

void handleSta() {
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "Body not received");
    return;
  }

  String body = server.arg("plain");

  StaticJsonDocument<96> doc;
  DeserializationError error = deserializeJson(doc, body);
  if (error) {
    server.send(500, "text/plain", "Deserialization error: " + String(error.c_str()));
    return;
  }

  JsonObject obj = doc.as<JsonObject>();
  if (!saveJsonFile(STA_FILE, obj)) {
    server.send(500, "text/plain", "Error to write config file");
    return;
  }

  server.send(200, "text/plain", "STA saved");
}
/////////////////////////> SERVER METHODS </////////////////////////////////


////////////////////////// SPIFFS METHODS //////////////////////////////////
bool saveJsonFile(const char* filename, JsonObject obj) {
  File file = SPIFFS.open(filename, "w");
  if (!file) {
#ifdef SERIAL_VERBOSE
    Serial.println("Failed to open/create file " + String(filename));
#endif
    return false;
  }

  // Serialize JSON to file
  if (serializeJson(obj, file) == 0) {
#ifdef SERIAL_VERBOSE
    Serial.println("Failed to write file " + String(filename));
#endif
    return false;
  }

#ifdef SERIAL_VERBOSE
  Serial.println("File created/saved " + String(filename));
#endif

  file.close();
  return true;
}

bool loadJsonFile(const char* filename, String *obj) {
  File file = SPIFFS.open(filename, "r");
  if (!file) {
#ifdef SERIAL_VERBOSE
    Serial.println("Failed to open " + String(filename));
#endif
    return false;
  }

  size_t size = file.size();
  if (size > 512) {
#ifdef SERIAL_VERBOSE
    Serial.println("File size is too large " + String(size));
#endif
    return false;
  } else if (size == 0) {
#ifdef SERIAL_VERBOSE
    Serial.println(String(filename) + " is empty");
#endif
    return false;
  }

  StaticJsonDocument<512> doc;
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
#ifdef SERIAL_VERBOSE
    Serial.println("Failed to deserialize file " + String(filename));
    Serial.println("Error: " + String(error.c_str()));
#endif
    return false;
  }

  serializeJson(doc, *obj);
  file.close();
  return true;
}

void showFiles() {
  Dir dir = SPIFFS.openDir("/");
  Serial.println("Flash memory content:");
  while (dir.next()) {
    Serial.print(dir.fileName() + " - ");
    File f = dir.openFile("r");
    Serial.println(f.size());
  }
}
/////////////////////////> SPIFFS METHODS </////////////////////////////////

////////////////////////// SETUP METHODS //////////////////////////////////
void defaultConfig() {
  Serial.print("Setting mode to WIFI_AP_STA: ");
  Serial.println(WiFi.mode(WIFI_AP_STA) ? "Ready" : "Failed");

  Serial.print("Configuring access point: ");
  Serial.println(WiFi.softAP(AP_NAME , AP_PASSWORD) ? "Ready" : "Failed");

  Serial.println(String(AP_NAME) + " started, Ip: " + WiFi.softAPIP().toString());

  StaticJsonDocument<96> doc;
  String str;

  if (loadJsonFile(STA_FILE, &str)) {
    deserializeJson(doc, str);
    JsonObject sta = doc.as<JsonObject>();

    Serial.println("Setting STA from file " STA_FILE);
#ifdef SERIAL_VERBOSE
    serializeJson(sta, Serial);
    Serial.println();
#endif
    connectAp(sta);
  }

  server.on("/sta", HTTP_POST, handleSta);
}

bool connectAp(JsonObject network) {
  if (network.containsKey("ip") && network.containsKey("gateway") && network.containsKey("subnet")) {
    IPAddress ip, gateway, subnet;
    byte fail = 0;
    if (!ip.fromString(network["ip"].as<String>())) fail++;
    if (!gateway.fromString(network["gateway"].as<String>())) fail++;
    if (!subnet.fromString(network["subnet"].as<String>())) fail++;

    if (fail == 0) {
      Serial.print("Setting static ip: ");
      Serial.println(WiFi.config(ip, gateway, subnet) ? "Ready" : "Failed");
    } else {
      Serial.print("Fail to setting static address: ");
      Serial.println(network["ip"].as<String>() + " - " + network["gateway"].as<String>() + " - " + network["subnet"].as<String>());
    }
  }

  Serial.println("Connecting to " + network["ssid"].as<String>());
  WiFi.begin(network["ssid"].as<char*>(), network["password"].as<char*>());
  unsigned long tm = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - tm < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.isConnected()) {
    Serial.println("\nConnected, Ip: " + WiFi.localIP().toString());
    return true;
  } else {
    Serial.println("\nNot connected, Wifi satus: " + String(WiFi.status()));
    return false;
  }
}

/**
   1: Fail to begin HTTP client
   2: HTTP request failed
   3: HTTP Code is not HTTP_CODE_OK or HTTP_CODE_MOVED_PERMANENTLY
   String: All ok, payload of HTTP response
*/
String sendRequest(String url, JsonObject data) {
  WiFiClient client;
  HTTPClient http;
  String payload;

  if (http.begin(client, url)) {
    http.addHeader("Content-Type", "application/json");
    //    http.setAuthorization("guest", "guest");

    serializeJson(data, payload);

#ifdef SERIAL_VERBOSE
    Serial.println("[HTTP] POST request to: " + url);
    Serial.println("[HTTP] Request payload: " + payload);
#endif
    int httpCode = http.POST(payload);

    if (httpCode > 0) {
#ifdef SERIAL_VERBOSE
      Serial.println("[HTTP] HTTP return code: " + String(httpCode));
      Serial.println("[HTTP] POST response payload:");
      Serial.println(http.getString());
#endif
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        payload = http.getString();
      } else {
        payload = "3";
      }
    } else {
#ifdef SERIAL_VERBOSE
      Serial.println("[HTTP] POST failed, error: " + http.errorToString(httpCode));
#endif
      payload = "2";
    }
    http.end();
  } else {
#ifdef SERIAL_VERBOSE
    Serial.println("[HTTP} Unable to connect");
#endif
    payload = "1";
  }
  return payload;
}


network getApConfig() {
  network net;
  net.mac = WiFi.macAddress();
  net.ip = WiFi.localIP().toString();
  net.gateway = WiFi.gatewayIP().toString();
  net.subnet = WiFi.subnetMask().toString();
  net.hostname = WiFi.hostname();
  net.status = WiFi.status();
  net.ssid = WiFi.SSID();
  net.psk = WiFi.psk();
  net.bssid = WiFi.BSSIDstr();
  net.rssid = WiFi.RSSI();

  return net;
}
/////////////////////////> SETUP METHODS </////////////////////////////////


////////////////////////////// SETUP ///////////////////////////////////////
void setup() {
  Serial.begin(115200);
  Serial.println("\r\n");
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }

  WiFi.softAPdisconnect();
  WiFi.disconnect();

  StaticJsonDocument<512> doc;
  String str;

  if (loadJsonFile(CONFIG_FILE, &str)) {
    deserializeJson(doc, str);
    JsonObject config = doc.as<JsonObject>();

    Serial.print("Setting mode to WIFI_STA: ");
    Serial.println(WiFi.mode(WIFI_STA) ? "Ready" : "Failed");

    Serial.println("Setting STA from file " CONFIG_FILE);
#ifdef SERIAL_VERBOSE
    serializeJson(config, Serial);
    Serial.println();
#endif

    JsonObject network = config["network"].as<JsonObject>();
    if (connectAp(network)) {
      Serial.println("Config server routes");
      haveConfig = true;

      params.id = config["id"].as<String>();
      params.idS = config["idS"].as<String>();
      params.base = config["url"]["base"].as<String>();
      params.ctrl = config["url"]["ctrl"].as<String>();
      params.mov = config["url"]["mov"].as<String>();
      params.report = config["interval"]["report"].as<long>();
      params.read = config["interval"]["read"].as<long>();
    } else {
      defaultConfig();
    }
  } else {
    Serial.println("Failed to read config from " CONFIG_FILE);
    defaultConfig();
  }

  server.on("/", handleRoot);
  server.on("/config", HTTP_POST, handleConfig);

  server.onNotFound([]() {
    server.send(404, "text/plain", "Not found");
  });
  server.begin();
}
/////////////////////////////> SETUP <//////////////////////////////////////

void loop() {
  server.handleClient();

  if (WiFi.status() == WL_CONNECTED && haveConfig) {
    unsigned long currentMillisCtrl = millis();

    StaticJsonDocument<96> doc;
    JsonObject data;
    String url = params.base;


    if (currentMillisCtrl - lastCtrlRequest >  params.report * 1000) {
      lastCtrlRequest = currentMillisCtrl;

      data = doc.to<JsonObject>();
      data["id"] = params.id;

      url += params.ctrl;
      sendRequest(url, data);
    }

    if (Serial.available() > 0) {
      unsigned long currentMillisMov = millis();
      String value = Serial.readString();

      data = doc.to<JsonObject>();
      data["idS"] = params.idS;
      data["tag"] = value;

      url += params.mov;
      sendRequest(url, data);

      //      if (currentMillisMov - lastMovRequest >  params.read * 1000) {
      //        lastMovReq = currentMillis;
      //      }
    }
  }
}
