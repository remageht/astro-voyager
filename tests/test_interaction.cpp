#include "../src/Catalog.h"  // v3.6.0: distance/ETA helpers
#include "../src/Interaction.h"
#include "../src/Types.h"  // v3.3.0: ShipState / astro::ship balance logic

#include <cmath>
#include <iostream>
#include <cstdlib>
#include <string>

namespace {

void AssertNear(float actual, float expected, float tolerance, const char* label) {
  if (std::abs(actual - expected) > tolerance) {
    std::cerr << label << " expected " << expected << " but got " << actual << '\n';
    std::exit(1);
  }
}

}  // namespace

int main() {
  // Test 1: mouse capture when UI does not need mouse
  if (!blackhole::ShouldCaptureMouseForCamera(true, false)) {
    std::cerr << "left mouse button should enable camera look when UI does not need mouse\n";
    return 1;
  }

  // Test 2: mouse capture off when button is released
  if (blackhole::ShouldCaptureMouseForCamera(false, false)) {
    std::cerr << "camera look should be off while left mouse button is released\n";
    return 1;
  }

  // Test 3: mouse capture off when UI wants mouse
  if (blackhole::ShouldCaptureMouseForCamera(true, true)) {
    std::cerr << "camera look should stay off while UI needs mouse input\n";
    return 1;
  }

  // Test 4: keyboard movement when UI does not need keyboard
  if (!blackhole::ShouldProcessKeyboardMovement(false)) {
    std::cerr << "keyboard movement should work when UI does not need keyboard\n";
    return 1;
  }

  // Test 5: keyboard movement blocked when UI needs keyboard
  if (blackhole::ShouldProcessKeyboardMovement(true)) {
    std::cerr << "keyboard movement should pause while UI needs keyboard input\n";
    return 1;
  }

  // Test 6: clamp radius
  AssertNear(blackhole::ClampCameraRadius(0.4f, 1.05f), 1.05f, 0.0001f, "radius below horizon");
  AssertNear(blackhole::ClampCameraRadius(8.0f, 1.05f), 8.0f, 0.0001f, "radius outside horizon");
  AssertNear(blackhole::ClampCameraRadius(31.0f, 1.05f), 31.0f, 0.0001f, "radius beyond shell");

  // Test 7 (v3.3.0): warp jump costs 5 fuel and is blocked under 5.
  astro::ShipState jumpShip;  // 100/100 defaults
  AssertNear(jumpShip.fuel, 100.0f, 0.0001f, "fresh ship fuel");
  AssertNear(jumpShip.shield, 100.0f, 0.0001f, "fresh ship shield");
  if (!astro::ship::canJump(jumpShip)) {
    std::cerr << "jump should be allowed at full fuel\n";
    return 1;
  }
  if (!astro::ship::spendJumpFuel(jumpShip)) {
    std::cerr << "jump fuel spend should succeed at full fuel\n";
    return 1;
  }
  AssertNear(jumpShip.fuel, 95.0f, 0.0001f, "fuel after one jump");
  jumpShip.fuel = 4.0f;
  if (astro::ship::canJump(jumpShip)) {
    std::cerr << "jump must be blocked below 5 fuel\n";
    return 1;
  }
  if (astro::ship::spendJumpFuel(jumpShip)) {
    std::cerr << "blocked jump must not spend fuel\n";
    return 1;
  }
  AssertNear(jumpShip.fuel, 4.0f, 0.0001f, "fuel after blocked jump");

  // Test 8 (v3.3.0): pulsar radiation drains 10/s, star proximity refuels 2/s.
  astro::ShipState zoneShip;
  astro::ship::applyRadiation(zoneShip, 2.5f, true);
  AssertNear(zoneShip.shield, 75.0f, 0.0001f, "shield after 2.5s in pulsar zone");
  astro::ship::applyRadiation(zoneShip, 1.0f, false);
  AssertNear(zoneShip.shield, 75.0f, 0.0001f, "shield unchanged outside zone");
  zoneShip.fuel = 50.0f;
  astro::ship::updateFuel(zoneShip, 3.0f, true);
  AssertNear(zoneShip.fuel, 56.0f, 0.0001f, "fuel regenerated near a star");
  astro::ship::updateFuel(zoneShip, 3.0f, false);
  AssertNear(zoneShip.fuel, 56.0f, 0.0001f, "no regen away from a star");
  astro::ship::updateFuel(zoneShip, 100.0f, true);
  AssertNear(zoneShip.fuel, 100.0f, 0.0001f, "fuel capped at 100");
  zoneShip.shield = 2.0f;
  astro::ship::applyRadiation(zoneShip, 5.0f, true);
  AssertNear(zoneShip.shield, 0.0f, 0.0001f, "shield floors at zero");

  // Test 8b (v3.6.0): сложность масштабирует цену прыжка, заправку и радиацию.
  const astro::DifficultyTuning easy = astro::difficultyTuning(0);
  const astro::DifficultyTuning normal = astro::difficultyTuning(1);
  const astro::DifficultyTuning hard = astro::difficultyTuning(2);
  AssertNear(easy.jumpCostK, 0.6f, 0.0001f, "easy jump cost");
  AssertNear(normal.jumpCostK, 1.0f, 0.0001f, "normal jump cost");
  AssertNear(hard.jumpCostK, 1.6f, 0.0001f, "hard jump cost");
  if (!easy.hints || !normal.hints || hard.hints) {
    std::cerr << "difficulty hints flags wrong\n";
    return 1;
  }
  astro::ShipState hardShip;
  hardShip.fuel = 6.0f;  // Normal бы пустил (5), Hard требует 8
  if (astro::ship::canJump(hardShip) ==
      astro::ship::canJump(hardShip, hard.jumpCostK)) {
    std::cerr << "hard canJump must differ from normal at 6 fuel\n";
    return 1;
  }
  if (astro::ship::canJump(hardShip, hard.jumpCostK)) {
    std::cerr << "6 fuel must not cover the hard jump (8)\n";
    return 1;
  }
  astro::ShipState easyShip;
  easyShip.fuel = 4.0f;  // Easy стоит 3
  if (!astro::ship::spendJumpFuel(easyShip, easy.jumpCostK)) {
    std::cerr << "easy spend at 4 fuel must succeed\n";
    return 1;
  }
  AssertNear(easyShip.fuel, 1.0f, 0.0001f, "easy fuel after jump");
  astro::ShipState radShip;
  astro::ship::applyRadiation(radShip, 2.0f, true, hard.radiationK);
  AssertNear(radShip.shield, 70.0f, 0.0001f, "hard radiation x1.5");
  astro::ShipState refuelShip;
  refuelShip.fuel = 50.0f;
  astro::ship::updateFuel(refuelShip, 2.0f, true, easy.starFuelK);
  AssertNear(refuelShip.fuel, 56.4f, 0.0001f, "easy star refuel x1.6");

  // Test 8c (v3.6.0): варп-роли корпусов и дистанция/ETA в варп-панели.
  if (!astro::ship::styleCanWarp(0) || !astro::ship::styleCanWarp(1)) {
    std::cerr << "Dart and Ring Ship must be warp-capable\n";
    return 1;
  }
  if (astro::ship::styleCanWarp(2)) {
    std::cerr << "Shuttle must not be warp-capable\n";
    return 1;
  }
  // Классическая шкала v = f^3 c: 26000 ly на Warp 9 => ~35.7 лет.
  AssertNear(static_cast<float>(astro::warpEtaYears(26000.0, 9)), 35.6653f,
             0.001f, "eta sgra at warp 9");
  AssertNear(static_cast<float>(astro::warpEtaYears(0.0, 9)), 0.0f, 0.0001f,
             "eta zero distance");
  AssertNear(static_cast<float>(astro::warpEtaYears(1344.0, 7)), 3.9184f,
             0.001f, "eta orion at warp 7");
  if (astro::formatDistanceLy(2.54e6) != "2.54 Mly") {
    std::cerr << "formatDistanceLy Mly wrong\n";
    return 1;
  }
  if (astro::formatDistanceLy(26000.0) != "26.0 kly" ||
      astro::formatDistanceLy(0.0) != "local") {
    std::cerr << "formatDistanceLy kly/local wrong\n";
    return 1;
  }
  if (astro::formatEtaYears(35.6653) != "35.7 y" ||
      astro::formatEtaYears(0.5) != "6.0 mo" ||
      astro::formatEtaYears(0.0) != "-") {
    std::cerr << "formatEtaYears wrong\n";
    return 1;
  }

  // Test 9 (v3.3.0): death triggers the respawn balance (50 shield / 30 fuel).
  astro::ShipState deadShip;
  deadShip.shield = 0.0f;
  if (!astro::ship::isDead(deadShip)) {
    std::cerr << "zero shield must count as death\n";
    return 1;
  }
  deadShip.gravVel = astro::Vec3(1.0, 2.0, 3.0);
  astro::ship::respawn(deadShip);
  AssertNear(deadShip.shield, 50.0f, 0.0001f, "respawn shield");
  AssertNear(deadShip.fuel, 30.0f, 0.0001f, "respawn fuel");
  AssertNear(static_cast<float>(deadShip.gravVel.x), 0.0f, 0.0001f,
             "respawn drift reset");
  if (astro::ship::isDead(deadShip)) {
    std::cerr << "respawned ship must be alive\n";
    return 1;
  }

  // Test 10 (v3.3.0): low-shield postfx gates and Newtonian falloff.
  AssertNear(astro::ship::chromaK(100.0f), 0.0f, 0.0001f, "no chroma at full shield");
  AssertNear(astro::ship::chromaK(30.0f), 0.0f, 0.0001f, "chroma gate at 30");
  AssertNear(astro::ship::chromaK(15.0f), 0.5f, 0.0001f, "chroma half at 15");
  AssertNear(astro::ship::staticK(10.0f), 0.0f, 0.0001f, "static gate at 10");
  AssertNear(astro::ship::staticK(5.0f), 0.5f, 0.0001f, "static half at 5");
  AssertNear(static_cast<float>(astro::ship::gravAccel(40.0, 2.0)), 10.0f,
             0.0001f, "GM/r^2 at 2 units");
  AssertNear(static_cast<float>(astro::ship::gravAccel(40.0, 0.1)), 160.0f,
             0.0001f, "GM/r^2 clamps singular distance");

  // Test 11 (v3.3.0): V-цикл видов камеры 0 -> 1 -> 2 -> 0.
  astro::view::ViewMode vm = astro::view::ViewMode::Cockpit;
  vm = astro::view::nextView(vm);
  if (vm != astro::view::ViewMode::Chase) {
    std::cerr << "first V toggle must switch to Chase\n";
    return 1;
  }
  vm = astro::view::nextView(vm);
  if (vm != astro::view::ViewMode::Overview) {
    std::cerr << "second V toggle must switch to Overview\n";
    return 1;
  }
  vm = astro::view::nextView(vm);
  if (vm != astro::view::ViewMode::Cockpit) {
    std::cerr << "third V toggle must wrap back to Cockpit\n";
    return 1;
  }

  // Test 12 (v3.3.0): смещение камеры кадра для видов (Cockpit = identity).
  const astro::Vec3 camPos(1.0, 2.0, 3.0);
  const astro::Vec3 camFwd(0.0, 0.0, 1.0);
  const astro::Vec3 cockpit = astro::view::offsetCameraPos(
      camPos, camFwd, astro::view::ViewMode::Cockpit);
  AssertNear(static_cast<float>(cockpit.x), 1.0f, 0.0001f, "cockpit x");
  AssertNear(static_cast<float>(cockpit.y), 2.0f, 0.0001f, "cockpit y");
  AssertNear(static_cast<float>(cockpit.z), 3.0f, 0.0001f, "cockpit z");
  const astro::Vec3 chase = astro::view::offsetCameraPos(
      camPos, camFwd, astro::view::ViewMode::Chase);
  AssertNear(static_cast<float>(chase.y), 3.1f, 0.0001f, "chase up offset");
  AssertNear(static_cast<float>(chase.z), -1.5f, 0.0001f, "chase back offset");
  const astro::Vec3 far = astro::view::offsetCameraPos(
      camPos, camFwd, astro::view::ViewMode::Overview);
  AssertNear(static_cast<float>(far.y), 4.2f, 0.0001f, "overview up offset");
  AssertNear(static_cast<float>(far.z), -6.0f, 0.0001f, "overview back offset");

  // Test 13 (v3.3.0): масштаб эффекта скорости (кламп 0..1, защита ref).
  AssertNear(astro::view::speedFx(6.0f, 12.0f), 0.5f, 0.0001f, "half ref speed");
  AssertNear(astro::view::speedFx(24.0f, 12.0f), 1.0f, 0.0001f, "speed clamp");
  AssertNear(astro::view::speedFx(0.0f, 12.0f), 0.0f, 0.0001f, "zero speed");
  AssertNear(astro::view::speedFx(5.0f, 0.0f), 0.0f, 0.0001f, "ref guard");

  // Test 14 (v3.3.0): вспышка щита — импульс от урона и затухание.
  AssertNear(astro::ship::shieldFlashHit(0.0f, 1.2f), 0.42f, 0.0001f,
             "flash impulse scales with damage");
  AssertNear(astro::ship::shieldFlashHit(0.9f, 1.0f), 1.0f, 0.0001f,
             "flash impulse clamps at one");
  AssertNear(astro::ship::shieldFlashDecay(1.0f, 0.0f), 1.0f, 0.0001f,
             "flash decays only with dt");
  AssertNear(astro::ship::shieldFlashDecay(1.0f, 1.0f), 0.049787f, 0.001f,
             "flash decay exp(-3)");

  // Test 15 (v3.6.0): орбитальный ассист — окно скоростей и стабилизация
  // к круговой орбите вокруг массивного центра в начале координат.
  {
    const astro::Vec3 pos(10.0, 0.0, 0.0);
    const astro::Vec3 g(-0.5, 0.0, 0.0);  // a * r = 5 → vCirc ≈ 2.236
    const double vCirc = 2.23606797749979;
    // Круговое курсирование: ассист активен, курс не меняется.
    astro::Vec3 in(0.0, vCirc, 0.0), dr;
    if (!astro::ship::orbitAssist(pos, g, in, dr, 0.016)) {
      std::cerr << "orbit assist must engage at circular speed\n";
      return 1;
    }
    const double vx1 = in.x + dr.x, vy1 = in.y + dr.y;
    AssertNear(static_cast<float>(vx1), 0.0f, 0.001f, "circular keeps radial");
    AssertNear(static_cast<float>(vy1), static_cast<float>(vCirc), 0.001f,
               "circular keeps tangential");
    // Падение радиально (vr=0.8, vt=2.2): радиальная скорость гасится,
    // тангенциальная подтягивается к круговой.
    astro::Vec3 in2(0.8, 2.2, 0.0), dr2;
    if (!astro::ship::orbitAssist(pos, g, in2, dr2, 0.5)) {
      std::cerr << "orbit assist must engage on shallow infall\n";
      return 1;
    }
    const double kAdj = 1.0 - std::exp(-0.5 * 1.1);
    AssertNear(static_cast<float>(in2.x + dr2.x),
               static_cast<float>(0.8 * (1.0 - kAdj)), 0.001f,
               "radial drift damped");
    AssertNear(static_cast<float>(in2.y + dr2.y),
               static_cast<float>(2.2 + (vCirc - 2.2) * kAdj), 0.001f,
               "tangential pulled to circular");
    // Слишком медленно (vt < 0.55 vCirc) — ассист молчит.
    astro::Vec3 in3(0.0, 1.0, 0.0), dr3;
    if (astro::ship::orbitAssist(pos, g, in3, dr3, 0.016)) {
      std::cerr << "orbit assist must not catch slow drifts\n";
      return 1;
    }
    // Слишком быстро (vt > 2.2 vCirc) — пролёт не перехватывается.
    astro::Vec3 in4(0.0, 6.0, 0.0), dr4;
    if (astro::ship::orbitAssist(pos, g, in4, dr4, 0.016)) {
      std::cerr << "orbit assist must not catch fast flybys\n";
      return 1;
    }
    // Радиальное доминирование — не орбита, а прямой ход.
    astro::Vec3 in5(3.0, 1.5, 0.0), dr5;
    if (astro::ship::orbitAssist(pos, g, in5, dr5, 0.016)) {
      std::cerr << "orbit assist must ignore radial-dominant motion\n";
      return 1;
    }
    // Без гравитации ассист выключен.
    astro::Vec3 in6(0.0, 2.2, 0.0), dr6;
    if (astro::ship::orbitAssist(pos, astro::Vec3(), in6, dr6, 0.016)) {
      std::cerr << "orbit assist must stay off without gravity\n";
      return 1;
    }
  }

  std::cout << "All BlackHoleInteraction tests passed!\n";
  return 0;
}
