#include "esp_random.h"
#include <types.h>
#include <ssd1306.h>
#include <adxl345.h>

#define BUTTON_LEFT 3
#define BUTTON_RIGHT 10

float magnitude(const FVec2& vec) {
  return sqrtf(vec.x*vec.x + vec.y*vec.y);
}

FVec2 normal(const FVec2& vec) {
  float mag = magnitude(vec);
  return FVec2{vec.x / mag, vec.y / mag};
}

float dot(const FVec2& a, const FVec2& b) {
  return a.x*b.x + a.y*b.y;
}

static inline bool pixel_exists(IVec2 coord) {
  return (coord.x < SCREEN_WIDTH && coord.x >= 0) && (coord.y < SCREEN_HEIGHT && coord.y >= 0);
}

static inline Pixel pixel_get(uint8_t* buffer, IVec2 coord) {
  Pixel p;
  p.byte = &buffer[SCREEN_WIDTH * (coord.y / 8) + coord.x];
  p.mask = 1 << (coord.y % 8);
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

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT, INPUT_PULLUP);

  ssd_init();

  SPI.begin(ADXL_SCK, ADXL_MISO, ADXL_MOSI, ADXL_CS);
  if (!adxl_init()) {
    for (;;);
  }

  srand(esp_random());
}

FVec3 accel_reading;
bool changed;

FVec2 neighbouroffset[8] = {
  {-1, -1}, {0, -1}, {1,-1},
  {-1, 0},           {1, 0},
  {-1, 1}, {0, 1},   {1, 1}
};

float score[8];

void loop() {
  uint8_t leftbutton = digitalRead(BUTTON_LEFT);
  uint8_t rightbutton = digitalRead(BUTTON_RIGHT);

  adxl_readaccel(accel_reading);

  if (rightbutton == LOW) {
    pixel_set(pixel_get(framebuffer, IVec2(64, 0)));
  }

  FVec2 gravitydir = normal(FVec2{accel_reading.x, accel_reading.y});
  for (uint32_t x=0; x<SCREEN_WIDTH; ++x) {
    for (uint32_t y=0; y<SCREEN_HEIGHT; ++y) {
      FVec2 currpos{x, y};
      if (!pixel_exists(currpos) || !pixel_read(pixel_get(displaybuffer, currpos)))
        continue;

      float highestscore = 0.0f;
      int8_t bestoffset{-1};
      for (uint8_t i=0; i<8; ++i) {
        IVec2 movepos = currpos + neighbouroffset[i];
        if (!pixel_exists(movepos) || pixel_read(pixel_get(displaybuffer, movepos)) || pixel_read(pixel_get(framebuffer, movepos))) {
          score[i] = -100.0f;
          continue;
        }

        score[i] = dot(gravitydir, neighbouroffset[i]);
        if (score[i] > highestscore) {
          highestscore = score[i];
          bestoffset = i;
        }
      }
      if (bestoffset == -1)
        continue;

      FVec2 bestmove = currpos + neighbouroffset[bestoffset];
      Pixel currpixel = pixel_get(framebuffer, IVec2(x, y));
      Pixel nextpixel = pixel_get(framebuffer, IVec2(bestmove));
      pixel_move(currpixel, nextpixel);
    }
  }

  if (leftbutton == LOW) {
    ssd_clear();
  }

  ssd_display();
}
