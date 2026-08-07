#include "geometry.h"
#include "mat.h"
#include "obj_loader.h"
#include "vec.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <vector>

const int WIDTH = 800;
const int HEIGHT = 600;

// Create framebuffer
std::vector<uint32_t> framebuffer(WIDTH *HEIGHT);

// Create z-buffer
std::vector<float> depth_buffer(WIDTH *HEIGHT);

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

// just for checkerboard later replace with sample_texture
uint32_t sample_checker(float u, float v) {
  int cell = ((int)std::floor(u) + (int)std::floor(v)) & 1;
  return cell ? 0xFFEEEEEE : 0xFF334455; // light / dark
}

void fill_triangle_3d(VecScreen a, VecScreen b, VecScreen c) {
  const Vec2 pa(a.x, a.y), pb(b.x, b.y), pc(c.x, c.y);

  const float area = orient2D(pa, pb, pc);
  if (area == 0.0f) // Avoid div by 0
    return;
  const float inv_area = 1.0f / area; // Optimization for division

  const int min_x = std::max(0, (int)std::floor(std::min({a.x, b.x, c.x})));
  const int max_x =
      std::min(WIDTH - 1, (int)std::ceil(std::max({a.x, b.x, c.x})));
  const int min_y = std::max(0, (int)std::floor(std::min({a.y, b.y, c.y})));
  const int max_y =
      std::min(HEIGHT - 1, (int)std::ceil(std::max({a.y, b.y, c.y})));

  for (int y = min_y; y <= max_y; y++) {
    for (int x = min_x; x <= max_x; x++) {
      const Vec2 p{x + 0.5f, y + 0.5f}; // sample at pixel center

      const float w0 = orient2D(pa, pb, p);
      const float w1 = orient2D(pb, pc, p);
      const float w2 = orient2D(pc, pa, p);

      // Barycentric weights
      const float alpha = w1 * inv_area;
      const float beta = w2 * inv_area;
      const float gamma = w0 * inv_area;

      // Dividing by the *signed* area normalizes winding, so one sign test
      // suffices
      if (alpha < 0.0f || beta < 0.0f || gamma < 0.0f)
        continue;

      // Linear interpolation of depth values
      const float depth = alpha * a.z + beta * b.z + gamma * c.z;

      const int idx = y * WIDTH + x;
      if (depth < depth_buffer[idx]) {
        depth_buffer[idx] = depth;

        // Perspective correct interpolation
        const float inv_w =
            alpha * a.inv_w + beta * b.inv_w + gamma * c.inv_w; // inv_w = rhw

        // UV attribute
        const float u = (alpha * a.u * a.inv_w + beta * b.u * b.inv_w +
                         gamma * c.u * c.inv_w) /
                        inv_w;
        const float v = (alpha * a.v * a.inv_w + beta * b.v * b.inv_w +
                         gamma * c.v * c.inv_w) /
                        inv_w;

        // Normal attribute
        float nx = (alpha * a.nx * a.inv_w + beta * b.nx * b.inv_w +
                    gamma * c.nx * c.inv_w) /
                   inv_w;
        float ny = (alpha * a.ny * a.inv_w + beta * b.ny * b.inv_w +
                    gamma * c.ny * c.inv_w) /
                   inv_w;
        float nz = (alpha * a.nz * a.inv_w + beta * b.nz * b.inv_w +
                    gamma * c.nz * c.inv_w) /
                   inv_w;
        // Renormalize normals now
        float len = std::sqrt(nx * nx + ny * ny + nz * nz);
        nx /= len;
        ny /= len;
        nz /= len;

        uint8_t R = (uint8_t)((nx * 0.5f + 0.5f) * 255);
        uint8_t G = (uint8_t)((ny * 0.5f + 0.5f) * 255);
        uint8_t B = (uint8_t)((nz * 0.5f + 0.5f) * 255);
        draw_pixel(x, y, 0xFF000000 | (R << 16) | (G << 8) | B);
      }
    }
  }
}

float to_rad(int deg) { return deg * (std::numbers::pi / 180); }

VecScreen project(Vec4 clip, float u, float v, Vec3 n) {
  const float rhw = 1.0f / clip.w;
  return VecScreen{(clip.x * rhw + 1.0f) * 0.5f * WIDTH,
                   (1.0f - clip.y * rhw) * 0.5f * HEIGHT,
                   clip.z * rhw,
                   rhw,
                   u,
                   v,
                   n.x,
                   n.y,
                   n.z};
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

  // ––––––––––––––––––––Load Model––––––––––––––––––––

  Mesh mesh;
  if (!load_obj("../scene1.obj", mesh))
    return 1;

  // ––––––––––––––––––––Setup Proj––––––––––––––––––––

  Vec3 camera_pos{0, 0, 0};
  float fov = to_rad(60);
  float aspect = float(WIDTH) / float(HEIGHT);
  Mat4 proj = Mat4::perspective(fov, aspect, .1f, 100.0f);

  // ––––––––––––––––––––Start Loop––––––––––––––––––––
  bool running = true;
  SDL_Event event;
  while (running) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT)
        running = false;
    }

    // Clear framebuffer and depth buffer
    std::fill(framebuffer.begin(), framebuffer.end(), 0xFF000000);
    std::fill(depth_buffer.begin(), depth_buffer.end(),
              std::numeric_limits<float>::infinity());

    // ––––––––––––––Perspective Projection––––––––––––––
    // Camera inverse transform
    Mat4 view = Mat4::translate({-camera_pos.x, -camera_pos.y, -camera_pos.z});
    Mat4 model = Mat4::identity();

    Mat4 mvp = model * view * proj; // Row-vector convention: v * M

    for (const Triangle &t : mesh.triangles) {
      Vec3 pos[] = {mesh.positions[t.p[0]], mesh.positions[t.p[1]],
                    mesh.positions[t.p[2]]};
      Vec2 uv[] = {mesh.texcoords[t.t[0]], mesh.texcoords[t.t[1]],
                   mesh.texcoords[t.t[2]]};
      Vec3 normals[] = {mesh.normals[t.n[0]], mesh.normals[t.n[1]],
                        mesh.normals[t.n[2]]};

      // Move triangle's three vertices to clip space
      Vec4 clip[3];
      bool behind_near = false;
      for (int i = 0; i < 3; i++) {
        clip[i] = Vec4(pos[i].x, pos[i].y, pos[i].z, 1.0f) * mvp;
        if (clip[i].w <= 1e-5f)
          behind_near = true;
      }
      if (behind_near) // Cut vertices that are behind camera
        continue;

      // Move each normal into world space
      Vec3 world_normals[3];
      for (int i = 0; i < 3; i++) {
        Vec4 n4 = Vec4(normals[i].x, normals[i].y, normals[i].z, 0.0f) * model;
        world_normals[i] =
            Vec3(n4.x, n4.y,
                 n4.z); // Don't normalize just yet, do it post interpolation.
      }

      VecScreen screen[3];
      for (int i = 0; i < 3; i++)
        screen[i] = project(clip[i], uv[i].x, uv[i].y, world_normals[i]);

      fill_triangle_3d(screen[0], screen[1], screen[2]);
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
