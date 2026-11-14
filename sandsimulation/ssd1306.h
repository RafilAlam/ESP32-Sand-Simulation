#pragma once

#include <Wire.h>

#define I2C_SDA 0
#define I2C_SCL 1
#define SSD_ADDRESS 0x3C

#define SCREEN_HEIGHT 64
#define SCREEN_WIDTH 128
#define NUM_PAGES SCREEN_HEIGHT/8
#define GAP_THRESHOLD 8

extern uint8_t framebuffer[SCREEN_WIDTH * NUM_PAGES];
extern uint8_t displaybuffer[SCREEN_WIDTH * NUM_PAGES];

void ssd_init();
void ssd_setCursor(uint8_t col, uint8_t page);
void ssd_display();
void ssd_clear();