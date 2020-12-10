#include <Arduino.h>
#include <M5CoreInk.h>
#include <WiFi.h>
#include <time.h>
#include <Preferences.h>
#include <envsensors.hpp>
#include "esp_adc_cal.h"
#include "icon.h"
#include "AXP192.h"

#define WIFI_RETRY_CONNECTION 10

Ink_Sprite TimePageSprite(&M5.M5Ink);
// Ink_Sprite TimeSprite(&M5.M5Ink);
// Ink_Sprite DateSprite(&M5.M5Ink);

RTC_TimeTypeDef RTCtime, RTCTimeSave;
RTC_DateTypeDef RTCDate;
uint8_t second = 0, minutes = 0;

Preferences preferences;

const char* NTP_SERVER = "ch.pool.ntp.org";
const char* TZ_INFO    = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00";  // enter your time zone (https://remotemonitoringsystems.ca/time-zone-abbreviations.php)

tm timeinfo;
time_t now;
long unsigned lastNTPtime;
unsigned long lastEntryTime;

//!Power
AXP192 axp = AXP192();

void drawImageToSprite(int posX, int posY, image_t *imagePtr, Ink_Sprite *sprite) {
    sprite->drawBuff(posX, posY,
                     imagePtr->width, imagePtr->height, imagePtr->ptr);
}

void drawTime(RTC_TimeTypeDef *time) {
    drawImageToSprite(10, 48, &num55[time->Hours / 10], &TimePageSprite);
    drawImageToSprite(50, 48, &num55[time->Hours % 10], &TimePageSprite);
    drawImageToSprite(90, 48, &num55[10], &TimePageSprite);
    drawImageToSprite(110, 48, &num55[time->Minutes / 10], &TimePageSprite);
    drawImageToSprite(150, 48, &num55[time->Minutes % 10], &TimePageSprite);
}

void drawDate(RTC_DateTypeDef *date) {
    int posX = 15, num = 0;
    for (int i = 0; i < 4; i++) {
        num = (date->Year / int(pow(10, 3 - i)) % 10);
        drawImageToSprite(posX, 124, &num18x29[num], &TimePageSprite);
        posX += 17;
    }
    drawImageToSprite(posX, 124, &num18x29[10], &TimePageSprite);
    posX += 17;

    drawImageToSprite(posX, 124, &num18x29[date->Month / 10 % 10], &TimePageSprite);
    posX += 17;
    drawImageToSprite(posX, 124, &num18x29[date->Month % 10], &TimePageSprite);
    posX += 17;

    drawImageToSprite(posX, 124, &num18x29[10], &TimePageSprite);
    posX += 17;

    drawImageToSprite(posX, 124, &num18x29[date->Date / 10 % 10], &TimePageSprite);
    posX += 17;
    drawImageToSprite(posX, 124, &num18x29[date->Date % 10], &TimePageSprite);
    posX += 17;
}


void drawScanWifi() {
    M5.M5Ink.clear();
    TimePageSprite.clear();
}

void drawWarning(const char *str) {
    M5.M5Ink.clear();
    TimePageSprite.clear(CLEAR_DRAWBUFF | CLEAR_LASTBUFF);
    drawImageToSprite(76, 40, &warningImage, &TimePageSprite);
    int length = 0;
    while (*(str + length) != '\0') length++;
    TimePageSprite.drawString((200 - length * 8) / 2, 100, str, &AsciiFont8x16);
    TimePageSprite.pushSprite();
}

void drawTimePage() {
    M5.rtc.GetTime(&RTCtime);
    drawTime(&RTCtime);
    minutes = RTCtime.Minutes;
    M5.rtc.GetDate(&RTCDate);
    drawDate(&RTCDate);
    TimePageSprite.pushSprite();
}

void drawSensors() {
}

void saveBool(String key, bool value){
    preferences.begin("M5CoreInk", false);
    preferences.putBool(key.c_str(), value);
    preferences.end();
}

bool loadBool(String key) {
    preferences.begin("M5CoreInk", false);
    bool keyvalue =preferences.getBool(key.c_str(),false);
    preferences.end();
    return keyvalue;
}

void flushTimePage() {
    // M5.M5Ink.clear();
    // TimePageSprite.clear( CLEAR_DRAWBUFF | CLEAR_LASTBUFF );
    // drawTimePage();
    while (1) {
        M5.rtc.GetTime(&RTCtime);
        if (minutes != RTCtime.Minutes) {
            M5.rtc.GetTime(&RTCtime);
            M5.rtc.GetDate(&RTCDate);

            // if (RTCtime.Minutes % 10 == 0) {
            //     M5.M5Ink.clear();
            //     TimePageSprite.clear(CLEAR_DRAWBUFF | CLEAR_LASTBUFF);
            // }
            drawTime(&RTCtime);
            drawDate(&RTCDate);
            TimePageSprite.pushSprite();
            minutes = RTCtime.Minutes;
            // saveBool("clock_suspend",true);
            M5.shutdown(58);
        }

        delay(10);
        M5.update();
        if (M5.BtnPWR.wasPressed()) {
            digitalWrite(LED_EXT_PIN, LOW);
            saveBool("clock_suspend",false);
            M5.shutdown();
        }
        if (M5.BtnDOWN.wasPressed() || M5.BtnUP.wasPressed()) break;
    }
    M5.M5Ink.clear();
    TimePageSprite.clear(CLEAR_DRAWBUFF | CLEAR_LASTBUFF);
}

float getBatVoltage() {
    analogSetPinAttenuation(35, ADC_11db);
    esp_adc_cal_characteristics_t *adc_chars = (esp_adc_cal_characteristics_t *)calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 3600, adc_chars);
    uint16_t ADCValue = analogRead(35);

    uint32_t BatVolmV = esp_adc_cal_raw_to_voltage(ADCValue, adc_chars);
    float BatVol = float(BatVolmV) * 25.1 / 5.1 / 1000;
    return BatVol;
}

void checkBatteryVoltage(bool powerDownFlag) {
    float batVol = getBatVoltage();
    Serial.printf("[BATT] Voltage %.2f\r\n", batVol);

    if (batVol > 3.2) return;

    drawWarning("Battery voltage is low");
    if (powerDownFlag == true) {
        M5.shutdown();
    }
    while (1) {
        batVol = getBatVoltage();
        if (batVol > 3.2) return;
    }
}

void checkRTC() {
    M5.rtc.GetTime(&RTCtime);
    if (RTCtime.Seconds == RTCTimeSave.Seconds) {
        drawWarning("RTC Error");
        while (1) {
            if (M5.BtnMID.wasPressed()) return;
            delay(10);
            M5.update();
        }
    }
}

void showTime(tm localTime) {
    Serial.print("[NTP] ");
    Serial.print(localTime.tm_mday);
    Serial.print('/');
    Serial.print(localTime.tm_mon + 1);
    Serial.print('/');
    Serial.print(localTime.tm_year - 100);
    Serial.print('-');
    Serial.print(localTime.tm_hour);
    Serial.print(':');
    Serial.print(localTime.tm_min);
    Serial.print(':');
    Serial.print(localTime.tm_sec);
    Serial.print(" Day of Week ");
    if (localTime.tm_wday == 0)
        Serial.println(7);
    else
        Serial.println(localTime.tm_wday);
}

void saveRtcData() {
    RTCtime.Minutes = timeinfo.tm_min;
    RTCtime.Seconds = timeinfo.tm_sec;
    RTCtime.Hours = timeinfo.tm_hour;
    RTCDate.Year = timeinfo.tm_year+1900;
    RTCDate.Month = timeinfo.tm_mon+1;
    RTCDate.Date = timeinfo.tm_mday;
    RTCDate.WeekDay = timeinfo.tm_wday;

    char timeStrbuff[64];
    sprintf(timeStrbuff, "%d/%02d/%02d %02d:%02d:%02d",
            RTCDate.Year, RTCDate.Month, RTCDate.Date,
            RTCtime.Hours, RTCtime.Minutes, RTCtime.Seconds);

    Serial.println("[NTP] in: " + String(timeStrbuff));

    M5.rtc.SetTime(&RTCtime);
    M5.rtc.SetDate(&RTCDate);
}

bool getNTPtime(int sec) {
    {
        Serial.print("[NTP] sync.");
        uint32_t start = millis();
        do {
            time(&now);
            localtime_r(&now, &timeinfo);
            Serial.print(".");
            delay(10);
        } while (((millis() - start) <= (1000 * sec)) && (timeinfo.tm_year < (2016 - 1900)));
        if (timeinfo.tm_year <= (2016 - 1900)) return false;  // the NTP call was not successful

        Serial.print("now ");
        Serial.println(now);
        saveRtcData();
        char time_output[30];
        strftime(time_output, 30, "%a  %d-%m-%y %T", localtime(&now));
        Serial.print("[NTP] ");
        Serial.println(time_output);
    }
    return true;
}

void ntpInit() {
    if (WiFi.isConnected()) {
        configTime(0, 0, NTP_SERVER);
        // See https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv for Timezone codes for your region
        setenv("TZ", TZ_INFO, 1);
        if (getNTPtime(10)) {  // wait up to 10sec to sync
        } else {
            Serial.println("[NTP] Time not set");
            ESP.restart();
        }
        showTime(timeinfo);
        lastNTPtime = time(&now);
        lastEntryTime = millis();
        M5.Speaker.tone(2700,200);
        delay(100);
        M5.Speaker.mute();
    }
}

void wifiInit() {
    Serial.print("[WiFi] connecting to "+String(WIFI_SSID));
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    int wifi_retry = 0;
    while (WiFi.status() != WL_CONNECTED && wifi_retry++ < WIFI_RETRY_CONNECTION) { 
        Serial.print(".");
        delay(500);
    }
    if (wifi_retry >= WIFI_RETRY_CONNECTION)
        Serial.println(" failed!");
    else
        Serial.println(" connected!");
}

void setup() {
    M5.begin();
    axp.begin();
    // digitalWrite(LED_EXT_PIN, HIGH);   // turnoff it for improve battery life
    digitalWrite(LED_EXT_PIN, LOW);   // turnoff it for improve battery life
    // Wire.begin(25,26);              // for Hat sensors
    delay(100);
    Serial.println(__TIME__);
    M5.rtc.GetTime(&RTCTimeSave);
    M5.update();
    if (M5.BtnMID.isPressed()) {
        M5.M5Ink.clear();
        M5.M5Ink.drawBuff((uint8_t *)image_CoreInkWWellcome);
        delay(100);
        wifiInit();
        ntpInit();
    }
    checkBatteryVoltage(false);
    TimePageSprite.creatSprite(0, 0, 200, 200);
    //TimePageSprite.clear( CLEAR_DRAWBUFF | CLEAR_LASTBUFF );
    // delay(500);
    // envsensors_init();
    // M5.Speaker.tone(2700,200);
    // M5.M5Ink.clear();
    // checkRTC();
    drawTimePage();
}

void loop() {
    flushTimePage();
    // envsensors_loop();

    /*
    if( M5.BtnUP.wasPressed())
    {

    }
    if( M5.BtnDOWN.wasPressed())
    {

    }
    if( M5.BtnMID.wasPressed())
    {

    */
    if (M5.BtnPWR.wasPressed()) {
        Serial.printf("Btn %d was pressed \r\n", BUTTON_EXT_PIN);
        digitalWrite(LED_EXT_PIN, LOW);
        M5.shutdown();
    }
    M5.update();
}