#pragma once

#include <vector>
#include <glm/glm.hpp>

namespace astro {

// 3-Tier Distance-Based Level of Detail for Stations and Megastructures (v5.3.0 / Issue #218)
enum class StationLodLevel {
  LOD0_Close = 0,    // <= 15.0f units: Full PBR geometry, subcomponent articulation, full VFX
  LOD1_Medium = 1,   // 15.0f..40.0f units: Consolidated mesh, streamlined lighting
  LOD2_Far = 2       // > 40.0f units: Compact silhouette, cull if > 120.0f units
};

constexpr float kStationLod0Threshold = 15.0f;
constexpr float kStationLod1Threshold = 40.0f;
constexpr float kStationLodCullDistance = 120.0f;

inline StationLodLevel calculateStationLod(float distance) {
  if (distance <= kStationLod0Threshold) return StationLodLevel::LOD0_Close;
  if (distance <= kStationLod1Threshold) return StationLodLevel::LOD1_Medium;
  return StationLodLevel::LOD2_Far;
}

// Procedural Multi-Nozzle Thruster Layout & Standing Mach Diamonds (v5.3.0 / Issue #218)
struct ThrusterNozzle {
  float localX;
  float localY;
  float localZ;
  float scale;
};

inline std::vector<ThrusterNozzle> getShipNozzleLayout(int style) {
  switch (style) {
    case 0:  // Dart: 2 high-performance twin nozzles
      return {
        { -0.16f, 0.0f, 0.95f, 0.85f },
        {  0.16f, 0.0f, 0.95f, 0.85f }
      };
    case 1:  // Ring Ship: 1 central heavy fusion ring cluster
      return {
        { 0.0f, 0.0f, 1.15f, 1.25f }
      };
    case 2:  // Shuttle: 3 main engines in triangular cluster (Space Shuttle SSME layout)
      return {
        {  0.0f,  0.08f, 1.10f, 0.90f },
        { -0.20f, -0.05f, 1.10f, 0.80f },
        {  0.20f, -0.05f, 1.10f, 0.80f }
      };
    case 3:  // Warp Cruiser: 2 warp nacelle exhausts + 1 central impulse engine
      return {
        { -0.60f,  0.22f, 1.40f, 0.95f },
        {  0.60f,  0.22f, 1.40f, 0.95f },
        {  0.0f,  -0.05f, 1.05f, 0.75f }
      };
    case 4:  // Mining Dreadnought: 4 heavy fusion bells in 2x2 matrix
      return {
        { -0.28f,  0.14f, 1.18f, 0.85f },
        {  0.28f,  0.14f, 1.18f, 0.85f },
        { -0.28f, -0.14f, 1.18f, 0.85f },
        {  0.28f, -0.14f, 1.18f, 0.85f }
      };
    default:
      return {
        { 0.0f, 0.0f, 0.75f, 1.0f }
      };
  }
}

}  // namespace astro
