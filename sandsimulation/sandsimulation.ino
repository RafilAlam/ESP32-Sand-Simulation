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
  Serial.printf("%f, %f", mag, vec.x);
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

IVec2 neighbouroffset[8] = {
  {-1, -1}, {0, -1}, {1,-1},
  {-1, 0},           {1, 0},
  {-1, 1}, {0, 1},   {1, 1}
};

float score[8];

IVec2 activepixels[8192];
uint16_t activepixelcount = 0;

int64_t intervalMicros = 1000;
int64_t previoustime = 0;
void loop() {
  int64_t currenttime = esp_timer_get_time();

  if (currenttime - previoustime > intervalMicros) {
    previoustime = currenttime;
    uint8_t leftbutton = digitalRead(BUTTON_LEFT);
    uint8_t rightbutton = digitalRead(BUTTON_RIGHT);

    adxl_readaccel(accel_reading);

    if (rightbutton == LOW) {
      activepixels[activepixelcount++] = {64, 0};
      pixel_set(pixel_get(framebuffer, IVec2(64, 0)));
    }

    FVec2 gravitydir = normal(FVec2{accel_reading.x, accel_reading.y});

    int64_t computetime = esp_timer_get_time();
    for (int i=0; i<activepixelcount; ++i) {
      IVec2& currpos = activepixels[i];
      float highscore = 0.0f;
      int8_t bestoffset{-1};
      for (uint8_t j=0; j<8; ++j) {
        IVec2 movepos = currpos + neighbouroffset[j];
        if (!pixel_exists(movepos) || pixel_read(pixel_get(framebuffer, movepos))) {
          score[j] = -100.0f;
          continue;
        }

        score[j] = dot(gravitydir, neighbouroffset[j]);
        if (score[j] > highscore) {
          highscore = score[j];
          bestoffset = j;
        }
      }
      if (bestoffset == -1)
        continue;

      Pixel currpixel = pixel_get(framebuffer, currpos);

      currpos = currpos + neighbouroffset[bestoffset];
      Pixel nextpixel = pixel_get(framebuffer, currpos);
      pixel_move(currpixel, nextpixel);
    }
    Serial.printf("[COMPUTE]: %lld us", esp_timer_get_time() - computetime);

    if (leftbutton == LOW) {
      activepixelcount = 0;
      ssd_clear();
    }

    ssd_display();
    Serial.printf("[LOOP]: %lld us\n", esp_timer_get_time() - currenttime);
  }
}
