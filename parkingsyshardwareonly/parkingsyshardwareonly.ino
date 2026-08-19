#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

// ================= RFID =================
#define RFID_SS   5
#define RFID_RST  4

MFRC522 rfid(RFID_SS, RFID_RST);

// ================= LCD =================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= SERVO =================
#define SERVO_PIN 13

Servo gateServo;

// ================= PARKING =================
const int TOTAL_SLOTS = 4;

int occupiedSlots = 0;
int availableSlots = TOTAL_SLOTS;

// ================= VEHICLES =================
// Replace these with your actual RFID UIDs

String vehicleUID[] = {
  "03 BC 1C DA",
  "9A ED 8A 3F",
  "AA BB CC DD",
  "11 22 33 44"
};

String vehicleNumber[] = {
  "MH01AB1234",
  "MH01CD5678",
  "MH01EF9012",
  "MH01GH3456"
};

bool vehicleInside[] = {
  false,
  false,
  false,
  false
};

const int TOTAL_VEHICLES = 4;


// =================================================
// GET RFID UID
// =================================================

String getRFIDUID()
{
  String uid = "";

  for (byte i = 0; i < rfid.uid.size; i++)
  {
    if (rfid.uid.uidByte[i] < 0x10)
      uid += "0";

    uid += String(rfid.uid.uidByte[i], HEX);

    if (i < rfid.uid.size - 1)
      uid += " ";
  }

  uid.toUpperCase();

  return uid;
}


// =================================================
// DISPLAY PARKING STATUS
// =================================================

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


// =================================================
// OPEN GATE
// =================================================

void openGate()
{
  gateServo.write(0);

  delay(3000);

  gateServo.write(90);
}


// =================================================
// SETUP
// =================================================

void setup()
{
  Serial.begin(115200);

  // ================= LCD =================

  Wire.begin(21, 22);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Smart Parking");

  lcd.setCursor(0, 1);
  lcd.print("Initializing");

  delay(2000);


  // ================= RFID =================

  SPI.begin(18, 19, 23, RFID_SS);

  rfid.PCD_Init();


  // ================= SERVO =================

  gateServo.attach(SERVO_PIN);

  gateServo.write(90);


  // ================= START =================

  displayParkingStatus();

  Serial.println("Smart Parking System");
  Serial.println("System Ready");
}


// =================================================
// MAIN LOOP
// =================================================

void loop()
{
  // Check for RFID card

  if (!rfid.PICC_IsNewCardPresent())
    return;

  if (!rfid.PICC_ReadCardSerial())
    return;


  String uid = getRFIDUID();

  Serial.println("-------------------------");

  Serial.print("RFID UID: ");
  Serial.println(uid);


  // =================================================
  // FIND REGISTERED VEHICLE
  // =================================================

  int vehicleIndex = -1;

  for (int i = 0; i < TOTAL_VEHICLES; i++)
  {
    if (uid == vehicleUID[i])
    {
      vehicleIndex = i;
      break;
    }
  }


  // =================================================
  // INVALID RFID
  // =================================================

  if (vehicleIndex == -1)
  {
    Serial.println("ACCESS DENIED");

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


  // =================================================
  // VEHICLE ENTRY
  // =================================================

  if (vehicleInside[vehicleIndex] == false)
  {
    Serial.println("ENTRY");

    Serial.print("Vehicle: ");
    Serial.println(vehicleNumber[vehicleIndex]);


    // Check available space

    if (availableSlots > 0)
    {
      vehicleInside[vehicleIndex] = true;

      occupiedSlots++;
      availableSlots--;


      Serial.println("ACCESS GRANTED");

      lcd.clear();

      lcd.setCursor(0, 0);
      lcd.print("Entry Granted");

      lcd.setCursor(0, 1);
      lcd.print("Gate Opening");


      // Open gate

      openGate();


      delay(1000);

      displayParkingStatus();
    }

    else
    {
      Serial.println("PARKING FULL");

      lcd.clear();

      lcd.setCursor(0, 0);
      lcd.print("Parking FULL");

      lcd.setCursor(0, 1);
      lcd.print("Access Denied");

      delay(2000);

      displayParkingStatus();
    }
  }


  // =================================================
  // VEHICLE EXIT
  // =================================================

  else
  {
    Serial.println("EXIT");

    Serial.print("Vehicle: ");
    Serial.println(vehicleNumber[vehicleIndex]);


    vehicleInside[vehicleIndex] = false;

    occupiedSlots--;
    availableSlots++;


    Serial.println("EXIT GRANTED");

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("Exit Granted");

    lcd.setCursor(0, 1);
    lcd.print("Gate Opening");


    // Open gate

    openGate();


    delay(1000);

    displayParkingStatus();
  }


  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  delay(1000);
}