#include "Kerr.h"
#include <cmath>
#include <algorithm>

namespace astro {

double kerrPhotonSphereRadius(double a, double rs, bool prograde) {
  const double M = 0.5 * rs;
  const double aClamped = std::clamp(std::abs(a), 0.0, M);
  if (aClamped < 1e-15) {
    return 3.0 * M;
  }
  const double sign = prograde ? 1.0 : -1.0;
  const double ratio = aClamped / M;
  // Bardeen (1972) formula: r_ph = 2M [1 + cos((2/3) arccos(-sign * a / M))]
  const double arg = -sign * ratio;
  const double theta = std::acos(std::clamp(arg, -1.0, 1.0));
  return 2.0 * M * (1.0 + std::cos((2.0 / 3.0) * theta));
}

double kerrBCrit(double a, double rs, bool prograde) {
  const double M = 0.5 * rs;
  const double aClamped = std::clamp(std::abs(a), 0.0, M);
  if (aClamped < 1e-15) {
    return 1.5 * std::sqrt(3.0) * rs;  // 3 * sqrt(3) * M
  }
  const double r_ph = kerrPhotonSphereRadius(aClamped, rs, prograde);
  const double num = -(r_ph * r_ph * r_ph - 3.0 * M * r_ph * r_ph +
                       aClamped * aClamped * r_ph + aClamped * aClamped * M);
  const double den = aClamped * (r_ph - M);
  const double xi = num / den;
  return std::abs(xi);
}

KerrOrbitResult kerrPhotonOrbit(double b, double a, double rs,
                                double r0, double step,
                                int maxSteps, double rEscape) {
  KerrOrbitResult res;
  const double M = 0.5 * rs;
  const double aMag = std::abs(a);
  const double aClamped = std::min(aMag, M);
  const double rHorizon = M + std::sqrt(std::max(0.0, M * M - aClamped * aClamped));
  const double rHorizonCutoff = rHorizon * 1.001;

  if (r0 <= rHorizonCutoff || rs <= 0.0) {
    res.captured = true;
    return res;
  }

  // Energy is normalized to E = 1.
  // Radial potential R(r) = [r^2 + a(a - b)]^2 - Delta * (b - a)^2
  // = r^4 + (a^2 - b^2) r^2 + 2 M r (b - a)^2
  auto calcR = [M, a, b](double r) {
    const double r2 = r * r;
    const double diff = b - a;
    return r2 * r2 + (a * a - b * b) * r2 + 2.0 * M * r * diff * diff;
  };

  auto calcRPrime = [M, a, b](double r) {
    const double diff = b - a;
    return 4.0 * r * r * r + 2.0 * (a * a - b * b) * r + 2.0 * M * diff * diff;
  };

  // State: [r, vr, phi] where vr = dr/dlambda
  double r = r0;
  const double R0 = calcR(r0);
  if (R0 < 0.0) {
    // Kinematically forbidden
    return res;
  }

  // Launch inward: vr = - sqrt(R(r0)) / r0^2
  double vr = -std::sqrt(std::max(0.0, R0)) / (r0 * r0);
  double phi = 0.0;
  double lambda = 0.0;

  auto dphi_dlambda = [M, a, b](double rVal) {
    const double Delta = rVal * rVal - 2.0 * M * rVal + a * a;
    const double safeDelta = std::max(Delta, 1e-6);
    // r^2 dphi/dlambda = (a / Delta) * [ r^2 + a(a - b) ] + (b - a)
    // = [ a (r^2 + a^2 - a b) + (b - a) Delta ] / Delta
    const double P = rVal * rVal + a * (a - b);
    const double num = a * P / safeDelta + (b - a);
    return num / (rVal * rVal);
  };

  auto r_accel = [calcR, calcRPrime](double rVal) {
    const double r2 = rVal * rVal;
    const double r4 = r2 * r2;
    const double r5 = r4 * rVal;
    return 0.5 * calcRPrime(rVal) / r4 - 2.0 * calcR(rVal) / r5;
  };

  res.points.push_back({lambda, r, phi, vr, dphi_dlambda(r)});
  res.rMin = r0;

  for (int i = 0; i < maxSteps; ++i) {
    if (r <= rHorizonCutoff) {
      res.captured = true;
      break;
    }
    if (vr > 0.0 && (r >= rEscape || r >= r0)) {
      res.escaped = true;
      break;
    }

    // RK4 step for [r, vr, phi]
    double k1_r = vr;
    double k1_vr = r_accel(r);
    double k1_phi = dphi_dlambda(r);

    double r_mid1 = r + 0.5 * step * k1_r;
    double vr_mid1 = vr + 0.5 * step * k1_vr;
    double k2_r = vr_mid1;
    double k2_vr = r_accel(r_mid1);
    double k2_phi = dphi_dlambda(r_mid1);

    double r_mid2 = r + 0.5 * step * k2_r;
    double vr_mid2 = vr + 0.5 * step * k2_vr;
    double k3_r = vr_mid2;
    double k3_vr = r_accel(r_mid2);
    double k3_phi = dphi_dlambda(r_mid2);

    double r_end = r + step * k3_r;
    double vr_end = vr + step * k3_vr;
    double k4_r = vr_end;
    double k4_vr = r_accel(r_end);
    double k4_phi = dphi_dlambda(r_end);

    r += (step / 6.0) * (k1_r + 2.0 * k2_r + 2.0 * k3_r + k4_r);
    vr += (step / 6.0) * (k1_vr + 2.0 * k2_vr + 2.0 * k3_vr + k4_vr);
    phi += (step / 6.0) * (k1_phi + 2.0 * k2_phi + 2.0 * k3_phi + k4_phi);
    lambda += step;

    if (r < res.rMin) {
      res.rMin = r;
    }

    res.points.push_back({lambda, r, phi, vr, dphi_dlambda(r)});
  }

  return res;
}

}  // namespace astro
