#include <iostream>
#include <string>
#include <cmath>
#include "Catalog.h"
#include "Camera.h"
#include "Geodesic.h"
#include "Binet.h"
#include "Kerr.h"
#include "Interaction.h"
#include "Version.h"
#include "WarpMetric.h"
#include "WarpGeodesic.h"

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
               "--demo-geodesic | --demo-binet | --demo-rkf45 | --demo-kerr | --demo-warp | --test-interaction | --version";
#ifdef ASTROVOYAGER_ENABLE_GL
  std::cout << " | --gl | --screenshot-all [--disk]"
               " | --capture-station <png> | --capture-dragon <png>";
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
  if (arg == "--demo-rkf45") {
    // Validation of the adaptive RKF45 (Fehlberg 5(4)) ray tracer:
    // (A) capture/escape parity with the fixed-step RK4 law,
    // (B) equal-or-better endpoint accuracy with fewer acceleration
    //     evaluations (the speedup the roadmap item is about),
    // (C) tighter energy observables than the fixed law (dt from E/f).
    const double rs = 1.0;
    const double r0 = 10.0;
    const double bCrit = 1.5 * std::sqrt(3.0) * rs;
    std::cout << "=== RKF45 adaptive tracer validation (Rs=" << rs
              << ", r0=" << r0 << ", b_crit=" << bCrit << ") ===\n";
    bool okA = true, okB = true, okC = true;

    // (A) Parity across the capture/escape boundary.
    const double kBs[5] = {2.0, 2.5, 2.7, 3.0, 4.0};
    for (double b : kBs) {
      bool capF = false, capA = false;
      astro::traceRay(makeInwardRay(r0, b, rs), rs, 30.0, 0.02, 8000, capF);
      astro::SchwState rayA = makeInwardRay(r0, b, rs);
      astro::traceRayRkf45(rayA, rs, 30.0, 0.02, 1e-10, 8000, capA);
      const bool ok = (capF == capA) && (capA == (b < bCrit));
      std::cout << "b=" << b << ": fixed=" << (capF ? "capture" : "escape")
                << " rkf45=" << (capA ? "capture" : "escape") << " -> "
                << (ok ? "PASS" : "FAIL") << "\n";
      if (!ok) okA = false;
    }

    // (B) Endpoint accuracy vs evaluations: reference RK4 h=0.002.
    const double bRef = 2.7;
    astro::SchwState refS = makeInwardRay(r0, bRef, rs);
    for (int i = 0; i < 100000; ++i) {
      astro::rk4Step(0.002, refS, rs);
      if (refS.r <= rs * 1.001 || refS.r >= 30.0) break;
    }
    astro::SchwState fixS = makeInwardRay(r0, bRef, rs);
    long fixEvals = 0;
    for (int i = 0; i < 100000; ++i) {
      astro::rk4Step(0.02, fixS, rs);
      fixEvals += 4;
      if (fixS.r <= rs * 1.001 || fixS.r >= 30.0) break;
    }
    astro::SchwState rkfS = makeInwardRay(r0, bRef, rs);
    bool capB = false;
    astro::Rkf45Stats st;
    astro::traceRayRkf45(rkfS, rs, 30.0, 0.02, 1e-10, 8000, capB, &st);
    const double refPhi = refS.phi;
    const double eFix = std::abs(fixS.phi - refPhi) / (std::abs(refPhi) + 1e-12);
    const double eRkf = std::abs(rkfS.phi - refPhi) / (std::abs(refPhi) + 1e-12);
    if (!(eRkf <= 2.0 * eFix) || !(st.evals <= 0.7 * fixEvals) || capB) okB = false;
    std::cout << "b=" << bRef << ": phi rel err fixed=" << eFix
              << " rkf45=" << eRkf << ", evals fixed=" << fixEvals
              << " rkf45=" << st.evals << " -> "
              << (okB ? "PASS" : "FAIL") << "\n";

    // (C) Energy observable after the full trace: dt must equal
    // E0 / (1 - Rs/r) at the escape shell (tighter than the 1e-6
    // RK4 gate from --demo-geodesic since renormalize pins dt each step).
    astro::SchwState cS = makeInwardRay(12.0, 4.0, rs);
    const double E0 = (1.0 - rs / cS.r) * cS.dt;
    bool capC = false;
    astro::traceRayRkf45(cS, rs, 30.0, 0.02, 1e-10, 8000, capC);
    if (cS.r >= 30.0) {
      const double f = 1.0 - rs / cS.r;
      const double drift = std::abs(cS.dt - E0 / f);
      if (drift >= 1e-6) okC = false;
      std::cout << "energy dt drift at shell: " << drift << " (< 1e-6 -> "
                << (drift < 1e-6 ? "PASS" : "FAIL") << ")\n";
    }

    std::cout << "\nRKF45 validation result: "
              << ((okA && okB && okC) ? "ALL PASS" : "FAIL") << "\n";
    return (okA && okB && okC) ? 0 : 1;
  }
  if (arg == "--demo-kerr") {
    // Validation of Kerr null geodesics module (Kerr.cpp):
    // (A) Limit a -> 0 matches Schwarzschild (Geodesic.cpp RK4).
    // (B) Asymmetry direction: b_crit(prograde) < 3*sqrt(3)/2 Rs < b_crit(retrograde) at a = 0.9M.
    // (C) Verification against literature analytics (Bardeen 1972/1973, Chandrasekhar 1983).
    const double rs = 1.0;
    const double M = 0.5 * rs;
    const double r0 = 10.0;
    const double bCritSchw = 1.5 * std::sqrt(3.0) * rs;  // ~ 2.598076211 Rs
    std::cout << "=== Kerr null geodesics validation (Rs=" << rs << ", M=" << M
              << ", b_crit(Schw)=" << bCritSchw << ") ===\n\n";

    bool passA = true;
    bool passB = true;
    bool passC = true;

    // ------------------------------------------------------------------------
    // (A) Limit a -> 0: Compare Kerr orbit (a = 0) vs Schwarzschild RK4
    // ------------------------------------------------------------------------
    std::cout << "--- (A) Schwarzschild limit (a -> 0) ---\n";
    const double testBs[] = {1.5, 2.0, 3.0, 5.0};
    const double kMaxRelErrThreshold = 1e-4;

    for (double b : testBs) {
      astro::KerrOrbitResult kerrRes =
          astro::kerrPhotonOrbit(b, 0.0, rs, r0, 0.002, 8000, 30.0);

      astro::SchwState ray = makeInwardRay(r0, b, rs);
      const double h = 0.002;
      double maxRelErr = 0.0;
      double maxAbsErr = 0.0;
      int comparedPoints = 0;

      for (int step = 0; step < 8000; ++step) {
        astro::rk4Step(h, ray, rs);
        if (ray.r <= rs * 1.001 || ray.r >= 30.0) break;

        if (static_cast<size_t>(step + 1) < kerrRes.points.size()) {
          double rKerr = kerrRes.points[step + 1].r;
          double absErr = std::abs(ray.r - rKerr);
          double relErr = absErr / ray.r;
          if (absErr > maxAbsErr) maxAbsErr = absErr;
          if (relErr > maxRelErr) maxRelErr = relErr;
          ++comparedPoints;
        }
      }

      bool passErr = (comparedPoints > 20) && (maxRelErr < kMaxRelErrThreshold);
      if (!passErr) passA = false;

      std::cout << "b=" << b << " Rs: compared=" << comparedPoints
                << " max_abs_err=" << maxAbsErr
                << " max_rel_err=" << maxRelErr
                << " (< 1e-4 -> " << (passErr ? "PASS" : "FAIL") << ")\n";
    }

    // Capture/escape scenario match in a -> 0 limit
    for (double b : {2.5, 2.7}) {
      astro::KerrOrbitResult kerrRes =
          astro::kerrPhotonOrbit(b, 0.0, rs, r0, 0.005, 5000, 30.0);
      bool rk4Cap = false;
      astro::traceRay(makeInwardRay(r0, b, rs), rs, 30.0, 0.01, 3000, rk4Cap);

      bool match = (kerrRes.captured == rk4Cap);
      if (!match) passA = false;
      std::cout << "scenario b=" << b << " Rs: Kerr="
                << (kerrRes.captured ? "capture" : "escape")
                << " Schw=" << (rk4Cap ? "capture" : "escape")
                << " -> " << (match ? "PASS" : "FAIL") << "\n";
    }
    std::cout << "Part (A) Result: " << (passA ? "PASS" : "FAIL") << "\n\n";

    // ------------------------------------------------------------------------
    // (B) Direction of asymmetry at a = 0.9 M:
    //     b_crit(prograde) < 3*sqrt(3)/2 Rs < b_crit(retrograde)
    // ------------------------------------------------------------------------
    std::cout << "--- (B) Asymmetry direction (a = 0.9 M = " << (0.9 * M) << ") ---\n";
    const double aTest = 0.9 * M;
    const double bCritProg = astro::kerrBCrit(aTest, rs, true);
    const double bCritRetro = astro::kerrBCrit(aTest, rs, false);

    std::cout << "b_crit(prograde)   = " << bCritProg << " Rs ("
              << (bCritProg / M) << " M)\n";
    std::cout << "b_crit(Schw)       = " << bCritSchw << " Rs ("
              << (bCritSchw / M) << " M)\n";
    std::cout << "b_crit(retrograde) = " << bCritRetro << " Rs ("
              << (bCritRetro / M) << " M)\n";

    bool strictInequality = (bCritProg < bCritSchw) && (bCritSchw < bCritRetro);
    if (!strictInequality) passB = false;
    std::cout << "Strict inequality b_crit(prog) < b_crit(Schw) < b_crit(retro): "
              << (strictInequality ? "PASS" : "FAIL") << "\n";

    // Numerical trajectory verification near critical thresholds:
    // Prograde: b = 1.40 Rs (< bCritProg ~ 1.422 Rs -> capture),
    //           b = 1.45 Rs (> bCritProg -> escape)
    astro::KerrOrbitResult progCap =
        astro::kerrPhotonOrbit(1.40, aTest, rs, r0, 0.005, 5000, 30.0);
    astro::KerrOrbitResult progEsc =
        astro::kerrPhotonOrbit(1.45, aTest, rs, r0, 0.005, 5000, 30.0);
    bool progDynOk = progCap.captured && progEsc.escaped;
    if (!progDynOk) passB = false;
    std::cout << "Prograde dynamic threshold (b=1.40 capture=" << progCap.captured
              << ", b=1.45 escape=" << progEsc.escaped
              << "): " << (progDynOk ? "PASS" : "FAIL") << "\n";

    // Retrograde: b = -3.40 Rs (|b| < bCritRetro ~ 3.416 Rs -> capture),
    //             b = -3.45 Rs (|b| > bCritRetro -> escape)
    astro::KerrOrbitResult retroCap =
        astro::kerrPhotonOrbit(-3.40, aTest, rs, r0, 0.005, 5000, 30.0);
    astro::KerrOrbitResult retroEsc =
        astro::kerrPhotonOrbit(-3.45, aTest, rs, r0, 0.005, 5000, 30.0);
    bool retroDynOk = retroCap.captured && retroEsc.escaped;
    if (!retroDynOk) passB = false;
    std::cout << "Retrograde dynamic threshold (b=-3.40 capture=" << retroCap.captured
              << ", b=-3.45 escape=" << retroEsc.escaped
              << "): " << (retroDynOk ? "PASS" : "FAIL") << "\n";
    std::cout << "Part (B) Result: " << (passB ? "PASS" : "FAIL") << "\n\n";

    // ------------------------------------------------------------------------
    // (C) Literature verification (Bardeen 1972/1973, Chandrasekhar 1983):
    //
    // Citations & Analytical formulas:
    //   1. Bardeen, Press, Teukolsky (1972) ApJ 178, 347 (Eq. 2.18):
    //      r_ph(prograde)   = 2M [ 1 + cos( (2/3) arccos( -a / M ) ) ]
    //      r_ph(retrograde) = 2M [ 1 + cos( (2/3) arccos(  a / M ) ) ]
    //   2. Bardeen (1973) in "Black Holes" (DeWitt & DeWitt eds.), Gordon and Breach:
    //      xi_c = - (r_ph^3 - 3M r_ph^2 + a^2 r_ph + M a^2) / (a (r_ph - M))
    //      |b_crit| = |xi_c|
    //   3. Known exact values:
    //      a = 0:               b_crit = 3*sqrt(3)*M  ~ 5.1961524 M (2.598076 Rs)
    //      a -> M (prograde):   r_ph -> 1.0 M, b_crit -> 2.0 M       (1.0 Rs)
    //      a -> M (retrograde): r_ph -> 4.0 M, b_crit -> 7.0 M       (3.5 Rs)
    // ------------------------------------------------------------------------
    std::cout << "--- (C) Literature verification (Bardeen 1972/1973, Chandrasekhar 1983) ---\n";
    // Check 1: a = 0 against 3*sqrt(3)*M
    const double b0 = astro::kerrBCrit(0.0, rs, true);
    const double b0_expected = 3.0 * std::sqrt(3.0) * M;
    const double err0 = std::abs(b0 - b0_expected);
    bool ok0 = (err0 < 1e-12);
    if (!ok0) passC = false;
    std::cout << "a = 0: b_crit=" << b0 << " expected=" << b0_expected
              << " err=" << err0 << " -> " << (ok0 ? "PASS" : "FAIL") << "\n";

    // Check 2: a -> M (a = 0.999999 M)
    const double aExtremal = 0.999999 * M;
    const double bProgExt = astro::kerrBCrit(aExtremal, rs, true);
    const double bRetroExt = astro::kerrBCrit(aExtremal, rs, false);

    // Expected limits: prograde -> 2.0 M = 1.0 Rs, retrograde -> 7.0 M = 3.5 Rs
    const double errProgExt = std::abs(bProgExt - 2.0 * M);
    const double errRetroExt = std::abs(bRetroExt - 7.0 * M);
    bool okProgExt = (errProgExt < 5e-3);
    bool okRetroExt = (errRetroExt < 5e-3);
    if (!okProgExt || !okRetroExt) passC = false;
    std::cout << "a -> M (prograde limit):   b_crit=" << (bProgExt / M)
              << " M expected=2.000000 M err=" << (errProgExt / M)
              << " -> " << (okProgExt ? "PASS" : "FAIL") << "\n";
    std::cout << "a -> M (retrograde limit): b_crit=" << (bRetroExt / M)
              << " M expected=7.000000 M err=" << (errRetroExt / M)
              << " -> " << (okRetroExt ? "PASS" : "FAIL") << "\n";

    // Check 3: numerical bisect of dynamic threshold vs analytic b_crit at a = 0.9 M
    // Analytical: bCritProg ~ 2.84442 M (1.42221 Rs), bCritRetro ~ 6.83232 M (3.41616 Rs)
    auto bisectThreshold = [&](double aVal, bool prograde) {
      double low = prograde ? 1.0 * rs : -4.0 * rs;
      double high = prograde ? 2.0 * rs : -3.0 * rs;
      for (int it = 0; it < 20; ++it) {
        double mid = 0.5 * (low + high);
        astro::KerrOrbitResult orRes =
            astro::kerrPhotonOrbit(mid, aVal, rs, r0, 0.005, 5000, 30.0);
        if (prograde) {
          if (orRes.captured) low = mid;
          else high = mid;
        } else {
          // Retrograde: mid is negative, smaller magnitude means capture
          if (orRes.captured) high = mid;
          else low = mid;
        }
      }
      return std::abs(0.5 * (low + high));
    };

    const double bNumProg = bisectThreshold(aTest, true);
    const double bNumRetro = bisectThreshold(aTest, false);
    const double diffProg = std::abs(bNumProg - bCritProg);
    const double diffRetro = std::abs(bNumRetro - bCritRetro);
    // Numerical bisect tolerance < 1e-2 Rs
    bool okNumProg = (diffProg < 0.01);
    bool okNumRetro = (diffRetro < 0.01);
    if (!okNumProg || !okNumRetro) passC = false;
    std::cout << "a = 0.9M prograde:   numerical_b_crit=" << bNumProg
              << " analytic=" << bCritProg << " diff=" << diffProg
              << " (< 0.01 -> " << (okNumProg ? "PASS" : "FAIL") << ")\n";
    std::cout << "a = 0.9M retrograde: numerical_b_crit=" << bNumRetro
              << " analytic=" << bCritRetro << " diff=" << diffRetro
              << " (< 0.01 -> " << (okNumRetro ? "PASS" : "FAIL") << ")\n";

    std::cout << "Part (C) Result: " << (passC ? "PASS" : "FAIL") << "\n\n";

    bool allKerrOk = passA && passB && passC;
    std::cout << "Kerr validation result: " << (allKerrOk ? "ALL PASS" : "FAIL") << "\n";
    return allKerrOk ? 0 : 1;
  }
  if (arg == "--demo-warp") {
    // Validation of M1 WarpMetric (3+1 ADM interface) and Hamiltonian null geodesic integrator:
    // (A) Metric inversion consistency (gamma^{ik} gamma_{kj} = delta^i_j, g^{mu alpha} g_{alpha nu} = delta^mu_nu)
    // (B) Schwarzschild limit parity with Geodesic.cpp RK4 and analytical b_crit = 2.598 Rs
    // (C) Hamiltonian constraint conservation along trajectories (|H| < 1e-6)
    // (D) Critical boundary sensitivity (b_crit +/- 0.005 Rs)
    const double rs = 1.0;
    const double r0 = 10.0;
    const double bCrit = 1.5 * std::sqrt(3.0) * rs;
    std::cout << "=== Warp ADM null geodesic validation (Rs=" << rs
              << ", r0=" << r0 << ", b_crit=" << bCrit << ") ===\n\n";

    bool passA = true, passB = true, passC = true, passD = true;
    astro::SchwarzschildADM metric(rs);

    // --- (A) Metric Inversion & Derivation ---
    std::cout << "--- (A) ADM Metric Inversion & Analytical Derivatives ---\n";
    double alpha = 0.0;
    double beta[3] = {0.0, 0.0, 0.0};
    double g_ij[3][3], g_inv[3][3];
    metric.evalADM(3.0, 4.0, 5.0, 0.0, alpha, beta, g_ij, g_inv);

    double maxErrA = 0.0;
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        double sum = 0.0;
        for (int k = 0; k < 3; ++k) sum += g_inv[i][k] * g_ij[k][j];
        const double delta = (i == j) ? 1.0 : 0.0;
        const double err = std::abs(sum - delta);
        if (err > maxErrA) maxErrA = err;
      }
    }
    bool okInv = (maxErrA < 1e-10);
    if (!okInv) passA = false;
    std::cout << "gamma * gamma_inv = I: max_err=" << maxErrA
              << " (< 1e-10 -> " << (okInv ? "PASS" : "FAIL") << ")\n";

    double dg_an[3][4][4], dg_num[3][4][4];
    metric.evalDerivInv4Metric(4.0, -3.0, 2.0, 0.0, dg_an);
    metric.WarpMetric::evalDerivInv4Metric(4.0, -3.0, 2.0, 0.0, dg_num);
    double maxDiffDg = 0.0;
    for (int k = 0; k < 3; ++k) {
      for (int mu = 0; mu < 4; ++mu) {
        for (int nu = 0; nu < 4; ++nu) {
          const double diff = std::abs(dg_an[k][mu][nu] - dg_num[k][mu][nu]);
          if (diff > maxDiffDg) maxDiffDg = diff;
        }
      }
    }
    bool okDg = (maxDiffDg < 1e-5);
    if (!okDg) passA = false;
    std::cout << "Analytical vs numerical dg/dx: max_diff=" << maxDiffDg
              << " (< 1e-5 -> " << (okDg ? "PASS" : "FAIL") << ")\n";
    std::cout << "Part (A) Result: " << (passA ? "PASS" : "FAIL") << "\n\n";

    // --- (B) Parity with Geodesic.cpp RK4 ---
    std::cout << "--- (B) Schwarzschild Capture/Escape Parity ---\n";
    const double testBs[] = {1.5, 2.0, 2.5, 2.7, 3.0, 5.0};
    for (double b : testBs) {
      const bool expectCap = (b < bCrit);
      astro::WarpTraceResult warpRes = astro::traceWarpRay(
          metric, astro::makeWarpInwardRay(metric, r0, b), 30.0, 0.01, 4000);

      const double f0 = 1.0 - rs / r0;
      astro::SchwState geoRay;
      geoRay.t = 0.0;
      geoRay.r = r0;
      geoRay.theta = 1.5707963267948966;
      geoRay.phi = 0.0;
      geoRay.dt = 1.0 / f0;
      geoRay.dr = -std::sqrt(std::max(1.0 - f0 * b * b / (r0 * r0), 0.0));
      geoRay.dtheta = 0.0;
      geoRay.dphi = b / (r0 * r0);
      bool geoCap = false;
      astro::traceRay(geoRay, rs, 30.0, 0.01, 4000, geoCap);

      const bool match = (warpRes.captured == expectCap) && (warpRes.captured == geoCap);
      if (!match) passB = false;
      std::cout << "scenario b=" << b << " Rs: ADM="
                << (warpRes.captured ? "capture" : "escape")
                << " Geodesic=" << (geoCap ? "capture" : "escape")
                << " -> " << (match ? "PASS" : "FAIL") << "\n";
    }
    std::cout << "Part (B) Result: " << (passB ? "PASS" : "FAIL") << "\n\n";

    // --- (C) Hamiltonian Constraint Conservation ---
    std::cout << "--- (C) Hamiltonian Null Constraint Conservation ---\n";
    astro::WarpRayState rayH = astro::makeWarpInwardRay(metric, r0, 3.0);
    double maxDriftH = 0.0;
    for (int step = 0; step < 1000; ++step) {
      astro::warpRk4Step(metric, 0.01, rayH);
      const double hVal = std::abs(astro::evalHamiltonian(metric, rayH));
      if (hVal > maxDriftH) maxDriftH = hVal;
    }
    bool okH = (maxDriftH < 1e-6);
    if (!okH) passC = false;
    std::cout << "Max |H| drift after 1000 steps: " << maxDriftH
              << " (< 1e-6 -> " << (okH ? "PASS" : "FAIL") << ")\n";
    std::cout << "Part (C) Result: " << (passC ? "PASS" : "FAIL") << "\n\n";

    // --- (D) Critical Boundary Sensitivity ---
    std::cout << "--- (D) Critical Boundary Sensitivity (b_crit +/- 0.005 Rs) ---\n";
    const double bUnder = 2.598 - 0.005;
    const double bOver = 2.598 + 0.005;
    astro::WarpTraceResult resUnder = astro::traceWarpRay(
        metric, astro::makeWarpInwardRay(metric, r0, bUnder), 30.0, 0.005, 6000);
    astro::WarpTraceResult resOver = astro::traceWarpRay(
        metric, astro::makeWarpInwardRay(metric, r0, bOver), 30.0, 0.005, 6000);

    bool underOk = resUnder.captured;
    bool overOk = !resOver.captured;
    if (!underOk || !overOk) passD = false;
    std::cout << "b=" << bUnder << " (< b_crit): captured=" << resUnder.captured
              << " -> " << (underOk ? "PASS" : "FAIL") << "\n";
    std::cout << "b=" << bOver << " (> b_crit): captured=" << resOver.captured
              << " -> " << (overOk ? "PASS" : "FAIL") << "\n";
    std::cout << "Part (D) Result: " << (passD ? "PASS" : "FAIL") << "\n\n";

    // --- (E) Alcubierre Skiff Warp Flight: Earth -> Proxima-16 (Skyluck) ---
    std::cout << "--- (E) Alcubierre Skiff Warp Flight: Earth -> Proxima-16 (Skyluck) ---\n";
    const double earthPos[3] = {0.0, 0.0, 0.0};
    const double proximaPos[3] = {1000.0, 250.0, -100.0};
    const double vMax = 12.5;
    const double sigma = 0.5;
    const int steps = 50;

    std::vector<astro::SkiffWaypoint> path =
        astro::WarpGeodesic::skiffPath(earthPos, proximaPos, vMax, sigma, steps);

    const double dx = proximaPos[0] - earthPos[0];
    const double dy = proximaPos[1] - earthPos[1];
    const double dz = proximaPos[2] - earthPos[2];
    const double dist = std::sqrt(dx * dx + dy * dy + dz * dz);
    const double wallLength = 3.0 * sigma;

    double maxThetaObserved = 0.0;
    double minWallClearance = 1e9;
    double maxDtsDtau = 0.0;
    for (const auto& wp : path) {
      if (wp.thetaMax > maxThetaObserved) maxThetaObserved = wp.thetaMax;
      minWallClearance = std::min(minWallClearance, wp.wallClearance);
      maxDtsDtau = std::max(maxDtsDtau, wp.dtsDtau);
    }

    std::cout << "Flight plan: Sol (Earth) -> Proxima-16 (Skyluck)\n";
    std::cout << "Distance: " << dist << " units\n";
    std::cout << "Peak velocity: " << vMax << " c\n";
    std::cout << "Bubble wall thickness: " << sigma << " units (transition length ~" << wallLength << ")\n";
    std::cout << "York time theta_max: " << maxThetaObserved << " s^-1\n";
    std::cout << "Waypoints: " << path.size() << " | Inner-wall clearance: " << minWallClearance
              << " units (ship at bubble centre)\n";
    std::cout << "dt_s/dtau at cruise: " << maxDtsDtau << "\n";

    const bool passE = (minWallClearance > 0.0) && (maxDtsDtau <= 1.0 + 1e-9) &&
                       (maxThetaObserved > 0.0) && (path.size() == static_cast<size_t>(steps));
    std::cout << "Part (E) Result: " << (passE ? "PASS" : "FAIL") << "\n\n";

    // --- (F) Crew frame Doppler: stars ahead/behind (Issue I1) ---
    std::cout << "--- (F) Crew frame Doppler: stars ahead/behind (Issue I1) ---\n";
    const double dopplerV = 0.5;
    astro::AlcubierreMetric crewMetric(5.0, 0.5, dopplerV,
                                       0.0, 0.0, 0.0, 1.0, 0.0, 0.0,
                                       10000.0, 1000.0);
    const double crewT = 5000.0;
    double crewCenter[3];
    crewMetric.bubbleCenter(crewT, crewCenter);

    auto traceCrewPhoton = [&](double startX, double px, double& gInvariant) {
      astro::WarpRayState ray;
      ray.t = crewT;
      ray.x = crewCenter[0] + startX;
      ray.y = crewCenter[1];
      ray.z = crewCenter[2];
      ray.pt = -1.0;
      ray.px = px;
      ray.py = 0.0;
      ray.pz = 0.0;
      for (int step = 0; step < 4000; ++step) {
        astro::warpRk4Step(crewMetric, 0.01, ray);
        double c[3];
        crewMetric.bubbleCenter(ray.t, c);
        if (std::abs(ray.x - c[0]) < 0.05) {
          const double v = crewMetric.velocity(ray.t);
          // The distant static emitter has E = -p_t = 1 at launch, so the
          // comoving energy -p.u_obs is exactly the frequency ratio.
          gInvariant = -(ray.pt + v * ray.px);
          return true;
        }
      }
      return false;
    };

    double gAhead = 0.0;
    double gBehind = 0.0;
    const bool aheadHit = traceCrewPhoton(+10.0, -1.0, gAhead);
    const bool behindHit = traceCrewPhoton(-10.0, +1.0, gBehind);
    const double nAhead[3] = {-1.0, 0.0, 0.0};
    const double nBehind[3] = {1.0, 0.0, 0.0};
    const double dirX[3] = {1.0, 0.0, 0.0};
    const double formulaAhead =
        astro::alcubierreCrewShift(dopplerV, nAhead, dirX);
    const double formulaBehind =
        astro::alcubierreCrewShift(dopplerV, nBehind, dirX);

    std::cout.precision(12);
    std::cout << "Bubble speed: " << dopplerV << " c\n";
    std::cout << "Ahead:  g_invariant=" << gAhead << " formula=" << formulaAhead
              << " (expected 1.5)\n";
    std::cout << "Behind: g_invariant=" << gBehind << " formula=" << formulaBehind
              << " (expected 0.5)\n";
    std::cout.precision(6);

    const bool passF = aheadHit && behindHit &&
                       std::abs(gAhead - 1.5) < 1e-6 &&
                       std::abs(gBehind - 0.5) < 1e-6 &&
                       std::abs(gAhead - formulaAhead) < 1e-6 &&
                       std::abs(gBehind - formulaBehind) < 1e-6;
    std::cout << "Part (F) Result: " << (passF ? "PASS" : "FAIL") << "\n\n";

    const bool allWarpOk = passA && passB && passC && passD && passE && passF;
    std::cout << "Warp ADM validation result: " << (allWarpOk ? "ALL PASS" : "FAIL") << "\n";
    return allWarpOk ? 0 : 1;
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
  if (arg == "--capture-station") {
    std::string outPng = (argc > 2) ? argv[2] : "blender/out/ingame_station_check.png";
    return astro::AppGL::captureStationCheck(outPng);
  }
  if (arg == "--capture-dragon") {
    std::string outPng = (argc > 2) ? argv[2] : "docs/screens/spacex_dragon_ingame.png";
    return astro::AppGL::captureDragonCheck(outPng);
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
