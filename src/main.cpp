#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "secrets.h"

// Coin icons
#include "coin_icons.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// WiFi credentials
const char* ssid = SECRET_SSID;
const char* password = SECRET_WIFI_PASSWORD;

// Coin display settings
const unsigned char* logos[] = { btc_logo, eth_logo, doge_logo, rvn_logo };
const char* symbols[] = { "BTC", "ETH", "DOGE", "RVN" };
const char* ids[] = { "bitcoin", "ethereum", "dogecoin", "ravencoin" };
float prices[4] = {0};

int currentCoin = 0;
unsigned long lastSwitchTime = 0;
const unsigned long switchInterval = 4000;  // ms

unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 60000; // 60 seconds

void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");
}

void fetchPrices() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  HTTPClient http;
  http.begin("https://api.coingecko.com/api/v3/simple/price?ids=bitcoin,ethereum,dogecoin,ravencoin&vs_currencies=usd");
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);

    prices[0] = doc["bitcoin"]["usd"];
    prices[1] = doc["ethereum"]["usd"];
    prices[2] = doc["dogecoin"]["usd"];
    prices[3] = doc["ravencoin"]["usd"];

    Serial.println("Prices updated");
  } else {
    Serial.println("Failed to fetch prices");
  }

  http.end();
}

void showCoin(int index) {
  display.clearDisplay();

  // Draw logo in yellow zone (0–15px)
  display.drawBitmap(4, 0, logos[index], 16, 16, WHITE);

  // Symbol text
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(24, 4);
  display.print(symbols[index]);

  // Price in big text
  display.setTextSize(2);
  display.setCursor(0, 30);
  // Precision varies by value
  if (prices[index] > 999) {
    display.printf("$%.1f", prices[index]);
  }
  else if (prices[index] > 9) {
      display.printf("$%.2f", prices[index]);
  }
  else {
      display.printf("$%.4f", prices[index]);
  }

  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(100);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 failed");
    while (true);
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 8);
  display.println("Connecting...");
  display.display();

  connectWiFi();
  fetchPrices();
  showCoin(currentCoin);
  lastUpdateTime = millis();
  lastSwitchTime = millis();
}

void loop() {
  unsigned long now = millis();

  // Rotate coin every few seconds
  if (now - lastSwitchTime > switchInterval) {
    currentCoin = (currentCoin + 1) % 4;
    showCoin(currentCoin);
    lastSwitchTime = now;
  }

  // Fetch prices every 60 seconds
  if (now - lastUpdateTime > updateInterval) {
    fetchPrices();
    lastUpdateTime = now;
  }
}