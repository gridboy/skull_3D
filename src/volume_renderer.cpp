// Based on the educational volume rendering project by Michel Grave.
#include "volume_renderer.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

std::uint16_t data[IMAX][JMAX][KMAX];
static std::uint16_t rotated[IMAX][JMAX][KMAX];
int imax = 0, jmax = 0, kmax = 0, xmax = 384, ymax = 384, mode = OS,
    currentLayer = 0;
float spacing[3] = {1, 2, 1}, globalTransparency = 0.5f;
bool clipEnabled = false;
double windowCenter = 1100, windowWidth = 1800;
int fondR = 20, fondG = 24, fondB = 31;
std::uint8_t image[3 * XMAX * YMAX];

namespace {
int bound(int n, int size) { return std::max(0, std::min(n, size - 1)); }
double clamp(double v, double lo, double hi) {
  return std::max(lo, std::min(v, hi));
}
double voxel(int i, int j, int k) {
  return data[bound(i, imax)][bound(j, jmax)][bound(k, kmax)];
}
double sample(double x, double y, int k) {
  x = clamp(x, 0, imax - 1);
  y = clamp(y, 0, jmax - 1);
  int i = static_cast<int>(x), j = static_cast<int>(y);
  double u = x - i, v = y - j;
  return (1 - v) * ((1 - u) * voxel(i, j, k) + u * voxel(i + 1, j, k)) +
         v * ((1 - u) * voxel(i, j + 1, k) + u * voxel(i + 1, j + 1, k));
}
double shade(double x, double y, int k) {
  double gx = (sample(x + 1, y, k) - sample(x - 1, y, k)) / spacing[0];
  double gy = (sample(x, y + 1, k) - sample(x, y - 1, k)) / spacing[1];
  double gz = (sample(x, y, k + 1) - sample(x, y, k - 1)) / spacing[2];
  double norm = std::sqrt(gx * gx + gy * gy + gz * gz);
  return norm > 1e-9 ? 0.22 + 0.78 * std::abs(gz) / norm : 0.55;
}
void pixel(int x, int y, const double color[3]) {
  int p = 3 * (y * xmax + x);
  for (int c = 0; c < 3; ++c)
    image[p + c] =
        static_cast<std::uint8_t>(std::lround(clamp(color[c], 0, 255)));
}
} // namespace

void update_image_geometry() {
  // Use physical cell extents, not the number of slices as a square image.
  double w = std::max(1, imax - 1) * spacing[0],
         h = std::max(1, jmax - 1) * spacing[1];
  double scale = 384.0 / std::max(w, h);
  xmax = std::max(1, static_cast<int>(std::lround(w * scale)));
  ymax = std::max(1, static_cast<int>(std::lround(h * scale)));
}
void move_layer(int delta) { currentLayer = bound(currentLayer + delta, kmax); }

int charger_volume(const char *directory) {
  imax = 256;
  jmax = 113;
  kmax = 256;
  spacing[0] = 1;
  spacing[1] = 2;
  spacing[2] = 1;
  bool nonzero = false;
  for (int slice = 0; slice < 113; ++slice) {
    std::string path =
        std::string(directory) + "/CThead." + std::to_string(slice + 1);
    FILE *f = std::fopen(path.c_str(), "rb");
    if (!f) {
      std::fprintf(stderr, "Cannot open %s\n", path.c_str());
      return -1;
    }
    unsigned char bytes[256 * 256 * 2];
    size_t count = std::fread(bytes, 1, sizeof(bytes), f);
    bool valid =
        count == sizeof(bytes) && std::fgetc(f) == EOF && !std::ferror(f);
    std::fclose(f);
    if (!valid) {
      std::fprintf(stderr, "Invalid slice size: %s\n", path.c_str());
      return -1;
    }
    for (int row = 0; row < 256; ++row)
      for (int col = 0; col < 256; ++col) {
        int offset = 2 * (row * 256 + col);
        // Stanford CThead: unsigned 16-bit big-endian, no header.
        auto value = static_cast<std::uint16_t>((bytes[offset] << 8) |
                                                bytes[offset + 1]);
        data[col][112 - slice][255 - row] = value;
        nonzero = nonzero || value != 0;
      }
  }
  if (!nonzero) {
    std::fprintf(stderr, "Volume contains only zeros\n");
    return -1;
  }
  currentLayer = kmax / 2;
  clipEnabled = false;
  update_image_geometry();
  return 0;
}

void rotation(int direction) {
  if (direction < DROITE || direction > HAUT)
    return;
  double fraction =
      kmax > 1 ? static_cast<double>(currentLayer) / (kmax - 1) : 0.5;
  for (int i = 0; i < imax; ++i)
    for (int j = 0; j < jmax; ++j)
      for (int k = 0; k < kmax; ++k) {
        switch (direction) {
        case DROITE:
          rotated[k][j][i] = data[imax - 1 - i][j][k];
          break;
        case GAUCHE:
          rotated[k][j][i] = data[i][j][kmax - 1 - k];
          break;
        case BAS:
          rotated[i][k][j] = data[i][jmax - 1 - j][k];
          break;
        case HAUT:
          rotated[i][k][j] = data[i][j][kmax - 1 - k];
          break;
        }
      }
  if (direction == DROITE || direction == GAUCHE) {
    std::swap(imax, kmax);
    std::swap(spacing[0], spacing[2]);
  } else {
    std::swap(jmax, kmax);
    std::swap(spacing[1], spacing[2]);
  }
  for (int i = 0; i < imax; ++i)
    for (int j = 0; j < jmax; ++j)
      for (int k = 0; k < kmax; ++k)
        data[i][j][k] = rotated[i][j][k];
  currentLayer =
      bound(static_cast<int>(std::lround(fraction * (kmax - 1))), kmax);
  update_image_geometry();
}

void rendu(int ia, int ib, int ja, int jb, int kmin, double bone,
           double tissue) {
  const double background[3] = {double(fondR), double(fondG), double(fondB)};
  int front = clipEnabled ? bound(currentLayer, kmax) : kmax - 1;
  for (int y = std::max(0, ja); y < std::min(jb, ymax); ++y)
    for (int x = std::max(0, ia); x < std::min(ib, xmax); ++x) {
      double vx = xmax > 1 ? double(x) * (imax - 1) / (xmax - 1) : 0;
      double vy = ymax > 1 ? double(y) * (jmax - 1) / (ymax - 1) : 0;
      double color[3] = {0, 0, 0}, alpha = 0;
      const bool skinBrain = mode == 8;
      bool soft = mode == TISSU || (mode == TISSUOS && x < xmax / 2) ||
                  (mode == OSTISSU && x >= xmax / 2);
      for (int k = front; k >= std::max(0, kmin); --k) {
        double value = sample(vx, vy, k);
        bool isBone = value >= bone;
        if (value < (mode == TRANS || skinBrain ? tissue
                                                : (soft ? tissue : bone)))
          continue;
        double light = shade(vx, vy, k);
        double rgb[3];
        if (skinBrain) {
          // CThead has no labels: this is an intensity-based educational view.
          rgb[0] = isBone ? 224.0 : 214.0;
          rgb[1] = isBone ? 184.0 : 126.0;
          rgb[2] = isBone ? 153.0 : 145.0;
        } else {
          rgb[0] = isBone ? 242.0 : 210.0;
          rgb[1] = isBone ? 237.0 : 143.0;
          rgb[2] = isBone ? 218.0 : 116.0;
        }
        double a = 1;
        if (mode == TRANS || skinBrain) {
          // Extinction per physical unit; alpha is corrected for ray spacing.
          double density = skinBrain ? (isBone ? 0.030 : 0.010)
                                     : (isBone ? 0.13 : 0.008);
          a = 1 -
              std::exp(-density * clamp(globalTransparency, 0, 1) * spacing[2]);
        }
        for (int c = 0; c < 3; ++c)
          color[c] += (1 - alpha) * a * rgb[c] * light;
        alpha += (1 - alpha) * a;
        if (alpha >= 0.995)
          break;
      }
      for (int c = 0; c < 3; ++c)
        color[c] += (1 - alpha) * background[c];
      pixel(x, y, color);
    }
}

void rendu_layer(int slice, float opacity) {
  slice = bound(slice, kmax);
  double a = clamp(opacity, 0, 1);
  for (int y = 0; y < ymax; ++y)
    for (int x = 0; x < xmax; ++x) {
      double vx = xmax > 1 ? double(x) * (imax - 1) / (xmax - 1) : 0;
      double vy = ymax > 1 ? double(y) * (jmax - 1) / (ymax - 1) : 0;
      double value = sample(vx, vy, slice);
      double gray =
          255 * clamp((value - windowCenter) / std::max(1.0, windowWidth) + 0.5,
                      0, 1);
      double color[3] = {a * gray + (1 - a) * fondR, a * gray + (1 - a) * fondG,
                         a * gray + (1 - a) * fondB};
      pixel(x, y, color);
    }
}
