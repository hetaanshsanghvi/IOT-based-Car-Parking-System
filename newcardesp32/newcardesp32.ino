#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>

// ================= RFID RC522 =================
#define SS_PIN  5
#define RST_PIN 4

// ESP32 VSPI pins
#define SCK_PIN  18
#define MISO_PIN 19
#define MOSI_PIN 23

MFRC522 rfid(SS_PIN, RST_PIN);

// ================= LCD =================
#define LCD_SDA 21
#define LCD_SCL 22

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(115200);

  // Initialize I2C for LCD
  Wire.begin(LCD_SDA, LCD_SCL);

  // Initialize SPI for RC522
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  rfid.PCD_Init();

  // Initialize LCD
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Scan your card");

  Serial.println("ESP32 RFID + LCD");
  Serial.println("================");
  Serial.println("Scan your RFID card...");
}

void loop() {

  // Check for a new card
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  // Read card
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  // ================= DISPLAY UID =================

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Card UID:");

  Serial.print("Card UID: ");

  String uidStr = "";

  for (byte i = 0; i < rfid.uid.size; i++) {

    char hexChar[3];

    sprintf(hexChar, "%02X", rfid.uid.uidByte[i]);

    uidStr += hexChar;

    Serial.print(hexChar);

    if (i < rfid.uid.size - 1) {
      Serial.print(" ");
    }
  }

  Serial.println();

  // Display UID on LCD
  lcd.setCursor(0, 1);

  // 16x2 LCD can display maximum 16 characters
  if (uidStr.length() <= 16) {
    lcd.print(uidStr);
  } else {
    lcd.print(uidStr.substring(0, 16));
  }

  delay(2000);

  // ================= READY FOR NEXT CARD =================

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scan your card");

  // Stop communication with current card
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}