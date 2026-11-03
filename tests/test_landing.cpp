// test_landing.cpp (v4.7.0 RC2): Planetary Landing, Deployable Gear & Surface Operations
#include "LandingSystem.h"

#include <cmath>
#include <iostream>
#include <string>

using namespace astro;

int main() {
  // Test 1: Gear state machine transitions
  {
    LandingTelemetry telem;
    if (telem.gearState != GearState::Retracted || telem.gearProgress != 0.0f) {
      std::cerr << "Test 1 failed: Initial gear state must be Retracted (0.0)\n";
      return 1;
    }

    // Request deploy
    updateLandingGear(telem, 0.016f, true);
    if (telem.gearState != GearState::Deploying) {
      std::cerr << "Test 1 failed: Toggle must initiate Deploying state\n";
      return 1;
    }

    // Halfway through transit (0.75s / 1.5s = 0.5)
    updateLandingGear(telem, 0.75f, false);
    if (std::abs(telem.gearProgress - 0.51f) > 0.05f) {
      std::cerr << "Test 1 failed: Expected ~0.5 progress at 0.75s, got " << telem.gearProgress << '\n';
      return 1;
    }

    // Complete deployment (another 0.8s)
    updateLandingGear(telem, 0.8f, false);
    if (telem.gearState != GearState::Deployed || telem.gearProgress != 1.0f) {
      std::cerr << "Test 1 failed: Gear must lock into Deployed (1.0)\n";
      return 1;
    }

    // Request retract
    updateLandingGear(telem, 0.016f, true);
    if (telem.gearState != GearState::Retracting) {
      std::cerr << "Test 1 failed: Toggle when deployed must initiate Retracting state\n";
      return 1;
    }

    // Complete retraction (1.6s)
    updateLandingGear(telem, 1.6f, false);
    if (telem.gearState != GearState::Retracted || telem.gearProgress != 0.0f) {
      std::cerr << "Test 1 failed: Gear must fully retract to Retracted (0.0)\n";
      return 1;
    }
  }

  // Test 2: Safe touchdown detection (|v_v| <= 2.0 m/s with gear locked)
  {
    LandingTelemetry telem;
    telem.gearState = GearState::Deployed;
    telem.gearProgress = 1.0f;

    float hullDmg = 0.0f, shieldDmg = 0.0f;
    updateTouchdownPhysics(telem, 0.016f, 1.2f, -1.4f, 0.2f, hullDmg, shieldDmg);

    if (telem.touchdown != TouchdownRating::Safe || !telem.isLanded) {
      std::cerr << "Test 2 failed: Expected Safe touchdown with isLanded true\n";
      return 1;
    }
    if (hullDmg > 0.0f || shieldDmg > 0.0f) {
      std::cerr << "Test 2 failed: Safe touchdown must deal zero damage\n";
      return 1;
    }
    if (telem.suspensionCompression <= 0.0f) {
      std::cerr << "Test 2 failed: Strut suspension must compress on landing\n";
      return 1;
    }
  }

  // Test 3: Hard landing penalty (2.0 < |v_v| <= 5.0 m/s)
  {
    LandingTelemetry telem;
    telem.gearState = GearState::Deployed;
    telem.gearProgress = 1.0f;

    float hullDmg = 0.0f, shieldDmg = 0.0f;
    updateTouchdownPhysics(telem, 0.016f, 1.0f, -3.8f, 0.1f, hullDmg, shieldDmg);

    if (telem.touchdown != TouchdownRating::Hard || !telem.isLanded) {
      std::cerr << "Test 3 failed: Expected Hard touchdown with isLanded true\n";
      return 1;
    }
    if (hullDmg <= 0.0f || shieldDmg <= 0.0f) {
      std::cerr << "Test 3 failed: Hard touchdown must inflict minor structural wear\n";
      return 1;
    }
    if (telem.suspensionCompression < 0.99f) {
      std::cerr << "Test 3 failed: Hard touchdown must bottom out suspension struts\n";
      return 1;
    }
  }

  // Test 4: Crash touchdown conditions
  {
    // Case 4A: Excessive velocity (|v_v| = 8.5 m/s)
    LandingTelemetry telemFast;
    telemFast.gearState = GearState::Deployed;
    telemFast.gearProgress = 1.0f;
    float hDmg = 0.0f, sDmg = 0.0f;
    updateTouchdownPhysics(telemFast, 0.016f, 1.0f, -8.5f, 0.0f, hDmg, sDmg);

    if (telemFast.touchdown != TouchdownRating::Crash || telemFast.isLanded) {
      std::cerr << "Test 4A failed: High speed impact must result in Crash without landed state\n";
      return 1;
    }
    if (hDmg < 40.0f || sDmg < 60.0f) {
      std::cerr << "Test 4A failed: Crash impact must deal major damage\n";
      return 1;
    }

    // Case 4B: Gear retracted belly flop (|v_v| = 1.5 m/s)
    LandingTelemetry telemNoGear;
    telemNoGear.gearState = GearState::Retracted;
    telemNoGear.gearProgress = 0.0f;
    hDmg = 0.0f; sDmg = 0.0f;
    updateTouchdownPhysics(telemNoGear, 0.016f, 1.0f, -1.5f, 0.0f, hDmg, sDmg);

    if (telemNoGear.touchdown != TouchdownRating::Crash || telemNoGear.isLanded) {
      std::cerr << "Test 4B failed: Touchdown without gear must register as Crash\n";
      return 1;
    }
    if (hDmg < 30.0f || sDmg < 50.0f) {
      std::cerr << "Test 4B failed: Belly impact without gear must deal heavy damage\n";
      return 1;
    }
  }

  // Test 5: Inhibit gear retraction when securely landed
  {
    LandingTelemetry telem;
    telem.gearState = GearState::Deployed;
    telem.gearProgress = 1.0f;
    telem.isLanded = true;

    updateLandingGear(telem, 0.016f, true);
    if (telem.gearState != GearState::Deployed || telem.gearProgress != 1.0f) {
      std::cerr << "Test 5 failed: Ground interlock must prevent gear retraction while landed\n";
      return 1;
    }
  }

  // Test 6: Takeoff transition and ground dust kickup
  {
    LandingTelemetry telem;
    telem.gearState = GearState::Deployed;
    telem.gearProgress = 1.0f;
    telem.isLanded = true;
    telem.suspensionCompression = 0.8f;

    // Apply vertical takeoff thrust
    float hDmg = 0.0f, sDmg = 0.0f;
    updateTouchdownPhysics(telem, 0.016f, 5.0f, 1.8f, 0.9f, hDmg, sDmg);

    if (telem.isLanded || telem.touchdown != TouchdownRating::Airborne) {
      std::cerr << "Test 6 failed: Ship must transition to Airborne upon lift-off thrust\n";
      return 1;
    }
    if (telem.surfaceDustK <= 0.5f) {
      std::cerr << "Test 6 failed: Low altitude full thrust must generate prominent dust kickup\n";
      return 1;
    }

    // At 200m altitude (outside ground effect)
    updateTouchdownPhysics(telem, 0.016f, 200.0f, 10.0f, 1.0f, hDmg, sDmg);
    if (telem.surfaceDustK != 0.0f) {
      std::cerr << "Test 6 failed: Dust kickup must be 0 above ground effect altitude\n";
      return 1;
    }
  }

  // Test 7: Surface expedition report catalog generation
  {
    SurfaceReport mars = generateSurfaceReport("mars");
    if (mars.nameEn != "Mars" || std::abs(mars.gravityMps2 - 3.72f) > 0.05f) {
      std::cerr << "Test 7 failed: Mars telemetry metrics incorrect\n";
      return 1;
    }
    if (mars.researchPointsReward != 150) {
      std::cerr << "Test 7 failed: Surface expedition reward must be 150 RP\n";
      return 1;
    }

    SurfaceReport moon = generateSurfaceReport("moon");
    if (moon.nameEn != "The Moon" || std::abs(moon.gravityMps2 - 1.62f) > 0.05f || moon.pressureBar != 0.0f) {
      std::cerr << "Test 7 failed: Moon vacuum/gravity telemetry incorrect\n";
      return 1;
    }

    SurfaceReport titan = generateSurfaceReport("titan");
    if (titan.pressureBar <= 1.0f || titan.geologyRu.find("метан") == std::string::npos) {
      std::cerr << "Test 7 failed: Titan dense atmosphere / hydrocarbon geology missing\n";
      return 1;
    }
  }

  std::cout << "All 7 LandingSystem tests passed successfully!\n";
  return 0;
}
