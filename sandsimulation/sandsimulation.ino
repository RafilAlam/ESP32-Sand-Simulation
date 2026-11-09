#include "soc/gpio_struct.h"
#include "esp_random.h"
#include <SPI.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#define SCREEN_HEIGHT 64
#define SCREEN_WIDTH 128

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

void init_display() {
  Wire.begin();
  Wire.setClock(1000000);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for (;;);
  }

  display.clearDisplay();
  display.display();
}

void flush_display() {
  display.display();
}

bool pixel_exists(int16_t x, int16_t y) {
  return (x < SCREEN_WIDTH && x >= 0) && (y < SCREEN_HEIGHT && y >= 0);
}

class Pixel {
  uint8_t* byte;
  uint8_t mask;
public:  
  bool bind(uint8_t* buffer, int16_t x, int16_t y) {
    if (!pixel_exists(x, y))
      return false;

    byte = &buffer[SCREEN_WIDTH * (y / 8) + x];
    mask = 1 << (y % 8);
    return true;
  }
  bool read() {
    return *byte & mask;
  }
  void set() {
    *byte |= mask;
  }
  void clear() {
    *byte &= ~mask;
  }
};

class Simulator {
  inline void move(Pixel& p, Pixel& nextp) {
    p.clear();
    nextp.set();
  }
public:
  uint8_t simulation_buffer[SCREEN_WIDTH * SCREEN_HEIGHT / 8] = {};
  void simulate_sand(uint8_t* display_buffer) {
    for (int16_t x=SCREEN_WIDTH-1; x>=0; --x) {
      for (int16_t y=SCREEN_HEIGHT-1; y>=0; --y) {
        Pixel dis_p;
        Pixel sim_p;
        Pixel dis_nextp;
        Pixel sim_nextp;
        sim_p.bind(simulation_buffer, x, y);
        if (!dis_p.bind(display_buffer, x, y) || !dis_p.read() || !dis_nextp.bind(display_buffer, x, y+1))
          continue;

        sim_nextp.bind(simulation_buffer, x, y+1);
        if (!dis_nextp.read() && !sim_nextp.read()) {
          move(sim_p, sim_nextp);
        }
        else if (dis_nextp.read() && sim_nextp.read()) {
          bool left = esp_random() & 1;
          if (left) {
            if (dis_nextp.bind(display_buffer, x-1, y+1) && !dis_nextp.read() && sim_nextp.bind(simulation_buffer, x-1, y+1) && !sim_nextp.read()) {
              move(sim_p, sim_nextp);
            }
            else if (dis_nextp.bind(display_buffer, x+1, y+1) && !dis_nextp.read() && sim_nextp.bind(simulation_buffer, x+1, y+1) && !sim_nextp.read()) {
              move(sim_p, sim_nextp);
            }
          }
          else {
            if (dis_nextp.bind(display_buffer, x+1, y+1) && !dis_nextp.read() && sim_nextp.bind(simulation_buffer, x+1, y+1) && !sim_nextp.read()) {
              move(sim_p, sim_nextp);
            }
            else if (dis_nextp.bind(display_buffer, x-1, y+1) && !dis_nextp.read() && sim_nextp.bind(simulation_buffer, x-1, y+1) && !sim_nextp.read() ) {
              move(sim_p, sim_nextp);
            }
          }
        }
      }
    }
    memcpy(display_buffer, simulation_buffer, sizeof(simulation_buffer));
  }
};

Simulator sim;
Pixel sandgenerator;

void setup() {
  init_display();
}

void loop() {
  uint8_t* display_buffer = display.getBuffer();

  if (millis() % 2 == 0) {
    sandgenerator.bind(display_buffer, 64, 0);
    sandgenerator.set();
  }

  sim.simulate_sand(display_buffer);

  flush_display();
}
