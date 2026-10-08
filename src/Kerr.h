#pragma once

#include <vector>

namespace astro {

// Point along Kerr equatorial photon orbit in Boyer-Lindquist coordinates (r, phi)
struct KerrPoint {
  double lambda = 0.0;  // Affine parameter
  double r = 0.0;
  double phi = 0.0;
  double dr = 0.0;      // dr/dlambda
  double dphi = 0.0;    // dphi/dlambda
};

// Result of Kerr photon orbit integration
struct KerrOrbitResult {
  std::vector<KerrPoint> points;
  bool captured = false;  // ray crossed the outer event horizon r_plus = M + sqrt(M^2 - a^2)
  bool escaped = false;   // ray reached escape shell (r >= rEscape)
  double rMin = -1.0;     // distance of closest approach (turning point)
};

// ============================================================================
// Kerr Metric Equatorial Null Geodesics & Carter Equations:
// ============================================================================
//
// 1. Metric in Boyer-Lindquist coordinates (c = 1, G = 1, M = Rs / 2, |a| <= M):
//    ds^2 = - (1 - 2Mr/Sigma) dt^2 - (4Mar sin^2(theta)/Sigma) dt dphi
//           + (Sigma/Delta) dr^2 + Sigma dtheta^2
//           + ((r^2 + a^2)^2 - a^2 Delta sin^2(theta)) sin^2(theta)/Sigma dphi^2
//    where:
//      Delta = r^2 - 2Mr + a^2
//      Sigma = r^2 + a^2 cos^2(theta)
//
// 2. Equatorial plane (theta = pi/2, dtheta = 0, Sigma = r^2):
//    Carter constant Q = 0.
//    Constants of motion:
//      E = - p_t = 1 (energy parameter, normalized to 1)
//      L = p_phi      (axial angular momentum)
//      Impact parameter b = L / E.
//
//    First-order Carter equations along affine parameter lambda (with dtau = dlambda / r^2):
//      dt/dlambda   = [(r^2 + a^2)^2 / Delta - a^2] E - [2Mar / Delta] L
//                   = [ (r^2 + a^2)( (r^2 + a^2) - a b ) + 2Mar(a - b) ] / (r^2 Delta)  ...
//      More cleanly in equatorial plane:
//        r^2 (dt/dlambda)   = (r^2 + a^2) P / Delta + a(L - a E) = T(r)
//        r^2 (dphi/dlambda) = a P / Delta + (L - a E)            = Phi(r)
//        where P = (r^2 + a^2)E - a L = r^2 + a(a - b)   (for E = 1)
//        Note: P/Delta + (L - aE) = [ (r^2 + a^2 - a b) + (b - a)(r^2 - 2Mr + a^2) ] / Delta
//
//    Radial Carter equation for null geodesics:
//      (r^2 dr/dlambda)^2 = R(r)
//      where R(r) = P^2 - Delta (L - a E)^2
//                 = [r^2 + a(a - b)]^2 - Delta (b - a)^2
//                 = r^4 + (a^2 - b^2) r^2 + 2 M r (b - a)^2
//
//    Second-order equation of motion for r (d^2r/dlambda^2):
//      Differentiating (dr/dlambda)^2 = R(r) / r^4 with respect to lambda:
//      2 (dr/dlambda) (d^2r/dlambda^2) = (dr/dlambda) * d/dr [ R(r) / r^4 ]
//      d^2r/dlambda^2 = (1 / (2 r^4)) R'(r) - (2 / r^5) R(r)
//      where:
//        R(r)  = r^4 + (a^2 - b^2) r^2 + 2 M r (b - a)^2
//        R'(r) = 4 r^3 + 2 (a^2 - b^2) r + 2 M (b - a)^2
//
// 3. Schwarzschild limit (a -> 0):
//    Delta = r^2 - 2Mr = r(r - Rs)
//    P = r^2, L = b
//    R(r) = r^4 - b^2 r^2 + 2 M r b^2 = r^2 [ r^2 - b^2 (1 - 2M/r) ]
//    dr/dlambda = +- sqrt(1 - (1 - Rs/r) b^2 / r^2)
//    dphi/dlambda = b / r^2
//    d^2r/dlambda^2 = - (Rs * b^2) / r^4 + b^2 / r^3 * (1 - Rs/r) ...
//    Matches exactly Geodesic.cpp and Binet equations in the equatorial plane!
//
// 4. Critical Impact Parameter b_crit(a) (Literature & Derivation):
//    Circular photon orbits satisfy R(r_ph) = 0 and R'(r_ph) = 0:
//      r_ph^3 + (a^2 - b^2) r_ph + 2 M (b - a)^2 = 0
//      4 r_ph^3 + 2 (a^2 - b^2) r_ph + 2 M (b - a)^2 = 0
//    Subtracting gives: 3 r_ph^3 + (a^2 - b^2) r_ph = 0 ==> b^2 - a^2 = 3 r_ph^2.
//    Substituting back yields Bardeen's famous cubic equation for the photon sphere radius:
//      r_ph^2 - 3 M r_ph \pm 2 a \sqrt{M r_ph} = 0
//    whose analytic solution was found by Bardeen (1972, 1973):
//      r_ph(prograde)   = 2 M [ 1 + cos( (2/3) arccos( -a / M ) ) ]
//      r_ph(retrograde) = 2 M [ 1 + cos( (2/3) arccos(  a / M ) ) ]
//    And the corresponding critical impact parameter b_crit = L / E is:
//      b_crit = - (r_ph^3 - 3 M r_ph^2 + a^2 r_ph + M a^2) / (a (r_ph - M))
//    Literature citations:
//      - Bardeen, Press, Teukolsky (1972), ApJ 178, 347 (Eq. 2.18)
//      - Bardeen (1973), in "Black Holes" (DeWitt & DeWitt eds.), Gordon and Breach
//      - Chandrasekhar (1983), "The Mathematical Theory of Black Holes", Oxford Univ. Press, Ch. 7
//      - Wikipedia: "Photon sphere" / "Kerr metric"
//
//    Known reference values for M = 1 (Rs = 2):
//      - a = 0:       r_ph = 3.0 M,  b_crit = \pm 3 \sqrt{3} M \approx \pm 5.196152 M
//      - a -> M (+):  r_ph -> 1.0 M, b_crit -> 2.0 M  (prograde)
//      - a -> M (-):  r_ph -> 4.0 M, b_crit -> -7.0 M (retrograde)
// ============================================================================

// Analytical critical impact parameter b_crit(a) for equatorial photons.
// Parameters:
//   a:        Kerr spin parameter (|a| <= M = Rs / 2)
//   rs:       Schwarzschild radius (Rs = 2M)
//   prograde: true for co-rotating photon (L > 0), false for counter-rotating (L < 0, returns magnitude |b_crit|)
double kerrBCrit(double a, double rs, bool prograde);

// Analytical photon sphere radius r_ph(a) for equatorial circular orbits.
double kerrPhotonSphereRadius(double a, double rs, bool prograde);

// Integrate an equatorial null geodesic in Kerr spacetime using RK4.
// Parameters:
//   b:       impact parameter L/E (positive for prograde launch, negative for retrograde)
//   a:       Kerr spin parameter (|a| <= Rs / 2)
//   rs:      Schwarzschild radius (Rs = 2M)
//   r0:      launch radius (e.g. 10.0 Rs)
//   step:    affine step size h (e.g. 0.005)
//   maxSteps:maximum integration steps
//   rEscape: radius considered escaped (e.g. 30.0 Rs)
KerrOrbitResult kerrPhotonOrbit(double b, double a, double rs,
                                double r0 = 10.0, double step = 0.005,
                                int maxSteps = 10000, double rEscape = 30.0);

}  // namespace astro
