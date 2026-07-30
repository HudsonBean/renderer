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
    m.mat[0][3] = transform.x;
    m.mat[1][3] = transform.y;
    m.mat[2][3] = transform.z;
    return m;
  }

  static Mat4 perspective(float fov_y, float aspect, float near, float far) {
    Mat4 m{}; // Zero'd matrix
    const float f = 1 / (tan(fov_y / 2));
    m.mat[0][0] = f / aspect;
    m.mat[1][1] = f;
    m.mat[2][2] = -(far + near) / (far - near);        // A
    m.mat[2][3] = -(2.0f * far * near) / (far - near); // B
    m.mat[3][2] = -1.0f;                               // Stages w divide
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
  return Vec4(
      v.x * m.mat[0][0] + v.y * m.mat[1][0] + v.z * m.mat[2][0] + v.w * m.mat[3][0],
      v.x * m.mat[0][1] + v.y * m.mat[1][1] + v.z * m.mat[2][1] + v.w * m.mat[3][1],
      v.x * m.mat[0][2] + v.y * m.mat[1][2] + v.z * m.mat[2][2] + v.w * m.mat[3][2],
      v.x * m.mat[0][3] + v.y * m.mat[1][3] + v.z * m.mat[2][3] + v.w * m.mat[3][3]);
}

#endif