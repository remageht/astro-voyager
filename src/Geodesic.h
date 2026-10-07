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

// Returns steps until capture (r<=rs) or escape (r>=shell), or maxSteps.
int traceRay(SchwState s, double rs, double shell, double step, int maxSteps,
             bool& captured);

}  // namespace astro
