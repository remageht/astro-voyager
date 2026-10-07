#include <iostream>
#include <string>
#include <cmath>
#include "Catalog.h"
#include "Camera.h"
#include "Geodesic.h"
#include "Interaction.h"
#include "Version.h"

#ifdef ASTROVOYAGER_ENABLE_GL
#include "AppGL.h"
#endif

namespace {
// Null geodesic with impact parameter b = L/E launched inward from r0 in the
// equatorial plane. Energy is normalized to E = dt/dLambda = 1, so:
//   f0        = 1 - Rs/r0
//   dt        = 1 / f0            (from ds2 = 0)
//   dr        = -sqrt(1 - f0 b^2 / r0^2)   (inward, radial equation)
//   dphi      = b / r0^2                   (L = r^2 dphi/dLambda = b)
// Critical impact parameter of the Schwarzschild photon sphere:
//   b_crit = 3 sqrt(3) M = 3 sqrt(3) / 2 Rs = 2.598076... Rs
astro::SchwState makeInwardRay(double r0, double b, double rs) {
  const double kPi = 3.141592653589793;
  const double f0 = 1.0 - rs / r0;
  astro::SchwState s;
  s.t = 0.0;
  s.r = r0;
  s.theta = 0.5 * kPi;
  s.phi = 0.0;
  s.dt = 1.0 / f0;
  s.dr = -std::sqrt(std::max(1.0 - f0 * b * b / (r0 * r0), 0.0));
  s.dtheta = 0.0;
  s.dphi = b / (r0 * r0);
  return s;
}

void printUsage() {
  std::cout << "Usage: astro-voyager [--list | --info <id> | "
               "--demo-geodesic | --test-interaction | --version";
#ifdef ASTROVOYAGER_ENABLE_GL
  std::cout << " | --gl | --screenshot-all";
#endif
  std::cout << "]\n";
}
}  // namespace

int main(int argc, char** argv) {
  const std::string arg = argc > 1 ? argv[1] : "";
  if (arg == "--version") {
    std::cout << astro::kAppName << " " << astro::kVersion << "\n";
    return 0;
  }
  if (arg == "--list") {
    for (const auto& s : astro::builtinCatalog()) {
      std::cout << s.id << " [" << astro::typeToString(s.type) << "] "
                << s.name << "\n";
    }
    return 0;
  }
  if (arg == "--info" && argc > 2) {
    const astro::Station* st = astro::findStation(argv[2]);
    if (!st) {
      std::cerr << "unknown station: " << argv[2] << "\n";
      return 1;
    }
    std::cout << st->id << " [" << astro::typeToString(st->type) << "] "
              << st->name << "\n" << st->description << "\nspawn Rs: "
              << st->spawnDistanceRs << "\n";
    return 0;
  }
  if (arg == "--demo-geodesic") {
    // Radial in-fall must be captured; tangential far ray must escape.
    astro::SchwState inward;
    inward.r = 10.0;
    inward.theta = 1.5707;
    inward.phi = 0.0;
    inward.dt = 1.0;
    inward.dr = -1.0;
    bool cap = false;
    const int n1 =
        astro::traceRay(inward, 1.0, 30.0, 0.02, 2000, cap);
    std::cout << "inward: steps=" << n1 << " captured=" << cap << "\n";

    astro::SchwState tangent;
    tangent.r = 10.0;
    tangent.theta = 1.5707;
    tangent.phi = 0.0;
    tangent.dt = 1.0;
    tangent.dphi = 0.09;
    bool cap2 = false;
    const int n2 =
        astro::traceRay(tangent, 1.0, 30.0, 0.02, 2000, cap2);
    std::cout << "tangent: steps=" << n2 << " captured=" << cap2 << "\n";

    // Critical impact parameter: b_crit = 3 sqrt(3) / 2 Rs ~ 2.598076 Rs.
    // Same start radius for every ray, so only b decides capture vs escape.
    const double bCrit = 3.0 * std::sqrt(3.0) / 2.0;
    const double r0 = 10.0;
    bool okCrit = true;
    std::cout << "b_crit = " << bCrit << " Rs (r0=" << r0 << ")\n";
    const double kBs[3] = {2.50, 2.70, 3.00};
    for (int i = 0; i < 3; ++i) {
      const double b = kBs[i];
      bool capB = false;
      const int steps =
          astro::traceRay(makeInwardRay(r0, b, 1.0), 1.0, 30.0, 0.02, 2000, capB);
      const bool expectCapture = b < bCrit;
      std::cout << "b=" << b << ": steps=" << steps
                << " captured=" << capB
                << " (b" << (expectCapture ? "<" : ">") << "b_crit -> "
                << (expectCapture ? "capture" : "escape") << ")\n";
      if (capB != expectCapture) okCrit = false;
    }

    // Verify dt drift from analytical E / (1 - Rs/r) after 1000 steps (< 1e-6)
    astro::SchwState driftRay = makeInwardRay(12.0, 4.0, 1.0);
    const double E0 = (1.0 - 1.0 / driftRay.r) * driftRay.dt;
    double maxDtDrift = 0.0;
    for (int step = 0; step < 1000; ++step) {
      astro::rk4Step(0.005, driftRay, 1.0);
      const double f = 1.0 - 1.0 / driftRay.r;
      const double dtAnalytic = E0 / f;
      const double drift = std::abs(driftRay.dt - dtAnalytic);
      if (drift > maxDtDrift) maxDtDrift = drift;
    }
    const bool okDrift = (maxDtDrift < 1e-6);
    std::cout << "dt drift after 1000 steps: " << maxDtDrift
              << " (< 1e-6 -> " << (okDrift ? "PASS" : "FAIL") << ")\n";

    astro::Camera cam;
    cam.clampNearHorizon(1.0);
    std::cout << "camera z=" << cam.position.z << "\n";
    return (cap && !cap2 && okCrit && okDrift) ? 0 : 2;
  }
  if (arg == "--test-interaction") {
    bool ok = blackhole::ShouldCaptureMouseForCamera(true, false) &&
              !blackhole::ShouldCaptureMouseForCamera(false, false) &&
              !blackhole::ShouldCaptureMouseForCamera(true, true) &&
              blackhole::ShouldProcessKeyboardMovement(false) &&
              !blackhole::ShouldProcessKeyboardMovement(true) &&
              std::abs(blackhole::ClampCameraRadius(0.4f, 1.05f) - 1.05f) < 0.001f;
    std::cout << "interaction: " << (ok ? "PASS" : "FAIL") << "\n";
    return ok ? 0 : 1;
  }

#ifdef ASTROVOYAGER_ENABLE_GL
  if (arg == "--screenshot-all") {
    return astro::AppGL::captureAllScreenshots("docs/screens");
  }
  if (arg == "--gl" || arg.empty()) {
    try {
      astro::AppGL app;
      return app.run();
    } catch (const std::exception& e) {
      std::cerr << "GL Error: " << e.what() << "\n";
      return 1;
    }
  }
#endif

  printUsage();
  return arg.empty() ? 0 : 1;
}
