#include <lvgl.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <vector>
#include "config.h"
#include "weather_functions.h"
#include "tomorrow_weather_functions.h"

// ---------------------------------------------------------
#include <DHT.h>
#include <DHT_U.h>

// Define DHT settings
#define DHTPIN 27
#define DHTTYPE DHT11 
DHT dht(DHTPIN, DHTTYPE);  // Create a DHT object

float dht_temperature = 0.0;
float dht_humidity = 0.0;

// ---------------------------------------------------------



std::vector<float> tomorrowTemperatures; 
std::vector<String> weather_descriptions_tomorrow;

String weather_description_tomorrow;
String current_date;
String last_weather_update;
String temperature;
String humidity;
int weather_code = 0;
const char degree_symbol[] = "\u00B0C";

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

void log_print(lv_log_level_t level, const char * buf) {
  LV_UNUSED(level);
  Serial.println(buf);
  Serial.flush();
}


static lv_obj_t * text_label_local_temp;
static lv_obj_t * text_label_local_hum;

static lv_obj_t * text_label_date;
static lv_obj_t * text_label_temperature;
static lv_obj_t * text_label_humidity;
static lv_obj_t * text_label_weather_description;
static lv_obj_t * text_label_time_location;
lv_obj_t *timeLabel;
static lv_obj_t * text_label_maxTemp;
static lv_obj_t * text_label_minTemp;
static lv_obj_t * text_label_tomorrow_weather_description;
float maxTemp = -100.0;
float minTemp = 100.0;
static lv_obj_t* wifi_signal_bars[4];
static lv_obj_t * text_label_wifi_ssid;



static void timer_cb(lv_timer_t * timer){
  LV_UNUSED(timer);
  // Read DHT data
  read_dht_data();

  // Update DHT display
  update_dht_display();

  get_weather_data();
  get_weather_description(weather_code);
  lv_label_set_text(text_label_date, current_date.c_str());
  lv_label_set_text(text_label_temperature, String("      " + temperature + degree_symbol).c_str());
  lv_label_set_text(text_label_humidity, String("   " + humidity + "%").c_str());
  lv_label_set_text(text_label_weather_description, weather_description.c_str());
  lv_label_set_text(text_label_time_location, String("Last Update: " + last_weather_update + "  |  " + Config::location).c_str());

  if (!weather_descriptions_tomorrow.empty()) {
    lv_label_set_text(text_label_tomorrow_weather_description, ("Tomorrow:  " + weather_descriptions_tomorrow[0]).c_str());
  } else {
    lv_label_set_text(text_label_tomorrow_weather_description, "...");
  }
  
  if (!tomorrowTemperatures.empty()) {
      maxTemp = *std::max_element(tomorrowTemperatures.begin(), tomorrowTemperatures.end());
      minTemp = *std::min_element(tomorrowTemperatures.begin(), tomorrowTemperatures.end());
      lv_label_set_text(text_label_maxTemp, String("Max: " + String(maxTemp, 0) + degree_symbol).c_str());
      lv_label_set_text(text_label_minTemp, String("Min: " + String(minTemp, 0) + degree_symbol).c_str());
  }
  update_wifi_signal();
}

void lv_create_main_gui(void) {

 // ------------------- MAIN BACKGROUND GRADIENT ----------------------------------------------------------
  static lv_style_t style_bg;
  lv_style_init(&style_bg);
  lv_style_set_bg_color(&style_bg, lv_color_make(17, 24, 39));       // Start color (top)
  lv_style_set_bg_grad_color(&style_bg, lv_color_make(17, 24, 39));     // End color (bottom)
  lv_style_set_bg_grad_dir(&style_bg, LV_GRAD_DIR_VER);               // Vertical gradient
  lv_style_set_bg_opa(&style_bg, LV_OPA_COVER); // Fully opaque
  lv_obj_t *main_screen = lv_scr_act();
  lv_obj_add_style(main_screen, &style_bg, 0);
  LV_IMAGE_DECLARE(image_weather_sun);
  LV_IMAGE_DECLARE(image_weather_cloud);
  LV_IMAGE_DECLARE(image_weather_rain);
  LV_IMAGE_DECLARE(image_weather_thunder);
  LV_IMAGE_DECLARE(image_weather_snow);
  LV_IMAGE_DECLARE(image_weather_night);
  LV_IMAGE_DECLARE(image_weather_temperature);
  LV_IMAGE_DECLARE(image_weather_humidity);

 // ------------------- WEATHER RECTANGLE -----------------------------------------------------------------
  static lv_style_t style_rect;
  lv_style_init(&style_rect);
  lv_style_set_bg_color(&style_rect, lv_color_make(31, 41, 55));
  lv_style_set_bg_opa(&style_rect, LV_OPA_COVER); // Fully opaque background
  lv_style_set_border_width(&style_rect, 0); // No border
  lv_style_set_radius(&style_rect, 10); // Adjust as needed
  lv_obj_t *rect = lv_obj_create(lv_scr_act());
  lv_obj_add_style(rect, &style_rect, 0);
  lv_obj_set_size(rect, 308, 88);  // Width, Height
  lv_obj_set_pos(rect, 5, 81); // Alignment

 // ------------------- TOMORROW WEATHER RECTANGLE --------------------------------------------------------
  static lv_style_t style_rect3;
  lv_style_init(&style_rect3);
  lv_style_set_bg_color(&style_rect3, lv_color_make(31, 41, 55));
  lv_style_set_bg_opa(&style_rect3, LV_OPA_COVER); // Fully opaque background
  lv_style_set_border_width(&style_rect3, 0); // No border
  lv_style_set_radius(&style_rect3, 10); // Adjust as needed
  lv_obj_t *rect3 = lv_obj_create(lv_scr_act());
  lv_obj_add_style(rect3, &style_rect3, 0);
  lv_obj_set_size(rect3, 308, 58);  // Width, Height
  lv_obj_set_pos(rect3, 5, 176); // Alignment

 // ------------------- WEATHER IMAGE ---------------------------------------------------------------------
  weather_image = lv_image_create(lv_screen_active());
  lv_obj_set_pos(weather_image, -5, 60);
  lv_img_set_zoom(weather_image, 128); // Set ZOOM_VALUE to 128 or as needed
  get_weather_description(weather_code);

 // ------------------- DATE LABEL ------------------------------------------------------------------------
  struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        char date_str[20];
        char day_str[10]; 

        strftime(date_str, sizeof(date_str), "%d/%m/%Y", &timeinfo);
        strftime(day_str, sizeof(day_str), "%a", &timeinfo);
        current_date = String(day_str) + " " + String(date_str);
    }

    text_label_date = lv_label_create(lv_scr_act());
    lv_label_set_text(text_label_date, current_date.c_str());
    lv_obj_set_pos(text_label_date, 5, 3);
    lv_obj_set_style_text_font((lv_obj_t*) text_label_date, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color((lv_obj_t*) text_label_date, lv_color_make(202, 202, 202), 0);

 //-------------------- TEMPERATURE ICON ------------------------------------------------------------------
  static lv_style_t style_temperature_icon;
  lv_style_init(&style_temperature_icon);
  lv_style_set_img_recolor(&style_temperature_icon, lv_color_make(255, 0, 0)); // Red color
  lv_style_set_img_recolor_opa(&style_temperature_icon, LV_OPA_COVER); // Fully opaque

  lv_obj_t * weather_image_temperature = lv_image_create(lv_screen_active());
  lv_image_set_src(weather_image_temperature, &image_weather_temperature);
  lv_obj_set_pos(weather_image_temperature, 115, 114);
  lv_obj_add_style(weather_image_temperature, &style_temperature_icon, 0);

 // ------------------- TEMPERATURE LABEL -----------------------------------------------------------------
  text_label_temperature = lv_label_create(lv_screen_active());
  lv_obj_set_style_text_color(text_label_temperature, lv_color_make(202, 202, 202), 0); // Temperature color
  lv_label_set_text(text_label_temperature, String("     " + temperature + degree_symbol).c_str());
  lv_obj_set_pos(text_label_temperature, 110, 114);
  lv_obj_set_style_text_font((lv_obj_t*) text_label_temperature, &lv_font_montserrat_22, 0);

 // ------------------- HUMIDITY ICON ---------------------------------------------------------------------
  static lv_style_t style_humidity_icon;
  lv_style_init(&style_humidity_icon);

  lv_style_set_img_recolor(&style_humidity_icon, lv_color_make(0, 0, 255)); // Humidity icon color
  lv_style_set_img_recolor_opa(&style_humidity_icon, LV_OPA_COVER); // Fully opaque

  lv_obj_t * weather_image_humidity = lv_image_create(lv_screen_active());
  lv_image_set_src(weather_image_humidity, &image_weather_humidity);
  lv_obj_set_pos(weather_image_humidity, 210, 114);
  lv_obj_add_style(weather_image_humidity, &style_humidity_icon, 0);

 // ------------------- HUMIDITY LABEL --------------------------------------------------------------------
  text_label_humidity = lv_label_create(lv_screen_active());
  lv_obj_set_style_text_color(text_label_humidity, lv_color_make(202, 202, 202), 0); // Temperature color
  lv_label_set_text(text_label_humidity, String("   " + humidity + "%").c_str());
  lv_obj_set_pos(text_label_humidity, 230, 114);
  lv_obj_set_style_text_font((lv_obj_t*) text_label_humidity, &lv_font_montserrat_22, 0);

 // ------------------- WEATHER DESCRIPTION ---------------------------------------------------------------
  text_label_weather_description = lv_label_create(lv_screen_active());
  lv_obj_set_style_text_color(text_label_weather_description, lv_color_make(202, 202, 202), 0);
  lv_label_set_text(text_label_weather_description, weather_description.c_str());
  lv_obj_set_pos(text_label_weather_description, 125, 87);
  //lv_obj_align(text_label_weather_description, LV_ALIGN_BOTTOM_MID, 0, -40);
  lv_obj_set_style_text_font((lv_obj_t*) text_label_weather_description, &lv_font_montserrat_18, 0);

 // ------------------- TIMEZONE --------------------------------------------------------------------------
  text_label_time_location = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_time_location, String("Updated: " + last_weather_update + "  |  " + Config::location).c_str());
  lv_obj_set_pos(text_label_time_location, 120, 148);
  lv_obj_set_style_text_font((lv_obj_t*) text_label_time_location, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color((lv_obj_t*) text_label_time_location, lv_color_make(202, 202, 202), 0);
  lv_timer_t * timer = lv_timer_create(timer_cb, 600000, NULL);
  lv_timer_ready(timer);

 // ------------------- TIME JMD --------------------------------------------------------------------------
  timeLabel = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_color(timeLabel, lv_color_make(202, 202, 202), 0);
  lv_obj_set_pos(timeLabel, 6, 27);
  lv_obj_set_style_text_font(timeLabel, &lv_font_montserrat_40, LV_STATE_DEFAULT);

 // ------------------- TOMORROW WEATHER TEMPERATURES -----------------------------------------------------
  text_label_maxTemp = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_color(text_label_maxTemp, lv_color_make(202, 202, 202), 0); // Color for Max Temp (e.g., Red)
  lv_label_set_text(text_label_maxTemp, "Max: --"); // Initial placeholder text
  lv_obj_set_pos(text_label_maxTemp, 120, 207); // Set position
  lv_obj_set_style_text_font((lv_obj_t*) text_label_maxTemp, &lv_font_montserrat_18, 0);
  
  text_label_minTemp = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_color(text_label_minTemp, lv_color_make(202, 202, 202), 0); // Color for Min Temp (e.g., Blue)
  lv_label_set_text(text_label_minTemp, "Min: --"); // Initial placeholder text
  lv_obj_set_pos(text_label_minTemp, 20, 207); // Set position
  lv_obj_set_style_text_font((lv_obj_t*) text_label_minTemp, &lv_font_montserrat_18, 0);

 // --------------------TOMORROW WEATHER DESCRIPTION ------------------------------------------------------
  text_label_tomorrow_weather_description = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_color(text_label_tomorrow_weather_description, lv_color_make(202, 202, 202), 0);
  lv_label_set_text(text_label_tomorrow_weather_description, "Weather: --");
  lv_obj_set_pos(text_label_tomorrow_weather_description, 20, 181);
  lv_obj_set_style_text_font((lv_obj_t*) text_label_tomorrow_weather_description, &lv_font_montserrat_18, 0);

 // ------------------- WIFI SIGNAL -----------------------------------------------------------------------
  int dotSize = 10;    // Diameter of each dot
  int dotSpacing = 1;  // Spacing between dots
  int dotX = 270;      // X position on screen (adjust as needed)
  int dotY = 6;        // Y position on screen (adjust as needed)

  for (int i = 0; i < 4; i++) {
    wifi_signal_bars[i] = lv_obj_create(lv_scr_act());
    lv_obj_set_size(wifi_signal_bars[i], dotSize, dotSize);   // Set as a dot (circle)
    lv_obj_set_pos(wifi_signal_bars[i], dotX + i * (dotSize + dotSpacing), dotY);

    // Make each dot circular by setting the radius to half the diameter
    lv_obj_set_style_radius(wifi_signal_bars[i], dotSize / 2, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Initial color for empty dots (gray)
    lv_obj_set_style_bg_color(wifi_signal_bars[i], lv_color_make(17, 24, 39), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(wifi_signal_bars[i], LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  }

 // ------------------- WIFI SSID LABEL -------------------------------------------------------------------
  text_label_wifi_ssid = lv_label_create(lv_scr_act());
  lv_label_set_text(text_label_wifi_ssid, ("SSID: " + WiFi.SSID()).c_str()); // Initial SSID text
  lv_obj_set_pos(text_label_wifi_ssid, 130, 3);  // Position the label as needed
  lv_obj_set_style_text_font(text_label_wifi_ssid, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(text_label_wifi_ssid, lv_color_make(202, 202, 202), 0);

 // ------------------- LOCAL TEMPERATURE AND HUMIDITY ---------------------------------------------------
  text_label_local_temp = lv_label_create(lv_scr_act());
  text_label_local_hum = lv_label_create(lv_scr_act());

  lv_label_set_text(text_label_local_temp, String("Temp: " + String(dht_temperature) + degree_symbol).c_str());
  lv_obj_set_pos(text_label_local_temp, 190, 30); 
  lv_obj_set_style_text_font(text_label_local_temp, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(text_label_local_temp, lv_color_make(202, 202, 202), 0);

  lv_label_set_text(text_label_local_hum, String("Hum: " + String(dht_humidity) + "%").c_str());
  lv_obj_set_pos(text_label_local_hum, 190, 50);
  lv_obj_set_style_text_font(text_label_local_hum, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(text_label_local_hum, lv_color_make(202, 202, 202), 0); 
}

void update_time_label() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        char date_str[11];  // Array to store the date in format "DD/MM/YYYY"
        char day_str[10];   // Array to store the day name (e.g., "Monday")
        char time_str[9];   // Array to store the time in format "HH:MM:SS"

        strftime(date_str, sizeof(date_str), "%d/%m/%Y", &timeinfo);
        strftime(day_str, sizeof(day_str), "%a", &timeinfo);
        strftime(time_str, sizeof(time_str), "%H:%M:%S", &timeinfo);
        String date_day_str = String(day_str) + " " + String(date_str);
        lv_label_set_text(text_label_date, date_day_str.c_str());  
        lv_label_set_text(timeLabel, time_str); 
    }
}

void connectToWiFi() {
    for (int i = 0; i < Config::wifiNetworkCount; i++) {
        Serial.print("Attempting to connect to ");
        Serial.println(Config::wifiNetworks[i].ssid);

        WiFi.begin(Config::wifiNetworks[i].ssid, Config::wifiNetworks[i].password);
        
        int attemptCount = 0;
        while (WiFi.status() != WL_CONNECTED && attemptCount < 10) { // retry up to 10 times per network
            delay(500);
            Serial.print(".");
            attemptCount++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nConnected to Wi-Fi network with IP Address: ");
            Serial.println(WiFi.localIP());
            return;
        }

        Serial.println("\nFailed to connect to this network.");
    }

    Serial.println("Could not connect to any available networks.");
}

void update_wifi_signal() {
    int32_t rssi = WiFi.RSSI();
    int signalLevel;

    if (rssi > -50) {
        signalLevel = 4; // Excellent
    } else if (rssi > -60) {
        signalLevel = 3; // Good
    } else if (rssi > -85) {
        signalLevel = 2; // Fair
    } else if (rssi > -100) {
        signalLevel = 1; // Weak
    } else {
        signalLevel = 0; // No signal
    }

    //Serial.print("RSSI: ");
    //Serial.println(rssi);

    // Update each dot based on the signal level
    for (int i = 0; i < 4; i++) {
        if (i < signalLevel) {
            // Filled color for active dots (e.g., Green)
            lv_obj_set_style_bg_color(wifi_signal_bars[i], lv_color_make(202, 202, 202), LV_PART_MAIN | LV_STATE_DEFAULT);
        } else {
            // Gray color for inactive (empty) dots
            lv_obj_set_style_bg_color(wifi_signal_bars[i], lv_color_make(17, 24, 39), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        lv_obj_set_style_bg_opa(wifi_signal_bars[i], LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
  lv_label_set_text(text_label_wifi_ssid, ("SSID: " + WiFi.SSID()).c_str());
}

void read_dht_data() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  if (isnan(temp) || isnan(hum)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  dht_temperature = temp;
  dht_humidity = hum;

  Serial.print("Temperature: ");
  Serial.print(dht_temperature);
  Serial.print("°C  Hum: ");
  Serial.print(dht_humidity);
  Serial.println("%");
}

void update_dht_display() {
  lv_label_set_text(text_label_local_temp, String("Temp: " + String(dht_temperature, 1) + degree_symbol).c_str());
  lv_label_set_text(text_label_local_hum, String("Hum: " + String(dht_humidity, 1) + "%").c_str());
}

void setup() {
  String LVGL_Arduino = String("LVGL Library Version: ") + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  Serial.begin(115200);
  Serial.println(LVGL_Arduino);

  dht.begin();

  connectToWiFi();

  Serial.print("\nConnected to Wi-Fi network with IP Address: ");
  Serial.println(WiFi.localIP());

  lv_init();
  lv_log_register_print_cb(log_print);
  lv_display_t * disp;
  disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);

  configTime(Config::GMT_OFFSET_SEC, Config::DAYLIGHT_OFFSET_SEC, Config::NTP_SERVER);
  lv_timer_create([](lv_timer_t * timer) {
    update_time_label();
  }, 1000, NULL); 

  fetchWeatherData();
  get_weather_data();
  lv_create_main_gui();
}

void loop() {
  lv_task_handler(); 
  lv_tick_inc(5);   
  delay(5);
  update_wifi_signal();         
}
