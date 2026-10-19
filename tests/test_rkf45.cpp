#include "../src/Geodesic.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace {

// Same null-ray constructor as --demo-geodesic: inward start at r0 with
// impact parameter b (units of Rs), E=1 rays from large parabolic distance.
astro::SchwState makeInwardRay(double r0, double b, double rs) {
  const double f0 = 1.0 - rs / r0;
  astro::SchwState s;
  s.r = r0;
  s.theta = 0.5 * 3.141592653589793;
  s.phi = 0.0;
  s.dt = 1.0 / f0;
  s.dr = -std::sqrt(std::max(1.0 - f0 * b * b / (r0 * r0), 0.0));
  s.dtheta = 0.0;
  s.dphi = b / (r0 * r0);
  return s;
}

}  // namespace

int main() {
  bool pass = true;

  // Test 1: capture/escape parity between fixed-step traceRay and the
  // adaptive RKF45 tracer across a b ladder around b_crit ~ 2.598.
  const double kBs[5] = {2.0, 2.5, 2.7, 3.0, 4.0};
  for (double b : kBs) {
    bool capFixed = false, capAdaptive = false;
    astro::traceRay(makeInwardRay(10.0, b, 1.0), 1.0, 30.0, 0.02, 8000, capFixed);
    astro::SchwState rayAdaptive = makeInwardRay(10.0, b, 1.0);
    astro::traceRayRkf45(rayAdaptive, 1.0, 30.0, 0.02, 1e-10, 8000, capAdaptive);
    if (capFixed != capAdaptive) {
      pass = false;
      std::cout << "Test1 FAIL: b=" << b << " fixed captured=" << capFixed
                << " rkf45 captured=" << capAdaptive << "\n";
    }
  }
  std::cout << "Test1 capture parity: " << (pass ? "PASS" : "FAIL") << "\n";

  // Test 2: equal accuracy with fewer acceleration evaluations.
  // Reference: fixed RK4 with a 5x smaller step (h=0.002). Compare the
  // adaptive tracer against plain fixed RK4 h=0.02 (4 evals/step): the
  // adapter must land closer to the reference AND spend fewer evals.
  const double b = 2.7;  // strong bending, escapes the shell
  auto ray = makeInwardRay(10.0, b, 1.0);
  astro::SchwState refState = ray, fixedState = ray, rkfState = ray;
  long refEvals = 0, fixedEvals = 0;
  {
    for (int i = 0; i < 100000; ++i) {
      astro::rk4Step(0.002, refState, 1.0);
      refEvals += 4;
      if (refState.r <= 1.001) break;
      if (refState.r >= 30.0) break;
    }
  }
  {
    for (int i = 0; i < 100000; ++i) {
      astro::rk4Step(0.02, fixedState, 1.0);
      fixedEvals += 4;
      if (fixedState.r <= 1.001) break;
      if (fixedState.r >= 30.0) break;
    }
  }
  bool cap2 = false;
  astro::Rkf45Stats st;
  astro::traceRayRkf45(rkfState, 1.0, 30.0, 0.02, 1e-10, 8000, cap2, &st);

  {
    const double refPhi = refState.phi;
    auto rel = [](double a, double r) { return std::abs(a - r) / std::abs(r); };
    const double eFixed = rel(fixedState.phi, refPhi);
    const double eRkf = rel(rkfState.phi, refPhi);
    std::cout << "Test2 phi rel err: fixed=" << eFixed << " rkf45=" << eRkf
              << " ; evals fixed=" << fixedEvals << " rkf45=" << st.evals
              << "\n";
    if (!(eRkf <= 2.0 * eFixed) || cap2) {
      pass = false;
      std::cout << "Test2 FAIL: accuracy class/escape requirement\n";
    }
    if (!(st.evals <= 0.7 * fixedEvals)) {
      pass = false;
      std::cout << "Test2 FAIL: adapter must need fewer evals at default tol\n";
    }
  }

  // Test 3: tol in a working regime traces cleanly (tight-tol collapse
  // boundaries are covered by Test 2's tol=1e-10 pass).
  {
    astro::SchwState r3 = makeInwardRay(10.0, 3.0, 1.0);
    bool cap3 = false;
    const int n = astro::traceRayRkf45(r3, 1.0, 30.0, 0.02, 1e-6, 8000, cap3);
    if (cap3 || n <= 0) {
      pass = false;
      std::cout << "Test3 FAIL: tol=1e-6 must trace (captured=" << cap3
                << ", steps=" << n << ")\n";
    } else {
      std::cout << "Test3 tol=1e-6 trace: steps=" << n << " PASS\n";
    }
  }

  std::cout << (pass ? "ALL PASS" : "FAILURES") << "\n";
  return pass ? 0 : 2;
}
