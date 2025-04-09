#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "secrets.h" // wifi and supabase credentials
#include <TFT_eSPI.h>
#include <Wire.h> // touch
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Pin defs (match User_Setup.h)
#define TOUCH_SDA   6
#define TOUCH_SCL   7
#define TOUCH_INT   5
#define TOUCH_RST  13
#define TFT_BL      2

TFT_eSPI tft = TFT_eSPI();

const long gmtOffset_sec = -5 * 3600;
const int daylightOffset_sec = 3600;

void setup() {
  Serial.begin(115200);
  delay(1000);

  
  // Init I2C for touchscreen
  Wire.begin(TOUCH_SDA, TOUCH_SCL);
  pinMode(TOUCH_RST, OUTPUT);
  pinMode(TOUCH_INT, INPUT);
  
  // Reset touch chip
  digitalWrite(TOUCH_RST, LOW);
  delay(100);
  digitalWrite(TOUCH_RST, HIGH);
  delay(100);
  
  // Turn on backlight
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  
  // Init display
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(2);
  tft.drawString("Hello Adventourers!", 120, 110);

  // Start WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected!");

  // Set system time
  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org");

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String url = SUPABASE_URL_BASE;
    http.begin(url);

    http.addHeader("apiKey", SUPABASE_ANONKEY);
    http.addHeader("Authorization", "Bearer " + String(SUPABASE_ANONKEY));

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String payload = http.getString();
      Serial.println("Supabase payload: ");
      Serial.println(payload);

      JsonDocument doc;
      
      DeserializationError error = deserializeJson(doc, payload);
      if (!error) {
        // If the response is an array and has at least one object
        JsonArray arr = doc.as<JsonArray>();
        if (arr.size() > 0) {
          // Retrieve a field named "message" from the first object.
          const char* message = arr[0]["message"];
          if (message != nullptr) {
            // Clear a portion of the screen and then display the message.
            tft.fillScreen(TFT_BLACK);
            tft.drawString(message, 120, 120);
          }
          else {
            tft.fillScreen(TFT_BLACK);
            tft.drawString("Field missing", 120, 120);
          }
        }
        else {
          tft.fillScreen(TFT_BLACK);
          tft.drawString("No results", 120, 120);
        }
      }
      else {
        Serial.print("JSON parse error: ");
        Serial.println(error.c_str());
        tft.fillScreen(TFT_BLACK);
        tft.drawString("Parse error", 120, 120);
      }
    }
    else {
      Serial.print("HTTP GET error: ");
      Serial.println(http.errorToString(httpResponseCode));
      tft.fillScreen(TFT_BLACK);
      tft.drawString("HTTP error", 120, 120);
    }
    http.end();
  }
  else {
    tft.fillScreen(TFT_BLACK);
    tft.drawString("WiFi disconnected", 120, 120);
  }
  
  delay(10000);

  /*
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    int hour12 = timeinfo.tm_hour % 12;
    if (hour12 == 0) hour12 = 12;
    const char* ampm = (timeinfo.tm_hour < 12) ? "AM" : "PM";

    char dateStr[16];
    char timeStr[16];

    // date formatting
    snprintf(dateStr, sizeof(dateStr), "%02d/%02d/%04d", 
             timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_year + 1900);

    // tiem formatting
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d %s", 
             hour12, timeinfo.tm_min, timeinfo.tm_sec, ampm);

    // Display text centered
    tft.fillScreen(TFT_BLACK);
    tft.drawString(dateStr, 120, 105);   // Center of screen
    tft.drawString(timeStr, 120, 135);

    Serial.println(dateStr);
    Serial.println(timeStr);
    Serial.println();
  }
  else {
    Serial.println("Failed to get time");
  }
  delay(1000);
  */
}