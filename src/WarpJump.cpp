#include "WarpJump.h"

#include <cmath>

namespace astro {

void WarpJump::start(const std::string& target, float astRoll01) {
  if (target.empty()) {
    stop();
    return;
  }
  m_target = target;
  m_phase = JumpPhase::Charge;
  m_t = 0.0f;
  m_switched = false;
  // v3.6.0: пасс D — астероидное поле на этот прыжок (детерминированный
  // роул от вызывающего: тест / AppGL).
  m_astField = (astRoll01 < kAstChance);
  m_astEntered = false;
  m_astDodgedDone = false;
  m_astHitDone = false;
  m_astDodgeT = 0.0f;
  m_dodgeAxis = 0.0f;
}

void WarpJump::stop() {
  m_phase = JumpPhase::Idle;
  m_t = 0.0f;
  m_target.clear();
  m_switched = false;
  m_astField = false;
  m_astEntered = false;
  m_astDodgedDone = false;
  m_astHitDone = false;
  m_astDodgeT = 0.0f;
  m_dodgeAxis = 0.0f;
}

void WarpJump::setDodgeAxis(float axis) {
  if (axis < -1.0f) axis = -1.0f;
  if (axis > 1.0f) axis = 1.0f;
  m_dodgeAxis = axis;
}

float WarpJump::phaseDur(JumpPhase p) {
  switch (p) {
    case JumpPhase::Charge: return kChargeDur;
    case JumpPhase::Pullback: return kPullbackDur;
    case JumpPhase::Burst: return kBurstDur;
    case JumpPhase::Travel: return kTravelDur;
    case JumpPhase::Arrival: return kArrivalDur;
    case JumpPhase::Pushin: return kPushinDur;
    case JumpPhase::Cooldown: return kCooldownDur;
    case JumpPhase::Idle: return 0.0f;
  }
  return 0.0f;
}

float WarpJump::phaseT01() const {
  const float d = phaseDur(m_phase);
  if (d <= 0.0f) return 0.0f;
  float t = m_t / d;
  if (t < 0.0f) t = 0.0f;
  if (t > 1.0f) t = 1.0f;
  return t;
}

JumpEvent WarpJump::update(float dt) {
  JumpEvent ev;
  if (!active()) return ev;
  if (dt < 0.0f) dt = 0.0f;
  if (dt > 0.5f) dt = 0.5f;  // кламп против лагов/паузы: фазы не проскакивают
  if (m_target.empty()) {
    // Защитная ветвь: битого состояния быть не должно, но выход — всегда Idle.
    stop();
    return ev;
  }
  m_t += dt;
  switch (m_phase) {
    case JumpPhase::Idle:
      break;
    case JumpPhase::Charge:
      if (m_t >= kChargeDur) {
        m_phase = JumpPhase::Pullback;
        m_t = 0.0f;
      }
      break;
    case JumpPhase::Pullback:
      if (m_t >= kPullbackDur) {
        m_phase = JumpPhase::Burst;
        m_t = 0.0f;
      }
      break;
    case JumpPhase::Burst:
      // Пик вспышки — середина фазы: ровно одно событие смены станции.
      if (!m_switched && m_t >= kBurstDur * 0.5f) {
        m_switched = true;
        ev.switchStation = true;
      }
      if (m_t >= kBurstDur) {
        m_phase = JumpPhase::Travel;
        m_t = 0.0f;
      }
      break;
    case JumpPhase::Travel:
      // v3.6.0: полоса обломков в Travel (пасс D). Каждый астероидный
      // флаг выдаётся ровно один раз за пролёт; уклонение копится только
      // внутри окна опасности по суммарному времени удержания A/D.
      if (m_astField) {
        if (!m_astEntered && m_t >= kTravelDur * kAstEnterT) {
          m_astEntered = true;
          ev.asteroidEnter = true;
        }
        const float t01 = m_t / kTravelDur;
        const bool inDanger =
            (t01 >= kAstDangerStart) && (t01 <= kAstDangerEnd);
        if (inDanger && std::fabs(m_dodgeAxis) >= 0.5f) {
          m_astDodgeT += dt;
          if (!m_astDodgedDone && m_astDodgeT >= kAstDodgeDwell) {
            m_astDodgedDone = true;
            ev.asteroidDodged = true;
          }
        } else if (inDanger) {
          // Рваный ввод не копится: в поле нужно вести уклонение непрерывно.
          m_astDodgeT = 0.0f;
        }
        if (!m_astDodgedDone && !m_astHitDone && t01 > kAstDangerEnd) {
          m_astHitDone = true;
          ev.asteroidHit = true;
        }
      }
      if (m_t >= kTravelDur) {
        m_phase = JumpPhase::Arrival;
        m_t = 0.0f;
      }
      break;
    case JumpPhase::Arrival:
      if (m_t >= kArrivalDur) {
        m_phase = JumpPhase::Pushin;
        m_t = 0.0f;
      }
      break;
    case JumpPhase::Pushin:
      if (m_t >= kPushinDur) {
        m_phase = JumpPhase::Cooldown;
        m_t = 0.0f;
      }
      break;
    case JumpPhase::Cooldown:
      if (m_t >= kCooldownDur) {
        ev.finished = true;
        stop();  // конец sequence: всегда в Idle, повторов и циклов нет
      }
      break;
  }
  return ev;
}

float WarpJump::pullK(float t01) {
  if (t01 < 0.0f) t01 = 0.0f;
  if (t01 > 1.0f) t01 = 1.0f;
  return t01 * t01 * (3.0f - 2.0f * t01);  // smoothstep ease in-out 0->1
}

float WarpJump::burstEnv(float t01) {
  if (t01 < 0.0f) t01 = 0.0f;
  if (t01 > 1.0f) t01 = 1.0f;
  // Синус-пик: 0 на краях, ровно 1 в середине (там же switchStation).
  // flash > 0.8 держится ~0.2с из 0.5с — белый экран прячет телепорт-поп.
  static const float kPi = 3.14159265358979323846f;
  return std::sin(kPi * t01);
}

float WarpJump::travelK(float /*t01*/) {
  // Ровная полка: в Travel туннель уже на максимуме (пришли из Burst).
  return 1.0f;
}

float WarpJump::pushK(float t01) {
  // Зеркало отъезда: плавный возврат TP->FP.
  return pullK(t01);
}

}  // namespace astro
