#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "secrets.h"

const long gmtOffset_sec = -5 * 3600;
const int daylightOffset_sec = 3600;

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected!");

  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org");

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
}

void loop() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    int hour12 = timeinfo.tm_hour % 12;
    if (hour12 == 0) hour12 = 12;
    const char* ampm = (timeinfo.tm_hour < 12) ? "AM" : "PM";

    char line1[16];
    char line2[16];

    // date formatting
    snprintf(line1, sizeof(line1), "%02d/%02d/%04d", 
             timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_year + 1900);

    // tiem formatting
    snprintf(line2, sizeof(line2), "%02d:%02d:%02d %s", 
             hour12, timeinfo.tm_min, timeinfo.tm_sec, ampm);

    Serial.println(line1);
    Serial.println(line2);
    Serial.println();
  } else {
    Serial.println("Failed to get time");
  }
  delay(1000);
}
