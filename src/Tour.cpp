#include "Tour.h"

#include <cmath>

namespace astro {

void Tour::start(const std::vector<std::string>& ids) {
  if (ids.empty()) {
    stop();
    return;
  }
  m_ids = ids;
  m_index = 0;
  m_current.clear();
  m_phase = TourPhase::FadeOut;
  m_t = 0.0f;
}

void Tour::stop() {
  m_phase = TourPhase::Idle;
  m_t = 0.0f;
  m_current.clear();
  m_ids.clear();
  m_index = 0;
}

void Tour::skip() {
  if (!active()) return;
  // Индекс строго растёт и ограничен размером списка: зациклиться негде.
  // На последней станции пропуск = финиш (Idle, вуаль 0).
  if (m_index + 1 >= m_ids.size()) {
    stop();
    return;
  }
  ++m_index;
  m_phase = TourPhase::FadeOut;
  m_t = 0.0f;
}

float Tour::fadeAlpha() const {
  if (m_phase == TourPhase::FadeOut) {
    float a = m_t / kFadeDur;
    if (a < 0.0f) a = 0.0f;
    if (a > 1.0f) a = 1.0f;
    return a;
  }
  if (m_phase == TourPhase::FadeIn) {
    float a = 1.0f - m_t / kFadeDur;
    if (a < 0.0f) a = 0.0f;
    if (a > 1.0f) a = 1.0f;
    return a;
  }
  return 0.0f;
}

TourEvent Tour::update(float dt) {
  TourEvent ev;
  if (!active()) return ev;
  if (dt < 0.0f) dt = 0.0f;
  if (dt > 0.5f) dt = 0.5f;  // кламп против лагов/паузы: фазы не проскакивают
  if (m_ids.empty() || m_index >= m_ids.size()) {
    // Защитная ветвь: битого состояния быть не должно, но выход — всегда Idle.
    stop();
    return ev;
  }
  m_t += dt;
  switch (m_phase) {
    case TourPhase::Idle:
      break;
    case TourPhase::FadeOut:
      if (m_t >= kFadeDur) {
        // Единственное место смены станции: ровно одно событие за переход.
        m_current = m_ids[m_index];
        ev.stationChanged = true;
        ev.stationId = m_current;
        m_phase = TourPhase::FadeIn;
        m_t = 0.0f;
      }
      break;
    case TourPhase::FadeIn:
      if (m_t >= kFadeDur) {
        m_phase = TourPhase::Dwell;
        m_t = 0.0f;
      }
      break;
    case TourPhase::Dwell:
      if (m_t >= kDwellDur) {
        if (m_index + 1 >= m_ids.size()) {
          ev.finished = true;
          stop();
        } else {
          ++m_index;
          m_phase = TourPhase::FadeOut;
          m_t = 0.0f;
        }
      }
      break;
  }
  return ev;
}

void Tour::dwellOffsets(float t, double& dx, double& dy,
                        double& dYaw, double& dPitch) {
  // Медленные периоды (6-11с) и малые амплитуды: камера плывёт, а не дёргается.
  // Позиционные амплитуды в мировых единицах; вызывающая сторона масштабирует
  // их под дистанцию (k = clamp(baseDist / 20, 0.4, 3.0)).
  const double kPi = 3.14159265358979323846;
  const double td = static_cast<double>(t < 0.0f ? 0.0f : t);
  dx = 0.55 * std::sin(2.0 * kPi * td / 11.0);
  dy = 0.32 * std::sin(2.0 * kPi * td / 7.0 + 1.3);
  dYaw = 0.045 * std::sin(2.0 * kPi * td / 9.0 + 0.5);
  dPitch = 0.028 * std::sin(2.0 * kPi * td / 6.0 + 0.7);
}

}  // namespace astro
