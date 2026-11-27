#include "LandingSystem.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace astro {

void updateLandingGear(LandingTelemetry& telem, float deltaTime, bool toggleRequested) {
  constexpr float kDeploySpeed = 1.0f / 1.5f;  // 1.5s transit time

  if (toggleRequested) {
    // If securely landed, gear cannot be retracted on the ground
    if (!telem.isLanded) {
      if (telem.gearState == GearState::Retracted || telem.gearState == GearState::Retracting) {
        telem.gearState = GearState::Deploying;
      } else if (telem.gearState == GearState::Deployed || telem.gearState == GearState::Deploying) {
        telem.gearState = GearState::Retracting;
      }
    }
  }

  if (telem.gearState == GearState::Deploying) {
    telem.gearProgress += deltaTime * kDeploySpeed;
    if (telem.gearProgress >= 1.0f) {
      telem.gearProgress = 1.0f;
      telem.gearState = GearState::Deployed;
    }
  } else if (telem.gearState == GearState::Retracting) {
    telem.gearProgress -= deltaTime * kDeploySpeed;
    if (telem.gearProgress <= 0.0f) {
      telem.gearProgress = 0.0f;
      telem.gearState = GearState::Retracted;
    }
  }
}

void updateTouchdownPhysics(LandingTelemetry& telem,
                           float deltaTime,
                           float altitudeM,
                           float verticalVelMs,
                           float thrustK,
                           float& outHullDamage,
                           float& outShieldDamage) {
  outHullDamage = 0.0f;
  outShieldDamage = 0.0f;
  telem.radarAltM = altitudeM;
  telem.verticalSpeedMs = verticalVelMs;

  // 1. Dynamic Surface Dust Kickup:
  // When engines/thrusters are firing near the ground (< 120m)
  if (altitudeM >= 0.0f && altitudeM < 120.0f) {
    const float prox = (120.0f - altitudeM) / 120.0f;
    telem.surfaceDustK = std::clamp(prox * (0.35f + 0.65f * thrustK), 0.0f, 1.0f);
  } else {
    telem.surfaceDustK = 0.0f;
  }

  // 2. Touchdown Evaluation (Contact threshold <= 1.5 meters)
  constexpr float kContactAltitudeM = 1.5f;

  if (!telem.isLanded) {
    if (altitudeM <= kContactAltitudeM && verticalVelMs <= 0.0f) {
      const float impactSpeed = std::abs(verticalVelMs);

      if (telem.gearState == GearState::Deployed) {
        if (impactSpeed <= 2.0f) {
          // Safe touchdown: nominal shock absorption
          telem.touchdown = TouchdownRating::Safe;
          telem.isLanded = true;
          telem.suspensionCompression = std::clamp(impactSpeed / 2.0f * 0.70f + 0.30f, 0.0f, 1.0f);
        } else if (impactSpeed <= 5.0f) {
          // Hard touchdown: suspension bottomed out, structural strain
          telem.touchdown = TouchdownRating::Hard;
          telem.isLanded = true;
          telem.suspensionCompression = 1.0f;
          outShieldDamage = (impactSpeed - 2.0f) * 6.0f;
          outHullDamage = (impactSpeed - 2.0f) * 3.5f;
        } else {
          // Crash impact: excessive velocity
          telem.touchdown = TouchdownRating::Crash;
          telem.isLanded = false;
          telem.suspensionCompression = 1.0f;
          outShieldDamage = 50.0f + (impactSpeed - 5.0f) * 10.0f;
          outHullDamage = 30.0f + (impactSpeed - 5.0f) * 8.0f;
        }
      } else {
        // Belly flop without gear locked: severe damage
        telem.touchdown = TouchdownRating::Crash;
        telem.isLanded = false;
        telem.suspensionCompression = 0.0f;
        outShieldDamage = 80.0f + impactSpeed * 5.0f;
        outHullDamage = 50.0f + impactSpeed * 4.0f;
      }
    }
  } else {
    // 3. Landed State Maintenance & Takeoff
    if (thrustK > 0.55f && verticalVelMs > 0.4f) {
      // Lift-off!
      telem.isLanded = false;
      telem.touchdown = TouchdownRating::Airborne;
      telem.suspensionCompression = 0.0f;
    } else {
      // Settle suspension towards static rest (0.25 compression)
      if (telem.suspensionCompression > 0.25f) {
        telem.suspensionCompression -= deltaTime * 0.6f;
        if (telem.suspensionCompression < 0.25f) telem.suspensionCompression = 0.25f;
      }
    }
  }
}

float computePlanetarySurfaceSlope(const std::string& bodyId, float x, float z) {
  // Multi-scale procedural terrain gradient modeling craters, dunes and basalt ridges
  const float n1 = std::sin(x * 0.045f) * std::cos(z * 0.038f);
  const float n2 = std::sin(x * 0.120f + 1.2f) * std::cos(z * 0.145f + 0.8f);
  const float n3 = std::sin((x + z) * 0.35f);

  float slope = std::abs(n1 * 9.5f + n2 * 6.0f + n3 * 2.5f);

  // Body-specific terrain relief modifiers
  if (bodyId == "moon" || bodyId == "mercury") {
    slope *= 1.35f;
  } else if (bodyId == "mars") {
    slope *= 1.20f;
  } else if (bodyId == "jupiter" || bodyId == "saturn" || bodyId == "uranus" || bodyId == "neptune") {
    return 0.0f;
  }
  return std::clamp(slope, 0.4f, 32.0f);
}

SurfaceReport generateSurfaceReport(const std::string& bodyId) {
  SurfaceReport rep;
  rep.bodyId = bodyId;
  rep.researchPointsReward = 150;
  rep.samplesCollected = false;

  if (bodyId == "mars") {
    rep.nameRu = "Марс";
    rep.nameEn = "Mars";
    rep.biomeRu = "Красная базальтовая пустыня и каньоны";
    rep.biomeEn = "Red basaltic desert and rift canyons";
    rep.gravityMps2 = 3.72f;
    rep.pressureBar = 0.00636f;
    rep.tempK = 210.0f;
    rep.geologyRu = "Высокое содержание оксидов железа (Fe2O3), оливиновые базальты, вулканические туфы горы Олимп.";
    rep.geologyEn = "High ferric oxide (Fe2O3) regolith, olivine basalts, volcanic ash of Olympus Mons.";
    rep.atmosphereRu = "Разреженная газовая оболочка: 95.3% CO2, 2.6% N2, 1.9% Ar, следы водяного пара.";
    rep.atmosphereEn = "Thin atmospheric envelope: 95.3% CO2, 2.6% N2, 1.9% Ar, trace H2O.";
  } else if (bodyId == "moon") {
    rep.nameRu = "Луна";
    rep.nameEn = "The Moon";
    rep.biomeRu = "Безатмосферный титанистый реголит";
    rep.biomeEn = "Airless titanium-bearing regolith";
    rep.gravityMps2 = 1.62f;
    rep.pressureBar = 0.0f;
    rep.tempK = 220.0f;
    rep.geologyRu = "Анортозитовая кора возвышенностей, базальтовые брекчии лунных морей, обогащённые ильменитом (FeTiO3) и He-3.";
    rep.geologyEn = "Highland anorthosite crust, mare basalt breccias rich in ilmenite (FeTiO3) and trapped solar Helium-3.";
    rep.atmosphereRu = "Жёсткий вакуум. Экзосфера из захваченных ионов солнечного ветра (Ne, Ar, H).";
    rep.atmosphereEn = "Hard vacuum. Tenuous exosphere formed by trapped solar wind ions (Ne, Ar, H).";
  } else if (bodyId == "earth" || bodyId == "sol") {
    rep.nameRu = "Земля";
    rep.nameEn = "Earth";
    rep.biomeRu = "Умеренная океаническая биосфера";
    rep.biomeEn = "Temperate oceanic biosphere";
    rep.gravityMps2 = 9.81f;
    rep.pressureBar = 1.01325f;
    rep.tempK = 288.15f;
    rep.geologyRu = "Динамическая тектоника плит: гранитные континентальные щиты, базальтовое океаническое дно, осадочные карбонаты.";
    rep.geologyEn = "Active plate tectonics: granitic continental cratons, basaltic seafloor, biogenic carbonates.";
    rep.atmosphereRu = "Плотная азотно-кислородная среда: 78.08% N2, 20.95% O2, 0.93% Ar, 0.4% H2O, озоновый щит.";
    rep.atmosphereEn = "Dense nitrogen-oxygen biosphere: 78.08% N2, 20.95% O2, 0.93% Ar, 0.4% H2O, ozone screen.";
  } else if (bodyId == "venus") {
    rep.nameRu = "Венера";
    rep.nameEn = "Venus";
    rep.biomeRu = "Сверхкритическая пирокластическая равнина";
    rep.biomeEn = "Supercritical pyroclastic plains";
    rep.gravityMps2 = 8.87f;
    rep.pressureBar = 92.0f;
    rep.tempK = 737.0f;
    rep.geologyRu = "Базальтовые лавовые реки, венцы и патеры, покрытые тонким слоем электропроводящего полупроводникового пирита.";
    rep.geologyEn = "Basaltic lava flows, coronae and pancake domes frosted with conductive semiconductor pyrite.";
    rep.atmosphereRu = "Экстремальный парниковый котёл: 96.5% CO2, 3.5% N2, непроницаемый слой капель H2SO4.";
    rep.atmosphereEn = "Runaway greenhouse inferno: 96.5% CO2, 3.5% N2, opaque sulphuric acid cloud deck.";
  } else if (bodyId == "titan") {
    rep.nameRu = "Титан";
    rep.nameEn = "Titan";
    rep.biomeRu = "Криовулканические дюны и углеводородные озёра";
    rep.biomeEn = "Cryovolcanic dunes and hydrocarbon seas";
    rep.gravityMps2 = 1.35f;
    rep.pressureBar = 1.45f;
    rep.tempK = 93.7f;
    rep.geologyRu = "Кристаллический водяной лёд с плотностью гранита, углеводородные дюны из органических толинов, жидкий метан/этан.";
    rep.geologyEn = "Granite-hard water ice bedrock, tholin organic dune fields, liquid methane/ethane seas.";
    rep.atmosphereRu = "Плотная азотная среда: 98.4% N2, 1.4% CH4, 0.2% H2, густой оранжевый фотохимический смог.";
    rep.atmosphereEn = "Dense nitrogen atmosphere: 98.4% N2, 1.4% CH4, 0.2% H2, dense orange photochemical haze.";
  } else if (bodyId == "europa") {
    rep.nameRu = "Европа";
    rep.nameEn = "Europa";
    rep.biomeRu = "Хаотический ледяной панцирь и линеи";
    rep.biomeEn = "Chaotic fractured ice shell and lineae";
    rep.gravityMps2 = 1.315f;
    rep.pressureBar = 0.0f;
    rep.tempK = 102.0f;
    rep.geologyRu = "Тектонические разломы водяного льда толщиной 15-25 км над подповерхностным глобальным океаном, соли сульфата магния.";
    rep.geologyEn = "Tectonically fractured 15-25 km ice shell over global sub-surface ocean, magnesium sulphate salts.";
    rep.atmosphereRu = "Ультраразреженная радиационная атмосфера молекулярного кислорода (O2) от радиолиза льда.";
    rep.atmosphereEn = "Tenuous radiolytic molecular oxygen (O2) atmosphere generated by jovian magnetospheric sputtering.";
  } else if (bodyId == "hercules-b") {
    rep.nameRu = "Геркулес b";
    rep.nameEn = "Hercules b";
    rep.biomeRu = "Суперземля скопления M13: кремниевые плато";
    rep.biomeEn = "M13 Super-Earth: silicate highlands";
    rep.gravityMps2 = 11.2f;
    rep.pressureBar = 1.25f;
    rep.tempK = 295.0f;
    rep.geologyRu = "Древняя континентальная кора возрастом 11 млрд лет, богатая тугоплавкими силикатами, пироксенами и кристаллическим кварцем.";
    rep.geologyEn = "Ancient 11-billion-year craton rich in refractory silicates, pyroxenes, and crystalline quartz.";
    rep.atmosphereRu = "Стабильная атмосфера: 75% N2, 18% O2, 5% CO2, 2% Ar. Пригодна для создания постоянной колонии.";
    rep.atmosphereEn = "Stable atmosphere: 75% N2, 18% O2, 5% CO2, 2% Ar. Prime candidate for permanent human colony.";
  } else {
    rep.nameRu = "Планетоид " + bodyId;
    rep.nameEn = "Planetoid " + bodyId;
    rep.biomeRu = "Скалистая поверхность глубокого космоса";
    rep.biomeEn = "Deep space rocky surface";
    rep.gravityMps2 = 2.5f;
    rep.pressureBar = 0.0f;
    rep.tempK = 150.0f;
    rep.geologyRu = "Силикатный реголит, хондритовые брекчии, ударные кратеры.";
    rep.geologyEn = "Silicate regolith, chondritic breccia, impact cratering.";
    rep.atmosphereRu = "Безатмосферное тело / вакуум.";
    rep.atmosphereEn = "Airless body / vacuum.";
  }

  return rep;
}

// === v5.3.0 Surface Science & Core Sampler ===
CoreDrillTelemetry generateCoreDrillProfile(const std::string& bodyId, float surfaceTempK,
                                           float surfaceGravity) {
  (void)surfaceGravity;
  CoreDrillTelemetry d;
  d.strata.clear();
  d.bitTempC = std::max(20.0f, surfaceTempK - 273.15f);

  auto addLayer = [&](float mn, float mx, const std::string& ru, const std::string& en,
                      const std::string& compRu, const std::string& compEn, float hard,
                      float dens, float tK) {
    CoreStrataLayer L;
    L.depthMinM = mn;
    L.depthMaxM = mx;
    L.layerNameRu = ru;
    L.layerNameEn = en;
    L.compositionRu = compRu;
    L.compositionEn = compEn;
    L.hardness = hard;
    L.densityGcm3 = dens;
    L.tempK = tK;
    d.strata.push_back(L);
  };

  std::string bid = bodyId;
  for (char& c : bid) c = (char)std::tolower((unsigned char)c);

  if (bid == "mars") {
    d.primaryMineralRu = "Оливиновый базальт и ярозит";
    d.primaryMineralEn = "Olivine Basalt & Jarosite";
    d.rareElementsRu = "Оливиновые базальты, вечномерзлотный лед";
    d.rareElementsEn = "Olivine basalts, permafrost ice";
    d.helium3Ppm = 14.2f;
    d.volatileWaterEquivalentPercent = 12.8f;
    addLayer(0.0f, 1.2f, "Нанореголит", "Nanoregolith",
             "Fe2O3, пыль", "Fe2O3 dust", 0.25f, 1.4f, surfaceTempK);
    addLayer(1.2f, 3.4f, "Оливиновый базальт", "Olivine basalt",
             "SiO2, Fe, Mg", "SiO2, Fe, Mg", 0.6f, 2.9f, surfaceTempK);
    addLayer(3.4f, 5.0f, "Вечномерзлотный лед", "Permafrost ice",
             "H2O ice", "H2O ice", 0.4f, 0.92f, surfaceTempK);
  } else if (bid == "moon" || bid == "luna") {
    d.primaryMineralRu = "Ильменит (FeTiO3) и анортозит";
    d.primaryMineralEn = "Ilmenite (FeTiO3) & Anorthosite";
    d.rareElementsRu = "Титаномагнетитовые брекчии, анортозит";
    d.rareElementsEn = "Titanomagnetite breccia, anorthosite";
    d.helium3Ppm = 38.5f;
    d.volatileWaterEquivalentPercent = 0.5f;
    addLayer(0.0f, 1.2f, "Микрореголит", "Micrometeorite regolith",
             "Agglutinates, glass", "Agglutinates, glass", 0.3f, 1.5f, surfaceTempK);
    addLayer(1.2f, 3.4f, "Титаномагнетитовая брекчия", "Titanomagnetite breccia",
             "FeTiO3", "FeTiO3", 0.7f, 3.2f, surfaceTempK);
    addLayer(3.4f, 5.0f, "Анортозитовая кора", "Anorthosite crust",
             "CaAl2Si2O8", "CaAl2Si2O8", 0.65f, 2.7f, surfaceTempK);
  } else if (bid == "titan") {
    d.primaryMineralRu = "Органические толины и клатраты метана";
    d.primaryMineralEn = "Organic Tholins & Methane Clathrates";
    d.rareElementsRu = "Клатратные гидраты метана, водяной крио-лед";
    d.rareElementsEn = "Methane clathrates, water cryo-ice";
    d.helium3Ppm = 2.1f;
    d.volatileWaterEquivalentPercent = 64.5f;
    addLayer(0.0f, 1.2f, "Толины", "Tholins",
             "C/H/N polymers", "C/H/N polymers", 0.2f, 1.1f, surfaceTempK);
    addLayer(1.2f, 3.4f, "Клатраты метана", "Methane clathrates",
             "CH4·nH2O", "CH4·nH2O", 0.35f, 0.9f, surfaceTempK);
    addLayer(3.4f, 5.0f, "Водяной крио-лед", "Water cryo-ice",
             "H2O ice", "H2O ice", 0.3f, 0.92f, surfaceTempK);
  } else if (bid == "europa") {
    d.primaryMineralRu = "Радиолитический снег H2O";
    d.primaryMineralEn = "Radiolytic H2O snow";
    d.rareElementsRu = "Сульфаты магния (эпсомит), океанический лед";
    d.rareElementsEn = "Magnesium sulfates (epsomite), ocean ice";
    d.helium3Ppm = 1.5f;
    d.volatileWaterEquivalentPercent = 92.5f;
    addLayer(0.0f, 1.2f, "Радиолитический снег", "Radiolytic snow",
             "H2O", "H2O", 0.15f, 0.5f, surfaceTempK);
    addLayer(1.2f, 3.4f, "Сульфаты магния", "Magnesium sulfates",
             "MgSO4", "MgSO4", 0.4f, 1.6f, surfaceTempK);
    addLayer(3.4f, 5.0f, "Микротрещиноватый лед", "Microfractured ice",
             "H2O ice", "H2O ice", 0.25f, 0.92f, surfaceTempK);
  } else if (bid == "venus") {
    d.primaryMineralRu = "Спекшийся пирокластический базальт";
    d.primaryMineralEn = "Sintered pyroclastic basalt";
    d.rareElementsRu = "Полупроводниковый пирит, пироксеновый монолит";
    d.rareElementsEn = "Semiconducting pyrite, pyroxene monolith";
    d.helium3Ppm = 0.8f;
    d.volatileWaterEquivalentPercent = 0.1f;
    addLayer(0.0f, 1.2f, "Пирокластический базальт", "Pyroclastic basalt",
             "SiO2, FeO", "SiO2, FeO", 0.85f, 3.0f, surfaceTempK);
    addLayer(1.2f, 3.4f, "Полупроводниковый пирит", "Semiconducting pyrite",
             "FeS2", "FeS2", 0.9f, 5.0f, surfaceTempK);
    addLayer(3.4f, 5.0f, "Пироксеновый монолит", "Pyroxene monolith",
             "MgSiO3", "MgSiO3", 0.95f, 3.3f, surfaceTempK);
  } else if (bid == "hercules-b" || bid == "hercules_b") {
    d.primaryMineralRu = "Силикатный нанос";
    d.primaryMineralEn = "Silicate nanodust";
    d.rareElementsRu = "Ультраосновной перидотит, углеродно-титановая матрица";
    d.rareElementsEn = "Ultramafic peridotite, C-Ti matrix";
    d.helium3Ppm = 52.0f;
    d.volatileWaterEquivalentPercent = 1.2f;
    addLayer(0.0f, 1.2f, "Силикатный нанос", "Silicate nanodust",
             "SiO2", "SiO2", 0.3f, 1.2f, surfaceTempK);
    addLayer(1.2f, 3.4f, "Перидотит", "Peridotite",
             "Mg2SiO4", "Mg2SiO4", 0.8f, 3.3f, surfaceTempK);
    addLayer(3.4f, 5.0f, "Углеродно-титановая матрица", "C-Ti matrix",
             "C, Ti", "C, Ti", 0.98f, 4.5f, surfaceTempK);
  } else if (bid == "earth") {
    d.primaryMineralRu = "Гумус и кварцевые пески";
    d.primaryMineralEn = "Humus and quartz sands";
    d.rareElementsRu = "Карбонаты, гранодиоритовый фундамент";
    d.rareElementsEn = "Carbonates, granodiorite basement";
    d.helium3Ppm = 0.01f;
    d.volatileWaterEquivalentPercent = 12.0f;
    addLayer(0.0f, 1.2f, "Гумус и пески", "Humus and sands",
             "SiO2, organics", "SiO2, organics", 0.2f, 1.6f, surfaceTempK);
    addLayer(1.2f, 3.4f, "Карбонаты", "Carbonates",
             "CaCO3", "CaCO3", 0.5f, 2.7f, surfaceTempK);
    addLayer(3.4f, 5.0f, "Гранодиорит", "Granodiorite",
             "SiO2, feldspar", "SiO2, feldspar", 0.7f, 2.7f, surfaceTempK);
  } else {
    d.primaryMineralRu = "Хондритовые брекчии";
    d.primaryMineralEn = "Chondritic breccia";
    d.rareElementsRu = "Никель-железные включения, оливин";
    d.rareElementsEn = "Ni-Fe inclusions, olivine";
    d.helium3Ppm = 5.0f;
    d.volatileWaterEquivalentPercent = 2.0f;
    addLayer(0.0f, 1.2f, "Реголит", "Regolith",
             "Silicates", "Silicates", 0.3f, 1.8f, surfaceTempK);
    addLayer(1.2f, 3.4f, "Хондрит", "Chondrite",
             "Fe-Ni, olivine", "Fe-Ni, olivine", 0.6f, 3.3f, surfaceTempK);
    addLayer(3.4f, 5.0f, "Оливиновый монолит", "Olivine monolith",
             "Mg2SiO4", "Mg2SiO4", 0.7f, 3.3f, surfaceTempK);
  }

  d.volatileWaterPercent = d.volatileWaterEquivalentPercent;
  return d;
}

void updateCoreDrill(CoreDrillTelemetry& drill, float deltaTime, bool drillInput, bool retractInput,
                     float surfaceTempK, float surfaceGravity) {
  (void)surfaceGravity;
  (void)surfaceTempK;
  if (deltaTime <= 0.0f) return;

  if (drill.bitTempC >= 280.0f) {
    drill.state = CoreDrillState::Overheated;
    drill.rpm = 0.0f;
    drill.motorTorque = 0.0f;
    drill.penetrationRate = 0.0f;
  }

  if (drill.state == CoreDrillState::Overheated) {
    drill.rpm = 0.0f;
    drill.motorTorque = 0.0f;
    drill.penetrationRate = 0.0f;
    float coolRate = 45.0f;
    drill.bitTempC -= coolRate * deltaTime;
    if (drill.bitTempC <= 160.0f) {
      drill.state = CoreDrillState::Idle;
      if (drill.bitTempC < 20.0f) drill.bitTempC = 20.0f;
    }
    return;
  }

  if (retractInput && drill.state != CoreDrillState::Idle) {
    drill.state = CoreDrillState::Retracting;
  }

  if (drill.state == CoreDrillState::Retracting) {
    drill.rpm = 0.0f;
    drill.motorTorque = 0.0f;
    drill.penetrationRate = 0.0f;
    drill.depthM -= 1.6f * deltaTime;
    if (drill.depthM <= 0.0f) {
      drill.depthM = 0.0f;
      drill.state = CoreDrillState::Extracted;
      drill.sampleExtracted = true;
    }
    drill.bitTempC -= 20.0f * deltaTime;
    if (drill.bitTempC < 20.0f) drill.bitTempC = 20.0f;
    return;
  }

  if (drill.state == CoreDrillState::Extracted) {
    drill.rpm = 0.0f;
    drill.motorTorque = 0.0f;
    drill.penetrationRate = 0.0f;
    drill.bitTempC -= 15.0f * deltaTime;
    if (drill.bitTempC < 20.0f) drill.bitTempC = 20.0f;
    return;
  }

  if (drillInput && drill.state == CoreDrillState::Idle) {
    drill.state = CoreDrillState::Drilling;
  }

  if (drill.state == CoreDrillState::Drilling) {
    float hardness = 0.5f;
    for (const auto& L : drill.strata) {
      if (drill.depthM >= L.depthMinM && drill.depthM < L.depthMaxM) {
        hardness = L.hardness;
        break;
      }
    }
    if (drill.depthM >= drill.maxDepthM) hardness = 1.0f;

    drill.rpm = 420.0f;
    drill.motorTorque = 0.3f + hardness * 0.7f;
    drill.penetrationRate = (1.0f - hardness * 0.4f) * 1.0f;
    if (drill.penetrationRate < 0.1f) drill.penetrationRate = 0.1f;

    drill.depthM += drill.penetrationRate * deltaTime;
    if (drill.depthM > drill.maxDepthM) drill.depthM = drill.maxDepthM;

    float heatRate = drill.rpm * hardness * drill.motorTorque * 0.25f;
    drill.bitTempC += heatRate * deltaTime;
    float coolRate = 2.0f;
    drill.bitTempC -= coolRate * deltaTime;
    if (drill.bitTempC < 20.0f) drill.bitTempC = 20.0f;

    if (drill.bitTempC >= 280.0f) {
      drill.state = CoreDrillState::Overheated;
      drill.rpm = 0.0f;
      drill.motorTorque = 0.0f;
      drill.penetrationRate = 0.0f;
    }
  } else {
    drill.rpm = 0.0f;
    drill.motorTorque = 0.0f;
    drill.penetrationRate = 0.0f;
    drill.bitTempC -= 10.0f * deltaTime;
    if (drill.bitTempC < 20.0f) drill.bitTempC = 20.0f;
  }
}

std::string formatCoreSampleZettel(const std::string& bodyId, const std::string& bodyNameRu,
                                   const CoreDrillTelemetry& drill) {
  std::string s;
  s += "### Спектрохимический паспорт керна: " + bodyNameRu + " (5.0 м)\n\n";
  s += "**Идентификатор тела**: `" + bodyId + "`\n";
  s += "**Первичная минеральная фация**: " + drill.primaryMineralRu + "\n";
  s += "**Концентрация изотопа Гелий-3 (^3He)**: " + std::to_string(static_cast<int>(drill.helium3Ppm)) + " ppm\n";
  s += "**Летучие компоненты**: " + std::to_string(static_cast<int>(drill.volatileWaterPercent)) + " %\n";
  s += "**Редкие элементы / металлы**: " + drill.rareElementsRu + "\n\n";
  s += "#### Стратиграфический профиль скважины:\n";
  for (const auto& l : drill.strata) {
    char buf[128];
    std::snprintf(buf, sizeof(buf), "- **[%.1f - %.1f м] %s**: %s (Твердость: %.2f, Плотность: %.2f г/см³)\n",
                  l.depthMinM, l.depthMaxM, l.layerNameRu.c_str(), l.compositionRu.c_str(), l.hardness, l.densityGcm3);
    s += buf;
  }
  s += "\n*Образцы реголита запечатаны в герметичные контейнеры научного отсека корабля. Начислено +250 очков исследований.*";
  return s;
}

// === v5.1.0 Stage 6 Autonomous Surface Reconnaissance Drone (10 km Radius) ===
ReconDroneTelemetry createReconDroneMission(const std::string& bodyId) {
  ReconDroneTelemetry drone;
  std::string bid = bodyId;
  for (char& c : bid) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  drone.bodyId = bid;

  auto addAnom = [&](const char* id, const char* ru, const char* en,
                     const char* typeRu, const char* typeEn,
                     float xKm, float zKm, float sig, int rp) {
    // Strictly clamp every anomaly coordinate inside the 10 km reconnaissance perimeter.
    const float dist = std::sqrt(xKm * xKm + zKm * zKm);
    const float maxR = std::max(0.5f, drone.rangeRadiusKm - 0.5f);
    if (dist > maxR && dist > 1e-4f) {
      const float k = maxR / dist;
      xKm *= k;
      zKm *= k;
    }
    SurfaceAnomaly a;
    a.id = id;
    a.nameRu = ru;
    a.nameEn = en;
    a.typeRu = typeRu;
    a.typeEn = typeEn;
    a.posXKm = xKm;
    a.posZKm = zKm;
    a.distanceKm = std::sqrt(xKm * xKm + zKm * zKm);
    a.signalStrength = sig;
    a.rpReward = rp;
    drone.anomalies.push_back(std::move(a));
  };

  if (bid == "mars") {
    addAnom("mars_tube", "Провал лавовой трубки", "Lava Tube Skylight", "Геология", "Geology", 3.2f, 2.4f, 0.94f, 90);
    addAnom("mars_brine", "Выход соляного рассола", "Subsurface Brine Seep", "Гидрология", "Hydrology", -4.8f, 3.6f, 0.88f, 110);
    addAnom("mars_hem", "Россыпь гематитовых сфер", "Hematite Concretion Field", "Минералогия", "Mineralogy", 5.5f, -5.1f, 0.82f, 85);
    addAnom("mars_caldera", "Кальдера оливинового конуса", "Olivine Basalt Caldera", "Тектоника", "Tectonics", -2.9f, -7.2f, 0.91f, 105);
  } else if (bid == "titan") {
    addAnom("titan_seep", "Выход жидкого метана", "Hydrocarbon Methane Seep", "Гидрология", "Hydrology", 2.8f, 4.1f, 0.96f, 120);
    addAnom("titan_dune", "Гребень толиновой дюны", "Organic Tholin Mega-Dune", "Органика", "Organics", -5.2f, 2.2f, 0.85f, 95);
    addAnom("titan_cryo", "Криовулканический канал", "Cryovolcanic Conduit", "Криовулканизм", "Cryovolcanism", 6.1f, -4.4f, 0.92f, 115);
    addAnom("titan_hyd", "Обнажение клатрата CH4", "Methane Hydrate Outcrop", "Минералогия", "Mineralogy", -3.4f, -6.5f, 0.89f, 100);
  } else if (bid == "europa") {
    addAnom("eur_plume", "Плюм солёной воды", "Saline Chaos Plume", "Гидрология", "Hydrology", 3.6f, -2.9f, 0.98f, 130);
    addAnom("eur_lin", "Хребет сульфатных линей", "Lineae Sulphate Ridge", "Тектоника", "Tectonics", -4.3f, 4.8f, 0.90f, 105);
    addAnom("eur_lens", "Подлёдная линза рассола", "Sub-Ice Brine Lens", "Океанология", "Oceanography", 6.8f, 3.1f, 0.93f, 120);
    addAnom("eur_chaos", "Раздробленный ледяной плот", "Fractured Chaos Raft", "Гляциология", "Glaciology", -5.9f, -5.0f, 0.86f, 95);
  } else if (bid == "hercules-b" || bid == "hercules_b") {
    addAnom("hb_reef", "Бирюзовый коралловый шельф", "Turquoise Reef Shelf", "Биосфера", "Biosphere", 2.5f, 3.9f, 0.97f, 135);
    addAnom("hb_vent", "Прибрежный гидротермальный плюм", "Coastal Hydrothermal Vent", "Гидрология", "Hydrology", -4.1f, 5.2f, 0.94f, 125);
    addAnom("hb_canyon", "Базальтовый каньон кратона", "Ancient Basalt Canyon", "Геология", "Geology", 5.8f, -4.6f, 0.89f, 110);
    addAnom("hb_quartz", "Кварцевый монолит M13", "Silicate Quartz Monolith", "Минералогия", "Mineralogy", -3.7f, -6.8f, 0.92f, 115);
  } else {
    addAnom("gen_reg", "Импактный кратерный вал", "Impact Crater Rim", "Геология", "Geology", 2.9f, 2.6f, 0.90f, 80);
    addAnom("gen_mag", "Магнитная аномалия коры", "Crustal Magnetic Anomaly", "Геофизика", "Geophysics", -4.4f, 3.3f, 0.86f, 90);
    addAnom("gen_ice", "Ловушка летучих в тени", "Shadowed Volatile Pocket", "Ресурсы", "Resources", 5.0f, -4.9f, 0.84f, 85);
    addAnom("gen_fiss", "Тектонический сброс породы", "Tectonic Fault Scarp", "Тектоника", "Tectonics", -3.2f, -6.2f, 0.88f, 95);
  }
  return drone;
}

bool deployReconDrone(ReconDroneTelemetry& drone) {
  if (drone.state != ReconDroneState::Docked && drone.state != ReconDroneState::DockedComplete) {
    return false;
  }
  if (drone.batteryPct < 20.0f) {
    return false;
  }
  drone.state = ReconDroneState::Deploying;
  drone.targetIndex = -1;
  drone.scanDwellSec = 0.0f;
  drone.reportLogged = false;
  return true;
}

void recallReconDrone(ReconDroneTelemetry& drone) {
  if (drone.state == ReconDroneState::Deploying || drone.state == ReconDroneState::Scouting) {
    drone.state = ReconDroneState::Returning;
    drone.targetIndex = -1;
    drone.scanDwellSec = 0.0f;
  }
}

void updateReconDrone(ReconDroneTelemetry& drone, float deltaTime, bool isLanded, int& outRpEarned) {
  outRpEarned = 0;
  if (deltaTime <= 0.0f) return;

  if (!isLanded && (drone.state == ReconDroneState::Deploying || drone.state == ReconDroneState::Scouting)) {
    recallReconDrone(drone);
  }

  if (drone.state == ReconDroneState::Docked || drone.state == ReconDroneState::DockedComplete) {
    drone.speedKmh = 0.0f;
    drone.altitudeM = 0.0f;
    drone.posXKm = 0.0f;
    drone.posZKm = 0.0f;
    if (isLanded && drone.batteryPct < 100.0f) {
      drone.batteryPct = std::min(100.0f, drone.batteryPct + 12.0f * deltaTime);
    }
    return;
  }

  if (drone.state == ReconDroneState::Deploying) {
    drone.altitudeM += 24.0f * deltaTime;
    drone.speedKmh = 35.0f;
    drone.batteryPct = std::max(0.0f, drone.batteryPct - 0.6f * deltaTime);
    if (drone.altitudeM >= 65.0f) {
      drone.altitudeM = 65.0f;
      drone.state = ReconDroneState::Scouting;
    }
    return;
  }

  auto steerTowards = [&](float tx, float tz, float cruiseKmh) -> float {
    const float dx = tx - drone.posXKm;
    const float dz = tz - drone.posZKm;
    const float dist = std::sqrt(dx * dx + dz * dz);
    if (dist <= 1e-5f) {
      drone.speedKmh = 0.0f;
      return 0.0f;
    }
    const float stepKm = std::min(dist, (cruiseKmh / 3600.0f) * deltaTime);
    drone.posXKm += (dx / dist) * stepKm;
    drone.posZKm += (dz / dist) * stepKm;
    // Clamp drone position within declared 10 km scouting perimeter
    const float rad = std::sqrt(drone.posXKm * drone.posXKm + drone.posZKm * drone.posZKm);
    if (rad > drone.rangeRadiusKm) {
      const float k = drone.rangeRadiusKm / rad;
      drone.posXKm *= k;
      drone.posZKm *= k;
    }
    drone.distanceTraveledKm += stepKm;
    drone.speedKmh = cruiseKmh;
    drone.headingDeg = std::atan2(dx, -dz) * (180.0f / 3.14159265f);
    return std::max(0.0f, dist - stepKm);
  };

  if (drone.state == ReconDroneState::Scouting) {
    // Safety low-battery failsafe triggers autonomous return before reserve depletes
    if (drone.batteryPct <= 25.0f) {
      drone.state = ReconDroneState::Returning;
      drone.targetIndex = -1;
      drone.scanDwellSec = 0.0f;
      return;
    }

    // Select nearest unscanned anomaly within 10 km range
    int bestIdx = -1;
    float bestDist = 1e9f;
    for (int i = 0; i < static_cast<int>(drone.anomalies.size()); ++i) {
      const auto& a = drone.anomalies[i];
      if (a.scanned) continue;
      const float dx = a.posXKm - drone.posXKm;
      const float dz = a.posZKm - drone.posZKm;
      const float d = std::sqrt(dx * dx + dz * dz);
      if (d < bestDist && a.distanceKm <= drone.rangeRadiusKm) {
        bestDist = d;
        bestIdx = i;
      }
    }

    if (bestIdx < 0) {
      drone.state = ReconDroneState::Returning;
      drone.targetIndex = -1;
      drone.scanDwellSec = 0.0f;
      return;
    }

    drone.targetIndex = bestIdx;
    auto& target = drone.anomalies[bestIdx];
    const float rem = steerTowards(target.posXKm, target.posZKm, 1800.0f);
    drone.batteryPct = std::max(0.0f, drone.batteryPct - 0.9f * deltaTime);

    if (rem <= 0.08f) {
      drone.speedKmh = 15.0f;
      drone.scanDwellSec += deltaTime;
      if (drone.scanDwellSec >= 1.0f) {
        target.scanned = true;
        drone.scanDwellSec = 0.0f;
        ++drone.scannedCount;
        drone.totalRpEarned += target.rpReward;
        outRpEarned += target.rpReward;
      }
    }
    return;
  }

  if (drone.state == ReconDroneState::Returning) {
    const float rem = steerTowards(0.0f, 0.0f, 2160.0f);
    drone.batteryPct = std::max(0.0f, drone.batteryPct - 0.7f * deltaTime);
    if (rem <= 0.06f) {
      drone.posXKm = 0.0f;
      drone.posZKm = 0.0f;
      drone.speedKmh = 0.0f;
      drone.altitudeM = std::max(0.0f, drone.altitudeM - 32.0f * deltaTime);
      if (drone.altitudeM <= 0.05f) {
        drone.altitudeM = 0.0f;
        drone.state = ReconDroneState::DockedComplete;
      }
    }
  }
}

std::string formatReconDroneZettel(const std::string& bodyId, const std::string& bodyNameRu,
                                   const ReconDroneTelemetry& drone) {
  std::string s;
  s += "### Отчёт автономного дрона-разведчика (10 км): " + bodyNameRu + "\n\n";
  s += "**Идентификатор тела**: `" + bodyId + "`\n";
  s += "**Периметр разведки**: 10.0 км от посадочной площадки\n";
  s += "**Изучено аномалий**: " + std::to_string(drone.scannedCount) + " / " +
       std::to_string(drone.anomalies.size()) + "\n";
  s += "**Пройденная дистанция**: " + std::to_string(static_cast<int>(std::round(drone.distanceTraveledKm))) + " км\n";
  s += "**Начислено RP**: +" + std::to_string(drone.totalRpEarned) + "\n\n";
  s += "#### Обнаруженные объекты:\n";
  for (const auto& a : drone.anomalies) {
    char buf[160];
    std::snprintf(buf, sizeof(buf), "- **%s** (%s) — R=%.1f км [%.1f, %.1f] %s\n",
                  a.nameRu.c_str(), a.typeRu.c_str(), a.distanceKm, a.posXKm, a.posZKm,
                  a.scanned ? "[ИЗУЧЕНО]" : "[ПРОПУЩЕНО]");
    s += buf;
  }
  return s;
}

}  // namespace astro
