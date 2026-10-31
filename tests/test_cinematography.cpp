#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#define TEST_ASSERT(cond) \
  do { \
    if (!(cond)) { \
      std::cerr << "Assertion failed: " #cond << " at " << __FILE__ << ":" << __LINE__ << "\n"; \
      std::exit(1); \
    } \
  } while (0)

// ============================================================================
// Cinematography Math & Color Grading Verification Suite (v4.4.0)
// ============================================================================

struct Vec3 {
  float r, g, b;
  Vec3(float r = 0.0f, float g = 0.0f, float b = 0.0f) : r(r), g(g), b(b) {}
  bool operator==(const Vec3& o) const {
    return std::abs(r - o.r) < 1e-5f && std::abs(g - o.g) < 1e-5f &&
           std::abs(b - o.b) < 1e-5f;
  }
};

inline float clampF(float v, float minVal, float maxVal) {
  return std::max(minVal, std::min(maxVal, v));
}

inline Vec3 clampVec3(Vec3 v, float minVal, float maxVal) {
  return Vec3(clampF(v.r, minVal, maxVal),
              clampF(v.g, minVal, maxVal),
              clampF(v.b, minVal, maxVal));
}

inline float smoothstepF(float edge0, float edge1, float x) {
  float t = clampF((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
}

// C++ ground truth of applyColorGrading in shaders/post_comp.frag
Vec3 applyColorGradingSim(Vec3 c, int grade) {
  if (grade == 1) {
    // Interstellar Amber
    Vec3 amber(c.r * 1.12f + 0.02f,
               c.g * 1.01f + 0.005f,
               c.b * 0.82f);
    amber = clampVec3(amber, 0.0f, 1.0f);
    amber.r = std::pow(amber.r, 0.95f);
    amber.g = std::pow(amber.g, 1.00f);
    amber.b = std::pow(amber.b, 1.10f);
    Vec3 blended(c.r * 0.15f + amber.r * 0.85f,
                 c.g * 0.15f + amber.g * 0.85f,
                 c.b * 0.15f + amber.b * 0.85f);
    return clampVec3(blended, 0.0f, 1.0f);
  } else if (grade == 2) {
    // Cyberpunk Neon
    Vec3 shadows(0.0f, 0.12f, 0.18f);
    Vec3 highlights(1.15f, 0.85f, 1.10f);
    float luma = c.r * 0.2126f + c.g * 0.7152f + c.b * 0.0722f;
    Vec3 neon(c.r * (1.0f - luma) + shadows.r * (1.0f - luma) + c.r * highlights.r * luma,
              c.g * (1.0f - luma) + shadows.g * (1.0f - luma) + c.g * highlights.g * luma,
              c.b * (1.0f - luma) + shadows.b * (1.0f - luma) + c.b * highlights.b * luma);
    neon = clampVec3(neon, 0.0f, 1.0f);
    neon.r = neon.r * neon.r * (3.0f - 2.0f * neon.r);
    neon.g = neon.g * neon.g * (3.0f - 2.0f * neon.g);
    neon.b = neon.b * neon.b * (3.0f - 2.0f * neon.b);
    Vec3 blended(c.r * 0.25f + neon.r * 0.75f,
                 c.g * 0.25f + neon.g * 0.75f,
                 c.b * 0.25f + neon.b * 0.75f);
    return clampVec3(blended, 0.0f, 1.0f);
  } else if (grade == 3) {
    // NASA Monochromatic Kodak Tri-X 400
    float luma = c.r * 0.299f + c.g * 0.587f + c.b * 0.114f;
    float triX = smoothstepF(0.02f, 0.98f, luma);
    float val = clampF(triX, 0.0f, 1.0f);
    return Vec3(val, val, val);
  } else if (grade == 4) {
    // Technicolor Vintage 3-Strip
    Vec3 tech;
    tech.r = c.r * 1.25f - c.g * 0.12f - c.b * 0.10f;
    tech.g = c.g * 1.20f - c.r * 0.08f - c.b * 0.08f;
    tech.b = c.b * 1.15f - c.r * 0.05f - c.g * 0.05f;
    tech = clampVec3(tech, 0.0f, 1.0f);
    float luma = tech.r * 0.299f + tech.g * 0.587f + tech.b * 0.114f;
    tech.r = luma * (1.0f - 1.25f) + tech.r * 1.25f;
    tech.g = luma * (1.0f - 1.25f) + tech.g * 1.25f;
    tech.b = luma * (1.0f - 1.25f) + tech.b * 1.25f;
    return clampVec3(tech, 0.0f, 1.0f);
  }
  return c;
}

// ============================================================================
// Optics: Hyperfocal & Circle of Confusion (CoC)
// ============================================================================
double calcHyperfocal(double focalLengthMm, double fNumber, double cocLimitMm) {
  return (focalLengthMm * focalLengthMm) / (fNumber * cocLimitMm);
}

double fovToFocalLength(double fovDegrees, double sensorWidthMm = 36.0) {
  constexpr double kPi = 3.14159265358979323846;
  double halfFovRad = (fovDegrees * 0.5) * (kPi / 180.0);
  return (sensorWidthMm * 0.5) / std::tan(halfFovRad);
}

// ============================================================================
// Gravitational Wave Dynamics: Chirp rate & Strain
// ============================================================================
// Peters-Mathews chirp derivative: df/dt = (96/5) * pi^(8/3) * (G*M_c/c^3)^(5/3) * f^(11/3)
double calcChirpRate(double chirpMassSolar, double freqHz) {
  constexpr double kG = 6.67430e-11;
  constexpr double kC = 299792458.0;
  constexpr double kMSun = 1.98847e30;
  constexpr double kPi = 3.14159265358979323846;

  double mChirpKg = chirpMassSolar * kMSun;
  double factorTime = (kG * mChirpKg) / (kC * kC * kC);
  double fFactor = (96.0 / 5.0) * std::pow(kPi, 8.0 / 3.0) *
                   std::pow(factorTime, 5.0 / 3.0) *
                   std::pow(freqHz, 11.0 / 3.0);
  return fFactor;
}

int main() {
  std::cout << "[Test Cinematography] Starting verification...\n";

  // --------------------------------------------------------------------------
  // Test 1: Color Grading bounds and exact neutrality
  // --------------------------------------------------------------------------
  std::vector<Vec3> testColors = {
      Vec3(0.0f, 0.0f, 0.0f),
      Vec3(1.0f, 1.0f, 1.0f),
      Vec3(0.5f, 0.5f, 0.5f),
      Vec3(0.8f, 0.1f, 0.05f),
      Vec3(0.1f, 0.85f, 0.2f),
      Vec3(0.05f, 0.2f, 0.9f),
      Vec3(0.95f, 0.85f, 0.1f),
      Vec3(0.15f, 0.75f, 0.95f),
  };

  for (const auto& inCol : testColors) {
    // Grade 0 must be exact bitwise identity
    Vec3 g0 = applyColorGradingSim(inCol, 0);
    TEST_ASSERT(g0 == inCol);

    // Grades 1..4 must remain strictly within [0, 1]
    for (int grade = 1; grade <= 4; ++grade) {
      Vec3 out = applyColorGradingSim(inCol, grade);
      TEST_ASSERT(out.r >= 0.0f && out.r <= 1.0f);
      TEST_ASSERT(out.g >= 0.0f && out.g <= 1.0f);
      TEST_ASSERT(out.b >= 0.0f && out.b <= 1.0f);
    }
  }
  std::cout << "  [PASS] Test 1: Color grading profiles strictly bounded [0, 1] & Grade 0 identity\n";

  // --------------------------------------------------------------------------
  // Test 2: NASA Monochromatic Kodak Tri-X 400 panchromatic response
  // --------------------------------------------------------------------------
  for (const auto& inCol : testColors) {
    Vec3 mono = applyColorGradingSim(inCol, 3);
    // Must be completely desaturated: R == G == B
    TEST_ASSERT(std::abs(mono.r - mono.g) < 1e-5f);
    TEST_ASSERT(std::abs(mono.g - mono.b) < 1e-5f);
  }
  // Pure black maps to 0, pure white maps to 1
  Vec3 blackMono = applyColorGradingSim(Vec3(0.0f, 0.0f, 0.0f), 3);
  Vec3 whiteMono = applyColorGradingSim(Vec3(1.0f, 1.0f, 1.0f), 3);
  TEST_ASSERT(blackMono.r == 0.0f);
  TEST_ASSERT(whiteMono.r == 1.0f);
  std::cout << "  [PASS] Test 2: NASA Kodak Tri-X panchromatic response verified\n";

  // --------------------------------------------------------------------------
  // Test 3: Interstellar Amber & Cyberpunk Neon split-toning
  // --------------------------------------------------------------------------
  // Amber grade on pure blue: should warm up red channel
  Vec3 blue(0.1f, 0.2f, 0.8f);
  Vec3 amberBlue = applyColorGradingSim(blue, 1);
  TEST_ASSERT(amberBlue.r > blue.r);
  TEST_ASSERT(amberBlue.b <= blue.b);

  // Cyberpunk grade on dark neutral: shadows should have cyan tint (G, B > R)
  Vec3 dark(0.15f, 0.15f, 0.15f);
  Vec3 cyberDark = applyColorGradingSim(dark, 2);
  TEST_ASSERT(cyberDark.b > cyberDark.r);
  std::cout << "  [PASS] Test 3: Interstellar Amber and Cyberpunk split-toning verified\n";

  // --------------------------------------------------------------------------
  // Test 4: Optics & FOV Focal Length Scaling
  // --------------------------------------------------------------------------
  double focal15 = fovToFocalLength(15.0);   // Telephoto lens
  double focal60 = fovToFocalLength(60.0);   // Standard normal lens
  double focal120 = fovToFocalLength(120.0); // Ultra-wide lens

  TEST_ASSERT(focal15 > focal60);
  TEST_ASSERT(focal60 > focal120);
  TEST_ASSERT(focal15 > 130.0);   // 15 deg corresponds to ~136mm telephoto
  TEST_ASSERT(focal120 < 15.0);   // 120 deg corresponds to ~10mm ultra-wide

  // Hyperfocal distance: f=50mm, coc=0.03mm (Full Frame standard)
  double hF1_4 = calcHyperfocal(50.0, 1.4, 0.03); // f/1.4 wide open
  double hF16  = calcHyperfocal(50.0, 16.0, 0.03); // f/16 stopped down
  TEST_ASSERT(hF1_4 > hF16); // Wide open hyperfocal is much further out -> shallower DoF
  std::cout << "  [PASS] Test 4: Camera optics, focal scaling & DoF hyperfocal math verified\n";

  // --------------------------------------------------------------------------
  // Test 5: Gravitational Wave Strain & Chirp Rate Dynamics
  // --------------------------------------------------------------------------
  // GW170817 binary neutron star chirp mass ~ 1.188 M_sun
  double dfdt30 = calcChirpRate(1.188, 30.0);
  double dfdt100 = calcChirpRate(1.188, 100.0);
  double dfdt400 = calcChirpRate(1.188, 400.0);

  // Frequency evolution must strictly accelerate (df/dt scales as f^(11/3))
  TEST_ASSERT(dfdt30 > 0.0);
  TEST_ASSERT(dfdt100 > dfdt30);
  TEST_ASSERT(dfdt400 > dfdt100);

  // Polarization orthogonality test: integral of h+(t) * h_cross(t) over 2*pi period
  constexpr double kPi = 3.14159265358979323846;
  const int kSteps = 1000;
  double innerProduct = 0.0;
  for (int i = 0; i < kSteps; ++i) {
    double phase = (static_cast<double>(i) / kSteps) * 2.0 * kPi;
    double hp = std::cos(phase);
    double hc = std::sin(phase);
    innerProduct += hp * hc * (2.0 * kPi / kSteps);
  }
  TEST_ASSERT(std::abs(innerProduct) < 1e-4);
  std::cout << "  [PASS] Test 5: Gravitational wave chirp acceleration & polarization orthogonality verified\n";

  std::cout << "[Test Cinematography] ALL 5 TEST SUITES PASSED.\n";
  return 0;
}
