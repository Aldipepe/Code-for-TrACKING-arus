// ============================================
// PZEM-016 MINIMAL CODE - BEGINNER FRIENDLY
// ============================================
// 
// Ini adalah versi PALING SIMPLE untuk membaca
// sensor PZEM-016 dari ESP32
// 
// Cocok untuk:
// - Testing koneksi hardware
// - Pemula yang baru belajar
// - Troubleshooting komunikasi RS485
// 
// ============================================

#include <HardwareSerial.h>

// Inisialisasi UART2 (GPIO 16 = RX, GPIO 17 = TX)
HardwareSerial rs485(2);

// ============================================
// SETUP - Berjalan 1x saat startup
// ============================================
void setup() {
  // Serial Monitor untuk debug
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n🚀 PZEM-016 Simple Test Started!");
  Serial.println("Connecting to sensor...");
  
  // Komunikasi RS485 dengan PZEM-016
  // Baudrate: 9600 (fixed)
  // Data bits: 8
  // Parity: None (N)
  // Stop bits: 1
  rs485.begin(9600, SERIAL_8N1, 16, 17);
  
  delay(2000);
  Serial.println("✅ Ready!\n");
}

// ============================================
// LOOP - Berjalan terus-menerus
// ============================================
void loop() {
  // Perintah Modbus RTU untuk baca 10 register dari PZEM-016
  // Format: [Slave Addr] [Function] [Start Addr Hi] [Start Addr Lo] [Count Hi] [Count Lo] [CRC Lo] [CRC Hi]
  byte command[] = {0x01, 0x04, 0x00, 0x00, 0x00, 0x0A, 0x30, 0x39};
  
  Serial.println("📡 Sending command...");
  
  // Kosongkan buffer terlebih dahulu
  while (rs485.available()) {
    rs485.read();
  }
  
  // Kirim command ke PZEM-016
  rs485.write(command, 8);
  rs485.flush();
  
  // Tunggu response dari sensor
  delay(100);
  
  // Baca response
  byte response[25] = {0};
  int bytesRead = 0;
  
  unsigned long startTime = millis();
  while (millis() - startTime < 500) {  // Tunggu max 500ms
    if (rs485.available()) {
      response[bytesRead] = rs485.read();
      bytesRead++;
      
      if (bytesRead >= 25) break;  // Response lengkap
    }
  }
  
  Serial.print("📥 Bytes received: ");
  Serial.println(bytesRead);
  
  // ============================================
  // CEK VALIDITAS DATA
  // ============================================
  if (bytesRead < 25) {
    Serial.println("❌ ERROR: Response tidak lengkap!");
    Serial.println("   - Cek koneksi hardware RS485");
    Serial.println("   - Cek power supply PZEM-016");
    Serial.println("   - Cek baudrate (harus 9600)");
    Serial.println();
  } else if (response[0] != 0x01 || response[1] != 0x04) {
    Serial.println("❌ ERROR: Response header tidak valid!");
    Serial.println();
  } else {
    // ============================================
    // PARSE DATA DARI RESPONSE
    // ============================================
    
    Serial.println("✅ Response valid!");
    
    // Tegangan (Register 0) - 2 bytes
    // Format: value / 10
    // Contoh: 0x00 0xEC = 236 → 23.6V
    uint16_t voltageRaw = (response[3] << 8) | response[4];
    float voltage = voltageRaw / 10.0;
    
    // Arus (Register 1) - 2 bytes
    // Format: value / 1000
    // Contoh: 0x00 0xC8 = 200 → 0.2A
    uint16_t currentRaw = (response[5] << 8) | response[6];
    float current = currentRaw / 1000.0;
    
    // Daya (Register 3-4) - 4 bytes
    // Format: direct value (W)
    uint32_t power = ((response[7] << 24) | (response[8] << 16) | 
                     (response[9] << 8) | response[10]);
    
    // Energi (Register 5-6) - 4 bytes
    // Format: direct value (Wh)
    uint32_t energy = ((response[11] << 24) | (response[12] << 16) | 
                      (response[13] << 8) | response[14]);
    
    // Frekuensi (Register 7) - 2 bytes
    // Format: value / 10
    uint16_t frequencyRaw = (response[15] << 8) | response[16];
    float frequency = frequencyRaw / 10.0;
    
    // Power Factor (Register 8) - 2 bytes
    // Format: value / 1000
    // Range: 0.000 - 1.000
    uint16_t pfRaw = (response[17] << 8) | response[18];
    float powerFactor = pfRaw / 1000.0;
    
    // ============================================
    // TAMPILKAN DATA DI SERIAL MONITOR
    // ============================================
    
    Serial.println("\n" + String(40, '-'));
    Serial.println("📊 PZEM-016 SENSOR READINGS");
    Serial.println(String(40, '-'));
    
    Serial.print("⚡ Voltage:      ");
    Serial.print(voltage, 1);
    Serial.println(" V");
    
    Serial.print("📊 Current:      ");
    Serial.print(current, 3);
    Serial.println(" A");
    
    Serial.print("💡 Power:        ");
    Serial.print(power);
    Serial.println(" W");
    
    Serial.print("📦 Energy:       ");
    Serial.print(energy);
    Serial.println(" Wh");
    
    Serial.print("⏱️  Frequency:    ");
    Serial.print(frequency, 1);
    Serial.println(" Hz");
    
    Serial.print("📐 Power Factor: ");
    Serial.print(powerFactor, 3);
    Serial.println(" (cos φ)");
    
    Serial.println(String(40, '-'));
    Serial.println();
  }
  
  // Tunggu 2 detik sebelum baca ulang
  delay(2000);
}

// ============================================
// TROUBLESHOOTING TIPS:
// ============================================
// 
// 1. Jika "Response tidak lengkap":
//    → Periksa kabel RS485 A & B terhubung ke PZEM
//    → Pastikan GND semua komponen terhubung
//    → Test dengan multimeter: RS485 A & B harus beda tegangan
// 
// 2. Jika "Response header tidak valid":
//    → Cek baudrate serial (harus 9600)
//    → Cek koneksi GPIO 16 (RX) dan GPIO 17 (TX)
//    → Reset ESP32 dan PZEM-016
// 
// 3. Jika nilai tidak masuk akal:
//    → Cek koneksi AC load ke PZEM (L, N, Earth)
//    → Pastikan PZEM punya supply AC 80-260V
//    → Cek CRC (bisa ditambahkan validasi CRC)
// 
// ============================================
