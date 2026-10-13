#pragma once

// Кинотур: автопилот-экскурсия по станциям (агент C).
// Чистая логика без GL/ImGui: время, фазы и переходы живут здесь,
// камеру и отрисовку ведёт AppGL. Харнесс скриншотов тур не видит:
// по умолчанию тур неактивен, все оверлеи рисуются только из
// AppGL::drawCinematic(), который captureAllScreenshots не вызывает.

#include <cstddef>
#include <string>
#include <vector>

namespace astro {

enum class TourPhase {
  Idle,     // тур выключен (дефолт; скриншоты чистые)
  FadeOut,  // затемнение 0.5с перед сменой станции
  FadeIn,   // проявление 0.5с после смены станции
  Dwell     // медленный облёт станции 8с (lissajous-дрейф)
};

struct TourEvent {
  bool stationChanged = false;  // в этом update пора сменить станцию
  std::string stationId;        // id станции для переключения
  bool finished = false;        // тур завершён (последняя станция облетана)
};

class Tour {
 public:
  static constexpr float kFadeDur = 0.5f;   // затемнение/проявление, с
  static constexpr float kDwellDur = 8.0f;  // облёт каждой станции, с

  Tour() = default;

  // Старт по списку id (порядок каталога). Пустой список = остаться в Idle.
  void start(const std::vector<std::string>& ids);
  // Мгновенный выход (ESC): гасит тур и вуаль, зацикливаний нет.
  void stop();
  // Пропуск (кнопка/пробел): следующая станция; на последней — финиш.
  // Индекс строго растёт, повторов и циклов нет.
  void skip();

  bool active() const { return m_phase != TourPhase::Idle; }
  TourPhase phase() const { return m_phase; }
  size_t index() const { return m_index; }
  size_t total() const { return m_ids.size(); }
  float phaseTime() const { return m_t; }
  const std::string& currentId() const { return m_current; }

  // 0..1, чёрная вуаль: FadeOut 0->1, FadeIn 1->0, иначе 0.
  float fadeAlpha() const;

  // Продвинуть state machine на dt секунд (dt клампится в [0, 0.5]).
  // Idle = no-op. Смена станции возвращается ровно одним событием.
  TourEvent update(float dt);

  // Медленный lissajous-дрейф камеры для Dwell.
  // t — время в фазе (с). Вызывающая сторона может масштабировать
  // dx/dy под дистанцию базы; углы уже в радианах и малы всегда.
  static void dwellOffsets(float t, double& dx, double& dy,
                           double& dYaw, double& dPitch);

 private:
  std::vector<std::string> m_ids;
  std::string m_current;  // последняя показанная станция (пусто до 1-й смены)
  size_t m_index = 0;     // предстоящая (FadeOut) или текущая станция
  TourPhase m_phase = TourPhase::Idle;
  float m_t = 0.0f;
};

}  // namespace astro
