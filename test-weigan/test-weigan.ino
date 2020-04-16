#include <Wiegand.h>

WIEGAND wg;

// WIEGAND
#define DATA0 D1 // GPIO5
#define DATA1 D2 // GPIO4
#define POOL_SIZE 10 // GPIO4

unsigned long lastMovRequest = 0;

struct Read {
  unsigned long data;
  unsigned long time;
};

Read read[POOL_SIZE];
int count = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("\n\r");

  wg.begin(DATA0, DATA1);
}

void loop() {
  if (validCode()) {
    printData();
  }
}

bool validCode() {
  bool valid = false;
  if (wg.getWiegandType() == 34) {
    unsigned long data = wg.getCode();
    unsigned long currentMillisMov = millis();

    bool exist = false;
    for (int i = 0; i < count; i++) {
      if (currentMillisMov - read[i].time >  10 * 1000) {
        if (read[i].data == data) {
          valid = true;
          exist = true;
          read[i].time = currentMillisMov;
        } else {
          memmove(read + i, read + i + 1, (POOL_SIZE - i - 1) * POOL_SIZE);
          read[POOL_SIZE - 1] = {0, 0};
          count--;
          i--;
        }
      } else {
        if (read[i].data == data) {
          exist = true;
        }
      }
    }

    if (!exist) {
      if (count < POOL_SIZE) {
        valid = true;
        read[count] = {data, currentMillisMov};
        count++;
      } else {
        valid = true;
      }
    }
  }
  return valid;
}

void printData() {
  Serial.print(" Wiegand HEX = ");
  Serial.print(wg.getCode(), HEX);
  Serial.print(", DECIMAL = ");
  Serial.print(wg.getCode());
  Serial.print(", Type W");
  Serial.println(wg.getWiegandType());

  Serial.print("[ ");
  for (int i = 0; i < POOL_SIZE; i++) {
    Serial.print("{" + String(read[i].data) + ", " + String(read[i].time) + "} ");
  }
  Serial.println("]");
}
