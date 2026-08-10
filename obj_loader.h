#pragma once
// obj_loader.h — minimal, fast OBJ reader for the software renderer.
//
// Handles the subset this project emits: v, vn, f (triangulated, v//vn form).
// Also tolerates v/vt/vn and v/vt faces, and quads (fan-triangulated), so it
// won't choke on OBJs from Blender. Ignores comments, groups, materials.
//
// Usage:
//   Mesh mesh;
//   if (!load_obj("scene.obj", mesh)) { /* handle error */ }
//   for (const Triangle& t : mesh.triangles) {
//       Vec3 p0 = mesh.positions[t.p[0]];   // ... push through your pipeline
//   }

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "vec.h" // <-- your Vec3. Adjust include to wherever Vec3 lives.

struct Triangle {
  int p[3]; // indices into Mesh::positions
  int t[3]; // indices into Mesh::texcoords
  int n[3]; // indices into Mesh::normals  (-1 if the face had no normals)
};

struct Mesh {
  std::vector<Vec3> positions;
  std::vector<Vec2> texcoords;
  std::vector<Vec3> normals;
  std::vector<Triangle> triangles;

  void clear() {
    positions.clear();
    texcoords.clear();
    normals.clear();
    triangles.clear();
  }
};

namespace objdetail {

inline bool parse_ref(const char *s, const char *end, int &pos_out,
                      int &tex_out, int &nrm_out) {
  pos_out = -1;
  tex_out = -1;
  nrm_out = -1;

  char *cur = const_cast<char *>(s);
  long p = std::strtol(cur, &cur, 10);
  if (cur == s)
    return false;                    // no number at all
  pos_out = static_cast<int>(p) - 1; // OBJ is 1-based

  if (cur >= end || *cur != '/')
    return true; // "12"
  ++cur;         // skip first '/'

  if (cur < end && *cur == '/') { // "12//n" — no texcoord
    ++cur;
    char *after = cur;
    long n = std::strtol(cur, &after, 10);
    if (after != cur)
      nrm_out = static_cast<int>(n) - 1;
    return true;
  }

  // "12/t..." — capture the texcoord instead of skipping it
  {
    char *after = cur;
    long t = std::strtol(cur, &after, 10);
    if (after != cur)
      tex_out = static_cast<int>(t) - 1;
    cur = after;
  }

  if (cur < end && *cur == '/') { // "12/t/n"
    ++cur;
    char *after = cur;
    long n = std::strtol(cur, &after, 10);
    if (after != cur)
      nrm_out = static_cast<int>(n) - 1;
  }
  return true;
}

// Resolve possibly-negative OBJ index (relative-to-end) against a known count.
inline int resolve(int idx, size_t count) {
  if (idx >= 0)
    return idx; // already 0-based positive
  // negative-in-file was stored as (value-1); recover a relative offset
  return static_cast<int>(count) + (idx + 1);
}

} // namespace objdetail

inline bool load_obj(const std::string &path, Mesh &mesh) {
  std::ifstream file(path);
  if (!file) {
    std::fprintf(stderr, "load_obj: cannot open '%s'\n", path.c_str());
    return false;
  }

  mesh.clear();
  // Reserve to avoid repeated reallocation on large meshes; grows if needed.
  mesh.positions.reserve(1024);
  mesh.texcoords.reserve(1024);
  mesh.normals.reserve(1024);
  mesh.triangles.reserve(2048);

  std::string line;
  // Scratch buffers reused per face so we don't allocate in the hot loop.
  std::vector<int> face_p;
  std::vector<int> face_t;
  std::vector<int> face_n;
  face_p.reserve(8);
  face_t.reserve(8);
  face_n.reserve(8);

  size_t line_no = 0;
  while (std::getline(file, line)) {
    ++line_no;
    const char *c = line.c_str();
    const char *end = c + line.size();

    // skip leading whitespace
    while (c < end && (*c == ' ' || *c == '\t'))
      ++c;
    if (c >= end || *c == '#')
      continue; // blank or comment

    // ---- vertex position ----
    if (c[0] == 'v' && (c[1] == ' ' || c[1] == '\t')) {
      Vec3 p{};
      if (std::sscanf(c + 1, "%f %f %f", &p.x, &p.y, &p.z) == 3)
        mesh.positions.push_back(p);
      continue;
    }

    // ---- vertex normal ----
    if (c[0] == 'v' && c[1] == 'n') {
      Vec3 n{};
      if (std::sscanf(c + 2, "%f %f %f", &n.x, &n.y, &n.z) == 3)
        mesh.normals.push_back(n);
      continue;
    }

    // ---- texture coordinate ----
    if (c[0] == 'v' && c[1] == 't') {
      Vec2 uv{};
      if (std::sscanf(c + 2, "%f %f", &uv.x, &uv.y) == 2)
        mesh.texcoords.push_back(uv);
      continue;
    }

    // ---- face ----
    if (c[0] == 'f' && (c[1] == ' ' || c[1] == '\t')) {
      face_p.clear();
      face_t.clear();
      face_n.clear();

      const char *cur = c + 1;
      while (cur < end) {
        while (cur < end && (*cur == ' ' || *cur == '\t'))
          ++cur;
        if (cur >= end)
          break;

        const char *tok_end = cur;
        while (tok_end < end && *tok_end != ' ' && *tok_end != '\t')
          ++tok_end;

        int pi, ti, ni; // added ti
        if (objdetail::parse_ref(cur, tok_end, pi, ti, ni)) {
          face_p.push_back(objdetail::resolve(pi, mesh.positions.size()));
          face_t.push_back(
              ti < 0 ? -1 : objdetail::resolve(ti, mesh.texcoords.size()));
          face_n.push_back(
              ni < 0 ? -1 : objdetail::resolve(ni, mesh.normals.size()));
        }
        cur = tok_end;
      }

      // Fan-triangulate: (0,1,2), (0,2,3), ... — works for tris and convex
      // polys.
      if (face_p.size() >= 3) {
        for (size_t i = 1; i + 1 < face_p.size(); ++i) {
          Triangle t;
          t.p[0] = face_p[0];
          t.t[0] = face_t[0];
          t.n[0] = face_n[0];
          t.p[1] = face_p[i];
          t.t[1] = face_t[i];
          t.n[1] = face_n[i];
          t.p[2] = face_p[i + 1];
          t.t[2] = face_t[i + 1];
          t.n[2] = face_n[i + 1];
          mesh.triangles.push_back(t);
        }
      } else {
        std::fprintf(stderr, "load_obj: %s:%zu face with <3 verts, skipped\n",
                     path.c_str(), line_no);
      }
      continue;
    }

    // everything else (g, o, usemtl, mtllib, s, vt-only, ...) is ignored
  }

  if (mesh.positions.empty() || mesh.triangles.empty()) {
    std::fprintf(stderr, "load_obj: '%s' produced no geometry\n", path.c_str());
    return false;
  }
  return true;
}