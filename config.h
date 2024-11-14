#ifndef CONFIG_H
#define CONFIG_H

namespace Config {

    const char* apiKey = "c86b3c8c49a759cd21dbe280885047d9";
    const String city = "Santiago,CL";

    struct WiFiCredentials {
        const char* ssid;
        const char* password;
    };

    const WiFiCredentials wifiNetworks[] = {
        {"Desarrollo", "Desarrollo2022"},
        {"VTR-0663620", "Sc3rwcyrwfqn"},
        {"Room24", "Luanita24"}
    };

    const int wifiNetworkCount = sizeof(wifiNetworks) / sizeof(wifiNetworks[0]);

    const char* NTP_SERVER = "pool.ntp.org";
    const long GMT_OFFSET_SEC = -10800; // -10800 at the office | -14400 at home
    const int DAYLIGHT_OFFSET_SEC = 3600;


    String latitude = "-33.3817366";
    String longitude = "-70.7444498";
    String location = "Santiago";
    String timezone = "America/Santiago";
}

#endif // CONFIG_H
