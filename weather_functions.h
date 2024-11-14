#ifndef WEATHER_FUNCTIONS_H
#define WEATHER_FUNCTIONS_H

#include <lvgl.h>
#include "weather_images.h"

#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <vector>

int is_day;
static lv_obj_t * weather_image;
String weather_description;

extern std::vector<float> tomorrowTemperatures;
extern std::vector<String> weather_descriptions_tomorrow;
extern String current_date;
extern String last_weather_update;
extern String temperature;
extern String humidity;
extern int weather_code;

void get_weather_description(int code) {
  switch (code) {
    case 0:
      if(is_day==1) { lv_image_set_src(weather_image, &image_weather_sun); }
      else { lv_image_set_src(weather_image, &image_weather_night); }
      weather_description = "CLEAR SKY";
      break;
    case 1:
      if(is_day==1) { lv_image_set_src(weather_image, &image_weather_sun); }
      else { lv_image_set_src(weather_image, &image_weather_night); }
      weather_description = "MAINLY CLEAR";
      break;
    case 2:
      lv_image_set_src(weather_image, &image_weather_cloud);
      weather_description = "PARTLY CLOUDY";
      break;
    case 3:
      lv_image_set_src(weather_image, &image_weather_cloud);
      weather_description = "OVERCAST";
      break;
    case 45:
      lv_image_set_src(weather_image, &image_weather_cloud);
      weather_description = "FOG";
      break;
    case 48:
      lv_image_set_src(weather_image, &image_weather_cloud);
      weather_description = "DEPOSITING RIME FOG";
      break;
    case 51:
      lv_image_set_src(weather_image, &image_weather_rain);
      weather_description = "DRIZZLE LIGHT INTENSITY";
      break;
    case 53:
      lv_image_set_src(weather_image, &image_weather_rain);
      weather_description = "DRIZZLE MODERATE INTENSITY";
      break;
    case 55:
      lv_image_set_src(weather_image, &image_weather_rain);
      weather_description = "DRIZZLE DENSE INTENSITY";
      break;
    case 56:
      lv_image_set_src(weather_image, &image_weather_rain);
      weather_description = "FREEZING DRIZZLE LIGHT";
      break;
    case 57:
      lv_image_set_src(weather_image, &image_weather_rain);
      weather_description = "FREEZING DRIZZLE DENSE";
      break;
    case 61:
      lv_image_set_src(weather_image, &image_weather_rain);
      weather_description = "RAIN SLIGHT INTENSITY";
      break;
    case 63:
      lv_image_set_src(weather_image, &image_weather_rain);
      weather_description = "RAIN MODERATE INTENSITY";
      break;
    case 65:
      lv_image_set_src(weather_image, &image_weather_rain);
      weather_description = "RAIN HEAVY INTENSITY";
      break;
    case 66:
      lv_image_set_src(weather_image, &image_weather_rain);
      weather_description = "FREEZING RAIN LIGHT INTENSITY";
      break;
    case 67:
      lv_image_set_src(weather_image, &image_weather_rain);
      weather_description = "FREEZING RAIN HEAVY INTENSITY";
      break;
    case 71:
      lv_image_set_src(weather_image, &image_weather_snow);
      weather_description = "SNOW FALL SLIGHT INTENSITY";
      break;
    case 73:
      lv_image_set_src(weather_image, &image_weather_snow);
      weather_description = "SNOW FALL MODERATE INTENSITY";
      break;
    case 75:
      lv_image_set_src(weather_image, &image_weather_snow);
      weather_description = "SNOW FALL HEAVY INTENSITY";
      break;
    case 77:
      lv_image_set_src(weather_image, &image_weather_snow);
      weather_description = "SNOW GRAINS";
      break;
    case 80:
      lv_image_set_src(weather_image, &image_weather_rain);
      weather_description = "RAIN SHOWERS SLIGHT";
      break;
    case 81:
      lv_image_set_src(weather_image, &image_weather_rain);
      weather_description = "RAIN SHOWERS MODERATE";
      break;
    case 82:
      lv_image_set_src(weather_image, &image_weather_rain);
      weather_description = "RAIN SHOWERS VIOLENT";
      break;
    case 85:
      lv_image_set_src(weather_image, &image_weather_snow);
      weather_description = "SNOW SHOWERS SLIGHT";
      break;
    case 86:
      lv_image_set_src(weather_image, &image_weather_snow);
      weather_description = "SNOW SHOWERS HEAVY";
      break;
    case 95:
      lv_image_set_src(weather_image, &image_weather_thunder);
      weather_description = "THUNDERSTORM";
      break;
    case 96:
      lv_image_set_src(weather_image, &image_weather_thunder);
      weather_description = "THUNDERSTORM SLIGHT HAIL";
      break;
    case 99:
      lv_image_set_src(weather_image, &image_weather_thunder);
      weather_description = "THUNDERSTORM HEAVY HAIL";
      break;
    default:
      weather_description = "UNKNOWN WEATHER CODE";
      break;
  }
}

void get_weather_data() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String("http://api.open-meteo.com/v1/forecast?latitude=" + Config::latitude + "&longitude=" + Config::longitude + "&current=temperature_2m,relative_humidity_2m,is_day,precipitation,rain,weather_code" + "&timezone=" + Config::timezone + "&forecast_days=2");
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
          const char* datetime = doc["current"]["time"];
          temperature = String(round(doc["current"]["temperature_2m"].as<float>()), 0);
          humidity = String(doc["current"]["relative_humidity_2m"]);
          is_day = String(doc["current"]["is_day"]).toInt();
          weather_code = String(doc["current"]["weather_code"]).toInt();
          String datetime_str = String(datetime);
          int splitIndex = datetime_str.indexOf('T');
          current_date = datetime_str.substring(0, splitIndex);
          last_weather_update = datetime_str.substring(splitIndex + 1, splitIndex + 9); 
        } else {
          Serial.print("deserializeJson() failed: ");
          Serial.println(error.c_str());
        }
      }
    } else {
      Serial.printf("GET request failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end(); 
  } else {
    Serial.println("Not connected to Wi-Fi");
  }
}

#endif
