#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30100_PulseOximeter.h"
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
#define NUMFLAKES 10
#define LOGO_HEIGHT 16
#define LOGO_WIDTH 16
#define REPORTING_PERIOD_MS 1000
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


// Insert your network credentials
#define WIFI_SSID ""
#define WIFI_PASSWORD ""

// Insert Firebase project API Key
#define API_KEY ""

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL ""

#define USER_EMAIL ""
#define USER_PASSWORD ""

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

PulseOximeter pox;

const unsigned char bitmap[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x18, 0x00, 0x0f, 0xe0, 0x7f, 0x00, 0x3f, 0xf9, 0xff, 0xc0,
  0x7f, 0xf9, 0xff, 0xc0, 0x7f, 0xff, 0xff, 0xe0, 0x7f, 0xff, 0xff, 0xe0, 0xff, 0xff, 0xff, 0xf0,
  0xff, 0xf7, 0xff, 0xf0, 0xff, 0xe7, 0xff, 0xf0, 0xff, 0xe7, 0xff, 0xf0, 0x7f, 0xdb, 0xff, 0xe0,
  0x7f, 0x9b, 0xff, 0xe0, 0x00, 0x3b, 0xc0, 0x00, 0x3f, 0xf9, 0x9f, 0xc0, 0x3f, 0xfd, 0xbf, 0xc0,
  0x1f, 0xfd, 0xbf, 0x80, 0x0f, 0xfd, 0x7f, 0x00, 0x07, 0xfe, 0x7e, 0x00, 0x03, 0xfe, 0xfc, 0x00,
  0x01, 0xff, 0xf8, 0x00, 0x00, 0xff, 0xf0, 0x00, 0x00, 0x7f, 0xe0, 0x00, 0x00, 0x3f, 0xc0, 0x00,
  0x00, 0x0f, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void onBeatDetected() {
  Serial.println("Beat!");
  display.drawBitmap(60, 20, bitmap, 28, 28, 1);
  display.display();
}
void ShowData(int check);
void sendData(float AvgBPM, int SPO);

bool signupOK = false;

void setup() {
  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  signupOK = true;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback;  //see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  display.display();
  delay(2000);

  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(1);
  display.setCursor(0, 0);

  display.println("Initializing pulse oximeter..");
  display.display();
  Serial.print("Initializing pulse oximeter..");

  if (!pox.begin()) {
    Serial.println("FAILED");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(1);
    display.setCursor(0, 0);
    display.println("FAILED");
    display.display();
    for (;;)
      ;
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(1);
    display.setCursor(0, 0);
    display.println("SUCCESS");
    display.display();
    Serial.println("SUCCESS");
  }

  pox.setOnBeatDetectedCallback(onBeatDetected);
}

float SumBPM = 0;
int SumSPO = 0;
int Cnt = 0;

void loop() {
  pox.update();

  if (millis() % 1500 == 0) {
    Cnt++;

    float BPM = pox.getHeartRate();
    int SPO = pox.getSpO2();

    SumBPM = SumBPM + BPM;
    SumSPO = SumSPO + BPM;

    float AvgBPM = SumBPM / Cnt;

    display.clearDisplay();

    display.setTextSize(1);
    display.setTextColor(1);
    display.setCursor(0, 0);
    display.println("Heart BPM");

    display.setTextSize(1);
    display.setTextColor(1);
    display.setCursor(0, 16);
    display.println(BPM);


    display.setTextSize(1);
    display.setTextColor(1);
    display.setCursor(0, 30);
    display.println("Spo2");

    display.setTextSize(1);
    display.setTextColor(1);
    display.setCursor(0, 45);
    display.println(SPO);
    display.display();

    if (Cnt % 15 == 0) {
      if (Firebase.ready() && signupOK) {
        Firebase.RTDB.setFloat(&fbdo, "Glucokit/AveregeBPM", AvgBPM);
        Firebase.RTDB.setFloat(&fbdo, "Glucokit/SPO", SPO);
      }
    }
  }
}
