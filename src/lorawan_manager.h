#ifndef LORAWAN_MANAGER_H
#define LORAWAN_MANAGER_H

#include <Arduino.h>
#include <RadioLib.h>
#include "../include/project_config.h"

// ===============================================================
// LORAWAN MESSAGE STRUCTURES
// ===============================================================

struct GPSData {
    int32_t latitude;      // * 1e6
    int32_t longitude;     // * 1e6
    int16_t altitude;      // meters
    uint8_t satellites;
    uint8_t hdop;          // Horizontal dilution of precision
};

struct GeofenceEvent {
    uint8_t geofence_id;
    uint8_t event_type;    // 0=exit, 1=enter
    int32_t latitude;      // * 1e6
    int32_t longitude;     // * 1e6
    uint32_t timestamp;
};

struct StatusUpdate {
    uint8_t battery_level;
    uint16_t uptime_hours;
    uint8_t gps_status;
    uint8_t system_status;
};

// ===============================================================
// LORAWAN MANAGER CLASS
// ===============================================================

class LoRaWANManager {
private:
    SX1262* radio;
    LoRaWANNode* node;
    
    // OTAA Credentials
    uint64_t devEUI;
    uint64_t appEUI;
    uint8_t appKey[16];
    
    // Status tracking
    bool isJoined;
    bool isInitialized;
    uint32_t lastTxTime;
    uint32_t lastJoinAttempt;
    uint8_t joinAttempts;
    uint32_t txCounter;
    
    // Statistics
    uint32_t totalTransmissions;
    uint32_t successfulTransmissions;
    uint32_t failedTransmissions;
    uint32_t totalJoinAttempts;
    
    // Private methods
    bool initializeRadio();
    bool parseCredentials();
    void saveSession();
    bool loadSession();
    void resetSession();
    
public:
    // Constructor & Destructor
    LoRaWANManager();
    ~LoRaWANManager();
    
    // Initialization
    bool begin();
    bool configure();
    
    // OTAA Join Process
    bool startJoin();
    bool isJoinInProgress();
    bool checkJoinStatus();
    void handleJoinResult(int state);
    
    // Data Transmission
    bool sendGPSData(const GPSData& gpsData);
    bool sendGeofenceEvent(const GeofenceEvent& event);
    bool sendStatusUpdate(const StatusUpdate& status);
    bool sendCustomPayload(uint8_t* payload, size_t length, uint8_t port = LORAWAN_PORT);
    
    // Status & Monitoring
    bool isConnected() const { return isJoined; }
    bool canTransmit() const;
    uint32_t getNextTxTime() const;
    uint32_t getTxCounter() const { return txCounter; }
    float getSuccessRate() const;
    
    // Statistics
    void getStatistics(uint32_t& total, uint32_t& success, uint32_t& failed) const;
    void resetStatistics();
    
    // Downlink handling
    bool hasDownlink();
    bool getDownlink(uint8_t* buffer, size_t& length, uint8_t& port);
    void processDownlink(uint8_t* payload, size_t length, uint8_t port);
    
    // Configuration
    void setTxInterval(uint32_t intervalMs);
    void setTxPower(int8_t power);
    void setDataRate(uint8_t dr);
    
    // Sleep/Wake management
    void sleep();
    void wake();
    
    // Debug & Logging
    void printStatus();
    void printStatistics();
    String getStatusString();
};

// ===============================================================
// HELPER FUNCTIONS
// ===============================================================

// Convert hex string to byte array
bool hexStringToBytes(const char* hexStr, uint8_t* bytes, size_t maxBytes);

// Convert byte array to hex string
String bytesToHexString(const uint8_t* bytes, size_t length);

// Payload encoding helpers
size_t encodeGPSData(const GPSData& gps, uint8_t* buffer);
size_t encodeGeofenceEvent(const GeofenceEvent& event, uint8_t* buffer);
size_t encodeStatusUpdate(const StatusUpdate& status, uint8_t* buffer);

// Error code to string
String loraErrorToString(int errorCode);

#endif // LORAWAN_MANAGER_H