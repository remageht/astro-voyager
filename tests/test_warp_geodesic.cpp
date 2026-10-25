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

  // 6. Alcubierre Metric: g^{00} = -1 identically (lapse alpha = 1) and g_00 = -1 at rest/exterior
  {
    astro::AlcubierreMetric alc(5.0, 0.5, 2.0);
    double g_inv[4][4];
    alc.evalInv4Metric(0.0, 0.0, 0.0, 0.0, g_inv);
    const bool g00_inv_ok = (std::abs(g_inv[0][0] - (-1.0)) < 1e-12);

    double alpha = 0.0, beta[3];
    double g_ij[3][3], g_inv3[3][3];
    // At t=0 (at rest, v=0)
    alc.evalADM(0.0, 0.0, 0.0, 0.0, alpha, beta, g_ij, g_inv3);
    const double g00_rest = -alpha * alpha + (beta[0]*beta[0] + beta[1]*beta[1] + beta[2]*beta[2]);
    const bool g00_rest_ok = (std::abs(g00_rest - (-1.0)) < 1e-12);

    // Far outside the bubble (r_s >> R + 3*sigma) at t = 50.0 (where v > 0)
    alc.evalADM(100.0, 100.0, 100.0, 50.0, alpha, beta, g_ij, g_inv3);
    const double g00_ext = -alpha * alpha + (beta[0]*beta[0] + beta[1]*beta[1] + beta[2]*beta[2]);
    const bool g00_ext_ok = (std::abs(g00_ext - (-1.0)) < 1e-6);

    const bool test6Ok = g00_inv_ok && g00_rest_ok && g00_ext_ok;
    std::cout << "Test 6 (Alcubierre g00 = -1 at rest/exterior & g^00 = -1): "
              << "g^00=" << g_inv[0][0] << ", g00(rest)=" << g00_rest << ", g00(ext)=" << g00_ext
              << " -> " << (test6Ok ? "PASS" : "FAIL") << "\n";
    if (!test6Ok) return 1;
  }

  // 7. Shape function: f(0) = 1 at center and f(rs) ~ 0 for rs > R + 3*sigma
  {
    const double R = 4.0;
    const double sigma = 0.5;
    astro::AlcubierreMetric alc(R, sigma, 1.0);

    const double f_center = alc.shapeFunction(0.0);
    const double f_mid = alc.shapeFunction(R);
    const double f_out3 = alc.shapeFunction(R + 3.0 * sigma);
    const double f_out5 = alc.shapeFunction(R + 5.0 * sigma);

    const bool centerOk = (std::abs(f_center - 1.0) < 1e-12);
    const bool midOk = (std::abs(f_mid - 0.5) < 0.05); // near wall midpoint
    const bool out3Ok = (f_out3 < 0.01);               // exponential dropoff
    const bool out5Ok = (f_out5 < 1e-4);

    const bool test7Ok = centerOk && midOk && out3Ok && out5Ok;
    std::cout << "Test 7 (Shape function f(0)=" << f_center << ", f(R)=" << f_mid
              << ", f(R+3s)=" << f_out3 << ", f(R+5s)=" << f_out5 << "): "
              << (test7Ok ? "PASS" : "FAIL") << "\n";
    if (!test7Ok) return 1;
  }

  // 8. York extrinsic curvature: max(theta) matches analytical formula theta_max = |v| / (2*sigma*tanh(R/sigma))
  {
    const double R = 5.0;
    const double sigma = 0.5;
    const double v = 3.0;
    astro::AlcubierreMetric alc(R, sigma, v, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 100.0, 10.0);

    const double t_mid = 50.0; // peak velocity v = 3.0
    const double theta_analytic = alc.thetaMax(t_mid);

    // Numerically sample theta along x-axis through bubble center
    double center[3];
    alc.bubbleCenter(t_mid, center);
    double maxThetaNum = 0.0;
    for (double x = center[0] - 2.0 * R; x <= center[0] + 2.0 * R; x += 0.005) {
      const double th = std::abs(alc.yorkTime(x, center[1], center[2], t_mid));
      if (th > maxThetaNum) maxThetaNum = th;
    }

    const double diff = std::abs(maxThetaNum - theta_analytic);
    const double relErr = diff / theta_analytic;
    const bool test8Ok = (relErr < 0.002); // within 0.2%
    std::cout << "Test 8 (York time max(theta): numeric=" << maxThetaNum
              << " analytic=" << theta_analytic << " rel_err=" << relErr << "): "
              << (test8Ok ? "PASS" : "FAIL") << "\n";
    if (!test8Ok) return 1;
  }

  // 9. Analytical vs Numerical derivatives of Alcubierre inverse 4-metric
  {
    astro::AlcubierreMetric alc(4.0, 0.5, 2.5, 0.0, 0.0, 0.0, 0.6, 0.8, 0.0, 100.0, 15.0);
    double dg_an[3][4][4], dg_num[3][4][4];
    alc.evalDerivInv4Metric(2.5, 3.0, 0.5, 30.0, dg_an);
    alc.WarpMetric::evalDerivInv4Metric(2.5, 3.0, 0.5, 30.0, dg_num);

    double maxDiffAlcubierre = 0.0;
    for (int k = 0; k < 3; ++k) {
      for (int mu = 0; mu < 4; ++mu) {
        for (int nu = 0; nu < 4; ++nu) {
          const double d = std::abs(dg_an[k][mu][nu] - dg_num[k][mu][nu]);
          if (d > maxDiffAlcubierre) maxDiffAlcubierre = d;
        }
      }
    }
    const bool test9Ok = (maxDiffAlcubierre < 1e-5);
    std::cout << "Test 9 (Alcubierre analytic vs numeric dg/dx: max_diff="
              << maxDiffAlcubierre << "): " << (test9Ok ? "PASS" : "FAIL") << "\n";
    if (!test9Ok) return 1;
  }

  // 10. WarpGeodesic::skiffPath roundtrip symmetry, |dts/dtau| <= 1, and wall safety
  {
    const double pA[3] = {10.0, -20.0, 5.0};
    const double pB[3] = {110.0, 30.0, -45.0};
    const double vMax = 8.0;
    const double sigma = 0.5;
    const int N = 60;

    std::vector<astro::SkiffWaypoint> pathAB = astro::WarpGeodesic::skiffPath(pA, pB, vMax, sigma, N);
    std::vector<astro::SkiffWaypoint> pathBA = astro::WarpGeodesic::skiffPath(pB, pA, vMax, sigma, N);

    bool allWallSafe = true;
    bool allDtauValid = true;
    double maxSymPosDiff = 0.0;
    double maxSymVelDiff = 0.0;
    double maxSymThetaDiff = 0.0;

    for (int k = 0; k < N; ++k) {
      if (!pathAB[k].wallSafe || !pathBA[k].wallSafe) allWallSafe = false;
      if (!pathAB[k].dtsDtauValid || !pathBA[k].dtsDtauValid) allDtauValid = false;

      // Roundtrip symmetry: pathAB[k] should correspond to pathBA[N - 1 - k]
      const int opp = N - 1 - k;
      const double dx = pathAB[k].x - pathBA[opp].x;
      const double dy = pathAB[k].y - pathBA[opp].y;
      const double dz = pathAB[k].z - pathBA[opp].z;
      const double posDiff = std::sqrt(dx * dx + dy * dy + dz * dz);
      if (posDiff > maxSymPosDiff) maxSymPosDiff = posDiff;

      const double velDiff = std::abs(pathAB[k].v - pathBA[k].v);
      if (velDiff > maxSymVelDiff) maxSymVelDiff = velDiff;

      const double thetaDiff = std::abs(pathAB[k].thetaMax - pathBA[k].thetaMax);
      if (thetaDiff > maxSymThetaDiff) maxSymThetaDiff = thetaDiff;
    }

    const bool endpointsOk = (std::abs(pathAB.front().x - pA[0]) < 1e-10) &&
                             (std::abs(pathAB.back().x - pB[0]) < 1e-10);
    const bool symOk = (maxSymPosDiff < 1e-10) && (maxSymVelDiff < 1e-10) && (maxSymThetaDiff < 1e-10);
    const bool test10Ok = allWallSafe && allDtauValid && endpointsOk && symOk;

    std::cout << "Test 10 (skiffPath roundtrip symmetry & safety: "
              << "pos_sym_err=" << maxSymPosDiff << ", vel_sym_err=" << maxSymVelDiff
              << ", theta_sym_err=" << maxSymThetaDiff << ", wallSafe=" << allWallSafe
              << ", dts/dtau=" << allDtauValid << "): " << (test10Ok ? "PASS" : "FAIL") << "\n";
    if (!test10Ok) return 1;
  }

  // 11. Warp blueshift and Killing charge conservation in moving Alcubierre bubble (Issue N1)
  {
    const double R = 5.0;
    const double sigma = 0.5;
    const double vMax = 0.5;
    const double T = 10000.0;
    const double tau = 1000.0;
    astro::AlcubierreMetric alc(R, sigma, vMax, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, T, tau);

    // Mid-cruise state at t0 = 5000.0 where v(t) = vMax = 0.5
    const double t0 = 5000.0;
    double center0[3];
    alc.bubbleCenter(t0, center0);

    // Launch photon 20 units ahead of the bubble, propagating in -X direction
    astro::WarpRayState ray;
    ray.t = t0;
    ray.x = center0[0] + 20.0;
    ray.y = center0[1];
    ray.z = center0[2];
    ray.pt = -1.0;
    ray.px = -1.0;  // In Minkowski exterior, null condition gives px = -1
    ray.py = 0.0;
    ray.pz = 0.0;
    ray.lambda = 0.0;

    const double vInit = alc.velocity(ray.t);
    const double C_initial = ray.pt + vInit * ray.px;  // Expected C = -1.0 + 0.5*(-1.0) = -1.5

    double maxCDrift = 0.0;
    bool reachedCenter = false;
    double finalECom = 0.0;

    const double h = 0.01;
    for (int step = 0; step < 4000; ++step) {
      astro::warpRk4Step(alc, h, ray);

      double curCenter[3];
      alc.bubbleCenter(ray.t, curCenter);
      const double curV = alc.velocity(ray.t);
      const double C = ray.pt + curV * ray.px;
      const double drift = std::abs(C - C_initial);
      if (drift > maxCDrift) maxCDrift = drift;

      // Check when photon passes through the bubble center
      if (std::abs(ray.x - curCenter[0]) < 0.05 && !reachedCenter) {
        reachedCenter = true;
        finalECom = -(ray.pt + curV * ray.px);
      }
    }

    const double expectedECom = 1.0 + vMax;  // 1.5
    const double eComErr = std::abs(finalECom - expectedECom);
    const bool test11Ok = reachedCenter && (maxCDrift < 1e-6) && (eComErr < 1e-5);

    std::cout << "Test 11 (Alcubierre blueshift & Killing charge: reachedCenter=" << reachedCenter
              << " E_com=" << finalECom << " expected=" << expectedECom
              << " max_C_drift=" << maxCDrift << "): "
              << (test11Ok ? "PASS" : "FAIL") << "\n";
    if (!test11Ok) return 1;
  }

  // 12. Analytical vs Numerical time derivative of Alcubierre inverse 4-metric
  {
    astro::AlcubierreMetric alc(4.0, 0.5, 2.5, 1.0, 2.0, 3.0, 0.6, 0.8, 0.0, 100.0, 15.0);
    double dg_dt_an[4][4], dg_dt_num[4][4];
    alc.evalDerivTimeInv4Metric(2.5, 3.0, 0.5, 30.0, dg_dt_an);
    alc.WarpMetric::evalDerivTimeInv4Metric(2.5, 3.0, 0.5, 30.0, dg_dt_num);

    double maxDiffTime = 0.0;
    for (int mu = 0; mu < 4; ++mu) {
      for (int nu = 0; nu < 4; ++nu) {
        const double d = std::abs(dg_dt_an[mu][nu] - dg_dt_num[mu][nu]);
        if (d > maxDiffTime) maxDiffTime = d;
      }
    }
    const bool test12Ok = (maxDiffTime < 1e-5);
    std::cout << "Test 12 (Alcubierre analytic vs numeric dg/dt: max_diff="
              << maxDiffTime << "): " << (test12Ok ? "PASS" : "FAIL") << "\n";
    if (!test12Ok) return 1;
  }

  // 13. Analytical bubbleCenter vs high-precision numerical Simpson quadrature (Issue N4)
  {
    const double invSqrt3 = 1.0 / std::sqrt(3.0);
    astro::AlcubierreMetric alc(5.0, 0.5, 3.0, 5.0, -10.0, 15.0, invSqrt3, invSqrt3, invSqrt3, 120.0, 18.0);
    const double testTimes[] = {0.0, 1.5, 18.0, 60.0, 110.0, 120.0, 150.0};
    double maxCenterDiff = 0.0;

    for (double t : testTimes) {
      double cAna[3];
      alc.bubbleCenter(t, cAna);

      // Numerical Simpson integration with 50000 intervals
      const double tClamped = std::min(t, alc.totalTime());
      double sNum = 0.0;
      if (tClamped > 0.0) {
        const int N = 50000;
        const double dt = tClamped / N;
        double sum = alc.velocity(0.0) + alc.velocity(tClamped);
        for (int i = 1; i < N; ++i) {
          const double ti = i * dt;
          const double w = (i % 2 == 1) ? 4.0 : 2.0;
          sum += w * alc.velocity(ti);
        }
        sNum = (dt / 3.0) * sum;
      }
      const double cNum[3] = {
          5.0 + invSqrt3 * sNum,
          -10.0 + invSqrt3 * sNum,
          15.0 + invSqrt3 * sNum
      };

      for (int i = 0; i < 3; ++i) {
        const double d = std::abs(cAna[i] - cNum[i]);
        if (d > maxCenterDiff) maxCenterDiff = d;
      }
    }

    const bool test13Ok = (maxCenterDiff < 1e-9);
    std::cout << "Test 13 (Alcubierre analytic bubbleCenter vs 50000-pt Simpson: max_diff="
              << maxCenterDiff << "): " << (test13Ok ? "PASS" : "FAIL") << "\n";
    if (!test13Ok) return 1;
  }

  std::cout << "\nAll test_warp_geodesic checks passed successfully (13/13 PASS)!\n";
  return 0;
}
