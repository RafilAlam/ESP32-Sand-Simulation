#include "soc/gpio_struct.h"
#include "esp_random.h"
#include <SPI.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ADXL345_U.h>

#define BUTTON_LEFT 3
#define BUTTON_RIGHT 10

#define I2C_SDA 0
#define I2C_SCL 1

#define ADXL_SCK 4
#define ADXL_MISO 5
#define ADXL_MOSI 6
#define ADXL_CS 7

#define SCREEN_HEIGHT 64
#define SCREEN_WIDTH 128

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

Adafruit_ADXL345_U accel = Adafruit_ADXL345_Unified(ADXL_CS, &SPI);

struct Vec2 {
  int16_t x;
  int16_t y;
};
static inline bool pixel_exists(Vec2 pos) {
  return (pos.x < SCREEN_WIDTH && pos.x >= 0) && (pos.y < SCREEN_HEIGHT && pos.y >= 0);
}

struct Pixel {
  uint8_t* byte;
  uint8_t mask;
};
static inline Pixel pixel_get(uint8_t* buffer, Vec2 pos) {
  Pixel p;
  p.byte = &buffer[SCREEN_WIDTH * (pos.y / 8) + pos.x];
  p.mask = 1 << (pos.y % 8);
  return p;
}
static inline bool pixel_read(const Pixel& p) {
  return *p.byte & p.mask;
}
static inline void pixel_set(const Pixel& p) {
  *p.byte |= p.mask;
}
static inline void pixel_clear(const Pixel& p) {
  *p.byte &= ~p.mask;
}

static inline void pixel_move(const Pixel& p, const Pixel& nextp) {
  pixel_clear(p);
  pixel_set(nextp);
}

uint8_t simulation_buffer[SCREEN_WIDTH * SCREEN_HEIGHT / 8] = {};
void simulate_sand(uint8_t* display_buffer) {
  for (int16_t x=SCREEN_WIDTH-1; x>=0; --x) {
    for (int16_t y=SCREEN_HEIGHT-1; y>=0; --y) {
      bool display_pixel_filled = pixel_read(pixel_get(display_buffer, Vec2{x, y}));
      Vec2 currpos{x, y};
      Vec2 bottompos{x, y+1};

      if (!pixel_exists(currpos) || !display_pixel_filled || !pixel_exists(bottompos))
        continue;

      Pixel simulated_pixel = pixel_get(simulation_buffer, currpos);
      Pixel simulated_bottom = pixel_get(simulation_buffer, bottompos);
      bool display_bottom_filled = pixel_read(pixel_get(display_buffer, bottompos));
      bool simulated_bottom_filled = pixel_read(simulated_bottom);

      if (!display_bottom_filled && !simulated_bottom_filled) {
        pixel_move(simulated_pixel, simulated_bottom);
      }
      else if (display_bottom_filled && simulated_bottom_filled) {
        bool startleft = esp_random() & 1;
        int16_t diag_x[2] = {startleft ? x-1 : x+1, startleft ? x+1 : x-1};
        for (uint8_t i = 0; i < 2; ++i) {
          Pixel display_diagonal = pixel_get(display_buffer, Vec2{diag_x[i], y+1});
          Pixel simulated_diagonal = pixel_get(simulation_buffer, Vec2{diag_x[i], y+1});
          if (pixel_exists(Vec2{diag_x[i], y+1}) && !pixel_read(display_diagonal) && !pixel_read(simulated_diagonal)) {
            pixel_move(simulated_pixel, simulated_diagonal);
            break;
          }
        }
      }
    }
  }
  memcpy(display_buffer, simulation_buffer, sizeof(simulation_buffer));
}

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT, INPUT_PULLUP);

  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(1000000);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for (;;);
  }

  display.clearDisplay();
  display.display();

  SPI.begin(ADXL_SCK, ADXL_MISO, ADXL_MOSI, ADXL_CS);
  if (!accel.begin()) {
    for (;;);
  }
}

void loop() {
  uint8_t leftbutton = digitalRead(BUTTON_LEFT);
  uint8_t rightbutton = digitalRead(BUTTON_RIGHT);

  uint8_t* display_buffer = display.getBuffer();
  
  if (leftbutton == LOW) {
    display.clearDisplay();
    memcpy(simulation_buffer, display_buffer, sizeof(simulation_buffer));
  }

  sensors_event_t event;
  accel.getEvent(&event);

  Serial.print("X: "); Serial.print(event.acceleration.x); Serial.print("  ");
  Serial.print("Y: "); Serial.print(event.acceleration.y); Serial.print("  ");
  Serial.print("Z: "); Serial.print(event.acceleration.z); Serial.println(" m/s^2");

  if (rightbutton == LOW) {
    pixel_set(pixel_get(display_buffer, Vec2{64, 0}));
    memcpy(simulation_buffer, display_buffer, sizeof(simulation_buffer));
  }

  simulate_sand(display_buffer);

  display.display();
}
