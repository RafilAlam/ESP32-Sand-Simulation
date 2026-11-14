#pragma once

struct IVec2 {
  int16_t x;
  int16_t y;

  IVec2() = default;
  IVec2(int16_t a, int16_t b) {
    x = a;
    y = b;
  }
  IVec2 operator+(const IVec2& rhs) const {
    return IVec2(x+rhs.x,  y+rhs.y);
  }
  IVec2 operator-(const IVec2& rhs) const {
    return IVec2(x-rhs.x,  y-rhs.y);
  }
};

struct FVec2 {
  float x;
  float y;
  FVec2(float a, float b) {
    x = a;
    y = b;
  }
  FVec2(int16_t a, int16_t b) {
    x = a;
    y = b;
  }
  FVec2(const IVec2& vec) {
    x = static_cast<float>(vec.x);
    y = static_cast<float>(vec.y);
  }
  FVec2 operator+(const FVec2& rhs) const {
    return FVec2(x+rhs.x,  y+rhs.y);
  }
  FVec2 operator-(const FVec2& rhs) const {
    return FVec2(x-rhs.x,  y-rhs.y);
  }
};

struct Pixel {
  uint8_t* byte;
  uint8_t mask;
};