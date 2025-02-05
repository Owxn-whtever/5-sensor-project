#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <SPI.h>

// ✅ ข้อมูล WiFi
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// ✅ ข้อมูล Telegram Bot
const char* botToken = "YOUR_BOT_TOKEN";
const char* chatId = "YOUR_CHAT_ID";

// ✅ ขา GPIO สำหรับ IR Sensors
const int irPinIn = D0;
const int irPinOut = D1;

// ✅ ตัวแปรสถานะ
bool previousStateIn = HIGH;
bool previousStateOut = HIGH;
unsigned long lastTriggerTime = 0;
const unsigned long debounceTime = 500; // ปรับค่าตามความเหมาะสม

// ✅ จำนวนเอกสาร
int documentCount = 0;

// ✅ WiFiClientSecure และ UniversalTelegramBot
WiFiClientSecure client;
UniversalTelegramBot bot(botToken, client);

void setup() {
  Serial.begin(115200);

  // ✅ เชื่อมต่อ WiFi
  WiFi.begin(ssid, password);
  Serial.print("กำลังเชื่อมต่อ WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\n✅ เชื่อมต่อ WiFi สำเร็จ!");

  // ✅ ตั้งค่า IR Sensors
  pinMode(irPinIn, INPUT_PULLUP);
  pinMode(irPinOut, INPUT_PULLUP);

  // ✅ ปิดการตรวจสอบ SSL (สำหรับ Telegram Bot)
  client.setInsecure();
}

void loop() {
  // ✅ ตรวจสอบการเชื่อมต่อ WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi หลุดการเชื่อมต่อ!");
    // ทำการเชื่อมต่อ WiFi ใหม่ หรือแจ้งเตือนผู้ใช้
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.print(".");
    }
    Serial.println("\n✅ เชื่อมต่อ WiFi สำเร็จ!");
  }

  // ✅ อ่านสถานะ IR Sensors
  bool currentStateIn = digitalRead(irPinIn);
  bool currentStateOut = digitalRead(irPinOut);

  // ✅ ตรวจจับเอกสาร "เข้า"
  if (previousStateIn == HIGH && currentStateIn == LOW && currentStateOut == HIGH) {
    if (millis() - lastTriggerTime > debounceTime) {
      documentCount++;
      sendNotification("มีเอกสารเข้า", documentCount);
      lastTriggerTime = millis();
    }
  }

  // ✅ ตรวจจับเอกสาร "ออก"
  if (previousStateOut == HIGH && currentStateOut == LOW && currentStateIn == HIGH) {
    if (millis() - lastTriggerTime > debounceTime) {
      if (documentCount > 0) {
        documentCount--;
        sendNotification("มีเอกสารออก", documentCount);
        lastTriggerTime = millis();
      }
    }
  }

  // ✅ อัปเดตสถานะก่อนหน้า
  previousStateIn = currentStateIn;
  previousStateOut = currentStateOut;

  delay(10); // ลดการหน่วงเวลา
}

// ✅ ฟังก์ชันส่งการแจ้งเตือนไปยัง Telegram Bot
void sendNotification(String message, int count) {
  String text = message + "\nจำนวนเอกสารปัจจุบัน: " + String(count);

  if (bot.sendMessage(chatId, text)) {
    Serial.println("✅ ส่งข้อความสำเร็จ!");
  } else {
    Serial.println("❌ ส่งข้อความล้มเหลว!");
  }
}
