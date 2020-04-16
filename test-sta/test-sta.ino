#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <PageBuilder.h>

ESP8266WebServer server(80);

static const char _BODY[] PROGMEM = "<h1>You are connected {{NAME}}</h1>";

String name(PageArgument& args) {
  return String("bryan");
}

PageElement body(_BODY, {{"NAME", name}});

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\r\n");

  WiFi.mode(WIFI_AP);

  String apssid = "reader-esp" + String(ESP.getChipId(), HEX);

  Serial.print("Configuring access point... ");
  Serial.println(WiFi.softAP(apssid) ? "Ready" : "Failed!");

  server.on("/", body);
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
