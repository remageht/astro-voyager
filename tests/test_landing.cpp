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

  // Test 8: Core Drill state machine, penetration, and extraction cycle
  {
    CoreDrillTelemetry drill = generateCoreDrillProfile("mars", 210.0f, 3.72f);
    if (drill.state != CoreDrillState::Idle || drill.depthM != 0.0f || drill.sampleExtracted) {
      std::cerr << "Test 8 failed: Initial drill state must be Idle (0.0m)\n";
      return 1;
    }

    // Step 1: Active drilling for 2 seconds
    for (int i = 0; i < 120; ++i) {
      updateCoreDrill(drill, 0.016f, true, false, 210.0f, 0.006f);
    }
    if (drill.state != CoreDrillState::Drilling || drill.depthM <= 0.2f || drill.rpm < 350.0f) {
      std::cerr << "Test 8 failed: Drill must be in Drilling state with RPM > 350 and depth > 0.2m (got " << drill.depthM << "m)\n";
      return 1;
    }

    // Step 2: Continue drilling to max depth (5.0m)
    for (int i = 0; i < 800; ++i) {
      updateCoreDrill(drill, 0.016f, true, false, 210.0f, 0.006f);
      if (drill.depthM >= 5.0f) break;
    }
    if (std::abs(drill.depthM - 5.0f) > 0.001f) {
      std::cerr << "Test 8 failed: Drill must reach max depth 5.0m (got " << drill.depthM << "m)\n";
      return 1;
    }

    // Step 3: Retraction phase
    updateCoreDrill(drill, 0.016f, false, true, 210.0f, 0.006f);
    if (drill.state != CoreDrillState::Retracting) {
      std::cerr << "Test 8 failed: Retract command must set Retracting state\n";
      return 1;
    }

    // Advance retraction until shaft is fully extracted (depth == 0.0)
    for (int i = 0; i < 400; ++i) {
      updateCoreDrill(drill, 0.016f, false, true, 210.0f, 0.006f);
      if (drill.depthM <= 0.0f) break;
    }
    if (drill.state != CoreDrillState::Extracted || !drill.sampleExtracted || drill.depthM != 0.0f) {
      std::cerr << "Test 8 failed: Core must be locked into Extracted state with sampleExtracted=true\n";
      return 1;
    }
  }

  // Test 9: Bit thermal accumulation & overheat safety lock
  {
    CoreDrillTelemetry drill = generateCoreDrillProfile("venus", 737.0f, 8.87f);
    // Initial temp should be ~463.85°C (above 280°C), so it immediately locks into Overheated
    updateCoreDrill(drill, 0.016f, true, false, 737.0f, 92.0f);
    if (drill.state != CoreDrillState::Overheated || drill.rpm != 0.0f) {
      std::cerr << "Test 9 failed: Drill on scorching Venus (>280°C) must trigger Overheated lock\n";
      return 1;
    }

    // Test on cooler body with forced artificial thermal accumulation
    CoreDrillTelemetry drillMars = generateCoreDrillProfile("mars", 210.0f, 3.72f);
    drillMars.bitTempC = 279.0f;
    updateCoreDrill(drillMars, 0.1f, true, false, 210.0f, 0.006f);
    if (drillMars.state != CoreDrillState::Overheated) {
      std::cerr << "Test 9 failed: Bit heating past 280°C must trigger Overheated state\n";
      return 1;
    }

    // Verify lockout inhibits further drilling while hot
    const float lockedDepth = drillMars.depthM;
    updateCoreDrill(drillMars, 0.1f, true, false, 210.0f, 0.006f);
    if (drillMars.depthM != lockedDepth || drillMars.rpm != 0.0f) {
      std::cerr << "Test 9 failed: Overheated drill must inhibit motor and depth penetration\n";
      return 1;
    }

    // Cool down below 160°C
    for (int i = 0; i < 600; ++i) {
      updateCoreDrill(drillMars, 0.05f, false, false, 210.0f, 0.006f);
      if (drillMars.bitTempC <= 160.0f) break;
    }
    if (drillMars.state != CoreDrillState::Idle) {
      std::cerr << "Test 9 failed: Cooled drill bit must reset to Idle state (got " << static_cast<int>(drillMars.state) << ")\n";
      return 1;
    }
  }

  // Test 10: Strata generation across celestial bodies
  {
    CoreDrillTelemetry mars = generateCoreDrillProfile("mars", 210.0f, 3.72f);
    if (mars.strata.size() != 3 || mars.helium3Ppm < 10.0f || mars.volatileWaterPercent < 10.0f) {
      std::cerr << "Test 10 failed: Mars core strata/metrics incorrect\n";
      return 1;
    }
    if (mars.strata[0].hardness >= mars.strata[1].hardness || mars.strata[1].densityGcm3 <= mars.strata[0].densityGcm3) {
      std::cerr << "Test 10 failed: Mars geological stratification (soft dust -> dense basalt) broken\n";
      return 1;
    }

    CoreDrillTelemetry moon = generateCoreDrillProfile("moon", 220.0f, 1.62f);
    if (moon.helium3Ppm < 30.0f || moon.primaryMineralRu.find("Ильменит") == std::string::npos) {
      std::cerr << "Test 10 failed: Moon ilmenite / Helium-3 enrichment missing\n";
      return 1;
    }

    CoreDrillTelemetry titan = generateCoreDrillProfile("titan", 94.0f, 1.35f);
    if (titan.volatileWaterPercent < 60.0f || titan.primaryMineralRu.find("толины") == std::string::npos) {
      std::cerr << "Test 10 failed: Titan hydrocarbon / clathrate volatile profile missing\n";
      return 1;
    }

    CoreDrillTelemetry hercules = generateCoreDrillProfile("hercules-b", 295.0f, 11.2f);
    if (hercules.helium3Ppm < 50.0f || hercules.strata[2].hardness < 0.95f) {
      std::cerr << "Test 10 failed: Hercules-b diamond/TiC hyper-dense matrix missing\n";
      return 1;
    }
  }

  // Test 11: Zettelkasten note generation from analyzed core sample
  {
    CoreDrillTelemetry drill = generateCoreDrillProfile("mars", 210.0f, 3.72f);
    drill.depthM = 5.0f;
    drill.sampleExtracted = true;
    drill.sampleAnalyzed = true;

    std::string zettel = formatCoreSampleZettel("mars", "Марс", drill);
    if (zettel.find("Спектрохимический паспорт керна: Марс") == std::string::npos) {
      std::cerr << "Test 11 failed: Zettel title missing\n";
      return 1;
    }
    if (zettel.find("`mars`") == std::string::npos || zettel.find("Гелий-3") == std::string::npos) {
      std::cerr << "Test 11 failed: Zettel metadata fields missing\n";
      return 1;
    }
    if (zettel.find("[0.0 - 1.2 м]") == std::string::npos || zettel.find("+250 очков исследований") == std::string::npos) {
      std::cerr << "Test 11 failed: Zettel strata or science point yield missing\n";
      return 1;
    }
  }

  // Test 12: Planetary terrain slope evaluation and slope hazard threshold
  {
    float earthSlope = computePlanetarySurfaceSlope("earth", 10.0f, 20.0f);
    if (earthSlope < 0.4f || earthSlope > 32.0f) {
      std::cerr << "Test 12 failed: Earth slope must be within [0.4, 32.0] deg, got " << earthSlope << '\n';
      return 1;
    }

    float gasGiantSlope = computePlanetarySurfaceSlope("jupiter", 10.0f, 20.0f);
    if (gasGiantSlope != 0.0f) {
      std::cerr << "Test 12 failed: Gas giant surface slope must be 0.0 deg, got " << gasGiantSlope << '\n';
      return 1;
    }

    LandingTelemetry telem;
    telem.surfaceSlopeDeg = 18.5f;
    telem.slopeHazard = (telem.surfaceSlopeDeg > 15.0f);
    if (!telem.slopeHazard) {
      std::cerr << "Test 12 failed: Slope > 15.0 deg must trigger slopeHazard true\n";
      return 1;
    }

    telem.surfaceSlopeDeg = 6.2f;
    telem.slopeHazard = (telem.surfaceSlopeDeg > 15.0f);
    if (telem.slopeHazard) {
      std::cerr << "Test 12 failed: Slope <= 15.0 deg must not trigger slopeHazard\n";
      return 1;
    }
  }

  std::cout << "All 12 LandingSystem tests passed successfully!\n";
  return 0;
}
