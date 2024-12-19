#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// **WiFi Credentials**
const char* ssid = "Owxn"; // SSID ของ Wi-Fi
const char* password = "Owxn2409"; // รหัสผ่านของ Wi-Fi

// **Telegram Bot Token และ Chat IDs**
const char* botTokens[] = { // โทเค็นของ Telegram Bots
  "7702438986:AAEeokB03nKz0Y9s7Vs4VWi-U7pzHHVO8v8", // Bot 1
  "7713083064:AAFNzaIMmlDjwM6nyl6z1eAwkKHY1Zcnu9Q", // Bot 2
  "7731694722:AAGIyRqH4XgT-Bh48aQWDWks0IN9x7mzveo", // Bot 3
  "Token bot",                                     // Bot 4
  "Token bot"                                      // Bot 5
};

const int numBots = sizeof(botTokens) / sizeof(botTokens[0]); // จำนวนบอททั้งหมด

// **Chat IDs ของแต่ละ Bot**
const char* chatIdsBot1[] = {"-4767274518"}; // Chat ID ของ Bot 1
const char* chatIdsBot2[] = {"-4583053373"}; // Chat ID ของ Bot 2
const char* chatIdsBot3[] = {"Chat id"}; // Chat ID ของ Bot 3
const char* chatIdsBot4[] = {"Chat id"}; // Chat ID ของ Bot 4
const char* chatIdsBot5[] = {"Chai id"}; // Chat ID ของ Bot 5

// เก็บ Chat IDs และจำนวน Chat IDs สำหรับแต่ละบอท
const char** chatIds[] = {chatIdsBot1, chatIdsBot2, chatIdsBot3, chatIdsBot4, chatIdsBot5};
const int chatCounts[] = {
  sizeof(chatIdsBot1) / sizeof(chatIdsBot1[0]),
  sizeof(chatIdsBot2) / sizeof(chatIdsBot2[0]),
  sizeof(chatIdsBot3) / sizeof(chatIdsBot3[0]),
  sizeof(chatIdsBot4) / sizeof(chatIdsBot4[0]),
  sizeof(chatIdsBot5) / sizeof(chatIdsBot5[0])
};

// **สร้าง Client สำหรับ WiFi และ Telegram Bot**
WiFiClientSecure client; // ใช้สำหรับการเชื่อมต่อ HTTPS
UniversalTelegramBot* bots[numBots]; // อาร์เรย์สำหรับเก็บบอท

// **พินสำหรับเซ็นเซอร์**
const int sensorPin1 = D1; // พินของเซ็นเซอร์ที่ 1
const int sensorPin2 = D2; // พินของเซ็นเซอร์ที่ 2
const int sensorPin3 = D3; // พินของเซ็นเซอร์ที่ 3
const int sensorPin4 = D4; // พินของเซ็นเซอร์ที่ 4
const int sensorPin5 = D5; // พินของเซ็นเซอร์ที่ 5

// ตัวแปรสถานะของลิ้นชักแต่ละตัว
bool drawerOccupied1 = false;
bool drawerOccupied2 = false;
bool drawerOccupied3 = false;
bool drawerOccupied4 = false;
bool drawerOccupied5 = false;

// สถานะการเชื่อมต่อ Wi-Fi
bool wifiConnected = false;

void setup() {
  Serial.begin(115200); // เริ่มต้น Serial Monitor
  pinMode(sensorPin1, INPUT); // ตั้งค่าพินเซ็นเซอร์ 1 เป็น Input
  pinMode(sensorPin2, INPUT); // ตั้งค่าพินเซ็นเซอร์ 2 เป็น Input
  pinMode(sensorPin3, INPUT); // ตั้งค่าพินเซ็นเซอร์ 3 เป็น Input
  pinMode(sensorPin4, INPUT); // ตั้งค่าพินเซ็นเซอร์ 4 เป็น Input
  pinMode(sensorPin5, INPUT); // ตั้งค่าพินเซ็นเซอร์ 5 เป็น Input

  // เชื่อมต่อ Wi-Fi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { // รอจนกว่า Wi-Fi จะเชื่อมต่อ
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  wifiConnected = true; // อัปเดตสถานะ Wi-Fi
  client.setInsecure(); // ปิดการตรวจสอบใบรับรอง SSL/TLS

  // สร้าง Bot สำหรับแต่ละ Token
  for (int i = 0; i < numBots; i++) {
    bots[i] = new UniversalTelegramBot(botTokens[i], client);
  }

  // แจ้งเตือนเมื่อเชื่อมต่อ Wi-Fi เสร็จสิ้น
  sendNotificationToAll("อุปกรณ์เชื่อมต่อ Wi-Fi เรียบร้อยแล้ว!");
}

// ฟังก์ชันสำหรับส่งข้อความถึงทุกบอท
void sendNotificationToAll(const String& message) {
  for (int botIndex = 0; botIndex < numBots; botIndex++) {
    for (int chatIndex = 0; chatIndex < chatCounts[botIndex]; chatIndex++) {
      bots[botIndex]->sendMessage(chatIds[botIndex][chatIndex], message, "Markdown");
      Serial.println("Notification sent to Bot " + String(botIndex + 1) + 
                     " (Chat ID: " + String(chatIds[botIndex][chatIndex]) + ")");
    }
  }
}

// ตรวจสอบสถานะการเชื่อมต่อ Wi-Fi และแจ้งเตือนเมื่อหลุดหรือเชื่อมต่อใหม่
void checkWiFiConnection() {
  if (WiFi.status() != WL_CONNECTED && wifiConnected) {
    wifiConnected = false;
    Serial.println("Wi-Fi disconnected!");
    sendNotificationToAll("การเชื่อมต่อ Wi-Fi หลุด! โปรดตรวจสอบเครือข่าย.");
  } else if (WiFi.status() == WL_CONNECTED && !wifiConnected) {
    wifiConnected = true;
    Serial.println("Wi-Fi reconnected!");
    sendNotificationToAll("อุปกรณ์เชื่อมต่อ Wi-Fi ได้อีกครั้ง!");
  }
}

// ฟังก์ชันสำหรับส่งข้อความถึงบอทที่ระบุ
void sendNotificationToSpecificBot(int botIndex, const String& message) {
  for (int chatIndex = 0; chatIndex < chatCounts[botIndex]; chatIndex++) {
    bots[botIndex]->sendMessage(chatIds[botIndex][chatIndex], message, "Markdown");
    Serial.println("Notification sent to Bot " + String(botIndex + 1) + 
                   " (Chat ID: " + String(chatIds[botIndex][chatIndex]) + ")");
  }
}

// ฟังก์ชันตรวจสอบสถานะเซ็นเซอร์ของแต่ละลิ้นชัก
void checkDrawerSensor(int sensorPin, bool &drawerOccupied, const String &drawerName, int botIndex) {
  int sensorValue = digitalRead(sensorPin); // อ่านค่าจากเซ็นเซอร์

  // ตรวจจับกระดาษเข้า (สถานะเปลี่ยนจาก HIGH → LOW)
  if (sensorValue == LOW && !drawerOccupied) { 
    sendNotificationToSpecificBot(botIndex, drawerName + ": มีเอกสารเข้าในลิ้นชักแล้ว!");
    drawerOccupied = true; // อัปเดตสถานะว่ามีเอกสารในลิ้นชักแล้ว
  } 
  // ตรวจจับกระดาษออก (สถานะเปลี่ยนจาก LOW → HIGH)
  else if (sensorValue == HIGH && drawerOccupied) { 
    sendNotificationToSpecificBot(botIndex, drawerName + ": ลิ้นชักว่างแล้ว!");
    drawerOccupied = false; // อัปเดตสถานะว่าไม่มีเอกสารในลิ้นชัก
  }
}

void loop() {
  // ตรวจสอบการเชื่อมต่อ Wi-Fi
  checkWiFiConnection();

  // ตรวจสอบสถานะของแต่ละเซ็นเซอร์แบบอิสระ
  checkDrawerSensor(sensorPin1, drawerOccupied1, "ลิ้นชักที่ 1", 0); // Bot 1
  checkDrawerSensor(sensorPin2, drawerOccupied2, "ลิ้นชักที่ 2", 1); // Bot 2
  checkDrawerSensor(sensorPin3, drawerOccupied3, "ลิ้นชักที่ 3", 2); // Bot 3
  checkDrawerSensor(sensorPin4, drawerOccupied4, "ลิ้นชักที่ 4", 3); // Bot 4
  checkDrawerSensor(sensorPin5, drawerOccupied5, "ลิ้นชักที่ 5", 4); // Bot 5

  delay(100); // ลดความถี่ในการอ่านค่า
}
