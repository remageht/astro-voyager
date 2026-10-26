#include "WarpGeodesic.h"
#include <algorithm>
#include <cmath>

namespace astro {

double evalHamiltonian(const WarpMetric& metric, const WarpRayState& s) {
  double g_inv[4][4];
  metric.evalInv4Metric(s.x, s.y, s.z, s.t, g_inv);

  const double p[4] = {s.pt, s.px, s.py, s.pz};
  double hVal = 0.0;
  for (int mu = 0; mu < 4; ++mu) {
    for (int nu = 0; nu < 4; ++nu) {
      hVal += g_inv[mu][nu] * p[mu] * p[nu];
    }
  }
  return 0.5 * hVal;
}

void renormalizeNull(const WarpMetric& metric, WarpRayState& s) {
  // If the spacetime metric is non-stationary, p_t evolves dynamically via dp_t/dlambda = -dH/dt.
  // Artificially rescaling spatial momentum while holding p_t fixed corrupts energy conservation.
  if (!metric.isStationary()) {
    return;
  }

  double g_inv[4][4];
  metric.evalInv4Metric(s.x, s.y, s.z, s.t, g_inv);

  // We seek scale factor k > 0 such that H(p_t, k * p_i) = 0:
  //   1/2 * [ g^00 * p_t^2 + 2 * k * sum_i(g^0i * p_t * p_i) + k^2 * sum_{i,j}(g^ij * p_i * p_j) ] = 0
  // Quadratic form: A * k^2 + B * k + C = 0
  double A = 0.0;
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      const double pi = (i == 0) ? s.px : ((i == 1) ? s.py : s.pz);
      const double pj = (j == 0) ? s.px : ((j == 1) ? s.py : s.pz);
      A += g_inv[i + 1][j + 1] * pi * pj;
    }
  }

  double B = 0.0;
  for (int i = 0; i < 3; ++i) {
    const double pi = (i == 0) ? s.px : ((i == 1) ? s.py : s.pz);
    B += 2.0 * g_inv[0][i + 1] * s.pt * pi;
  }

  const double C = g_inv[0][0] * s.pt * s.pt;

  if (std::abs(A) < 1e-30) return;

  const double disc = B * B - 4.0 * A * C;
  if (disc < 0.0) return;  // Space-like or complex, preserve state

  const double sqrtDisc = std::sqrt(disc);
  // Pick positive root closest to 1.0
  const double k1 = (-B + sqrtDisc) / (2.0 * A);
  const double k2 = (-B - sqrtDisc) / (2.0 * A);

  double kBest = k1;
  if (k1 <= 0.0 || (k2 > 0.0 && std::abs(k2 - 1.0) < std::abs(k1 - 1.0))) {
    kBest = k2;
  }

  if (kBest > 0.0 && std::abs(kBest - 1.0) < 0.5) {
    s.px *= kBest;
    s.py *= kBest;
    s.pz *= kBest;
  }
}

void evalHamiltonianDerivatives(const WarpMetric& metric, const WarpRayState& s,
                                double dxdlambda[4], double dpdlambda[4]) {
  double g_inv[4][4];
  metric.evalInv4Metric(s.x, s.y, s.z, s.t, g_inv);

  double dg_inv_dx[3][4][4];
  metric.evalDerivInv4Metric(s.x, s.y, s.z, s.t, dg_inv_dx);

  const double p[4] = {s.pt, s.px, s.py, s.pz};

  // dx^mu / dlambda = dH / dp_mu = sum_nu g^{mu nu} p_nu
  for (int mu = 0; mu < 4; ++mu) {
    dxdlambda[mu] = 0.0;
    for (int nu = 0; nu < 4; ++nu) {
      dxdlambda[mu] += g_inv[mu][nu] * p[nu];
    }
  }

  // dp_t / dlambda = -dH / dt = -1/2 sum_{alpha, beta} (d g^{alpha beta} / dt) p_alpha p_beta
  double sum_t = 0.0;
  if (!metric.isStationary()) {
    double dg_inv_dt[4][4];
    metric.evalDerivTimeInv4Metric(s.x, s.y, s.z, s.t, dg_inv_dt);
    for (int alpha = 0; alpha < 4; ++alpha) {
      for (int beta = 0; beta < 4; ++beta) {
        sum_t += dg_inv_dt[alpha][beta] * p[alpha] * p[beta];
      }
    }
  }
  dpdlambda[0] = -0.5 * sum_t;

  // dp_i / dlambda = -dH / dx^i = -1/2 sum_{alpha, beta} (d g^{alpha beta} / dx^i) p_alpha p_beta
  for (int i = 0; i < 3; ++i) {
    double sum = 0.0;
    for (int alpha = 0; alpha < 4; ++alpha) {
      for (int beta = 0; beta < 4; ++beta) {
        sum += dg_inv_dx[i][alpha][beta] * p[alpha] * p[beta];
      }
    }
    dpdlambda[i + 1] = -0.5 * sum;
  }
}

void warpRk4Step(const WarpMetric& metric, double h, WarpRayState& s) {
  // Stage 1
  double dx1[4], dp1[4];
  evalHamiltonianDerivatives(metric, s, dx1, dp1);

  // Stage 2
  WarpRayState s2 = s;
  s2.t += 0.5 * h * dx1[0];
  s2.x += 0.5 * h * dx1[1];
  s2.y += 0.5 * h * dx1[2];
  s2.z += 0.5 * h * dx1[3];
  s2.pt += 0.5 * h * dp1[0];
  s2.px += 0.5 * h * dp1[1];
  s2.py += 0.5 * h * dp1[2];
  s2.pz += 0.5 * h * dp1[3];

  double dx2[4], dp2[4];
  evalHamiltonianDerivatives(metric, s2, dx2, dp2);

  // Stage 3
  WarpRayState s3 = s;
  s3.t += 0.5 * h * dx2[0];
  s3.x += 0.5 * h * dx2[1];
  s3.y += 0.5 * h * dx2[2];
  s3.z += 0.5 * h * dx2[3];
  s3.pt += 0.5 * h * dp2[0];
  s3.px += 0.5 * h * dp2[1];
  s3.py += 0.5 * h * dp2[2];
  s3.pz += 0.5 * h * dp2[3];

  double dx3[4], dp3[4];
  evalHamiltonianDerivatives(metric, s3, dx3, dp3);

  // Stage 4
  WarpRayState s4 = s;
  s4.t += h * dx3[0];
  s4.x += h * dx3[1];
  s4.y += h * dx3[2];
  s4.z += h * dx3[3];
  s4.pt += h * dp3[0];
  s4.px += h * dp3[1];
  s4.py += h * dp3[2];
  s4.pz += h * dp3[3];

  double dx4[4], dp4[4];
  evalHamiltonianDerivatives(metric, s4, dx4, dp4);

  // Combine stages
  const double sixth = h / 6.0;
  s.t += sixth * (dx1[0] + 2.0 * dx2[0] + 2.0 * dx3[0] + dx4[0]);
  s.x += sixth * (dx1[1] + 2.0 * dx2[1] + 2.0 * dx3[1] + dx4[1]);
  s.y += sixth * (dx1[2] + 2.0 * dx2[2] + 2.0 * dx3[2] + dx4[2]);
  s.z += sixth * (dx1[3] + 2.0 * dx2[3] + 2.0 * dx3[3] + dx4[3]);

  s.pt += sixth * (dp1[0] + 2.0 * dp2[0] + 2.0 * dp3[0] + dp4[0]);
  s.px += sixth * (dp1[1] + 2.0 * dp2[1] + 2.0 * dp3[1] + dp4[1]);
  s.py += sixth * (dp1[2] + 2.0 * dp2[2] + 2.0 * dp3[2] + dp4[2]);
  s.pz += sixth * (dp1[3] + 2.0 * dp2[3] + 2.0 * dp3[3] + dp4[3]);
  s.lambda += h;

  renormalizeNull(metric, s);
}

WarpTraceResult traceWarpRay(const WarpMetric& metric, WarpRayState s,
                             double rMax, double h, int maxSteps) {
  WarpTraceResult res;
  res.maxHConstraint = 0.0;

  for (int step = 0; step < maxSteps; ++step) {
    const double r = std::sqrt(s.x * s.x + s.y * s.y + s.z * s.z);
    res.finalR = r;

    const double hVal = std::abs(evalHamiltonian(metric, s));
    if (hVal > res.maxHConstraint) res.maxHConstraint = hVal;

    if (metric.isInsideHorizon(s.x, s.y, s.z)) {
      res.captured = true;
      res.steps = step;
      return res;
    }

    if (r >= rMax && step > 5) {
      res.captured = false;
      res.steps = step;
      return res;
    }

    // Adaptive step size scaling based on distance to horizon
    double curH = h;
    const double rh = metric.horizonRadius();
    if (rh > 0.0 && r > rh) {
      const double q = std::max(r - rh, 1e-4 * rh);
      curH = std::clamp(h * (q / rh) * std::sqrt(r / rh), 0.005 * h, 3.0 * h);
    }

    warpRk4Step(metric, curH, s);
  }

  res.captured = metric.isInsideHorizon(s.x, s.y, s.z);
  res.steps = maxSteps;
  return res;
}

WarpRayState makeWarpInwardRay(const WarpMetric& metric, double r0, double b) {
  WarpRayState s;
  s.t = 0.0;
  s.x = r0;
  s.y = 0.0;
  s.z = 0.0;
  s.pt = -1.0;  // Conserved energy E = 1
  s.py = (r0 > 1e-12) ? (b / r0) : 0.0;
  s.pz = 0.0;
  s.lambda = 0.0;

  double g_inv[4][4];
  metric.evalInv4Metric(s.x, s.y, s.z, s.t, g_inv);

  // Inward null ray condition: g^{mu nu} p_mu p_nu = 0
  // Quadratic in p_x: A * p_x^2 + B * p_x + C = 0
  const double A = g_inv[1][1];
  const double B = 2.0 * (g_inv[0][1] * s.pt + g_inv[1][2] * s.py + g_inv[1][3] * s.pz);
  const double C = g_inv[0][0] * s.pt * s.pt +
                   2.0 * (g_inv[0][2] * s.pt * s.py + g_inv[0][3] * s.pt * s.pz) +
                   g_inv[2][2] * s.py * s.py +
                   2.0 * g_inv[2][3] * s.py * s.pz +
                   g_inv[3][3] * s.pz * s.pz;

  if (std::abs(A) > 1e-30) {
    const double disc = B * B - 4.0 * A * C;
    if (disc >= 0.0) {
      // Inward motion along -X requires coordinate velocity dx/dlambda = g^{x mu} p_mu < 0.
      // Since dx/dlambda = 1/2 B + A * px, choosing the root with -sqrt(disc) ensures:
      // dx/dlambda = -1/2 * sqrt(disc) <= 0.
      s.px = (-B - std::sqrt(disc)) / (2.0 * A);
    } else {
      s.px = 0.0;
    }
  } else {
    s.px = 0.0;
  }

  renormalizeNull(metric, s);
  return s;
}

// ============================================================================
// Skiff Warp Waypoint Generator (M2 Framework)
// ============================================================================
namespace {

// Normalized symmetric tanh velocity profile w(u) on u in [0, 1]
inline double evalTanhProfile(double u, double alpha) {
  if (u <= 0.0 || u >= 1.0) return 0.0;
  const double norm = std::tanh(0.5 / alpha);
  const double normSq = norm * norm;
  if (normSq < 1e-30) return 0.0;
  return (std::tanh(u / alpha) * std::tanh((1.0 - u) / alpha)) / normSq;
}

// Integrated distance fraction s(u) / D = (int_0^u w(u') du') / (int_0^1 w(u') du')
double integrateProfile(double uTarget, double alpha, double& totalIntegral) {
  const int M = 1000;
  const double du = 1.0 / M;

  // Total integral over [0, 1]
  double total = evalTanhProfile(0.0, alpha) + evalTanhProfile(1.0, alpha);
  for (int i = 1; i < M; ++i) {
    const double ui = i * du;
    const double weight = (i % 2 == 1) ? 4.0 : 2.0;
    total += weight * evalTanhProfile(ui, alpha);
  }
  totalIntegral = (du / 3.0) * total;
  if (totalIntegral < 1e-30) totalIntegral = 1.0;

  if (uTarget <= 0.0) return 0.0;
  if (uTarget >= 1.0) return 1.0;

  // Symmetric integration
  const bool useReflect = (uTarget > 0.5);
  const double uEff = useReflect ? (1.0 - uTarget) : uTarget;

  const int stepsTarget = std::max(static_cast<int>(uEff * M), 2);
  const double dt = uEff / stepsTarget;
  double sumPart = evalTanhProfile(0.0, alpha) + evalTanhProfile(uEff, alpha);
  for (int i = 1; i < stepsTarget; ++i) {
    const double ui = i * dt;
    const double weight = (i % 2 == 1) ? 4.0 : 2.0;
    sumPart += weight * evalTanhProfile(ui, alpha);
  }
  const double partIntegral = (dt / 3.0) * sumPart;
  const double frac = partIntegral / totalIntegral;
  return useReflect ? (1.0 - frac) : frac;
}

}  // namespace

std::vector<SkiffWaypoint> WarpGeodesic::skiffPath(
    const double start[3], const double end[3],
    double vMax, double sigma, int steps, double bubbleRadius, double lapse) {
  const int N = std::max(steps, 2);
  std::vector<SkiffWaypoint> result(N);

  const double dx = end[0] - start[0];
  const double dy = end[1] - start[1];
  const double dz = end[2] - start[2];
  const double dist = std::sqrt(dx * dx + dy * dy + dz * dz);

  const double effSigma = (sigma > 1e-6) ? sigma : 0.5;
  const double effVMax = (vMax > 1e-6) ? vMax : 1.0;
  const double bubbleR = (bubbleRadius > 0.0) ? bubbleRadius : std::max(5.0 * effSigma, 2.0);
  const double effLapse = (lapse > 1e-12) ? lapse : 1e-12;
  const double alpha = 0.15;  // 15% acceleration, cruise, 15% deceleration

  double totalIntegral = 0.0;
  integrateProfile(1.0, alpha, totalIntegral);
  const double totalTime = (dist > 1e-12) ? (dist / (effVMax * totalIntegral)) : 0.0;

  double dir[3] = {1.0, 0.0, 0.0};
  if (dist > 1e-12) {
    dir[0] = dx / dist;
    dir[1] = dy / dist;
    dir[2] = dz / dist;
  }

  // Pre-calculate distance fractions ensuring exact symmetry
  std::vector<double> frac(N);
  for (int k = 0; k <= (N - 1) / 2; ++k) {
    const double u = static_cast<double>(k) / (N - 1);
    frac[k] = integrateProfile(u, alpha, totalIntegral);
    frac[N - 1 - k] = 1.0 - frac[k];
  }

  const double wallClearance = bubbleR - 2.0 * effSigma;  // Centred ship -> inner wall edge
  const double dtsDtau = 1.0 / effLapse;
  const double denomTheta = 2.0 * effSigma * std::tanh(bubbleR / effSigma);

  for (int k = 0; k < N; ++k) {
    const double u = static_cast<double>(k) / (N - 1);
    const double v = effVMax * evalTanhProfile(u, alpha);
    const double s = dist * frac[k];

    SkiffWaypoint wp;
    wp.t = u * totalTime;
    wp.x = start[0] + dir[0] * s;
    wp.y = start[1] + dir[1] * s;
    wp.z = start[2] + dir[2] * s;
    wp.vx = dir[0] * v;
    wp.vy = dir[1] * v;
    wp.vz = dir[2] * v;
    wp.v = v;
    wp.tau = wp.t * effLapse;  // proper time for the assumed lapse at the bubble centre
    wp.thetaMax = (denomTheta > 1e-30) ? (v / denomTheta) : 0.0;
    wp.wallClearance = wallClearance;
    wp.dtsDtau = dtsDtau;

    result[k] = wp;
  }

  return result;
}

std::vector<SkiffWaypoint> skiffPath(
    const double start[3], const double end[3],
    double vMax, double sigma, int steps, double bubbleRadius, double lapse) {
  return WarpGeodesic::skiffPath(start, end, vMax, sigma, steps, bubbleRadius, lapse);
}

}  // namespace astro
