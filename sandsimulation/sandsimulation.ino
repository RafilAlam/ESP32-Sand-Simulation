#include "esp_random.h"
#include <types.h>
#include <ssd1306.h>
#include <adxl345.h>

#define BUTTON_LEFT 3
#define BUTTON_RIGHT 10

#define GRAVITY_MAGNIFIER 0.04f

float magnitude(const FVec2& vec) {
  return sqrtf(vec.x*vec.x + vec.y*vec.y);
}

FVec2 normal(const FVec2& vec) {
  float mag = magnitude(vec);
  //Serial.printf("%f, %f", mag, vec.x);
  return FVec2{vec.x / mag, vec.y / mag};
}

float dot(const FVec2& a, const FVec2& b) {
  return a.x*b.x + a.y*b.y;
}

int16_t fast_round(float x) {
  return static_cast<int16_t>(std::lrint(x));
}

IVec2 fast_round(const FVec2& vec) {
  return {fast_round(vec.x), fast_round(vec.y)};
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

float score[8];
Cell activepixels[8192];
uint16_t activepixelcount = 0;

int64_t renderintervalMicros = 16667; // 60 Hz
int64_t physicsintervalMicros = 8333; // 120 Hz
int64_t previousrendertime = 0;
int64_t previousphysicstime = 0;
void loop() {
  int64_t currenttime = esp_timer_get_time();

  if (currenttime - previousphysicstime > physicsintervalMicros) {
    previousphysicstime = currenttime;
    uint8_t leftbutton = digitalRead(BUTTON_LEFT);
    uint8_t rightbutton = digitalRead(BUTTON_RIGHT);

    adxl_readaccel(accel_reading);
    FVec2 gravity{accel_reading.x, accel_reading.y};

    if (rightbutton == LOW) {
      IVec2 spawnpos;
      spawnpos = {64, gravity.y > 0 ? 0 : 63};

      activepixels[activepixelcount++].position = spawnpos;
      pixel_set(pixel_get(framebuffer, spawnpos));
    }

    gravity *= GRAVITY_MAGNIFIER;

    int64_t computetime = esp_timer_get_time();
    for (int i=0; i<activepixelcount; ++i) {
      Cell& curr = activepixels[i];
      curr.velocity += gravity;
      curr.velocity *= 0.99f;
      curr.velocity.x = max(min(curr.velocity.x, 1.0f), -1.0f);
      curr.velocity.y = max(min(curr.velocity.y, 1.0f), -1.0f);

      IVec2 roundedvelocity = fast_round(curr.velocity);
      IVec2 movepos = curr.position + roundedvelocity;
      if (!pixel_exists(movepos)) {
        curr.velocity *= 0.2f;
        continue;
      }
      else if (pixel_read(pixel_get(framebuffer, movepos))) {
        if (movepos.x != curr.position.x && movepos.y != curr.position.y) { // Diagonal
          IVec2 slide_offsets[2] = {IVec2{roundedvelocity.x, 0}, IVec2{0, roundedvelocity.y}};
          if (abs(curr.velocity.x) <= abs(curr.velocity.y))
            std::swap(slide_offsets[0], slide_offsets[1]);
          bool moved = false;
          for (uint8_t i=0; i<2; ++i) {
            IVec2 slidepos = curr.position + slide_offsets[i];
            if (pixel_exists(slidepos) && !pixel_read(pixel_get(framebuffer, slidepos))) {
              movepos = slidepos;
              moved = true;
              break;
            }
          }
          if (!moved)
            continue;
        }
        else if (movepos.x != curr.position.x) { // Moving in X
          int16_t slide_offsets[2] = {1, -1};
          if (curr.velocity.y < 0)
            std::swap(slide_offsets[0], slide_offsets[1]);
          bool moved = false;
          for (uint8_t i=0; i<2; ++i) {
            IVec2 slidepos = curr.position + IVec2{roundedvelocity.x, slide_offsets[i]};
            if (pixel_exists(slidepos) && !pixel_read(pixel_get(framebuffer, slidepos))) {
              movepos = slidepos;
              moved = true;
              break;
            }
          }
          if (!moved) // No movement
            continue;
        }
        else if (movepos.y != curr.position.y) { // Moving in Y
          int16_t slide_offsets[2] = {1, -1};
          if (curr.velocity.x < 0)
            std::swap(slide_offsets[0], slide_offsets[1]);
          bool moved = false;
          for (uint8_t i=0; i<2; ++i) {
            IVec2 slidepos = curr.position + IVec2{slide_offsets[i], roundedvelocity.y};
            if (pixel_exists(slidepos) && !pixel_read(pixel_get(framebuffer, slidepos))) {
              movepos = slidepos;
              moved = true;
              break;
            }
          }
          if (!moved) // No movement
            continue;
        }
        else { // Stationary
          continue;
        }
      }

      Pixel currpixel = pixel_get(framebuffer, curr.position);

      curr.position = movepos;
      Pixel nextpixel = pixel_get(framebuffer, curr.position);
      pixel_move(currpixel, nextpixel);
    }
    //Serial.printf("[COMPUTE]: %lld us", esp_timer_get_time() - computetime);

    if (leftbutton == LOW) {
      activepixelcount = 0;
      ssd_clear();
    }
    //Serial.printf("[LOOP]: %lld us\n", esp_timer_get_time() - currenttime);
  }

  if (currenttime - previousrendertime >= renderintervalMicros) {
    previousrendertime = currenttime;
    ssd_display();
  }
}
