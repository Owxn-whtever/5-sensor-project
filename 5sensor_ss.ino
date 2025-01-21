#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// **WiFi Credentials**
const char* ssid = "Owxn"; // SSID ของ Wi-Fi
const char* password = "Owxn2409"; // รหัสผ่านของ Wi-Fi

// **Telegram Bot Token และ Chat IDs**
const char* botTokens[] = {
  "7702438986:AAEeokB03nKz0Y9s7Vs4VWi-U7pzHHVO8v8", // Bot 1
  "7713083064:AAFNzaIMmlDjwM6nyl6z1eAwkKHY1Zcnu9Q", // Bot 2
  "7731694722:AAGIyRqH4XgT-Bh48aQWDWks0IN9x7mzveo", // Bot 3
  "Token bot",                                     // Bot 4
  "Token bot"                                      // Bot 5
};
const int numBots = sizeof(botTokens) / sizeof(botTokens[0]);

// **Chat IDs**
const char* chatIdsBot1[] = {"-4767274518"};
const char* chatIdsBot2[] = {"-4734652541"};
const char* chatIdsBot3[] = {"-4637803081"};
const char* chatIdsBot4[] = {""};
const char* chatIdsBot5[] = {"Chai id"};
const char** chatIds[] = {chatIdsBot1, chatIdsBot2, chatIdsBot3, chatIdsBot4, chatIdsBot5};
const int chatCounts[] = {
  sizeof(chatIdsBot1) / sizeof(chatIdsBot1[0]),
  sizeof(chatIdsBot2) / sizeof(chatIdsBot2[0]),
  sizeof(chatIdsBot3) / sizeof(chatIdsBot3[0]),
  sizeof(chatIdsBot4) / sizeof(chatIdsBot4[0]),
  sizeof(chatIdsBot5) / sizeof(chatIdsBot5[0])
};

// **WiFi & Telegram Bot Initialization**
WiFiClientSecure client; // ใช้สำหรับเชื่อมต่อ HTTPS
UniversalTelegramBot* bots[numBots]; // อาร์เรย์เก็บบอท

// **Sensor Pins**
const int sensorPins[] = {D1, D2, D3, D4, D5}; // พินของเซ็นเซอร์

// **Drawer States**
bool drawerOccupied[5] = {false}; // สถานะลิ้นชัก (ว่างหรือเต็ม)
unsigned long lastDetectTime[5] = {0}; // เวลาของการตรวจจับเอกสารล่าสุด
int stackedDocuments[5] = {0}; // จำนวนเอกสารที่ถูกซ้อนในลิ้นชัก

// **Sensor State Variables**
int previousSensorValue[5] = {HIGH, HIGH, HIGH, HIGH, HIGH}; // เก็บค่าผลการตรวจจับก่อนหน้านี้ของเซ็นเซอร์
unsigned long stateChangeTime[5] = {0}; // เวลาที่เซ็นเซอร์เปลี่ยนสถานะ
const unsigned long initialDelay = 5000; // หน่วงเวลาการตรวจจับเริ่มต้น (5 วินาที)

bool wifiConnected = false; // สถานะการเชื่อมต่อ Wi-Fi

void setup() {
  Serial.begin(115200); // เริ่มต้น Serial Monitor
  // ตั้งค่าพินของเซ็นเซอร์
  for (int i = 0; i < 5; i++) {
    pinMode(sensorPins[i], INPUT); // เซ็นเซอร์เป็น Input
  }

  // เชื่อมต่อ Wi-Fi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  wifiConnected = true; // อัปเดตสถานะ Wi-Fi
  client.setInsecure(); // ปิดการตรวจสอบใบรับรอง SSL/TLS

  // สร้าง Telegram Bots
  for (int i = 0; i < numBots; i++) {
    bots[i] = new UniversalTelegramBot(botTokens[i], client);
  }

  // แจ้งเตือนเมื่อเชื่อมต่อ Wi-Fi
  sendNotificationToAll("อุปกรณ์เชื่อมต่อ Wi-Fi เรียบร้อยแล้ว!");
}

// ฟังก์ชันส่งข้อความไปยังทุกบอท
void sendNotificationToAll(const String& message) {
  for (int botIndex = 0; botIndex < numBots; botIndex++) {
    for (int chatIndex = 0; chatIndex < chatCounts[botIndex]; chatIndex++) {
      bots[botIndex]->sendMessage(chatIds[botIndex][chatIndex], message, "Markdown");
      Serial.println("Notification sent to Bot " + String(botIndex + 1));
    }
  }
}

// ฟังก์ชันส่งข้อความไปยังบอทที่ระบุ
void sendNotificationToSpecificBot(int botIndex, const String& message) {
  for (int chatIndex = 0; chatIndex < chatCounts[botIndex]; chatIndex++) {
    bots[botIndex]->sendMessage(chatIds[botIndex][chatIndex], message, "Markdown");
    Serial.println("Notification sent to Bot " + String(botIndex + 1));
  }
}

// ฟังก์ชันตรวจสอบสถานะเซ็นเซอร์และตรวจจับการเปลี่ยนแปลง
void checkDrawerSensorWithVerification(int sensorIndex, int botIndex) {
  unsigned long currentMillis = millis(); // เวลาปัจจุบัน
  int sensorValue = digitalRead(sensorPins[sensorIndex]); // อ่านค่าจากเซ็นเซอร์

  // ตรวจสอบการเปลี่ยนแปลงสถานะของเซ็นเซอร์
  if (sensorValue != previousSensorValue[sensorIndex]) {
    stateChangeTime[sensorIndex] = currentMillis; // บันทึกเวลาที่สถานะเซ็นเซอร์เปลี่ยน
    previousSensorValue[sensorIndex] = sensorValue; // อัปเดตค่าผลการตรวจจับ
  }

  // ถ้าพบเอกสารใหม่ (ค่าของเซ็นเซอร์เป็น LOW)
  if (sensorValue == LOW) {
    if (!drawerOccupied[sensorIndex] || (currentMillis - stateChangeTime[sensorIndex] >= 500)) {
      // ส่งการแจ้งเตือนเมื่อมีเอกสารเข้า
      sendNotificationToSpecificBot(botIndex, "มีเอกสารเข้าในลิ้นชักที่ " + String(sensorIndex + 1) + 
                                      " (" + String(stackedDocuments[sensorIndex] + 1) + " ซ้อน)");
      drawerOccupied[sensorIndex] = true; // อัปเดตสถานะลิ้นชัก
      lastDetectTime[sensorIndex] = currentMillis; // บันทึกเวลาการตรวจจับล่าสุด
      stackedDocuments[sensorIndex]++; // เพิ่มจำนวนเอกสารในลิ้นชัก
    }
  } else { // ถ้าไม่มีเอกสาร (ค่าของเซ็นเซอร์เป็น HIGH)
    unsigned long dynamicDelay = initialDelay + (stackedDocuments[sensorIndex] * 500); // คำนวณเวลาหน่วงตามจำนวนเอกสาร
    if (drawerOccupied[sensorIndex] && (currentMillis - lastDetectTime[sensorIndex] >= dynamicDelay)) {
      // ส่งการแจ้งเตือนเมื่อลิ้นชักว่าง
      sendNotificationToSpecificBot(botIndex, "ลิ้นชักที่ " + String(sensorIndex + 1) + " ว่างแล้ว!");
      drawerOccupied[sensorIndex] = false; // อัปเดตสถานะลิ้นชัก
      lastDetectTime[sensorIndex] = currentMillis; // บันทึกเวลาการตรวจจับล่าสุด
      stackedDocuments[sensorIndex] = 0; // รีเซ็ตจำนวนเอกสาร
    }
  }
}

void loop() {
  // ตรวจสอบสถานะการเชื่อมต่อ Wi-Fi
  if (WiFi.status() != WL_CONNECTED && wifiConnected) {
    wifiConnected = false;
    sendNotificationToAll("การเชื่อมต่อ Wi-Fi หลุด! โปรดตรวจสอบเครือข่าย.");
  } else if (WiFi.status() == WL_CONNECTED && !wifiConnected) {
    wifiConnected = true;
    sendNotificationToAll("อุปกรณ์เชื่อมต่อ Wi-Fi ได้อีกครั้ง!");
  }

  // ตรวจสอบสถานะเซ็นเซอร์แต่ละตัว
  for (int i = 0; i < 5; i++) {
    checkDrawerSensorWithVerification(i, i); // ตรวจสอบแต่ละเซ็นเซอร์
  }

  delay(100); // หน่วงเวลาเพื่อลดความถี่ในการตรวจจับ
}
