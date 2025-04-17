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

String getUserFirstName(const char* user_id) {
  String url = SUPABASE_URL_PER;
  url += "?select=first_name&id=eq.";
  url += user_id;
  url += "&limit=1";  // Limit to the first matching row

  HTTPClient http;
  http.begin(url);
  http.addHeader("apikey", SUPABASE_ANONKEY);
  http.addHeader("Authorization", String("Bearer ") + SUPABASE_ANONKEY);
  
  int httpCode = http.GET();
  String firstName = "";
  if (httpCode > 0) {
    String payload = http.getString();
    /*
    Serial.println("Users payload:");
    Serial.println(payload);
    */
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (!error) {
      if (doc.is<JsonArray>()) {
        JsonArray arr = doc.as<JsonArray>();
        if (arr.size() > 0) {
          JsonObject user = arr[0];
          // Extract the 'first_name' field.
          if (user["first_name"]) {
            firstName = user["first_name"].as<const char*>();
          } else {
            Serial.println("first_name field missing");
          }
        } else {
          Serial.println("No user found in the array");
        }
      } else {
        Serial.println("Unexpected JSON format for Users payload");
      }
    } else {
      Serial.print("deserializeJson() failed in getUserFirstName: ");
      Serial.println(error.f_str());
    }
  } else {
    Serial.printf("HTTP GET error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
  return firstName;
}

String getNextEventSlot(struct tm timeinfo) {
  int currentMinutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;
  if (currentMinutes < 600) return "event_1000";     // Before 10:00 AM
  else if (currentMinutes < 645) return "event_1045";  // 10:00 - 10:44
  else if (currentMinutes < 690) return "event_1130";  // 10:45 - 11:29
  else if (currentMinutes < 735) return "event_1215";  // 11:30 - 12:14
  else if (currentMinutes < 780) return "event_1300";  // 12:15 - 12:59
  else if (currentMinutes < 825) return "event_1345";  // 1:00 - 1:44
  else if (currentMinutes < 870) return "event_1430";  // 1:45 - 2:29
  else if (currentMinutes < 915) return "event_1515";  // 2:30 - 3:14
  else if (currentMinutes < 960) return "event_1600";  // 3:15 - 3:59
  else if (currentMinutes < 1005) return "event_1645"; // 4:00 - 4:44
  else if (currentMinutes < 1050) return "event_1730"; // 4:45 - 5:29
  else if (currentMinutes < 1095) return "event_1815"; // 5:30 - 6:14
  else if (currentMinutes < 1140) return "event_1900"; // 6:15 - 6:59
  else if (currentMinutes < 1185) return "event_1945"; // 7:00 - 7:44
  else if (currentMinutes < 1230) return "event_2030"; // 7:45 - 8:29
  else if (currentMinutes < 1275) return "event_2115"; // 8:30 - 9:14
  else return "";
}

bool getEventDetails(const JsonObject& eventData, String result[2]) {
  // Check required fields
  if (!eventData["id"] || !eventData["type"]) {
    Serial.println("Event data missing 'id' or 'type'");
    return false;
  }
  
  const char* event_id = eventData["id"].as<const char*>();
  const char* event_type = eventData["type"].as<const char*>();
  
  String url;
  String nameField;

  // Choose Supabase endpoint and the appropriate name field for each event type.
  if (strcmp(event_type, "Shops") == 0) {
    url = SUPABASE_URL_SHP;
    url += "?select=shop_name,location&id=eq.";
    nameField = "shop_name";
  } else if (strcmp(event_type, "Rides") == 0) {
    url = SUPABASE_URL_RID;
    url += "?select=ride_name,location&id=eq.";
    nameField = "ride_name";
  } else if (strcmp(event_type, "Dining") == 0) {
    url = SUPABASE_URL_DIN;
    url += "?select=dining_name,location&id=eq.";
    nameField = "dining_name";
  } else if (strcmp(event_type, "Shows") == 0) {
    url = SUPABASE_URL_SHW;
    url += "?select=show_name,location&id=eq.";
    nameField = "show_name";
  } else if (strcmp(event_type, "Animals") == 0) {
    url = SUPABASE_URL_ANI;
    url += "?select=habitat_name,location&id=eq.";
    nameField = "habitat_name";
  } else {
    Serial.print("Unknown event type: ");
    Serial.println(event_type);
    return false;
  }
  
  url += event_id;
  url += "&limit=1";
  
  /*
  Serial.print("Requesting details with URL: ");
  Serial.println(url);
  */
  
  HTTPClient http;
  http.begin(url);
  http.addHeader("apikey", SUPABASE_ANONKEY);
  http.addHeader("Authorization", String("Bearer ") + SUPABASE_ANONKEY);
  
  int httpCode = http.GET();
  if (httpCode <= 0) {
    Serial.printf("HTTP GET error: %s\n", http.errorToString(httpCode).c_str());
    http.end();
    return false;
  }
  
  String response = http.getString();
  http.end();
  /*
  Serial.println("Response:");
  Serial.println(response);
  */
  
  // Parse the JSON response.
  // Expecting an array with one object.
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, response);
  if (err) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(err.f_str());
    return false;
  }
  
  JsonArray arr = doc.as<JsonArray>();
  if (arr.size() == 0) {
    Serial.println("No details returned");
    return false;
  }
  
  JsonObject details = arr[0];
  const char* name = details[nameField.c_str()];
  const char* location = details["location"];
  
  result[0] = (name ? String(name) : "");
  result[1] = (location ? String(location) : "");
  
  return true;
}

int minutesUntilSlot(const struct tm &now, const String &slotKey) {
  // slotKey looks like "event_1000" -> we want the "1000" part
  String hhmm = slotKey.substring(slotKey.indexOf('_') + 1);  // "1000"
  if (hhmm.length() != 4) return 0;  // malformed
  
  // parse hours/minutes from the four chars
  int h = (hhmm[0] - '0') * 10 + (hhmm[1] - '0');
  int m = (hhmm[2] - '0') * 10 + (hhmm[3] - '0');
  
  int nowMins   = now.tm_hour * 60 + now.tm_min;
  int eventMins = h * 60 + m;
  return eventMins - nowMins;
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
      /*
      Serial.println("Supabase payload: ");
      Serial.println(payload);
      */

      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        if (doc.is<JsonArray>()) {
          // IMPORTANT: plans is an array of json objects
          JsonArray plans = doc.as<JsonArray>();
          /*
          Serial.println("Plans: ");
          serializeJsonPretty(plans, Serial);
          Serial.println();
          */

          if (plans.size() > 0) {
            for (int i = 0; i < plans.size(); i++) {
              // IMPORTANT: object is a singular row in an array of json objects
              JsonObject object = plans[i].as<JsonObject>();
              /*
              Serial.printf("Object %d: \n", i);
              serializeJsonPretty(object, Serial);
              Serial.println();
              */

              if (object["current_plan"] == true) {
                const char* user_id = object["user_id"];
                String first_name = getUserFirstName(user_id);

                String line_1 = "";
                String line_2 = "";
                String line_3 = "";
                String line_4 = "";
                if (first_name != "") {
                  Serial.println(first_name);
                }
                else {
                  Serial.println("Couldn't find first name");
                }
                // always:
                // get user's name
                // display current date and time
                //
                struct tm timeinfo;
                bool haveCurrentTime = false;
                if (getLocalTime(&timeinfo)) {
                  int hour12 = timeinfo.tm_hour % 12;
                  if (hour12 == 0) hour12 = 12;
                  const char* ampm = (timeinfo.tm_hour < 12) ? "AM" : "PM";

                  char dateStr[16];
                  char timeStr[16];

                  // date formatting
                  snprintf(dateStr, sizeof(dateStr), "%02d/%02d", 
                          timeinfo.tm_mon + 1, timeinfo.tm_mday);

                  // tiem formatting
                  snprintf(timeStr, sizeof(timeStr), "%02d:%02d%s", 
                          hour12, timeinfo.tm_min, ampm);

                  Serial.println(dateStr);
                  Serial.println(timeStr);
                  line_1 = first_name + " " + String(dateStr) + " " + String(timeStr);
                  haveCurrentTime = true;
                }
                else {
                  Serial.println("Failed to get current time");
                  line_1 = "unknown time";
                }

                const String date = object["date"];
                Serial.print("Plan date: ");
                Serial.println(date);
                int date_year = date.substring(0,4).toInt();
                int date_month = date.substring(5,7).toInt();
                int date_day = date.substring(8,10).toInt();

                int current_year = 0;
                int current_month = 0;
                int current_day = 0;

                int day_diff = 0;

                enum dateWhen { PAST, TODAY, FUTURE, ERROR };
                dateWhen planWhen = ERROR;

                String nextEventSlot = "event_1000";

                if (haveCurrentTime) {
                  current_year = timeinfo.tm_year + 1900;
                  current_month = timeinfo.tm_mon + 1;
                  current_day = timeinfo.tm_mday;

                  if (date_year == current_year && date_month == current_month && date_day == current_day) {
                    Serial.println("The selected plan is for today.");
                    planWhen = TODAY;

                  }
                  else if (date_year >= current_year && date_month >= current_month && date_day >= current_day) {
                    Serial.println("The selected plan is in the future.");
                    planWhen = FUTURE;
                    day_diff = date_day - current_day;
                  }
                  else {
                    Serial.println("The selected plan is in the past.");
                    planWhen = PAST;
                    day_diff = current_day - date_day;
                  }
                  
                  if (planWhen == PAST) {
                    nextEventSlot = "event_2115";
                  }
                  else if (planWhen == FUTURE) {
                    nextEventSlot = "event_1000";
                  }
                  else {  // else plan is for TODAY
                    nextEventSlot = getNextEventSlot(timeinfo);
                  }
                }

                if (planWhen == ERROR) {
                  // bla bla blaaaa display error stuff on screen
                  Serial.println("AT Error Code 1: Plan does not have a date set.");
                  tft.drawString("AT Error Code 1", 120, 120);
                }
                else if (planWhen == PAST) {
                  // bla bla bla your trip was X days ago
                  Serial.printf("Your trip was %d days ago\n", day_diff);
                  line_2 = "Your Trip Was";
                  line_3 = String(day_diff) + "days";
                  line_4 = "ago :(";
                  
                }
                else if (planWhen == FUTURE) {
                  // bla bla bla your trip will be in X days
                  Serial.printf("Your trip will be in %d days\n", day_diff);
                  line_2 = "Your Trip Is In";
                  line_3 = String(day_diff) + "days";
                  line_4 = "woohoo!";
                }
                else {  // else plan is for TODAY
                  // display ride name and location
                  // calculate the time until event in minutes
                  // Compute how many minutes until that next slot:
                  int minsToNext = minutesUntilSlot(timeinfo, nextEventSlot);
                  Serial.printf("Minutes until %s: %d\n", nextEventSlot.c_str(), minsToNext);

                  // Now display it however you like:
                  // e.g. if you got your eventDetails already:

                  JsonObject eventData = object[nextEventSlot.c_str()].as<JsonObject>();
                  Serial.printf("Next Event Is %s: \n", nextEventSlot);
                  /*
                  serializeJsonPretty(eventData, Serial);
                  Serial.println();
                  */

                  String eventDetails[2];

                  if (getEventDetails(eventData, eventDetails)) {
                    Serial.print("Event Name: ");
                    Serial.println(eventDetails[0]);
                    Serial.print("Event Location: ");
                    Serial.println(eventDetails[1]);

                    line_2 = String(eventDetails[0]);
                    line_3 = String(abs(minsToNext) + " minutes");
                    line_4 = "in " + eventDetails[1];
                  } else {
                    Serial.println("AT Error Code 2: Failed to retrieve event details");
                    tft.drawString("AT Error Code 2", 120, 120);
                  }
                  
                  // if nextEventSlot = 10am, choose latest time between 10am and time_start
                  // while current time < time_end, display time until next event
                  // if time until < 60 minutes, display number of minutes till
                  // else display time next event is at
                }

                // if current date is before plan date, display event_1000 or first populated event_XXXX slot
                // else do time math to find the next event
                // display the ride name and location
                // calculate the time until in minutes

                tft.setTextSize(1);
                tft.drawString(line_1, 120, 50);
                tft.setTextSize(3);
                tft.drawString(line_2, 120, 70);
                tft.setTextSize(5);
                tft.drawString(line_3, 120, 90);
                tft.setTextSize(3);
                tft.drawString(line_4, 120, 110);
              } else {
                continue;
              }
            }

        /*
            if (object.containsKey("event_1000")) {
              JsonObject event1000 = object["event_1000"];
              // For example, print the "type" within event_1000:
              if (event1000.containsKey("type")) {
                const char* eventType = event1000["type"];
                Serial.print("event_1000 type: ");
                Serial.println(eventType);
              } else {
                Serial.println("event_1000 type missing");
              }
            } else {
              Serial.println("event_1000 missing");
            }
*/
          }
        }
      }
      
      else {
        Serial.print("JSON parse error: ");
        Serial.println(error.f_str());
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
  
  delay(60000);

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