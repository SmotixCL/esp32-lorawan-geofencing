#ifndef PROJECT_CONFIG_H
#define PROJECT_CONFIG_H

// ===============================================================
// PROJECT INFORMATION
// ===============================================================
#define PROJECT_NAME "ESP32-S3 LoRaWAN Geofencing"
#define PROJECT_VERSION "1.0.0"
#define PROJECT_AUTHOR "Smotix"
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__

// ===============================================================
// HARDWARE CONFIGURATION - ESP32-S3 Heltec WiFi LoRa V3.2
// ===============================================================

// OLED Display SSD1306
#define OLED_SDA_PIN        17
#define OLED_SCL_PIN        18
#define OLED_RST_PIN        21
#define OLED_ADDRESS        0x3C
#define OLED_GEOMETRY       GEOMETRY_128_64

// External Power Control
#define VEXT_PIN            36
#define VEXT_ON_STATE       LOW

// GPS Module
#define GPS_RX_PIN          3
#define GPS_TX_PIN          4
#define GPS_BAUD_RATE       9600
#define GPS_SERIAL_NUM      1

// User Interface
#define LED_WHITE_PIN       35
#define LED_ALERT_PIN       25
#define BUTTON_PIN          0

// Audio Feedback
#define BUZZER_PIN          7
#define BUZZER_RESOLUTION   8
#define BUZZER_CHANNEL      0

// LoRa SX1262 Radio
#define LORA_SCK_PIN        9
#define LORA_MISO_PIN       11
#define LORA_MOSI_PIN       10
#define LORA_NSS_PIN        8
#define LORA_DIO1_PIN       14
#define LORA_RST_PIN        12
#define LORA_BUSY_PIN       13

// ===============================================================
// LORAWAN CONFIGURATION - OTAA MODE
// ===============================================================

// Regional Parameters
#define LORAWAN_REGION      AS923_3  // AS923-3 (915-921 MHz) for Chile
#define LORAWAN_SUBBAND     1        // Sub-band 1
#define LORAWAN_CLASS       CLASS_A  // Class A device

// OTAA Credentials (CHANGE THESE!)
// DevEUI - Device Extended Unique Identifier (16 bytes hex)
#define LORAWAN_DEVEUI      "58EC3C43CA480000"

// AppEUI/JoinEUI - Application Extended Unique Identifier (16 bytes hex)
#define LORAWAN_APPEUI      "0000000000000000"  // Change this

// AppKey - Application Key (32 bytes hex)
#define LORAWAN_APPKEY      "CE8A96F54327D1CB20078F78D4746517"  // Change this

// Communication Settings
#define LORAWAN_PORT        1        // Application port
#define TX_INTERVAL_MS      60000    // 60 seconds between transmissions
#define JOIN_RETRY_DELAY    30000    // 30 seconds between join attempts
#define MAX_JOIN_ATTEMPTS   10       // Maximum join attempts before restart

// ===============================================================
// GPS CONFIGURATION
// ===============================================================
#define GPS_UPDATE_RATE     1000     // GPS reading interval (ms)
#define GPS_TIMEOUT         5000     // GPS timeout (ms)
#define GPS_MIN_SATELLITES  4        // Minimum satellites for valid fix
#define GPS_ACCURACY_THRESHOLD 10.0  // Minimum accuracy in meters

// ===============================================================
// GEOFENCING CONFIGURATION
// ===============================================================
#define MAX_GEOFENCES       5        // Maximum number of geofences
#define GEOFENCE_CHECK_INTERVAL 5000 // Check geofences every 5 seconds
#define GEOFENCE_HYSTERESIS 2.0      // Hysteresis in meters to prevent bouncing

// Default geofence example (Santiago, Chile)
#define DEFAULT_GEOFENCE_LAT    -33.4489
#define DEFAULT_GEOFENCE_LON    -70.6693
#define DEFAULT_GEOFENCE_RADIUS 100.0  // meters

// ===============================================================
// DISPLAY CONFIGURATION
// ===============================================================
#define DISPLAY_UPDATE_RATE     500    // Display update interval (ms)
#define SCREEN_TIMEOUT          30000  // Screen timeout (ms)
#define NUM_SCREENS             4      // Number of display screens
#define BUTTON_DEBOUNCE_TIME    100    // Button debounce (ms)

// ===============================================================
// POWER MANAGEMENT
// ===============================================================
#define DEEP_SLEEP_ENABLED      false  // Enable deep sleep mode
#define SLEEP_INTERVAL_MS       300000 // 5 minutes sleep interval
#define BATTERY_CHECK_INTERVAL  60000  // Check battery every minute
#define LOW_BATTERY_THRESHOLD   3.3    // Low battery voltage threshold

// ===============================================================
// DEBUGGING & MONITORING
// ===============================================================
#ifdef DEBUG
  #define DEBUG_SERIAL_ENABLED  true
  #define DEBUG_BAUD_RATE       115200
  #define LOG_LEVEL            LOG_LEVEL_DEBUG
#else
  #define DEBUG_SERIAL_ENABLED  false
  #define LOG_LEVEL            LOG_LEVEL_INFO
#endif

// Log levels
#define LOG_LEVEL_ERROR     0
#define LOG_LEVEL_WARN      1
#define LOG_LEVEL_INFO      2
#define LOG_LEVEL_DEBUG     3

// ===============================================================
// AUDIO FEEDBACK TONES
// ===============================================================
#define TONE_STARTUP        {1000, 200}
#define TONE_GPS_LOCK       {1500, 100}
#define TONE_LORAWAN_JOIN   {2000, 300}
#define TONE_TX_SUCCESS     {1800, 100}
#define TONE_TX_FAILED      {500, 500}
#define TONE_GEOFENCE_ENTER {2500, 200}
#define TONE_GEOFENCE_EXIT  {800, 300}
#define TONE_ERROR          {400, 1000}

// ===============================================================
// STORAGE KEYS (EEPROM/Preferences)
// ===============================================================
#define STORAGE_NAMESPACE   "geofence"
#define KEY_DEVICE_CONFIG   "device_cfg"
#define KEY_GEOFENCES       "geofences"
#define KEY_STATISTICS      "stats"
#define KEY_LORAWAN_SESSION "lw_session"

// ===============================================================
// NETWORK TIMEOUTS
// ===============================================================
#define LORAWAN_TX_TIMEOUT      30000   // LoRaWAN transmission timeout
#define LORAWAN_RX_TIMEOUT      5000    // LoRaWAN receive timeout
#define WIFI_CONNECT_TIMEOUT    15000   // WiFi connection timeout (if used)

// ===============================================================
// MESSAGE TYPES FOR LORAWAN PAYLOAD
// ===============================================================
#define MSG_TYPE_GPS_DATA       0x01
#define MSG_TYPE_GEOFENCE_EVENT 0x02
#define MSG_TYPE_STATUS_UPDATE  0x03
#define MSG_TYPE_ALERT          0x04
#define MSG_TYPE_HEARTBEAT      0x05

#endif // PROJECT_CONFIG_H