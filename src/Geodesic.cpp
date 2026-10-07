#include "Geodesic.h"
#include <algorithm>
#include <cmath>

namespace astro {

SchwAccel geodesicAcceleration(const SchwState& s, double rs) {
  const double kPi = 3.141592653589793;
  const double radius = std::max(s.r, rs + 0.001);
  const double theta = std::min(std::max(s.theta, 0.0001), kPi - 0.0001);
  const double sinT = std::sin(theta);
  const double cosT = std::cos(theta);
  const double safeSin = std::abs(sinT) < 1e-4 ? 1e-4 : sinT;
  const double cot = cosT / safeSin;
  const double rMinusRs = std::max(radius - rs, 1e-4);

  SchwAccel a;
  a.ddt = -(s.dr * rs * s.dt) / (radius * rMinusRs);
  a.ddr = rMinusRs *
              (s.dphi * sinT * (s.dphi * sinT) + s.dtheta * s.dtheta -
               (rs * s.dt * s.dt) / (2.0 * radius * radius * radius)) +
          (rs * s.dr * s.dr) / (2.0 * radius * rMinusRs);
  a.ddtheta =
      s.dphi * s.dphi * sinT * cosT - (2.0 * s.dr * s.dtheta) / radius;
  a.ddphi =
      -2.0 * s.dphi * s.dtheta * cot - (2.0 * s.dr * s.dphi) / radius;
  return a;
}

void rk4Step(double h, SchwState& s, double rs) {
  const double kPi = 3.141592653589793;
  auto deriv = [&](const SchwState& st, SchwState& dP, SchwAccel& dDp) {
    dP.t = st.dt;
    dP.r = st.dr;
    dP.theta = st.dtheta;
    dP.phi = st.dphi;
    dDp = geodesicAcceleration(st, rs);
  };
  SchwState k1P, k2P, k3P, k4P;
  SchwAccel k1D, k2D, k3D, k4D;
  SchwState tmp = s;
  deriv(tmp, k1P, k1D);
  tmp = s;
  tmp.t += 0.5 * h * k1P.t;
  tmp.r += 0.5 * h * k1P.r;
  tmp.theta += 0.5 * h * k1P.theta;
  tmp.phi += 0.5 * h * k1P.phi;
  tmp.dt += 0.5 * h * k1D.ddt;
  tmp.dr += 0.5 * h * k1D.ddr;
  tmp.dtheta += 0.5 * h * k1D.ddtheta;
  tmp.dphi += 0.5 * h * k1D.ddphi;
  deriv(tmp, k2P, k2D);
  tmp = s;
  tmp.t += 0.5 * h * k2P.t;
  tmp.r += 0.5 * h * k2P.r;
  tmp.theta += 0.5 * h * k2P.theta;
  tmp.phi += 0.5 * h * k2P.phi;
  tmp.dt += 0.5 * h * k2D.ddt;
  tmp.dr += 0.5 * h * k2D.ddr;
  tmp.dtheta += 0.5 * h * k2D.ddtheta;
  tmp.dphi += 0.5 * h * k2D.ddphi;
  deriv(tmp, k3P, k3D);
  tmp = s;
  tmp.t += h * k3P.t;
  tmp.r += h * k3P.r;
  tmp.theta += h * k3P.theta;
  tmp.phi += h * k3P.phi;
  tmp.dt += h * k3D.ddt;
  tmp.dr += h * k3D.ddr;
  tmp.dtheta += h * k3D.ddtheta;
  tmp.dphi += h * k3D.ddphi;
  deriv(tmp, k4P, k4D);

  s.t += (h / 6.0) * (k1P.t + 2 * k2P.t + 2 * k3P.t + k4P.t);
  s.r += (h / 6.0) * (k1P.r + 2 * k2P.r + 2 * k3P.r + k4P.r);
  s.theta += (h / 6.0) * (k1P.theta + 2 * k2P.theta + 2 * k3P.theta + k4P.theta);
  s.phi += (h / 6.0) * (k1P.phi + 2 * k2P.phi + 2 * k3P.phi + k4P.phi);
  s.dt += (h / 6.0) * (k1D.ddt + 2 * k2D.ddt + 2 * k3D.ddt + k4D.ddt);
  s.dr += (h / 6.0) * (k1D.ddr + 2 * k2D.ddr + 2 * k3D.ddr + k4D.ddr);
  s.dtheta +=
      (h / 6.0) * (k1D.ddtheta + 2 * k2D.ddtheta + 2 * k3D.ddtheta + k4D.ddtheta);
  s.dphi +=
      (h / 6.0) * (k1D.ddphi + 2 * k2D.ddphi + 2 * k3D.ddphi + k4D.ddphi);
  s.theta = std::min(std::max(s.theta, 0.0001), kPi - 0.0001);
}

namespace {
inline double smoothstep(double edge0, double edge1, double x) {
  if (edge0 >= edge1) return 0.0;
  double t = std::clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
  return t * t * (3.0 - 2.0 * t);
}
}  // namespace

int traceRay(SchwState s, double rs, double shell, double step, int maxSteps,
             bool& captured) {
  captured = false;
  const double hNear = step;
  const double hFar = step * 2.5;
  for (int i = 0; i < maxSteps; ++i) {
    const double smoothFactor = 1.0 - smoothstep(3.0 * rs, 8.0 * rs, s.r);
    const double h = hFar + (hNear - hFar) * smoothFactor;
    rk4Step(h, s, rs);
    if (s.r <= rs * 1.001) {
      captured = true;
      return i + 1;
    }
    if (s.r >= shell) return i + 1;
  }
  return maxSteps;
}

}  // namespace astro
