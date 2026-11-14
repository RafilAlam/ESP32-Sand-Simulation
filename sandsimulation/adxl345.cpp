#include <adxl345.h>

SPISettings adxlSettings(2000000, SPI_MSBFIRST, SPI_MODE3);

void adxl_write(uint8_t reg, uint8_t val) {
  SPI.beginTransaction(adxlSettings);
  digitalWrite(ADXL_CS, LOW);
  SPI.transfer(reg);
  SPI.transfer(val);
  digitalWrite(ADXL_CS, HIGH);
  SPI.endTransaction();
}

uint8_t adxl_read(uint8_t reg) {
  SPI.beginTransaction(adxlSettings);
  digitalWrite(ADXL_CS, LOW);
  SPI.transfer(0x80 | reg);
  uint8_t val = SPI.transfer(0x00);
  digitalWrite(ADXL_CS, HIGH);
  SPI.endTransaction();
  return val;
}

bool adxl_init() {
  pinMode(ADXL_CS, OUTPUT);
  digitalWrite(ADXL_CS, HIGH);
  SPI.begin(ADXL_SCK, ADXL_MISO, ADXL_MOSI, ADXL_CS);

  if (adxl_read(0x00) != 0xE5) return false;

  adxl_write(0x31, 0x0B);
  adxl_write(0x2D, 0x08);
  return true;
}

void adxl_readaccel(FVec3& vec3) {
  uint8_t buf[6];
  SPI.beginTransaction(adxlSettings);
  digitalWrite(ADXL_CS, LOW);
  SPI.transfer(0x80 | 0x40 | 0x32); // Read + Multi-byte + DATAX0
  for (int i=0; i<6; ++i) buf[i] = SPI.transfer(0x00);
  digitalWrite(ADXL_CS, HIGH);
  SPI.endTransaction();

  vec3.x = (int16_t)((buf[1] << 8) | buf[0]) * 0.038245f;
  vec3.y = (int16_t)((buf[3] << 8) | buf[2]) * -0.038245f;
  vec3.z = (int16_t)((buf[5] << 8) | buf[4]) * 0.038245f;

  if (vec3.x < ACCEL_DEADZONE && vec3.x > -ACCEL_DEADZONE)
    vec3.x = 0.0f;
  if (vec3.y < ACCEL_DEADZONE && vec3.y > -ACCEL_DEADZONE)
    vec3.y = 0.0f;
}