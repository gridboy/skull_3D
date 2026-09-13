#include "volume_renderer.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <unistd.h>
#include <vector>
double seuil_a = 1550, seuil_b = 670;
static std::vector<std::uint8_t> frame() {
  return {image, image + 3 * xmax * ymax};
}
static bool background() {
  for (int n = 0; n < xmax * ymax; ++n)
    if (image[3 * n] != fondR || image[3 * n + 1] != fondG ||
        image[3 * n + 2] != fondB)
      return false;
  return true;
}
static void render() { rendu(0, xmax, 0, ymax, 0, seuil_a, seuil_b); }
int main() {
  imax = jmax = kmax = 8;
  xmax = ymax = 8;
  spacing[0] = spacing[1] = spacing[2] = 1;
  std::memset(data, 0, sizeof(data));
  for (int i = 2; i < 6; ++i)
    for (int j = 2; j < 6; ++j)
      for (int k = 2; k < 6; ++k)
        data[i][j][k] = 2000;
  mode = TRANS;
  globalTransparency = 0;
  render();
  assert(background());
  globalTransparency = .2f;
  render();
  auto low = frame();
  globalTransparency = .8f;
  render();
  assert(!background());
  assert(image[3 * (3 * xmax + 3)] > low[3 * (3 * xmax + 3)]);
  // Two material planes: soft in front of bone, ten-unit ray step.
  std::memset(data, 0, sizeof(data));
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j) {
      data[i][j][4] = 1000;
      data[i][j][3] = 2000;
    }
  globalTransparency = 1;
  spacing[2] = 10;
  render();
  const int center = 3 * (3 * xmax + 3);
  assert(image[center] == 184 && image[center + 1] == 176 &&
         image[center + 2] == 163);
  // Same physical thickness at two ray spacings must composite identically.
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        data[i][j][k] = 2000;
  kmax = 4;
  spacing[2] = 1;
  render();
  auto thinSteps = frame();
  kmax = 2;
  spacing[2] = 2;
  render();
  assert(frame() == thinSteps);
  kmax = 8;
  spacing[2] = 1;
  std::memset(data, 0, sizeof(data));
  for (int i = 2; i < 6; ++i)
    for (int j = 2; j < 6; ++j)
      for (int k = 2; k < 6; ++k)
        data[i][j][k] = 2000;
  mode = OS;
  render();
  assert(!background());
  clipEnabled = true;
  currentLayer = 1;
  render();
  assert(background());
  clipEnabled = false;
  currentLayer = 0;
  move_layer(-1);
  assert(currentLayer == 0);
  currentLayer = 7;
  move_layer(1);
  assert(currentLayer == 7);
  // Slice and volume must place the asymmetric marker on the same pixel.
  std::memset(data, 0, sizeof(data));
  data[2][5][7] = 2000;
  mode = OS;
  render();
  assert(image[3 * (5 * xmax + 2)] != fondR);
  windowCenter = 1000;
  windowWidth = 2000;
  rendu_layer(7, 1);
  assert(image[3 * (5 * xmax + 2)] == 255 && image[3 * (2 * xmax + 5)] == 0);
  // Physical proportions must survive axis permutations, without voxel loss.
  imax = 8;
  jmax = 4;
  kmax = 6;
  spacing[0] = 1;
  spacing[1] = 2;
  spacing[2] = 3;
  std::memset(data, 0, sizeof(data));
  data[2][1][4] = 1234;
  currentLayer = 2;
  update_image_geometry();
  double ratio = double(xmax) / ymax;
  assert(ratio > 1.16 && ratio < 1.18); // 7 / 6, not 8 / 4.
  for (int n = 0; n < 4; ++n)
    rotation(DROITE);
  assert(imax == 8 && jmax == 4 && kmax == 6 && data[2][1][4] == 1234);
  assert(spacing[0] == 1 && spacing[1] == 2 && spacing[2] == 3);
  rotation(BAS);
  rotation(HAUT);
  assert(data[2][1][4] == 1234 && jmax == 4 && kmax == 6);
  rotation(DROITE);
  rotation(GAUCHE);
  assert(data[2][1][4] == 1234);
  // Real bytes establish byte order and the upright display transform.
  int loaded = charger_volume();
  assert(loaded == 0);
  assert(imax == 256 && jmax == 113 && kmax == 256);
  std::ifstream source("data/cthead/CThead.40", std::ios::binary);
  std::vector<unsigned char> bytes(131072);
  source.read(reinterpret_cast<char *>(bytes.data()), bytes.size());
  assert(source);
  for (int row = 0; row < 256; ++row)
    for (int col = 0; col < 256; ++col) {
      int p = 2 * (row * 256 + col);
      assert(data[col][73][255 - row] == ((bytes[p] << 8) | bytes[p + 1]));
    }
  for (mode = 0; mode < 6; ++mode) {
    if (mode == LAYER)
      rendu_layer(currentLayer, 1);
    else
      render();
    assert(!background());
  }
  mode = 8;
  globalTransparency = .5f;
  render();
  assert(!background());
  char temp[] = "/tmp/skull-invalid-XXXXXX";
  char *created = mkdtemp(temp);
  assert(created);
  std::string shortFile = std::string(temp) + "/CThead.1";
  {
    std::ofstream f(shortFile, std::ios::binary);
    f.put(0);
  }
  int invalid = charger_volume(temp);
  assert(invalid != 0);
  std::remove(shortFile.c_str());
  rmdir(temp);
  std::puts("PASS: opacity, clipping, slice alignment, bounds, physical "
            "aspect, rotations, original data, truncated input, six modes");
}
