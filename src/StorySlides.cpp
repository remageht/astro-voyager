#include "StorySlides.h"
#include "Expedition.h"

#include <cmath>

#include <imgui.h>

namespace {

// Procedural miniature scenes (ImDrawList vector art). Each painter works
// on a size x size square at pos, dark glass background included. Keep
// shapes chunky: 2-color gradients via AddRectFilledMultiColor, circles
// via AddCircleFilled, silhouettes via triangles/rects.

void artGroundAndStars(ImDrawList* dl, const ImVec2& pos, float size) {
  dl->AddRectFilledMultiColor(pos, ImVec2(pos.x + size, pos.y + size),
                              IM_COL32(4, 6, 14, 255), IM_COL32(10, 12, 28, 255),
                              IM_COL32(4, 6, 16, 255), IM_COL32(2, 3, 8, 255));
  for (int i = 0; i < 14; ++i) {
    float fx = static_cast<float>((i * 37) % 100) / 100.0f;
    float fy = static_cast<float>((i * 53 + 11) % 60) / 100.0f;
    float r = 1.0f + static_cast<float>(i % 3);
    dl->AddCircleFilled(ImVec2(pos.x + fx * size, pos.y + fy * size * 0.55f), r,
                        IM_COL32(235, 240, 255, 180 - 8 * (i % 6)));
  }
  // Ground silhouette.
  float gy = pos.y + size * 0.72f;
  dl->AddRectFilled(ImVec2(pos.x, gy), ImVec2(pos.x + size, pos.y + size),
                    IM_COL32(6, 8, 14, 255));
  dl->AddTriangleFilled(ImVec2(pos.x + size * 0.72f, gy),
                        ImVec2(pos.x + size * 0.94f, gy),
                        ImVec2(pos.x + size * 0.83f, pos.y + size * 0.62f),
                        IM_COL32(5, 7, 12, 255));
}

void artHouseWindow(ImDrawList* dl, const ImVec2& pos, float size) {
  artGroundAndStars(dl, pos, size);
  float gx = pos.x + size * 0.28f;
  float gy = pos.y + size * 0.66f;
  // House body + roof + warm window.
  dl->AddRectFilled(ImVec2(gx, gy), ImVec2(gx + size * 0.30f, pos.y + size * 0.86f),
                    IM_COL32(14, 12, 12, 255));
  dl->AddTriangleFilled(ImVec2(gx - size * 0.03f, gy),
                        ImVec2(gx + size * 0.33f, gy),
                        ImVec2(gx + size * 0.15f, pos.y + size * 0.56f),
                        IM_COL32(12, 10, 10, 255));
  dl->AddRectFilled(ImVec2(gx + size * 0.10f, gy + size * 0.05f),
                    ImVec2(gx + size * 0.20f, gy + size * 0.12f),
                    IM_COL32(255, 196, 90, 235));
  // Telescope silhouette on the roof? Simple antenna:
  dl->AddLine(ImVec2(gx + size * 0.24f, gy), ImVec2(gx + size * 0.30f, pos.y + size * 0.50f),
              IM_COL32(20, 20, 22, 255), 1.6f);
}

void artToyRocket(ImDrawList* dl, const ImVec2& pos, float size) {
  artGroundAndStars(dl, pos, size);
  float cx = pos.x + size * 0.5f;
  float cy = pos.y + size * 0.44f;
  // Toy rocket: body, nose cone, fins, window, flame.
  dl->AddRectFilled(ImVec2(cx - size * 0.045f, cy - size * 0.10f),
                    ImVec2(cx + size * 0.045f, cy + size * 0.16f),
                    IM_COL32(210, 215, 225, 255));
  dl->AddTriangleFilled(ImVec2(cx - size * 0.05f, cy - size * 0.10f),
                        ImVec2(cx + size * 0.05f, cy - size * 0.10f),
                        ImVec2(cx, cy - size * 0.19f), IM_COL32(225, 130, 60, 255));
  dl->AddTriangleFilled(ImVec2(cx - size * 0.045f, cy + size * 0.16f),
                        ImVec2(cx, cy + size * 0.16f),
                        ImVec2(cx - size * 0.085f, cy + size * 0.26f),
                        IM_COL32(150, 60, 40, 255));
  dl->AddCircleFilled(ImVec2(cx, cy - size * 0.03f), size * 0.02f, IM_COL32(60, 140, 200, 255));
  dl->AddTriangleFilled(ImVec2(cx - size * 0.03f, cy + size * 0.16f),
                        ImVec2(cx + size * 0.03f, cy + size * 0.16f),
                        ImVec2(cx, cy + size * 0.29f), IM_COL32(255, 170, 60, 210));
  // Small girl silhouette watching.
  dl->AddCircleFilled(ImVec2(pos.x + size * 0.30f, pos.y + size * 0.660f), size * 0.022f,
                      IM_COL32(18, 16, 20, 255));
  dl->AddRectFilled(ImVec2(pos.x + size * 0.286f, pos.y + size * 0.682f),
                    ImVec2(pos.x + size * 0.314f, pos.y + size * 0.760f),
                    IM_COL32(18, 16, 20, 255));
}

void artDimmingSun(ImDrawList* dl, const ImVec2& pos, float size) {
  dl->AddRectFilledMultiColor(pos, ImVec2(pos.x + size, pos.y + size),
                              IM_COL32(18, 14, 20, 255), IM_COL32(30, 24, 26, 255),
                              IM_COL32(20, 15, 18, 255), IM_COL32(8, 7, 10, 255));
  float cx = pos.x + size * 0.5f;
  float cy = pos.y + size * 0.42f;
  float cd = size * 0.30f;
  dl->AddCircleFilled(ImVec2(cx, cy), cd, IM_COL32(90, 44, 22, 255));
  dl->AddCircleFilled(ImVec2(cx, cy), cd * 0.78f, IM_COL32(120, 58, 28, 255));
  // Faint dying rays.
  for (int i = 0; i < 8; ++i) {
    float a = static_cast<float>(i) * 6.28318f / 8.0f;
    ImVec2 p1(cx + cosf(a) * (cd + size * 0.03f), cy + sinf(a) * (cd + size * 0.03f));
    ImVec2 p2(cx + cosf(a) * (cd + size * 0.075f), cy + sinf(a) * (cd + size * 0.075f));
    dl->AddLine(p1, p2, IM_COL32(160, 80, 40, 120), 2.2f);
  }
  // Frosted land below.
  dl->AddRectFilled(ImVec2(pos.x, pos.y + size * 0.66f), ImVec2(pos.x + size, pos.y + size),
                    IM_COL32(14, 12, 18, 255));
  dl->AddRectFilledMultiColor(ImVec2(pos.x, pos.y + size * 0.66f),
                              ImVec2(pos.x + size, pos.y + size * 0.74f),
                              IM_COL32(28, 22, 26, 255), IM_COL32(18, 15, 20, 255),
                              IM_COL32(18, 15, 20, 255), IM_COL32(28, 22, 26, 255));
}

void artCrisis(ImDrawList* dl, const ImVec2& pos, float size) {
  dl->AddRectFilledMultiColor(pos, ImVec2(pos.x + size, pos.y + size * 0.75f),
                              IM_COL32(46, 16, 14, 255), IM_COL32(24, 10, 10, 255),
                              IM_COL32(24, 10, 10, 255), IM_COL32(46, 16, 14, 255));
  dl->AddRectFilled(ImVec2(pos.x, pos.y + size * 0.75f), ImVec2(pos.x + size, pos.y + size),
                    IM_COL32(8, 5, 6, 255));
  // City silhouette row.
  float xx = pos.x + size * 0.06f;
  for (int i = 0; i < 10; ++i) {
    float h = size * (0.06f + static_cast<float>((i * 23) % 9) / 90.0f);
    dl->AddRectFilled(ImVec2(xx, pos.y + size * 0.75f - h),
                      ImVec2(xx + size * 0.055f, pos.y + size * 0.75f),
                      IM_COL32(4, 3, 4, 255));
    if (i % 3 == 1) {
      dl->AddRectFilled(ImVec2(xx + size * 0.02f, pos.y + size * 0.75f - h * 0.5f),
                        ImVec2(xx + size * 0.035f, pos.y + size * 0.75f - h * 0.35f),
                        IM_COL32(200, 90, 40, 130));
    }
    xx += size * 0.092f;
  }
  // Distress flare arc.
  dl->AddLine(ImVec2(pos.x + size * 0.2f, pos.y + size * 0.18f),
              ImVec2(pos.x + size * 0.5f, pos.y + size * 0.08f),
              IM_COL32(220, 120, 50, 150), 2.0f);
  dl->AddLine(ImVec2(pos.x + size * 0.5f, pos.y + size * 0.08f),
              ImVec2(pos.x + size * 0.82f, pos.y + size * 0.20f),
              IM_COL32(220, 120, 50, 110), 2.0f);
}

void artLaunchPad(ImDrawList* dl, const ImVec2& pos, float size) {
  artGroundAndStars(dl, pos, size);
  float cx = pos.x + size * 0.5f;
  float gy = pos.y + size * 0.78f;
  // Pad lights pools.
  dl->AddCircleFilled(ImVec2(cx - size * 0.16f, gy), size * 0.035f, IM_COL32(255, 240, 190, 70));
  dl->AddCircleFilled(ImVec2(cx + size * 0.16f, gy), size * 0.035f, IM_COL32(255, 240, 190, 70));
  // Rocket on tower.
  dl->AddRectFilled(ImVec2(cx - size * 0.04f, gy - size * 0.30f),
                    ImVec2(cx + size * 0.04f, gy), IM_COL32(205, 210, 220, 255));
  dl->AddTriangleFilled(ImVec2(cx - size * 0.04f, gy - size * 0.30f),
                        ImVec2(cx + size * 0.04f, gy - size * 0.30f),
                        ImVec2(cx, gy - size * 0.37f), IM_COL32(225, 130, 60, 255));
  // Service tower left.
  dl->AddRectFilled(ImVec2(cx - size * 0.10f, gy - size * 0.34f),
                    ImVec2(cx - size * 0.075f, gy), IM_COL32(40, 42, 48, 255));
  dl->AddLine(ImVec2(cx - size * 0.10f, gy - size * 0.28f),
              ImVec2(cx - size * 0.04f, gy - size * 0.28f), IM_COL32(40, 42, 48, 255), 2.0f);
  // Exhaust mist.
  dl->AddCircleFilled(ImVec2(cx, gy), size * 0.05f, IM_COL32(180, 185, 195, 60));
}

void artDeparture(ImDrawList* dl, const ImVec2& pos, float size) {
  dl->AddRectFilledMultiColor(pos, ImVec2(pos.x + size, pos.y + size),
                              IM_COL32(3, 5, 12, 255), IM_COL32(8, 10, 22, 255),
                              IM_COL32(6, 8, 18, 255), IM_COL32(3, 4, 10, 255));
  for (int i = 0; i < 16; ++i) {
    float fx = static_cast<float>((i * 41 + 7) % 100) / 100.0f;
    float fy = static_cast<float>((i * 29) % 100) / 100.0f;
    dl->AddCircleFilled(ImVec2(pos.x + fx * size, pos.y + fy * size),
                        1.0f + static_cast<float>(i % 2), IM_COL32(235, 240, 255, 150));
  }
  // Earth arc with atmosphere glow at bottom.
  dl->AddCircleFilled(ImVec2(pos.x + size * 0.5f, pos.y + size * 1.24f), size * 0.42f,
                      IM_COL32(10, 18, 60, 255));
  dl->AddCircleFilled(ImVec2(pos.x + size * 0.5f, pos.y + size * 1.24f), size * 0.40f,
                      IM_COL32(24, 44, 110, 255));
  dl->AddCircleFilled(ImVec2(pos.x + size * 0.5f, pos.y + size * 1.24f), size * 0.37f,
                      IM_COL32(38, 70, 150, 255));
  dl->AddCircleFilled(ImVec2(pos.x + size * 0.28f, pos.y + size * 1.12f), size * 0.05f,
                      IM_COL32(90, 160, 120, 110));
  // Climbing rocket with flame trail.
  float cx = pos.x + size * 0.62f;
  float cy = pos.y + size * 0.42f;
  dl->AddRectFilled(ImVec2(cx - size * 0.035f, cy - size * 0.09f),
                    ImVec2(cx + size * 0.035f, cy + size * 0.10f),
                    IM_COL32(225, 228, 235, 255));
  dl->AddTriangleFilled(ImVec2(cx - size * 0.035f, cy - size * 0.09f),
                        ImVec2(cx + size * 0.035f, cy - size * 0.09f),
                        ImVec2(cx, cy - size * 0.16f), IM_COL32(235, 140, 70, 255));
  dl->AddLine(ImVec2(cx, cy + size * 0.10f), ImVec2(cx, cy + size * 0.30f),
              IM_COL32(255, 180, 70, 200), 4.0f);
  dl->AddLine(ImVec2(cx, cy + size * 0.30f), ImVec2(cx, cy + size * 0.40f),
              IM_COL32(255, 200, 90, 90), 2.4f);
}

void artPhotonRing(ImDrawList* dl, const ImVec2& pos, float size) {
  dl->AddRectFilled(pos, ImVec2(pos.x + size, pos.y + size), IM_COL32(0, 0, 0, 255));
  for (int i = 0; i < 10; ++i) {
    float fx = static_cast<float>((i * 43 + 3) % 100) / 100.0f;
    float fy = static_cast<float>((i * 31 + 5) % 100) / 100.0f;
    dl->AddCircleFilled(ImVec2(pos.x + fx * size, pos.y + fy * size), 1.2f,
                        IM_COL32(220, 228, 255, 140));
  }
  float cx = pos.x + size * 0.5f;
  float cy = pos.y + size * 0.5f;
  // Lensing arcs (gold torus halves) + shadow.
  dl->AddCircleFilled(ImVec2(cx, cy), size * 0.34f, IM_COL32(250, 205, 120, 60));
  float rg = size * 0.32f;
  for (int i = 0; i < 48; ++i) {
    float a = static_cast<float>(i) * 6.28318f / 48.0f;
    float w = 1.6f + 1.2f * (1.0f + cosf(a * 2.0f)) * 0.5f;
    dl->AddLine(ImVec2(cx + cosf(a) * rg, cy + sinf(a) * rg),
                ImVec2(cx + cosf(a + 0.1f) * rg, cy + sinf(a + 0.1f) * rg),
                IM_COL32(255, 200, 110, 230), w);
  }
  dl->AddCircleFilled(ImVec2(cx, cy), size * 0.21f, IM_COL32(2, 2, 4, 255));
}

void artJet(ImDrawList* dl, const ImVec2& pos, float size) {
  dl->AddRectFilledMultiColor(pos, ImVec2(pos.x + size, pos.y + size),
                              IM_COL32(4, 5, 11, 255), IM_COL32(10, 10, 20, 255),
                              IM_COL32(10, 10, 20, 255), IM_COL32(4, 5, 11, 255));
  float cx = pos.x + size * 0.36f;
  float cy = pos.y + size * 0.52f;
  // Torus from the side with jet cone to the right.
  dl->AddCircleFilled(ImVec2(cx, cy), size * 0.16f, IM_COL32(255, 190, 110, 70));
  dl->AddCircleFilled(ImVec2(cx, cy), size * 0.10f, IM_COL32(255, 210, 140, 200));
  dl->AddCircleFilled(ImVec2(cx, cy), size * 0.06f, IM_COL32(6, 5, 8, 255));
  // Jet: tapering bright cone.
  for (int i = 0; i < 14; ++i) {
    float t = static_cast<float>(i) / 13.0f;
    float y0 = cy - (1.0f - t) * size * 0.045f;
    float w = (1.0f - t) * size * 0.045f + size * 0.010f;
    float x0 = cx + size * (0.14f + t * size / size * 0.62f);
    dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x0 + size * 0.02f, y0 + w),
                      IM_COL32(200 + int(55 * t), 230 - int(60 * t), 255, 200 - int(120 * t)));
  }
}

void artQuasar(ImDrawList* dl, const ImVec2& pos, float size) {
  dl->AddRectFilledMultiColor(pos, ImVec2(pos.x + size, pos.y + size),
                              IM_COL32(3, 4, 10, 255), IM_COL32(8, 8, 18, 255),
                              IM_COL32(8, 8, 18, 255), IM_COL32(3, 4, 10, 255));
  float cx = pos.x + size * 0.5f;
  float cy = pos.y + size * 0.46f;
  // Brilliant core + twin jets tilted.
  dl->AddCircleFilled(ImVec2(cx, cy), size * 0.05f, IM_COL32(255, 250, 235, 255));
  dl->AddCircleFilled(ImVec2(cx, cy), size * 0.10f, IM_COL32(255, 235, 190, 90));
  dl->AddCircleFilled(ImVec2(cx, cy), size * 0.18f, IM_COL32(170, 190, 255, 50));
  float ja = -0.9f;
  for (int k = 0; k < 2; ++k) {
    float dir = (k == 0) ? 1.0f : -1.0f;
    for (int i = 0; i < 12; ++i) {
      float t = static_cast<float>(i) / 11.0f;
      float ex = cx + dir * cosf(ja) * size * t * 0.42f;
      float ey = cy + dir * sinf(ja) * size * t * 0.42f;
      dl->AddCircleFilled(ImVec2(ex, ey), (1.0f - t) * size * 0.028f + 1.0f,
                          IM_COL32(200, 220, 255, 160 - int(130 * t)));
    }
  }
}

void artCluster(ImDrawList* dl, const ImVec2& pos, float size) {
  dl->AddRectFilledMultiColor(pos, ImVec2(pos.x + size, pos.y + size),
                              IM_COL32(5, 5, 12, 255), IM_COL32(10, 10, 20, 255),
                              IM_COL32(10, 10, 20, 255), IM_COL32(5, 5, 12, 255));
  float cx = pos.x + size * 0.5f;
  float cy = pos.y + size * 0.5f;
  // Globular cluster: dense core of warm stars.
  for (int i = 0; i < 90; ++i) {
    float a = static_cast<float>((i * 97) % 628) / 100.0f;
    float rr = static_cast<float>((i * 61) % 100) / 100.0f;
    float r = size * 0.012f + rr * rr * size * 0.30f;
    dl->AddCircleFilled(ImVec2(cx + cosf(a) * r, cy + sinf(a) * r * 0.9f),
                        1.0f + 2.0f * (1.0f - rr), IM_COL32(255, 236 + ((i * 13) % 17), 210 - (i % 40), 220 - int(150 * rr)));
  }
}

void artColony(ImDrawList* dl, const ImVec2& pos, float size) {
  dl->AddRectFilledMultiColor(pos, ImVec2(pos.x + size, pos.y + size),
                              IM_COL32(4, 6, 12, 255), IM_COL32(6, 10, 18, 255),
                              IM_COL32(6, 10, 18, 255), IM_COL32(4, 6, 12, 255));
  for (int i = 0; i < 10; ++i) {
    float fx = static_cast<float>((i * 47 + 3) % 100) / 100.0f;
    float fy = static_cast<float>((i * 23) % 100) / 100.0f;
    dl->AddCircleFilled(ImVec2(pos.x + fx * size, pos.y + fy * size), 1.2f,
                        IM_COL32(232, 238, 255, 160));
  }
  // Habit planet: blue-green disc + city lights + small moon.
  float cx = pos.x + size * 0.48f;
  float cy = pos.y + size * 0.52f;
  dl->AddCircleFilled(ImVec2(cx, cy), size * 0.30f, IM_COL32(30, 66, 132, 255));
  dl->AddCircleFilled(ImVec2(cx - size * 0.04f, cy - size * 0.03f), size * 0.29f,
                      IM_COL32(46, 96, 150, 255));
  // Continents.
  dl->AddCircleFilled(ImVec2(cx - size * 0.08f, cy + size * 0.05f), size * 0.06f,
                      IM_COL32(96, 150, 110, 130));
  dl->AddCircleFilled(ImVec2(cx + size * 0.10f, cy - size * 0.10f), size * 0.05f,
                      IM_COL32(96, 150, 110, 120));
  dl->AddCircleFilled(ImVec2(cx + size * 0.02f, cy + size * 0.14f), size * 0.04f,
                      IM_COL32(96, 150, 110, 110));
  // Colony lights sprinkled.
  for (int i = 0; i < 12; ++i) {
    float a = static_cast<float>((i * 59) % 628) / 100.0f;
    float rr = size * (0.12f + static_cast<float>((i * 31) % 60) / 1000.0f);
    float lx = cx + cosf(a) * rr;
    float ly = cy + sinf(a) * rr;
    const float d2 = (lx - cx) * (lx - cx) + (ly - cy) * (ly - cy);
    if (d2 < size * size * 0.078f && d2 > size * size * 0.0009f) {
      dl->AddCircleFilled(ImVec2(lx, ly), 1.4f, IM_COL32(255, 220, 150, 230));
    }
  }
  dl->AddCircleFilled(ImVec2(pos.x + size * 0.82f, pos.y + size * 0.22f), size * 0.05f,
                      IM_COL32(180, 184, 196, 220));
}

}  // namespace

namespace astro {

const std::vector<StorySlide>& storyUniverseTourSlides() {
  static const std::vector<StorySlide> slides = [] {
    return std::vector<StorySlide>{
        StorySlide{"", "", 0, "", "res/story/worlds_tour", 30.0}};
  }();
  return slides;
}

const std::vector<StorySlide>& storyIntroSlides() {
  static const std::vector<StorySlide> slides = [] {
    std::vector<StorySlide> s;
    s.push_back(StorySlide{
        "2157 год, приморский город. Мия Сато мечтает о звёздах: вечерами "
        "она указывает на созвездия и говорит - однажды я улечу.",
        "Year 2157, a coastal town. Mia Sato dreams of the stars: at night "
        "she points at the constellations and says - someday I will fly.",
        0});
    s.push_back(StorySlide{
        "Лётный корпус, грохот пусковых, три года за симулятором "
        "варп-двигателя. Мия становится лучшим пилотом курса.",
        "Flight academy, launch-pad rattle, three years of warp-drive "
        "simulators. Mia becomes the best pilot on her course.",
        1});
    s.push_back(StorySlide{
        "Но свет Солнца слабеет. Каждое десятилетие - минус доли процента. "
        "Учёные молчат, а термометры говорят: век - и дом превратится в лёд.",
        "But the light of the Sun is failing. A fraction of a percent every "
        "decade. Officials stay quiet; thermometers say one century - and "
        "home turns to ice.",
        2});
    s.push_back(StorySlide{
        "Города гаснут по очереди. Совет народов принимает невиданное "
        "решение: найти новый мир быстрее, чем спасать старый.",
        "Cities go dark, one after another. The Council of Peoples chooses "
        "the unprecedented: find a new world faster than saving the old one.",
        3});
    s.push_back(StorySlide{
        "Мия возглавила экипаж экспедиции «Астронавигатор-1». На мостике рядом — "
        "Егорка с чертежами асимметричного резонатора и загадочным звёздным кулоном. "
        "В их руках карта прыжков: сверхмассивные дыры, квазары и старые скопления — "
        "все места, где жизнь ещё может удержаться.",
        "Mia leads the crew of the expedition AstroNavigator-1. Beside her on the "
        "bridge is Egorka with the asymmetric cavity schematics and a pulsing star pendant. "
        "In their hands is a jump map: supermassive holes, quasars, old clusters — "
        "every place life might still hold on.",
        4});
    s.push_back(StorySlide{
        "Старт. Позади - Земля. Впереди - вся кривизна пространства-времени.",
        "Launch. Behind them - Earth. Ahead - all the curvature of "
        "spacetime.",
        5});
    s.push_back(StorySlide{
        "Экспедиция начинается с Солнечной системы: Мия и Егорка осмотрят Землю "
        "и проложат курс к центру Галактики. Бортовой журнал открыт — "
        "клавиша J запишет всё.",
        "The expedition begins with the Solar System: Mia and Egorka will inspect Earth "
        "and set course to the center of the Galaxy. The ship's log is open — "
        "key J will record everything.",
        6});
    return s;
  }();
  return slides;
}

const std::vector<StorySlide>& storyChapterSlides() {
  static const std::vector<StorySlide> slides = [] {
    std::vector<StorySlide> s;
    s.push_back(StorySlide{  // 0: earth done
        "Мия: «Мы посмотрели дом и поняли: возвращения может не быть». "
        "Егорка проверяет метрику варп-пузыря и тихо добавляет: «Значит, "
        "сделаем домом всё, до чего дотянемся». Курс проложен к орбитальному причалу: "
        "перед прыжком экипаж прощается с первым флотом.",
        "Mia: \"We looked at our home and understood: there may be no "
        "return.\" Egorka monitors the warp bubble metric and murmurs: "
        "\"Then we'll make a home of whatever we can reach.\" Course set for the orbital pier: "
        "before the jump the crew bids farewell to the first fleet.",
        5});
    s.push_back(StorySlide{  // 1: fleet done - Dragon / Soyuz interlude
        "Мия: «Мы уходим дальше, чем уходил любой экипаж». Командор Марк Хейл встречает "
        "у шлюза: «Вы несёте надежду восьми миллиардов. Если планеты не примут вас — помните: "
        "крепость людей строится на орбите». Доктор Елена Ростова по каналу верфи добавляет: "
        "«Формула Казимира стабильна, Егорка. Если вернётесь с энергией — мы зажжём Солнце заново». "
        "Две эпохи, одна великая цель.",
        "Mia: \"We go farther than any crew before us.\" Cmdr. Mark Hale meets them at the "
        "hatch: \"You carry the hope of eight billion. If alien planets reject you - remember: "
        "humanity's fortress is built in orbit.\" Dr. Elena Rostova radios from the shipyard: "
        "\"The Casimir metric is stable, Egorka. If you return with energy - we can reignite the Sun.\" "
        "Two eras, one great purpose.",
        0, "res/story/06_last_watch.png"});
    s.push_back(StorySlide{  // 2: sgra done
        "Мия: «Свет огибает горизонт — и мы видим его кольцо». Егорка перехватывает "
        "штурвал, когда приливная гравитация тянет корабль: «Не бойся, Мия. "
        "Я держу пузырь». Звёздный кулон резонирует на частоте метрики Керра. "
        "В эфир с рыком пробивается Арсений («Палач Ложных Надежд»): «Куда ты лезешь, смертник?! "
        "Твой варп сомнётся через три секунды! Ложись на мой стазис-вектор, живо!» "
        "Елена передаёт с Земли: «Гравитационное замедление на пределе — вы в миллисекундах от сингулярности!»",
        "Mia: \"Light bends around the horizon - and we see its ring.\" Egorka steeps "
        "the thrusters as tidal drag pulls toward the singularity: \"Don't be afraid, Mia. "
        "I've got the bubble.\" The star pendant resonates at the Kerr metric frequency. "
        "Arseny (\"Executioner of False Hopes\") barks over the comms: \"Where do you think you're going, suicidal fool?! "
        "Your warp field will buckle in three seconds! Align with my stasis vector, now!\" "
        "Elena radios: \"Gravitational dilation is extreme - you are milliseconds from the singularity!\"",
        6});
    s.push_back(StorySlide{  // 3: m87 done
        "Мия: «Доплер не врёт: одна сторона диска ярче другой». Джет M87* "
        "брошен на тысячи световых лет. Егорка смотрит на спектрограф: «Релятивистский "
        "биминг ослепляет сенсоры. Держись за меня, мы разворачиваемся». Марк с орбиты "
        "подбадривает: «Синхронизируйте щиты, держите строй!»",
        "Mia: \"Doppler does not lie: one side of the disk burns brighter.\" "
        "The M87* jet reaches thousands of light-years. Egorka glances at the spectrograph: "
        "\"Relativistic beaming is blinding the sensors. Hold on to me, we're turning.\" Mark "
        "radios from orbit: \"Sync shields, hold the line!\"",
        7});
    s.push_back(StorySlide{  // 4: qso done
        "Мия: «Этот свет шёл к нам ещё до первых динозавров». Егорка достаёт звёздный кулон: "
        "«Два с половиной миллиарда лет... В этой тьме время теряет смысл». "
        "Арсений едко замечает по закрытому каналу: «Время теряет смысл только у романтиков. "
        "Для физики это 2.4 миллиарда лет энтропии. Не расслабляйтесь — впереди M13». "
        "Квазар 3C 273 позади; метрика вакуума полностью откалибрована.",
        "Mia: \"This light traveled to us before the first dinosaurs.\" Egorka reaches for "
        "his star pendant: \"Two and a half billion years... In this void, time loses meaning.\" "
        "Arseny snaps bitterly over encrypted radio: \"Time only loses meaning to romantics. "
        "To physics, this is 2.4 billion years of relentless entropy. Don't slack off — M13 is dead ahead.\" "
        "Quasar 3C 273 behind them; vacuum metric fully calibrated.",
        8});
    s.push_back(StorySlide{  // 5: m13 done - new candidates
        "Мия: «Вот он — второй дом. Но у нас остался один выбор». На канале связи голоса "
        "разделились. Елена: «Вернитесь к Земле, и мы возродим Солнце!» Марк: «Объединяйте флот "
        "в мобильный Ковчег!» Арсений с борта крейсера «Мянь Лун» холодно бросает: «Хватит бегать от энтропии. "
        "Объедините мой стазис-щит с варп-ядром — или погибнете в наивных иллюзиях». "
        "Судьба экспедиции — в руках экипажа.",
        "Mia: \"There it is - a second home. But we have one choice left.\" On the comm link, "
        "voices diverge. Elena: \"Return to Earth and reignite the Sun!\" Mark: \"Unite the fleet "
        "into a mobile Ark Citadel!\" Arseny radios coldly from cruiser \"Mian Long\": \"Stop running from entropy. "
        "Merge my stasis shield with the warp core — or perish in your naive delusions.\" "
        "The fate of the mission rests in the crew's hands.",
        9});
    s.push_back(StorySlide{  // 6: neutral finale fallback (legacy saves)
        "Финал экспедиции: Новому Дому — быть. Мия и Егорка завершили сюжетную арку; "
        "Галактика всё ещё ждёт исследователей.",
        "Expedition finale: the New Home shall be. Mia and Egorka completed the story arc; "
        "the Galaxy is still waiting for explorers.",
        10});
    s.push_back(StorySlide{  // 7: colony ending (branch A)
        "Мия: «Мы остаёмся. Здесь будут дома, сады и дети, которые увидят "
        "настоящее небо». Егорка опускает руку на пульт рядом с её ладонью: «Значит, "
        "наш кочевой варп-пузырь наконец-то причалил». Экипаж «Астронавигатора-1» "
        "основал Новый Дом у старых звёзд M13. Экспедиция завершена — история только начинается.",
        "Mia: \"We stay. There will be homes, gardens and children who see a "
        "real sky.\" Egorka rests his hand beside hers on the console: \"Then our "
        "wandering warp bubble has finally docked.\" The crew of AstroNavigator-1 "
        "founds the New Home by the old stars of M13. The expedition is over - the story is just "
        "beginning.",
        0, "res/story/finale_colony.jpg"});
    s.push_back(StorySlide{  // 8: far horizon ending (branch B)
        "Мия: «Мы не ставим точку. Один прыжок — и за горизонтом будет новый "
        "свет». Егорка с улыбкой переводит варп-привод в овердрайв: «Только ты, я "
        "и бесконечность. Вперёд, штурман». Последнее топливо уходит в двигатель. "
        "«Астронавигатор-1» покидает M13 и уходит к краю Галактики. Колония останется "
        "мечтой; экипаж — легендой.",
        "Mia: \"We do not end the story. One jump - and there will be new "
        "light beyond the horizon.\" Egorka smiles as he dials the warp drive into "
        "overdrive: \"Just you, me, and infinity. Lead on, navigator.\" The last fuel "
        "feeds the drive. AstroNavigator-1 leaves M13 for the edge of the Galaxy. "
        "The colony remains a dream; the crew becomes a legend.",
        0, "res/story/finale_horizon.jpg"});
    s.push_back(StorySlide{  // 9: epilogue - shared legacy
        "Эпилог · Наследие. Спустя годы сигнал «Астронавигатора» достиг "
        "Земли. На орбите, где стоят Dragon и «Союз», вырос новый причал. "
        "На бортовой панели древней станции обнаружена простая гравировка: "
        "«Мия + Егорка = сквозь пространство и время». "
        "Первые корабли больше не летают — но каждый пилот, уходящий к "
        "звёздам, оставляет на их обшивке свой позывной. История продолжается.",
        "Epilogue · Legacy. Years later, the signal of AstroNavigator reached "
        "Earth. A new pier has grown in the orbit where Dragon and Soyuz "
        "stand. On the panel of the ancient station, a simple engraving was found: "
        "\"Mia + Egorka = across spacetime\". "
        "The first ships no longer fly - but every pilot heading for "
        "the stars leaves a call sign on their hull. The story continues.",
        5});
    s.push_back(StorySlide{  // 10: solar rebirth ending (branch C)
        "Д-р Елена Ростова: «Квантовая фаза зафиксирована! Солнце принимает импульс!» "
        "Astro-Voyager выходит на околосолнечную орбиту и производит инжекцию антиматерии "
        "в солнечное ядро через резонатор Казимира. Угасающий багровый гигант вспыхивает "
        "живительным тёплым золотым светом. Земля спасена! В иллюминаторах сияют огни Парижа "
        "и океаны. Человечество остаётся дома — колыбель цивилизации спасена наукой.",
        "Dr. Elena Rostova: \"Quantum phase locked! The Sun is absorbing the pulse!\" "
        "Astro-Voyager enters near-solar orbit and injects antimatter directly into the core "
        "through the Casimir cavity. The dying red giant reignites with warm, golden stellar light. "
        "Earth is saved! Through the canopy, city lights of Paris and blue oceans glow once more. "
        "Humanity stays home - the cradle of civilization is preserved.",
        0, "res/story/finale_solar_rebirth.jpg"});
    s.push_back(StorySlide{  // 11: loop transcendence ending (branch D)
        "Егорка: «Я всё понял... Это был не чужой пришелец. Это был я сам». "
        "На предельной мощности резонатора Astro-Voyager погружается в горизонт событий "
        "сверхмассивной чёрной дыры Sgr A*. Гравитационная сингулярность сжимается, "
        "кулон раскрывает временной разлом. Вспышка синего портала над Сеной в Париже 2026 года: "
        "странник в скафандре передаёт кулон юному Егорке. Временная петля замкнута — "
        "знание передано сквозь поколения.",
        "Egorka: \"I understand now... It wasn't an alien visitor. It was me all along.\" "
        "At maximum warp harmonics, Astro-Voyager dives into the event horizon of Sgr A*. "
        "The gravitational singularity warps space, opening a dimensional fissure. "
        "A flash of a blue portal over the Seine in Paris 2026: a spacefarer passes the pendant "
        "to young Egorka. The temporal loop of the Fifth Universe is closed - knowledge passed on.",
        0, "res/story/finale_loop_transcendence.jpg"});
    s.push_back(StorySlide{  // 12: ark citadel ending (branch E)
        "Командор Марк Хейл: «Шлюзы сцеплены! Монтаж Цитадели начат!» "
        "Astro-Voyager стыкуется с Dragon, «Союзом» и орбитальными модулями верфей. "
        "Вокруг резонатора вырастает колоссальный вращающийся город-мегаполис — «Астро-Ковчег», "
        "не зависящий от капризов умирающих планет. Мия и Егорка встают у руля автономного "
        "флагмана великого космического кочевья. Человечество теперь живёт среди звёзд.",
        "Cmdr. Mark Hale: \"Docking clamps engaged! Citadel construction underway!\" "
        "Astro-Voyager docks with Dragon, Soyuz, and orbital drydocks. Around the Casimir "
        "core grows a colossal rotating metropolis - the \"Astro-Ark\", independent from "
        "the perils of dying worlds. Mia and Egorka take the helm of humanity's grand nomadic fleet.",
        0, "res/story/finale_ark_citadel.jpg"});
    s.push_back(StorySlide{  // 13: hyper-breakthrough ending (branch F - Egorka-6)
        "Егорка-6 («Шестой»): «Петли времени — это клетка для тех, кто боится шагнуть за край! "
        "Включай перегрузку на 135%, я синхронизирую фазовый кристалл!» "
        "Варп-катушки взвыли на запредельной частоте. Ослепительное янтарно-бирюзовое сияние раскололо горизонт: "
        "Astro-Voyager не падает в сингулярность, а пробивает мембрану Мультивселенной! "
        "Кольцо времени разбито. Впереди — бесчисленные миры и чистый горизонт свободы.",
        "Egorka-6 (\"The Sixth\"): \"Time loops are a cage for those afraid to step beyond the edge! "
        "Push the core to 135%, I'll align the phase crystal!\" "
        "Warp coils scream at extreme frequencies. Blinding amber-cyan radiance shatters the horizon: "
        "Astro-Voyager does not fall into the singularity, but punches straight through the multiverse membrane! "
        "The temporal loop is shattered. Ahead lie infinite realities and boundless freedom.",
        0, "res/story/finale_hyper_breakthrough.jpg"});
    s.push_back(StorySlide{  // 14: fifth universe alliance ending (branch G - Arseny)
        "Арсений («Палач Ложных Надежд»): «Твою мать... Коэффициент сошёлся! "
        "Демпфер нулевой кинетики держит перегрузку, варп-пузырь стабилен!» "
        "Обсидиановый крейсер «Мянь Лун» и Astro-Voyager слили щиты в единый каскад. "
        "Мост между Вселенными открыт; миллионы спасены. Арсений с трудом скрывает скупую улыбку: "
        "«Не смей говорить, что это дружба, Егорка. Это математика. А теперь дай мне поспать на моём 'матрасе'».",
        "Arseny (\"Executioner of False Hopes\"): \"Damn it all... The metric coefficients converged! "
        "The zero-kinetic dampener is absorbing the tidal drag, the warp bubble holds firm!\" "
        "The obsidian cruiser \"Mian Long\" and Astro-Voyager merged their shields into a unified cascade. "
        "The inter-universal bridge is anchored; millions are saved. Arseny barely conceals a faint, grim smile: "
        "\"Don't you dare call this friendship, Egorka. It's pure mathematics. Now let me sleep on my so-called 'mattress'\".",
        0, "res/story/finale_fifth_alliance.jpg"});
    return s;
  }();
  return slides;
}

int slideIndexForFinale(int choice) {
  switch (choice) {
    case kFinaleColony: return 7;
    case kFinaleHorizon: return 8;
    case kFinaleSolarRebirth: return 10;
    case kFinaleLoopTranscendence: return 11;
    case kFinaleArkCitadel: return 12;
    case kFinaleHyperBreakthrough: return 13;
    case kFinaleFifthAlliance: return 14;
    default: return 6;
  }
}

// v4.0 prologue of the "New Game / Adventure" slot: Egorka's own path.
// Photos live in res/story/ (jpg/png, stb_image); art slides reuse the table.
const std::vector<StorySlide>& storyPrologueSlides() {
  static const std::vector<StorySlide> slides = [] {
    std::vector<StorySlide> s;
    s.push_back(StorySlide{  // 0: Act 1 - Peaceful Paris & Stargazing Dreams
        "Париж. Мирный осенний вечер на набережной Сены. В окнах мансарды отражаются "
        "тёплые огни кафе «De Paris», а над старыми крышами зажигаются первые звёзды. "
        "Молодой студент политеха Егорка проводил ночи за чертежами и подержанным телескопом: "
        "«Мы не останемся в колыбели навсегда. Наступит день — и человечество шагнёт за край "
        "Солнечной системы». Никто вокруг не воспринимал эти слова всерьёз.",
        "Paris. A peaceful autumn evening on the banks of the Seine. Warm lights of "
        "Cafe de Paris reflect in the attic windows, while the first stars ignite above old rooftops. "
        "Young polytechnic student Egorka spent his nights over schematics and a secondhand telescope: "
        "\"We will not stay in the cradle forever. One day, humanity will step beyond the edge "
        "of the Solar System.\" Nobody around took his words seriously.",
        0, "res/story/paris_peace.jpg"});
    s.push_back(StorySlide{  // 1: Act 2 - The Tear in Spacetime
        "Год спустя. Дождливая ноябрьская ночь на набережной. Воздух внезапно запах "
        "озоном и жжёной медью; уличные фонари затрещали и погасли. Ткань пространства "
        "с сухим громоподобным треском раскололась лазурным порталом: из сияющего вихря "
        "искривлённой метрики шагнул человек в тёмном герметичном комбинезоне со светящимся "
        "хроно-компасом в руке.",
        "A year later. A rain-drenched November night by the Seine. The air suddenly smelled "
        "of ozone and scorched copper; streetlamps flickered and died. Spacetime ruptured "
        "with a dry, thunderous crack into an azure portal: from the swirling vortex of curved "
        "metric stepped a traveler in a dark pressure suit, holding a glowing chrononavigational compass.",
        0, "res/story/01_paris_portal.jpg"});
    s.push_back(StorySlide{  // 2: Act 3 - Handover of the Casimir Key
        "Странник поднял забрало: лицо было старше, изрезано космическим ультрафиолетом, "
        "но это был... он сам. «Слушай внимательно, у меня меньше двух минут. Все ваши теории "
        "варп-привода зашли в тупик, потому что физики ищут экзотическую отрицательную массу. "
        "Её нет в природе. Нужен асимметричный резонатор Казимира: аннигиляция антивещества "
        "создаёт локальный сдвиг квантового вакуума, опуская плотность энергии ниже нуля. "
        "Не толкай корабль, Егорка. Согни само пространство перед ним». Он вложил в ладонь юноши "
        "пульсирующий кулон из гиперсплава.",
        "The traveler raised his visor: the face was older, weathered by cosmic ultraviolet, "
        "yet unmistakably... himself. \"Listen carefully, I have under two minutes. Your warp theories "
        "stalled because physicists hunt for exotic negative mass. It does not exist in nature. "
        "What you need is an asymmetric Casimir cavity: antimatter annihilation creates a localized "
        "quantum vacuum shift, dropping energy density below zero. Don't push the ship, Egorka. "
        "Bend spacetime itself ahead of the bow.\" He pressed a pulsing hyper-alloy pendant into the youth's palm.",
        0, "res/story/02_pendant_handover.jpg"});
    s.push_back(StorySlide{  // 3: Act 4 - The Attic Equations
        "«И ещё: не жди, пока станешь великим академиком. Начинай сейчас, пока ты всё ещё дерзкий "
        "троечник, не знающий слова „невозможно“». Портал схлопнулся, оставив на асфальте круг "
        "оплавленного камня. Месяцы на чердаке под стук парижского дождя превратили интуицию "
        "в строгие тензорные уравнения. На школьной доске родилась математическая модель "
        "локального искривления метрики пространства-времени без парадоксов теории относительности.",
        "\"And one more thing: don't wait until you become an eminent academic. Start now, while "
        "you are still a daring C-student who doesn't believe in the word 'impossible'.\" The rift "
        "collapsed, leaving a ring of vitrified asphalt. Months in the attic under the drum of Parisian "
        "rain forged intuition into rigorous tensor equations. On an old chalkboard, the mathematical "
        "framework for localized spacetime curvature without relativistic paradoxes was born.",
        0, "res/story/03_attic_equation.jpg"});
    s.push_back(StorySlide{  // 4: Act 5 - The Dying Sun & Global Freeze
        "Прошли годы, и пророчество странника сбылось. Спектральная светимость Солнца начала "
        "необратимо падать: ядерный синтез в ядре дал необъяснимый квантовый сбой. Ледники двинулись "
        "на юг, набережные Сены сковало чёрным льдом, а небо затянуло багровой дымкой остывающего "
        "светила. Земля умирала от холода: расчёты климатологов давали цивилизации не более двух "
        "поколений до полного вымерзания биосферы.",
        "Years passed, and the traveler's grim prophecy came to pass. The Sun's spectral output began "
        "an irreversible decay: core fusion suffered an unexplained quantum damping. Glaciers crept "
        "southward, the Seine froze under black ice, and the sky turned into the crimson twilight "
        "of a cooling star. Earth was freezing to death: climatologists gave civilization less than "
        "two generations before the biosphere perished.",
        0, "res/story/paris_burn.jpg"});
    s.push_back(StorySlide{  // 5: Act 6 - The Council & The Heresy
        "На Всемирном Научном Консилиуме царило отчаяние. Главы комиссий предлагали рыть подземные "
        "геотермальные бункеры и консервировать генофонд: «Межзвёздные перелёты на досветовой тяге "
        "займут тысячи лет, а варп-двигатель требует энергии целой галактики». Егорка шагнул к трибуне "
        "и активировал голографический проектор. Формулы Казимира зажглись в воздухе: «Ваша геометрия "
        "статична. С миллиграммами антиматерии в нано-резонаторе мы можем свернуть парсек пространства "
        "в ленту Мёбиуса. Мы полетим к другим звёздам — или замёрзнем здесь».",
        "Despair reigned at the World Science Council. Commission chairs argued for underground "
        "geothermal bunkers and cryogenic vaults: \"Sub-light arks will take millennia, and warp drives "
        "require the mass of an entire galaxy.\" Egorka stepped to the podium and engaged his holographic "
        "projector. Casimir tensors ignited in the air: \"Your geometry is static. With mere milligrams "
        "of antimatter inside a resonant cavity, we can compress a parsec into a Mobius strip. "
        "We reach the stars — or freeze in the dark\".",
        0, "res/story/04_council_crisis.jpg"});
    s.push_back(StorySlide{  // 6: Act 7 - Meeting Test Pilot Mia Sato
        "Идею назвали опасной ересью, но в полночь у ангара космодрома его ждала Мия Сато — легендарный "
        "пилот-испытатель экспериментальной авиации, отстранённая начальством за отказ смириться "
        "с гибелью планеты. Она внимательно изучила трёхмерный расчёт гравитационного градиента: "
        "«Теоретики боятся того, чего не могут пощупать руками. Если твои магнитные катушки удержат пузырь "
        "без приливного разрыва — я подниму эту машину в небо. Мы сделаем это вместе».",
        "The council branded the theory dangerous heresy, but at midnight near the spaceport hangars, "
        "Mia Sato waited for him. The elite test pilot, grounded by command for refusing to accept "
        "extinction, studied the 3D gravitational gradient: \"Theoreticians fear what they cannot "
        "touch with their hands. If your magnetic coils hold the bubble without tidal shear — I will fly "
        "this machine to the stars. We will do this together\".",
        0, "res/story/05_mia_hangar.jpg"});
    s.push_back(StorySlide{  // 7: Act 8 - The Simulator & Steering Silence
        "Полгода каторжных тренировок в шестиосевом варп-симуляторе. Перегрузки в центрифуге "
        "сменялись леденящей невесомостью. Мия учила Егорку слушать отклик привода кончиками пальцев: "
        "«Варп-корабль не толкает газы назад, как ракета. Он скользит по искусственному наклону метрики. "
        "Ты управляешь не тягой, Егорка, а абсолютной тишиной. Одно неверное движение интерферометра — "
        "и приливные силы разорвут корпус на атомы». На тринадцатой симуляции траектория легла в идеальный коридор.",
        "Six months of grueling training in the six-axis warp simulator. Centrifuge g-forces alternated "
        "with chilling zero-g. Mia taught Egorka to read drive harmonics through fingertips: \"A warp ship "
        "does not push reaction mass like a rocket. It surfs an artificial slope of metric curvature. "
        "You don't steer thrust, Egorka — you steer absolute silence. One twitch on the interferometer, "
        "and tidal gradients will tear the hull into atoms.\" On the thirteenth run, their trajectory locked "
        "into a flawless corridor.",
        0, "res/story/05_egorka_cockpit.png"});
    s.push_back(StorySlide{  // 8: Act 9 - Orbital Shipyard Construction
        "На околоземной верфи ожили исполинские стапели. Лучшие инженеры, поверившие в проект, "
        "работали в три смены в открытом космосе. Снопы плазменных сварочных дуг озаряли тёмную сторону "
        "планеты: вокруг композитного корпуса «Астронавигатора» монтировались титановые тороидальные кольца "
        "генераторов Алькубьерре. В криогенные ловушки бережно закачивали первые граммы позитронов и антипротонов.",
        "At the orbital shipyard, colossal construction gantries roared to life. Humanity's finest "
        "engineers who believed in the dream worked around the clock in hard vacuum. Plasma welding arcs "
        "illuminated Earth's night side: dual titanium toroidal Alcubierre rings were anchored around "
        "Astro-Voyager's composite hull. The first precious grams of positrons and antiprotons were "
        "chilled and loaded into superconducting magnetic traps.",
        0, "res/story/06_shipyard_assembly.jpg"});
    s.push_back(StorySlide{  // 9: Act 10 - The Last Watch & Fleet Farewell
        "Перед генеральным пуском корабль ошвартовался у исторического причала Международной станции. "
        "У шлюза их встретили ветераны космонавтики — командор Марк Хейл и доктор Елена Ростова. "
        "Марк положил руку на шлем Мии: «Восемь миллиардов душ смотрят сегодня на ваши варп-кольца. "
        "Если найдёте новый мир у далёких звёзд — зажгите маяк. Если нет — знайте, что вы дали нам "
        "право гордиться тем, что мы люди». За бронестеклом сияла беззащитная голубая колыбель Земли.",
        "Before final departure, the vessel docked at the historic berth of the International Station. "
        "Spacefarer veterans Cmdr. Mark Hale and Dr. Elena Rostova greeted them at the airlock. Mark placed "
        "a hand on Mia's helmet: \"Eight billion souls are gazing up at your warp rings tonight. "
        "If you find a new home among distant suns — light the beacon. If you don't — know that you gave "
        "humanity the right to die proud of who we were.\" Beyond the viewport glowed the fragile blue cradle of Earth.",
        0, "res/story/06_last_watch.png"});
    s.push_back(StorySlide{  // 10: Act 11 - Awakening the Singularity Core
        "«Астронавигатор-1» отдал швартовы и вышел на исходную точку над экватором. Магнитные захваты "
        "расцепились. В глубине машинного отсека взвыли инжекторы: асимметричный резонатор Казимира замкнул "
        "квантовый контур. Двойные кольца вспыхнули ослепительным фиолетово-лазурным черенковским сиянием "
        "антивещества. Плотность энергии на внешней кромке упала ниже нуля: пространство вокруг корабля "
        "зарябило, как воздух над раскалённой пустыней.",
        "AstroNavigator-1 cast off her moorings and drifted to the departure coordinates above the equator. "
        "Docking clamps released. Deep within the engineering deck, the injectors screamed: the asymmetric "
        "Casimir cavity locked its quantum circuit. The twin rings flared with blinding violet-azure Cherenkov "
        "radiance. Energy density along the outer boundary plunged below zero: space around the ship rippled "
        "like heat mirages above a desert.",
        0, "res/story/07_ship_orbit.jpg"});
    s.push_back(StorySlide{  // 11: Act 12 - On the Command Bridge
        "Панорамный мостик. В огромном фонаре кабины тихо тает серебряный серп Земли. Мия перевела "
        "маневровые контуры в боевой режим, её пальцы лежали на рычагах пространственной геометрии: "
        "«Резонатор синхронизирован на 99.8%. Метрика стабильна. Курс проложен к сверхмассивной чёрной дыре "
        "в сердце Галактики». Егорка коснулся звёздного кулона на груди и улыбнулся: «Автопилот выключен, "
        "капитан. Мы не стали ждать, пока станем великими. Пора».",
        "The panoramic command bridge. In the sprawling canopy, the silver crescent of Earth silently receded. "
        "Mia engaged the spatial thrusters, fingers firm on the metric control yokes: \"Cavity synchronized "
        "at 99.8%. Bubble geometry stable. Trajectory locked toward the supermassive black hole at the Galactic Core.\" "
        "Egorka touched the star pendant on his chest and smiled: \"Autopilot off, Captain. We didn't wait until "
        "we were great. It's time\".",
        0, "res/story/08_bridge_departure.jpg"});
    s.push_back(StorySlide{  // 12: Act 13 - Warp Factor 9: Folding the Void
        "Варп-фактор 9! Корабль не сдвинулся с места в своей локальной системе отсчёта — пространство "
        "перед носом сжалось в гармошку, а позади расширилось с немыслимой скоростью. Ослепительная квантовая "
        "вспышка сорвала темноту; видимый спектр звёзд растянулся в радужные релятивистские кольца и лазурные "
        "нити. «Астронавигатор-1» преодолел световой барьер и устремился в неизведанную Вселенную!",
        "Warp Factor 9! The ship did not move within its local reference frame — instead, spacetime ahead "
        "collapsed into origami folds while expanding behind at unimaginable velocity. A blinding Cherenkov "
        "shockwave tore the void; the visible starfield bent into kaleidoscopic relativistic rings and azure "
        "threads. AstroNavigator-1 shattered the speed of light and plunged into the unknown cosmos!",
        0, "res/story/09_warp_jump_cinema.jpg"});
    s.push_back(StorySlide{  // 13: Act 14 - The Hyperspace Flight Chrysalis
        "Разрывая пустоту, Astro-Voyager несётся сквозь искривлённый гиперпространственный коридор. "
        "Гравитационные кольца ревут на пике квантовой мощности: солнечная колыбель осталась позади, "
        "впереди — гравитационные бездны, квазары и надежда на спасение человечества.",
        "Tearing through the void, Astro-Voyager hurtles through the warped hyperspace corridor. "
        "Gravitational rings roar at peak quantum harmonics: the solar cradle lies far behind, ahead "
        "lie gravitational chasms, quasars, and the salvation of mankind.",
        0, "", "res/story/warp_flight-frames"});
    s.push_back(StorySlide{  // 14: Act 15 - Arrival at the Sol Gateway
        "Выход из варпа на рубежах Солнечной системы. Метрика выравнивается, звёздные нити вновь "
        "обретают покой. Перед экипажем открывается панорама родного дома и орбитальной станции: "
        "Мие и Егорке предстоит провести финальную диагностику систем и проложить вектор к центру "
        "Галактики. Бортовой журнал открыт — клавиша J запишет каждый шаг экспедиции!",
        "Dropout from warp at the frontier of the Solar System. The metric flattens, stellar threads "
        "settle into tranquil points of light. Before the crew stretches the panorama of home and "
        "the orbital gateway: Mia and Egorka must run final system diagnostics and chart the vector "
        "to the Galactic Core. The mission log is open — key J will record every step of the expedition!",
        6});
    return s;
  }();
  return slides;
}

void drawStoryArt(ImDrawList* dl, int art, const ImVec2& pos, float size) {
  if (!dl || size <= 0.0f) return;
  switch (art) {
    case 0: artHouseWindow(dl, pos, size); break;
    case 1: artToyRocket(dl, pos, size); break;
    case 2: artDimmingSun(dl, pos, size); break;
    case 3: artCrisis(dl, pos, size); break;
    case 4: artLaunchPad(dl, pos, size); break;
    case 5: artDeparture(dl, pos, size); break;
    case 6: artPhotonRing(dl, pos, size); break;
    case 7: artJet(dl, pos, size); break;
    case 8: artQuasar(dl, pos, size); break;
    case 9: artCluster(dl, pos, size); break;
    case 10: artColony(dl, pos, size); break;
    default: artGroundAndStars(dl, pos, size); break;
  }
}

}  // namespace astro
