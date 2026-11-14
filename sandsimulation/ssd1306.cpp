#include <ssd1306.h>

uint8_t framebuffer[SCREEN_WIDTH * NUM_PAGES];
uint8_t displaybuffer[SCREEN_WIDTH * NUM_PAGES];

void ssd1306_command(uint8_t cmd) {
  Wire.beginTransmission(SSD_ADDRESS);
  Wire.write(0x00); // Control byte: Command mode
  Wire.write(cmd);
  Wire.endTransmission();
}

void ssd1306_setCursor(uint8_t col, uint8_t page) {
  Wire.beginTransmission(SSD_ADDRESS);
  Wire.write(0x00);             // Command mode
  Wire.write(0x21);             // Set Column Address
  Wire.write(col);              // Start col
  Wire.write(SCREEN_WIDTH - 1); // End col (127)
  Wire.write(0x22);             // Set Page Address
  Wire.write(page);             // Start page
  Wire.write(page);             // End page (stay on this page)
  Wire.endTransmission();
}

void ssd_display() {
  for (uint8_t page = 0; page < NUM_PAGES; page++) {
    uint16_t offset = page * SCREEN_WIDTH;
    uint8_t col = 0;

    while (col < SCREEN_WIDTH) {
      while (col < SCREEN_WIDTH && framebuffer[offset + col] == displaybuffer[offset + col]) {
        col++;
      }

      if (col >= SCREEN_WIDTH) break;

      ssd1306_setCursor(col, page);

      while (col < SCREEN_WIDTH) {
        uint8_t cleanRun = 0;
        while ((col + cleanRun) < SCREEN_WIDTH && framebuffer[offset + col + cleanRun] == displaybuffer[offset + col + cleanRun]) {
          cleanRun++;
        }

        if ((col + cleanRun) >= SCREEN_WIDTH || cleanRun > GAP_THRESHOLD) {
          col += cleanRun;
          break;
        }

        Wire.beginTransmission(SSD_ADDRESS);
        Wire.write(0x40); // Data mode

        uint8_t packetBytes = 0;
        while (col < SCREEN_WIDTH && packetBytes < 127) {
          if (framebuffer[offset + col] == displaybuffer[offset + col]) {
            uint8_t upcomingClean = 0;
            while ((col + upcomingClean) < SCREEN_WIDTH && framebuffer[offset + col + upcomingClean] == displaybuffer[offset + col + upcomingClean]) {
              upcomingClean++;
            }
            if ((col + upcomingClean) >= SCREEN_WIDTH || upcomingClean > GAP_THRESHOLD) {
              break;
            }
          }

          uint8_t b = framebuffer[offset + col];
          Wire.write(b);
          displaybuffer[offset + col] = b;
          col++;
          packetBytes++;
        }

        Wire.endTransmission();
      }
    }
  }
}

void ssd_clear() {
  memset(framebuffer, 0x00, sizeof(framebuffer));
}

void ssd_init() {
  Wire.begin(I2C_SDA, I2C_SCL, SSD_ADDRESS);
  Wire.setClock(1000000);

    // Standard initialization sequence for 128x64
  ssd1306_command(0xAE); // Display OFF
  ssd1306_command(0xD5); // Set Display Clock Divide Ratio
  ssd1306_command(0x80); // Default ratio
  ssd1306_command(0xA8); // Set Multiplex Ratio
  ssd1306_command(0x3F); // 64MUX (0x3F = 63)
  ssd1306_command(0xD3); // Set Display Offset
  ssd1306_command(0x00); // No offset
  ssd1306_command(0x40); // Set Display Start Line to 0
  ssd1306_command(0x8D); // Charge Pump Setting
  ssd1306_command(0x14); // Enable Charge Pump (Required for internal VCC)
  ssd1306_command(0x20); // Memory Addressing Mode
  ssd1306_command(0x00); // Horizontal Addressing Mode
  ssd1306_command(0xA1); // Segment Re-map (flip horizontally)
  ssd1306_command(0xC8); // COM Output Scan Direction (flip vertically)
  ssd1306_command(0xDA); // Set COM Pins Hardware Configuration
  ssd1306_command(0x12); // Alternative COM pin config
  ssd1306_command(0x81); // Set Contrast Control
  ssd1306_command(0xCF); // High contrast
  ssd1306_command(0xD9); // Set Pre-charge Period
  ssd1306_command(0xF1);
  ssd1306_command(0xDB); // Set VCOMH Deselect Level
  ssd1306_command(0x40);
  ssd1306_command(0xA4); // Entire Display ON (resume from RAM)
  ssd1306_command(0xA6); // Normal display (0xA7 for inverted)
  ssd1306_command(0xAF); // Display ON

  memset(displaybuffer, 0x00, sizeof(displaybuffer));
  memset(framebuffer, 0xFF, sizeof(framebuffer));
  ssd_display();
}