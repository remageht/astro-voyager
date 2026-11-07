#include "LandingSystem.h"

#include <algorithm>
#include <cmath>

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
  CoreDrillTelemetry d;
  d.strata.clear();

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
    d.primaryMineralRu = "Нанореголит Fe2O3";
    d.primaryMineralEn = "Nanophase Fe2O3 regolith";
    d.rareElementsRu = "Оливиновые базальты, вечномерзлотный лед";
    d.rareElementsEn = "Olivine basalts, permafrost ice";
    d.helium3Ppm = 14.2f;
    d.volatileWaterEquivalentPercent = 3.5f;
    addLayer(0.0f, 1.2f, "Нанореголит", "Nanoregolith",
             "Fe2O3, пыль", "Fe2O3 dust", 0.25f, 1.4f, surfaceTempK);
    addLayer(1.2f, 3.4f, "Оливиновый базальт", "Olivine basalt",
             "SiO2, Fe, Mg", "SiO2, Fe, Mg", 0.6f, 2.9f, surfaceTempK);
    addLayer(3.4f, 5.0f, "Вечномерзлотный лед", "Permafrost ice",
             "H2O ice", "H2O ice", 0.4f, 0.92f, surfaceTempK);
  } else if (bid == "moon" || bid == "luna") {
    d.primaryMineralRu = "Абразивный микрореголит";
    d.primaryMineralEn = "Abrasive micrometeorite regolith";
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
    d.primaryMineralRu = "Фотохимические толины";
    d.primaryMineralEn = "Photochemical tholins";
    d.rareElementsRu = "Клатратные гидраты метана, водяной крио-лед";
    d.rareElementsEn = "Methane clathrates, water cryo-ice";
    d.helium3Ppm = 2.1f;
    d.volatileWaterEquivalentPercent = 42.0f;
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
             "C, Ti", "C, Ti", 0.95f, 4.5f, surfaceTempK);
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

  return d;
}

void updateCoreDrill(CoreDrillTelemetry& drill, float deltaTime, bool drillInput, bool retractInput,
                     float surfaceTempK, float surfaceGravity) {
  if (deltaTime <= 0.0f) return;

  if (drill.state == CoreDrillState::Overheated) {
    drill.rpm = 0.0f;
    drill.motorTorque = 0.0f;
    drill.penetrationRate = 0.0f;
    float coolRate = 30.0f;
    drill.bitTempC -= coolRate * deltaTime;
    if (drill.bitTempC < 180.0f) {
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
    drill.depthM -= 0.5f * deltaTime;
    if (drill.depthM <= 0.0f) {
      drill.depthM = 0.0f;
      drill.state = CoreDrillState::Extracted;
      drill.sampleExtracted = true;
    }
    drill.bitTempC -= 15.0f * deltaTime;
    if (drill.bitTempC < 20.0f) drill.bitTempC = 20.0f;
    return;
  }

  if (drill.state == CoreDrillState::Extracted) {
    drill.rpm = 0.0f;
    drill.motorTorque = 0.0f;
    drill.penetrationRate = 0.0f;
    drill.bitTempC -= 10.0f * deltaTime;
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
    drill.penetrationRate = (1.0f - hardness * 0.8f) * 0.5f;
    if (drill.penetrationRate < 0.01f) drill.penetrationRate = 0.01f;

    drill.depthM += drill.penetrationRate * deltaTime;
    if (drill.depthM > drill.maxDepthM) drill.depthM = drill.maxDepthM;

    float heatRate = drill.rpm * hardness * drill.motorTorque * 0.0008f;
    drill.bitTempC += heatRate * deltaTime;
    float coolRate = 5.0f + (surfaceTempK - 273.15f) * 0.02f;
    if (coolRate < 2.0f) coolRate = 2.0f;
    drill.bitTempC -= coolRate * deltaTime;
    if (drill.bitTempC < 20.0f) drill.bitTempC = 20.0f;

    if (drill.bitTempC > 280.0f) {
      drill.state = CoreDrillState::Overheated;
      drill.bitTempC = 280.0f;
    }
  } else {
    drill.rpm = 0.0f;
    drill.motorTorque = 0.0f;
    drill.penetrationRate = 0.0f;
    drill.bitTempC -= 10.0f * deltaTime;
    if (drill.bitTempC < 20.0f) drill.bitTempC = 20.0f;
  }

  (void)surfaceGravity;
}

std::string formatCoreSampleZettel(const std::string& bodyId, const std::string& bodyNameRu,
                                   const CoreDrillTelemetry& drill) {
  std::string s;
  s += "Керн: " + bodyNameRu + " (" + bodyId + ")\n";
  s += "Глубина: " + std::to_string(drill.depthM) + " м\n";
  s += "Минерал: " + drill.primaryMineralRu + "\n";
  s += "He-3: " + std::to_string(drill.helium3Ppm) + " ppm\n";
  s += "Летучие: " + std::to_string(drill.volatileWaterEquivalentPercent) + " %\n";
  s += "Страты: " + std::to_string(drill.strata.size()) + "\n";
  return s;
}

}  // namespace astro
