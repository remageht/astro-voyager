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

}  // namespace astro
