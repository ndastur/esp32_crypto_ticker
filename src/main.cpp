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
float changes[12] = {0}; // 3 changes per coin (1h, 24h, 7d)

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
    Serial.println("WiFi disconnected, attempting reconnect...");
    WiFi.disconnect();
    delay(100);
    WiFi.begin(ssid, password);

    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi reconnection failed");
      return;
    }
  }

  HTTPClient http;
  http.begin("https://api.coingecko.com/api/v3/coins/markets?vs_currency=usd&ids=bitcoin,ethereum,dogecoin,ravencoin&price_change_percentage=1h,24h,7d");

  int httpCode = http.GET();
  if (httpCode != 200) {
    Serial.printf("HTTP error: %d\n", httpCode);
    http.end();
    return;
  }

  String payload = http.getString();
  DynamicJsonDocument doc(8192);
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print("JSON parse error: ");
    Serial.println(error.c_str());
    http.end();
    return;
  }

  for (int i = 0; i < 4; i++) {
    prices[i] = doc[i]["current_price"];
    changes[i * 3 + 0] = doc[i]["price_change_percentage_1h_in_currency"];
    changes[i * 3 + 1] = doc[i]["price_change_percentage_24h_in_currency"];
    changes[i * 3 + 2] = doc[i]["price_change_percentage_7d_in_currency"];
  }

  Serial.println("Prices and % changes updated");
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
  display.setCursor(0, 22);
  // Precision varies by value
  if (prices[index] > 9999) {
    display.printf("$%.0f", prices[index]);
  }
  else if (prices[index] > 999) {
      display.printf("$%.2f", prices[index]);
  }
  else if (prices[index] > 9) {
      display.printf("$%.3f", prices[index]);
  }
  else {
      display.printf("$%.5f", prices[index]);
  }

  display.setTextSize(1);
  display.setCursor(0, 48);
  display.printf("1h:%+2.0f%% 24h:%+2.0f%%", changes[index*3], changes[index*3+1]);
  display.setCursor(0, 56);
  display.printf("7d:%+2.0f%%", changes[index*3+2]);
  
  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(100);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 display setup failed");
    while (true); // loop forever
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