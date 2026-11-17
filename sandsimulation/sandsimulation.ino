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

IVec2 ceilf(const FVec2& vec) {
  return {ceilf(vec.x), ceilf(vec.y)};
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

    if (rightbutton == LOW && activepixelcount < 8192) {
      IVec2 spawnpos;
      spawnpos = {64, gravity.y > 0 ? 0 : 63};

      activepixels[activepixelcount].pixelcoord = spawnpos;
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

      Pixel currpixel = pixel_get(framebuffer, curr.pixelcoord);
      IVec2 roundedvelocity = fast_round(curr.velocity);
      FVec2 floatingmovepos = curr.position + curr.velocity;
      IVec2 movepos = fast_round(floatingmovepos);
      IVec2 nextcoord = curr.pixelcoord;
      
      int16_t dir1 = (rand() & 1) ? 1 : -1;
      int16_t dir2 = -dir1;

      if (nextcoord.x == movepos.x) {
        curr.position.x = floatingmovepos.x;
      } else {
        IVec2 test_coord = {movepos.x, nextcoord.y};
        if (pixel_exists(test_coord) && !pixel_read(pixel_get(framebuffer, test_coord))) {
          nextcoord.x = movepos.x;
          curr.position.x = floatingmovepos.x;
        } else {
          // Hit wall or other pixel
          bool tumbled = false;
          if (abs(gravity.x) > abs(gravity.y)) {
            int16_t offsets[2] = {dir1, dir2};
            for (int j = 0; j < 2; ++j) {
              IVec2 diag = {movepos.x, nextcoord.y + offsets[j]};
              if (pixel_exists(diag) && !pixel_read(pixel_get(framebuffer, diag))) {
                curr.position = FVec2{(float)diag.x, (float)diag.y};
                nextcoord = diag;
                tumbled = true;
                break;
              }
            }
          }

          if (!tumbled) {
            curr.position.x = static_cast<float>(curr.pixelcoord.x);
            curr.velocity.x = 0.0f;
          }
        }
      }

      if (curr.pixelcoord.y == movepos.y) {
        curr.position.y = floatingmovepos.y;
      } else {
        IVec2 test_coord = {nextcoord.x, movepos.y};
        if (pixel_exists(test_coord) && !pixel_read(pixel_get(framebuffer, test_coord))) {
          nextcoord.y = movepos.y;
          curr.position.y = floatingmovepos.y;
        } else {
          // Hit wall or other pixel
          bool tumbled = false;
          if (abs(gravity.y) > abs(gravity.x)) {
            int16_t offsets[2] = {dir1, dir2};
            for (int j = 0; j < 2; ++j) {
              IVec2 diag = {nextcoord.x + offsets[j], movepos.y};
              if (pixel_exists(diag) && !pixel_read(pixel_get(framebuffer, diag))) {
                curr.position = FVec2{(float)diag.x, (float)diag.y};
                nextcoord = diag;
                tumbled = true;
                break;
              }
            }
          }

          if (!tumbled) {
            curr.position.y = static_cast<float>(curr.pixelcoord.y);
            curr.velocity.y = 0.0f;
          }
        }
      }

      if (nextcoord != curr.pixelcoord) {
        curr.pixelcoord = nextcoord;
        Pixel nextpixel = pixel_get(framebuffer, curr.pixelcoord);
        pixel_move(currpixel, nextpixel);
      }
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
