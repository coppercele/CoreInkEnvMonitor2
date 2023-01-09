
#include <Arduino.h>
#include <M5CoreInk.h>
#define LGFX_M5STACK_COREINK // M5Stack CoreInk
#include <LovyanGFX.hpp>
#include <SensirionI2CScd4x.h>
#include <Wire.h>
SensirionI2CScd4x scd4x;

void printUint16Hex(uint16_t value) {
  Serial.print(value < 4096 ? "0" : "");
  Serial.print(value < 256 ? "0" : "");
  Serial.print(value < 16 ? "0" : "");
  Serial.print(value, HEX);
}

void printSerialNumber(uint16_t serial0, uint16_t serial1, uint16_t serial2) {
  Serial.print("Serial: 0x");
  printUint16Hex(serial0);
  printUint16Hex(serial1);
  printUint16Hex(serial2);
  Serial.println();
}
void pushSprite(Ink_Sprite *coreinkSprite, LGFX_Sprite *lgfxSprite) {
  coreinkSprite->clear();
  for (int y = 0; y < 200; y++) {
    for (int x = 0; x < 200; x++) {
      uint16_t c = lgfxSprite->readPixel(x, y);
      if (c == 0x0000) {
        coreinkSprite->drawPix(x, y, 0);
      }
    }
  }
  coreinkSprite->pushSprite();
}
Ink_Sprite InkPageSprite(&M5.M5Ink);

static LGFX_Sprite sprite;

static LGFX lcd;

void setup() {
  M5.begin(true, true, false);
  lcd.init();
  if (InkPageSprite.creatSprite(0, 0, 200, 200, true) != 0) {
    Serial.printf("Ink Sprite create faild");
  }
  // スプライト作成
  sprite.setColorDepth(1);
  sprite.createPalette();
  sprite.createSprite(200, 200);

  sprite.clear(TFT_WHITE);
  sprite.setFont(&fonts::lgfxJapanGothicP_20);
  sprite.setTextSize(1);
  sprite.setTextColor(TFT_BLACK, TFT_WHITE);

  pushSprite(&InkPageSprite, &sprite);
  // Serial.begin(115200);
  // while (!Serial) {
  //   delay(100);
  // }

  // Wire.begin();

  uint16_t error;
  char errorMessage[256];

  scd4x.begin(Wire);

  // stop potentially previously started measurement
  error = scd4x.stopPeriodicMeasurement();
  if (error) {
    Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }

  uint16_t serial0;
  uint16_t serial1;
  uint16_t serial2;
  error = scd4x.getSerialNumber(serial0, serial1, serial2);
  if (error) {
    Serial.print("Error trying to execute getSerialNumber(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }
  else {
    printSerialNumber(serial0, serial1, serial2);
  }

  // Start Measurement
  error = scd4x.startPeriodicMeasurement();
  if (error) {
    Serial.print("Error trying to execute startPeriodicMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }

  Serial.println("Waiting for first measurement... (5 sec)");
}

void loop() {
  M5.update();

  if (M5.BtnPWR.isPressed()) {
    M5.shutdown();
  }

  uint16_t error;
  char errorMessage[256];

  // Read Measurement
  uint16_t co2 = 0;
  float temperature = 0.0f;
  float humidity = 0.0f;
  bool isDataReady = false;
  error = scd4x.getDataReadyFlag(isDataReady);
  if (error) {
    Serial.print("Error trying to execute readMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
    return;
  }
  if (!isDataReady) {
    return;
  }
  error = scd4x.readMeasurement(co2, temperature, humidity);
  if (error) {
    Serial.print("Error trying to execute readMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }
  else if (co2 == 0) {
    Serial.println("Invalid sample detected, skipping.");
  }
  else {
    Serial.print("Co2:");
    Serial.print(co2);
    Serial.print("\t");
    Serial.print("Temperature:");
    Serial.print(temperature);
    Serial.print("\t");
    Serial.print("Humidity:");
    Serial.println(humidity);

    sprite.clear(TFT_WHITE);
    sprite.setFont(&fonts::lgfxJapanGothicP_20);
    sprite.setTextSize(1);
    sprite.setTextColor(TFT_BLACK, TFT_WHITE);

    sprite.setCursor(0, 65);
    sprite.printf("気温%2.0f℃ 湿度%2.0f％\n", temperature, humidity);

    sprite.setCursor(0, 105);
    sprite.setTextSize(3);
    sprite.printf("%4d", co2);
    sprite.setTextSize(1);
    sprite.print("ppm");
    sprite.setCursor(0, 90);
    sprite.printf("二酸化炭素濃度\n");

    pushSprite(&InkPageSprite, &sprite);
    delay(10000);
  }
}
