#pragma once

namespace astro {

// CPU reference of blackHole geodesic math (Schwarzschild, Rs=1 by default).
// Mirrors BlackHoleRaytrace.shader: cartToSchwarzschild,
// cartDirectionToSchwarzschildVelocity, geodesicAcceleration, rk4Step.
struct SchwState {
  double t = 0.0, r = 10.0, theta = 1.5707, phi = 0.0;
  double dt = 1.0, dr = 0.0, dtheta = 0.0, dphi = 0.0;
};

struct SchwAccel {
  double ddt = 0.0, ddr = 0.0, ddtheta = 0.0, ddphi = 0.0;
};

SchwAccel geodesicAcceleration(const SchwState& s, double rs);

// Re-project dt/dLambda so the geodesic stays null (ds2 = 0):
// f = 1 - Rs/r; spatial = dr^2/f + r^2 dTheta^2 + r^2 sin^2(Theta) dPhi^2;
// dt = sqrt(spatial / f). Bit-for-bit mirror of renormalizeTime() in
// shaders/bh.frag; called at the end of rk4Step().
void renormalizeTime(SchwState& s, double rs);

void rk4Step(double h, SchwState& s, double rs);

// One adaptive RKF45 (Fehlberg 5(4)) step: advances s by a single accepted
// step with embedded error control. h is the trial step, tol the relative
// error tolerance. On true the step was taken, renormalizeTime() applied and
// hNext carries the controller's suggested next trial; false means the
// trial never converged (s left untouched).
bool rkf45Step(double h, double tol, SchwState& s, double rs, double& hNext);

// Adaptive error-controlled ray tracing until capture (r<=rs) or escape
// (r>=shell) or maxSteps accepted-steps budget. Advances s in place so the
// caller can inspect the endpoint. Optional stats (may be null): accepted
// acceleration evaluations (6 per trial step; rejected trials included).
struct Rkf45Stats {
  long evals = 0;
};

int traceRayRkf45(SchwState& s, double rs, double shell, double h0, double tol,
                  int maxSteps, bool& captured, Rkf45Stats* stats = nullptr);

// Returns steps until capture (r<=rs) or escape (r>=shell), or maxSteps.
int traceRay(SchwState s, double rs, double shell, double step, int maxSteps,
             bool& captured);

}  // namespace astro
