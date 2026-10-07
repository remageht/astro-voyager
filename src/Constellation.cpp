#include "Constellation.h"

#include <cmath>

namespace astro {

namespace {

void projectStars(ConstellationInfo& c, float scale) {
  const float kPi = 3.14159265358979323846f;
  const float cosDec0 = std::cos(c.centerDec * kPi / 180.0f);
  for (auto& s : c.stars) {
    float deltaRa = (s.ra - c.centerRa) * cosDec0;
    float deltaDec = s.dec - c.centerDec;
    s.x = -deltaRa * scale;
    s.y = deltaDec * scale;
    s.z = 0.0f;
  }
}

std::vector<ConstellationInfo> initConstellations() {
  std::vector<ConstellationInfo> list;

  // 1. Orion (ori)
  ConstellationInfo ori;
  ori.id = "ori";
  ori.name = "Orion";
  ori.centerRa = 83.5f;
  ori.centerDec = 0.0f;
  ori.stars = {
      {"Betelgeuse", "alpha Ori", 27989, 88.79f, 7.41f, 0.42f, 0, 0, 0},
      {"Rigel", "beta Ori", 24436, 78.63f, -8.20f, 0.18f, 0, 0, 0},
      {"Bellatrix", "gamma Ori", 25336, 81.28f, 6.35f, 1.64f, 0, 0, 0},
      {"Mintaka", "delta Ori", 25930, 83.00f, -0.30f, 2.23f, 0, 0, 0},
      {"Alnilam", "epsilon Ori", 26311, 84.05f, -1.20f, 1.69f, 0, 0, 0},
      {"Alnitak", "zeta Ori", 26727, 85.19f, -1.94f, 1.74f, 0, 0, 0},
      {"Saiph", "kappa Ori", 27366, 86.94f, -9.67f, 2.07f, 0, 0, 0},
      {"Meissa", "lambda Ori", 26207, 83.78f, 9.93f, 3.39f, 0, 0, 0},
      {"Hatsya", "iota Ori", 26241, 83.86f, -5.91f, 2.75f, 0, 0, 0},
      {"Theta Ori", "theta1 Ori", 26221, 83.82f, -5.39f, 3.70f, 0, 0, 0},
      {"Tabit", "pi3 Ori", 23875, 73.66f, 6.96f, 3.16f, 0, 0, 0},
      {"pi4 Ori", "pi4 Ori", 24197, 74.75f, 5.61f, 3.69f, 0, 0, 0},
      {"pi5 Ori", "pi5 Ori", 24483, 75.76f, 2.44f, 3.72f, 0, 0, 0},
      {"pi1 Ori", "pi1 Ori", 23123, 71.30f, 10.15f, 4.66f, 0, 0, 0},
      {"pi2 Ori", "pi2 Ori", 23416, 72.19f, 8.90f, 4.36f, 0, 0, 0},
      {"pi6 Ori", "pi6 Ori", 24674, 76.47f, 1.71f, 4.47f, 0, 0, 0},
      {"Saif al Jabbar", "eta Ori", 25281, 81.08f, -2.39f, 3.38f, 0, 0, 0},
      {"tau Ori", "tau Ori", 25028, 80.26f, -6.84f, 3.59f, 0, 0, 0},
      {"sigma Ori", "sigma Ori", 26549, 84.69f, -2.60f, 3.77f, 0, 0, 0},
      {"29 Ori", "e Ori", 27364, 86.93f, -7.81f, 4.13f, 0, 0, 0},
      {"nu Ori", "nu Ori", 29426, 91.95f, 14.77f, 4.42f, 0, 0, 0},
      {"xi Ori", "xi Ori", 29038, 90.66f, 14.21f, 4.45f, 0, 0, 0},
      {"chi1 Ori", "chi1 Ori", 27913, 88.54f, 20.28f, 4.39f, 0, 0, 0},
      {"chi2 Ori", "chi2 Ori", 28716, 89.65f, 20.14f, 4.64f, 0, 0, 0}
  };
  // Exactly 9 canonical segments:
  ori.segments = {
      {0, 2}, // Betelgeuse - Bellatrix (shoulders)
      {0, 7}, // Betelgeuse - Meissa (shoulder to head)
      {2, 7}, // Bellatrix - Meissa (shoulder to head)
      {0, 5}, // Betelgeuse - Alnitak (shoulder to belt)
      {2, 3}, // Bellatrix - Mintaka (shoulder to belt)
      {3, 4}, // Mintaka - Alnilam (belt 1)
      {4, 5}, // Alnilam - Alnitak (belt 2)
      {5, 6}, // Alnitak - Saiph (belt to left leg)
      {3, 1}  // Mintaka - Rigel (belt to right leg)
  };
  projectStars(ori, 0.9f);
  list.push_back(ori);

  // 2. Ursa Major (uma)
  ConstellationInfo uma;
  uma.id = "uma";
  uma.name = "Ursa Major";
  uma.centerRa = 186.0f;
  uma.centerDec = 55.0f;
  uma.stars = {
      {"Dubhe", "alpha UMa", 54061, 165.93f, 61.75f, 1.79f, 0, 0, 0},
      {"Merak", "beta UMa", 53910, 165.46f, 56.38f, 2.37f, 0, 0, 0},
      {"Phecda", "gamma UMa", 58001, 178.46f, 53.69f, 2.44f, 0, 0, 0},
      {"Megrez", "delta UMa", 59774, 183.86f, 57.03f, 3.31f, 0, 0, 0},
      {"Alioth", "epsilon UMa", 62956, 193.51f, 55.96f, 1.77f, 0, 0, 0},
      {"Mizar", "zeta UMa", 65378, 200.98f, 54.92f, 2.23f, 0, 0, 0},
      {"Alkaid", "eta UMa", 67301, 206.88f, 49.31f, 1.86f, 0, 0, 0},
      {"Alcor", "80 UMa", 65477, 201.30f, 54.99f, 3.99f, 0, 0, 0},
      {"Muscida", "omicron UMa", 41704, 127.56f, 60.72f, 3.35f, 0, 0, 0},
      {"Talitha", "iota UMa", 44127, 134.87f, 48.04f, 3.14f, 0, 0, 0},
      {"Tania Borealis", "lambda UMa", 50372, 154.27f, 42.91f, 3.45f, 0, 0, 0},
      {"Tania Australis", "mu UMa", 50583, 155.08f, 41.50f, 3.06f, 0, 0, 0},
      {"Alula Borealis", "nu UMa", 55203, 169.55f, 33.09f, 3.49f, 0, 0, 0},
      {"Alula Australis", "xi UMa", 55219, 169.61f, 31.53f, 3.79f, 0, 0, 0},
      {"theta UMa", "theta UMa", 46853, 143.25f, 51.68f, 3.17f, 0, 0, 0},
      {"psi UMa", "psi UMa", 54682, 167.75f, 44.50f, 3.01f, 0, 0, 0},
      {"chi UMa", "chi UMa", 57926, 178.13f, 47.78f, 3.71f, 0, 0, 0},
      {"23 UMa", "23 UMa", 47854, 146.40f, 63.06f, 3.65f, 0, 0, 0},
      {"upsilon UMa", "upsilon UMa", 48202, 147.50f, 59.04f, 3.78f, 0, 0, 0},
      {"phi UMa", "phi UMa", 48402, 148.16f, 54.06f, 4.56f, 0, 0, 0}
  };
  // Exactly 7 canonical segments:
  uma.segments = {
      {0, 1}, // Dubhe - Merak
      {1, 2}, // Merak - Phecda
      {2, 3}, // Phecda - Megrez
      {3, 0}, // Megrez - Dubhe
      {3, 4}, // Megrez - Alioth
      {4, 5}, // Alioth - Mizar
      {5, 6}  // Mizar - Alkaid
  };
  projectStars(uma, 0.85f);
  list.push_back(uma);

  return list;
}

}  // namespace

const std::vector<ConstellationInfo>& builtinConstellations() {
  static const std::vector<ConstellationInfo> kConstellations = initConstellations();
  return kConstellations;
}

const ConstellationInfo* findConstellation(const std::string& id) {
  for (const auto& c : builtinConstellations()) {
    if (c.id == id) return &c;
  }
  return nullptr;
}

}  // namespace astro
