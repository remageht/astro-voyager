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

// Frequency ratio g = nu_obs / nu_emit for a photon with covariant momentum
// p[4] seen by an observer with contravariant 4-velocity uObs[4] and emitted
// by a source with 4-velocity uEmit[4], all at the same event:
//   g = (-p_mu uObs^mu) / (-p_mu uEmit^mu).
// Future-directed null rays and timelike 4-velocities give a positive ratio.
double observedFrequencyRatio(const double p[4], const double uObs[4],
                              const double uEmit[4]);

// Crew-frame shift in the Alcubierre bubble. A photon arrives along the
// coordinate direction n[3] (unit, direction of propagation) while the bubble
// moves with speed beta along dir[3] (unit). The comoving observer measures
//   g = 1 - beta * (n . dir),
// so a star ahead (n = -dir) gives 1 + beta and a star behind 1 - beta.
// This is the exact small-wall Alcubierre result and must match the invariant
// g = (-p.u_obs)/(-p.u_emit) along a traced ray.
double alcubierreCrewShift(double beta, const double n[3], const double dir[3]);

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
  double wallClearance = 0.0; // R - 2*sigma: distance from the centred ship to the inner wall edge
  double dtsDtau = 1.0;       // coordinate-to-proper time rate 1/lapse at this waypoint
};

class WarpGeodesic {
public:
  // Generates a deterministic Skiff warp path from start to end with tanh
  // acceleration/deceleration. The ship sits at the bubble centre by
  // construction, so the inner-wall clearance is R - 2*sigma and the
  // coordinate/proper time rate is 1/lapse. Both are reported as derived
  // values; the caller decides whether they are acceptable. There are no
  // boolean "safety" flags, which would be constant by definition here.
  // Exact time-reversal and roundtrip symmetry are preserved.
  static std::vector<SkiffWaypoint> skiffPath(
      const double start[3], const double end[3],
      double vMax = 5.0, double sigma = 0.5, int steps = 100,
      double bubbleRadius = -1.0, double lapse = 1.0);
};

// Direct namespace-level helper
std::vector<SkiffWaypoint> skiffPath(
    const double start[3], const double end[3],
    double vMax = 5.0, double sigma = 0.5, int steps = 100,
    double bubbleRadius = -1.0, double lapse = 1.0);

}  // namespace astro
