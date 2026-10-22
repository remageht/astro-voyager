#pragma once

#include <cmath>
#include <string>
#include <vector>

namespace astro {

// ============================================================================
// WarpMetric: Universal 3+1 ADM Spacetime Metric Interface (M1 Framework)
// ============================================================================
// Line element in Arnowitt-Deser-Misner (ADM) 3+1 form:
//   ds^2 = -alpha^2 dt^2 + gamma_ij (dx^i + beta^i dt)(dx^j + beta^j dt)
//
// 4-metric g_mu_nu:
//   g_00 = -alpha^2 + beta_k beta^k = -alpha^2 + gamma_kl beta^k beta^l
//   g_0i = g_i0 = beta_i = gamma_ik beta^k
//   g_ij = gamma_ij
//
// Contravariant (inverse) 4-metric g^{mu nu}:
//   g^00 = -1 / alpha^2
//   g^0i = g^i0 = beta^i / alpha^2
//   g^ij = gamma^ij - (beta^i beta^j) / alpha^2
//
// Hamiltonian for null geodesics:
//   H = 1/2 g^{mu nu} p_mu p_nu = 0
// ============================================================================
class WarpMetric {
public:
  virtual ~WarpMetric() = default;

  virtual std::string name() const = 0;

  // ADM 3+1 decomposition at spatial point (x, y, z) and coordinate time t:
  //   alpha: lapse function (alpha > 0)
  //   beta: shift vector (contravariant beta^x, beta^y, beta^z)
  //   gamma_ij: spatial 3-metric (symmetric 3x3)
  //   gamma_inv: inverse spatial 3-metric gamma^{ij} (symmetric 3x3)
  virtual void evalADM(double x, double y, double z, double t,
                       double& alpha, double beta[3],
                       double gamma_ij[3][3], double gamma_inv[3][3]) const = 0;

  // Evaluates the inverse 4-metric g^{mu nu} (indices: 0=t, 1=x, 2=y, 3=z)
  virtual void evalInv4Metric(double x, double y, double z, double t,
                              double g_inv[4][4]) const;

  // Spatial derivative of inverse 4-metric: dg_inv_dx[k][mu][nu] = d(g^{mu nu})/dx^k
  // where k in {0, 1, 2} corresponds to spatial directions {x, y, z}.
  virtual void evalDerivInv4Metric(double x, double y, double z, double t,
                                   double dg_inv_dx[3][4][4]) const;

  // Check if coordinates lie inside event horizon / singular region
  virtual bool isInsideHorizon(double x, double y, double z) const = 0;

  // Event horizon or boundary radius
  virtual double horizonRadius() const { return 0.0; }
};

// ============================================================================
// SchwarzschildADM: Schwarzschild black hole in pole-free Cartesian 3+1 form
// ============================================================================
//   r = sqrt(x^2 + y^2 + z^2)
//   f(r) = 1 - Rs / r  (Rs = 2M)
//   alpha = sqrt(f(r))
//   beta^i = 0
//   gamma_ij = delta_ij + (1/f - 1) * r_hat_i * r_hat_j = delta_ij + (Rs/(r - Rs)) * r_hat_i * r_hat_j
//   gamma^ij = delta_ij - (1 - f) * r_hat_i * r_hat_j = delta_ij - (Rs/r) * r_hat_i * r_hat_j
// ============================================================================
class SchwarzschildADM : public WarpMetric {
public:
  explicit SchwarzschildADM(double rs = 1.0);

  std::string name() const override { return "SchwarzschildADM"; }

  void evalADM(double x, double y, double z, double t,
               double& alpha, double beta[3],
               double gamma_ij[3][3], double gamma_inv[3][3]) const override;

  void evalDerivInv4Metric(double x, double y, double z, double t,
                           double dg_inv_dx[3][4][4]) const override;

  bool isInsideHorizon(double x, double y, double z) const override;

  double horizonRadius() const override { return m_rs; }
  double mass() const { return 0.5 * m_rs; }

private:
  double m_rs;
};

}  // namespace astro
