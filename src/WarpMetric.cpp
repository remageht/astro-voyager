#include "WarpMetric.h"
#include <algorithm>
#include <cstring>

namespace astro {

void WarpMetric::evalInv4Metric(double x, double y, double z, double t,
                                double g_inv[4][4]) const {
  double alpha = 1.0;
  double beta[3] = {0.0, 0.0, 0.0};
  double gamma_ij[3][3];
  double gamma_inv[3][3];

  evalADM(x, y, z, t, alpha, beta, gamma_ij, gamma_inv);

  const double alphaSq = alpha * alpha;
  const double invAlphaSq = (alphaSq > 1e-30) ? (1.0 / alphaSq) : 1e30;

  g_inv[0][0] = -invAlphaSq;
  for (int i = 0; i < 3; ++i) {
    g_inv[0][i + 1] = beta[i] * invAlphaSq;
    g_inv[i + 1][0] = beta[i] * invAlphaSq;
  }

  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      g_inv[i + 1][j + 1] = gamma_inv[i][j] - (beta[i] * beta[j]) * invAlphaSq;
    }
  }
}

void WarpMetric::evalDerivInv4Metric(double x, double y, double z, double t,
                                     double dg_inv_dx[3][4][4]) const {
  // 4th-order central finite difference fallback
  const double eps = 1e-5;
  const double coords[3] = {x, y, z};

  for (int k = 0; k < 3; ++k) {
    double g_p2[4][4], g_p1[4][4], g_m1[4][4], g_m2[4][4];

    double c[3] = {coords[0], coords[1], coords[2]};
    c[k] = coords[k] + 2.0 * eps;
    evalInv4Metric(c[0], c[1], c[2], t, g_p2);

    c[k] = coords[k] + 1.0 * eps;
    evalInv4Metric(c[0], c[1], c[2], t, g_p1);

    c[k] = coords[k] - 1.0 * eps;
    evalInv4Metric(c[0], c[1], c[2], t, g_m1);

    c[k] = coords[k] - 2.0 * eps;
    evalInv4Metric(c[0], c[1], c[2], t, g_m2);

    for (int mu = 0; mu < 4; ++mu) {
      for (int nu = 0; nu < 4; ++nu) {
        dg_inv_dx[k][mu][nu] =
            (-g_p2[mu][nu] + 8.0 * g_p1[mu][nu] - 8.0 * g_m1[mu][nu] + g_m2[mu][nu]) /
            (12.0 * eps);
      }
    }
  }
}

void WarpMetric::evalDerivTimeInv4Metric(double x, double y, double z, double t,
                                         double dg_inv_dt[4][4]) const {
  if (isStationary()) {
    std::memset(dg_inv_dt, 0, sizeof(double) * 4 * 4);
    return;
  }

  // 4th-order central finite difference fallback for time derivative
  const double eps = 1e-5;
  double g_p2[4][4], g_p1[4][4], g_m1[4][4], g_m2[4][4];

  evalInv4Metric(x, y, z, t + 2.0 * eps, g_p2);
  evalInv4Metric(x, y, z, t + 1.0 * eps, g_p1);
  evalInv4Metric(x, y, z, t - 1.0 * eps, g_m1);
  evalInv4Metric(x, y, z, t - 2.0 * eps, g_m2);

  for (int mu = 0; mu < 4; ++mu) {
    for (int nu = 0; nu < 4; ++nu) {
      dg_inv_dt[mu][nu] =
          (-g_p2[mu][nu] + 8.0 * g_p1[mu][nu] - 8.0 * g_m1[mu][nu] + g_m2[mu][nu]) /
          (12.0 * eps);
    }
  }
}

SchwarzschildADM::SchwarzschildADM(double rs) : m_rs(rs > 0.0 ? rs : 1.0) {}

void SchwarzschildADM::evalADM(double x, double y, double z, double /*t*/,
                               double& alpha, double beta[3],
                               double gamma_ij[3][3], double gamma_inv[3][3]) const {
  beta[0] = 0.0;
  beta[1] = 0.0;
  beta[2] = 0.0;

  double r = std::sqrt(x * x + y * y + z * z);
  if (r < 1e-12) r = 1e-12;

  // Protect against singular horizon collapse in eval
  const double effR = std::max(r, m_rs * 1.0001);
  const double f = 1.0 - m_rs / effR;
  alpha = std::sqrt(std::max(f, 1e-12));

  const double rHat[3] = {x / r, y / r, z / r};
  const double fInvMinus1 = m_rs / (effR - m_rs);
  const double oneMinusF = m_rs / effR;

  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      const double delta = (i == j) ? 1.0 : 0.0;
      gamma_ij[i][j] = delta + fInvMinus1 * rHat[i] * rHat[j];
      gamma_inv[i][j] = delta - oneMinusF * rHat[i] * rHat[j];
    }
  }
}

void SchwarzschildADM::evalDerivInv4Metric(double x, double y, double z, double /*t*/,
                                           double dg_inv_dx[3][4][4]) const {
  std::memset(dg_inv_dx, 0, sizeof(double) * 3 * 4 * 4);

  const double r = std::sqrt(x * x + y * y + z * z);
  if (r <= m_rs * 1.0001) {
    // Inside or at horizon, use fallback to avoid divide by zero
    WarpMetric::evalDerivInv4Metric(x, y, z, 0.0, dg_inv_dx);
    return;
  }

  const double rHat[3] = {x / r, y / r, z / r};
  const double diff = r - m_rs;
  const double rSq = r * r;

  for (int k = 0; k < 3; ++k) {
    // d(g^00)/dx^k = +Rs * r_hat_k / (r - Rs)^2
    dg_inv_dx[k][0][0] = m_rs * rHat[k] / (diff * diff);

    // d(g^0i)/dx^k = 0 (shift beta=0)

    // d(g^ij)/dx^k = -Rs / r^2 * [ delta_ik * r_hat_j + delta_jk * r_hat_i - 3 * r_hat_i * r_hat_j * r_hat_k ]
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        const double d_ik = (i == k) ? 1.0 : 0.0;
        const double d_jk = (j == k) ? 1.0 : 0.0;
        const double term = d_ik * rHat[j] + d_jk * rHat[i] - 3.0 * rHat[i] * rHat[j] * rHat[k];
        dg_inv_dx[k][i + 1][j + 1] = (-m_rs / rSq) * term;
      }
    }
  }
}

bool SchwarzschildADM::isInsideHorizon(double x, double y, double z) const {
  const double r = std::sqrt(x * x + y * y + z * z);
  return r <= m_rs * 1.001;
}

// ============================================================================
// AlcubierreMetric Implementation
// ============================================================================
AlcubierreMetric::AlcubierreMetric(double R, double sigma, double vMax,
                                   double x0, double y0, double z0,
                                   double nx, double ny, double nz,
                                   double totalTime, double tauAcc)
    : m_R(R > 1e-4 ? R : 5.0),
      m_sigma(sigma > 1e-6 ? sigma : 0.5),
      m_vMax(vMax),
      m_totalTime(totalTime > 1e-4 ? totalTime : 100.0),
      m_tauAcc(tauAcc > 1e-4 ? tauAcc : 10.0) {
  m_pos0[0] = x0;
  m_pos0[1] = y0;
  m_pos0[2] = z0;
  const double len = std::sqrt(nx * nx + ny * ny + nz * nz);
  if (len < 1e-12) {
    m_dir[0] = 1.0;
    m_dir[1] = 0.0;
    m_dir[2] = 0.0;
  } else {
    m_dir[0] = nx / len;
    m_dir[1] = ny / len;
    m_dir[2] = nz / len;
  }
}

double AlcubierreMetric::shapeFunction(double rs) const {
  const double r = std::max(rs, 0.0);
  const double up = (r + m_R) / m_sigma;
  const double um = (r - m_R) / m_sigma;
  const double denom = 2.0 * std::tanh(m_R / m_sigma);
  if (std::abs(denom) < 1e-30) return 0.0;
  return (std::tanh(up) - std::tanh(um)) / denom;
}

double AlcubierreMetric::shapeFunctionDeriv(double rs) const {
  const double r = std::max(rs, 0.0);
  const double up = (r + m_R) / m_sigma;
  const double um = (r - m_R) / m_sigma;
  const double tu_p = std::tanh(up);
  const double tu_m = std::tanh(um);
  const double sechSq_p = 1.0 - tu_p * tu_p;
  const double sechSq_m = 1.0 - tu_m * tu_m;
  const double denom = 2.0 * m_sigma * std::tanh(m_R / m_sigma);
  if (std::abs(denom) < 1e-30) return 0.0;
  return (sechSq_p - sechSq_m) / denom;
}

double AlcubierreMetric::velocity(double t) const {
  if (m_totalTime <= 0.0) return m_vMax;
  if (t <= 0.0 || t >= m_totalTime) return 0.0;
  const double u1 = t / m_tauAcc;
  const double u2 = (m_totalTime - t) / m_tauAcc;
  const double mid = 0.5 * m_totalTime / m_tauAcc;
  const double norm = std::tanh(mid);
  const double normSq = norm * norm;
  if (normSq < 1e-30) return 0.0;
  return m_vMax * (std::tanh(u1) * std::tanh(u2)) / normSq;
}

double AlcubierreMetric::velocityDeriv(double t) const {
  if (m_totalTime <= 0.0 || t <= 0.0 || t >= m_totalTime) return 0.0;
  const double u1 = t / m_tauAcc;
  const double u2 = (m_totalTime - t) / m_tauAcc;
  const double mid = 0.5 * m_totalTime / m_tauAcc;
  const double norm = std::tanh(mid);
  const double normSq = norm * norm;
  if (normSq < 1e-30) return 0.0;
  const double t1 = std::tanh(u1);
  const double t2 = std::tanh(u2);
  const double s1 = 1.0 - t1 * t1;  // sech^2(u1)
  const double s2 = 1.0 - t2 * t2;  // sech^2(u2)
  return (m_vMax / (m_tauAcc * normSq)) * (s1 * t2 - t1 * s2);
}

void AlcubierreMetric::bubbleCenter(double t, double center[3]) const {
  if (m_totalTime <= 0.0) {
    const double s = m_vMax * t;
    center[0] = m_pos0[0] + m_dir[0] * s;
    center[1] = m_pos0[1] + m_dir[1] * s;
    center[2] = m_pos0[2] + m_dir[2] * s;
    return;
  }
  if (t <= 0.0) {
    center[0] = m_pos0[0];
    center[1] = m_pos0[1];
    center[2] = m_pos0[2];
    return;
  }
  const double tClamped = std::min(t, m_totalTime);
  const double U = m_totalTime / m_tauAcc;
  const double k = std::tanh(U * 0.5);
  const double kSq = k * k;
  if (kSq < 1e-30) {
    center[0] = m_pos0[0];
    center[1] = m_pos0[1];
    center[2] = m_pos0[2];
    return;
  }
  const double u = tClamped / m_tauAcc;
  const double z = std::tanh((2.0 * u - U) * 0.5);
  const double kz = std::clamp(k * z, -0.999999999999999, 0.999999999999999);
  const double k2 = std::clamp(kSq, -0.999999999999999, 0.999999999999999);
  const double term1 = ((1.0 + kSq) / k) * (std::atanh(kz) + std::atanh(k2));
  const double term2 = u;
  const double s = (m_vMax * m_tauAcc / kSq) * (term1 - term2);

  center[0] = m_pos0[0] + m_dir[0] * s;
  center[1] = m_pos0[1] + m_dir[1] * s;
  center[2] = m_pos0[2] + m_dir[2] * s;
}

void AlcubierreMetric::evalADM(double x, double y, double z, double t,
                               double& alpha, double beta[3],
                               double gamma_ij[3][3], double gamma_inv[3][3]) const {
  alpha = 1.0;
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      gamma_ij[i][j] = (i == j) ? 1.0 : 0.0;
      gamma_inv[i][j] = (i == j) ? 1.0 : 0.0;
    }
  }

  double c[3];
  bubbleCenter(t, c);
  const double dx = x - c[0];
  const double dy = y - c[1];
  const double dz = z - c[2];
  const double rs = std::sqrt(dx * dx + dy * dy + dz * dz);
  const double f = shapeFunction(rs);
  const double v = velocity(t);

  beta[0] = -v * f * m_dir[0];
  beta[1] = -v * f * m_dir[1];
  beta[2] = -v * f * m_dir[2];
}

void AlcubierreMetric::evalDerivInv4Metric(double x, double y, double z, double t,
                                          double dg_inv_dx[3][4][4]) const {
  std::memset(dg_inv_dx, 0, sizeof(double) * 3 * 4 * 4);

  double c[3];
  bubbleCenter(t, c);
  const double diff[3] = {x - c[0], y - c[1], z - c[2]};
  const double rs = std::sqrt(diff[0] * diff[0] + diff[1] * diff[1] + diff[2] * diff[2]);
  if (rs < 1e-12) return;

  const double invRs = 1.0 / rs;
  const double rHat[3] = {diff[0] * invRs, diff[1] * invRs, diff[2] * invRs};
  const double f = shapeFunction(rs);
  const double df = shapeFunctionDeriv(rs);
  const double v = velocity(t);

  for (int k = 0; k < 3; ++k) {
    // d(g^00)/dx^k = 0 (alpha = 1 identically)
    dg_inv_dx[k][0][0] = 0.0;

    // d(g^0i)/dx^k = -v * n^i * df * rHat_k
    for (int i = 0; i < 3; ++i) {
      const double d_0i = -v * m_dir[i] * df * rHat[k];
      dg_inv_dx[k][0][i + 1] = d_0i;
      dg_inv_dx[k][i + 1][0] = d_0i;
    }

    // d(g^ij)/dx^k = -2 * v^2 * n^i * n^j * f * df * rHat_k
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        dg_inv_dx[k][i + 1][j + 1] = -2.0 * (v * v) * (m_dir[i] * m_dir[j]) * (f * df) * rHat[k];
      }
    }
  }
}

void AlcubierreMetric::evalDerivTimeInv4Metric(double x, double y, double z, double t,
                                              double dg_inv_dt[4][4]) const {
  std::memset(dg_inv_dt, 0, sizeof(double) * 4 * 4);

  double c[3];
  bubbleCenter(t, c);
  const double diff[3] = {x - c[0], y - c[1], z - c[2]};
  const double rs = std::sqrt(diff[0] * diff[0] + diff[1] * diff[1] + diff[2] * diff[2]);
  if (rs < 1e-12) return;

  const double invRs = 1.0 / rs;
  const double f = shapeFunction(rs);
  const double df = shapeFunctionDeriv(rs);
  const double v = velocity(t);
  const double vDot = velocityDeriv(t);
  const double dot_n = (diff[0] * m_dir[0] + diff[1] * m_dir[1] + diff[2] * m_dir[2]) * invRs;

  // d(beta0)/dt = - [ vDot * f - v^2 * df * (diff . n / rs) ]
  const double dbeta0_dt = -(vDot * f - (v * v) * df * dot_n);
  const double beta0 = -v * f;

  // d(g^00)/dt = 0
  dg_inv_dt[0][0] = 0.0;

  // d(g^0i)/dt = d(beta^i)/dt = d(beta0)/dt * n^i
  for (int i = 0; i < 3; ++i) {
    const double val = dbeta0_dt * m_dir[i];
    dg_inv_dt[0][i + 1] = val;
    dg_inv_dt[i + 1][0] = val;
  }

  // d(g^ij)/dt = -2 * beta0 * d(beta0)/dt * n^i * n^j
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      dg_inv_dt[i + 1][j + 1] = -2.0 * beta0 * dbeta0_dt * (m_dir[i] * m_dir[j]);
    }
  }
}

double AlcubierreMetric::yorkTime(double x, double y, double z, double t) const {
  double c[3];
  bubbleCenter(t, c);
  const double dx = x - c[0];
  const double dy = y - c[1];
  const double dz = z - c[2];
  const double rs = std::sqrt(dx * dx + dy * dy + dz * dz);
  if (rs < 1e-12) return 0.0;

  const double cosAngle = (m_dir[0] * dx + m_dir[1] * dy + m_dir[2] * dz) / rs;
  const double df = shapeFunctionDeriv(rs);
  const double v = velocity(t);
  return v * df * cosAngle;
}

double AlcubierreMetric::thetaMax(double t) const {
  const double v = velocity(t);
  const double denom = 2.0 * m_sigma * std::tanh(m_R / m_sigma);
  if (std::abs(denom) < 1e-30) return 0.0;
  return std::abs(v) / denom;
}

}  // namespace astro
