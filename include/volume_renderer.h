#pragma once
#include <cstdint>

// Educational volume renderer, derived from Michel Grave's teaching project.
// See docs/SOURCES_AND_LICENSES.md for provenance and licensing limitations.
constexpr int IMAX = 256, JMAX = 256, KMAX = 256;
constexpr int XMAX = 1024, YMAX = 1024;
enum { DROITE = 1, GAUCHE, BAS, HAUT };
enum { TISSU, TISSUOS, OSTISSU, OS, TRANS, LAYER };
extern std::uint16_t data[IMAX][JMAX][KMAX];
extern int imax, jmax, kmax, xmax, ymax, mode, currentLayer;
extern float spacing[3], globalTransparency;
extern bool clipEnabled;
extern double seuil_a, seuil_b, windowCenter, windowWidth;
extern int fondR, fondG, fondB;
extern std::uint8_t image[3 * XMAX * YMAX];
int charger_volume(const char *directory = "data/cthead");
void rotation(int direction);
void update_image_geometry();
void move_layer(int delta);
void rendu(int ia, int ib, int ja, int jb, int kmin, double bone,
           double tissue);
void rendu_layer(int slice, float opacity);
