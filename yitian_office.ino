// ============================================================
// YITIAN OFFICE
// RFID ATTENDANCE + AUTOMATIC DOOR + GOOGLE SHEETS
// ============================================================
// See README.md for system overview, flow, and hardware pinout.
// ============================================================


#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>


// ============================================================
// 1. WIFI SETTINGS
// ============================================================

const char* WIFI_SSID = "iPhone";

const char* WIFI_PASSWORD = "12121212";


// ============================================================
// 2. GOOGLE APPS SCRIPT URL
// ============================================================

const char* GOOGLE_SCRIPT_URL =
  "https://script.google.com/macros/s/AKfycbx1RQ41MZEr6NIc_kiZ-GpNrN8GHtdKfZfPCYhHSTSmT5qDqjhVdVgiQO4J7vtTzLpk/exec";


// ============================================================
// 3. HARDWARE PINS
// ============================================================

// RC522
#define RFID_SS_PIN D8
#define RFID_RST_PIN D0

// Buzzer
#define BUZZER_PIN D3

// Servo
#define SERVO_PIN D4


// ============================================================
// 4. OBJECTS
// ============================================================

MFRC522 rfid(
  RFID_SS_PIN,
  RFID_RST_PIN
);

LiquidCrystal_I2C lcd(
  0x27,
  16,
  2
);

Servo doorServo;


// ============================================================
// 5. EMPLOYEE DATABASE
// ============================================================

struct User {

  String uid;
  String firstName;
  String lastName;
  String DeptName;
  String RoleName;

};


User knownUsers[] = {

  {
    "9A5AF0E7",
    "Thoeun",
    "Sodalen",
    "Cyber&System",
    "Engineer"
  },

  {
    "39F49729",
    "Em",
    "Putheary",
    "Telecommunications",
    "Engineer"
  },

  {
    "64B4EFE7",
    "Tang",
    "Tola",
    "Network",
    "Engineer"
  },

  {
    "6006F95C",
    "Ly",
    "Chhungan",
    "Electronic",
    "Engineer"
  }

};


const int numKnownUsers =
  sizeof(knownUsers) /
  sizeof(knownUsers[0]);


// ============================================================
// 6. SERVO SETTINGS
// ============================================================

const int DOOR_CLOSED_ANGLE = 0;

const int DOOR_OPEN_ANGLE = 180;

// Door stays open for 4 seconds
const unsigned long DOOR_OPEN_TIME = 4000;


// ============================================================
// 7. RFID SCAN COOLDOWN
// ============================================================

const unsigned long SCAN_COOLDOWN = 3000;

unsigned long lastScanTime = 0;


// ============================================================
// 8. UNKNOWN CARD SETTINGS
// ============================================================

String lastUnknownUID = "";

int unknownScanCount = 0;

unsigned long lastUnknownScanTime = 0;

const unsigned long UNKNOWN_SCAN_WINDOW = 5000;


// ============================================================
// 9. FUNCTION DECLARATIONS
// ============================================================

void showIdleScreen();

String readRFID();

int findUser(String uid);

void handleValidUser(User user);

void handleUnknownCard(String uid);

void openDoor();

String logAttendance(User user, String uid);

String urlEncode(String str);


// ============================================================
// 10. SETUP
// ============================================================

void setup() {

  // ----------------------------------------------------------
  // SERIAL
  // ----------------------------------------------------------

  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("================================");
  Serial.println("YITIAN SMART OFFICE STARTING");
  Serial.println("================================");


  // ----------------------------------------------------------
  // SPI
  // ----------------------------------------------------------

  SPI.begin();


  // ----------------------------------------------------------
  // RC522
  // ----------------------------------------------------------

  rfid.PCD_Init();

  delay(100);

  Serial.println("RC522 initialized.");


  // ----------------------------------------------------------
  // LCD
  // ----------------------------------------------------------

  Wire.begin(
    D2,
    D1
  );

  lcd.init();

  lcd.backlight();


  // ----------------------------------------------------------
  // BUZZER
  // ----------------------------------------------------------

  pinMode(
    BUZZER_PIN,
    OUTPUT
  );

  digitalWrite(
    BUZZER_PIN,
    LOW
  );


  // ----------------------------------------------------------
  // SERVO
  // ----------------------------------------------------------

  doorServo.attach(
    SERVO_PIN
  );

  delay(300);

  doorServo.write(
    DOOR_CLOSED_ANGLE
  );

  delay(500);


  // ----------------------------------------------------------
  // STARTUP LCD
  // ----------------------------------------------------------

  lcd.clear();

  lcd.setCursor(
    0,
    0
  );

  lcd.print(
    "Yitian Office"
  );

  lcd.setCursor(
    0,
    1
  );

  lcd.print(
    "Starting..."
  );

  delay(1500);


  // ----------------------------------------------------------
  // WIFI
  // ----------------------------------------------------------

  lcd.clear();

  lcd.setCursor(
    0,
    0
  );

  lcd.print(
    "Connecting WiFi"
  );

  Serial.println(
    "Connecting to WiFi..."
  );

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );


  while (
    WiFi.status() != WL_CONNECTED
  ) {

    delay(500);

    Serial.print(".");

  }


  Serial.println();

  Serial.println(
    "WiFi connected!"
  );

  Serial.print(
    "IP Address: "
  );

  Serial.println(
    WiFi.localIP()
  );


  // ----------------------------------------------------------
  // READY
  // ----------------------------------------------------------

  showIdleScreen();

}


// ============================================================
// 11. MAIN LOOP
// ============================================================

void loop() {

  // ----------------------------------------------------------
  // CHECK RFID
  // ----------------------------------------------------------

  if (
    !rfid.PICC_IsNewCardPresent()
  ) {

    return;

  }


  if (
    !rfid.PICC_ReadCardSerial()
  ) {

    return;

  }


  // ----------------------------------------------------------
  // READ UID
  // ----------------------------------------------------------

  String uid =
    readRFID();


  Serial.println();
  Serial.println(
    "================================"
  );

  Serial.print(
    "RFID UID: "
  );

  Serial.println(
    uid
  );


  // ----------------------------------------------------------
  // COOLDOWN
  // ----------------------------------------------------------

  if (
    millis() - lastScanTime <
    SCAN_COOLDOWN
  ) {

    Serial.println(
      "Scan ignored: cooldown."
    );

    rfid.PICC_HaltA();

    rfid.PCD_StopCrypto1();

    return;

  }


  lastScanTime =
    millis();


  // ----------------------------------------------------------
  // FIND USER
  // ----------------------------------------------------------

  int userIndex =
    findUser(
      uid
    );


  // ----------------------------------------------------------
  // VALID CARD
  // ----------------------------------------------------------

  if (
    userIndex != -1
  ) {

    Serial.println(
      "Employee recognized!"
    );

    User user =
      knownUsers[userIndex];

    handleValidUser(
      user
    );

  }


  // ----------------------------------------------------------
  // UNKNOWN CARD
  // ----------------------------------------------------------

  else {

    Serial.println(
      "Unknown RFID card!"
    );

    handleUnknownCard(
      uid
    );

  }


  // ----------------------------------------------------------
  // STOP RFID
  // ----------------------------------------------------------

  rfid.PICC_HaltA();

  rfid.PCD_StopCrypto1();

}


// ============================================================
// 12. READ RFID UID
// ============================================================

String readRFID() {

  String uid = "";

  for (
    byte i = 0;
    i < rfid.uid.size;
    i++
  ) {

    if (
      rfid.uid.uidByte[i] < 0x10
    ) {

      uid += "0";

    }

    uid += String(
      rfid.uid.uidByte[i],
      HEX
    );

  }

  uid.toUpperCase();

  return uid;

}


// ============================================================
// 13. FIND USER
// ============================================================

int findUser(
  String uid
) {

  for (
    int i = 0;
    i < numKnownUsers;
    i++
  ) {

    if (
      knownUsers[i].uid == uid
    ) {

      return i;

    }

  }

  return -1;

}


// ============================================================
// 14. HANDLE VALID USER
// ============================================================

void handleValidUser(
  User user
) {

  // ----------------------------------------------------------
  // RESET UNKNOWN CARD TRACKING
  // ----------------------------------------------------------

  unknownScanCount = 0;

  lastUnknownUID = "";


  // ----------------------------------------------------------
  // WELCOME
  // ----------------------------------------------------------

  lcd.clear();

  lcd.setCursor(
    0,
    0
  );

  lcd.print(
    "Welcome!"
  );

  lcd.setCursor(
    0,
    1
  );

  lcd.print(
    user.firstName
  );


  Serial.print(
    "Employee: "
  );

  Serial.print(
    user.firstName
  );

  Serial.print(
    " "
  );

  Serial.println(
    user.lastName
  );


  delay(1000);


  // ==========================================================
  // ACCESS GRANTED
  // ==========================================================

  Serial.println(
    "Access granted."
  );


  lcd.clear();

  lcd.setCursor(
    0,
    0
  );

  lcd.print(
    "Access Granted"
  );

  lcd.setCursor(
    0,
    1
  );

  lcd.print(
    "Door Opening..."
  );


  // ----------------------------------------------------------
  // BUZZER
  // ----------------------------------------------------------

  digitalWrite(
    BUZZER_PIN,
    HIGH
  );

  delay(150);

  digitalWrite(
    BUZZER_PIN,
    LOW
  );


  // ==========================================================
  // OPEN DOOR
  // ==========================================================

  openDoor();


  // ==========================================================
  // DOOR CLOSED
  // ==========================================================

  Serial.println(
    "Door closed."
  );


  // ----------------------------------------------------------
  // IMPORTANT:
  // LCD RETURNS TO SCAN SCREEN IMMEDIATELY
  // ----------------------------------------------------------

  showIdleScreen();


  // ==========================================================
  // GOOGLE SHEETS
  // ==========================================================
  //
  // LCD does NOT wait for or display Google result.
  // Result appears ONLY in Serial Monitor.
  //
  // ==========================================================

  Serial.println(
    "Now recording attendance..."
  );


  String result =
    logAttendance(
      user,
      user.uid
    );


  // ----------------------------------------------------------
  // GOOGLE RESULT - SERIAL ONLY
  // ----------------------------------------------------------

  if (
    result == "IN"
  ) {

    Serial.println(
      "Attendance result: CHECK IN"
    );

  }

  else if (
    result == "OUT"
  ) {

    Serial.println(
      "Attendance result: CHECK OUT"
    );

  }

  else {

    Serial.println(
      "Attendance result: ERROR"
    );

  }

}


// ============================================================
// 15. HANDLE UNKNOWN CARD
// ============================================================

void handleUnknownCard(
  String uid
) {

  // ----------------------------------------------------------
  // SAME CARD?
  // ----------------------------------------------------------

  if (
    uid == lastUnknownUID &&
    millis() - lastUnknownScanTime
      <= UNKNOWN_SCAN_WINDOW
  ) {

    unknownScanCount++;

  }

  else {

    unknownScanCount = 1;

  }


  lastUnknownUID =
    uid;

  lastUnknownScanTime =
    millis();


  Serial.print(
    "Unknown scan count: "
  );

  Serial.println(
    unknownScanCount
  );


  // ==========================================================
  // FIRST UNKNOWN SCAN
  // ==========================================================

  if (
    unknownScanCount == 1
  ) {

    lcd.clear();

    lcd.setCursor(
      0,
      0
    );

    lcd.print(
      "Access Denied"
    );

    lcd.setCursor(
      0,
      1
    );

    lcd.print(
      "Scan Again"
    );


    // Buzzer

    digitalWrite(
      BUZZER_PIN,
      HIGH
    );

    delay(150);

    digitalWrite(
      BUZZER_PIN,
      LOW
    );


    delay(1500);

    showIdleScreen();

  }


  // ==========================================================
  // SECOND SAME UNKNOWN SCAN
  // ==========================================================

  else if (
    unknownScanCount >= 2
  ) {

    Serial.println(
      "================================"
    );

    Serial.println(
      "UNKNOWN CARD SCANNED TWICE!"
    );

    Serial.println(
      "WEBCAM NEEDED"
    );

    Serial.println(
      "================================"
    );


    lcd.clear();

    lcd.setCursor(
      0,
      0
    );

    lcd.print(
      "WEBCAM NEEDED"
    );

    lcd.setCursor(
      0,
      1
    );

    lcd.print(
      "Please check!"
    );


    // First beep

    digitalWrite(
      BUZZER_PIN,
      HIGH
    );

    delay(200);

    digitalWrite(
      BUZZER_PIN,
      LOW
    );

    delay(250);


    // Second beep

    digitalWrite(
      BUZZER_PIN,
      HIGH
    );

    delay(200);

    digitalWrite(
      BUZZER_PIN,
      LOW
    );


    delay(5000);


    // Reset

    unknownScanCount = 0;

    lastUnknownUID = "";


    showIdleScreen();

  }

}


// ============================================================
// 16. OPEN DOOR
// ============================================================

void openDoor() {

  Serial.println(
    "Opening door..."
  );


  // ----------------------------------------------------------
  // OPEN TO 180°
  // ----------------------------------------------------------

  doorServo.write(
    DOOR_OPEN_ANGLE
  );


  Serial.println(
    "Door is OPEN."
  );


  // ----------------------------------------------------------
  // KEEP OPEN 4 SECONDS
  // ----------------------------------------------------------

  delay(
    DOOR_OPEN_TIME
  );


  // ----------------------------------------------------------
  // CLOSE TO 0°
  // ----------------------------------------------------------

  Serial.println(
    "Closing door..."
  );


  doorServo.write(
    DOOR_CLOSED_ANGLE
  );


  delay(500);


  Serial.println(
    "Door closed."
  );

}


// ============================================================
// 17. GOOGLE SHEETS
// ============================================================

String logAttendance(
  User user,
  String uid
) {

  // ----------------------------------------------------------
  // WIFI CHECK
  // ----------------------------------------------------------

  if (
    WiFi.status() != WL_CONNECTED
  ) {

    Serial.println(
      "WiFi disconnected!"
    );

    return "ERROR";

  }


  // ----------------------------------------------------------
  // HTTPS
  // ----------------------------------------------------------

  WiFiClientSecure client;

  client.setInsecure();

  HTTPClient http;


  // ----------------------------------------------------------
  // URL
  // ----------------------------------------------------------

  String url =
    String(
      GOOGLE_SCRIPT_URL
    );


  // firstName

  url +=
    "?firstName=";

  url +=
    urlEncode(
      user.firstName
    );


  // lastName

  url +=
    "&lastName=";

  url +=
    urlEncode(
      user.lastName
    );


  // DeptName

  url +=
    "&DeptName=";

  url +=
    urlEncode(
      user.DeptName
    );


  // RoleName

  url +=
    "&RoleName=";

  url +=
    urlEncode(
      user.RoleName
    );


  // UID

  url +=
    "&uid=";

  url +=
    urlEncode(
      uid
    );


  // ----------------------------------------------------------
  // SERIAL
  // ----------------------------------------------------------

  Serial.println();

  Serial.println(
    "Sending attendance to Google..."
  );

  Serial.println(
    url
  );


  // ----------------------------------------------------------
  // HTTP BEGIN
  // ----------------------------------------------------------

  if (
    !http.begin(
      client,
      url
    )
  ) {

    Serial.println(
      "HTTP begin failed!"
    );

    return "ERROR";

  }


  // ----------------------------------------------------------
  // GET
  // ----------------------------------------------------------

  int httpCode =
    http.GET();


  // ----------------------------------------------------------
  // RESPONSE
  // ----------------------------------------------------------

  if (
    httpCode > 0
  ) {

    Serial.print(
      "HTTP Response Code: "
    );

    Serial.println(
      httpCode
    );


    String response =
      http.getString();

    response.trim();


    Serial.print(
      "Google Response: "
    );

    Serial.println(
      response
    );


    http.end();


    // IN

    if (
      response == "IN"
    ) {

      return "IN";

    }


    // OUT

    if (
      response == "OUT"
    ) {

      return "OUT";

    }


    return "ERROR";

  }


  // ----------------------------------------------------------
  // HTTP ERROR
  // ----------------------------------------------------------

  else {

    Serial.print(
      "HTTP Error: "
    );

    Serial.println(
      http.errorToString(
        httpCode
      )
    );


    http.end();

    return "ERROR";

  }

}


// ============================================================
// 18. URL ENCODING
// ============================================================

String urlEncode(
  String str
) {

  String encoded = "";

  char c;

  char code0;

  char code1;


  for (
    int i = 0;
    i < str.length();
    i++
  ) {

    c =
      str.charAt(i);


    if (
      isalnum(c)
    ) {

      encoded += c;

    }

    else {

      code1 =
        (c & 0x0F)
        + '0';


      if (
        (c & 0x0F) > 9
      ) {

        code1 =
          (c & 0x0F)
          - 10
          + 'A';

      }


      c =
        (c >> 4)
        & 0x0F;


      code0 =
        c + '0';


      if (
        c > 9
      ) {

        code0 =
          c - 10
          + 'A';

      }


      encoded += '%';

      encoded += code0;

      encoded += code1;

    }

  }


  return encoded;

}


// ============================================================
// 19. IDLE SCREEN
// ============================================================

void showIdleScreen() {

  lcd.clear();

  lcd.setCursor(
    0,
    0
  );

  lcd.print(
    "Yitian Office"
  );

  lcd.setCursor(
    0,
    1
  );

  lcd.print(
    "Scan your card"
  );

}
