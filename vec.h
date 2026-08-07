#pragma once
#ifndef VEC
#define VEC

struct Vec2 {
  float x, y;

  Vec2() : x(0.0f), y(0.0f) {}
  Vec2(float x, float y) : x(x), y(y) {}

  Vec2 operator+(const Vec2 &other) const {
    return Vec2(x + other.x, y + other.y);
  }

  Vec2 operator-(const Vec2 &other) const {
    return Vec2(x - other.x, y - other.y);
  }

  Vec2 operator*(float scalar) const { return Vec2(x * scalar, y * scalar); }

  float dot(const Vec2 &other) const { return x * other.x + y * other.y; }

  float cross2D(const Vec2 &other) const { return x * other.y - y * other.x; }
};

struct Vec3 {
  float x, y, z;

  Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
  Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

  Vec3 operator+(const Vec3 &other) const {
    return Vec3(x + other.x, y + other.y, z + other.z);
  }

  Vec3 operator-(const Vec3 &other) const {
    return Vec3(x - other.x, y - other.y, z - other.z);
  }

  Vec3 operator*(float scalar) const {
    return Vec3(x * scalar, y * scalar, z * scalar);
  }

  float dot(const Vec3 &other) const {
    return x * other.x + y * other.y + z * other.z;
  }

  Vec3 cross(const Vec3 &other) const {
    return Vec3(y * other.z - z * other.y, z * other.x - x * other.z,
                x * other.y - y * other.x);
  }
};

struct Vec4 {
  float x, y, z, w;

  Vec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
  Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

  Vec4 operator+(const Vec4 &other) const {
    return Vec4(x + other.x, y + other.y, z + other.z, w + other.w);
  }

  Vec4 operator-(const Vec4 &other) const {
    return Vec4(x - other.x, y - other.y, z - other.z, w - other.w);
  }

  Vec4 operator*(float scalar) const {
    return Vec4(x * scalar, y * scalar, z * scalar, w * scalar);
  }

  float dot(const Vec4 &other) const {
    return x * other.x + y * other.y + z * other.z + w * other.w;
  }
};

struct VecScreen {
  float x, y, z, inv_w;
  float u, v;
  float nx, ny, nz;
};

#endif