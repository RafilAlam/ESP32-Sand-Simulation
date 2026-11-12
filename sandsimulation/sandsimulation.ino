#include "esp_random.h"
#include <ssd1306.h>
#include <adxl345.h>

#define BUTTON_LEFT 3
#define BUTTON_RIGHT 10

struct Vec2 {
  uint16_t x;
  uint16_t y;
};

struct Pixel {
  uint8_t* byte;
  uint8_t mask;
};

static inline bool pixel_exists(Vec2 pos) {
  return (pos.x < SCREEN_WIDTH && pos.x >= 0) && (pos.y < SCREEN_HEIGHT && pos.y >= 0);
}

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

bool simulate_sand(const floatvec3& accel) {
  for (int16_t x=0; x<SCREEN_WIDTH; ++x) {
    for (int16_t y=0; y<SCREEN_HEIGHT; ++y) {
      bool display_pixel_filled = pixel_read(pixel_get(displayedbuffer, Vec2{x, y}));
      Vec2 currpos{x, y};
      Vec2 bottompos;

      if (accel.x < ACCEL_DEADZONE && accel.x > -ACCEL_DEADZONE && accel.y < ACCEL_DEADZONE && accel.y > -ACCEL_DEADZONE)
        return 0;

      if (accel.x < -ACCEL_DEADZONE)
        bottompos.x = x-1;
      else if (accel.x > ACCEL_DEADZONE)
        bottompos.x = x+1;
      else
       bottompos.x = x;
       
      if (accel.y < -ACCEL_DEADZONE)
        bottompos.y = y+1;
      else if (accel.y > ACCEL_DEADZONE)
        bottompos.y = y-1;
      else
       bottompos.y = y;

      if (!pixel_exists(currpos) || !display_pixel_filled || !pixel_exists(bottompos))
        continue;

      Pixel simulated_pixel = pixel_get(framebuffer, currpos);
      Pixel simulated_bottom = pixel_get(framebuffer, bottompos);
      bool display_bottom_filled = pixel_read(pixel_get(displayedbuffer, bottompos));
      bool simulated_bottom_filled = pixel_read(simulated_bottom);

      if (!display_bottom_filled && !simulated_bottom_filled) {
        pixel_move(simulated_pixel, simulated_bottom);
      }
      else if (display_bottom_filled && simulated_bottom_filled) {
        bool startleft = esp_random() & 1;
        int16_t diag_x[2] = {currpos.x, currpos.x};
        int16_t diag_y[2] = {currpos.y, currpos.y};

        int8_t dx = bottompos.x - currpos.x;
        int8_t dy = bottompos.y - currpos.y;
        if (dx != 0 && dy != 0) {
          diag_x[0] = startleft ? bottompos.x-dx : bottompos.x;
          diag_x[1] = startleft ? bottompos.x : bottompos.x-dx;
          diag_y[0] = startleft ? bottompos.y : bottompos.y-dy;
          diag_y[1] = startleft ? bottompos.y-dy : bottompos.y;
        }
        else if (dx != 0) {
          diag_y[0] = startleft ? bottompos.y-1 : bottompos.y+1;
          diag_y[1] = startleft ? bottompos.y+1 : bottompos.y-1;
        }
        else if (dy != 0) {
          diag_x[0] = startleft ? bottompos.x-1 : bottompos.x+1;
          diag_x[1] = startleft ? bottompos.x+1 : bottompos.x-1;
        }

        for (uint8_t i = 0; i < 2; ++i) {
          if (!pixel_exists(Vec2{diag_x[i], diag_y[i]}))
            continue;

          Pixel display_diagonal = pixel_get(displayedbuffer, Vec2{diag_x[i], diag_y[i]});
          Pixel simulated_diagonal = pixel_get(framebuffer, Vec2{diag_x[i], diag_y[i]});
          if (!pixel_read(display_diagonal) && !pixel_read(simulated_diagonal)) {
            pixel_move(simulated_pixel, simulated_diagonal);
            break;
          }
        }
      }
    }
  }
  return 1;
}

void setup() {
  pinMode(BUTTON_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT, INPUT_PULLUP);

  ssd_init();

  SPI.begin(ADXL_SCK, ADXL_MISO, ADXL_MOSI, ADXL_CS);
  if (!adxl_init()) {
    for (;;);
  }
}

floatvec3 accel_reading;
bool changed;

void loop() {
  uint8_t leftbutton = digitalRead(BUTTON_LEFT);
  uint8_t rightbutton = digitalRead(BUTTON_RIGHT);
  
  if (leftbutton == LOW) {
    ssd_clear();
  }

  adxl_readaccel(accel_reading);

  if (rightbutton == LOW) {
    pixel_set(pixel_get(displayedbuffer, Vec2{64, 0}));
  }

  bool changed{false};
  changed = simulate_sand(accel_reading);

  ssd_display();
}
