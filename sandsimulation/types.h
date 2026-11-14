#pragma once

struct FVec2 {
  float x;
  float y;
  FVec2 operator+(const FVec2& rhs) const {
    return FVec2(x+rhs.x,  y+rhs.y);
  }
  FVec2 operator-(const FVec2& rhs) const {
    return FVec2(x-rhs.x,  y-rhs.y);
  }
};

struct IVec2 {
  int16_t x;
  int16_t y;

  IVec2(int16_t a, int16_t b) {
    x = a;
    y = b;
  }
  IVec2(const FVec2& vec) {
    x = static_cast<int16_t>(vec.x);
    y = static_cast<int16_t>(vec.y);
  }
};

struct Pixel {
  uint8_t* byte;
  uint8_t mask;
};