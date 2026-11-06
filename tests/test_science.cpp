// test_science.cpp (v4.6.0): Science Spectrometer, Codex, and Bridge Comms
#include "ScienceSystem.h"

#include <cmath>
#include <iostream>
#include <string>

using astro::analyzeSpectrum;
using astro::BridgeComms;
using astro::RadioMessage;
using astro::ScienceCodex;
using astro::SpectralAnalysis;

int main() {
  // Test 1: Sol & Earth spectrum with Fraunhofer lines and biosignatures
  {
    SpectralAnalysis sol = analyzeSpectrum("sol", 0.0);
    if (sol.stationId != "sol") {
      std::cerr << "Test 1 failed: expected stationId sol, got " << sol.stationId << '\n';
      return 1;
    }
    if (std::abs(sol.surfaceTempK - 5778.0f) > 1e-2f) {
      std::cerr << "Test 1 failed: unexpected surface temperature for Sol: " << sol.surfaceTempK << '\n';
      return 1;
    }
    if (!sol.hasBiosignature) {
      std::cerr << "Test 1 failed: Sol/Earth must have biosignature flag true\n";
      return 1;
    }
    bool foundHAlpha = false;
    bool foundNaD = false;
    bool foundCH4 = false;
    for (const auto& line : sol.lines) {
      if (std::abs(line.wavelengthNm - 656.3f) < 0.1f) foundHAlpha = true;
      if (std::abs(line.wavelengthNm - 589.0f) < 0.1f) foundNaD = true;
      if (std::abs(line.wavelengthNm - 727.0f) < 0.1f) foundCH4 = true;
    }
    if (!foundHAlpha || !foundNaD || !foundCH4) {
      std::cerr << "Test 1 failed: missing key spectral lines in Sol system\n";
      return 1;
    }
  }

  // Test 2: Relativistic Doppler effect (blue-shift and red-shift)
  {
    const double beta = 0.5;  // 0.5c
    SpectralAnalysis shifted = analyzeSpectrum("sol", beta);
    const double expectedFactor = std::sqrt((1.0 + beta) / (1.0 - beta));  // sqrt(3) ~ 1.73205
    const float baseHAlpha = 656.3f;
    const float expectedShifted = static_cast<float>(baseHAlpha * expectedFactor);

    bool foundShifted = false;
    for (const auto& line : shifted.lines) {
      if (line.element.find("H-alpha") != std::string::npos) {
        if (std::abs(line.wavelengthNm - expectedShifted) < 0.5f) {
          foundShifted = true;
          break;
        }
      }
    }
    if (!foundShifted) {
      std::cerr << "Test 2 failed: relativistic Doppler shift mismatch for beta = 0.5\n";
      return 1;
    }
  }

  // Test 3: Kilonova GW170817 heavy elements (gold Au-I and platinum Pt-I)
  {
    SpectralAnalysis kn = analyzeSpectrum("gw170817", 0.0);
    bool foundAu = false;
    bool foundPt = false;
    for (const auto& line : kn.lines) {
      if (std::abs(line.wavelengthNm - 267.6f) < 0.2f && line.element.find("Au-I") != std::string::npos) {
        foundAu = true;
      }
      if (std::abs(line.wavelengthNm - 306.8f) < 0.2f && line.element.find("Pt-I") != std::string::npos) {
        foundPt = true;
      }
    }
    if (!foundAu || !foundPt) {
      std::cerr << "Test 3 failed: Kilonova GW170817 missing Au-I or Pt-I spectral lines\n";
      return 1;
    }
  }

  // Test 4: Sgr A* and OJ 287 spectral classifications
  {
    SpectralAnalysis sgra = analyzeSpectrum("sgra", 0.0);
    if (sgra.magneticFieldTesla <= 0.0f || sgra.surfaceTempK <= 1e6f) {
      std::cerr << "Test 4 failed: Sgr A* metrics invalid\n";
      return 1;
    }
    SpectralAnalysis oj = analyzeSpectrum("oj287", 0.0);
    if (oj.chemicalAbundance.empty() || oj.lines.empty()) {
      std::cerr << "Test 4 failed: OJ 287 missing spectral data\n";
      return 1;
    }
  }

  // Test 5: Science Codex scanning and progression (+100 RP per discovery)
  {
    ScienceCodex codex;
    if (codex.getTotalCount() != 23) {
      std::cerr << "Test 5 failed: expected 23 catalog entries, got " << codex.getTotalCount() << '\n';
      return 1;
    }
    if (codex.getScannedCount() != 0 || codex.getTotalResearchPoints() != 0) {
      std::cerr << "Test 5 failed: fresh codex must have 0 scanned objects and 0 RP\n";
      return 1;
    }

    bool scan1 = codex.scanObject("sgra", 100.0);
    if (!scan1 || !codex.isScanned("sgra")) {
      std::cerr << "Test 5 failed: scanObject(sgra) should return true and mark as scanned\n";
      return 1;
    }
    if (codex.getScannedCount() != 1 || codex.getTotalResearchPoints() != 100) {
      std::cerr << "Test 5 failed: expected 1 scanned and 100 RP after first scan\n";
      return 1;
    }

    // Duplicate scan must be idempotent
    bool scanDup = codex.scanObject("sgra", 150.0);
    if (scanDup || codex.getScannedCount() != 1 || codex.getTotalResearchPoints() != 100) {
      std::cerr << "Test 5 failed: duplicate scan must return false and not add RP\n";
      return 1;
    }

    // Unknown station must return false
    bool scanUnknown = codex.scanObject("unknown_void", 200.0);
    if (scanUnknown) {
      std::cerr << "Test 5 failed: scanning unknown object must return false\n";
      return 1;
    }
  }

  // Test 6: Science Codex serialization and deserialization
  {
    ScienceCodex codex;
    codex.scanObject("sol", 10.0);
    codex.scanObject("m16", 25.0);
    codex.scanObject("gw170817", 40.0);

    const std::string serialized = codex.serialize();
    ScienceCodex restored;
    if (!restored.deserialize(serialized)) {
      std::cerr << "Test 6 failed: deserialize returned false\n";
      return 1;
    }

    if (restored.getTotalResearchPoints() != 300) {
      std::cerr << "Test 6 failed: restored points mismatch: " << restored.getTotalResearchPoints() << '\n';
      return 1;
    }
    if (!restored.isScanned("sol") || !restored.isScanned("m16") || !restored.isScanned("gw170817")) {
      std::cerr << "Test 6 failed: restored scanned entries missing\n";
      return 1;
    }
    if (restored.isScanned("sgra")) {
      std::cerr << "Test 6 failed: unscanned entry falsely marked as scanned\n";
      return 1;
    }
  }

  // Test 7: Bridge Comms message queue, priority, duration, and deduplication
  {
    BridgeComms comms;
    RadioMessage msg1;
    msg1.id = "test_routine";
    msg1.callsignRu = "Пилот Миа Сато";
    msg1.callsignEn = "Pilot Mia Sato";
    msg1.textRu = "Все системы в норме.";
    msg1.textEn = "All systems nominal.";
    msg1.priority = 1.0f;
    msg1.durationSec = 5.0;

    comms.queueMessage(msg1, 10.0);
    if (!comms.hasActiveMessage()) {
      std::cerr << "Test 7 failed: expected active message after queueMessage\n";
      return 1;
    }
    if (!comms.hasTriggered("test_routine")) {
      std::cerr << "Test 7 failed: expected hasTriggered to be true\n";
      return 1;
    }

    // Lower priority message should be ignored while msg1 is active
    RadioMessage msgLow;
    msgLow.id = "test_low";
    msgLow.priority = 0.5f;
    msgLow.durationSec = 5.0;
    comms.queueMessage(msgLow, 12.0);
    if (comms.getActiveMessage()->id != "test_routine") {
      std::cerr << "Test 7 failed: lower priority message preempted higher priority\n";
      return 1;
    }

    // Higher priority message should preempt active message
    RadioMessage msgUrgent;
    msgUrgent.id = "test_urgent";
    msgUrgent.priority = 2.5f;
    msgUrgent.durationSec = 4.0;
    comms.queueMessage(msgUrgent, 13.0);
    if (comms.getActiveMessage()->id != "test_urgent") {
      std::cerr << "Test 7 failed: urgent message failed to preempt\n";
      return 1;
    }

    // Message progress calculation
    float prog = comms.getMessageProgress(15.0);  // 15 - 13 = 2s out of 4s -> 0.5
    if (std::abs(prog - 0.5f) > 1e-2f) {
      std::cerr << "Test 7 failed: unexpected message progress: " << prog << '\n';
      return 1;
    }

    // Message expiration
    comms.update(17.5);  // 17.5 - 13.0 = 4.5s > 4.0s -> expired
    if (comms.hasActiveMessage()) {
      std::cerr << "Test 7 failed: active message should have expired\n";
      return 1;
    }
  }

  // Test 8: Voyager 1 interstellar probe spectrum and Golden Record codex entry
  {
    SpectralAnalysis voyager = analyzeSpectrum("voyager1", 0.0);
    if (voyager.stationId != "voyager1") {
      std::cerr << "Test 8 failed: expected stationId voyager1\n";
      return 1;
    }
    if (!voyager.hasBiosignature) {
      std::cerr << "Test 8 failed: Voyager 1 must have biosignature true (Earth voices)\n";
      return 1;
    }
    bool foundH1 = false;
    bool foundAu = false;
    for (const auto& line : voyager.lines) {
      if (std::abs(line.wavelengthNm - 1420.4f) < 0.2f) foundH1 = true;
      if (std::abs(line.wavelengthNm - 267.6f) < 0.2f) foundAu = true;
    }
    if (!foundH1 || !foundAu) {
      std::cerr << "Test 8 failed: Voyager 1 missing 21cm H-I or Au-I spectral lines\n";
      return 1;
    }

    ScienceCodex codex;
    const auto* entry = codex.getEntry("voyager1");
    if (!entry) {
      std::cerr << "Test 8 failed: missing voyager1 entry in ScienceCodex\n";
      return 1;
    }
    if (entry->categoryEn != "Cosmic Relic") {
      std::cerr << "Test 8 failed: unexpected category: " << entry->categoryEn << '\n';
      return 1;
    }
  }

  std::cout << "All 8 ScienceSystem tests passed successfully!\n";
  return 0;
}
