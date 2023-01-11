
#include <Arduino.h>
#include <M5CoreInk.h>
#define LGFX_M5STACK_COREINK // M5Stack CoreInk
#include <LovyanGFX.hpp>
#include <SensirionI2CScd4x.h>
#include <Wire.h>
#include <esp_adc_cal.h>

SensirionI2CScd4x scd4x;

struct beans {
  uint16_t co2 = 0;
  float tempeature = 0;
  float humidity = 0;
} data;

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

float getBatVoltage() {
  analogSetPinAttenuation(35, ADC_11db);
  esp_adc_cal_characteristics_t *adc_chars =
      (esp_adc_cal_characteristics_t *)calloc(
          1, sizeof(esp_adc_cal_characteristics_t));
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 3600,
                           adc_chars);
  uint16_t ADCValue = analogRead(35);

  uint32_t BatVolmV = esp_adc_cal_raw_to_voltage(ADCValue, adc_chars);
  float BatVol = float(BatVolmV) * 25.1 / 5.1 / 1000;
  free(adc_chars);
  return BatVol;
}

const float minVoltage = 3.3;
int getBatCapacity() {
  // 4.02 = 100%, 3.65 = 0%
  const float maxVoltage = 3.98;

  // int cap = (int) (100.0 * (getBatVoltage() - minVoltage)
  //                  / (maxVoltage - minVoltage));
  // cap = constrain(cap, 0, 100);

  int cap =
      map(getBatVoltage() * 100, minVoltage * 100, maxVoltage * 100, 0, 100);
  // if (cap > 100) {
  //   cap = 100;
  // }
  // if (cap < 0) {
  //   cap = 0;
  // }
  cap = constrain(cap, 0, 100);
  return cap;
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

void makeSprite() {
  sprite.clear(TFT_WHITE);
  sprite.setFont(&fonts::lgfxJapanGothicP_20);
  sprite.setTextSize(1);
  sprite.setTextColor(TFT_BLACK, TFT_WHITE);

  Serial.print("Co2:");
  Serial.print(data.co2);
  Serial.print("\t");
  Serial.print("Temperature:");
  Serial.print(data.tempeature);
  Serial.print("\t");
  Serial.print("Humidity:");
  Serial.println(data.humidity);

  sprite.clear(TFT_WHITE);

  sprite.setFont(&fonts::lgfxJapanGothicP_20);
  sprite.setTextSize(1);
  sprite.setTextColor(TFT_BLACK, TFT_WHITE);

  sprite.fillCircle(180, 20, 20, TFT_BLACK);
  sprite.fillCircle(180, 20, 16, TFT_WHITE);
  sprite.fillCircle(180, 20, 12, TFT_BLACK);
  sprite.fillCircle(180, 20, 8, TFT_WHITE);
  sprite.fillCircle(180, 20, 4, TFT_BLACK);
  sprite.fillRect(160, 0, 20, 40, TFT_WHITE);
  sprite.fillRect(160, 20, 40, 20, TFT_WHITE);

  sprite.setCursor(0, 65);
  sprite.printf("気温%2.0f℃ 湿度%2.0f％\n", data.tempeature, data.humidity);

  sprite.setCursor(0, 105);
  sprite.setTextSize(3);
  sprite.printf("%4d", data.co2);
  sprite.setTextSize(1);
  sprite.print("ppm");
  sprite.setCursor(0, 90);
  sprite.printf("二酸化炭素濃度\n");
  sprite.fillRect(185, 22, 10, 10, 0);
  sprite.fillRect(180, 27, 20, 35, 0);
  // sprite.fillRect(185, 42, 15, 25 * (100 - getBatCapacity()) / 100, 1);
  sprite.fillRect(185, 32, 10, 25, TFT_WHITE);

  RTC_TimeTypeDef RTCtime;
  RTC_DateTypeDef RTCDate;
  char timeStrbuff[20];

  M5.rtc.GetTime(&RTCtime);
  M5.rtc.GetDate(&RTCDate);
  int hour = 0;
  hour = RTCtime.Hours + (RTCDate.Date - 1) * 24;
  // 100時間を超えていたら2桁に切り捨てる
  if (100 <= hour) {
    hour %= 100;
  }

  sprintf(timeStrbuff, "%02d:%02d", hour, RTCtime.Minutes);

  sprite.setCursor(0, 0);
  sprite.setTextColor(TFT_BLACK, TFT_WHITE);
  sprite.setFont(&fonts::Font7);
  sprite.setTextSize(1.3);
  sprite.print(timeStrbuff);

  pushSprite(&InkPageSprite, &sprite);
}

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
  error = scd4x.readMeasurement(data.co2, data.tempeature, data.humidity);
  if (error) {
    Serial.print("Error trying to execute readMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }
  else if (data.co2 == 0) {
    Serial.println("Invalid sample detected, skipping.");
  }
  else {
    makeSprite();
    delay(10000);
  }
}
