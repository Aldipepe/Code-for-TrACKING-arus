# 📊 ESP32 + PZEM-016 AC Energy Tracking System

Sistem tracking arus listrik menggunakan **ESP32 microcontroller** dengan **PZEM-016 AC sensor** melalui komunikasi **RS485 to TTL converter**.

## 🎯 Fitur Utama

- ✅ Membaca Tegangan AC (0-300V)
- ✅ Membaca Arus AC (0-100A)
- ✅ Membaca Daya Aktif (0-25000W)
- ✅ Membaca Frekuensi (45-65Hz)
- ✅ Membaca Power Factor / Cosphi (0-1)
- ✅ Menampilkan data real-time di Serial Monitor
- ✅ Protekol Modbus RTU RS485

---

## 🔧 Hardware yang Dibutuhkan

| Komponen | Spesifikasi | Jumlah |
|----------|------------|--------|
| ESP32 Development Board | Any variant | 1 |
| PZEM-016 AC Sensor | RS485 Interface | 1 |
| RS485 to TTL Module | MAX485 or similar | 1 |
| Kabel Jumper | Male-Female/Male-Male | ~8 |
| Power Supply | 5V USB atau eksternal | 1 |

---

## 📌 Diagram Koneksi

### **PZEM-016 Pinout:**
```
Pin 1 (A)  ──> RS485 Module A
Pin 2 (B)  ──> RS485 Module B
Pin 3 (GND) ──> GND Common
```

### **RS485 to TTL Module Pinout:**
```
VCC (5V)   ──> 5V ESP32 atau USB Power
GND        ──> GND (Common)
A          ──> PZEM-016 A
B          ──> PZEM-016 B
RO (RX Out) ──> GPIO 16 (RX2 ESP32)
DI (TX In)  ──> GPIO 17 (TX2 ESP32)
```

### **Koneksi Lengkap:**

```
┌─────────────────────────────────────────────────────┐
│                     ESP32                           │
│  [GPIO 16/RX2]  ◄───────────┐                       │
│  [GPIO 17/TX2]  ───────────┐ │                       │
│  [GND]          ────────┐  │ │                       │
│  [5V]           ──────┐ │  │ │                       │
└────────────────────┼──┼──┼──┘                       │
                     │  │  │                          │
        ┌────────────┘  │  └──────────────┐           │
        │               │                 │           │
        │       ┌───────┴─────────────────┴──┐        │
        │       │    RS485 to TTL Module     │        │
        │       │  (MAX485 atau sejenis)     │        │
        │       │                            │        │
        │       │ VCC ──> 5V Power          │        │
        │       │ GND ──> GND Common        │        │
        │       │ RO  ──> GPIO 16 (RX)      │        │
        │       │ DI  ──> GPIO 17 (TX)      │        │
        │       │ A ──────────┐             │        │
        │       │ B ──────────┼─────────┐   │        │
        │       └────────────────────────┼──┘        │
        │                               │            │
        │              ┌────────────────┘            │
        │              │                             │
        └──────────────┼─────────────────────────────┘
                       │
            ┌──────────┴──────────┐
            │   PZEM-016 Sensor   │
            │                     │
            │ A ────────────────┐ │
            │ B ────────────────┼─┤
            │ GND ──────────────┘ │
            │                     │
            │ L ──> AC Live       │
            │ N ──> AC Neutral    │
            │ Earth ──> Ground    │
            └─────────────────────┘
```

---

## 📚 Parameter PZEM-016 (Modbus RTU)

| Parameter | Register | Tipe Data | Format | Range |
|-----------|----------|-----------|--------|-------|
| Tegangan | 0x0000 | Uint16 | V | 0-300V |
| Arus | 0x0001 | Uint16 | A (÷1000) | 0-100A |
| Daya | 0x0003 | Uint32 | W | 0-25000W |
| Energi | 0x0005 | Uint32 | Wh | Accumulative |
| Frekuensi | 0x0007 | Uint16 | Hz (÷10) | 45-65Hz |
| Power Factor | 0x0008 | Uint16 | (÷1000) | 0-1 |

**Slave Address**: `0x01` (default)

---

## 🛠️ Software Setup

### **1. Install Arduino IDE**
- Download dari: https://www.arduino.cc/en/software

### **2. Tambahkan Board ESP32**
Buka **File → Preferences**, tambahkan URL pada "Additional Boards Manager URLs":
```
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```

### **3. Install Library ModbusMaster**
**Sketch → Include Library → Manage Libraries**

Cari dan install:
- `ModbusMaster` by Doc Walker
- `ArduinoJson` (opsional, untuk parsing)

### **4. Pilih Board**
**Tools → Board → ESP32 Arduino → ESP32 Dev Module**

### **5. Konfigurasi Port & Baudrate**
- **Tools → Port**: Pilih COM port ESP32
- **Tools → Upload Speed**: 115200

---

## 📝 Contoh Kode

Lihat file `ESP32_PZEM016.ino` untuk kode lengkap dengan penjelasan.

### **Quickstart (Minimal Code):**

```cpp
#include <HardwareSerial.h>

// UART2: GPIO 16 (RX), GPIO 17 (TX)
HardwareSerial rs485(2);

void setup() {
  Serial.begin(115200);
  rs485.begin(9600, SERIAL_8N1, 16, 17);
  
  Serial.println("PZEM-016 Sensor Ready!");
}

void loop() {
  readPZEM();
  delay(1000);
}

void readPZEM() {
  byte cmd[] = {0x01, 0x04, 0x00, 0x00, 0x00, 0x0A, 0x30, 0x39};
  rs485.write(cmd, 8);
  
  delay(100);
  
  if (rs485.available()) {
    byte response[25] = {0};
    rs485.readBytes(response, 25);
    
    // Parse data
    float voltage = (response[3] << 8 | response[4]) / 10.0;
    float current = (response[5] << 8 | response[6]) / 1000.0;
    
    Serial.print("Voltage: "); Serial.print(voltage); Serial.println(" V");
    Serial.print("Current: "); Serial.print(current); Serial.println(" A");
  }
}
```

---

## 🔍 Troubleshooting

### **1. Serial Monitor tidak menampilkan data:**
- Cek baudrate Serial Monitor = 115200
- Cek koneksi USB ESP32
- Cek pin RX/TX (GPIO 16, 17)

### **2. Data PZEM tidak terbaca (response kosong):**
- Verifikasi koneksi RS485 A dan B ke PZEM-016
- Pastikan GND terhubung ke semua komponen
- Cek baudrate RS485 = 9600
- Gunakan multimeter untuk mengukur tegangan komunikasi

### **3. Checksum Error pada PZEM:**
- Lihat perhitungan CRC16 di kode
- Verifikasi format Modbus RTU

---

## 📖 Referensi

- **PZEM-016 Datasheet**: Register addresses & Modbus commands
- **Modbus RTU Protocol**: https://en.wikipedia.org/wiki/Modbus
- **ESP32 Documentation**: https://docs.espressif.com/projects/esp-idf/
- **Arduino Reference**: https://www.arduino.cc/reference/

---

## 📄 File Project

```
Code-for-TrACKING-arus/
│
├── README.md                      # File ini
├── ESP32_PZEM016.ino             # Kode utama lengkap
├── config.h                       # Konfigurasi parameter
└── examples/
    └── SimpleMinimalCode.ino      # Contoh minimal
```

---

## 🎓 Pembelajaran Lanjutan

1. **Simpan data ke SD Card** - Logging historis
2. **Kirim data ke cloud** - ThingSpeak, Firebase
3. **Buat dashboard web** - WebSocket real-time
4. **Alert otomatis** - Email/Telegram notifikasi

---

**Dibuat dengan ❤️ untuk monitoring energi listrik Indonesia**

