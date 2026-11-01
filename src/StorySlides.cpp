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
        "Елена передаёт с Земли: «Гравитационное замедление на пределе — вы в миллисекундах от сингулярности!»",
        "Mia: \"Light bends around the horizon - and we see its ring.\" Egorka steeps "
        "the thrusters as tidal drag pulls toward the singularity: \"Don't be afraid, Mia. "
        "I've got the bubble.\" The star pendant resonates at the Kerr metric frequency. "
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
        "«Два с половиной миллиарда лет... В этой тьме время теряет смысл. Главное — "
        "кто сидит в кресле рядом с тобой». Квазар 3C 273 позади; метрика вакуума полностью откалибрована.",
        "Mia: \"This light traveled to us before the first dinosaurs.\" Egorka reaches for "
        "his star pendant: \"Two and a half billion years... In this void, time loses meaning. All that "
        "matters is who sits in the seat beside you.\" Quasar 3C 273 behind them; vacuum metric fully calibrated.",
        8});
    s.push_back(StorySlide{  // 5: m13 done - new candidates
        "Мия: «Вот он — второй дом. Но у нас остался один выбор». На канале связи голоса "
        "разделились. Елена: «Вернитесь к Земле, и мы возродим Солнце!» Марк: «Объединяйте флот "
        "в мобильный Ковчег!» Мия смотрит на звёзды M13, а Егорка сжимает кулон. "
        "Судьба экспедиции — в руках экипажа.",
        "Mia: \"There it is - a second home. But we have one choice left.\" On the comm link, "
        "voices diverge. Elena: \"Return to Earth and reignite the Sun!\" Mark: \"Unite the fleet "
        "into a mobile Ark Citadel!\" Mia looks to M13's stars, while Egorka holds his pendant. "
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
        10});
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
        5});
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
        2});
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
        6});
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
        10});
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
    default: return 6;
  }
}

// v4.0 prologue of the "New Game / Adventure" slot: Egorka's own path.
// v4.0 prologue of the "New Game / Adventure" slot: Egorka's own path.
// Photos live in res/story/ (jpg/png, stb_image); art slides reuse the table.
const std::vector<StorySlide>& storyPrologueSlides() {
  static const std::vector<StorySlide> slides = [] {
    std::vector<StorySlide> s;
    s.push_back(StorySlide{  // 0: The Encounter in Paris
        "Париж. Дождливый вечер на набережной Сены. Пространство с треском "
        "вскрылось синим порталом: навстречу шагнул странник в звёздном плаще — "
        "Егорка из Пятой Вселенной с пульсирующей сингулярностью в руке.",
        "Paris. A rainy night by the Seine. Spacetime tore open with a crackling "
        "blue portal: a traveler in a starlight cloak stepped forward — older "
        "Egorka from the Fifth Universe, holding a pulsing singularity.",
        0, "res/story/01_paris_portal.jpg"});
    s.push_back(StorySlide{  // 1: The Secret of Antimatter
        "«Ваш варп-проект застрял, потому что ищут отрицательную массу. "
        "Нужен асимметричный резонатор Казимира: аннигиляция антиматерии создаёт "
        "когерентный сдвиг вакуума, локально опуская плотность энергии ниже нуля. "
        "Не антигравитация, Егорка. Инженерия метрики».",
        "\"Your warp project stalled hunting for negative mass. You need an "
        "asymmetric Casimir cavity: antimatter annihilation creates a coherent "
        "vacuum shift, driving energy density below zero. Not antigravity, "
        "Egorka. Spacetime engineering\".",
        0, "res/story/02_pendant_handover.jpg"});
    s.push_back(StorySlide{  // 2: Decoding the Metric in the Attic
        "«Портал нестабилен — у меня две минуты. Не жди, пока станешь великим. "
        "Начни, пока ты всё ещё троечник с несданной домашкой. Именно такие "
        "люди уводят корабли за горизонт... И да: выключи автопилот, он ужасно зануден». "
        "Ночи на чердаке под шум дождя превратили фантастику в уравнения.",
        "\"The portal is unstable — I have two minutes. Don't wait until you "
        "are great. Start while you are still a C-student with overdue homework. "
        "Those are the ones who steer ships beyond horizons... And turn off "
        "the autopilot, it is dreadfully boring\". Nights in the attic under rain turned fantasy into equations.",
        0, "res/story/03_attic_equation.jpg"});
    s.push_back(StorySlide{  // 3: The Crisis & The Global Scientific Council
        "Годы спустя. Солнце тускнело, покрывая планету багровой тенью холода. "
        "Совет учёных объявил варп невозможным: без отрицательной массы человечество обречено замерзнуть. "
        "Егорка вышел к трибуне и зажёг проекцию: «Масса положительна. Но плотность вакуума поддаётся кривизне».",
        "Years later. The Sun dimmed, casting a crimson chill over the continents. "
        "The Science Council declared interstellar flight impossible: without negative mass, humanity was doomed to freeze. "
        "Egorka stepped to the podium and lit the projection: \"The mass is positive. But vacuum density yields to curvature\".",
        0, "res/story/04_council_crisis.jpg"});
    s.push_back(StorySlide{  // 4: Meeting Mia Sato in the Hangar
        "В пустом ангаре космодрома его ждала Мия Сато — лучший пилот-испытатель корпуса. "
        "Она смотрела на звёздную голограмму в его руке: «Все называют тебя сумасшедшим. "
        "Но если твой резонатор удержит геометрию пузыря — я выведу нас в открытый космос. Мы сделаем это вместе».",
        "In the empty hangar, Mia Sato awaited him — the fleet's finest test pilot. "
        "She gazed at the stellar hologram in his hand: \"Everyone calls you mad. "
        "But if your cavity holds the bubble geometry — I will fly us through the horizon. We do this together\".",
        0, "res/story/05_mia_hangar.jpg"});
    s.push_back(StorySlide{  // 5: Orbital Shipyard Assembly
        "На околоземной верфи в невесомости ожили титановые стапели. Снопы плазменных искр "
        "озаряли ночную сторону Земли: инженеры монтировали сдвоенные кольца резонатора "
        "вокруг корпуса Astro-Voyager, заряжая магнитные ловушки антивеществом.",
        "In the zero-gravity orbital shipyard, titanium gantries roared to life. Cascades of plasma sparks "
        "lit Earth's night side: engineers mounted the dual resonator rings "
        "around Astro-Voyager's hull, charging magnetic traps with antimatter.",
        0, "res/story/06_shipyard_assembly.jpg"});
    s.push_back(StorySlide{  // 6: Astro-Voyager Awakens in Orbit
        "Колосс пробудился. Двойные тороидальные кольца варп-двигателя вспыхнули "
        "фиолетовой плазмой антиматерии. Отрицательная плотность энергии зафиксирована. "
        "Впервые в истории корабль стал готов согнуть пространство перед собой.",
        "The colossus awoke. The dual toroidal warp rings flared with "
        "violet antimatter plasma. Negative energy density secured. "
        "For the first time in history, a ship was ready to bend spacetime ahead of its bow.",
        0, "res/story/07_ship_orbit.jpg"});
    s.push_back(StorySlide{  // 7: Command Bridge Departure
        "Панорамный мостик. В огромном фонаре кабины тихо сияет полумесяц Земли. "
        "Мия переводит маневровые в боевой режим: «Сингулярный резонатор на 99.8% мощности. Курс за пределы системы». "
        "Егорка касается кулона на груди: «Мы не стали ждать, пока станем великими. Пора».",
        "The panoramic bridge. In the canopy, Earth's crescent glows in quiet silence. "
        "Mia switches the drives to standby: \"Singularity cavity locked at 99.8%. Trajectory set beyond the system\". "
        "Egorka touches the pendant on his chest: \"We didn't wait until we were great. It's time\".",
        0, "res/story/08_bridge_departure.jpg"});
    s.push_back(StorySlide{  // 8: Warp Factor 9 - Spacetime Jump (Ship Flies Away)
        "Варп-фактор 9. Корабль не летит — он договаривается с пространством, "
        "чтобы расстояние перестало быть наглым. Вспышка сине-фиолетовой плазмы, метрика сминается в гармошку, "
        "и звёзды вытягиваются в синие нити. «Астронавигатор-1» уходит в неизведанную Вселенную!",
        "Warp factor 9. The ship doesn't fly — it negotiates with spacetime "
        "so distance stops being insolent. A blinding Cherenkov flare, space folds like origami, "
        "and stars stretch into brilliant azure threads. AstroNavigator-1 hurtles into the unknown!",
        0, "res/story/09_warp_jump_cinema.jpg"});
    s.push_back(StorySlide{  // 9: Cinematic Warp Flight Animation
        "Разрывая пустоту, Astro-Voyager устремляется сквозь искривлённое "
        "пространство. Гравитационные кольца ревут на пике мощности: "
        "горизонт событий позади, впереди — неизведанные глубины космоса.",
        "Tearing through the void, the Astro-Voyager hurtles across warped "
        "space. Gravitational rings roar at peak power: the event horizon "
        "behind, the uncharted depths of the cosmos ahead.",
        0, "", "res/story/warp_flight-frames"});
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
