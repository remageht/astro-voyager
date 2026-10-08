#pragma once

#include <vector>

namespace astro {

// Point along photon orbit in polar coordinates (r, phi)
struct BinetPoint {
  double phi = 0.0;
  double u = 0.0;    // u = 1 / r
  double r = 0.0;    // r = 1 / u
  double du_dphi = 0.0;
};

// Result of Binet photon orbit integration
struct BinetOrbitResult {
  std::vector<BinetPoint> points;
  bool captured = false;  // ray fell through event horizon (r <= Rs)
  bool escaped = false;   // ray turned back and moved outward to escape shell (r >= rEscape)
  double phiTurn = -1.0;  // turning point phi if periapsis was reached
  double rMin = -1.0;     // distance of closest approach (periapsis)
};

// ============================================================================
// Derivation of the Binet equation for null geodesics in Schwarzschild metric:
// ============================================================================
//
// 1. Schwarzschild metric (equatorial plane theta = pi/2, c = 1, G = 1, Rs = 2M):
//    ds^2 = - (1 - 2M/r) dt^2 + (1 - 2M/r)^(-1) dr^2 + r^2 dphi^2
//
// 2. Constants of motion along geodesic with affine parameter lambda:
//    E = (1 - 2M/r) dt/dlambda        (Energy per unit mass / frequency parameter)
//    L = r^2 dphi/dlambda             (Angular momentum parameter)
//    Impact parameter b = L / E.
//
// 3. For null geodesics (photons, ds^2 = 0):
//    0 = - (1 - 2M/r) (dt/dlambda)^2 + (1 - 2M/r)^(-1) (dr/dlambda)^2 + r^2 (dphi/dlambda)^2
//    Substitute dt/dlambda = E / (1 - 2M/r) and dphi/dlambda = L / r^2:
//    0 = - E^2 / (1 - 2M/r) + (1 - 2M/r)^(-1) (dr/dlambda)^2 + L^2 / r^2
//    Multiply by (1 - 2M/r):
//    (dr/dlambda)^2 + (L^2 / r^2)(1 - 2M/r) = E^2
//
// 4. Transform to orbit equation r(phi) using dr/dlambda = (dr/dphi)(dphi/dlambda) = (L / r^2)(dr/dphi):
//    (L / r^2)^2 (dr/dphi)^2 + (L^2 / r^2)(1 - 2M/r) = E^2
//    Divide by L^2:
//    (1 / r^2)^2 (dr/dphi)^2 + (1 / r^2)(1 - 2M/r) = E^2 / L^2 = 1 / b^2
//
// 5. Introduce Binet variable u = 1 / r, du/dphi = - (1 / r^2) dr/dphi:
//    (du/dphi)^2 + u^2 (1 - 2M u) = 1 / b^2
//    (du/dphi)^2 + u^2 - 2M u^3 = 1 / b^2          [First-order energy equation]
//
// 6. Differentiate with respect to phi:
//    2 (du/dphi) (d^2 u / dphi^2) + 2 u (du/dphi) - 6 M u^2 (du/dphi) = 0
//    Dividing by 2 (du/dphi) (for non-circular segments where du/dphi != 0):
//    d^2 u / dphi^2 + u = 3 M u^2                   [Binet orbit equation for photons]
//    With Schwarzschild radius Rs = 2M (so M = Rs / 2):
//    d^2 u / dphi^2 + u = (3/2) Rs u^2
//
// Dimensions:
//    [r] = L (length), [u] = L^(-1), [phi] = 1 (dimensionless)
//    [d^2 u / dphi^2] = L^(-1), [u] = L^(-1)
//    [Rs] = L, [u^2] = L^(-2) ==> [Rs u^2] = L * L^(-2) = L^(-1).
//    All terms have consistent dimension L^(-1).
//
// Boundaries and Critical Behaviors:
//    - Effective potential: V_eff(u) = u^2 (1 - 2M u) = u^2 - 2M u^3.
//    - Maxima of V_eff: dV_eff/du = 2u - 6M u^2 = 0 ==> u_ph = 1 / (3M) = 2 / (3 Rs).
//      Photon sphere radius: r_ph = 3M = 1.5 Rs.
//    - Potential barrier height: V_eff(u_ph) = (1 / (3M))^2 (1 - 2/3) = 1 / (27 M^2).
//    - Critical impact parameter: 1 / b_crit^2 = V_eff(u_ph) = 1 / (27 M^2)
//      ==> b_crit = 3 sqrt(3) M = (3 sqrt(3) / 2) Rs ~ 2.598076211 Rs.
//    - If b < b_crit: 1/b^2 > V_eff(u_ph), no turning point outside r_ph; ray overcomes
//      barrier and falls to event horizon u -> 1/Rs (and singularity u -> inf). (Capture)
//    - If b > b_crit: 1/b^2 < V_eff(u_ph), ray encounters turning point (du/dphi = 0)
//      at periapsis r_min > 1.5 Rs, reverses radial direction, and escapes to infinity u -> 0. (Escape)
//    - If b -> b_crit from above: ray spirals asymptotically towards photon sphere
//      u -> 1/(3M) as phi -> inf, making an infinite number of revolutions.
// ============================================================================

// Check whether a photon launched inward from r0 with impact parameter b is captured.
// Analytically, in Schwarzschild geometry for rays launched from r0 > 1.5 Rs:
// captured <=> b < b_crit = 3*sqrt(3)/2 * Rs.
bool binetIsCaptured(double b, double rs);

// Numerically integrate the Binet photon orbit d^2u/dphi^2 + u = 3M u^2 using RK4
// from initial radius r0 (with phi = 0) directed inward with impact parameter b.
//
// Parameters:
//   b:       impact parameter L/E
//   rs:      Schwarzschild radius (Rs = 2M)
//   r0:      initial launch radius (default 10.0 Rs)
//   dphi:    integration step in phi (radians, e.g. 0.001)
//   phiMax:  maximum angle phi before stopping (radians, e.g. 8.0 * pi)
//   rEscape: radius considered escaped (e.g. 30.0 Rs)
//
// Returns:
//   BinetOrbitResult containing trajectory points and termination status.
BinetOrbitResult binetPhotonOrbit(double b, double rs, double r0 = 10.0,
                                  double dphi = 0.001, double phiMax = 30.0,
                                  double rEscape = 30.0);

}  // namespace astro
