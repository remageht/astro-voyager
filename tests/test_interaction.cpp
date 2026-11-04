#include "../src/Catalog.h"  // v3.6.0: distance/ETA helpers
#include "../src/Interaction.h"
#include "../src/Types.h"  // v3.3.0: ShipState / astro::ship balance logic
#include "../src/WarpJump.h"  // v3.6.0: asteroid barrage in Travel (pass D)

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
  // v3.8.0: урон обломков тоже масштабируется сложностью.
  AssertNear(easy.hazardK, 0.6f, 0.0001f, "easy hazard");
  AssertNear(normal.hazardK, 1.0f, 0.0001f, "normal hazard");
  AssertNear(hard.hazardK, 1.6f, 0.0001f, "hard hazard");
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

  // Test 8c (v3.6.0, v4.0.0): варп-роли корпусов. Варп-двигатель несут только
  // Ring Ship (1) и Model OBJ (3); Dart (0) и Shuttle (2) — досветовые.
  if (astro::ship::styleCanWarp(0) || !astro::ship::styleCanWarp(1) ||
      astro::ship::styleCanWarp(2) || !astro::ship::styleCanWarp(3)) {
    std::cerr << "only Ring Ship and Model OBJ must be warp-capable\n";
    return 1;
  }
  // v4.0.0: у досветовых корпусов форсаж заметно слабее варп-корпусов.
  if (!(astro::ship::styleBoostSpeedMult(0) <
            astro::ship::kBoostSpeedMultiplier) ||
      !(astro::ship::styleBoostSpeedMult(2) <
            astro::ship::kBoostSpeedMultiplier) ||
      astro::ship::styleBoostSpeedMult(1) !=
          astro::ship::kBoostSpeedMultiplier ||
      astro::ship::styleBoostSpeedMult(3) !=
          astro::ship::kBoostSpeedMultiplier) {
    std::cerr << "sublight hulls must have a capped boost\n";
    return 1;
  }
  // v4.0.0: корпус изнашивается от радиации и чинится в доке.
  astro::ShipState hullShip;
  astro::ship::applyRadiation(hullShip, 2.0f, true, 1.0f);
  if (hullShip.hull >= 100.0f || hullShip.hull <= 0.0f) {
    std::cerr << "radiation must wear the hull without killing it\n";
    return 1;
  }
  astro::ship::repairHull(hullShip);
  if (hullShip.hull != 100.0f) {
    std::cerr << "repairHull must restore the hull\n";
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
  // Test 12b (v4.0): зум колёсиком — zoomK масштабирует снос (кокпит всегда
  // identity), дефолт-аргументы обоих видов выше уже покрыты.
  const astro::Vec3 chaseIn = astro::view::offsetCameraPos(
      camPos, camFwd, astro::view::ViewMode::Chase, 0.35f);
  AssertNear(static_cast<float>(chaseIn.z), 1.425f, 0.0001f, "chase zoom-in");
  AssertNear(static_cast<float>(chaseIn.y), 2.6125f, 0.0001f, "chase zoom-in up");
  const astro::Vec3 farOut = astro::view::offsetCameraPos(
      camPos, camFwd, astro::view::ViewMode::Overview, 2.0f);
  AssertNear(static_cast<float>(farOut.z), -15.0f, 0.0001f, "overview zoom-out");
  AssertNear(static_cast<float>(farOut.y), 6.05f, 0.0001f, "overview zoom-out up");
  AssertNear(static_cast<float>(astro::view::offsetCameraPos(
                 camPos, camFwd, astro::view::ViewMode::Cockpit, 2.0f)
                 .z),
             3.0f, 0.0001f, "zoom must not touch cockpit");

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
    // Радиальный крейсер (vt < 0.55 vCirc): ассист держится в стороне.
    astro::Vec3 in6(0.0, 2.2, 0.0), dr6;
    if (astro::ship::orbitAssist(pos, astro::Vec3(), in6, dr6, 0.016)) {
      std::cerr << "orbit assist must stay off without gravity\n";
      return 1;
    }
  }

  // Test 16 (v3.6.0, пасс D): астероидное обломочное поле в Travel —
  // детерминированный ролл, уклонение по накоплению, единичный Hit.
  {
    using astro::JumpPhase;
    using astro::JumpEvent;
    using astro::WarpJump;
    // Скаляр 1. не ниже порога 0.25... порог строго частный: equal – нет поля.
    {
      WarpJump jN;
      jN.start("m13", 0.25f);
      if (jN.asteroidField()) {
        std::cerr << "asteroid roll == chance must NOT spawn a field\n";
        return 1;
      }
      jN.start("m13", 0.9f);
      if (jN.asteroidField()) {
        std::cerr << "asteroid roll 0.9 must stay field-free\n";
        return 1;
      }
      bool enter = false, douge = false, hit = false;
      for (int i = 0; i < 200; ++i) {
        if (!jN.active()) break;
        const JumpEvent e = jN.update(0.25f);
        enter = enter || e.asteroidEnter;
        douge = douge || e.asteroidDodged;
        hit = hit || e.asteroidHit;
      }
      if (enter || douge || hit) {
        std::cerr << "field-free jump must emit zero asteroid events\n";
        return 1;
      }
    }
    // Поле + непрерывный манёвр: Dodged есть, Hit нет.
    {
      WarpJump jY;
      jY.start("m13", 0.0f);
      if (!jY.asteroidField()) {
        std::cerr << "asteroid roll 0.0 must spawn a field\n";
        return 1;
      }
      bool enter = false, dodge = false, hit = false;
      for (int i = 0; i < 400; ++i) {
        if (!jY.active()) break;
        jY.setDodgeAxis(1.0f);
        const JumpEvent e = jY.update(0.125f);
        enter = enter || e.asteroidEnter;
        dodge = dodge || e.asteroidDodged;
        hit = hit || e.asteroidHit;
      }
      if (!enter || !dodge || hit) {
        std::cerr << "dodged jump: expects enter+dodge and no hit\n";
        return 1;
      }
    }
    // Поле без манёвра: ровно один Hit после окна опасности, без Dodged.
    {
      WarpJump jH;
      jH.start("m13", 0.0f);
      bool enter = false;
      int hits = 0, dodges = 0;
      for (int i = 0; i < 400; ++i) {
        if (!jH.active()) break;
        jH.setDodgeAxis(0.0f);
        const JumpEvent e = jH.update(0.125f);
        enter = enter || e.asteroidEnter;
        dodges += e.asteroidDodged ? 1 : 0;
        hits += e.asteroidHit ? 1 : 0;
      }
      if (!enter || dodges != 0 || hits != 1) {
        std::cerr << "no-dodge jump: expects one hit and zero dodges\n";
        return 1;
      }
      jH.start("m13", 0.0f);  // новый прыжок с чистым состоянием
      int hits2 = 0;
      for (int i = 0; i < 20; ++i) {
        if (!jH.active()) break;
        const JumpEvent e = jH.update(0.125f);
        hits2 += e.asteroidHit ? 1 : 0;
      }
      if (hits2 != 0) {
        std::cerr << "restart must reset hit flags (charge is clean)\n";
        return 1;
      }
    }
    // Рваный манёвр (ввод через такт): накопление не дотягивает до 0.25с.
    {
      WarpJump jB;
      jB.start("m13", 0.0f);
      int hits = 0;
      float idle = 0.0f;
      for (int i = 0; i < 400 && jB.active(); ++i) {
        jB.setDodgeAxis(idle <= 0.0f ? 1.0f : 0.0f);
        const JumpEvent e = jB.update(0.125f);
        idle += 0.125f;
        if (idle >= 0.25f) idle = 0.0f;
        hits += e.asteroidHit ? 1 : 0;
      }
      if (hits != 1) {
        std::cerr << "irregular dodging must still end with exactly one hit\n";
        return 1;
      }
    }
  }

  // Test 20 (v4.6.2): Docking corridor kinematics & orbital co-movement (Issue #63)
  {
    astro::ShipState ship;
    ship.isDocked = true;
    ship.dockedStationId = "dock-sol";
    if (!ship.isDocked || ship.dockedStationId != "dock-sol") {
      std::cerr << "Test 20 failed: ship must retain docked state and station id\n";
      return 1;
    }

    // Bezier corridor trajectory test: B(s) = (1-s)^2 P0 + 2(1-s)s P1 + s^2 P2
    const astro::Vec3 p0(0.0, 0.0, 5.0);        // Initial approach position
    const astro::Vec3 p1(0.0, 0.0, 2.0);        // Corridor entrance waypoint
    const astro::Vec3 p2(0.0, 0.0, 0.22);       // Latched collar position

    auto bezier = [&](double s) -> astro::Vec3 {
      double oneMinus = 1.0 - s;
      double b0 = oneMinus * oneMinus;
      double b1 = 2.0 * oneMinus * s;
      double b2 = s * s;
      return astro::Vec3(b0 * p0.x + b1 * p1.x + b2 * p2.x,
                         b0 * p0.y + b1 * p1.y + b2 * p2.y,
                         b0 * p0.z + b1 * p1.z + b2 * p2.z);
    };

    astro::Vec3 start = bezier(0.0);
    astro::Vec3 end = bezier(1.0);
    astro::Vec3 mid = bezier(0.5);

    AssertNear(static_cast<float>(start.z), 5.0f, 0.0001f, "start point at s=0");
    AssertNear(static_cast<float>(end.z), 0.22f, 0.0001f, "end point at s=1");
    // Midpoint should be between p1 and p2: 0.25*5 + 0.5*2 + 0.25*0.22 = 1.25 + 1.0 + 0.055 = 2.305
    AssertNear(static_cast<float>(mid.z), 2.305f, 0.0001f, "bezier midpoint");

    // Orbital co-movement test: when station moves by delta, docked ship tracks delta exactly
    astro::Vec3 stPos1(100.0, 0.0, 50.0);
    astro::Vec3 stPos2(100.5, 0.2, 50.8);       // Earth advanced along orbit
    astro::Vec3 portNormal(0.0, 0.0, 1.0);
    astro::Vec3 dockedPos1(stPos1.x + portNormal.x * 0.22, stPos1.y + portNormal.y * 0.22, stPos1.z + portNormal.z * 0.22);
    astro::Vec3 dockedPos2(stPos2.x + portNormal.x * 0.22, stPos2.y + portNormal.y * 0.22, stPos2.z + portNormal.z * 0.22);

    astro::Vec3 deltaShip(dockedPos2.x - dockedPos1.x, dockedPos2.y - dockedPos1.y, dockedPos2.z - dockedPos1.z);
    astro::Vec3 deltaStation(stPos2.x - stPos1.x, stPos2.y - stPos1.y, stPos2.z - stPos1.z);

    AssertNear(static_cast<float>(deltaShip.x), static_cast<float>(deltaStation.x), 0.0001f, "ship delta X tracks station");
    AssertNear(static_cast<float>(deltaShip.y), static_cast<float>(deltaStation.y), 0.0001f, "ship delta Y tracks station");
    AssertNear(static_cast<float>(deltaShip.z), static_cast<float>(deltaStation.z), 0.0001f, "ship delta Z tracks station");
  }

  std::cout << "All BlackHoleInteraction tests passed!\n";
  return 0;
}
