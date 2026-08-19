#include <WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <Firebase_ESP_Client.h>
// =====================================================
// WIFI
// =====================================================
#define WIFI_SSID "TP link"
#define WIFI_PASSWORD "87654321"
// =====================================================
// FIREBASE
// =====================================================
#define API_KEY "AIzaSyAsXN03thBp16wPMpYLUCkA4-47fTrZ1Wg"
#define DATABASE_URL "https://smart-parking-system-ce530-default-rtdb.asia-southeast1.firebasedatabase.app/"
// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
// =====================================================
// RFID RC522
// =====================================================
#define RFID_SS 5
#define RFID_RST 4
MFRC522 rfid(RFID_SS, RFID_RST);
// =====================================================
// 16x2 I2C LCD
// =====================================================
#define LCD_SDA 21
#define LCD_SCL 22
LiquidCrystal_I2C lcd(0x27, 16, 2);
// =====================================================
// SERVO
// =====================================================
#define SERVO_PIN 13
Servo gateServo;
// =====================================================
// PARKING SETTINGS
// =====================================================
const int TOTAL_CAPACITY = 4;
int occupiedSlots = 0;
int availableSlots = TOTAL_CAPACITY;
// =====================================================
// REGISTERED VEHICLES
// =====================================================
String vehicleUID[] =
{
  "9A ED 8A 3F",
  "03 BC 1C DA",
  "01 02 03 04",
  "11 22 33 44"
};
String vehicleNumber[] =
{
  "MH01AB1234",
  "MH01CD5678",
  "MH01EF9012",
  "MH01GH3456"
};
bool vehicleInside[] =
{
  false,
  false,
  false,
  false
};
const int TOTAL_VEHICLES = 4;
// =====================================================
// GET RFID UID
// =====================================================
String getRFIDUID()
{
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++)
  {
    if (rfid.uid.uidByte[i] < 0x10)
    {
      uid += "0";
    }
    uid += String(rfid.uid.uidByte[i], HEX);

    if (i < rfid.uid.size - 1)
    {
      uid += " ";
    }
  }
  uid.toUpperCase();
  return uid;
}
// =====================================================
// DISPLAY PARKING STATUS
// =====================================================
void displayParkingStatus()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Available: ");
  lcd.print(availableSlots);
  lcd.setCursor(0, 1);
  lcd.print("Occupied: ");
  lcd.print(occupiedSlots);
}
// =====================================================
// OPEN GATE
// =====================================================
void openGate()
{
  Serial.println("Opening Gate");
  gateServo.write(90);
  delay(3000);
  Serial.println("Closing Gate");
  gateServo.write(0);
}
// =====================================================
// CONNECT TO WIFI
// =====================================================
void connectWiFi()
{
  Serial.println();
  Serial.println("Connecting to WiFi...");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi Connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected");
  delay(1500);
}
// =====================================================
// FIREBASE LOGIN
// =====================================================
bool connectFirebase()
{
  Serial.println();
  Serial.println("Connecting to Firebase...");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Firebase");
  lcd.setCursor(0, 1);
  lcd.print("Connecting...");
  // Firebase configuration
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  // Anonymous authentication
  if (Firebase.signUp(
        &config,
        &auth,
        "",
        ""))
  {
    Serial.println("Firebase Anonymous Login Successful");
  }
  else
  {
    Serial.println("Firebase Anonymous Login Failed");
    Serial.print("Error: ");
    Serial.println(
      config.signer.signupError.message.c_str()
    );
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Firebase Error");
    delay(3000);
    return false;
  }
  // Start Firebase
  Firebase.begin(
    &config,
    &auth
  );
  Firebase.reconnectWiFi(true);
  Serial.println("Firebase Initialized");
  // Wait for Firebase to become ready
  unsigned long startTime = millis();
  while (!Firebase.ready() &&
         millis() - startTime < 10000)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  if (Firebase.ready())
  {
    Serial.println("Firebase Ready");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Firebase Ready");
    delay(1500);
    return true;
  }
  else
  {
    Serial.println("Firebase NOT Ready");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Firebase Failed");
    delay(2000);
    return false;
  }
}
// =====================================================
// UPDATE PARKING DATA TO FIREBASE
// =====================================================
void updateFirebase()
{
  if (!Firebase.ready())
  {
    Serial.println("Firebase not ready");
    return;
  }
  bool result;
  // Total Capacity
  result = Firebase.RTDB.setInt(
    &fbdo,
    "/parking/totalCapacity",
    TOTAL_CAPACITY
  );
  if (!result)
  {
    Serial.print("Capacity Error: ");
    Serial.println(
      fbdo.errorReason()
    );
  }
  // Occupied
  result = Firebase.RTDB.setInt(
    &fbdo,
    "/parking/occupied",
    occupiedSlots
  );
  if (!result)
  {
    Serial.print("Occupied Error: ");
    Serial.println(
      fbdo.errorReason()
    );
  }
  // Available
  result = Firebase.RTDB.setInt(
    &fbdo,
    "/parking/available",
    availableSlots
  );
  if (!result)
  {
    Serial.print("Available Error: ");
    Serial.println(
      fbdo.errorReason()
    );
  }
  Serial.println(
    "Firebase Parking Data Updated"
  );
}
// =====================================================
// SAVE VEHICLE INFORMATION
// =====================================================
void updateVehicleData(int index)
{
  if (!Firebase.ready())
  {
    return;
  }
  String path =
    "/vehicles/" +
    String(index + 1);
  Firebase.RTDB.setString(
    &fbdo,
    path + "/rfid",
    vehicleUID[index]
  );
  Firebase.RTDB.setString(
    &fbdo,
    path + "/vehicleNumber",
    vehicleNumber[index]
  );
  Firebase.RTDB.setBool(
    &fbdo,
    path + "/inside",
    vehicleInside[index]
  );
}
// =====================================================
// SAVE ACCESS LOG
// =====================================================
void saveAccessLog(
  String uid,
  String vehicle,
  String action
)
{
  if (!Firebase.ready())
  {
    return;
  }
  FirebaseJson json;
  json.set(
    "rfid",
    uid
  );
  json.set(
    "vehicleNumber",
    vehicle
  );
  json.set(
    "action",
    action
  );
  json.set(
    "occupied",
    occupiedSlots
  );
  json.set(
    "available",
    availableSlots
  );
  json.set(
    "timestamp",
    millis()
  );
  if (Firebase.RTDB.pushJSON(
        &fbdo,
        "/accessLogs",
        &json))
  {
    Serial.println(
      "Access Log Saved"
    );
  }
  else
  {
    Serial.print(
      "Log Error: "
    );
    Serial.println(
      fbdo.errorReason()
    );
  }
}
// =====================================================
// SETUP
// =====================================================
void setup()
{
  Serial.begin(115200);
  // ===================================================
  // LCD
  // ===================================================
  Wire.begin(
    LCD_SDA,
    LCD_SCL
  );
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SMART PARKING");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  delay(2000);
  // ===================================================
  // RFID
  // ===================================================
  SPI.begin(
    18,
    19,
    23,
    RFID_SS
  );
  rfid.PCD_Init();
  Serial.println(
    "RC522 Initialized"
  );
  // ===================================================
  // SERVO
  // ===================================================
  gateServo.attach(
    SERVO_PIN
  );
  gateServo.write(0);
  // ===================================================
  // WIFI
  // ===================================================
  connectWiFi();
  // ===================================================
  // FIREBASE
  // ===================================================
  bool firebaseConnected =
    connectFirebase();
  // ===================================================
  // INITIAL DATABASE UPDATE
  // ===================================================
  if (firebaseConnected)
  {
    updateFirebase();
    // =================================================
    // INITIAL VEHICLE DATA
    // =================================================
    for (
      int i = 0;
      i < TOTAL_VEHICLES;
      i++
    )
    {
      updateVehicleData(i);
    }
  }
  // ===================================================
  // READY
  // ===================================================
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Ready");
  lcd.setCursor(0, 1);
  lcd.print("Scan RFID");
  Serial.println();
  Serial.println(
    "=========================="
  );
  Serial.println(
    " SMART PARKING SYSTEM"
  );
  Serial.println(
    "=========================="
  );
  Serial.println(
    "System Ready"
  );
}
// =====================================================
// LOOP
// =====================================================
void loop()
{
  // ===================================================
  // CHECK RFID CARD
  // ===================================================
  if (!rfid.PICC_IsNewCardPresent())
  {
    return;
  }
  if (!rfid.PICC_ReadCardSerial())
  {
    return;
  }
  // ===================================================
  // GET UID
  // ===================================================
  String uid =
    getRFIDUID();
  Serial.println();
  Serial.println(
    "=========================="
  );
  Serial.print(
    "RFID UID: "
  );
  Serial.println(uid);
  // ===================================================
  // FIND VEHICLE
  // ===================================================
  int vehicleIndex = -1;
  for (
    int i = 0;
    i < TOTAL_VEHICLES;
    i++
  )
  {
    if (uid == vehicleUID[i])
    {
      vehicleIndex = i;
      break;
    }
  }
  // ===================================================
  // INVALID RFID
  // ===================================================
  if (vehicleIndex == -1)
  {
    Serial.println(
      "ACCESS DENIED"
    );
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Access Denied");
    lcd.setCursor(0, 1);
    lcd.print("Invalid RFID");
    delay(2000);
    displayParkingStatus();
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return;
  }
  // ===================================================
  // VEHICLE ENTRY
  // ===================================================
  if (
    vehicleInside[vehicleIndex]
    == false
  )
  {
    Serial.println(
      "Vehicle Entry"
    );
    Serial.print(
      "Vehicle: "
    );
    Serial.println(
      vehicleNumber[vehicleIndex]
    );
    // Check parking availability
    if (availableSlots > 0)
    {
      // Update software counter
      vehicleInside[
        vehicleIndex
      ] = true;
      occupiedSlots++;
      availableSlots--;
      // LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Entry Granted");
      lcd.setCursor(0, 1);
      lcd.print(
        vehicleNumber[
          vehicleIndex
        ]
      );
      Serial.println(
        "ACCESS GRANTED"
      );
      // Open gate
      openGate();
      // Firebase
      updateFirebase();
      updateVehicleData(
        vehicleIndex
      );
      saveAccessLog(
        uid,
        vehicleNumber[
          vehicleIndex
        ],
        "ENTRY"
      );
      delay(1000);
      displayParkingStatus();
    }
    else
    {
      // =================================================
      // PARKING FULL
      // =================================================
      Serial.println(
        "PARKING FULL"
      );
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Parking FULL");
      lcd.setCursor(0, 1);
      lcd.print("Access Denied");
      delay(2000);
      displayParkingStatus();
    }
  }
  // ===================================================
  // VEHICLE EXIT
  // ===================================================
  else
  {
    Serial.println(
      "Vehicle Exit"
    );
    Serial.print(
      "Vehicle: "
    );
    Serial.println(
      vehicleNumber[vehicleIndex]
    );
    // Update software counter
    vehicleInside[
      vehicleIndex
    ] = false;
    occupiedSlots--;
    availableSlots++;
    // LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Exit Granted");
    lcd.setCursor(0, 1);
    lcd.print(
      vehicleNumber[
        vehicleIndex
      ]
    );
    Serial.println(
      "EXIT GRANTED"
    );
    // Open gate
    openGate();
    // Firebase
    updateFirebase();
    updateVehicleData(
      vehicleIndex
    );
    saveAccessLog(
      uid,
      vehicleNumber[
        vehicleIndex
      ],
      "EXIT"
    );
    delay(1000);
    displayParkingStatus();
  }
  // ===================================================
  // STOP RFID
  // ===================================================
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  delay(1000);
}