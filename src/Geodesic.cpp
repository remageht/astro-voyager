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

void renormalizeTime(SchwState& s, double rs) {
  const double kPi = 3.141592653589793;
  const double radius = std::max(s.r, rs + 0.001);
  const double theta = std::min(std::max(s.theta, 0.0001), kPi - 0.0001);
  const double sinT = std::sin(theta);
  const double safeSin = std::abs(sinT) < 1e-4 ? 1e-4 : sinT;
  const double f = std::max(1.0 - rs / radius, 1e-4);
  const double spatial = (s.dr * s.dr) / f + radius * radius * s.dtheta * s.dtheta +
                         radius * radius * safeSin * safeSin * s.dphi * s.dphi;
  s.dt = std::sqrt(std::max(spatial / f, 0.0));
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
  renormalizeTime(s, rs);
}

namespace {
inline double smoothstep(double edge0, double edge1, double x) {
  if (edge0 >= edge1) return 0.0;
  double t = std::clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
  return t * t * (3.0 - 2.0 * t);
}
}  // namespace

bool rkf45Step(double h, double tol, SchwState& s, double rs, double& hNext) {
  // Fehlberg 5(4): six stages, the 5th-order solution is propagated and the
  // 4th-order weights provide the embedded error estimate. Coefficients from
  // Fehlberg, Computing 6 (1970) 61-71, tableau for local order 5.
  const double kPi = 3.141592653589793;
  auto deriv = [&](const SchwState& st, SchwState& dP, SchwAccel& dDp) {
    dP.t = st.dt;
    dP.r = st.dr;
    dP.theta = st.dtheta;
    dP.phi = st.dphi;
    dDp = geodesicAcceleration(st, rs);
  };
  for (int attempt = 0; attempt < 60; ++attempt) {
    SchwState kP[6];
    SchwAccel kD[6];
    SchwState tmp;
    // Stage 1
    tmp = s;
    deriv(tmp, kP[0], kD[0]);
    // Stage 2 (a21 = 1/4)
    tmp = s;
    tmp.t += h * (1.0 / 4.0) * kP[0].t;
    tmp.r += h * (1.0 / 4.0) * kP[0].r;
    tmp.theta += h * (1.0 / 4.0) * kP[0].theta;
    tmp.phi += h * (1.0 / 4.0) * kP[0].phi;
    tmp.dt += h * (1.0 / 4.0) * kD[0].ddt;
    tmp.dr += h * (1.0 / 4.0) * kD[0].ddr;
    tmp.dtheta += h * (1.0 / 4.0) * kD[0].ddtheta;
    tmp.dphi += h * (1.0 / 4.0) * kD[0].ddphi;
    deriv(tmp, kP[1], kD[1]);
    // Stage 3 (a31 = 3/32, a32 = 9/32)
    tmp = s;
    tmp.t += h * (3.0 * kP[0].t / 32.0 + 9.0 * kP[1].t / 32.0);
    tmp.r += h * (3.0 * kP[0].r / 32.0 + 9.0 * kP[1].r / 32.0);
    tmp.theta += h * (3.0 * kP[0].theta / 32.0 + 9.0 * kP[1].theta / 32.0);
    tmp.phi += h * (3.0 * kP[0].phi / 32.0 + 9.0 * kP[1].phi / 32.0);
    tmp.dt += h * (3.0 * kD[0].ddt / 32.0 + 9.0 * kD[1].ddt / 32.0);
    tmp.dr += h * (3.0 * kD[0].ddr / 32.0 + 9.0 * kD[1].ddr / 32.0);
    tmp.dtheta +=
        h * (3.0 * kD[0].ddtheta / 32.0 + 9.0 * kD[1].ddtheta / 32.0);
    tmp.dphi += h * (3.0 * kD[0].ddphi / 32.0 + 9.0 * kD[1].ddphi / 32.0);
    deriv(tmp, kP[2], kD[2]);
    // Stage 4 (12/13)
    tmp = s;
    tmp.t += h * (1932.0 * kP[0].t - 7200.0 * kP[1].t + 7296.0 * kP[2].t) / 2197.0;
    tmp.r += h * (1932.0 * kP[0].r - 7200.0 * kP[1].r + 7296.0 * kP[2].r) / 2197.0;
    tmp.theta += h * (1932.0 * kP[0].theta - 7200.0 * kP[1].theta +
                      7296.0 * kP[2].theta) / 2197.0;
    tmp.phi += h * (1932.0 * kP[0].phi - 7200.0 * kP[1].phi + 7296.0 * kP[2].phi) / 2197.0;
    tmp.dt += h * (1932.0 * kD[0].ddt - 7200.0 * kD[1].ddt + 7296.0 * kD[2].ddt) / 2197.0;
    tmp.dr += h * (1932.0 * kD[0].ddr - 7200.0 * kD[1].ddr + 7296.0 * kD[2].ddr) / 2197.0;
    tmp.dtheta += h * (1932.0 * kD[0].ddtheta - 7200.0 * kD[1].ddtheta +
                       7296.0 * kD[2].ddtheta) / 2197.0;
    tmp.dphi += h * (1932.0 * kD[0].ddphi - 7200.0 * kD[1].ddphi + 7296.0 * kD[2].ddphi) / 2197.0;
    deriv(tmp, kP[3], kD[3]);
    // Stage 5 (a51 = 439/216, a52 = -8, a53 = 3680/513, a54 = -845/4104)
    tmp = s;
    tmp.t += h * (439.0 / 216.0 * kP[0].t - 8.0 * kP[1].t +
                  3680.0 / 513.0 * kP[2].t - 845.0 / 4104.0 * kP[3].t);
    tmp.r += h * (439.0 / 216.0 * kP[0].r - 8.0 * kP[1].r +
                  3680.0 / 513.0 * kP[2].r - 845.0 / 4104.0 * kP[3].r);
    tmp.theta += h * (439.0 / 216.0 * kP[0].theta - 8.0 * kP[1].theta +
                      3680.0 / 513.0 * kP[2].theta - 845.0 / 4104.0 * kP[3].theta);
    tmp.phi += h * (439.0 / 216.0 * kP[0].phi - 8.0 * kP[1].phi +
                    3680.0 / 513.0 * kP[2].phi - 845.0 / 4104.0 * kP[3].phi);
    tmp.dt += h * (439.0 / 216.0 * kD[0].ddt - 8.0 * kD[1].ddt +
                   3680.0 / 513.0 * kD[2].ddt - 845.0 / 4104.0 * kD[3].ddt);
    tmp.dr += h * (439.0 / 216.0 * kD[0].ddr - 8.0 * kD[1].ddr +
                   3680.0 / 513.0 * kD[2].ddr - 845.0 / 4104.0 * kD[3].ddr);
    tmp.dtheta += h * (439.0 / 216.0 * kD[0].ddtheta - 8.0 * kD[1].ddtheta +
                       3680.0 / 513.0 * kD[2].ddtheta - 845.0 / 4104.0 * kD[3].ddtheta);
    tmp.dphi += h * (439.0 / 216.0 * kD[0].ddphi - 8.0 * kD[1].ddphi +
                     3680.0 / 513.0 * kD[2].ddphi - 845.0 / 4104.0 * kD[3].ddphi);
    deriv(tmp, kP[4], kD[4]);
    // Stage 6 (a61 = -8/27, a62 = 2, a63 = -3544/2565, a64 = 1859/4104,
    // a65 = -11/40)
    tmp = s;
    tmp.t += h * (-8.0 / 27.0 * kP[0].t + 2.0 * kP[1].t -
                  3544.0 / 2565.0 * kP[2].t + 1859.0 / 4104.0 * kP[3].t -
                  11.0 / 40.0 * kP[4].t);
    tmp.r += h * (-8.0 / 27.0 * kP[0].r + 2.0 * kP[1].r -
                  3544.0 / 2565.0 * kP[2].r + 1859.0 / 4104.0 * kP[3].r -
                  11.0 / 40.0 * kP[4].r);
    tmp.theta += h * (-8.0 / 27.0 * kP[0].theta + 2.0 * kP[1].theta -
                      3544.0 / 2565.0 * kP[2].theta + 1859.0 / 4104.0 * kP[3].theta -
                      11.0 / 40.0 * kP[4].theta);
    tmp.phi += h * (-8.0 / 27.0 * kP[0].phi + 2.0 * kP[1].phi -
                    3544.0 / 2565.0 * kP[2].phi + 1859.0 / 4104.0 * kP[3].phi -
                    11.0 / 40.0 * kP[4].phi);
    tmp.dt += h * (-8.0 / 27.0 * kD[0].ddt + 2.0 * kD[1].ddt -
                   3544.0 / 2565.0 * kD[2].ddt + 1859.0 / 4104.0 * kD[3].ddt -
                   11.0 / 40.0 * kD[4].ddt);
    tmp.dr += h * (-8.0 / 27.0 * kD[0].ddr + 2.0 * kD[1].ddr -
                   3544.0 / 2565.0 * kD[2].ddr + 1859.0 / 4104.0 * kD[3].ddr -
                   11.0 / 40.0 * kD[4].ddr);
    tmp.dtheta += h * (-8.0 / 27.0 * kD[0].ddtheta + 2.0 * kD[1].ddtheta -
                       3544.0 / 2565.0 * kD[2].ddtheta + 1859.0 / 4104.0 * kD[3].ddtheta -
                       11.0 / 40.0 * kD[4].ddtheta);
    tmp.dphi += h * (-8.0 / 27.0 * kD[0].ddphi + 2.0 * kD[1].ddphi -
                     3544.0 / 2565.0 * kD[2].ddphi + 1859.0 / 4104.0 * kD[3].ddphi -
                     11.0 / 40.0 * kD[4].ddphi);
    deriv(tmp, kP[5], kD[5]);

    // 5th-order solution weights b5, safe order-4 error weights b4.
    const double b5[6] = {16.0 / 135.0, 0.0, 6656.0 / 12825.0,
                          28561.0 / 56430.0, -9.0 / 50.0, 2.0 / 55.0};
    const double b4[6] = {25.0 / 216.0, 0.0, 1408.0 / 2565.0,
                          2197.0 / 4104.0, -1.0 / 5.0, 0.0};
    SchwState o5, o4;
    o5 = s;
    o4 = s;
    double err2 = 0.0;
    for (int j = 0; j < 6; ++j) {
      o5.t += h * b5[j] * kP[j].t;
      o5.r += h * b5[j] * kP[j].r;
      o5.theta += h * b5[j] * kP[j].theta;
      o5.phi += h * b5[j] * kP[j].phi;
      o5.dt += h * b5[j] * kD[j].ddt;
      o5.dr += h * b5[j] * kD[j].ddr;
      o5.dtheta += h * b5[j] * kD[j].ddtheta;
      o5.dphi += h * b5[j] * kD[j].ddphi;
      o4.t += h * b4[j] * kP[j].t;
      o4.r += h * b4[j] * kP[j].r;
      o4.theta += h * b4[j] * kP[j].theta;
      o4.phi += h * b4[j] * kP[j].phi;
      o4.dt += h * b4[j] * kD[j].ddt;
      o4.dr += h * b4[j] * kD[j].ddr;
      o4.dtheta += h * b4[j] * kD[j].ddtheta;
      o4.dphi += h * b4[j] * kD[j].ddphi;
    }
    const double d[6] = {std::abs(o5.r - o4.r),        std::abs(o5.theta - o4.theta),
                         std::abs(o5.phi - o4.phi),    std::abs(o5.dr - o4.dr),
                         std::abs(o5.dtheta - o4.dtheta), std::abs(o5.dphi - o4.dphi)};
    const double x[6] = {o5.r, o5.theta, o5.phi, o5.dr, o5.dtheta, o5.dphi};
    for (int j = 0; j < 6; ++j) {
      const double scale = std::max(std::abs(x[j]), 1.0);
      err2 += (d[j] / scale) * (d[j] / scale);
    }
    const double err = std::sqrt(err2 / 6.0);
    if (err > tol) {
      const double fac = std::min(5.0, std::max(0.2, 0.9 * std::pow(tol / err, 0.2)));
      h *= fac;
      if (h < 1e-9) return false;  // avoid infinite shrinking
      continue;
    }
    // Accept (propagate the order-5 solution), renormalize like rk4Step.
    s = o5;
    s.theta = std::min(std::max(s.theta, 0.0001), kPi - 0.0001);
    renormalizeTime(s, rs);
    // Step controller: classic growth on accept (larger tolerances stretch
    // the far-field steps quickly; near-field rejections shrink above).
    hNext = h * std::min(5.0, std::max(0.2, 0.9 * std::pow(tol / err, 0.2)));
    return true;
  }
  return false;
}

int traceRayRkf45(SchwState& s, double rs, double shell, double h0, double tol,
                  int maxSteps, bool& captured, Rkf45Stats* stats) {
  captured = false;
  double h = h0;
  for (int i = 0; i < maxSteps; ++i) {
    if (stats) stats->evals += 6;  // one trial = 6 acceleration evaluations
    double next = 0.0;
    if (!rkf45Step(h, tol, s, rs, next)) return i;  // shrunk to nothing
    // Growth cap mirrors the fixed law's far-field step (hFar = 2.5*h0):
    // larger steps stop paying because the per-step renormalizeTime()
    // projection dominates the error at big h.
    h = std::min(next, h0 * 2.5);
    if (s.r <= rs * 1.001) {
      captured = true;
      return i + 1;
    }
    if (s.r >= shell) return i + 1;
  }
  return maxSteps;
}

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
