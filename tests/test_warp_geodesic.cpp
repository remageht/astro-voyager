#include <iostream>
#include <cmath>
#include <cassert>
#include "WarpMetric.h"
#include "WarpGeodesic.h"
#include "Geodesic.h"

int main() {
  std::cout << "=== Running test_warp_geodesic (M1 ADM & Geodesic Engine) ===\n";

  const double rs = 1.0;
  const double r0 = 10.0;
  const double bCrit = 1.5 * std::sqrt(3.0) * rs;  // ~ 2.598076 Rs
  astro::SchwarzschildADM metric(rs);

  // 1. Metric ADM inversion test: gamma^{ik} * gamma_{kj} == delta^i_j
  {
    double alpha = 0.0;
    double beta[3] = {0.0, 0.0, 0.0};
    double g_ij[3][3], g_inv[3][3];
    metric.evalADM(3.0, 4.0, 5.0, 0.0, alpha, beta, g_ij, g_inv);

    double maxErr = 0.0;
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        double sum = 0.0;
        for (int k = 0; k < 3; ++k) {
          sum += g_inv[i][k] * g_ij[k][j];
        }
        const double delta = (i == j) ? 1.0 : 0.0;
        const double err = std::abs(sum - delta);
        if (err > maxErr) maxErr = err;
      }
    }
    std::cout << "Test 1 (gamma * gamma_inv = I): max_err=" << maxErr;
    if (maxErr < 1e-10) {
      std::cout << " -> PASS\n";
    } else {
      std::cout << " -> FAIL\n";
      return 1;
    }
  }

  // 2. Analytical vs Numerical derivatives of g^{mu nu}
  {
    double g_inv[4][4];
    metric.evalInv4Metric(4.0, -3.0, 2.0, 0.0, g_inv);

    double dg_analytic[3][4][4];
    metric.evalDerivInv4Metric(4.0, -3.0, 2.0, 0.0, dg_analytic);

    double dg_numeric[3][4][4];
    // Call base class finite difference implementation
    metric.WarpMetric::evalDerivInv4Metric(4.0, -3.0, 2.0, 0.0, dg_numeric);

    double maxDiff = 0.0;
    for (int k = 0; k < 3; ++k) {
      for (int mu = 0; mu < 4; ++mu) {
        for (int nu = 0; nu < 4; ++nu) {
          const double diff = std::abs(dg_analytic[k][mu][nu] - dg_numeric[k][mu][nu]);
          if (diff > maxDiff) maxDiff = diff;
        }
      }
    }
    std::cout << "Test 2 (analytic vs numeric dg/dx): max_diff=" << maxDiff;
    if (maxDiff < 1e-5) {
      std::cout << " -> PASS\n";
    } else {
      std::cout << " -> FAIL\n";
      return 1;
    }
  }

  // 3. Hamiltonian conservation along trajectory (H = 1/2 g^{mu nu} p_mu p_nu ~ 0)
  {
    astro::WarpRayState ray = astro::makeWarpInwardRay(metric, r0, 3.0);
    double maxH = 0.0;
    for (int step = 0; step < 1000; ++step) {
      astro::warpRk4Step(metric, 0.01, ray);
      const double hVal = std::abs(astro::evalHamiltonian(metric, ray));
      if (hVal > maxH) maxH = hVal;
    }
    std::cout << "Test 3 (Hamiltonian constraint drift over 1000 steps): " << maxH;
    if (maxH < 1e-6) {
      std::cout << " (< 1e-6 -> PASS)\n";
    } else {
      std::cout << " (FAIL)\n";
      return 1;
    }
  }

  // 4. Parity with Geodesic.cpp across various impact parameters b
  const double testBs[] = {1.5, 2.0, 2.5, 2.7, 3.0, 5.0};
  for (double b : testBs) {
    const bool expectCapture = (b < bCrit);

    // ADM Hamiltonian raytrace
    astro::WarpRayState warpRay = astro::makeWarpInwardRay(metric, r0, b);
    astro::WarpTraceResult warpRes = astro::traceWarpRay(metric, warpRay, 30.0, 0.01, 4000);

    // Geodesic.cpp RK4 reference
    // Tangent/inward ray with same b
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

    const bool match = (warpRes.captured == expectCapture) && (warpRes.captured == geoCap);
    std::cout << "b=" << b << " Rs: ADM_Hamiltonian="
              << (warpRes.captured ? "capture" : "escape")
              << " Geodesic_RK4=" << (geoCap ? "capture" : "escape")
              << " expected=" << (expectCapture ? "capture" : "escape")
              << " -> " << (match ? "PASS" : "FAIL") << "\n";
    if (!match) return 1;
  }

  // 5. Critical boundary sensitivity (b_crit +/- 0.005 Rs)
  const double bUnder = 2.598 - 0.005;  // 2.593 < b_crit -> capture
  const double bOver = 2.598 + 0.005;   // 2.603 > b_crit -> escape

  astro::WarpTraceResult resUnder = astro::traceWarpRay(
      metric, astro::makeWarpInwardRay(metric, r0, bUnder), 30.0, 0.005, 6000);
  astro::WarpTraceResult resOver = astro::traceWarpRay(
      metric, astro::makeWarpInwardRay(metric, r0, bOver), 30.0, 0.005, 6000);

  const bool underOk = resUnder.captured;
  const bool overOk = !resOver.captured;
  std::cout << "Critical threshold b=" << bUnder << " (< b_crit): captured=" << resUnder.captured
            << " -> " << (underOk ? "PASS" : "FAIL") << "\n";
  std::cout << "Critical threshold b=" << bOver << " (> b_crit): captured=" << resOver.captured
            << " -> " << (overOk ? "PASS" : "FAIL") << "\n";

  if (!underOk || !overOk) return 1;

  std::cout << "\nAll test_warp_geodesic checks passed successfully!\n";
  return 0;
}
