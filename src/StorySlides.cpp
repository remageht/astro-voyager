#include "StorySlides.h"

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
        "Мия вошла в первый экипаж экспедиции «Астронавигатор-1». В её "
        "руках карта прыжков: сверхмассивные дыры, квазары и старые "
        "скопления - все места, где жизнь ещё может удержаться.",
        "Mia joins the first crew of the expedition AstroNavigator-1. Her "
        "jump map: supermassive holes, quasars, old clusters - every place "
        "life might still hold on.",
        4});
    s.push_back(StorySlide{
        "Старт. Позади - Земля. Впереди - вся кривизна пространства-времени.",
        "Launch. Behind them - Earth. Ahead - all the curvature of "
        "spacetime.",
        5});
    s.push_back(StorySlide{
        "Экспедиция начинается с Солнечной системы: экипаж осмотрит Землю "
        "и проложит курс к центру Галактики. Бортовой журнал открыт - "
        "клавиша J запишет всё.",
        "The expedition starts with the Solar System: the crew will look "
        "at the Earth and set course to the center of the Galaxy. The "
        "ship's log is open - key J will record everything.",
        6});
    return s;
  }();
  return slides;
}

const std::vector<StorySlide>& storyChapterSlides() {
  static const std::vector<StorySlide> slides = [] {
    std::vector<StorySlide> s;
    s.push_back(StorySlide{  // 0: earth done
        "Экипаж осмотрел дом и принял решение: возвращения может не быть. "
        "Курс проложен к центру Галактики.",
        "The crew has looked at its home and made the choice: there may be "
        "no return. Course set to the center of the Galaxy.",
        5});
    s.push_back(StorySlide{  // 1: sgra done
        "Sgr A*: фотонное кольцо запечатлено, орбиты света вокруг горизонта "
        "измерены. Следующая цель - M87*.",
        "Sgr A*: the photon ring is recorded, light orbits around the "
        "horizon measured. Next target - M87*.",
        6});
    s.push_back(StorySlide{  // 2: m87 done
        "Джет M87* брошен на тысячи световых лет. Доплер подтвердил: одна "
        "сторона диска светит ярче другой.",
        "The M87* jet reaches thousands of light-years. Doppler confirmed: "
        "one side of the disk burns brighter than the other.",
        7});
    s.push_back(StorySlide{  // 3: qso done
        "3C 273: ядро квазара ярче целой галактики. Его свет шёл к нам "
        "2.4 миллиарда лет.",
        "3C 273: the quasar core outshines an entire galaxy. Its light "
        "traveled 2.4 billion years to reach us.",
        8});
    s.push_back(StorySlide{  // 4: m13 done - new candidates
        "Скопление M13: светила вдвое старше Солнца. Выбран мир для "
        "колонии - вторая Земля найдена.",
        "Cluster M13: stars twice as old as the Sun. A world for the "
        "colony is chosen - a second Earth is found.",
        9});
    s.push_back(StorySlide{  // 5: finale
        "Финал экспедиции: Новому Дому - быть. Сюжетная арка завершена; "
        "Галактика всё ещё ждёт исследователей.",
        "Expedition finale: the New Home shall be. The story arc is "
        "complete; the Galaxy is still waiting for explorers.",
        10});
    return s;
  }();
  return slides;
}

// v4.0 prologue of the "New Game / Adventure" slot: Yegorka's own path.
// v4.0 prologue of the "New Game / Adventure" slot: Yegorka's own path.
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
        "Yegorka from the Fifth Universe, holding a pulsing singularity.",
        0, "res/story/01_paris_portal.jpg"});
    s.push_back(StorySlide{  // 1: The Secret of Antimatter
        "\"Не ищи отрицательную массу — используй фазовый сдвиг вакуума при "
        "аннигиляции антиматерии в резонаторе Казимира\". Сияющий кулон-звезда "
        "и кристалл сингулярности легли в дрожащие руки школьника.",
        "\"Do not seek negative mass — use the vacuum phase shift of antimatter "
        "annihilation inside a Casimir cavity\". The radiant star pendant and "
        "the singularity crystal settled into the schoolboy's trembling hands.",
        0, "res/story/02_pendant_handover.jpg"});
    s.push_back(StorySlide{  // 2: Decoding the Metric in the Attic
        "Ночами в парижской мансарде звёздный кулон проецировал голограммы "
        "уравнений метрики Алькубьерре. То, над чем бились институты "
        "десятилетиями, сошлось в одной тетради: искривление пространства возможно.",
        "Nights in a Parisian attic, the star pendant projected holographic "
        "equations of the Alcubierre metric. What institutions struggled over "
        "for decades converged in a single notebook: spacetime warp is real.",
        0, "res/story/03_attic_equation.jpg"});
    s.push_back(StorySlide{  // 3: Astro-Voyager in Orbit
        "На орбитальной верфи ожил колосс — корабль Astro-Voyager. "
        "Двойные тороидальные кольца варп-двигателя вспыхнули фиолетовой "
        "плазмой антивещества, деформируя ткань Вселенной.",
        "At the orbital shipyard, the colossus awoke — the Astro-Voyager. "
        "The dual toroidal warp rings flared with violet antimatter plasma, "
        "bending the very fabric of spacetime.",
        0, "res/story/04_ship_orbit.jpg"});
    s.push_back(StorySlide{  // 4: EgorKa at the Helm - The First Jump
        "Егорка сжал штурвал корабля. На груди мерцает звёздный кулон, в "
        "панорамном окне пространство схлопывается в релятивистский тоннель. "
        "Первый межзвёздный прыжок начат — курс на неизведанный космос.",
        "Yegorka gripped the ship's control yoke. The star pendant gleamed on "
        "his chest, and through the canopy, spacetime collapsed into a "
        "relativistic warp tunnel. The first interstellar jump begins.",
        0, "res/story/05_egorka_cockpit.png"});
    s.push_back(StorySlide{  // 5: Cinematic Warp Flight
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
