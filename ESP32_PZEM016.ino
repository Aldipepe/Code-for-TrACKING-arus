#include <HardwareSerial.h>
#include "config.h"              // ← INCLUDE CONFIG.H

// ============================================
// ESP32 + PZEM-016 AC Energy Tracking System
// RS485 Modbus RTU Communication
// ============================================

// Configuration (dari config.h)
#define RX_PIN RS485_RX_PIN           // UART2 RX
#define TX_PIN RS485_TX_PIN           // UART2 TX
#define BAUDRATE PZEM_BAUDRATE       // PZEM-016 communication speed
#define PZEM_SLAVE PZEM_SLAVE_ADDRESS // PZEM-016 Modbus slave address

// UART2 Hardware Serial for RS485 communication
HardwareSerial rs485(2);

// Data structure for PZEM-016 readings
struct PZEM_Data {
  float voltage;      // V
  float current;      // A
  float power;        // W
  float energy;       // Wh
  float frequency;    // Hz
  float powerFactor;  // cos(phi)
  uint8_t status;     // Operation status
};

PZEM_Data sensorData = {0};

// ============================================
// CRC16 Modbus Calculation
// ============================================
uint16_t calculateCRC16(uint8_t *data, uint8_t length) {
  uint16_t crc = CRC16_INIT_VALUE;
  
  for (uint8_t i = 0; i < length; i++) {
    crc ^= data[i];
    
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x0001) {
        crc = (crc >> 1) ^ CRC16_POLYNOMIAL;
      } else {
        crc = crc >> 1;
      }
    }
  }
  
  return crc;  // Return as-is (Low byte first in transmission)
}

// ============================================
// Send Modbus RTU Command to PZEM-016
// ============================================
bool sendCommand(uint8_t *cmd, uint8_t cmdLen) {
  // Flush any old data
  while (rs485.available()) {
    rs485.read();
  }
  
  delay(MODBUS_PRE_DELAY);
  
  // Send command
  rs485.write(cmd, cmdLen);
  rs485.flush();
  
  return true;
}

// ============================================
// Read Response from PZEM-016
// ============================================
bool readResponse(uint8_t *response, uint8_t expectedLen, uint16_t timeout_ms) {
  uint32_t startTime = millis();
  uint8_t bytesRead = 0;
  
  while (millis() - startTime < timeout_ms) {
    if (rs485.available()) {
      response[bytesRead] = rs485.read();
      bytesRead++;
      
      if (bytesRead >= expectedLen) {
        return true;
      }
    }
  }
  
  return (bytesRead > 0);  // Return true if at least some data received
}

// ============================================
// Validate Modbus Response CRC
// ============================================
bool validateCRC(uint8_t *response, uint8_t length) {
  if (length < 3) return false;
  
  #if VALIDATE_CRC
    uint16_t receivedCRC = (response[length - 2] << 8) | response[length - 1];
    uint16_t calculatedCRC = calculateCRC16(response, length - 2);
    
    if (receivedCRC != calculatedCRC) {
      DEBUG_PRINTLN("❌ CRC mismatch!");
      return false;
    }
  #endif
  
  return true;
}

// ============================================
// Validate Sensor Data Range
// ============================================
bool validateSensorRange() {
  bool valid = true;
  
  if (sensorData.voltage < VOLTAGE_MIN || sensorData.voltage > VOLTAGE_MAX) {
    DEBUG_PRINTLN("⚠️  Voltage out of range!");
    valid = false;
  }
  
  if (sensorData.current < CURRENT_MIN || sensorData.current > CURRENT_MAX) {
    DEBUG_PRINTLN("⚠️  Current out of range!");
    valid = false;
  }
  
  if (sensorData.frequency < FREQUENCY_MIN || sensorData.frequency > FREQUENCY_MAX) {
    DEBUG_PRINTLN("⚠️  Frequency out of range!");
    valid = false;
  }
  
  if (sensorData.powerFactor < POWER_FACTOR_MIN || sensorData.powerFactor > POWER_FACTOR_MAX) {
    DEBUG_PRINTLN("⚠️  Power factor out of range!");
    valid = false;
  }
  
  return valid;
}

// ============================================
// Check Alerts & Thresholds
// ============================================
void checkAlerts() {
  if (!ALERT_ENABLE) return;
  
  if (sensorData.current > ALERT_CURRENT_THRESHOLD) {
    Serial.print("🚨 ALERT: Current ");
    Serial.print(sensorData.current);
    Serial.print("A exceeds threshold ");
    Serial.println(ALERT_CURRENT_THRESHOLD);
  }
  
  if (sensorData.power > ALERT_POWER_THRESHOLD) {
    Serial.print("🚨 ALERT: Power ");
    Serial.print(sensorData.power);
    Serial.print("W exceeds threshold ");
    Serial.println(ALERT_POWER_THRESHOLD);
  }
  
  if (sensorData.frequency < ALERT_FREQUENCY_MIN) {
    Serial.print("🚨 ALERT: Frequency ");
    Serial.print(sensorData.frequency);
    Serial.print("Hz below minimum ");
    Serial.println(ALERT_FREQUENCY_MIN);
  }
  
  if (sensorData.frequency > ALERT_FREQUENCY_MAX) {
    Serial.print("🚨 ALERT: Frequency ");
    Serial.print(sensorData.frequency);
    Serial.print("Hz exceeds maximum ");
    Serial.println(ALERT_FREQUENCY_MAX);
  }
}

// ============================================
// Read All Parameters from PZEM-016
// Function Code 0x04: Read Input Registers
// ============================================
bool readPZEM() {
  // Modbus RTU Command: Read 10 registers starting from 0x0000
  // Format: [Slave Addr] [Function Code] [Start Addr Hi] [Start Addr Lo] [Qty Hi] [Qty Lo] [CRC Lo] [CRC Hi]
  uint8_t cmd[] = {PZEM_SLAVE, MODBUS_FUNCTION_CODE, 0x00, 0x00, 0x00, 0x0A, 0x30, 0x39};
  uint8_t response[PZEM_RESPONSE_LENGTH] = {0};
  
  // Send command
  if (!sendCommand(cmd, sizeof(cmd))) {
    Serial.println("❌ Failed to send command");
    return false;
  }
  
  // Wait for response
  delay(MODBUS_POST_DELAY);
  
  // Read response
  if (!readResponse(response, PZEM_RESPONSE_LENGTH, MODBUS_READ_TIMEOUT)) {
    Serial.println("❌ No response from PZEM-016");
    return false;
  }
  
  #if ENABLE_DEBUG_MODE
    debugDisplayResponse(response, PZEM_RESPONSE_LENGTH);
  #endif
  
  // Validate response
  if (!validateCRC(response, PZEM_RESPONSE_LENGTH)) {
    Serial.println("❌ CRC validation failed");
    return false;
  }
  
  // Parse response data
  // Response format: [Slave] [Function] [ByteCount] [Data...] [CRC_Lo] [CRC_Hi]
  
  // Voltage: Register 0x0000 (2 bytes) - V
  sensorData.voltage = ((response[3] << 8) | response[4]) / VOLTAGE_DIVISOR;
  
  // Current: Register 0x0001 (2 bytes) - A (÷1000)
  uint16_t currentRaw = (response[5] << 8) | response[6];
  sensorData.current = currentRaw / CURRENT_DIVISOR;
  
  // Power: Register 0x0003 (4 bytes) - W
  uint32_t powerRaw = ((response[7] << 24) | (response[8] << 16) | 
                       (response[9] << 8) | response[10]);
  sensorData.power = powerRaw / POWER_DIVISOR;
  
  // Energy: Register 0x0005 (4 bytes) - Wh
  uint32_t energyRaw = ((response[11] << 24) | (response[12] << 16) | 
                        (response[13] << 8) | response[14]);
  sensorData.energy = energyRaw / ENERGY_DIVISOR;
  
  // Frequency: Register 0x0007 (2 bytes) - Hz (÷10)
  uint16_t freqRaw = (response[15] << 8) | response[16];
  sensorData.frequency = freqRaw / FREQUENCY_DIVISOR;
  
  // Power Factor: Register 0x0008 (2 bytes) - (÷1000)
  uint16_t pfRaw = (response[17] << 8) | response[18];
  sensorData.powerFactor = pfRaw / POWER_FACTOR_DIVISOR;
  
  // Validate range
  if (!validateSensorRange()) {
    DEBUG_PRINTLN("⚠️  Some values out of range, but proceeding...");
  }
  
  // Check alerts
  checkAlerts();
  
  return true;
}

// ============================================
// Calculate additional electrical parameters
// ============================================
float calculateApparentPower() {
  if (sensorData.voltage > 0) {
    return sensorData.voltage * sensorData.current;
  }
  return 0;
}

float calculateReactivePower() {
  float apparentPower = calculateApparentPower();
  if (apparentPower > 0 && sensorData.powerFactor > 0) {
    float pf2 = sensorData.powerFactor * sensorData.powerFactor;
    return apparentPower * sqrt(1 - pf2);
  }
  return 0;
}

// ============================================
// Display readings with formatted output
// ============================================
void displayReadings() {
  if (!ENABLE_SERIAL_OUTPUT) return;
  
  float apparentPower = calculateApparentPower();
  float reactivePower = calculateReactivePower();
  
  // Clear screen effect (optional)
  Serial.println("\n" + String(50, '='));
  Serial.println("  🔋 PZEM-016 SENSOR READINGS");
  Serial.println(String(50, '='));
  
  // Basic Parameters
  Serial.print("⚡ Voltage (V)     : ");
  Serial.print(sensorData.voltage, 1);
  Serial.println(" V");
  
  Serial.print("📊 Current (A)     : ");
  Serial.print(sensorData.current, 3);
  Serial.println(" A");
  
  Serial.print("💡 Power (W)       : ");
  Serial.print(sensorData.power, 0);
  Serial.println(" W");
  
  #if ENABLE_CALCULATED_VALUES
    Serial.print("📈 Apparent Power  : ");
    Serial.print(apparentPower, 0);
    Serial.println(" VA");
    
    Serial.print("🔄 Reactive Power  : ");
    Serial.print(reactivePower, 0);
    Serial.println(" VAR");
  #endif
  
  // Auxiliary Parameters
  Serial.print("⏱️  Frequency (Hz)  : ");
  Serial.print(sensorData.frequency, 1);
  Serial.println(" Hz");
  
  Serial.print("📐 Power Factor    : ");
  Serial.print(sensorData.powerFactor, 3);
  Serial.println(" (cos φ)");
  
  Serial.print("📦 Energy (Wh)     : ");
  Serial.print(sensorData.energy, 0);
  Serial.println(" Wh");
  
  Serial.println(String(50, '='));
  Serial.print("🕐 Timestamp: ");
  Serial.println(millis());
  Serial.println();
}

// ============================================
// Debug: Display raw response bytes
// ============================================
void debugDisplayResponse(uint8_t *response, uint8_t length) {
  #if ENABLE_DEBUG_MODE
    Serial.print("📥 Raw Response: ");
    for (uint8_t i = 0; i < length; i++) {
      Serial.print("0x");
      if (response[i] < 0x10) Serial.print("0");
      Serial.print(response[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  #endif
}

// ============================================
// Setup
// ============================================
void setup() {
  // Serial Monitor (USB)
  Serial.begin(SERIAL_MONITOR_BAUDRATE);
  delay(1000);
  
  Serial.println("\n🚀 ESP32 + PZEM-016 Energy Tracking System");
  Serial.println("Initializing...");
  
  // RS485 UART (UART2)
  rs485.begin(BAUDRATE, SERIAL_8N1, RX_PIN, TX_PIN);
  
  Serial.println("✅ Serial initialized");
  Serial.print("✅ RS485 initialized on GPIO ");
  Serial.print(RX_PIN);
  Serial.print(" (RX), GPIO ");
  Serial.print(TX_PIN);
  Serial.println(" (TX)");
  Serial.println("✅ PZEM-016 Sensor Ready!");
  Serial.println();
  
  #if ENABLE_DEBUG_MODE
    Serial.println("🐛 DEBUG MODE ENABLED");
  #endif
  
  #if ALERT_ENABLE
    Serial.print("🚨 ALERTS ENABLED - Current threshold: ");
    Serial.print(ALERT_CURRENT_THRESHOLD);
    Serial.println("A");
  #endif
  
  delay(2000);
}

// ============================================
// Main Loop
// ============================================
void loop() {
  static unsigned long lastReadTime = 0;
  
  // Check if it's time to read sensor
  if (millis() - lastReadTime >= SENSOR_READ_INTERVAL) {
    lastReadTime = millis();
    
    Serial.println("📡 Reading PZEM-016...");
    
    // Read sensor data
    if (readPZEM()) {
      // Display successful reading
      displayReadings();
    } else {
      // Handle read error
      Serial.println("❌ Failed to read sensor data");
      Serial.println("   - Check RS485 connections");
      Serial.println("   - Verify PZEM-016 power supply");
      Serial.println("   - Check baudrate (9600)");
      Serial.println();
    }
  }
  
  delay(10);  // Small delay to prevent watchdog timeout
}
