#include <iostream>
#include <string>
#include <cmath>
#include "Catalog.h"
#include "Camera.h"
#include "Geodesic.h"
#include "Binet.h"
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
               "--demo-geodesic | --demo-binet | --test-interaction | --version";
#ifdef ASTROVOYAGER_ENABLE_GL
  std::cout << " | --gl | --screenshot-all [--disk]";
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
  if (arg == "--demo-binet") {
    // Cross-validation of CPU geodesic integrator (Geodesic.cpp RK4)
    // against Schwarzschild Binet orbit equation d^2u/dphi^2 + u = 3M u^2.
    const double rs = 1.0;
    const double r0 = 10.0;
    const double bCrit = 1.5 * std::sqrt(3.0) * rs;  // ~ 2.598076211 Rs
    std::cout << "=== Binet u(phi) cross-validation (Rs=" << rs << ", r0=" << r0
              << ", b_crit=" << bCrit << ") ===\n";

    bool allPassed = true;

    // 1. Pointwise error comparison on common phi range for various impact parameters
    const double testBs[] = {1.5, 2.0, 3.0, 5.0};
    const double kMaxRelErrThreshold = 1e-4;

    for (double b : testBs) {
      // Binet orbit
      astro::BinetOrbitResult binetRes = astro::binetPhotonOrbit(b, rs, r0, 0.0005, 25.0, 30.0);

      // CPU geodesic raytrace via rk4Step with small step for precision reference
      astro::SchwState ray = makeInwardRay(r0, b, rs);
      const double h = 0.002;
      double maxRelErr = 0.0;
      double maxAbsErr = 0.0;
      int comparedPoints = 0;

      size_t binetIdx = 0;
      for (int step = 0; step < 8000; ++step) {
        astro::rk4Step(h, ray, rs);

        // Stop if ray hit horizon or escaped
        if (ray.r <= rs * 1.001 || ray.r >= 30.0) break;

        const double phiRay = ray.phi;
        // Advance Binet index to find closest phi
        while (binetIdx + 1 < binetRes.points.size() &&
               binetRes.points[binetIdx + 1].phi <= phiRay) {
          ++binetIdx;
        }

        if (binetIdx + 1 < binetRes.points.size()) {
          // Linear interpolation of u_Binet at phiRay
          const auto& p0 = binetRes.points[binetIdx];
          const auto& p1 = binetRes.points[binetIdx + 1];
          double frac = (phiRay - p0.phi) / (p1.phi - p0.phi);
          double uInterp = p0.u + frac * (p1.u - p0.u);

          double uRK4 = 1.0 / ray.r;
          double absErr = std::abs(uRK4 - uInterp);
          double relErr = absErr / uInterp;

          if (absErr > maxAbsErr) maxAbsErr = absErr;
          if (relErr > maxRelErr) maxRelErr = relErr;
          ++comparedPoints;
        }
      }

      bool passErr = (comparedPoints > 20) && (maxRelErr < kMaxRelErrThreshold);
      if (!passErr) allPassed = false;

      std::cout << "b=" << b << " Rs: compared=" << comparedPoints
                << " max_abs_err=" << maxAbsErr
                << " max_rel_err=" << maxRelErr
                << " (< 1e-4 -> " << (passErr ? "PASS" : "FAIL") << ")\n";
    }

    // 2. Scenario match: capture vs escape for b < b_crit and b > b_crit
    std::cout << "\nScenario matching:\n";
    const double scenarioBs[] = {1.5, 2.0, 2.5, 2.7, 3.0, 5.0};
    for (double b : scenarioBs) {
      bool binetCap = astro::binetIsCaptured(b, rs);
      astro::BinetOrbitResult binetRes = astro::binetPhotonOrbit(b, rs, r0, 0.001, 25.0, 30.0);

      bool rk4Cap = false;
      astro::traceRay(makeInwardRay(r0, b, rs), rs, 30.0, 0.01, 3000, rk4Cap);

      bool match = (binetCap == rk4Cap) && (binetRes.captured == rk4Cap);
      if (!match) allPassed = false;

      std::cout << "b=" << b << " Rs: Binet_theory=" << (binetCap ? "capture" : "escape")
                << " Binet_RK4=" << (binetRes.captured ? "capture" : "escape")
                << " Raytrace_RK4=" << (rk4Cap ? "capture" : "escape")
                << " -> " << (match ? "PASS" : "FAIL") << "\n";
    }

    // 3. Near-critical boundary test: b = 2.598 +/- 0.005
    std::cout << "\nCritical boundary sensitivity (b_crit +/- 0.005 Rs):\n";
    const double bUnder = 2.598 - 0.005;  // 2.593 < b_crit -> capture
    const double bOver = 2.598 + 0.005;   // 2.603 > b_crit -> escape

    bool underBinet = astro::binetIsCaptured(bUnder, rs);
    bool underRK4 = false;
    astro::traceRay(makeInwardRay(r0, bUnder, rs), rs, 30.0, 0.01, 4000, underRK4);
    bool underMatch = (underBinet && underRK4);
    if (!underMatch) allPassed = false;
    std::cout << "b=" << bUnder << " (< b_crit): Binet=" << (underBinet ? "capture" : "escape")
              << " RK4=" << (underRK4 ? "capture" : "escape")
              << " -> " << (underMatch ? "PASS" : "FAIL") << "\n";

    bool overBinet = astro::binetIsCaptured(bOver, rs);
    bool overRK4 = false;
    astro::traceRay(makeInwardRay(r0, bOver, rs), rs, 30.0, 0.01, 4000, overRK4);
    bool overMatch = (!overBinet && !overRK4);
    if (!overMatch) allPassed = false;
    std::cout << "b=" << bOver << " (> b_crit): Binet=" << (overBinet ? "capture" : "escape")
              << " RK4=" << (overRK4 ? "capture" : "escape")
              << " -> " << (overMatch ? "PASS" : "FAIL") << "\n";

    std::cout << "\nBinet validation result: " << (allPassed ? "ALL PASS" : "FAIL") << "\n";
    return allPassed ? 0 : 1;
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
    bool diskOn = (argc > 2 && std::string(argv[2]) == "--disk");
    std::string outDir = diskOn ? "docs/screens/disk" : "docs/screens";
    return astro::AppGL::captureAllScreenshots(outDir, diskOn);
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
