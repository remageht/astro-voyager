#include <cassert>
#include <cmath>
#include <iostream>

#include "LoadingScreen.h"

int main() {
  std::cout << "[test_loading] Starting LoadingScreen test suite..." << std::endl;

  astro::LoadingScreen ls;

  // 1. Initial State
  assert(!ls.isActive());
  assert(ls.phase() == astro::LoadingPhase::None);
  assert(std::fabs(ls.progress() - 0.0f) < 1e-4f);
  assert(!ls.justFinished());
  assert(!ls.quote().empty());
  assert(!ls.quoteAuthor().empty());
  std::cout << "[test_loading] Initial state passed: Quote='" << ls.quote()
            << "' Author='" << ls.quoteAuthor() << "'" << std::endl;

  // 2. Cold Boot Lifecycle
  ls.startColdBoot();
  assert(ls.isActive());
  assert(ls.phase() == astro::LoadingPhase::ColdBoot);
  assert(std::fabs(ls.progress() - 0.0f) < 1e-4f);
  assert(!ls.justFinished());

  // Half duration update (2.2s duration -> 1.1s is 50%)
  ls.update(1.1f);
  assert(ls.isActive());
  assert(std::fabs(ls.progress() - 0.5f) < 0.05f);

  // Complete cold boot
  ls.update(1.2f);
  assert(!ls.isActive());
  assert(ls.phase() == astro::LoadingPhase::None);
  assert(std::fabs(ls.progress() - 1.0f) < 1e-4f);
  assert(ls.justFinished());

  // Clear finished flag
  ls.clearFinishedFlag();
  assert(!ls.justFinished());
  std::cout << "[test_loading] Cold boot lifecycle passed." << std::endl;

  // 3. New Game Warp & Skip functionality
  ls.startNewGameWarp("EXPLORE KERR L1 HORIZON");
  assert(ls.isActive());
  assert(ls.phase() == astro::LoadingPhase::NewGameWarp);
  assert(std::fabs(ls.progress() - 0.0f) < 1e-4f);

  ls.update(0.5f);
  assert(ls.isActive());
  assert(ls.progress() > 0.1f && ls.progress() < 0.5f);

  // Instant skip
  ls.skip();
  assert(!ls.isActive());
  assert(ls.phase() == astro::LoadingPhase::None);
  assert(std::fabs(ls.progress() - 1.0f) < 1e-4f);
  assert(ls.justFinished());
  ls.clearFinishedFlag();
  std::cout << "[test_loading] New game warp and skip passed." << std::endl;

  // 4. Continue Warp Lifecycle
  ls.startContinueWarp("STATION FREEDOM-7");
  assert(ls.isActive());
  assert(ls.phase() == astro::LoadingPhase::ContinueWarp);
  assert(!ls.justFinished());

  // Update in increments
  ls.update(0.8f);
  assert(ls.isActive());
  assert(std::fabs(ls.progress() - 0.5f) < 0.05f);

  ls.update(0.9f);
  assert(!ls.isActive());
  assert(ls.phase() == astro::LoadingPhase::None);
  assert(ls.justFinished());
  std::cout << "[test_loading] Continue warp lifecycle passed." << std::endl;

  // 5. Test no-op draw without crash
  ls.startColdBoot();
  ls.draw(1920.0f, 1080.0f);
  assert(ls.isActive());
  ls.skip();
  assert(!ls.isActive());

  std::cout << "[test_loading] All LoadingScreen tests passed successfully!" << std::endl;
  return 0;
}
