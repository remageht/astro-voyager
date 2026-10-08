#include "Binet.h"
#include <cmath>
#include <algorithm>

namespace astro {

bool binetIsCaptured(double b, double rs) {
  const double bCrit = 1.5 * std::sqrt(3.0) * rs;
  return b < bCrit;
}

BinetOrbitResult binetPhotonOrbit(double b, double rs, double r0,
                                  double dphi, double phiMax,
                                  double rEscape) {
  BinetOrbitResult result;
  if (r0 <= rs || b <= 0.0 || rs <= 0.0) {
    result.captured = true;
    return result;
  }

  const double M = 0.5 * rs;
  // Effective potential V_eff(u) = u^2 * (1 - 2M u)
  // 1/b^2 = (du/dphi)^2 + V_eff(u)
  const double invB2 = 1.0 / (b * b);
  const double u0 = 1.0 / r0;
  const double vEff0 = u0 * u0 * (1.0 - 2.0 * M * u0);
  const double diff0 = invB2 - vEff0;

  if (diff0 < 0.0) {
    // Cannot even exist or propagate inward at r0 with this b
    return result;
  }

  // Initial inward direction: dr/dphi < 0 ==> du/dphi > 0
  double phi = 0.0;
  double u = u0;
  double w = std::sqrt(std::max(0.0, diff0));  // w = du/dphi

  const double uHorizon = 1.0 / (rs * 1.001);  // matches traceRay criterion (r <= rs * 1.001)
  const double uEscape = 1.0 / rEscape;
  const double step = (dphi > 0.0) ? dphi : 0.001;

  result.points.push_back({phi, u, 1.0 / u, w});

  // Second-order ODE system:
  // du/dphi = w
  // dw/dphi = 3 M u^2 - u
  auto accel = [M](double uVal) {
    return 3.0 * M * uVal * uVal - uVal;
  };

  bool reachedTurn = false;
  double uMax = u0;

  while (phi < phiMax) {
    // Termination checks:
    // 1. Horizon capture
    if (u >= uHorizon) {
      result.captured = true;
      break;
    }
    // 2. Escape: ray has turned around (w < 0) and reached r >= rEscape (u <= uEscape)
    if (reachedTurn && u <= uEscape) {
      result.escaped = true;
      break;
    }
    // 3. Complete escape to infinity
    if (u <= 0.0) {
      result.escaped = true;
      break;
    }

    // Single step RK4 for [u, w]
    double k1_u = w;
    double k1_w = accel(u);

    double u2 = u + 0.5 * step * k1_u;
    double w2 = w + 0.5 * step * k1_w;
    double k2_u = w2;
    double k2_w = accel(u2);

    double u3 = u + 0.5 * step * k2_u;
    double w3 = w + 0.5 * step * k2_w;
    double k3_u = w3;
    double k3_w = accel(u3);

    double u4 = u + step * k3_u;
    double w4 = w + step * k3_w;
    double k4_u = w4;
    double k4_w = accel(u4);

    double prevW = w;
    double nextU = u + (step / 6.0) * (k1_u + 2.0 * k2_u + 2.0 * k3_u + k4_u);
    double nextW = w + (step / 6.0) * (k1_w + 2.0 * k2_w + 2.0 * k3_w + k4_w);

    // Detect periapsis (turning point) where w crosses from positive to negative
    if (!reachedTurn && prevW > 0.0 && nextW <= 0.0) {
      reachedTurn = true;
      // Linear interpolation for turning phi
      double frac = prevW / (prevW - nextW);
      result.phiTurn = phi + frac * step;
      result.rMin = 1.0 / std::max(nextU, u);
    }

    u = nextU;
    w = nextW;
    phi += step;

    if (u > uMax) uMax = u;

    result.points.push_back({phi, u, (u > 0.0 ? 1.0 / u : 1e9), w});
  }

  if (result.rMin < 0.0 && reachedTurn) {
    result.rMin = 1.0 / uMax;
  }

  return result;
}

}  // namespace astro
