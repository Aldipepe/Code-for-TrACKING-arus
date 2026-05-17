#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// 🔧 CONFIGURATION FILE
// ESP32 + PZEM-016 AC Energy Tracking System
// ============================================

// ============================================
// 1. HARDWARE CONFIGURATION
// ============================================

// UART Pin Configuration
#define RS485_RX_PIN 16           // GPIO 16 - UART2 RX (from RS485 RO)
#define RS485_TX_PIN 17           // GPIO 17 - UART2 TX (to RS485 DI)
#define RS485_UART_NUM 2          // Use UART2

// Serial Monitor Configuration
#define SERIAL_MONITOR_BAUDRATE 115200

// ============================================
// 2. MODBUS RTU CONFIGURATION
// ============================================

// PZEM-016 Communication Settings
#define PZEM_BAUDRATE 9600        // PZEM-016 fixed baudrate
#define PZEM_SLAVE_ADDRESS 0x01   // PZEM-016 default slave address
#define MODBUS_FUNCTION_CODE 0x04 // Read Input Registers

// Modbus Timing
#define MODBUS_READ_TIMEOUT 500    // milliseconds - max wait time for response
#define MODBUS_PRE_DELAY 50        // milliseconds - delay before sending command
#define MODBUS_POST_DELAY 100      // milliseconds - delay between receiving response and next command

// ============================================
// 3. PZEM-016 REGISTER MAPPING (Modbus RTU)
// ============================================

// Register Addresses (Read-Only Input Registers)
#define PZEM_REG_VOLTAGE 0x0000    // Register 0 - Voltage (V) × 10
#define PZEM_REG_CURRENT 0x0001    // Register 1 - Current (A) × 1000
#define PZEM_REG_POWER 0x0003      // Register 3-4 - Power (W)
#define PZEM_REG_ENERGY 0x0005     // Register 5-6 - Energy (Wh)
#define PZEM_REG_FREQUENCY 0x0007  // Register 7 - Frequency (Hz) × 10
#define PZEM_REG_POWER_FACTOR 0x0008 // Register 8 - Power Factor × 1000

// Data Conversion Factors
#define VOLTAGE_DIVISOR 10.0f      // Voltage = raw value / 10
#define CURRENT_DIVISOR 1000.0f    // Current = raw value / 1000
#define POWER_DIVISOR 1.0f         // Power = raw value / 1 (direct)
#define ENERGY_DIVISOR 1.0f        // Energy = raw value / 1 (direct)
#define FREQUENCY_DIVISOR 10.0f    // Frequency = raw value / 10
#define POWER_FACTOR_DIVISOR 1000.0f // Power Factor = raw value / 1000

// ============================================
// 4. SENSOR RANGE LIMITS
// ============================================

#define VOLTAGE_MIN 0.0f           // V
#define VOLTAGE_MAX 300.0f         // V

#define CURRENT_MIN 0.0f           // A
#define CURRENT_MAX 100.0f         // A

#define POWER_MIN 0.0f             // W
#define POWER_MAX 25000.0f         // W

#define FREQUENCY_MIN 45.0f        // Hz
#define FREQUENCY_MAX 65.0f        // Hz

#define POWER_FACTOR_MIN 0.0f      // (0-1)
#define POWER_FACTOR_MAX 1.0f      // (0-1)

// ============================================
// 5. ALARM/ALERT THRESHOLDS (Optional)
// ============================================

#define ALERT_ENABLE true          // Enable/disable alert system

// Current threshold (ampere)
#define ALERT_CURRENT_THRESHOLD 15.0f   // Alert if current > 15A

// Power threshold (watt)
#define ALERT_POWER_THRESHOLD 3000.0f   // Alert if power > 3000W

// Frequency threshold (Hz)
#define ALERT_FREQUENCY_MIN 49.5f  // Alert if frequency < 49.5 Hz
#define ALERT_FREQUENCY_MAX 50.5f  // Alert if frequency > 50.5 Hz

// ============================================
// 6. FEATURE FLAGS
// ============================================

// Serial Monitor Output
#define ENABLE_SERIAL_OUTPUT true
#define ENABLE_DEBUG_MODE false    // Show raw response bytes
#define ENABLE_CALCULATED_VALUES true  // Show Apparent Power & Reactive Power

// Logging Features (can be implemented later)
#define ENABLE_SD_LOGGING false    // Log data to SD card
#define ENABLE_EEPROM_LOGGING false // Log to internal EEPROM
#define ENABLE_CLOUD_UPLOAD false  // Send data to cloud service

// Communication Features
#define ENABLE_MQTT false          // MQTT integration
#define ENABLE_HTTP_API false      // HTTP API endpoint
#define ENABLE_WEBSOCKET false     // WebSocket real-time streaming

// ============================================
// 7. SAMPLING & UPDATE INTERVAL
// ============================================

#define SENSOR_READ_INTERVAL 1000   // milliseconds - Read sensor every 1 second
#define DISPLAY_UPDATE_INTERVAL 2000 // milliseconds - Update display every 2 seconds
#define DATA_LOG_INTERVAL 5000      // milliseconds - Log data every 5 seconds

// ============================================
// 8. CRC16 CONFIGURATION
// ============================================

#define CRC16_POLYNOMIAL 0xA001    // Modbus CRC16 polynomial
#define CRC16_INIT_VALUE 0xFFFF    // Modbus CRC16 initial value

// ============================================
// 9. DEBUG MACRO HELPERS
// ============================================

#if ENABLE_DEBUG_MODE
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTF(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(fmt, ...)
#endif

// ============================================
// 10. ADVANCED SETTINGS
// ============================================

// CRC Validation
#define VALIDATE_CRC true          // Always validate CRC16

// Response Length Expected
#define PZEM_RESPONSE_LENGTH 25    // 3 + (2×10) + 2 bytes

// Retry Settings
#define MAX_RETRY_ATTEMPTS 3       // Retry count on communication failure
#define RETRY_DELAY 200            // milliseconds - Delay between retries

// ============================================
END CONFIG
// ============================================

#endif  // CONFIG_H
