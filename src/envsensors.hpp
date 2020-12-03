
#include <Wire.h>
#include "SHT3X.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include "bmm150.h"
#include "bmm150_defs.h"

void calibrate(uint32_t timeout);
void envsensors_init();
void envsensors_loop();