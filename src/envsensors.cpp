#include <envsensors.hpp>

SHT3X sht3x;
BMM150 bmm = BMM150();
bmm150_mag_data value_offset;
Adafruit_BMP280 bmp;

float tmp = 0.0;
float hum = 0.0;
float pressure = 0.0;
int setup_flag = 1;

void bmmCalibration(uint32_t timeout) {
    int16_t value_x_min = 0;
    int16_t value_x_max = 0;
    int16_t value_y_min = 0;
    int16_t value_y_max = 0;
    int16_t value_z_min = 0;
    int16_t value_z_max = 0;
    uint32_t timeStart = 0;

    bmm.read_mag_data();
    value_x_min = bmm.raw_mag_data.raw_datax;
    value_x_max = bmm.raw_mag_data.raw_datax;
    value_y_min = bmm.raw_mag_data.raw_datay;
    value_y_max = bmm.raw_mag_data.raw_datay;
    value_z_min = bmm.raw_mag_data.raw_dataz;
    value_z_max = bmm.raw_mag_data.raw_dataz;
    delay(100);

    timeStart = millis();

    Serial.print("[BMM150] calibration.");

    while ((millis() - timeStart) < timeout) {
        bmm.read_mag_data();

        /* Update x-Axis max/min value */
        if (value_x_min > bmm.raw_mag_data.raw_datax) {
            value_x_min = bmm.raw_mag_data.raw_datax;
            // Serial.print("Update value_x_min: ");
            // Serial.println(value_x_min);

        } else if (value_x_max < bmm.raw_mag_data.raw_datax) {
            value_x_max = bmm.raw_mag_data.raw_datax;
            // Serial.print("update value_x_max: ");
            // Serial.println(value_x_max);
        }

        /* Update y-Axis max/min value */
        if (value_y_min > bmm.raw_mag_data.raw_datay) {
            value_y_min = bmm.raw_mag_data.raw_datay;
            // Serial.print("Update value_y_min: ");
            // Serial.println(value_y_min);

        } else if (value_y_max < bmm.raw_mag_data.raw_datay) {
            value_y_max = bmm.raw_mag_data.raw_datay;
            // Serial.print("update value_y_max: ");
            // Serial.println(value_y_max);
        }

        /* Update z-Axis max/min value */
        if (value_z_min > bmm.raw_mag_data.raw_dataz) {
            value_z_min = bmm.raw_mag_data.raw_dataz;
            // Serial.print("Update value_z_min: ");
            // Serial.println(value_z_min);

        } else if (value_z_max < bmm.raw_mag_data.raw_dataz) {
            value_z_max = bmm.raw_mag_data.raw_dataz;
            // Serial.print("update value_z_max: ");
            // Serial.println(value_z_max);
        }

        Serial.print(".");
        delay(1);
    }

    Serial.println("done.");

    value_offset.x = value_x_min + (value_x_max - value_x_min) / 2;
    value_offset.y = value_y_min + (value_y_max - value_y_min) / 2;
    value_offset.z = value_z_min + (value_z_max - value_z_min) / 2;
}

void bmmInit() {
    if (bmm.initialize() == BMM150_E_ID_NOT_CONFORM) {
        Serial.println("[BMM150] Chip ID can not read!");
    } else {
        Serial.println("[BMM150] Initialize done!");
        bmmCalibration(10);
    }
}

void bmpInit() {
    if (!bmp.begin(0x76)) {
        Serial.println("[BMP280] Could not find a valid BMP280 sensor, check wiring!");
    }
}

void envsensors_init() {
    bmmInit();
    bmpInit();
}

void envsensors_loop() {

    // M5.Lcd.setCursor(0, 20, 2);
    // M5.Lcd.printf("Temp: %2.1f Humi: %2.0f", tmp, hum);

    bmm150_mag_data value;
    bmm.read_mag_data();

    value.x = bmm.raw_mag_data.raw_datax - value_offset.x;
    value.y = bmm.raw_mag_data.raw_datay - value_offset.y;
    value.z = bmm.raw_mag_data.raw_dataz - value_offset.z;

    float xyHeading = atan2(value.x, value.y);
    float zxHeading = atan2(value.z, value.x);
    float heading = xyHeading;

    if (heading < 0)
        heading += 2 * PI;
    if (heading > 2 * PI)
        heading -= 2 * PI;
    float headingDegrees = heading * 180 / M_PI;
    float xyHeadingDegrees = xyHeading * 180 / M_PI;
    float zxHeadingDegrees = zxHeading * 180 / M_PI;

    Serial.print("[BMM150] Heading: ");
    Serial.println(headingDegrees);
    Serial.print("[BMM150] xyHeadingDegrees: ");
    Serial.println(xyHeadingDegrees);
    Serial.print("[BMM150] zxHeadingDegrees: ");
    Serial.println(zxHeadingDegrees);

    // M5.Lcd.setCursor(0, 40, 2);
    // M5.Lcd.printf("headingDegrees: %2.1f", headingDegrees);
    if (sht3x.get() == 0) {
        tmp = sht3x.cTemp;
        hum = sht3x.humidity;
        Serial.printf("[SHT30] humi: %2.1f\n", hum);
        Serial.printf("[SHT30] temp: %2.1f\n", tmp);
    }
    float pressure = bmp.readPressure();
    float temp = bmp.readTemperature();
    Serial.printf("[BMP280] pressure: %2.1f\n", pressure);
    Serial.printf("[BMP280] temp: %2.1f\n", temp);
    // M5.Lcd.setCursor(0, 60, 2);
    // M5.Lcd.printf("pressure: %2.1f", pressure);
    delay(100);

    if (!setup_flag) {
        setup_flag = 1;
        bmmInit();
        bmpInit();
    }
}