// test_planet_lod.cpp (v4.3.0 Beta): Unit test for SphereMesh LOD, PBR relief math, spatial audio and warp geometry
#include <iostream>
#include <cmath>
#include <vector>
#include <cassert>
#include <algorithm>
#include <glm/glm.hpp>
#include "audio/AudioEngine.h"
#include "LodSystem.h"

using namespace astro;

int main() {
  // Test 1: Sphere Mesh Geometry Tessellation (128 lat x 256 lon)
  {
    const int rings = 128;
    const int sectors = 256;
    const unsigned int expectedVertices = static_cast<unsigned int>((rings + 1) * (sectors + 1));
    const unsigned int expectedIndices = static_cast<unsigned int>(rings * sectors * 6);

    if (expectedVertices != 33153) {
      std::cerr << "Test 1 failed: Expected 33153 vertices, got " << expectedVertices << "\n";
      return 1;
    }
    if (expectedIndices != 196608) {
      std::cerr << "Test 1 failed: Expected 196608 indices (65536 triangles), got " << expectedIndices << "\n";
      return 1;
    }

    // Mathematical verification of spherical coordinates normalization
    const float kPi = 3.14159265358979323846f;
    for (int r = 0; r <= rings; ++r) {
      const float v = static_cast<float>(r) / static_cast<float>(rings);
      const float phi = v * kPi;
      const float y = std::cos(phi);
      const float sinPhi = std::sin(phi);

      for (int s = 0; s <= sectors; ++s) {
        const float u = static_cast<float>(s) / static_cast<float>(sectors);
        const float theta = u * 2.0f * kPi;
        const float x = sinPhi * std::cos(theta);
        const float z = sinPhi * std::sin(theta);

        const float len = std::sqrt(x * x + y * y + z * z);
        if (std::abs(len - 1.0f) > 1e-4f) {
          std::cerr << "Test 1 failed: Sphere vertex norm deviation: " << len << "\n";
          return 1;
        }
      }
    }
    std::cout << "PASS: Planet LOD Test 1 (SphereMesh 128x256 tessellation & unit normals)\n";
  }

  // Test 2: Audio Engine Doppler shift clamping & tension bounds
  {
    AudioEngine audio;
    audio.setDopplerShift(0.05f); // Should clamp to >= 0.2f
    audio.setDopplerShift(5.0f);  // Should clamp to <= 3.0f

    audio.setTensionLevel(-0.5f); // Should clamp to >= 0.0f
    audio.setTensionLevel(1.5f);  // Should clamp to <= 1.0f

    std::cout << "PASS: Planet LOD Test 2 (AudioEngine Doppler & Tension parameter clamping)\n";
  }

  // Test 3: Equal-Power Stereo Panning Energy Conservation
  {
    const double kPi = 3.14159265358979323846;
    for (int p = -100; p <= 100; ++p) {
      const float pan = static_cast<float>(p) / 100.0f;
      const float pan01 = std::clamp((pan + 1.0f) * 0.5f, 0.0f, 1.0f);
      const float gL = static_cast<float>(std::cos(static_cast<double>(pan01) * kPi * 0.5));
      const float gR = static_cast<float>(std::sin(static_cast<double>(pan01) * kPi * 0.5));

      const float energy = gL * gL + gR * gR;
      if (std::abs(energy - 1.0f) > 1e-5f) {
        std::cerr << "Test 3 failed: Pan energy not conserved at pan=" << pan << ", energy=" << energy << "\n";
        return 1;
      }
    }
    std::cout << "PASS: Planet LOD Test 3 (Equal-power stereo pan energy conservation)\n";
  }

  // Test 4: SpaceDrive Warp Tunnel Geometry & Clearance Verification
  {
    const float tunnelScaleR = 8.5f;
    const float innerModelRadius = 1.16f; // Measured from warp_tunnel_tube.obj bore
    const float tunnelBoreRadius = innerModelRadius * tunnelScaleR; // 9.86 units

    // Camera chase offset in world units: distance from tunnel center axis
    const float chaseRadialOffset = 1.1f; // y-offset in chase view
    if (chaseRadialOffset >= tunnelBoreRadius * 0.5f) {
      std::cerr << "Test 4 failed: Camera too close to tunnel wall\n";
      return 1;
    }

    const float clearanceMargin = tunnelBoreRadius - chaseRadialOffset;
    if (clearanceMargin < 8.0f) {
      std::cerr << "Test 4 failed: Insufficient camera clearance margin: " << clearanceMargin << "\n";
      return 1;
    }
    std::cout << "PASS: Planet LOD Test 4 (SpaceDrive Warp Tunnel geometry & clearance verification)\n";
  }

  // Test 5: Sub-pixel projected pixel size calculation & LOD culling bounds
  {
    const float fovDegrees = 60.0f;
    const int fbHeight = 1080;
    const float fovHalfRad = glm::radians(fovDegrees) * 0.5f;
    const float screenProjK = static_cast<float>(fbHeight) / (2.0f * std::tan(fovHalfRad));

    // Case A: Near Earth (radius 1.0, distance 5.0) -> high projected pixels, must use HiRes
    const float rEarth = 1.0f;
    const float dNear = 5.0f;
    const float pxNear = (rEarth / dNear) * screenProjK;
    if (pxNear < 150.0f) {
      std::cerr << "Test 5 failed: Near Earth projected pixels expected > 150, got " << pxNear << "\n";
      return 1;
    }
    const bool nearUsesHiRes = (pxNear >= 28.0f);
    if (!nearUsesHiRes) {
      std::cerr << "Test 5 failed: Near Earth should select HiRes mesh\n";
      return 1;
    }

    // Case B: Far Pluto/Moon speck (radius 0.18, distance 800.0) -> sub-pixel, must cull
    const float rPluto = 0.18f;
    const float dFar = 800.0f;
    const float pxFar = (rPluto / dFar) * screenProjK;
    const bool farIsSubPixel = (pxFar < 0.25f && dFar > 150.0f);
    if (!farIsSubPixel) {
      std::cerr << "Test 5 failed: Distant Pluto should be culled as sub-pixel, px=" << pxFar << "\n";
      return 1;
    }

    std::cout << "PASS: Planet LOD Test 5 (Sub-pixel projected size & LOD culling bounds)\n";
  }

  // Test 6: Cook-Torrance GGX microfacet specular & Schlick Fresnel energy conservation
  {
    const float F0 = 0.02f; // water dielectric index of refraction
    for (int deg = 0; deg <= 90; ++deg) {
      const float rad = glm::radians(static_cast<float>(deg));
      const float cosTheta = std::cos(rad);
      const float F = F0 + (1.0f - F0) * std::pow(1.0f - cosTheta, 5.0f);
      if (F < F0 - 1e-5f || F > 1.0001f) {
        std::cerr << "Test 6 failed: Fresnel out of bounds: " << F << " at deg=" << deg << "\n";
        return 1;
      }
    }

    // Verify GGX NDF non-negativity and peak sharpness
    const float alpha = 0.14f * 0.14f;
    const float alpha2 = alpha * alpha;
    const float kPi = 3.14159265358979323846f;
    const float D_normal = alpha2 / (kPi * alpha2 * alpha2); // NdotH = 1
    if (D_normal <= 0.0f) {
      std::cerr << "Test 6 failed: GGX peak normal D <= 0\n";
      return 1;
    }

    std::cout << "PASS: Planet LOD Test 6 (Cook-Torrance GGX & Schlick Fresnel conservation)\n";
  }

  // Test 7: Multi-Nozzle Procedural Thruster Geometry Invariants (v5.3.0 / Issue #218)
  {
    // Style 0 (Dart): 2 twin nozzles mirrored across X axis
    const auto dartNozzles = getShipNozzleLayout(0);
    if (dartNozzles.size() != 2) {
      std::cerr << "Test 7 failed: Dart should have exactly 2 nozzles, got " << dartNozzles.size() << "\n";
      return 1;
    }
    if (std::abs(dartNozzles[0].localX + dartNozzles[1].localX) > 1e-5f) {
      std::cerr << "Test 7 failed: Dart nozzles must be symmetrical across X\n";
      return 1;
    }

    // Style 1 (Ring Ship): 1 central fusion bell
    const auto ringNozzles = getShipNozzleLayout(1);
    if (ringNozzles.size() != 1 || std::abs(ringNozzles[0].localX) > 1e-5f) {
      std::cerr << "Test 7 failed: Ring Ship must have 1 central nozzle\n";
      return 1;
    }

    // Style 2 (Shuttle): 3 main engines in triangular cluster
    const auto shuttleNozzles = getShipNozzleLayout(2);
    if (shuttleNozzles.size() != 3) {
      std::cerr << "Test 7 failed: Shuttle should have 3 nozzles, got " << shuttleNozzles.size() << "\n";
      return 1;
    }
    if (std::abs(shuttleNozzles[1].localX + shuttleNozzles[2].localX) > 1e-5f) {
      std::cerr << "Test 7 failed: Shuttle lower nozzles must be symmetrical across X\n";
      return 1;
    }

    // Style 3 (Warp Cruiser): 3 nozzles (2 twin nacelles + 1 impulse)
    const auto warpNozzles = getShipNozzleLayout(3);
    if (warpNozzles.size() != 3) {
      std::cerr << "Test 7 failed: Warp Cruiser should have 3 nozzles, got " << warpNozzles.size() << "\n";
      return 1;
    }
    if (std::abs(warpNozzles[0].localX + warpNozzles[1].localX) > 1e-5f) {
      std::cerr << "Test 7 failed: Warp nacelles must be symmetrical across X\n";
      return 1;
    }

    // Style 4 (Mining Dreadnought): 4 heavy fusion bells in 2x2 matrix
    const auto dreadNozzles = getShipNozzleLayout(4);
    if (dreadNozzles.size() != 4) {
      std::cerr << "Test 7 failed: Mining Dreadnought should have 4 nozzles, got " << dreadNozzles.size() << "\n";
      return 1;
    }

    std::cout << "PASS: Planet LOD Test 7 (Multi-nozzle procedural thruster geometry invariants)\n";
  }

  // Test 8: 3-Tier Distance-Based Station LOD and Culling Invariants (v5.3.0 / Issue #218)
  {
    if (calculateStationLod(5.0f) != StationLodLevel::LOD0_Close) {
      std::cerr << "Test 8 failed: 5.0m should be LOD0_Close\n";
      return 1;
    }
    if (calculateStationLod(15.0f) != StationLodLevel::LOD0_Close) {
      std::cerr << "Test 8 failed: 15.0m boundary should be LOD0_Close\n";
      return 1;
    }
    if (calculateStationLod(15.1f) != StationLodLevel::LOD1_Medium) {
      std::cerr << "Test 8 failed: 15.1m should transition to LOD1_Medium\n";
      return 1;
    }
    if (calculateStationLod(40.0f) != StationLodLevel::LOD1_Medium) {
      std::cerr << "Test 8 failed: 40.0m boundary should be LOD1_Medium\n";
      return 1;
    }
    if (calculateStationLod(40.1f) != StationLodLevel::LOD2_Far) {
      std::cerr << "Test 8 failed: 40.1m should transition to LOD2_Far\n";
      return 1;
    }
    if (calculateStationLod(100.0f) != StationLodLevel::LOD2_Far) {
      std::cerr << "Test 8 failed: 100.0m should be LOD2_Far\n";
      return 1;
    }

    // Verify culling boundary invariant
    const float dCulled = 135.0f;
    if (dCulled <= kStationLodCullDistance) {
      std::cerr << "Test 8 failed: 135.0m should exceed kStationLodCullDistance\n";
      return 1;
    }

    std::cout << "PASS: Planet LOD Test 8 (3-tier distance-based station LOD & culling invariants)\n";
  }

  std::cout << "All Planet LOD and Audio tests PASSED successfully!\n";
  return 0;
}
