#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <Arduino.h>

// WiFi
#define WIFI_SSID "Huize Heysterbach"
#define WIFI_PASSWORD "VPYQew9NUfb5Kz"

// Seeed Vision AI / SSCMA
#define SSCMA_RX_PIN D7
#define SSCMA_TX_PIN D6
#define SSCMA_MIN_SCORE 60
#define SSCMA_DEVICE_TSCORE 60

// GPS
#define GPS_ENABLED 1
#define GPS_RAW_DEBUG 1

// Account login
#define API_BASE_URL "https://avansafvalapi-production.up.railway.app"
#define ACCOUNT_LOGIN_EMAIL "lukaverzijl@hotmail.com"
#define ACCOUNT_LOGIN_PASSWORD "Test12345!"

// Detection report API
#define REPORT_PATH "/trash"
#define REPORT_REFRESH_TOKEN_HEADER "X-Refresh-Token"
#define REPORT_DETECTION_COUNT 3
#define REPORT_COOLDOWN_MS 30000
#define REPORT_TLS_INSECURE 1

// Weather API
#define OPEN_METEO_BASE_URL "https://api.open-meteo.com"

// Time sync for captureDate
#define TIME_NTP_SERVER "pool.ntp.org"

#endif
