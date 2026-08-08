#ifndef MAT
#define MAT

#include "vec.h"
#include <cmath>

struct Mat {};

struct Mat4 {
  float mat[4][4];

  static Mat4 identity() {
    Mat4 result{
        {
            {1, 0, 0, 0},
            {0, 1, 0, 0},
            {0, 0, 1, 0},
            {0, 0, 0, 1},
        },
    };
    return result;
  }

  static Mat4 translate(Vec3 transform) {
    Mat4 m = identity();
    m.mat[3][0] = transform.x;
    m.mat[3][1] = transform.y;
    m.mat[3][2] = transform.z;
    return m;
  }

  static Mat4 rotateX(float a) { // X fixed, Y and Z rotate
    float c = std::cos(a), s = std::sin(a);
    Mat4 m = identity();
    m.mat[1][1] = c;
    m.mat[1][2] = s;
    m.mat[2][1] = -s;
    m.mat[2][2] = c;
    return m;
  }
  static Mat4 rotateY(float a) { // Y fixed, X and Z rotate
    float c = std::cos(a), s = std::sin(a);
    Mat4 m = identity();
    m.mat[0][0] = c;
    m.mat[0][2] = -s;
    m.mat[2][0] = s;
    m.mat[2][2] = c;
    return m;
  }
  static Mat4 rotateZ(float a) {
    float c = std::cos(a), s = std::sin(a);
    Mat4 m = identity();
    m.mat[0][0] = c;
    m.mat[0][1] = s;
    m.mat[1][0] = -s;
    m.mat[1][1] = c;

    return m;
  }

  static Mat4 perspective(float fov_y, float aspect, float near, float far) {
    Mat4 m{}; // Zero'd matrix
    const float f = 1 / (tan(fov_y / 2));
    m.mat[0][0] = f / aspect;
    m.mat[1][1] = f;
    m.mat[2][2] = -far / (far - near);          // A utilizes [0, 1] range
    m.mat[2][3] = -1.0f;                        // w =-z
    m.mat[3][2] = -(far * near) / (far - near); // B utilizes [0, 1] range
    return m;
  }

  // Vec4 operator*(const Vec4 &v) const {
  //   return Vec4(
  //       v.x * mat[0][0] + v.y * mat[0][1] + v.z * mat[0][2] + v.w *
  //       mat[0][3], v.x * mat[1][0] + v.y * mat[1][1] + v.z * mat[1][2] + v.w
  //       * mat[1][3], v.x * mat[2][0] + v.y * mat[2][1] + v.z * mat[2][2] +
  //       v.w * mat[2][3], v.x * mat[3][0] + v.y * mat[3][1] + v.z * mat[3][2]
  //       + v.w * mat[3][3]);
  // }

  Mat4 operator*(const Mat4 &other) const {
    Mat4 result{};
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        result.mat[i][j] =
            mat[i][0] * other.mat[0][j] + mat[i][1] * other.mat[1][j] +
            mat[i][2] * other.mat[2][j] + mat[i][3] * other.mat[3][j];
      }
    }
    return result;
  }
};

inline Vec4 operator*(const Vec4 &v, const Mat4 &m) {
  return Vec4(v.x * m.mat[0][0] + v.y * m.mat[1][0] + v.z * m.mat[2][0] +
                  v.w * m.mat[3][0],
              v.x * m.mat[0][1] + v.y * m.mat[1][1] + v.z * m.mat[2][1] +
                  v.w * m.mat[3][1],
              v.x * m.mat[0][2] + v.y * m.mat[1][2] + v.z * m.mat[2][2] +
                  v.w * m.mat[3][2],
              v.x * m.mat[0][3] + v.y * m.mat[1][3] + v.z * m.mat[2][3] +
                  v.w * m.mat[3][3]);
}

#endif