#include <ArduinoJson.h>
#include "FS.h"

// VERBOSE OUTPUT
#define SERIAL_VERBOSE

// FILE NAME CONSTANT
#define CONFIG_FILE "/config.json"


bool loadFile(const char* filename, String *obj) {
  File file = SPIFFS.open(filename, "r");
  if (!file) {
#ifdef SERIAL_VERBOSE
    Serial.print("Failed to open ");
    Serial.println(filename);
#endif
    return false;
  }

  size_t size = file.size();
  if (size > 1024) {
#ifdef SERIAL_VERBOSE
    Serial.print("File size is too large ");
    Serial.println(size);
#endif
    return false;
  } else if (size == 0) {
#ifdef SERIAL_VERBOSE
    Serial.print(filename);
    Serial.println(" is empty");
#endif
    return false;
  }

  StaticJsonDocument<256> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
#ifdef SERIAL_VERBOSE
    Serial.print("Failed to parse ");
    Serial.println(filename);
    Serial.println(error.c_str());
#endif
    return false;
  }
  
  serializeJson(doc, *obj);
  file.close();
  return true;
}


bool saveFile(const char* filename, JsonObject obj) {
  File file = SPIFFS.open(filename, "w");
  if (!file) {
#ifdef SERIAL_VERBOSE
    Serial.print("Failed to open/create ");
    Serial.println(filename);
#endif
    return false;
  }

  // Serialize JSON to file
  if (serializeJson(obj, file) == 0) {
    Serial.print("Failed to write to ");
    Serial.println(filename);
    return false;
  }

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

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\r\n");

  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }

  StaticJsonDocument<256> doc;
  doc["ssid"] = "test";
  doc["lkjhgfd"] = "reader.1";
  doc["test"] = "asdf";
  JsonObject json = doc.as<JsonObject>();

  showFiles();
  
  if (saveFile(CONFIG_FILE, json)) {
    Serial.println("\nConfig saved " + String(CONFIG_FILE));
    serializeJson(doc, Serial);
  } else {
    Serial.println("\nFailed to save " + String(CONFIG_FILE));
  }

  String str;
  
  if (loadFile(CONFIG_FILE, &str)) {
    Serial.println("\nConfig loaded " + String(CONFIG_FILE));
    doc.clear();
    deserializeJson(doc, str);
    serializeJson(doc, Serial);
  } else {
    Serial.println("\nFailed to load config " + String(CONFIG_FILE));
  }

  Serial.println("\n");
  showFiles();
}

void loop() {
}
