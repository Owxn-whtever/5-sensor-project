#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <SPI.h>

// ✅ ข้อมูล WiFi
const char* ssid = "Owxn";            // ชื่อเครือข่าย WiFi
const char* password = "Owxn2409";    // รหัสผ่าน WiFi

// ✅ โทเค็นของ Telegram Bot (1 บอทต่อ 1 ชั้น)
const char* botTokens[] = {
  "7713083064:AAFNzaIMmlDjwM6nyl6z1eAwkKHY1Zcnu9Q", // Bot ชั้น 1
  "7702438986:AAEeokB03nKz0Y9s7Vs4VWi-U7pzHHVO8v8", // Bot ชั้น 2
  "8175471471:AAG3IpS62xQb_2pR-ZwfZnH_aVMy5ekjukw", // Bot ชั้น 3
  "", // Bot ชั้น 4 (ยังไม่มี)
  ""  // Bot ชั้น 5 (ยังไม่มี)
};

// ✅ Chat ID ของแต่ละบอท (1 ชั้นต่อ 1 บอท)
const char* chatIds[] = {
  "-4734652541",  // Chat ID ชั้น 1
  "-4767274518",  // Chat ID ชั้น 2
  "6928484464",   // Chat ID ชั้น 3
  "",             // Chat ID ชั้น 4 (ยังไม่มี)
  ""              // Chat ID ชั้น 5 (ยังไม่มี)
};

// ✅ จำนวนชั้นที่ใช้งาน
const int numDrawers = sizeof(botTokens) / sizeof(botTokens[0]);

// ✅ กำหนด GPIO สำหรับ IR Sensors (1 คู่ต่อ 1 ชั้น)
const int irPins[][2] = {
    {D0, D1},  // ชั้น 1
    {D2, D3},  // ชั้น 2
    {D4, D5},  // ชั้น 3
    {D6, D7},  // ชั้น 4
    {D8, D9}   // ชั้น 5 (แก้ RX/TX เป็นขาอื่น)
};

// ✅ ตัวแปรสถานะของ IR Sensors
bool previousState[numDrawers][2]; // สถานะก่อนหน้าของ IR Sensors
unsigned long lastTriggerTime[numDrawers]; // เวลาที่ตรวจจับล่าสุด
const unsigned long debounceTime = 3000; // ระยะเวลาหน่วงป้องกันการตรวจจับซ้ำ (3 วินาที)

// ✅ WiFiClientSecure และ UniversalTelegramBot
WiFiClientSecure client;
UniversalTelegramBot* bots[numDrawers]; // Array ของบอท Telegram (1 บอทต่อ 1 ชั้น)

// ✅ ฟังก์ชัน Setup
void setup() {
    Serial.begin(115200); // เริ่ม Serial Communication
    SPI.begin(); // เริ่ม SPI Communication

    Serial.print(" กำลังเชื่อมต่อ WiFi");
    WiFi.begin(ssid, password); // เชื่อมต่อ WiFi
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\n✅ เชื่อมต่อ WiFi สำเร็จ!");
    Serial.print(" IP Address: ");
    Serial.println(WiFi.localIP());

    // ⚡ ปิดการตรวจสอบ SSL (สำหรับ Telegram Bot)
    client.setInsecure();

    // ⚡ ตั้งค่า IR Sensors
    for (int i = 0; i < numDrawers; i++) {
        pinMode(irPins[i][0], INPUT); // กำหนดขา IR Sensor เป็น input
        pinMode(irPins[i][1], INPUT); // กำหนดขา IR Sensor เป็น input
        previousState[i][0] = digitalRead(irPins[i][0]); // อ่านสถานะเริ่มต้นของ IR Sensor
        previousState[i][1] = digitalRead(irPins[i][1]); // อ่านสถานะเริ่มต้นของ IR Sensor
        lastTriggerTime[i] = 0; // กำหนดเวลาตรวจจับล่าสุดเป็น 0
    }

    // ✅ สร้างบอท Telegram แต่ละตัว (1 ชั้นต่อ 1 บอท)
    for (int i = 0; i < numDrawers; i++) {
        if (strlen(botTokens[i]) > 0) { // ตรวจสอบว่ามีโทเคนสำหรับชั้นนี้หรือไม่
            bots[i] = new UniversalTelegramBot(botTokens[i], client); // สร้างบอท
        } else {
            bots[i] = nullptr;  // ถ้าไม่มีโทเคน ให้กำหนดเป็น nullptr
        }
    }
}

// ✅ ตัวแปรเก็บจำนวนเอกสารในลิ้นชักแต่ละชั้น
int documentCount[numDrawers] = {0}; 

// ✅ ฟังก์ชันส่งการแจ้งเตือนไปยัง Telegram Bot
void sendNotification(int drawer, String direction, int count) {
    if (WiFi.status() != WL_CONNECTED) { // ตรวจสอบการเชื่อมต่อ WiFi
        Serial.println("❌ WiFi ไม่เชื่อมต่อ");
        return;
    }

    if (bots[drawer] == nullptr || strlen(chatIds[drawer]) == 0) { // ตรวจสอบว่ามีบอทและ Chat ID หรือไม่
        Serial.println("⚠️ ไม่มีบอทสำหรับชั้นนี้ หรือไม่มี Chat ID");
        return;
    }

    String message = " เอกสาร" + direction + "ที่ลิ้นชัก " + String(drawer + 1) + 
                     "\n จำนวนเอกสารปัจจุบัน: " + String(count); // สร้างข้อความแจ้งเตือน
    Serial.println(" " + message); // แสดงข้อความใน Serial Monitor

    bool sent = bots[drawer]->sendMessage(chatIds[drawer], message, "Markdown"); // ส่งข้อความ
    if (sent) {
        Serial.println("✅ ส่งข้อความสำเร็จไปยัง Bot ชั้น " + String(drawer + 1));
    } else {
        Serial.println("❌ ส่งข้อความล้มเหลวที่ Bot ชั้น " + String(drawer + 1));
    }
}

// ✅ ฟังก์ชัน Loop
void loop() {
    for (int i = 0; i < numDrawers; i++) { // วนลูปสำหรับแต่ละชั้น
        bool currentState1 = digitalRead(irPins[i][0]); // อ่านสถานะปัจจุบันของ IR Sensor 1
        bool currentState2 = digitalRead(irPins[i][1]); // อ่านสถานะปัจจุบันของ IR Sensor 2

        unsigned long currentTime = millis(); // เวลาปัจจุบัน

        // ✅ ตรวจจับเอกสาร "เข้า" (IR1 -> IR2)
        if (previousState[i][0] == LOW && currentState1 == HIGH && 
            previousState[i][1] == HIGH && currentState2 == LOW) { // ตรวจจับการเปลี่ยนแปลงสถานะ
            if (currentTime - lastTriggerTime[i] > debounceTime) { // ตรวจสอบเวลาหน่วง
                documentCount[i]++;  // เพิ่มจำนวนเอกสาร
                sendNotification(i, "เข้า", documentCount[i]); // ส่งการแจ้งเตือน
                lastTriggerTime[i] = currentTime; // อัพเดทเวลาตรวจจับล่าสุด
            }
        }

        // ✅ ตรวจจับเอกสาร "ออก" (IR2 -> IR1)
        if (previousState[i][1] == LOW && currentState2 == HIGH && 
            previousState[i][0] == HIGH && currentState1 == LOW) { // ตรวจจับการเปลี่ยนแปลงสถานะ
            if (currentTime - lastTriggerTime[i] > debounceTime) { // ตรวจสอบเวลาหน่วง
                if (documentCount[i] > 0) { 
                    documentCount[i]--;  // ลดจำนวนเอกสาร
                }
                sendNotification(i, "ออก", documentCount[i]); // ส่งการแจ้งเตือน
                lastTriggerTime[i] = currentTime; // อัพเดทเวลาตรวจจับล่าสุด
            }
        }

        previousState[i][0] = currentState1; // อัพเดทสถานะก่อนหน้า
        previousState[i][1] = currentState2; // อัพเดทสถานะก่อนหน้า
    }

    delay(100); // หน่วงเวลาเล็กน้อย
}
