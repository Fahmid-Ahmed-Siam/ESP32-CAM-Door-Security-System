/*
  ESP32-CAM Door Security System
  - Zero-Delay Capture (Buffer Flush)
  - Hardware Logic Trigger (GPIO 14)
  - Low-Trigger Buzzer Support
*/

#include <Arduino.h>
#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// =====================================================
// WiFi & Telegram Credentials
// =====================================================
const char *ssid = "";
const char *password = "";
const char *BOT_TOKEN = "";
const char *CHAT_ID = "";

// Pins
#define ALARM_IN_PIN     14   // Signal from AND Gate
#define BUZZER_PIN       15   // Use this if you want software control, otherwise hardware logic handles it

// ESP32-CAM / GC2145 PIN CONFIGURATION
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

bool alarmLatched = false;   
uint8_t* telegramPhotoBuf = nullptr;
size_t telegramPhotoLen = 0;
size_t telegramPhotoIndex = 0;

// Telegram Binary Helpers
bool isMoreDataAvailable() { return telegramPhotoBuf && (telegramPhotoIndex < telegramPhotoLen); }
uint8_t getNextByte() { return telegramPhotoBuf[telegramPhotoIndex++]; }
uint8_t* getNextBuffer() { return telegramPhotoBuf + telegramPhotoIndex; }
int getNextBufferLen() { 
    int remaining = telegramPhotoLen - telegramPhotoIndex; 
    telegramPhotoIndex = telegramPhotoLen; 
    return remaining; 
}

void captureAndSendPhoto() {
  Serial.println("Flushing old buffer frames...");
  
  // FIX: Clear the 2-3 old frames stored in camera memory
  for (int i = 0; i < 3; i++) {
    camera_fb_t * fb_old = esp_camera_fb_get();
    if (fb_old) {
        esp_camera_fb_return(fb_old);
    }
    delay(50);
  }

  // Now capture the LIVE frame
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  Serial.println("Live image captured. Converting to JPEG...");
  bool converted = frame2jpg(fb, 12, &telegramPhotoBuf, &telegramPhotoLen);
  esp_camera_fb_return(fb);

  if(!converted) {
    Serial.println("JPEG conversion failed");
    return;
  }

  telegramPhotoIndex = 0;
  Serial.println("Sending to Telegram...");
  bot.sendPhotoByBinary(CHAT_ID, "image/jpeg", telegramPhotoLen,
                         isMoreDataAvailable, getNextByte,
                         getNextBuffer, getNextBufferLen);

  free(telegramPhotoBuf);
  telegramPhotoBuf = nullptr;
  bot.sendMessage(CHAT_ID, "🚨 SECURITY ALERT: Door Opened!", "");
  Serial.println("Notification Sent.");
}

void setup() {
    Serial.begin(115200);
    
    // Setup Inputs
    pinMode(ALARM_IN_PIN, INPUT);

    // Camera Init
    camera_config_t config = {};
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM; 
    config.pin_sccb_scl = SIOC_GPIO_NUM; 
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_RGB565;
    config.frame_size = FRAMESIZE_QVGA; 
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_count = 1;

    if (esp_camera_init(&config) != ESP_OK) {
        Serial.println("Camera Init Failed");
        while (true) delay(1000);
    }

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    secured_client.setInsecure();
    Serial.println("\nSystem Online.");
}

void loop() {
  // Read signal from Hardware Logic (AND gate output)
  bool alarmActive = digitalRead(ALARM_IN_PIN);

  if (alarmActive && !alarmLatched) {
    alarmLatched = true;
    Serial.println("ALARM! Taking photo...");
    captureAndSendPhoto();
  } 
  else if (!alarmActive && alarmLatched) {
    alarmLatched = false;
    Serial.println("Alarm Reset.");
  }
  
  delay(100); 
}