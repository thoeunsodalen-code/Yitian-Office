# Yitian Office — RFID Attendance & Automatic Door

An ESP8266-based system that combines RFID access control, an automatic servo door, and Google Sheets attendance logging.

## Features

- RFID card scan (MFRC522) for employee identification
- 16x2 I2C LCD for status messages
- Servo-controlled door (opens 180°, auto-closes after 4s)
- Buzzer feedback on scan events
- Attendance (IN/OUT) logged to Google Sheets via Apps Script
- Unknown-card handling: denies access, and flags "webcam needed" if the same unknown card is scanned twice within 5 seconds

## System Flow

**Known card:**
```
Scan → Welcome! → Access Granted → Buzzer → Door opens (180°)
→ waits 4s → Door closes (0°) → LCD: "Scan your card"
→ logs attendance to Google Sheets → IN/OUT/ERROR (Serial only)
```

**Unknown card:**
```
Scan → Access Denied → Scan Again
```

**Same unknown card twice (within 5s):**
```
→ WEBCAM NEEDED
```

## Hardware

| Component        | Pin   |
|-------------------|-------|
| RFID RC522 (SS)   | D8    |
| RFID RC522 (RST)  | D0    |
| Buzzer            | D3    |
| Servo             | D4    |
| LCD (I2C SDA)     | D2    |
| LCD (I2C SCL)     | D1    |

- Board: ESP8266 (NodeMCU)
- LCD address: `0x27`, 16x2

## Setup

1. Install required libraries: `MFRC522`, `LiquidCrystal_I2C`, `Servo`, `ESP8266WiFi`, `ESP8266HTTPClient`, `WiFiClientSecure`
2. Open `yitian_office.ino` in the Arduino IDE or PlatformIO
3. Set your WiFi credentials and Google Apps Script URL in the config section at the top of the file
4. Add authorized cards to the `knownUsers[]` array (UID, first name, last name, department, role)
5. Flash to the ESP8266 and open Serial Monitor at `115200` baud to confirm WiFi connection and see attendance results

## Notes

- Attendance results (`IN` / `OUT` / `ERROR`) are only visible in the Serial Monitor, not on the LCD
- Unknown-card tracking resets after a valid scan or after the "webcam needed" alert fires
