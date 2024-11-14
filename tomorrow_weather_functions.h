#ifndef TOMORROW_WEATHER_FUNCTIONS_H
#define TOMORROW_WEATHER_FUNCTIONS_H

#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <vector>
#include <time.h>
#include <math.h>  // Include math.h for the round() function

void parseHourlyTemperature(const JsonArray& forecastList) {
  Serial.println("Hourly Temperature Forecast for Tomorrow:");
  for (JsonObject forecast : forecastList) {
    String dateTime = forecast["dt_txt"];
    float temperature = forecast["main"]["temp"];
    int roundedTemperature = static_cast<int>(round(temperature));

    Serial.print(dateTime);
    Serial.print(" - Temp: ");
    Serial.print(roundedTemperature);
    Serial.println("°C");
  }
}

void getDescriptionForTomorrow(const JsonArray& forecastList) {
  struct tm timeInfo;
  if (!getLocalTime(&timeInfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  timeInfo.tm_hour = 0;
  timeInfo.tm_min = 0;
  timeInfo.tm_sec = 0;
  timeInfo.tm_mday += 1;
  mktime(&timeInfo);

  char tomorrowDate[11];
  strftime(tomorrowDate, sizeof(tomorrowDate), "%Y-%m-%d", &timeInfo);

  Serial.printf("Looking for weather descriptions for tomorrow (%s):\n", tomorrowDate);
  weather_descriptions_tomorrow.clear();

  for (JsonObject forecast : forecastList) {
    const char* dateTime = forecast["dt_txt"];
    if (strncmp(dateTime, tomorrowDate, 10) == 0) {
      const char* description = forecast["weather"][0]["description"];
      Serial.printf("Weather Description: %s\n", description);
      weather_descriptions_tomorrow.push_back(description);
    }
  }

  if (weather_descriptions_tomorrow.empty()) {
    weather_descriptions_tomorrow.push_back("No weather description available for tomorrow.");
  }
}

void storeAndCalculateTemperatures(const JsonArray& forecastList) {
  struct tm timeInfo;
  if (!getLocalTime(&timeInfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  timeInfo.tm_mday += 1;
  mktime(&timeInfo);

  char tomorrowDate[11];
  strftime(tomorrowDate, sizeof(tomorrowDate), "%Y-%m-%d", &timeInfo);

  Serial.print("Looking for forecasts for tomorrow (");
  Serial.print(tomorrowDate);
  Serial.println("):");

  tomorrowTemperatures.clear();
  int maxTemp = -273;  // Initialize with a realistic low temperature in °C
  int minTemp = 1000;  // Initialize with a high value for comparison

  for (JsonObject forecast : forecastList) {
    const char* dateTime = forecast["dt_txt"];
    if (strncmp(dateTime, tomorrowDate, 10) == 0) {
      float temp = forecast["main"]["temp"];
      int roundedTemp = static_cast<int>(round(temp));
      tomorrowTemperatures.push_back(roundedTemp);

      if (roundedTemp > maxTemp) maxTemp = roundedTemp;
      if (roundedTemp < minTemp) minTemp = roundedTemp;

      Serial.print("Stored Temp: ");
      Serial.println(roundedTemp);
    }
  }

  if (tomorrowTemperatures.empty()) {
    Serial.println("No temperatures stored for tomorrow. Check date format and API response.");
  } else {
    Serial.println("Stored Temperatures for Tomorrow:");
    for (size_t i = 0; i < tomorrowTemperatures.size(); i++) {
      Serial.print(tomorrowTemperatures[i]);
      Serial.println("°C");
    }
    Serial.print("Max Temp for Tomorrow: ");
    Serial.print(maxTemp);
    Serial.println("°C");

    Serial.print("Min Temp for Tomorrow: ");
    Serial.print(minTemp);
    Serial.println("°C");
  }
}

void fetchWeatherData() {
  HTTPClient http;
  String url = "http://api.openweathermap.org/data/2.5/forecast?q=" + Config::city + "&appid=" + Config::apiKey + "&units=metric";
  http.begin(url);

  int httpCode = http.GET();
  if (httpCode == 200) {
    String payload = http.getString();
    Serial.println("API Response:");
    Serial.println(payload);

    DynamicJsonDocument doc(10240);
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }

    JsonArray forecastList = doc["list"];

    // Call specific functions with only necessary data
    parseHourlyTemperature(forecastList);
    getDescriptionForTomorrow(forecastList);
    storeAndCalculateTemperatures(forecastList);
  } else {
    Serial.println("Failed to fetch weather data");
  }

  http.end();
}

#endif
