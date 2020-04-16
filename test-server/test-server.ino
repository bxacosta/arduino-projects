#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <PageBuilder.h>

ESP8266WebServer server(80);

void handleNotFound() {
   server.send(404, "text/plain", "Not found");
}

void handleRoot() {
  server.send(200, "text/html", "<h1>You are connected</h1>");
}

void handleConfig() {
   Serial.println("POST REQUEST");
   server.send(200, "text/plain", String(WiFi.getMode()));
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\r\n");

  WiFi.mode(WIFI_AP);

  String apssid = "reader-esp" + String(ESP.getChipId(), HEX);

  Serial.print("Configuring access point... ");
  Serial.println(WiFi.softAP(apssid) ? "Ready" : "Failed!");

  server.on("/", handleRoot);
  server.on("/config", HTTP_POST, handleConfig);
  
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
  
  Serial.print("Local IP address: ");
  Serial.println("Ip: " + WiFi.localIP());

  Serial.print("SoftAp IP address: ");
  Serial.println(WiFi.softAPIP());
}  

void loop() {
  server.handleClient();
}
