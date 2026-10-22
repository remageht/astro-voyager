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

// ============================================================================
// AlcubierreMetric: 3+1 ADM Alcubierre Warp Bubble (M2 Framework)
// ============================================================================
// Spacetime line element (Alcubierre 1994):
//   ds^2 = -dt^2 + (dx - v_s(t) f(r_s) dt)^2 + dy^2 + dz^2
// In 3+1 ADM form:
//   alpha = 1
//   gamma_ij = delta_ij, gamma^ij = delta^ij
//   beta^i = -v_s(t) * f(r_s) * n^i
//
// Shape function f(r_s) with smooth sigma-wall:
//   f(r_s) = (tanh((r_s + R) / sigma) - tanh((r_s - R) / sigma)) / (2 * tanh(R / sigma))
//   - At r_s = 0: f(0) = 1 (flat Eulerian bubble interior)
//   - At r_s >> R + 3*sigma: f(r_s) -> 0 (asymptotically flat exterior)
//
// Velocity profile v(t) (tanh acceleration / cruise / deceleration):
//   v(t) = vMax * tanh(t / tau_a) * tanh((T - t) / tau_a) / (tanh(T / (2*tau_a))^2)
//
// York extrinsic curvature scalar (York time):
//   theta = -nabla_i beta^i = v(t) * f'(r_s) * (n . (x - x_s)) / r_s
//   theta_max = |v(t)| / (2 * sigma * tanh(R / sigma))
// ============================================================================
class AlcubierreMetric : public WarpMetric {
public:
  AlcubierreMetric(double R = 5.0, double sigma = 0.5, double vMax = 1.0,
                   double x0 = 0.0, double y0 = 0.0, double z0 = 0.0,
                   double nx = 1.0, double ny = 0.0, double nz = 0.0,
                   double totalTime = 100.0, double tauAcc = 10.0);

  std::string name() const override { return "AlcubierreMetric"; }

  void evalADM(double x, double y, double z, double t,
               double& alpha, double beta[3],
               double gamma_ij[3][3], double gamma_inv[3][3]) const override;

  void evalDerivInv4Metric(double x, double y, double z, double t,
                           double dg_inv_dx[3][4][4]) const override;

  bool isInsideHorizon(double x, double y, double z) const override {
    (void)x; (void)y; (void)z;
    return false;
  }

  double horizonRadius() const override { return 0.0; }

  // Shape function f(r_s) and its analytical derivative f'(r_s)
  double shapeFunction(double rs) const;
  double shapeFunctionDeriv(double rs) const;

  // Velocity profile v(t) and center position x_s(t)
  double velocity(double t) const;
  void bubbleCenter(double t, double center[3]) const;

  // York time / expansion scalar theta(x, y, z, t) and analytical maximum theta_max(t)
  double yorkTime(double x, double y, double z, double t) const;
  double thetaMax(double t) const;

  // Parameters
  double bubbleRadius() const { return m_R; }
  double wallThickness() const { return m_sigma; }
  double peakVelocity() const { return m_vMax; }
  double totalTime() const { return m_totalTime; }
  double tauAcc() const { return m_tauAcc; }

private:
  double m_R;
  double m_sigma;
  double m_vMax;
  double m_pos0[3];
  double m_dir[3];
  double m_totalTime;
  double m_tauAcc;
};

}  // namespace astro
