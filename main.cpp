#include "geometry.h"
#include "mat.h"
#include "obj_loader.h"
#include "vec.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdint>
#include <numbers>
#include <vector>

const int WIDTH = 800;
const int HEIGHT = 600;

// Create framebuffer
std::vector<uint32_t> framebuffer(WIDTH *HEIGHT);

void draw_pixel(int x, int y, uint32_t color) {
  if (x >= 0 && y >= 0 && x < WIDTH && y < HEIGHT) {
    framebuffer[y * WIDTH + x] = color;
  }
}

void draw_pixel(Vec2 a, uint32_t color) {
  draw_pixel(static_cast<int>(a.x), static_cast<int>(a.y), color);
}

// Bresenham's line drawing algorithm
void draw_line_horizontal(int x0, int y0, int x1, int y1, uint32_t color) {
  if (x0 > x1) {
    std::swap(x0, x1);
    std::swap(y0, y1);
  }

  int dx = x1 - x0;
  int dy = y1 - y0;

  int dir = (dy < 0) ? -1 : 1;
  dy *= dir;

  if (dx != 0) {
    int y = y0;
    int drift = 2 * dy - dx;

    for (int i = 0; i < (dx + 1); i++) {
      draw_pixel(x0 + i, y, color);

      if (drift >= 0) {
        y += dir;
        drift = drift - 2 * dx;
      }

      drift = drift + 2 * dy;
    }
  }
}

void draw_line_vertical(int x0, int y0, int x1, int y1, uint32_t color) {
  if (y0 > y1) {
    std::swap(x0, x1);
    std::swap(y0, y1);
  }

  int dx = x1 - x0;
  int dy = y1 - y0;

  int dir = (dx < 0) ? -1 : 1;
  dx *= dir;

  if (dy != 0) {
    int x = x0;
    int drift = 2 * dx - dy;

    for (int i = 0; i < (dy + 1); i++) {
      draw_pixel(x, y0 + i, color);

      if (drift >= 0) {
        x += dir;
        drift = drift - 2 * dy;
      }

      drift = drift + 2 * dx;
    }
  }
}

void draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
  if (std::abs(x1 - x0) > std::abs(y1 - y0)) {
    draw_line_horizontal(x0, y0, x1, y1, color);
  } else {
    draw_line_vertical(x0, y0, x1, y1, color);
  }
  Vec2 a{static_cast<float>(x0), static_cast<float>(y0)};
  Vec2 b{static_cast<float>(x1), static_cast<float>(y1)};
}

void draw_line(Vec2 a, Vec2 b, uint32_t color) {
  draw_line(static_cast<int>(a.x), static_cast<int>(a.y), static_cast<int>(b.x),
            static_cast<int>(b.y), color);
}

void draw_triangle(Vec2 a, Vec2 b, Vec2 c, uint32_t color) {
  draw_line(a, b, color);
  draw_line(b, c, color);
  draw_line(c, a, color);
}

void fill_triangle(Vec2 a, Vec2 b, Vec2 c, uint32_t color) {
  const int min_x = static_cast<int>(std::floor(std::min({a.x, b.x, c.x})));
  const int max_x = static_cast<int>(std::ceil(std::max({a.x, b.x, c.x})));
  const int min_y = static_cast<int>(std::floor(std::min({a.y, b.y, c.y})));
  const int max_y = static_cast<int>(std::ceil(std::max({a.y, b.y, c.y})));

  for (int y = min_y; y <= max_y; y++) {
    for (int x = min_x; x <= max_x; x++) {
      Vec2 p{static_cast<float>(x), static_cast<float>(y)};

      const float triangle_area = orient2D(a, b, c);
      const float w0 = orient2D(a, b, p);
      const float w1 = orient2D(b, c, p);
      const float w2 = orient2D(c, a, p);

      // Barycentric weights
      const float alpha = w1 / triangle_area;
      const float beta = w2 / triangle_area;
      const float gamma = w0 / triangle_area;

      const bool has_neg = (w0 < 0) || (w1 < 0) || (w2 < 0);
      const bool has_pos = (w0 > 0) || (w1 > 0) || (w2 > 0);
      if (!(has_neg && has_pos)) {
        draw_pixel(x, y, color);
      }
    }
  }
}

float to_rad(int deg) { return deg * (std::numbers::pi / 180); }

Vec2 viewport(Vec3 ndc) {
  float x = (ndc.x + 1.0f) * 0.5f * WIDTH;
  float y = (1.0f - ndc.y) * 0.5f *
            HEIGHT; // 1-y because screen pixels count downward
  return Vec2{x, y};
}

int main(int argc, char *argv[]) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("SDL_Init failed: %s", SDL_GetError());
    return 1;
  }

  SDL_Window *window = SDL_CreateWindow("Rasterizer", WIDTH, HEIGHT, 0);
  if (!window) {
    SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
  if (!renderer) {
    SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  SDL_Texture *texture =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                        SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
  if (!texture) {
    SDL_Log("SDL_CreateTexture failed: %s", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  bool running = true;
  SDL_Event event;
  while (running) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT)
        running = false;
    }

    // Clear framebuffer
    for (auto &pixel : framebuffer) {
      pixel = 0xFF000000;
    }

    // ––––––––––––––––––––Load Model––––––––––––––––––––

    Mesh mesh;
    if (!load_obj("../model.obj", mesh))
      return 1;

    // ––––––––––––––Perspective Projection––––––––––––––
    // Maybe move these somewhere else later on
    Vec3 camera_pos{0, 0, 0};
    float fov = to_rad(60);
    float aspect = 1920.0f / 1080.0f;

    // Camera inverse transform
    Mat4 view = Mat4::translate({-camera_pos.x, -camera_pos.y, -camera_pos.z});
    Mat4 proj = Mat4::perspective(fov, aspect, .1f, 100.0f);
    Mat4 model = Mat4::identity();

    Mat4 mvp = model * view * proj; // Row-vector convention: v * M

    for (const Triangle &t : mesh.triangles) {
      Vec3 v[] = {mesh.positions[t.p[0]], mesh.positions[t.p[1]],
                  mesh.positions[t.p[2]]};
      // push each through mvp → divide by w → viewport → fill_triangle
      // normals (for lighting later): mesh.normals[t.n[0]], etc. (t.n[i] == -1
      // if none)

      Vec2 viewport0;
      Vec2 viewport1;
      Vec2 viewport2;
      for (int i = 0; i < 3; i++) {
        Vec4 clip = Vec4(v[i].x, v[i].y, v[i].z, 1.0f) * mvp;
        float invW = 1.0f / clip.w; // Optimization for division
        Vec3 ndc{clip.x * invW, clip.y * invW, clip.z * invW};

        if (i == 0) {
          viewport0 = viewport(ndc);
        } else if (i == 1) {
          viewport1 = viewport(ndc);
        } else {
          viewport2 = viewport(ndc);
        }
      }

      fill_triangle(viewport0, viewport1, viewport2, 0xFFFFFFFF);
    }

    // ––––––––––––––––––––––––––––––––––––––––––––––––––

    SDL_UpdateTexture(texture, nullptr, framebuffer.data(),
                      WIDTH * sizeof(uint32_t));
    SDL_RenderTexture(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
  }

  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
