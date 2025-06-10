#include "lorawan_manager.h"
#include <Preferences.h>

// ===============================================================
// CONSTRUCTOR & DESTRUCTOR
// ===============================================================

LoRaWANManager::LoRaWANManager() :
    radio(nullptr),
    node(nullptr),
    isJoined(false),
    isInitialized(false),
    lastTxTime(0),
    lastJoinAttempt(0),
    joinAttempts(0),
    txCounter(0),
    totalTransmissions(0),
    successfulTransmissions(0),
    failedTransmissions(0),
    totalJoinAttempts(0) {
}

LoRaWANManager::~LoRaWANManager() {
    if (node) delete node;
    if (radio) delete radio;
}

// ===============================================================
// INITIALIZATION
// ===============================================================

bool LoRaWANManager::begin() {
    Serial.println("LoRaWAN Manager: Initializing...");
    
    // Initialize radio hardware
    if (!initializeRadio()) {
        Serial.println("LoRaWAN Manager: Radio initialization failed!");
        return false;
    }
    
    // Parse OTAA credentials
    if (!parseCredentials()) {
        Serial.println("LoRaWAN Manager: Invalid OTAA credentials!");
        return false;
    }
    
    // Try to load previous session
    loadSession();
    
    isInitialized = true;
    Serial.println("LoRaWAN Manager: Initialization successful!");
    return true;
}

bool LoRaWANManager::initializeRadio() {
    // Create radio instance
    radio = new SX1262(new Module(LORA_NSS_PIN, LORA_DIO1_PIN, LORA_RST_PIN, LORA_BUSY_PIN));
    
    if (!radio) {
        Serial.println("LoRaWAN Manager: Failed to create radio instance!");
        return false;
    }
    
    // Initialize SPI
    SPI.begin(LORA_SCK_PIN, LORA_MISO_PIN, LORA_MOSI_PIN, LORA_NSS_PIN);
    
    // Initialize radio
    int state = radio->begin();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print("LoRaWAN Manager: Radio begin failed: ");
        Serial.println(loraErrorToString(state));
        return false;
    }
    
    // Configure radio for LoRaWAN
    radio->setDio2AsRfSwitch(true);
    radio->setCurrentLimit(140.0);  // mA
    
    // Create LoRaWAN node
    node = new LoRaWANNode(radio, &LORAWAN_REGION, LORAWAN_SUBBAND);
    
    if (!node) {
        Serial.println("LoRaWAN Manager: Failed to create LoRaWAN node!");
        return false;
    }
    
    Serial.println("LoRaWAN Manager: Radio initialized successfully!");
    return true;
}

bool LoRaWANManager::parseCredentials() {
    // Parse DevEUI
    if (!hexStringToBytes(LORAWAN_DEVEUI, (uint8_t*)&devEUI, 8)) {
        Serial.println("LoRaWAN Manager: Invalid DevEUI format!");
        return false;
    }
    
    // Parse AppEUI
    if (!hexStringToBytes(LORAWAN_APPEUI, (uint8_t*)&appEUI, 8)) {
        Serial.println("LoRaWAN Manager: Invalid AppEUI format!");
        return false;
    }
    
    // Parse AppKey
    if (!hexStringToBytes(LORAWAN_APPKEY, appKey, 16)) {
        Serial.println("LoRaWAN Manager: Invalid AppKey format!");
        return false;
    }
    
    Serial.println("LoRaWAN Manager: OTAA credentials parsed successfully!");
    Serial.print("DevEUI: ");
    Serial.println(LORAWAN_DEVEUI);
    Serial.print("AppEUI: ");
    Serial.println(LORAWAN_APPEUI);
    
    return true;
}

// ===============================================================
// OTAA JOIN PROCESS
// ===============================================================

bool LoRaWANManager::startJoin() {
    if (!isInitialized) {
        Serial.println("LoRaWAN Manager: Not initialized!");
        return false;
    }
    
    if (isJoined) {
        Serial.println("LoRaWAN Manager: Already joined!");
        return true;
    }
    
    // Check if we should attempt join (rate limiting)
    uint32_t now = millis();
    if (now - lastJoinAttempt < JOIN_RETRY_DELAY) {
        return false; // Too soon to retry
    }
    
    if (joinAttempts >= MAX_JOIN_ATTEMPTS) {
        Serial.println("LoRaWAN Manager: Maximum join attempts reached, restarting...");
        ESP.restart();
        return false;
    }
    
    Serial.println("LoRaWAN Manager: Starting OTAA join...");
    
    // Begin OTAA activation
    int state = node->beginOTAA(devEUI, appEUI, appKey, true);
    
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("LoRaWAN Manager: OTAA join initiated successfully!");
        lastJoinAttempt = now;
        joinAttempts++;
        totalJoinAttempts++;
        return true;
    } else {
        Serial.print("LoRaWAN Manager: OTAA join initiation failed: ");
        Serial.println(loraErrorToString(state));
        return false;
    }
}

bool LoRaWANManager::checkJoinStatus() {
    if (!isInitialized || isJoined) {
        return isJoined;
    }
    
    // Check if join completed
    if (node->isJoined()) {
        isJoined = true;
        joinAttempts = 0; // Reset counter on success
        
        Serial.println("LoRaWAN Manager: OTAA join successful!");
        Serial.print("DevAddr: 0x");
        Serial.println(node->getDevAddr(), HEX);
        
        // Save session for faster reconnection
        saveSession();
        
        return true;
    }
    
    return false;
}

// ===============================================================
// DATA TRANSMISSION
// ===============================================================

bool LoRaWANManager::sendGPSData(const GPSData& gpsData) {
    if (!canTransmit()) {
        return false;
    }
    
    // Encode GPS data
    uint8_t buffer[32];
    size_t length = encodeGPSData(gpsData, buffer);
    
    return sendCustomPayload(buffer, length, LORAWAN_PORT);
}

bool LoRaWANManager::sendGeofenceEvent(const GeofenceEvent& event) {
    if (!canTransmit()) {
        return false;
    }
    
    // Encode geofence event
    uint8_t buffer[32];
    size_t length = encodeGeofenceEvent(event, buffer);
    
    return sendCustomPayload(buffer, length, LORAWAN_PORT);
}

bool LoRaWANManager::sendStatusUpdate(const StatusUpdate& status) {
    if (!canTransmit()) {
        return false;
    }
    
    // Encode status update
    uint8_t buffer[32];
    size_t length = encodeStatusUpdate(status, buffer);
    
    return sendCustomPayload(buffer, length, LORAWAN_PORT);
}

bool LoRaWANManager::sendCustomPayload(uint8_t* payload, size_t length, uint8_t port) {
    if (!canTransmit()) {
        Serial.println("LoRaWAN Manager: Cannot transmit at this time!");
        return false;
    }
    
    Serial.print("LoRaWAN Manager: Sending payload (");
    Serial.print(length);
    Serial.print(" bytes) on port ");
    Serial.println(port);
    
    totalTransmissions++;
    
    // Send uplink
    int state = node->sendReceive(payload, length, port);
    
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("LoRaWAN Manager: Transmission successful!");
        successfulTransmissions++;
        lastTxTime = millis();
        txCounter++;
        saveSession(); // Update session after successful TX
        return true;
    } else {
        Serial.print("LoRaWAN Manager: Transmission failed: ");
        Serial.println(loraErrorToString(state));
        failedTransmissions++;
        return false;
    }
}

// ===============================================================
// STATUS & MONITORING
// ===============================================================

bool LoRaWANManager::canTransmit() const {
    if (!isInitialized || !isJoined) {
        return false;
    }
    
    // Check duty cycle / rate limiting
    uint32_t now = millis();
    if (now - lastTxTime < TX_INTERVAL_MS) {
        return false;
    }
    
    return true;
}

uint32_t LoRaWANManager::getNextTxTime() const {
    if (!isJoined) {
        return 0;
    }
    
    uint32_t elapsed = millis() - lastTxTime;
    if (elapsed >= TX_INTERVAL_MS) {
        return 0; // Can transmit now
    }
    
    return TX_INTERVAL_MS - elapsed;
}

float LoRaWANManager::getSuccessRate() const {
    if (totalTransmissions == 0) {
        return 0.0;
    }
    
    return (float)successfulTransmissions / totalTransmissions * 100.0;
}

void LoRaWANManager::getStatistics(uint32_t& total, uint32_t& success, uint32_t& failed) const {
    total = totalTransmissions;
    success = successfulTransmissions;
    failed = failedTransmissions;
}

// ===============================================================
// SESSION MANAGEMENT
// ===============================================================

void LoRaWANManager::saveSession() {
    Preferences prefs;
    if (prefs.begin(STORAGE_NAMESPACE, false)) {
        prefs.putBool("joined", isJoined);
        prefs.putUInt("tx_counter", txCounter);
        prefs.putUInt("total_tx", totalTransmissions);
        prefs.putUInt("success_tx", successfulTransmissions);
        prefs.putUInt("failed_tx", failedTransmissions);
        prefs.end();
        
        Serial.println("LoRaWAN Manager: Session saved!");
    }
}

bool LoRaWANManager::loadSession() {
    Preferences prefs;
    if (prefs.begin(STORAGE_NAMESPACE, true)) {
        isJoined = prefs.getBool("joined", false);
        txCounter = prefs.getUInt("tx_counter", 0);
        totalTransmissions = prefs.getUInt("total_tx", 0);
        successfulTransmissions = prefs.getUInt("success_tx", 0);
        failedTransmissions = prefs.getUInt("failed_tx", 0);
        prefs.end();
        
        if (isJoined) {
            Serial.println("LoRaWAN Manager: Previous session loaded!");
            return true;
        }
    }
    
    return false;
}

// ===============================================================
// HELPER FUNCTIONS
// ===============================================================

bool hexStringToBytes(const char* hexStr, uint8_t* bytes, size_t maxBytes) {
    size_t hexLen = strlen(hexStr);
    if (hexLen != maxBytes * 2) {
        return false;
    }
    
    for (size_t i = 0; i < maxBytes; i++) {
        char high = hexStr[i * 2];
        char low = hexStr[i * 2 + 1];
        
        if (!isxdigit(high) || !isxdigit(low)) {
            return false;
        }
        
        bytes[i] = ((high >= 'A') ? (high - 'A' + 10) : (high - '0')) << 4;
        bytes[i] |= (low >= 'A') ? (low - 'A' + 10) : (low - '0');
    }
    
    return true;
}

String bytesToHexString(const uint8_t* bytes, size_t length) {
    String result;
    result.reserve(length * 2);
    
    for (size_t i = 0; i < length; i++) {
        if (bytes[i] < 0x10) {
            result += "0";
        }
        result += String(bytes[i], HEX);
    }
    
    result.toUpperCase();
    return result;
}

size_t encodeGPSData(const GPSData& gps, uint8_t* buffer) {
    buffer[0] = MSG_TYPE_GPS_DATA;
    
    // Latitude (4 bytes, big-endian)
    buffer[1] = (gps.latitude >> 24) & 0xFF;
    buffer[2] = (gps.latitude >> 16) & 0xFF;
    buffer[3] = (gps.latitude >> 8) & 0xFF;
    buffer[4] = gps.latitude & 0xFF;
    
    // Longitude (4 bytes, big-endian)
    buffer[5] = (gps.longitude >> 24) & 0xFF;
    buffer[6] = (gps.longitude >> 16) & 0xFF;
    buffer[7] = (gps.longitude >> 8) & 0xFF;
    buffer[8] = gps.longitude & 0xFF;
    
    // Altitude (2 bytes, big-endian)
    buffer[9] = (gps.altitude >> 8) & 0xFF;
    buffer[10] = gps.altitude & 0xFF;
    
    // Satellites and HDOP
    buffer[11] = gps.satellites;
    buffer[12] = gps.hdop;
    
    return 13;
}

size_t encodeGeofenceEvent(const GeofenceEvent& event, uint8_t* buffer) {
    buffer[0] = MSG_TYPE_GEOFENCE_EVENT;
    buffer[1] = event.geofence_id;
    buffer[2] = event.event_type;
    
    // Latitude (4 bytes)
    buffer[3] = (event.latitude >> 24) & 0xFF;
    buffer[4] = (event.latitude >> 16) & 0xFF;
    buffer[5] = (event.latitude >> 8) & 0xFF;
    buffer[6] = event.latitude & 0xFF;
    
    // Longitude (4 bytes)
    buffer[7] = (event.longitude >> 24) & 0xFF;
    buffer[8] = (event.longitude >> 16) & 0xFF;
    buffer[9] = (event.longitude >> 8) & 0xFF;
    buffer[10] = event.longitude & 0xFF;
    
    // Timestamp (4 bytes)
    buffer[11] = (event.timestamp >> 24) & 0xFF;
    buffer[12] = (event.timestamp >> 16) & 0xFF;
    buffer[13] = (event.timestamp >> 8) & 0xFF;
    buffer[14] = event.timestamp & 0xFF;
    
    return 15;
}

String loraErrorToString(int errorCode) {
    switch (errorCode) {
        case RADIOLIB_ERR_NONE: return "Success";
        case RADIOLIB_ERR_CHIP_NOT_FOUND: return "Chip not found";
        case RADIOLIB_ERR_PACKET_TOO_LONG: return "Packet too long";
        case RADIOLIB_ERR_TX_TIMEOUT: return "TX timeout";
        case RADIOLIB_ERR_RX_TIMEOUT: return "RX timeout";
        case RADIOLIB_ERR_CRC_MISMATCH: return "CRC mismatch";
        default: return "Error " + String(errorCode);
    }
}