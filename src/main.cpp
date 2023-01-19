
#include <Arduino.h>
#include <M5CoreInk.h>
#define LGFX_M5STACK_COREINK // M5Stack CoreInk
#include <LovyanGFX.hpp>
#include <SensirionI2CScd4x.h>
#include <Wire.h>
#include <WiFi.h>>
#include <esp_adc_cal.h>

SensirionI2CScd4x scd4x;

struct beans {
  uint16_t co2 = 0;
  float tempeature = 0;
  float humidity = 0;
  bool isWifiEnable = false;
  char *message = "";
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

  // Wifiマーク
  sprite.fillCircle(180, 20, 20, TFT_BLACK);
  sprite.fillCircle(180, 20, 16, TFT_WHITE);
  sprite.fillCircle(180, 20, 12, TFT_BLACK);
  sprite.fillCircle(180, 20, 8, TFT_WHITE);
  sprite.fillCircle(180, 20, 4, TFT_BLACK);
  sprite.fillRect(160, 0, 20, 40, TFT_WHITE);
  sprite.fillRect(160, 20, 40, 20, TFT_WHITE);

  RTC_TimeTypeDef RTCtime;
  RTC_DateTypeDef RTCDate;
  char timeStrbuff[20];

  M5.rtc.GetTime(&RTCtime);
  M5.rtc.GetDate(&RTCDate);

  // 時計表示
  sprintf(timeStrbuff, "%02d:%02d", RTCtime.Hours, RTCtime.Minutes);
  sprite.setCursor(0, 0);
  sprite.setTextColor(TFT_BLACK, TFT_WHITE);
  sprite.setFont(&fonts::Font7);
  sprite.setTextSize(1.3);
  sprite.print(timeStrbuff);

  if (data.isWifiEnable) {
  }
  else {
    // wifiが使えない場合アイコンに斜線が入る
    sprite.drawLine(176, 20, 196, 0, TFT_WHITE);
    sprite.drawLine(177, 20, 197, 0, TFT_WHITE);
    sprite.drawLine(178, 20, 198, 0, TFT_WHITE);
    sprite.drawLine(179, 20, 199, 0, TFT_BLACK);
    sprite.drawLine(180, 20, 200, 0, TFT_BLACK);
    sprite.drawLine(181, 20, 201, 0, TFT_BLACK);
    sprite.drawLine(182, 20, 202, 0, TFT_WHITE);
    sprite.drawLine(183, 20, 203, 0, TFT_WHITE);
    sprite.drawLine(184, 20, 204, 0, TFT_WHITE);
  }

  sprite.setFont(&fonts::lgfxJapanGothicP_20);
  sprite.setTextSize(1);
  sprite.setCursor(0, 65);
  sprite.printf("気温%2.0f℃ 湿度%2.0f％\n", data.tempeature, data.humidity);

  sprite.setCursor(0, 105);
  sprite.setTextSize(3);
  sprite.printf("%4d", data.co2);
  sprite.setCursor(155, 135);
  sprite.setTextSize(1);
  sprite.print("ppm");
  sprite.setCursor(0, 90);
  sprite.printf("二酸化炭素濃度\n");

  // 乾電池マーク
  sprite.fillRect(185, 22, 10, 10, 0);
  sprite.fillRect(180, 27, 20, 35, 0);
  sprite.fillRect(185, 32, 10, 25 * (100 - getBatCapacity()) / 100, TFT_WHITE);
  // sprite.fillRect(185, 32, 10, 25, TFT_WHITE);
  if (1000 < data.co2) {
    sprite.setCursor(0, 170);
    sprite.setTextColor(TFT_WHITE, TFT_BLACK);
    sprite.setTextSize(1.2);
    sprite.print("換気してください\n");
    sprite.setTextColor(TFT_BLACK, TFT_WHITE);
    // sendNotify(String(sgp.eCO2) + "ppm 換気してください " +
    //            String(getBatCapacity()) + "％");
  }

  pushSprite(&InkPageSprite, &sprite);
}

void task1(void *pvParameters) {
  while (true) {
    delay(1);
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
      // 5分おきに測定
      delay(5 * 60 * 1000);
    }
  }
}

void setup() {
  M5.begin(true, true, false);
  M5.update();
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

  // Start Measurement
  error = scd4x.startPeriodicMeasurement();
  if (error) {
    Serial.print("Error trying to execute startPeriodicMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }

  Serial.println("Waiting for first measurement... (5 sec)");
  delay(5000);

  if (M5.BtnMID.isPressed()) {
    // calibration start
    uint16_t correction_;
    uint16_t FRC = 400; // FRCのターゲット値
    Serial.println("calibration start");
    sprite.setCursor(0, 0);
    sprite.setTextSize(1);
    sprite.printf(
        "キャリブレーション中です\n空気の綺麗なところに3分間放置してください");
    pushSprite(&InkPageSprite, &sprite);
    scd4x.stopPeriodicMeasurement(); // 定期測定モードを停止
    delay(500);
    scd4x.performFactoryReset();      // 設定の初期化
    scd4x.startPeriodicMeasurement(); // 定期測定モードを開始
    delay(3 * 60 * 1000);             // 3分間通常動作させる
    scd4x.stopPeriodicMeasurement();  // 定期測定モードを停止
    delay(500);
    scd4x.performForcedRecalibration(FRC, correction_); // FRCを実行
    delay(1000);                                        // FRC後1秒待つ

    // 通常モードでの測定開始
    while (scd4x.startPeriodicMeasurement() == false) {
    }
    Serial.println("Completed."); // FRC完了表示

    // M5.Lcd.setCursor(20, 20);
    Serial.printf("FRC. %d\n", correction_); // FRC補正値を表示

    scd4x.stopPeriodicMeasurement(); // 定期測定モードを停止
    uint16_t asc = 1;
    scd4x.setAutomaticSelfCalibration(asc); // ASCの有効化
    scd4x.getAutomaticSelfCalibration(asc);
    Serial.printf("SCD41:ASC: %s\n", asc == 0 ? "OFF" : "ON");
    scd4x.startPeriodicMeasurement(); // 定期測定モードを開始
  }

  WiFi.begin();
  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); // 500ms毎に.を表示
    Serial.print(".");
    count++;
    if (count == 10) {
      // 5秒
      data.isWifiEnable = false;
      Serial.println("Wifi connection failed");
      break;
    }
    data.isWifiEnable = true;
  }
  // NTPで時計合わせをする
  if (data.isWifiEnable) {
    Serial.println("\nConnected");
    Serial.println("ntp configured");

    configTime(9 * 3600L, 0, "ntp.nict.jp", "time.google.com",
               "ntp.jst.mfeed.ad.jp");

    struct tm timeInfo;

    getLocalTime(&timeInfo);

    RTC_TimeTypeDef TimeStruct;

    TimeStruct.Hours = timeInfo.tm_hour;
    TimeStruct.Minutes = timeInfo.tm_min;
    TimeStruct.Seconds = timeInfo.tm_sec;

    M5.Rtc.SetTime(&TimeStruct);
  }
  // loop()内でdelay()を使うとボタンなどが効かなくなるのでマルチタスクで測定する
  xTaskCreateUniversal(task1, "task1", 8192, NULL, 1, NULL, APP_CPU_NUM);
}

void loop() {
  M5.update();

  if (M5.BtnPWR.isPressed()) {
    M5.shutdown();
  }
  if (M5.BtnEXT.isPressed()) {

    esp_restart();
  }

  delay(1);
}
