#pragma once

#include "WarpMetric.h"

namespace astro {

// 4-position x^mu and covariant 4-momentum p_mu
struct WarpRayState {
  double t = 0.0;
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  double pt = -1.0;  // p_0 = -E (E=1)
  double px = 0.0;
  double py = 0.0;
  double pz = 0.0;
  double lambda = 0.0;  // affine parameter
};

// Evaluates the Hamiltonian H = 1/2 g^{mu nu} p_mu p_nu (must be 0 for null rays)
double evalHamiltonian(const WarpMetric& metric, const WarpRayState& s);

// Exact null cone projection: rescales spatial momentum p_i -> k * p_i so that H = 0
void renormalizeNull(const WarpMetric& metric, WarpRayState& s);

// Evaluates Hamiltonian equations of motion derivatives:
//   dx^mu / dlambda = dH / dp_mu = g^{mu nu} p_nu
//   dp_mu / dlambda = -dH / dx^mu = -1/2 (d g^{alpha beta} / dx^mu) p_alpha p_beta
void evalHamiltonianDerivatives(const WarpMetric& metric, const WarpRayState& s,
                                double dxdlambda[4], double dpdlambda[4]);

// Advances the null ray by step h using standard 4th-order Runge-Kutta
void warpRk4Step(const WarpMetric& metric, double h, WarpRayState& s);

struct WarpTraceResult {
  bool captured = false;
  int steps = 0;
  double finalR = 0.0;
  double maxHConstraint = 0.0;
};

// Traces a null ray until either horizon capture (r <= horizonRadius * 1.001)
// or escape to asymptotic region (r >= rMax).
WarpTraceResult traceWarpRay(const WarpMetric& metric, WarpRayState s,
                             double rMax, double h, int maxSteps);

// Constructs an equatorial inward-directed ray with impact parameter b launched from r0:
// Energy E = 1 (p_t = -1), angular momentum L = b, directed inward along -X axis.
WarpRayState makeWarpInwardRay(const WarpMetric& metric, double r0, double b);

// ============================================================================
// Skiff Warp Waypoint & Trajectory Generator (M2 Framework)
// ============================================================================
struct SkiffWaypoint {
  double t = 0.0;          // coordinate time
  double x = 0.0;          // bubble center X
  double y = 0.0;          // bubble center Y
  double z = 0.0;          // bubble center Z
  double vx = 0.0;         // bubble velocity X
  double vy = 0.0;         // bubble velocity Y
  double vz = 0.0;         // bubble velocity Z
  double v = 0.0;          // scalar speed |v|
  double tau = 0.0;        // proper time of comoving observer inside bubble
  double thetaMax = 0.0;   // peak York time expansion scalar at this instant
  double wallDist = 0.0;   // clearance distance from ship (center) to wall
  bool wallSafe = true;    // true if ship is safely in flat interior (wall does not intersect waypoint)
  bool dtsDtauValid = true;// true if |dts/dtau| <= 1 outside bubble
};

class WarpGeodesic {
public:
  // Generates a deterministic Skiff warp path from start to end with tanh acceleration/deceleration.
  // Performs internal safety checks:
  // - |dts/dtau| <= 1 outside bubble (proper time invariant)
  // - Bubble wall does not intersect waypoint
  // - Exact time-reversal and roundtrip symmetry
  static std::vector<SkiffWaypoint> skiffPath(
      const double start[3], const double end[3],
      double vMax = 5.0, double sigma = 0.5, int steps = 100);
};

// Direct namespace-level helper
std::vector<SkiffWaypoint> skiffPath(
    const double start[3], const double end[3],
    double vMax = 5.0, double sigma = 0.5, int steps = 100);

}  // namespace astro
