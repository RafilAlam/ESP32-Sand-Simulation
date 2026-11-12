#pragma once

#include <SPI.h>
#include <Arduino.h>

#define ADXL_SCK 4
#define ADXL_MISO 5
#define ADXL_MOSI 6
#define ADXL_CS 7

#define ACCEL_DEADZONE 0.1f

struct floatvec3 {
  float x;
  float y;
  float z;
};

void adxl_write(uint8_t reg, uint8_t val);
uint8_t adxl_read(uint8_t reg);

bool adxl_init();

void adxl_readaccel(floatvec3& vec3);