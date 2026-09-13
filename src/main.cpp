// GLFW desktop viewer for the CThead dataset. See docs/SOURCES_AND_LICENSES.md.
#define GLFW_INCLUDE_NONE
#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include "volume_renderer.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#ifdef __APPLE__
#include <CoreGraphics/CoreGraphics.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreText/CoreText.h>
#endif

#ifndef SKULL_DATA_DIR
#define SKULL_DATA_DIR "data/cthead"
#endif

double seuil_a = 1550, seuil_b = 670;
namespace {
GLFWwindow *window = nullptr;
GLuint texture = 0;
const int EXPLODED = 6;
const int SKIN_BRAIN = 7;
GLuint explodedTextures[5] = {0, 0, 0, 0, 0};
std::vector<std::uint8_t> explodedPixels[5];
int explodedSlices[5] = {0, 0, 0, 0, 0};
double displayRotation = 0;
GLuint legendTexture = 0;
bool showLegend = true;
bool dirty = true;
std::string dataDir = SKULL_DATA_DIR;
std::string view = "coronal (dataset)";
const char *modes[] = {"Tissue", "Tissue | bone",  "Bone | tissue",
                       "Bone",   "Volume opacity", "Slice", "CThead exploded",
                       "Skin + brain"};
void title() {
  char text[512];
  if (mode == EXPLODED) {
    std::snprintf(text, sizeof(text),
                  "Skull | eclate CThead | coupes reelles | rotation %.0f deg",
                  displayRotation);
    glfwSetWindowTitle(window, text);
    return;
  }
  std::snprintf(text, sizeof(text),
                "Skull | %s | %s | slice %d/%d | opacity %.2f | clip %s | bone "
                "%.0f tissue %.0f | W/L %.0f/%.0f",
                modes[mode], view.c_str(), currentLayer + 1, kmax,
                globalTransparency, clipEnabled ? "on" : "off", seuil_a,
                seuil_b, windowWidth, windowCenter);
  glfwSetWindowTitle(window, text);
}
void legendText(CGContextRef context, const char *text, CGFloat x, CGFloat y,
                CGFloat size, CGFloat r, CGFloat g, CGFloat b) {
#ifdef __APPLE__
  CFStringRef string = CFStringCreateWithCString(nullptr, text, kCFStringEncodingUTF8);
  if (!string) return;
  CFMutableAttributedStringRef attributed = CFAttributedStringCreateMutable(nullptr, 0);
  CTFontRef font = CTFontCreateWithName(CFSTR("Helvetica"), size, nullptr);
  CGColorRef color = CGColorCreateGenericRGB(r, g, b, 1);
  CFAttributedStringReplaceString(attributed, CFRangeMake(0, 0), string);
  CFAttributedStringSetAttribute(attributed, CFRangeMake(0, CFStringGetLength(string)),
                                 kCTFontAttributeName, font);
  CFAttributedStringSetAttribute(attributed, CFRangeMake(0, CFStringGetLength(string)),
                                 kCTForegroundColorAttributeName, color);
  CTLineRef line = CTLineCreateWithAttributedString(attributed);
  CGContextSetTextPosition(context, x, y);
  CTLineDraw(line, context);
  CFRelease(line); CGColorRelease(color); CFRelease(font);
  CFRelease(attributed); CFRelease(string);
#else
  (void)context; (void)text; (void)x; (void)y; (void)size; (void)r; (void)g; (void)b;
#endif
}
bool createLegendTexture() {
#ifdef __APPLE__
  const size_t width = 900, height = 430;
  std::vector<unsigned char> pixels(width * height * 4, 0);
  CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
  CGContextRef context = CGBitmapContextCreate(
      pixels.data(), width, height, 8, width * 4, space,
      kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
  if (!context) { CGColorSpaceRelease(space); return false; }
  CGContextSetRGBFillColor(context, 0.035, 0.045, 0.065, 0.90);
  CGContextFillRect(context, CGRectMake(0, 0, width, height));
  legendText(context, "Lecture des couches CThead", 22, 394, 22, 1, 1, 1);
  legendText(context, "Peau / tissus : seuil tissus", 60, 352, 18, 0.95, 0.65, 0.42);
  legendText(context, "Os / squelette : seuil os", 60, 320, 18, 0.96, 0.93, 0.78);
  legendText(context, "Cerveau : proxy d'intensite, non segmente", 60, 288, 18, 0.86, 0.56, 0.62);
  legendText(context, "Meninges : absentes des donnees CThead", 60, 256, 18, 0.72, 0.78, 0.84);
  legendText(context, "Arteres : absentes ; angio-CT requis", 60, 224, 18, 0.90, 0.38, 0.38);
  legendText(context, "1 tissus | 2 tissus+os | 3 os+tissus | 4 os", 22, 180, 15, 0.85, 0.88, 0.92);
  legendText(context, "5 transparence | 6 coupe | 7 eclate CThead | 8 peau+cerveau", 22, 154, 15, 0.85, 0.88, 0.92);
  legendText(context, "Fleches rotation fluide | C coupe + legende | H legende", 22, 128, 15, 0.85, 0.88, 0.92);
  legendText(context, "[ ] / molette / PgUp PgDn couches | Debut Fin bornes", 22, 102, 15, 0.85, 0.88, 0.92);
  legendText(context, "+ - opacite | o O seuil os | t T seuil tissus", 22, 76, 15, 0.85, 0.88, 0.92);
  legendText(context, "w W fenetre | l L niveau | X Y Z vues | R reinitialiser | Echap quitter", 22, 50, 15, 0.85, 0.88, 0.92);
  for (int i = 0; i < 3; ++i) {
    const CGFloat colors[3][3] = {{0.95,0.65,0.42},{0.96,0.93,0.78},{0.86,0.56,0.62}};
    CGContextSetRGBFillColor(context, colors[i][0], colors[i][1], colors[i][2], 1);
    CGContextFillRect(context, CGRectMake(22, 343 - i * 32, 22, 22));
  }
  CGContextFlush(context); CGContextRelease(context); CGColorSpaceRelease(space);
  glGenTextures(1, &legendTexture); glBindTexture(GL_TEXTURE_2D, legendTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, pixels.data());
  return glGetError() == GL_NO_ERROR;
#else
  return false;
#endif
}
void drawLegend(int width, int height) {
  if (!showLegend || !legendTexture) return;
  glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
  glOrtho(0, width, 0, height, -1, 1);
  glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
  glEnable(GL_TEXTURE_2D); glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBindTexture(GL_TEXTURE_2D, legendTexture); glColor4f(1, 1, 1, 1);
  const double w = 450, h = 215;
  glBegin(GL_QUADS);
  glTexCoord2f(0, 0); glVertex2d(18, 18); glTexCoord2f(1, 0); glVertex2d(18+w, 18);
  glTexCoord2f(1, 1); glVertex2d(18+w, 18+h); glTexCoord2f(0, 1); glVertex2d(18, 18+h);
  glEnd(); glDisable(GL_BLEND); glDisable(GL_TEXTURE_2D);
  glMatrixMode(GL_MODELVIEW); glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
}
void updateExplodedTextures() {
  const int positions[5] = {kmax / 10, kmax * 3 / 10, kmax / 2,
                             kmax * 7 / 10, kmax * 9 / 10};
  glGenTextures(5, explodedTextures);
  for (int i = 0; i < 5; ++i) {
    explodedSlices[i] = std::max(0, std::min(kmax - 1, positions[i]));
    const int oldMode = mode;
    mode = LAYER;
    rendu_layer(explodedSlices[i], 1.0f);
    mode = oldMode;
    explodedPixels[i].assign(image, image + 3 * xmax * ymax);
    glBindTexture(GL_TEXTURE_2D, explodedTextures[i]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, xmax, ymax, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, explodedPixels[i].data());
  }
}
void drawExplodedPanel(GLuint textureId, double x, double y, double width,
                       double height) {
  glPushMatrix();
  glTranslated(x, y, 0);
  glBindTexture(GL_TEXTURE_2D, textureId);
  glBegin(GL_QUADS);
  glTexCoord2f(0, 0); glVertex2d(-width, -height);
  glTexCoord2f(1, 0); glVertex2d(width, -height);
  glTexCoord2f(1, 1); glVertex2d(width, height);
  glTexCoord2f(0, 1); glVertex2d(-width, height);
  glEnd();
  glPopMatrix();
}
void calculate() {
  if (mode == LAYER)
    rendu_layer(currentLayer, 1.0f);
  else
    rendu(0, xmax, 0, ymax, 0, seuil_a, seuil_b);
}
void draw() {
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  if (width <= 0 || height <= 0)
    return;
  glViewport(0, 0, width, height);
  glClearColor(fondR / 255.f, fondG / 255.f, fondB / 255.f, 1);
  glClear(GL_COLOR_BUFFER_BIT);
  if (mode == EXPLODED) {
    glEnable(GL_TEXTURE_2D); glColor3f(1, 1, 1);
    glPushMatrix(); glRotated(displayRotation, 0, 0, 1);
    for (int i = 0; i < 5; ++i) {
      double offset = i - 2.0;
      drawExplodedPanel(explodedTextures[i], offset * 0.11, -offset * 0.075,
                        0.34, 0.235);
    }
    glPopMatrix(); glDisable(GL_TEXTURE_2D);
    drawLegend(width, height); glfwSwapBuffers(window); return;
  }
  double scale = std::min(double(width) / xmax, double(height) / ymax) * 0.94;
  double x = xmax * scale / width, y = ymax * scale / height;
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, texture);
  glPushMatrix();
  glRotated(displayRotation, 0, 0, 1);
  glColor3f(1, 1, 1);
  glBegin(GL_QUADS);
  glTexCoord2f(0, 0);
  glVertex2d(-x, -y);
  glTexCoord2f(1, 0);
  glVertex2d(x, -y);
  glTexCoord2f(1, 1);
  glVertex2d(x, y);
  glTexCoord2f(0, 1);
  glVertex2d(-x, y);
  glEnd();
  glPopMatrix();
  glDisable(GL_TEXTURE_2D);
  drawLegend(width, height);
  if (glGetError() != GL_NO_ERROR)
    std::fprintf(stderr, "OpenGL draw error\n");
  glfwSwapBuffers(window);
}
void setView(int axis) {
  if (charger_volume(dataDir.c_str()) != 0) {
    glfwSetWindowShouldClose(window, 1);
    return;
  }
  if (axis == 0) {
    rotation(BAS);
    view = "axial (dataset)";
  } else if (axis == 1) {
    view = "coronal (dataset)";
  } else {
    rotation(DROITE);
    view = "sagittal (dataset)";
  }
  currentLayer = kmax / 2;
  dirty = true;
}
void character(GLFWwindow *, unsigned int c) {
  switch (c) {
  case 'o':
    seuil_a = std::max(seuil_b, seuil_a - 50);
    break;
  case 'O':
    seuil_a = std::min(65535.0, seuil_a + 50);
    break;
  case 't':
    seuil_b = std::max(0.0, seuil_b - 50);
    break;
  case 'T':
    seuil_b = std::min(seuil_a, seuil_b + 50);
    break;
  case '+':
  case '=':
    globalTransparency = std::min(1.f, globalTransparency + .05f);
    break;
  case '-':
    globalTransparency = std::max(0.f, globalTransparency - .05f);
    break;
  case '[':
    move_layer(-1);
    break;
  case ']':
    move_layer(1);
    break;
  case 'w':
    windowWidth = std::max(1.0, windowWidth - 100);
    break;
  case 'W':
    windowWidth = std::min(65535.0, windowWidth + 100);
    break;
  case 'l':
    windowCenter = std::max(0.0, windowCenter - 50);
    break;
  case 'L':
    windowCenter = std::min(65535.0, windowCenter + 50);
    break;
  default:
    return;
  }
  dirty = true;
}
void key(GLFWwindow *, int key, int, int action, int) {
  if (action != GLFW_PRESS && action != GLFW_REPEAT)
    return;
  switch (key) {
  case GLFW_KEY_ESCAPE:
    glfwSetWindowShouldClose(window, 1);
    return;
  case GLFW_KEY_1:
    mode = TISSU;
    break;
  case GLFW_KEY_2:
    mode = TISSUOS;
    break;
  case GLFW_KEY_3:
    mode = OSTISSU;
    break;
  case GLFW_KEY_4:
    mode = OS;
    break;
  case GLFW_KEY_5:
    mode = TRANS;
    break;
  case GLFW_KEY_6:
    mode = LAYER;
    break;
  case GLFW_KEY_7:
    mode = EXPLODED;
    updateExplodedTextures();
    break;
  case GLFW_KEY_8:
    mode = SKIN_BRAIN;
    break;
  case GLFW_KEY_LEFT:
    displayRotation -= 8;
    break;
  case GLFW_KEY_RIGHT:
    displayRotation += 8;
    break;
  case GLFW_KEY_UP:
    displayRotation += 8;
    break;
  case GLFW_KEY_DOWN:
    displayRotation -= 8;
    break;
  case GLFW_KEY_PAGE_UP:
    move_layer(1);
    break;
  case GLFW_KEY_PAGE_DOWN:
    move_layer(-1);
    break;
  case GLFW_KEY_HOME:
    currentLayer = 0;
    break;
  case GLFW_KEY_END:
    currentLayer = kmax - 1;
    break;
  case GLFW_KEY_C:
    if (action == GLFW_PRESS) {
      clipEnabled = !clipEnabled;
      showLegend = true;
    }
    break;
  case GLFW_KEY_H:
    if (action == GLFW_PRESS) showLegend = !showLegend;
    break;
  case GLFW_KEY_X:
    setView(0);
    break;
  case GLFW_KEY_Y:
    setView(1);
    break;
  case GLFW_KEY_Z:
    setView(2);
    break;
  case GLFW_KEY_R:
    displayRotation = 0;
    setView(1);
    mode = OS;
    globalTransparency = .5f;
    seuil_a = 1550;
    seuil_b = 670;
    windowCenter = 1100;
    windowWidth = 1800;
    break;
  default:
    return;
  }
  dirty = true;
}
void scroll(GLFWwindow *, double, double y) {
  move_layer(y > 0 ? 1 : y < 0 ? -1 : 0);
  dirty = true;
}
bool save(const std::string &path) {
  FILE *f = std::fopen(path.c_str(), "wb");
  if (!f)
    return false;
  bool ok = std::fprintf(f, "P6\n%d %d\n255\n", xmax, ymax) > 0;
  for (int y = ymax - 1; y >= 0; --y)
    ok = (std::fwrite(image + y * xmax * 3, 3, xmax, f) ==
          static_cast<size_t>(xmax)) &&
         ok;
  return std::fclose(f) == 0 && ok;
}
} // namespace
int main(int argc, char **argv) {
  std::string snapshots;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--data" && i + 1 < argc)
      dataDir = argv[++i];
    else if (arg == "--snapshots" && i + 1 < argc)
      snapshots = argv[++i];
    else if (arg == "--help") {
      std::puts(
          "Skull [--data directory] [--snapshots existing-directory]\n1-6 "
          "modes; 7 exploded real CThead slices; 8 skin+brain; arrows rotate smoothly; X/Y/Z dataset views; C clip+legend; H hide legend; "
          "wheel/[ ]/PgUp/PgDn slice; Home/End endpoints; +/- opacity; o/O "
          "bone; t/T tissue; w/W window; l/L level; R reset; Esc quit.");
      return 0;
    } else {
      std::fprintf(stderr, "Invalid argument: %s\n", argv[i]);
      return 1;
    }
  }
  if (charger_volume(dataDir.c_str()) != 0)
    return 1;
  std::printf("Loaded CThead: 113 original slices, voxel proportions 1:1:2\n");
  if (!snapshots.empty()) {
    for (mode = 0; mode <= LAYER; ++mode) {
      calculate();
      if (!save(snapshots + "/mode-" + std::to_string(mode + 1) + ".ppm"))
        return 1;
    }
    return 0;
  }
  glfwSetErrorCallback(
      [](int, const char *s) { std::fprintf(stderr, "GLFW: %s\n", s); });
  if (!glfwInit())
    return 1;
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  window = glfwCreateWindow(1000, 850, "Skull", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return 1;
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glfwSetKeyCallback(window, key);
  glfwSetCharCallback(window, character);
  glfwSetScrollCallback(window, scroll);
  glfwSetWindowRefreshCallback(window, [](GLFWwindow *) { draw(); });
  updateExplodedTextures();
  createLegendTexture();
  while (!glfwWindowShouldClose(window)) {
    if (dirty) {
      if (mode != EXPLODED) {
        calculate();
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, xmax, ymax, 0, GL_RGB,
                     GL_UNSIGNED_BYTE, image);
      }
      title();
      dirty = false;
    }
    draw();
    glfwWaitEvents();
  }
  glDeleteTextures(1, &texture);
  glDeleteTextures(5, explodedTextures);
  glDeleteTextures(1, &legendTexture);
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
