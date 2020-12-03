#include <envsensors.hpp>

SHT3X sht3x;
BMM150 bmm = BMM150();
bmm150_mag_data value_offset;
Adafruit_BMP280 bme;

float tmp = 0.0;
float hum = 0.0;
float pressure = 0.0;
int setup_flag = 1;

void calibrate(uint32_t timeout) {
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

    value_offset.x = value_x_min + (value_x_max - value_x_min) / 2;
    value_offset.y = value_y_min + (value_y_max - value_y_min) / 2;
    value_offset.z = value_z_min + (value_z_max - value_z_min) / 2;
}

void envsensors_init() {
    Wire.begin(25,26);
    delay(1000);
    if (bmm.initialize() == BMM150_E_ID_NOT_CONFORM) {
        Serial.println("Chip ID can not read!");
        while (1)
            ;
    } else {
        Serial.println("Initialize done!");
    }
    if (!bme.begin(0x76)) {
        Serial.println("Could not find a valid BMP280 sensor, check wiring!");
        while (1)
            ;
    }
    calibrate(10);
    Serial.print("\n\rCalibrate done..");
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

    Serial.println("");
    Serial.print("Heading: ");
    Serial.println(headingDegrees);
    Serial.print("xyHeadingDegrees: ");
    Serial.println(xyHeadingDegrees);
    Serial.print("zxHeadingDegrees: ");
    Serial.println(zxHeadingDegrees);

    // M5.Lcd.setCursor(0, 40, 2);
    // M5.Lcd.printf("headingDegrees: %2.1f", headingDegrees);
    if (sht3x.get() == 0) {
        tmp = sht3x.cTemp;
        hum = sht3x.humidity;
        Serial.printf("[SHT30] humi: %2.1f\n", hum);
        Serial.printf("[SHT30] temp: %2.1f\n", tmp);
    }
    float pressure = bme.readPressure();
    float temp = bme.readTemperature();
    Serial.printf("[BMP280] pressure: %2.1f\n", pressure);
    Serial.printf("[BMP280] temp: %2.1f\n", temp);
    // M5.Lcd.setCursor(0, 60, 2);
    // M5.Lcd.printf("pressure: %2.1f", pressure);
    delay(100);

    if (!setup_flag) {
        setup_flag = 1;

        if (bmm.initialize() == BMM150_E_ID_NOT_CONFORM) {
            Serial.println("Chip ID can not read!");
            while (1)
                ;
        } else {
            Serial.println("Initialize done!");
        }
        if (!bme.begin(0x76)) {
            Serial.println("Could not find a valid BMP280 sensor, check wiring!");
            while (1)
                ;
        }
        calibrate(10);
        Serial.print("\n\rCalibrate done..");
    }
}