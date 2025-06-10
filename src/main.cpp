// ===============================================================
// ESP32-S3 LoRaWAN Geofencing Project - Main Application
// ===============================================================

#include <Arduino.h>
#include "../include/project_config.h"
#include "lorawan_manager.h"
#include "gps_manager.h"
#include "display_manager.h"
#include "audio_manager.h"
#include "geofence_manager.h"

// ===============================================================
// GLOBAL MANAGERS
// ===============================================================
LoRaWANManager loraManager;
GPSManager gpsManager;
DisplayManager displayManager;
AudioManager audioManager;
GeofenceManager geofenceManager;

// ===============================================================
// SYSTEM STATE
// ===============================================================
struct SystemState {
    bool systemInitialized;
    bool lorawanJoined;
    bool gpsLocked;
    uint8_t currentScreen;
    unsigned long lastScreenUpdate;
    unsigned long lastButtonCheck;
    unsigned long lastStatusCheck;
    unsigned long systemStartTime;
    uint32_t systemLoopCount;
} systemState;

// ===============================================================
// FUNCTION DECLARATIONS
// ===============================================================
void setupSystem();
void setupManagers();
void handleSystemLoop();
void handleUserInput();
void handleLoRaWANEvents();
void handleGPSEvents();
void handleGeofenceEvents();
void updateSystemStatus();
void performSystemMaintenance();
void printSystemInfo();

// ===============================================================
// ARDUINO SETUP
// ===============================================================
void setup() {
    // Initialize serial communication
    Serial.begin(DEBUG_BAUD_RATE);
    delay(2000); // Wait for serial to stabilize
    
    printSystemInfo();
    
    // Setup system hardware
    setupSystem();
    
    // Initialize all managers
    setupManagers();
    
    // System ready
    systemState.systemInitialized = true;
    systemState.systemStartTime = millis();
    
    Serial.println("=== SYSTEM READY ===");
    audioManager.playStartupTone();
}

// ===============================================================
// ARDUINO MAIN LOOP
// ===============================================================
void loop() {
    // Update system loop counter
    systemState.systemLoopCount++;
    
    // Handle main system operations
    handleSystemLoop();
    
    // Handle user interactions
    handleUserInput();
    
    // Handle LoRaWAN communication
    handleLoRaWANEvents();
    
    // Handle GPS updates
    handleGPSEvents();
    
    // Handle geofencing logic
    handleGeofenceEvents();
    
    // Update system status periodically
    if (millis() - systemState.lastStatusCheck >= 5000) {
        updateSystemStatus();
        systemState.lastStatusCheck = millis();
    }
    
    // Perform maintenance tasks
    performSystemMaintenance();
    
    // Small delay for system stability
    delay(10);
}

// ===============================================================
// SYSTEM INITIALIZATION
// ===============================================================
void setupSystem() {
    Serial.println("Setting up system hardware...");
    
    // Initialize system state
    memset(&systemState, 0, sizeof(systemState));
    
    // Configure GPIO pins
    pinMode(LED_WHITE_PIN, OUTPUT);
    pinMode(LED_ALERT_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    // Initial LED states
    digitalWrite(LED_WHITE_PIN, LOW);
    digitalWrite(LED_ALERT_PIN, HIGH); // Alert LED on during initialization
    
    // External power control for peripherals
    pinMode(VEXT_PIN, OUTPUT);
    digitalWrite(VEXT_PIN, VEXT_ON_STATE);
    delay(100);
    
    Serial.println("System hardware setup complete!");
}

void setupManagers() {
    Serial.println("Initializing system managers...");
    
    // Initialize Audio Manager (for feedback during setup)
    if (!audioManager.begin()) {
        Serial.println("WARNING: Audio Manager initialization failed!");
    }
    
    // Initialize Display Manager
    if (!displayManager.begin()) {
        Serial.println("ERROR: Display Manager initialization failed!");
        while(true) {
            digitalWrite(LED_ALERT_PIN, !digitalRead(LED_ALERT_PIN));
            delay(200);
        }
    }
    
    // Show initialization screen
    displayManager.showInitScreen(PROJECT_NAME, PROJECT_VERSION);
    
    // Initialize GPS Manager
    if (!gpsManager.begin()) {
        Serial.println("WARNING: GPS Manager initialization failed!");
        displayManager.showError("GPS Init Failed");
        delay(2000);
    }
    
    // Initialize LoRaWAN Manager
    displayManager.showStatus("Initializing LoRaWAN...");
    if (!loraManager.begin()) {
        Serial.println("ERROR: LoRaWAN Manager initialization failed!");
        displayManager.showError("LoRaWAN Init Failed");
        audioManager.playErrorTone();
        delay(5000);
        ESP.restart();
    }
    
    // Initialize Geofence Manager
    if (!geofenceManager.begin()) {
        Serial.println("WARNING: Geofence Manager initialization failed!");
    }
    
    // Start LoRaWAN join process
    displayManager.showStatus("Starting OTAA Join...");
    if (loraManager.startJoin()) {
        Serial.println("LoRaWAN OTAA join initiated!");
    } else {
        Serial.println("Failed to initiate LoRaWAN join!");
        displayManager.showError("Join Failed");
        audioManager.playErrorTone();
    }
    
    // Turn off alert LED
    digitalWrite(LED_ALERT_PIN, LOW);
    
    Serial.println("Manager initialization complete!");
}

// ===============================================================
// MAIN LOOP HANDLERS
// ===============================================================
void handleSystemLoop() {
    // Update display
    if (millis() - systemState.lastScreenUpdate >= DISPLAY_UPDATE_RATE) {
        updateDisplayContent();
        systemState.lastScreenUpdate = millis();
    }
}

void handleUserInput() {
    if (millis() - systemState.lastButtonCheck >= BUTTON_DEBOUNCE_TIME) {
        static bool lastButtonState = true; // Pulled up
        bool currentButtonState = digitalRead(BUTTON_PIN);
        
        // Button pressed (falling edge)
        if (!currentButtonState && lastButtonState) {
            systemState.currentScreen = (systemState.currentScreen + 1) % NUM_SCREENS;
            Serial.print("Screen changed to: ");
            Serial.println(systemState.currentScreen);
            audioManager.playClickTone();
        }
        
        lastButtonState = currentButtonState;
        systemState.lastButtonCheck = millis();
    }
}

void handleLoRaWANEvents() {
    // Check if we need to start/retry join
    if (!loraManager.isConnected()) {
        // Try to join if not already joined
        if (!loraManager.isJoinInProgress()) {
            loraManager.startJoin();
        }
    }
    
    // Check join status
    if (loraManager.checkJoinStatus() && !systemState.lorawanJoined) {
        systemState.lorawanJoined = true;
        Serial.println("LoRaWAN joined successfully!");
        audioManager.playJoinSuccessTone();
        displayManager.showStatus("LoRaWAN Joined!");
        delay(1000);
    }
    
    // Handle data transmission
    if (loraManager.isConnected() && loraManager.canTransmit()) {
        // Send GPS data if available
        if (gpsManager.hasValidFix()) {
            GPSData gpsData = gpsManager.getCurrentData();
            
            digitalWrite(LED_WHITE_PIN, HIGH);
            if (loraManager.sendGPSData(gpsData)) {
                Serial.println("GPS data sent successfully!");
                audioManager.playTxSuccessTone();
            } else {
                Serial.println("Failed to send GPS data!");
                audioManager.playTxFailedTone();
            }
            digitalWrite(LED_WHITE_PIN, LOW);
        }
    }
}

void handleGPSEvents() {
    // Update GPS data
    gpsManager.update();
    
    // Check for GPS lock status change
    bool currentGpsLock = gpsManager.hasValidFix();
    if (currentGpsLock != systemState.gpsLocked) {
        systemState.gpsLocked = currentGpsLock;
        
        if (systemState.gpsLocked) {
            Serial.println("GPS lock acquired!");
            audioManager.playGPSLockTone();
        } else {
            Serial.println("GPS lock lost!");
        }
    }
}

void handleGeofenceEvents() {
    // Update geofences if GPS is available
    if (gpsManager.hasValidFix()) {
        GPSData currentPos = gpsManager.getCurrentData();
        
        // Check for geofence events
        GeofenceEvent event;
        if (geofenceManager.checkGeofences(currentPos.latitude / 1e6, currentPos.longitude / 1e6, event)) {
            Serial.print("Geofence event: ");
            Serial.print(event.event_type == 1 ? "ENTER" : "EXIT");
            Serial.print(" fence ");
            Serial.println(event.geofence_id);
            
            // Send geofence event via LoRaWAN
            if (loraManager.isConnected()) {
                loraManager.sendGeofenceEvent(event);
            }
            
            // Audio feedback
            if (event.event_type == 1) {
                audioManager.playGeofenceEnterTone();
            } else {
                audioManager.playGeofenceExitTone();
            }
        }
    }
}

void updateSystemStatus() {
    // Update managers
    gpsManager.update();
    
    // Update display based on current screen
    updateDisplayContent();
    
    // Check system health
    if (ESP.getFreeHeap() < 10000) { // Less than 10KB free
        Serial.println("WARNING: Low memory!");
    }
}

void updateDisplayContent() {
    switch (systemState.currentScreen) {
        case 0:
            displayManager.showMainScreen(
                loraManager.isConnected(),
                gpsManager.hasValidFix(),
                gpsManager.getCurrentData(),
                loraManager.getTxCounter()
            );
            break;
            
        case 1:
            displayManager.showLoRaWANScreen(
                loraManager.isConnected(),
                loraManager.getTxCounter(),
                loraManager.getSuccessRate(),
                loraManager.getNextTxTime()
            );
            break;
            
        case 2:
            displayManager.showGPSScreen(
                gpsManager.getCurrentData(),
                gpsManager.getSatelliteCount(),
                gpsManager.getHDOP()
            );
            break;
            
        case 3:
            displayManager.showSystemScreen(
                PROJECT_VERSION,
                millis() - systemState.systemStartTime,
                ESP.getFreeHeap(),
                systemState.systemLoopCount
            );
            break;
    }
}

void performSystemMaintenance() {
    static unsigned long lastMaintenance = 0;
    
    if (millis() - lastMaintenance >= 60000) { // Every minute
        // Save system state
        // loraManager.saveSession(); // Already done automatically
        
        // Print statistics
        if (DEBUG_SERIAL_ENABLED) {
            Serial.println("=== SYSTEM STATISTICS ===");
            loraManager.printStatistics();
            gpsManager.printStatistics();
        }
        
        lastMaintenance = millis();
    }
}

// ===============================================================
// UTILITY FUNCTIONS
// ===============================================================
void printSystemInfo() {
    Serial.println("===============================================");
    Serial.println(PROJECT_NAME);
    Serial.print("Version: ");
    Serial.println(PROJECT_VERSION);
    Serial.print("Build: ");
    Serial.print(BUILD_DATE);
    Serial.print(" ");
    Serial.println(BUILD_TIME);
    Serial.print("Author: ");
    Serial.println(PROJECT_AUTHOR);
    Serial.println("===============================================");
    Serial.print("ESP32 Chip: ");
    Serial.println(ESP.getChipModel());
    Serial.print("Chip Revision: ");
    Serial.println(ESP.getChipRevision());
    Serial.print("Flash Size: ");
    Serial.print(ESP.getFlashChipSize() / 1024 / 1024);
    Serial.println(" MB");
    Serial.print("Free Heap: ");
    Serial.print(ESP.getFreeHeap() / 1024);
    Serial.println(" KB");
    Serial.println("===============================================");
}